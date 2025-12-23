#include "stdafx.h"
#include "websocket.h"
#include "conversation.h"

/**
 * Constructor for the CWebSocket class.
 *
 * Initializes a new WebSocket connection wrapper with the provided Mongoose connection.
 * Sets up the timer thread for periodic flushing of the send queue.
 *
 * @param mongooseConnection Pointer to the Mongoose connection structure.
 * @param mgr Pointer to the Mongoose manager structure.
 */
CWebSocket::CWebSocket(mg_connection* mongooseConnection, mg_mgr* mgr)
    : mMongooseConnection(mongooseConnection),
    mPipe(nullptr),
    mWasClosed(false),
    mSendCounter(0),
    mTimerRunning(false),
    mKeepTimerRunning(true) {

    if (mgr) {
        // Create a pipe for inter-thread communication
        mPipe = mg_mkpipe(mgr, CWebSocket::pipeEventHandler, this);
        if (mPipe) {
            // Set a meaningful label for debugging
            snprintf(mPipe->label, sizeof(mPipe->label), "ws_pipe_%lu", mongooseConnection->id);
        }
    }

    // Start the timer thread for periodic flushing
    mTimerThread = std::thread(&CWebSocket::timer, this);
}

/**
 * Destructor for the CWebSocket class.
 *
 * Ensures clean shutdown of the WebSocket connection by stopping the timer thread
 * and closing the connection.
 */
CWebSocket::~CWebSocket() {

    if (mMongooseConnection && mMongooseConnection->fn_data)
    {
        //delete mMongooseConnection->fn_data; check this
        //mMongooseConnection->fn_data = nullptr;
    }
    // Signal the timer thread to stop
    setKeepTimerRunning(false);

    // Wait for the timer thread to complete
    if (mTimerThread.joinable()) {
        mTimerThread.join();
    }

    // Ensure connection is closed
    close();
}

/**
 * Retrieves the IP address of the peer connected via the WebSocket.
 *
 * Uses the Mongoose utility function to get a readable IP address.
 *
 * @return The IP address of the peer as a string. Returns an empty string if the Mongoose connection is null.
 */
std::string CWebSocket::getIPAddress() {
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (!mMongooseConnection) {
        return "";
    }

    char addr[100] = { 0 };
    mg_ntoa(&mMongooseConnection->peer, addr, sizeof(addr));
    return std::string(addr);
}

/**
 * Sets the event handler callback for the Mongoose connection.
 *
 * @param handler The event handler callback to be set.
 * @param holder A pointer to the convHolder structure, which will be set as the user data for the connection.
 */
void CWebSocket::setOnEventCallback(mg_event_handler_t handler, convHolder* holder) {
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (mWasClosed || !mMongooseConnection) {
        return;
    }

    mMongooseConnection->fn = handler;
    mMongooseConnection->fn_data = holder;
}

/**
 * Gets the state of the keep timer running flag.
 *
 * @return True if the timer should continue running, false otherwise.
 */
bool CWebSocket::getKeepTimerRunning() {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mKeepTimerRunning;
}

/**
 * Sets the keep timer running flag.
 *
 * @param doIt Whether the timer should continue running.
 */
void CWebSocket::setKeepTimerRunning(bool doIt) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    mKeepTimerRunning = doIt;
}

/**
 * Gets the state of the timer running flag.
 *
 * @return True if the timer is currently running, false otherwise.
 */
bool CWebSocket::getIsTimerRunning() {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mTimerRunning;
}

/**
 * Sets the timer running flag.
 *
 * @param doIt Whether the timer is currently running.
 */
void CWebSocket::setIsTimerRunning(bool doIt) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    mTimerRunning = doIt;
}

/**
 * Sends binary data over the WebSocket connection.
 *
 * @param bytes The binary data to be sent.
 * @return The number of bytes queued for sending.
 */
size_t CWebSocket::sendBytes(std::vector<uint8_t> bytes) {
    return send(std::move(bytes), WEBSOCKET_OP_BINARY);
}

/**
 * Sends text data over the WebSocket connection.
 *
 * @param txt The text data to be sent.
 * @return The number of bytes queued for sending.
 */
size_t CWebSocket::sendText(std::string txt) {
    return send(std::vector<uint8_t>(txt.begin(), txt.end()), WEBSOCKET_OP_TEXT);
}

/**
 * Internal method to send data over the WebSocket connection.
 *
 * Queues the data to be sent and signals the timer to flush if enough data has accumulated.
 *
 * @param data The data to be sent.
 * @param op The WebSocket operation code (WEBSOCKET_OP_BINARY, WEBSOCKET_OP_TEXT, etc.)
 * @return The number of bytes queued for sending.
 */
size_t CWebSocket::send(std::vector<uint8_t> data, int op) {
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (!mMongooseConnection || mWasClosed || data.empty()) {
        return 0;
    }

    size_t dataSize = data.size();

    // Add to send queue
    mSendQueue.push({ std::move(data), op });

    // Increment counter and signal event loop if needed
    incSendCounter();
    if (mSendCounter > 3 && mPipe) {
        // Wake up the event loop to process the queue
        // CRITICAL: Do NOT call flush() directly here - that would cause
        // mg_ws_send() to be called from this worker thread while the
        // mongoose event loop may be simultaneously accessing c->send buffer.
        // The pipeEventHandler will call flush() in the event loop thread.
        mg_mgr_wakeup(mPipe);
    }

    return dataSize;
}

/**
 * Processes the send queue and sends data via Mongoose WebSocket.
 *
 * THREADING MODEL:
 * This function is ONLY called from the main event loop thread via
 * pipeEventHandler, ensuring single-threaded access to c->send buffer.
 * Worker threads add data to mSendQueue and call mg_mgr_wakeup() to
 * signal the event loop, which then calls flush() in the correct thread.
 *
 * CRITICAL: Never call mg_ws_send() from outside the event loop thread!
 * A mutex cannot protect against this because mongoose's event loop
 * doesn't use our mutex when it accesses c->send buffer.
 */
void CWebSocket::flush() {
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (mWasClosed || !mMongooseConnection) {
        return;
    }

    // Make a local copy of the connection pointer
    auto connectionCopy = mMongooseConnection;

    // Process all items in the queue
    // THREADING: This function is ONLY called from pipeEventHandler() which runs
    // in the mongoose event loop thread. This ensures single-threaded access to
    // c->send buffer, eliminating the race condition that caused heap corruption.
    while (!mSendQueue.empty() && connectionCopy && !mWasClosed){
        if (!mMongooseConnection || mWasClosed) {
            // Connection was closed during processing
            return;
        }

        // Get the next item to send
        auto& item = mSendQueue.front();

        // Send via mongoose WebSocket API
        // Safe because we're in the event loop thread (no mutex needed)
        int sent = mg_ws_send(connectionCopy,
            reinterpret_cast<const char*>(item.first.data()),
            item.first.size(),
            item.second);

        if (sent <= 0) {
            // Error in sending, mark connection for closing
            LOG(LL_ERROR, ("WebSocket send error, closing connection %lu", mMongooseConnection->id));

            if (mMongooseConnection) {
                mMongooseConnection->is_closing = true;
            }

            if (mPipe) {
                mPipe->is_closing = true;
            }

            mWasClosed = true;
            return;
        }

        // Successfully sent, remove from queue
        mSendQueue.pop();
    }
}

/**
 * Timer thread function that periodically signals the event loop to flush.
 *
 * CRITICAL FIX: Does NOT call flush() directly to avoid race conditions.
 * Instead, wakes up the Mongoose event loop which will call flush() in the
 * main event loop thread, ensuring all buffer access is single-threaded.
 *
 * Runs in a separate thread until signaled to stop via the mKeepTimerRunning flag.
 */
void CWebSocket::timer() {
    auto lastFlush = std::chrono::high_resolution_clock::now();
    const auto flushInterval = std::chrono::milliseconds(100);

    setIsTimerRunning(true);

    while (getKeepTimerRunning()) {
        // Sleep to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFlush);

        // Signal event loop to flush periodically
        if (elapsed >= flushInterval) {
            resetSendCounter();

            // CRITICAL FIX: Wake up the event loop instead of calling flush() directly
            // This ensures all sends happen in the main event loop thread, eliminating
            // race conditions on c->send buffer access
            {
                std::lock_guard<std::recursive_mutex> lock(mGuardian);
                if (mPipe && !mWasClosed) {
                    // Wake up the Mongoose event loop
                    // The pipeEventHandler will receive MG_EV_READ and call flush()
                    mg_mgr_wakeup(mPipe);
                }
            }

            lastFlush = now;
        }
    }

    setIsTimerRunning(false);
}


void CWebSocket::incSendCounter()
{
    std::lock_guard<std::mutex>  lock(mFieldsGuardian);
    mSendCounter++ ;
}

void CWebSocket::resetSendCounter()
{
    std::lock_guard<std::mutex>  lock(mFieldsGuardian);
    mSendCounter = 0;
}

/**
 * Closes the WebSocket connection.
 *
 * Marks the connection as closing and releases resources.
 *
 * CRITICAL: Does NOT send WebSocket close frame directly because that would
 * call mg_ws_send() from a worker thread. Instead, it sets the is_closing flag
 * which mongoose will handle in the event loop thread.
 */
void CWebSocket::close() {
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (mWasClosed) {
        return;
    }

    // Mark as closed before releasing resources to prevent concurrent access
    mWasClosed = true;

    // CRITICAL FIX: Do NOT call mg_ws_send() here - it would access c->send
    // buffer from a worker thread while the event loop may be using it.
    // Instead, just set the is_closing flag and let mongoose handle it.
    // The connection will be closed without a clean WebSocket close handshake,
    // but this is safer than risking heap corruption.

    // Set the is_closing flag on the connection - mongoose will close it
    if (mMongooseConnection) {
        mMongooseConnection->is_closing = true;
        mMongooseConnection = nullptr;
    }

    // Close the pipe connection
    if (mPipe) {
        mPipe->is_closing = true;
        mPipe = nullptr;
    }
}

/**
 * Event handler for the pipe connection.
 *
 * CRITICAL: This runs in the MAIN EVENT LOOP THREAD, not the timer thread.
 * When the timer thread calls mg_mgr_wakeup(mPipe), this handler receives
 * MG_EV_READ event in the main event loop thread and processes the send queue.
 *
 * This architecture ensures ALL buffer access (c->send) happens in a single
 * thread (the event loop thread), eliminating race conditions.
 *
 * @param c Pointer to the Mongoose connection structure (the pipe).
 * @param ev The event that triggered the handler (MG_EV_READ from wakeup).
 * @param ev_data Event-specific data.
 * @param fn_data User-defined function data (pointer to the CWebSocket instance).
 */
void CWebSocket::pipeEventHandler(struct mg_connection* c, int ev, void* ev_data, void* fn_data) {
    if (ev == MG_EV_READ) {
        CWebSocket* self = static_cast<CWebSocket*>(fn_data);
        if (self) {
            // Clear the pipe's receive buffer
            if (c) {
                c->recv.len = 0;
            }

            // Flush the send queue IN THE MAIN EVENT LOOP THREAD
            // This ensures all mg_ws_send() calls happen in the same thread
            // that Mongoose uses for socket I/O, preventing race conditions
            self->flush();
        }
    }
}

/**
 * Checks if the WebSocket connection is active and writable.
 *
 * @return True if the connection is active and writable, false otherwise.
 */
bool CWebSocket::isActive() {
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    return mMongooseConnection &&
        mMongooseConnection->is_writable &&
        !mWasClosed;
}

/**
 * Sets the conversation object associated with this WebSocket.
 *
 * @param conv Shared pointer to the associated CConversation object.
 */
void CWebSocket::setConversation(std::shared_ptr<CConversation> conv) {
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mConversation = conv;
}