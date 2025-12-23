#pragma once
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include "mongoose/mongoose.h"

class CConversation;

struct convHolder {
    std::weak_ptr<CConversation> conv;
};

class CWebSocket {
public:
    // Constructor and destructor
    CWebSocket(mg_connection* mongooseConnection, mg_mgr* mgr);
    ~CWebSocket();

    // Public API methods (unchanged as requested)
    size_t sendBytes(std::vector<uint8_t> bytes);
    size_t sendText(std::string txt);
    void close();
    void flush();

    std::string getIPAddress();
    void setOnEventCallback(mg_event_handler_t handler, convHolder* holder);

    bool getKeepTimerRunning();
    void setKeepTimerRunning(bool doIt = true);
    bool getIsTimerRunning();
    void setIsTimerRunning(bool doIt);

    bool isActive();
    void setConversation(std::shared_ptr<CConversation> conv);

    // Timer method
    void timer();

    void incSendCounter();

    void resetSendCounter();

    // Static Mongoose event handler
    static void pipeEventHandler(struct mg_connection* c, int ev, void* ev_data, void* fn_data);

private:
    // Mongoose connection management
    struct mg_connection* mMongooseConnection;
    struct mg_connection* mPipe;

    // State management
    bool mWasClosed;
    std::weak_ptr<CConversation> mConversation;

    // Thread and timer management
    std::thread mTimerThread;
    bool mTimerRunning;
    bool mKeepTimerRunning;

    // Send queue management
    std::queue<std::pair<std::vector<uint8_t>, int>> mSendQueue;
    uint64_t mSendCounter;

    // Locks
    std::recursive_mutex mGuardian;      // Main connection lock
    std::mutex mFieldsGuardian;          // State variables lock
    // NOTE: mMongooseSendMutex removed - it cannot protect against mongoose's
    // event loop thread. The fix is to ONLY call mg_ws_send() from the event
    // loop thread (via pipeEventHandler), not to use a mutex.

    // Internal sending method
    size_t send(std::vector<uint8_t> data, int op);
};