//[todo:CodesInChaos:medium]: analyse mutex usage. It's all been stable but somehow I'm not sure.
#pragma once
#include "conversation.h"
#include "NetworkManager.h"
#include "BlockchainManager.h"
#include "Settings.h"
#include "Verifiable.h"
#include "NetworkManager.h"
#include "KademliaServer.h"
#include <chrono>
#include "ThreadPool.h"
#include "StatusBarHub.h"
#include <future>
#include <thread>
#ifndef WIN32
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#endif

#include "Block.h"
#include "BlockHeader.h"
#include "Verifiable.h"
#include "udt/src/udt.h"

#include "NetMsg.h"
#include "conversationState.h"
#include "CGlobalSecSettings.h"
#include "EEndPoint.h"
#include <iostream>
#include <chrono>
#include "NetTask.h"
#include "DTIServer.h"
#include "DTI.h"
#include "cqrintentresponse.h"
#include "ScriptEngine.h"
#include "Receipt.h"
#include "CVMMetaParser.h"
#include "VMMetaSection.h"
#include "VMMetaEntry.h"
#include "VMMetaGenerator.h"
#include "SDPEntity.h"
#include "WebSocketServer.h"
#include "webRTCSwarm.h"
#include "CStateDomainManager.h"
#include "sessionDescription.h"
#include "CryptoFactory.h"
#include "OperationResult.h"
#include "Settings.h"
#include "DataRouter.h"
#include "CCertificate.h"
#include "websocket.h"
#include "ChatMsg.h"
#include "IdentityToken.h"
#include "ConnTracker.h"
#include "QUICConversationsServer.h"
#include "UDTConversationsServer.h"
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif


#define SANDBOX_ERG_LIMIT 100000000000
#define TOLERATE_TX_ERRORS  2
std::mutex  CConversation::sIDGenGuardian;
 uint64_t CConversation::sLastID=1;
 using namespace std::chrono_literals;



 void CConversation::pingMechanicsThread(std::shared_ptr<CConversation> self)
 {
	 // Lifetime Signaling - BEGIN
	 {
		 setIsPingThreadAlive(true);
		 auto sharedThis = shared_from_this();
	 }

	 struct ActivitySignal {

		 std::shared_ptr<CConversation> mSharedPtr;

		
		 ActivitySignal(std::shared_ptr<CConversation> sharedPtr)
			 : mSharedPtr(sharedPtr) {}

		 ~ActivitySignal() {
			 std::lock_guard<std::recursive_mutex> lock(mSharedPtr->mDestructionGuardian);
			 // Perform cleanup
			 mSharedPtr->setIsPingThreadAlive(false);
		 }

	 };

	 ActivitySignal exitSignal(shared_from_this());
	 // Lifetime Signaling - END


	 // Security - BEGIN
	 if (getPreventStartup())
	 {
		 return;
	 }
	 // Security - END

	 {
		 std::lock_guard<std::mutex> lock(mFieldsGuardian);
		 if (mPingMechanicsEnabled)
		 {
			 mFieldsGuardian.unlock();
			 return;
		 }
		 mPingMechanicsEnabled = true;
	 }

	
	 // Local Variables - BEGIN
	 std::shared_ptr<CNetMsg> msg;
	 std::shared_ptr<CTools> tools = getTools();
	 tools->SetThreadName("Keep-Alive Thread");
	 size_t now = tools->getTime();
	 std::shared_ptr<CNetTask> currentTask;
	 uint64_t lastPingSent = 0;
	 bool sessionKeyAvailable = false;
	 bool abort = false;
	 eTransportType::eTransportType protocol = getProtocol();
	 //Local Variables - END

	 while (getAreKeepAliveMechanicsToBeActive())
	 {
		 Sleep(100);

		 
		 lastPingSent = getLastTimePingSent();
		 now = tools->getTime();
	
		 sessionKeyAvailable = getSessionKey().size()>0;

		 //refresh - BEGIN
		 eConversationState::eConversationState state = getState()->getCurrentState();
		 if (state == eConversationState::ending || state == eConversationState::ended || state == eConversationState::unableToConnect)
			 return;

		 // if (!sessionKeyAvailable || state == eConversationState::connecting || state == eConversationState::initial
		 //	 || state == eConversationState::initializing ||  state == eConversationState::unableToConnect)
		 // {
		 if (state != eConversationState::running)
			 continue;

		 if (getIsHelloStage())
		 {
			 continue;
		 }
		 //}


		 currentTask = getCurrentTask(false); //do not dequeue the current task.

		 //refresh - END

		 if ( !getCeaseCommunication() && (currentTask == nullptr || (currentTask->getType() != eNetTaskType::endConversation)))
		 {
			 //Keep-Alive Mechanics - BEGIN

			 //outgress ping - BEGIN
			 switch (protocol)
			 {
			 case eTransportType::SSH:
				 break;
			 case eTransportType::WebSocket:
				 if ((now - lastPingSent) >= CGlobalSecSettings::getSendWebsocketPingAfter())
				 {
					 msg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::ping, eNetReqType::notify);
					 //note: these datagrams are not enqueued,- these are being dispatched right away instead.
					 if (sendNetMsg(msg, true))
					 {
						 pingSent();
					 }
				 }
				 break;
			 case eTransportType::UDT:
				 if ((now - lastPingSent) >= CGlobalSecSettings::getSendUDTPingAfter())
				 {
					 msg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::ping, eNetReqType::notify);

					 if (sendNetMsg(msg, true))
					 {
						 pingSent();
					 }
				 }
				 break;

			 case eTransportType::QUIC:
				 if ((now - lastPingSent) >= CGlobalSecSettings::getSendQUICPingAfter())
				 {
					 msg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::ping, eNetReqType::notify);

					 if (sendNetMsg(msg, true))
					 {
						 pingSent();
					 }
				 }
				 break;
			 case eTransportType::local:
				 break;
			 default:
				 break;
			 }

		 }
	 }
 }


 std::string CConversation::checkUDTError(int optRes) {
	 if (optRes == UDT::ERROR) {
		 std::stringstream errMsg;
		 errMsg << "UDT Error: " << UDT::getlasterror().getErrorMessage();
		 return errMsg.str();
	 }
	 return "";
 }

 std::shared_ptr<CQUICConversationsServer> CConversation::getQUICServer()
 {
	 std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 return mQUICServer;
 }

 /**
 * @brief Shuts down the QUIC connection associated with this conversation.
 *
 * This method is used to shut down the QUIC connection. It can perform either an abrupt
 * or a graceful shutdown based on the provided argument. An abrupt shutdown is typically used
 * in scenarios like handling a DDoS attack or other urgent situations where immediate disconnection
 * is necessary. In contrast, a graceful shutdown is used during normal operation closures.
 *
 * @param abruptly A boolean flag indicating whether the shutdown should be abrupt or graceful.
 *                 If true, the connection is terminated immediately using QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT.
 *                 If false, a normal shutdown procedure is initiated.
 *
 * @note This function checks if the connection handle is valid before attempting to shut it down.
 *       After the shutdown, it's safe to release or reset the connection handle.
 *
 * Usage Example:
 *     CConversation conversation;
 *     // ... setup and use conversation ...
 *     conversation.shutdownQUICConnection(true); // Abrupt shutdown
 *     // or
 *     conversation.shutdownQUICConnection(false); // Graceful shutdown
 */
 void CConversation::shutdownQUICConnection(bool abrutply) {
	 
	 std::lock_guard<std::mutex> lock(mQUICShutdownGuardian);

	 conversationFlags flags = getFlags();
	 std::shared_ptr<CTools> tools = getTools();
	 const QUIC_API_TABLE* MsQuic = getQUIC();
	 HQUIC connectionHandle = getQUICConnectionHandle(); // Retrieve the connection handle
	 bool logDebug = getLogDebugData();

	 // Local Variables - TRUE


	 // Pre-Flight - BEGIN
	 if (flags.QUICConnectionClosed || flags.QUICConnectionShutDown)
	 {
		 return;
	 }
	 if (getQUICShutdownInitiated())
	 {
		 return;
	 }
	 // Pre-Flight - END
	 

	 // Operational Logic - BEGIN
	 setQUICShutdownInitiated(true);

	 logDebug = true;//todo:remove
	 if (connectionHandle != nullptr) {
		 if (abrutply) {
			 // Use the QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT to terminate the connection abruptly
			 if (!getQUICShutdownInitiated() && !flags.QUICConnectionShutDown) {
				
				 flags.QUICConnectionShutDown = true;
				 setFlags(flags);
				 MsQuic->ConnectionShutdown(connectionHandle, QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT, 0);
				 /*The QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT flag is used in the
					MsQuic API during a connection shutdown operation to indicate that the
					connection should be closed without sending a closing frame to the peer. This
					flag is specified in a call to ConnectionShutdown to instruct the library to
					terminate the connection locally without notifying the remote endpoint of the
					shutdown reason.
				 */
				
			 }
			 /*
			 auto flags = getFlags();
			 if (flags.QUICConnectionClosed == false)
			 {
				 // Note: while the closing operation is inherently asynchronous as soon as ConnectionClose() returns
				 //       the connection handle needs to be assumed as INVALID.
				 //	      Consecutive events MAY fire, though.

				 flags.QUICConnectionClosed = true;
				 setFlags(flags);
				 MsQuic->ConnectionClose(connectionHandle);// In case of an abrupt abort - clean-up connection handle without awaiting further events.
			 }
			 */

		 }
		 else {
			 if (!flags.QUICConnectionShutDown)
			 {
				 flags.QUICConnectionShutDown = true;
				 setFlags(flags);
				 MsQuic->ConnectionShutdown(connectionHandle, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
				 flags.QUICConnectionShutDown = true;
			 }
			 // We would wait for QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE before releasing connectionHandle.
		 }
	 }



	 // Operational Logic - END
	 
	 // Debug Logging
	 if (logDebug) {
		 std::string logMsg = "QUIC connection shutdown initiated for IP: " + tools->getColoredString(getIPAddress(), eColor::blue) +
			 ". [Abruptly]: " + (abrutply ? "Yes" : "No");

		 tools->logEvent(logMsg, eLogEntryCategory::network, 1);
	 }


 }

 void CConversation::setLocalIsMobile(bool isIt)
 {
	 std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mFlags.localIsMobile = isIt;
 }


/// <summary>
/// The processing lasts for the duration of an UDT connection.
/// CNetMsgs are exchanged and processed.
/// Each CNetMsg is BER encoded and thus the data bundle boundaries are preserved.
/// When receiving the function first downloads and looks at the BER header and knows how many bytes to read further.
/// When sending we know the size of data to send by looking at the size of the serialized Netmsg->
/// </summary>
/// <param name="socket"></param>
 void CConversation::connectionHandlerThread(std::shared_ptr<CConversation> self,UDTSOCKET comSocket, std::string IPAddress, std::weak_ptr<CWebSocket> webSocket) {
	 
	 if (getState()->getCurrentState() == eConversationState::ended || !getIsScheduledToStart())
		 return; //do not allow for resuscitation
	 getTools()->setThreadPriority(ThreadPriority::HIGH);
	 // Lifetime Signaling - BEGIN
	 { 
		 setIsThreadAlive(true);
		 setIsScheduledToStart(false);
		 auto sharedThis = shared_from_this();
	 } 
	
	 struct ActivitySignal {
		 std::shared_ptr<CConversation> mSharedPtr;
		 std::shared_ptr <CNetworkManager> mNetworkManager;
		 ActivitySignal(std::shared_ptr<CConversation> sharedPtr, 
			 std::shared_ptr <CNetworkManager> nm)
			 : mSharedPtr(sharedPtr), mNetworkManager(nm){}

		 ~ActivitySignal() {
			 // Perform cleanup
 			std::lock_guard<std::recursive_mutex> lock(mSharedPtr->mDestructionGuardian);

			 mSharedPtr->closeSocket();
			 mSharedPtr->setIsScheduledToStart(false);
			 mSharedPtr->cleanTasks();
			 mSharedPtr->setIsThreadAlive(false);
			 mSharedPtr->setIsPingThreadAlive(false);

			 if (mSharedPtr->getState()->getCurrentState() != eConversationState::unableToConnect) {
				 mSharedPtr->getState()->setCurrentState(eConversationState::ended);
			 }

		 }

	 };


	 ActivitySignal exitSignal(shared_from_this(), getNetworkManager());
	// Lifetime Signaling - END


	// Security - BEGIN
	if (getPreventStartup())
	{
		return;
	}
	// Security - END

	try {

		std::shared_ptr<CTools> tools = getTools();
		tools->SetThreadName("Conversation Thread (active)");

		//System checks - BEGIN
		if (!(CGlobalSecSettings::getWarnPeerAboutPendingShutdownAfter() <
			CGlobalSecSettings::getGentleConversationShutdownAfter()
			< CGlobalSecSettings::getForcedConversationShutdownAfter()))
		{
			tools->logEvent("[CRITICAL Error]: Invalid system-wide conversation-timeouts configured.", eLogEntryCategory::localSystem, 3, eLogEntryType::failure);
		
			return;
		}

		//System checks - END

		//Local Variables - BEGIN
		size_t now = tools->getTime();
		bool wasConnecting = false;
		int timeout = static_cast<int>(CGlobalSecSettings::getMaxClientSocketTimeout());
		
		std::shared_ptr<CNetTask> newTask;
		size_t start = 0;
		std::shared_ptr<CNetTask> currentTask;

		if (getProtocol() == eTransportType::UDT)
		{
			mRXBuffer = std::vector<uint8_t>(CGlobalSecSettings::getMaxUDTNetworkPackageSize() + 10000);//UDT uses some required padding.
		}
		bool communicationErrored = false;
		//tasks are only local, from other parties are received only atomic intents/packages
		UDT::ERRORINFO errorInfo;
		size_t interPingTimeDiff = 0;
		int rsize = 0;
		std::shared_ptr<CBlock> block;
		std::shared_ptr<CBlockHeader> blockHeader;

		uint64_t lastValitNetMsgAt = std::time(0);
		//CNetMsg msg;
		std::shared_ptr<CNetMsg> msg;
		uint64_t leaderNotificationEvery = 5;//send our leader notification at least every X sec.
		bool logDebug = getLogDebugData();

		
		bool aboutToShutDownSent = false;
		bool peerNotified = false;
		bool switchedToAsyncReception = false;
		bool leaderRequested = false;
		bool taskDequeued = false;
		//uint64_t lastPingSent = 0;
		bool exitReasonReported = false;
		std::shared_ptr<CBlock> lastBlockNotificationAbout;
		std::shared_ptr<CConversation> conv;
		uint64_t rtEntriedRemoved = 0;
	
		int64_t connectTimeout_ms = 20000; // Define a timeout of 5 seconds for selectEx to wait on the socket operation
		auto flags = getFlags();
		bool socketRecvSync = false;
		bool socketSendSync = true;
		uint64_t dataTransitErrorThresholdExceededCount = 0;

		std::shared_ptr<CNetworkManager> nm = getNetworkManager();
		std::shared_ptr<CQUICConversationsServer>  quicServer = getQUICServer();

		bool canEventLogThisIP = nm->canEventLogAboutIP(getIPAddress());

		//Local Variables - END
		
		//Operation Logic - BEGIN

		// [Rationale]: kick-start the session negotiation protocol v------
		newTask = std::make_shared<CNetTask>(eNetTaskType::startConversation, 9);//highest priority 'hello' task gonna be the first one;only 'end'-conversation task has a higher priority.
		addTask(newTask);//enqueue it

		//await secure session if secure session required
		if (getIsEncryptionRequired())
		{//proceeding to other scheduled tasks won't be possible, until data allowing for encryption is made available.
			//Note: processing of ANY  incoming netmsgs would entirely   possible. 
			//It's just that the Tasks queue would remained blocked by the
			//requirement of EITHER session-key OR other peer's public key becoming available.

			// [Note]: if a task instructing for the Conversation to be shut-down appears, any pending blocking tasks
			//		  would be ABORTED. this includes both 'startConversation' and 'awaitSecureSession'.

			// In terms of outgoing CNetMsg containers:
			/*
			* - processing of asynchronous sendNetMsg() calls would BLOCK.
			* - synchronous sendNetMsg() calls would be processed In-Line.
			*/
			//[Rationale]: defer any any user-mode networking tasks   v------   until secure session established.
			newTask = std::make_shared<CNetTask>(eNetTaskType::awaitSecureSession, 3);// higher priority than any normal task
			addTask(newTask);// [WARNING]: no other task but for eNetTaskType::endConversation would be processed until secure session is established.
		}

		//Establish conversation if instructed to do so (client==nullptr && IPAddress provided)

		//----- BEGIN

		if (IPAddress.empty() == false)
		{
			canEventLogThisIP = nm->canEventLogAboutIP(IPAddress);
		}
		if (getProtocol() == eTransportType::QUIC) {
			HQUIC newConnection = nullptr;
			getState()->ping();

			if (flags.isIncoming == false)
			{//outgoing connection attempt


				if (IPAddress.empty() || !tools->validateIPv4(IPAddress))
				{
					if (canEventLogThisIP)
					{
						tools->logEvent("[QUIC " + tools->timeToString(std::time(0)) + "]: no valid IPv4 provided to an egress QUIC connection.", eLogEntryCategory::network, 5, eLogEntryType::failure, eColor::lightPink);
					}
					return;
				}
				const QUIC_API_TABLE* MsQuic = quicServer->getQUIC();

				// Initialize a new QUIC connection for outgoing communication
			
				quicServer->initializeQUICConfiguration(false, mConfiguration, geAllowedStreamCount());
				QUIC_STATUS Status = QUIC_STATUS_ABORTED;
				
				{
					std::lock_guard<std::mutex> lock(mFieldsGuardian);
					Status = MsQuic->ConnectionOpen(mQUICRegistration, CConversation::ConnectionCallbackTrampoline, this, &newConnection);
				}

				// Store the connection handle for further use, e.g., in the CConversation instance or within of callbacks.
				setQUICConnectionHandle(newConnection);

				if (QUIC_FAILED(Status)) {
					// Handle error in opening connection
					if(canEventLogThisIP)
					tools->logEvent("[QUIC " + tools->timeToString(std::time(0)) + "]: Unable to initialize QUIC connection attempt with " + IPAddress + ".", eLogEntryCategory::network, 5, eLogEntryType::failure, eColor::lightPink);
					return;
				}

				// Set the IP address and port for the connection
				QUIC_ADDR Addr = {};
				Addr.Ipv4.sin_family = AF_INET;
				int result = inet_pton(AF_INET, IPAddress.c_str(), &Addr.Ipv4.sin_addr); // Set the IPv4 address

				if (result != 1) {
					// Handle the error: result is 0 if IPAddress is not a valid IP address format,
					// or -1 if AF_INET does not match the address family in IPAddress
					if (newConnection)
					{
						auto flags = getFlags();
						if (flags.QUICConnectionClosed == false)
						{
							// Note: while the closing operation is inherently asynchronous as soon as ConnectionClose() returns
							//       the connection handle needs to be assumed as INVALID.
							 //	      Consecutive events MAY fire, though.
							flags.QUICConnectionClosed = true;
							setFlags(flags);
							MsQuic->ConnectionClose(newConnection);// free the newConnection object

							{
								std::lock_guard<std::mutex> lock(mFieldsGuardian);
								mSocketWasFreed = true;
							}
							
						}
					}

					getState()->setCurrentState(eConversationState::eConversationState::ended);
					if(canEventLogThisIP)
					tools->logEvent("[QUIC " + tools->timeToString(std::time(0)) + "]: Unable to initialize QUIC connection attempt with " + IPAddress + ".", eLogEntryCategory::network, 5, eLogEntryType::failure, eColor::lightPink);
					return;

				}
				uint16_t port = CGlobalSecSettings::getDefaultPortNrDataExchange(getBlockchainManager()->getMode(), true);
				QuicAddrSetPort(&Addr, port); // Set the port number

				// Start the connection process
				Status = MsQuic->ConnectionStart(newConnection, mConfiguration, QUIC_ADDRESS_FAMILY_INET, IPAddress.c_str(), port);
				wasConnecting = true;
				if (QUIC_FAILED(Status)) {

					// Handle error in starting connection
					if (newConnection)
					{

						if (flags.QUICConnectionClosed == false)
						{
							// Note: while the closing operation is inherently asynchronous as soon as ConnectionClose() returns
							//       the connection handle needs to be assumed as INVALID.
							//	      Consecutive events MAY fire, though.
							
							flags.QUICConnectionClosed = true;
							setFlags(flags);

							MsQuic->ConnectionClose(newConnection);// In case of an abrupt abort - clean-up connection handle without awaiting further events.

							{
								std::lock_guard<std::mutex> lock(mFieldsGuardian);
								mSocketWasFreed = true;
							}
						}
					}
					if (canEventLogThisIP)
					tools->logEvent("[QUIC " + tools->timeToString(std::time(0)) + "]: Unable to initialize QUIC connection attempt with " + IPAddress + ".", eLogEntryCategory::network, 5, eLogEntryType::failure, eColor::lightPink);

					return;
				}

				// At this point, the connection process has begun. The completion will be asynchronous,
				// and further actions should be handled in the ConnectionCallback, through a trampoline.
			}
			else
			{
				// Handling for incoming connection

				newConnection = getQUICConnectionHandle();

				if (newConnection == nullptr) {
					tools->logEvent("[QUIC" + tools->timeToString(std::time(0)) + "]: Incoming connection handle is null.", eLogEntryCategory::network, 5, eLogEntryType::failure);
					return;
				}
			}
			
		}
		else if (getProtocol() == eTransportType::UDT)
		{

			if (comSocket == 0 && IPAddress.size() > 0)
			{
				comSocket = UDT::socket(AF_INET, SOCK_DGRAM, 0);
				setUDTSocket(comSocket);// IMPORTANT: set it first as all the management function rely on it;
				//otherwise we might end up with zombie-threads within the UDT UDT library
				//do not set earlier as it might be a server socket with socket value already set
				getState()->setCurrentState(eConversationState::connecting);
				sockaddr_in serv_addr;
				serv_addr.sin_family = AF_INET;
				serv_addr.sin_port = htons(CGlobalSecSettings::getDefaultPortNrDataExchange(getBlockchainManager()->getMode()));
				inet_pton(AF_INET, IPAddress.c_str(), &serv_addr.sin_addr);
				memset(&(serv_addr.sin_zero), '\0', 8);
				int timeout = static_cast<int>(CGlobalSecSettings::getMaxClientSocketTimeout());
				int optRes = 0;
				std::string errorMsg;
				optRes = UDT::setsockopt(comSocket, 0, UDT_RCVTIMEO, &timeout, sizeof(timeout));
				if ((errorMsg = checkUDTError(optRes)) != "")
					assertGN(false);

				// Repeat for other options
				int maxUDTSendTimeout = 180000;
				optRes = UDT::setsockopt(comSocket, 0, UDT_SNDTIMEO, &maxUDTSendTimeout, sizeof(maxUDTSendTimeout));
				if ((errorMsg = checkUDTError(optRes)) != "") 
					assertGN(false);

				int maxRecvBuffer = CGlobalSecSettings::getMaxUDTNetworkPackageSize()+10000;//UDT uses some required padding.
				int maxSendBuffer = 2 * CGlobalSecSettings::getMaxUDTNetworkPackageSize();

				if (flags.isIncoming == false)
				{
					// buffer size for incoming connections is inherited from server socket. 
					// Note: these properties CANNOT be set after a socket is 'bound'.

					optRes = UDT::setsockopt(comSocket, 0, UDT_RCVBUF, &maxRecvBuffer, sizeof(maxRecvBuffer));
					if ((errorMsg = checkUDTError(optRes)) != "")
						assertGN(false);

					optRes = UDT::setsockopt(comSocket, 0, UDT_SNDBUF, &maxSendBuffer, sizeof(maxSendBuffer));
					if ((errorMsg = checkUDTError(optRes)) != "")
						assertGN(false);
				}

				

				optRes = UDT::setsockopt(comSocket, 0, UDT_RCVSYN, &socketRecvSync, sizeof(socketRecvSync));
				if ((errorMsg = checkUDTError(optRes)) != "")
					assertGN(false);

				optRes = UDT::setsockopt(comSocket, 0, UDT_SNDSYN, &socketSendSync, sizeof(socketSendSync));
				if ((errorMsg = checkUDTError(optRes)) != "")
					assertGN(false);

				//IMPORTANT: the above ensures that socket is acting in NON BLOCKING MODE even BEFORE the connection is attempted.
				if (canEventLogThisIP)
				{
					tools->logEvent("[UDT " + tools->timeToString(std::time(0)) + "] connecting to " + IPAddress, eLogEntryCategory::network, 2, eLogEntryType::notification);
				}
				// connect to other node, implicit bind

				// Prepare a list of UDT sockets to be monitored by selectEx
				//std::vector<UDTSOCKET> writefds;
				//writefds.push_back(comSocket);
				int eid = UDT::epoll_create();//docs advise to use epoll instead

				UDT::epoll_add_usock(eid, comSocket);

				// Try to establish a connection using UDT. The result will help us determine if it's a non-blocking attempt or an immediate connection
				int connectionResult = UDT::connect(comSocket, (sockaddr*)&serv_addr, sizeof(serv_addr));

				
				if (connectionResult == UDT::ERROR) {
					int err = UDT::getlasterror().getErrorCode();
					if (err == CUDTException::EASYNCRCV) {
						if (canEventLogThisIP)
						{
							// The connection attempt has started in non-blocking mode and is in progress.
							tools->logEvent("[UDT]: Connection attempt started in non-blocking mode with " + IPAddress, eLogEntryCategory::network, 2, eLogEntryType::notification);
						}
					}
					else {
						UDT::epoll_remove_usock(eid, comSocket);
						UDT::epoll_release(eid);

						closeSocket();
						// Some other error occurred, we handle and log it.
						if (canEventLogThisIP)
						tools->logEvent("[UDT " + tools->timeToString(std::time(0)) + "]: Unable to connect with " + IPAddress + ". Error: " + UDT::getlasterror().getErrorMessage(), eLogEntryCategory::network, 5, eLogEntryType::failure, eColor::cyborgBlood);
						getState()->setCurrentState(eConversationState::eConversationState::unableToConnect);
						return;
					}
				}
				else {
					// The connection was immediately successful. UPDATE:NO!
					///tools->logEvent("[UDT]: Connection established immediately with " + IPAddress, eLogEntryCategory::network, 2, eLogEntryType::notification, eColor::lightGreen);
					wasConnecting = true;
					// Additional logic for established connections can be added here.
				}

				// Use selectEx to wait and monitor the socket's state. 
				/*
				Here's how it works:

				UDT::selectEx() is provided with a list of socket descriptors (writefds in this case).
				It waits until the sockets are ready for writing, an error occurs on them, or the timeout (timeout_ms) is reached.
				If a socket becomes ready for writing (i.e., a non-blocking connect operation has completed successfully), it remains in the writefds list. Otherwise, if an error occurs or if the operation has not yet completed, it is removed from the list.
				After UDT::selectEx() returns, the code checks how many sockets are still ready for writing by examining the return value (ready). If the number is <= 0, then either there was an error, or the operation timed out.
				The code further checks if the specific socket (comSocket) is still present in the writefds list using std::find(). If it's not found, this means the connection attempt for that socket failed or hasn't completed yet.
				*/

				// Here, selectEx waits up to the specified timeout for the socket to become ready for writing or until an error occurs.
				//int ready = UDT::selectEx(writefds, nullptr, &writefds, nullptr, connectTimeout_ms);

				/*
				if (ready <= 0 || std::find(writefds.begin(), writefds.end(), comSocket) == writefds.end())
				{
					// The connection attempt either timed out or failed.
					tools->logEvent("[UDT]: unable to connect with " + IPAddress, eLogEntryCategory::network, 5, eLogEntryType::failure, eColor::cyborgBlood);
					getState()->setCurrentState(eConversationState::eConversationState::unableToConnect);
					closeSocket();
					setAreKeepAliveMechanicsToBeActive(false);
					setIsThreadAlive(false);
					return;
				}*/

				std::set<UDTSOCKET> writefds;
				int result = UDT::epoll_wait(eid, nullptr, &writefds, connectTimeout_ms);

				if (result < 0 || writefds.find(comSocket) == writefds.end()) {
					// Connection attempt failed or timed out.
					// The connection attempt either timed out or failed.

					if(canEventLogThisIP)
					tools->logEvent("[UDT" + tools->timeToString(std::time(0)) + "] unable to connect with " + IPAddress, eLogEntryCategory::network, 1, eLogEntryType::failure, eColor::lightPink);
					UDT::epoll_remove_usock(eid, comSocket);
					UDT::epoll_release(eid);
					closeSocket();

					getState()->setCurrentState(eConversationState::eConversationState::unableToConnect);
					return;
				}
				else {
					// Connection is ready.
				}

				UDT::epoll_release(eid);


				// If we've reached this point, the connection was successful.
				if (canEventLogThisIP)
				tools->logEvent("[UDT" + tools->timeToString(std::time(0)) + "]: connected with " + IPAddress, eLogEntryCategory::network, 2, eLogEntryType::notification, eColor::lightGreen);
				wasConnecting = true;

			}
			if (comSocket == 0)
			{
				return;
			}


			//----- END

			//handle edge-case (buggy) scenarios
			//----- BEGIN
			if (comSocket == 0 && IPAddress.size() == 0)
			{
				return;
			}
			// -----/END
		}

		else
		{
			setWebSocket(webSocket.lock());

			if (getWebSocket() == nullptr)
			{//instantiate websock-connection

				if (IPAddress.size() == 0)
				{
					tools->logEvent("Invalid web-socket, peer anonymous.", eLogEntryCategory::network, 2, eLogEntryType::failure);
	
					return;
				}
				else
				{

				}
			}

		}

		//Main part of the conversation  - BEGIN
		if (canEventLogThisIP)
		tools->logEvent((!wasConnecting? tools->getColoredString("Ingress ", eColor::orange): tools->getColoredString("Outgress ", eColor::none)) +
			tools->transportTypeToString(getProtocol()) + " conversation" + " with " + getIPAddress() + tools->getColoredString(" has begun.", eColor::lightGreen), eLogEntryCategory::network, 1);
		//getBM()->tools->writeLine("Conversation has begun.", true, true, eViewState::unspecified,"Conversation");
		//CConversationState state;


		if (getProtocol() == eTransportType::UDT)
		{
			int optRes = 0;
			std::string errorMsg;
		
			/*
			* NOTICE: these options CANNOT be set now since socket has been BOUND already.
			* More we do not want SYNCHRONICITY *AT ALL* since there's a separate property instructing UDT
			* whether datagrams are to be arriving IN ORDER and that's what we DO want.
			* 
			* 
			optRes = UDT::setsockopt(comSocket, 0, UDT_RCVSYN, &socketRecvSync, sizeof(socketRecvSync));
			if ((errorMsg = checkUDTError(optRes)) != "") 
				assertGN(false);

			optRes = UDT::setsockopt(comSocket, 0, UDT_SNDSYN, &socketSendSync, sizeof(socketSendSync));
			if ((errorMsg = checkUDTError(optRes)) != "") 
				assertGN(false);
			*/

			// Verify Buffers - BEGIN
			
			// Get RX buffer size
			int optionSize = 0;
			if (UDT::ERROR == UDT::getsockopt(comSocket, 0, UDT_RCVBUF, &mRXBufferSize, &optionSize)) {
				//	std::cerr << "Failed to get RX buffer size: " << UDT::getlasterror().getErrorMessage() << std::endl;
				mRXBufferSize = 0;
			}


			// Get TX buffer size
			if (UDT::ERROR == UDT::getsockopt(comSocket, 0, UDT_SNDBUF, &mTXBufferSize, &optionSize)) {
				//std::cerr << "Failed to get TX buffer size: " << UDT::getlasterror().getErrorMessage() << std::endl;
				mTXBufferSize = 0;
			}

			/*
			IMPORTANT: we cannot assume the returned buffer's dimensions are  expected since UDT library employs
			DYNAMIC allocation for both of these buffers while the values returned are momentary values not ceiling ones.

			if (!(mTXBufferSize >= CGlobalSecSettings::getMaxUDTNetworkPackageSize() && mRXBufferSize >= CGlobalSecSettings::getMaxUDTNetworkPackageSize()))
			{
				assertGN(false);
			}
			*/

			// Verify Buffers - END
		}

		start = tools->getTime();


		

	
		getState()->ping();

		eTransportType::eTransportType protocol = getProtocol();

		//[todo: discuss on 07.02.23] do we really need separate timeout tracking within the comm thread itself?
		//the clean-up procedure takes care of this anyway.

		uint64_t loopStartedAt = tools->getTime();
		uint64_t lastPingSent = 0;

		if (getProtocol() != eTransportType::QUIC || flags.isIncoming)
		{
			// [v incoming connection v] 
			// These are marked as 'running' right away since there's no formation stage.

			// [v outcoing conncetions v]

			// [ QUIC ]
			// QUIC connections are asynchronous and so is connection formation
			//for outgoing connections we mark them as 'running' when a 'connected' event fires.

			// [ UDT ]
			// UDT connection formations are asyncrhonous as well.
			// yet with UDT we use ePoll to block and wait during the initial stage above thus all UDT connections marked as 'running' here.

			if (getState()->getCurrentState() != eConversationState::eConversationState::ending
				&& getState()->getCurrentState() != eConversationState::eConversationState::ended)
			{
				getState()->setCurrentState(eConversationState::running);
			}
		}

		while (true)//when the 2nd timeout expires the connection is shut down forcefully
			//on 1st timeout we attempt to attempt a gentle negotiated disconnection
		{

		//Loop exit conditions - BEGIN
		/*Rationale: let us leave processing of loop-interruption conditions by the very end
		* so the conversation has a chance to notify the other peer (the Bye-task has highest priority, always).
		*
		* Conversation, on request, transitions into ::ending state.
		* The loop can exit ONLY after a conversation transitioned into ENDED state already.
		*
		*/

			if (getTransmissionErrorsCount() > TOLERATE_TX_ERRORS)
			{// THE CONVERSATION HAS ENDED. THAT"s it. All it remains to do is to exit the processing loop.

					//shouldn't be here during normal operation. The state should transition gently from 'ending' state.
				//if (logDebug)
				//{

				if(canEventLogThisIP)
				tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " excessive TX errors.", eLogEntryCategory::network, 0, eLogEntryType::notification);
				exitReasonReported = true;
				//}

				newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
				addTask(newTask);
			}

			/*
			WRONG:  the conversations transitions into ENDED only after the processing loop exited and the underlying socket was closed.

			if (getState()->getCurrentState() == eConversationState::eConversationState::ended)
			{
				if (logDebug)
				{
					//shouldn't be here during normal operation. The state should transition gently from 'ending' state.
					tools->logEvent("[Exiting Conversation Loop] for: " + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " state transited to 'ended'.", eLogEntryCategory::network, 1, eLogEntryType::notification);
				}
					exitReasonReported = true;


			}
			*/
			if (getState()->getCurrentState() == eConversationState::ending)
			{
				//All it remains to do is to exit the processing loop.
				//the other peer should be notified by now.
				//the ending tasks should have been processed right above due its highest priority.
				//just break the routine, the socket will be closed and THEN the state will be set as ENDED
			//	if (logDebug)
				//{

				if (canEventLogThisIP)
				tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + (getEndReason() == eConvEndReason::none ? "Gently exiting the processing loop as a result of 'ending' state-transition." : tools->convEndReasonToStr(getEndReason())), eLogEntryCategory::network, 0, eLogEntryType::notification);
				exitReasonReported = true;
				//}
				break; // THIS EFFECITEVELY EXISTS THE MAIN LOOP.
				// notice that state is updated only once the ActivitySignal gets out of scope (in its destructor).
			}

			if (interPingTimeDiff > CGlobalSecSettings::getForcedConversationShutdownAfter())
			{//should anything go wrong; should we not hear from the other peer for long enough - end the conversation.
				//if (logDebug)
				//{
				if (canEventLogThisIP)
				tools->logEvent("[Exiting Conversation Loop] for: " + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " the timeout for forceful termination expired. ", eLogEntryCategory::network, 1, eLogEntryType::failure);
				exitReasonReported = true;
				//}
				newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
				addTask(newTask);

			}

			//Loop exit conditions - END



			// Halt Processing - BEGIN
			
			if (getProtocol() == eTransportType::QUIC && getState()->getCurrentState() != eConversationState::running)
			{
				Sleep(3);
				continue;
			}
			// Halt Processing - END

			if (getState()->getCurrentState() == eConversationState::ended)
			{
				//todo: investigate this further. this should not happen.
				return;
			}

			// SECURITY - BEGIN
			if (getMisbehaviorsCount() > CGlobalSecSettings::getUDTDataTransitErrorWarningThreshold())
			{
				dataTransitErrorThresholdExceededCount++;

				if (dataTransitErrorThresholdExceededCount >= CGlobalSecSettings::getUDTDataTransitErrorMaxThresholdCrossings())
				{
					issueWarning();
					dataTransitErrorThresholdExceededCount = 0; // Reset the count if you want the warning to be reissued every 3 'exceedances'
				}
			}
			// Self Termination - BEGIN
			// Rationale: should the connections' monitoring sub-system become stuck for any reason.
			size_t secWinSince = getSecWindowSinceTimestamp();
			uint64_t warningsCount = getWarningsCount();
			uint64_t secWindowSinceTimestamp = getSecWindowSinceTimestamp();

			if (warningsCount)
			{
				if (warningsCount > CGlobalSecSettings::getMaxConversationWarningsAllowed())
				{
					newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
					addTask(newTask);
				}

				if ((now > secWindowSinceTimestamp) && ((now - secWindowSinceTimestamp) >= CGlobalSecSettings::getClearConversationWarningsAfter()))
				{
					clearWarnings();
				}
			}
			// Self Termination - END
		// SECURITY - END

			lastPingSent = getLastTimePingSent();
			Sleep(3);//let the others breath

			now = tools->getTime();

			//Reset Variables - BEGIN


			peerNotified = false;
			taskDequeued = false;
			flags = getFlags();
			msg = nullptr;
			if (getState()->getLastActivity() > tools->getTime())
				interPingTimeDiff = 0;
			else
				interPingTimeDiff = tools->getTime() - getState()->getLastActivity();

			currentTask = getCurrentTask();//assess the current Task
			//Reset Variables - END


			// Effective Synchronisation Toggling - BEGIN
			// as of the llatest revision - a node would PROPOSE to activate the Sync protocol by deliveirng a single Block Header.
			// only if the other node responds would the Synchronisation protocol be enabled.
			// Fresh nodes need to respond with a dummy Genesis Block.

			/*
			if (!flags.isMobileApp)
			{
				if (getSyncToBeActive())
				{
					if (getIsEncryptionAvailable())
					{
						setIsSyncEnabled(true); // render synchronisation as Operational.
					}
				}
			}
			else
			{
				setSyncToBeActive(false);
			}

			flags = getFlags();
			*/
			// Effective Synchronisation Toggling - END

			if (currentTask == nullptr || (currentTask->getType() != eNetTaskType::endConversation))
			{
				//Keep-Alive Mechanics - BEGIN

				//facilitated now through a separate thread.

				//Keep-Alive Mechanics  - END

		
			
					uint64_t timeout = getUDTTimeout();

					//Exit on no ping - BEGIN
					if ((loopStartedAt != now && (now - loopStartedAt) > 15) && ((now - getLastTimePingReceived()) >= timeout)
						&& ((now - lastValitNetMsgAt) > timeout))
					{
						//The main rationale here is that since UDT connections in DGRAM do not have a keep-alive mechanics, we need to know whenever the other peer 'closed' the connection without notifying us
						//and without having us to wait until the main timeouts, due to the lack of tasks being processes expire (~ 3 minutes).
						if (logDebug)
						{
							if (canEventLogThisIP)
							tools->logEvent(tools->getColoredString("[Exiting Conversation Loop] ", eColor::lightPink) + "for: " + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " no ping after threshold for a DSM-sync conversation. ", eLogEntryCategory::network, 0, eLogEntryType::failure);
							break;
							//exitReasonReported = true;
						}
						//newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
						//addTask(newTask);
						//break;
					}

					
					//Exit on no ping - END
					if (flags.isMobileApp == false)
					{
				   //DSM-sync support - BEGIN

					uint64_t lastHeaderOutTimestamp = getLastLeaderNotificationOutTimestamp();
					bool syncActive = getIsSyncEnabled();

					if (getSyncToBeActive())// initially assumed by defaults of Blockchain Manager. Important: contrast with getIsSyncEnabled().
					{
						if ((syncActive == false && lastHeaderOutTimestamp==0) // always propose the sub-protocol to be active
							|| syncActive && (now - lastHeaderOutTimestamp) > leaderNotificationEvery)// if both parties agreed for the sub-protocol to be active, then keep updating the other peer of local best known block.
						{

							block = mBlockchainManager->getCachedLeader();

							lastBlockNotificationAbout = block;

							if (block)
							{
								blockHeader = block->getHeader();
							}
							else
							{
								// simply send an empty datagram
							}

							if (blockHeader)
							{
								std::vector<uint8_t> headerBytes;
								if (blockHeader->getPackedData(headerBytes))
								{
									newTask = std::make_shared<CNetTask>(eNetTaskType::notifyBlock, 1);//notify about current leader
									newTask->setData(headerBytes);
									if (canEventLogThisIP)
										tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) + " scheduling a keep-alive leader notification to " + IPAddress, eLogEntryCategory::network, 0, eLogEntryType::notification,
											eColor::lightCyan);
									pingLastLeaderNotificaitonTimestamp();
									addTask(newTask);
									newTask = nullptr;
								}

							}
							else
							{
								//tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) + " no new block to notify about. " + IPAdrress, eLogEntryCategory::network, 1, eLogEntryType::notification);

								newTask = std::make_shared<CNetTask>(eNetTaskType::notifyBlock, 1);// simply send an empty datagram
								if (canEventLogThisIP)
									tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) + " telling " + IPAddress + " to enable block synchronization. ", eLogEntryCategory::network, 1, eLogEntryType::notification,
										eColor::lightCyan);
								pingLastLeaderNotificaitonTimestamp();
								addTask(newTask);
								newTask = nullptr;
							}


						}
					}
				}
				//DSM-sync support - END



				if (communicationErrored)
				{
					if (logDebug)
					{
						if (canEventLogThisIP)
						tools->logEvent("[Conversation] Ending conversation with: " + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " error during communication. Description: '" + std::string(UDT::getlasterror_desc()) + "'", eLogEntryCategory::network, 1, eLogEntryType::failure);
						exitReasonReported = true;
					}
					newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
					addTask(newTask);

				}



				if ((interPingTimeDiff > CGlobalSecSettings::getWarnPeerAboutPendingShutdownAfter()) && !aboutToShutDownSent)
				{
					if (logDebug &&canEventLogThisIP)
						tools->logEvent("[Conversation] Notifying peer that a connection ( " + getDescription() + " ) is about to shutdown due to inactivity in " + std::to_string(CGlobalSecSettings::getGentleConversationShutdownAfter() - CGlobalSecSettings::getWarnPeerAboutPendingShutdownAfter()) + " seconds.", eLogEntryCategory::network, 1);

					aboutToShutDownSent = true;
					sendNetMsgRAW(true, eNetEntType::VMStatus, eNetReqType::notify, tools->getSigBytesFromNumber(eVMStatus::aboutToShutDown), false);//do NOT update timestamps (false param) the conversation! that would delay the shut-down

					SE::vmFlags flags;
					notifyVMStatus(eVMStatus::aboutToShutDown, flags, 0, mScriptEngine != nullptr ? mScriptEngine->getID() : std::vector<uint8_t>(), std::vector<uint8_t>(), getID());

				}
				else
				{
					if (aboutToShutDownSent)
					{
						aboutToShutDownSent = false;
						sendNetMsgRAW(true, eNetEntType::VMStatus, eNetReqType::notify, tools->getSigBytesFromNumber(eVMStatus::ready));//ensure the other peer that the main thread is all good.
					}
				}

				if ((interPingTimeDiff > CGlobalSecSettings::getGentleConversationShutdownAfter()) && getState()->getCurrentState() != eConversationState::ending)
				{
					if (logDebug && canEventLogThisIP)
						tools->logEvent("[Conversation] Shutting down due to inactivity. Setting state to 'ending' and dispatching an 'end-conversation' task.. ( " + getDescription() + " ) ", eLogEntryCategory::network, 1);

					//attempt to gently end the conversation before the final timeout occurs
					getState()->setCurrentState(eConversationState::eConversationState::ending);
					newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
					addTask(newTask);
					getState()->ping();
				}

				//first - if connecting to another peer then let us process our task first (if it is not empty)
				//if being contacted by another peer -> then process task of the client first

			}
			if (currentTask != nullptr)
			{
				conv = currentTask->getConversation();


				eNetTaskProcessingResult::eNetTaskProcessingResult  result;
				if (currentTask->getState() != eNetTaskState::completed && currentTask->getState() != eNetTaskState::aborted)
				{
					result = processTask(currentTask, comSocket);
				}
				else
				{
					//in response to external state modification (connection being shut-down etc.)
					result = currentTask->getState() == eNetTaskProcessingResult::succeeded ? eNetTaskProcessingResult::succeeded : eNetTaskProcessingResult::aborted;
				}

				//Notify peer on progress - BEGIN
				if (conv != nullptr)
				{
					eOperationStatus::eOperationStatus stat;

					switch (result)
					{
					case eNetTaskProcessingResult::succeeded:
						stat = eOperationStatus::success;
						break;
					case eNetTaskProcessingResult::aborted:
						stat = eOperationStatus::failure;
						break;
					case eNetTaskProcessingResult::inProgress:
						stat = eOperationStatus::inProgress;
						break;
					case eNetTaskProcessingResult::error:
						stat = eOperationStatus::failure;
						break;
					default:
						break;
					}

					conv->notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit);
					//in case of an error/failure should be proceeded by a more concrete handler-specific notification
				}
				//Notify peer on progress - END

				if (isToEnd() || result == eNetTaskProcessingResult::aborted || result == eNetTaskProcessingResult::succeeded)// if the conversation is to end. Abort any pending tasks. Override their waiting.
				{
					dequeTask();//precessing of the network task already finished
					taskDequeued = true;
				}

				//Logging - BEGIN
				if (taskChanged())//to save on CPU cycles; note there's a separate duplicates detection mechanism within the logging function itself
					//here we save on preparation of the log-msg itself
				{
					if(canEventLogThisIP)
					tools->logEvent("NetTask " + currentTask->getDescription() + " [Result]:" + tools->netTaskResultToString(result)
						+ std::string(peerNotified ? " Peer notified." : " Peer NOT notified.") + std::string(taskDequeued ? " Task Dequeued." : "Task in Queue."), eLogEntryCategory::network, 0);
				}
				//Logging - END

			}

			//then, let us see what the other peer has to say - both parties should have sent hello Msgs by now
			if (getProtocol() == eTransportType::UDT && !getCeaseCommunication())
			{
				rsize = UDT::recvmsg(comSocket, reinterpret_cast<char*>(mRXBuffer.data()), mRXBuffer.size());

				if (rsize != UDT::ERROR)
				{
					if (!switchedToAsyncReception)
					{
						socketRecvSync = true;
						UDT::setsockopt(comSocket, 0, UDT_RCVSYN, &socketRecvSync, sizeof(socketRecvSync));//[WARNING] - receiving is ASYNCHRONOUS!
						switchedToAsyncReception = true;
					}
					totalNrOfBytesExchanged += rsize;//global counter

					if (logDebug && canEventLogThisIP)
						tools->logEvent("[UDT]: " + std::to_string(rsize) + " bytes received.", eLogEntryCategory::network, 0);
					getState()->ping();
					std::vector<uint8_t> receivedMsg(mRXBuffer.begin(), mRXBuffer.begin() + rsize);
					std::shared_ptr<CNetMsg> m = CNetMsg::instantiate(receivedMsg);

					if (m == nullptr)
					{
						if (logDebug && canEventLogThisIP)
							tools->logEvent("[UDT]: "+ tools->getColoredString("Invalid NetMsg received", eColor::lightPink)+".", eLogEntryCategory::network, 0);//debug-level notification

						if (incMisbehaviorCounter())// protocol incompatible data received
						{
							if(canEventLogThisIP)
							tools->logEvent("Peer " + getIPAddress() + " deemed as malicious. Aborting conversation.", "Security", eLogEntryCategory::network, 1, eLogEntryType::notification);
							endInternal(false, eConvEndReason::security);
							communicationErrored = true;
						}
					}
					else
					{
						
						lastValitNetMsgAt = std::time(0);
						if (canEventLogThisIP)
						tools->logEvent("[UDT] "+tools->getColoredString("Valid NetMsg received", eColor::lightGreen)+".", eLogEntryCategory::network, 0);
						if (m->getEntityType() == eNetEntType::bye)
							aboutToShutDownSent = false;
						enqueueNetMsg(m);
					}

				}
				else
				{
					errorInfo = UDT::getlasterror();
					int errCode = errorInfo.getErrorCode();
						if (errCode != CUDTException::EASYNCRCV)
						{
							if (canEventLogThisIP)
							tools->logEvent(tools->getColoredString("[RX UDT Error]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString("@" + getIPAddress(), eColor::blue) + tools->getColoredString(" Reason: ", eColor::orange) + errorInfo.getErrorMessage(), eLogEntryCategory::network, 0, eLogEntryType::failure);
						}
					if (errCode != CUDTException::ETIMEOUT && errCode != CUDTException::EASYNCRCV && errCode != CUDTException::EASYNCSND)
					{
						if (errCode == CUDTException::ECONNLOST)
						{
							setCeaseCommunication(true);
							newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
							addTask(newTask);
							if (canEventLogThisIP)
							tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " connection was broken.", eLogEntryCategory::network, 0, eLogEntryType::failure);

							//getNetworkManager()->banIP(getIPAddress());
							//tools->logEvent("Anti-DDOS mechanics triggered for " + getIPAddress() + ".", "Security", eLogEntryCategory::network, 1, eLogEntryType::warning);

						}
						else if (errCode == CUDTException::ECONNFAIL)
						{
							setCeaseCommunication(true);
							newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
							addTask(newTask);
							if (canEventLogThisIP)
							tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " connection establishment failed.", eLogEntryCategory::network, 0, eLogEntryType::failure);
						}

						else if (errCode == CUDTException::ECONNSETUP)
						{
							setCeaseCommunication(true);
							newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
							addTask(newTask);
							if (canEventLogThisIP)
							tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + "error during connection setup.", eLogEntryCategory::network, 0, eLogEntryType::failure);
						}
						else if (errCode == CUDTException::ESOCKFAIL)
						{
							setCeaseCommunication(true);
							newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
							addTask(newTask);
							if (canEventLogThisIP)
							tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + "couldn't configure UDP socket.", eLogEntryCategory::network, 0, eLogEntryType::failure);
						}
						else if (errorInfo.getErrorCode() == CUDTException::EINVSOCK)
						{
							setCeaseCommunication(true);
							std::shared_ptr<CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
							addTask(newTask);
							tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " invalid UDT socket.", eLogEntryCategory::network, 0, eLogEntryType::failure);
						}

						communicationErrored = true;
					}
					//there was an error in transmission
					//communicationErrored = true;
				}
			}
			else if (getProtocol() == eTransportType::WebSocket)
			{


			}
			else if (getProtocol() == eTransportType::QUIC)
			{


			}


			if (msg = getRecentMsg())
			{
				lastValitNetMsgAt = std::time(0);
				getState()->ping();
				dequeMsg();
				//do what there's to be done with the received msg reply/process with blockchain manager etc.
				if (getState()->getCurrentState() != eConversationState::ending)//if conversation is ending we're not interested
				{
					try {
						processMsg(msg, comSocket);
					}
					catch (const std::exception& e) {
						issueWarning();

						tools->logEvent("Conversation with " + IPAddress + " caused a fatal exception .", eLogEntryCategory::network, 3, eLogEntryType::failure);
					}
					catch (...) {
						issueWarning();
						tools->logEvent("Conversation with " + IPAddress + " caused a fatal exception .", eLogEntryCategory::network, 3,  eLogEntryType::failure);
					}
				}

				if (msg->getEntityType() == eNetEntType::bye)
					getState()->setCurrentState(eConversationState::eConversationState::ending);

			}



		}

		{
			std::lock_guard<std::mutex> lock(mPendingBlockBodyRequestsGuardian);
			mPendingBlockBodyRequests.clear();
		}

		rtEntriedRemoved = getNetworkManager()->getRouter()->removeEntriesWithNextHop(getAbstractEndpoint());//remove routing table entries which had this conversation set on path as an immediate hop.
		//(the data-path would be no longer reachable)

		//Operational Logic - END

		if (logDebug && canEventLogThisIP)
		{
			tools->logEvent("[Conversation Ended] Exited processing loop." + std::string(exitReasonReported ? "Reason reported." : "WARNING: The reason is *UNKNOWN*.") + getDescription()
				+ "\nRemoved " + std::to_string(rtEntriedRemoved) + " routing table entries as a result.", eLogEntryCategory::network, 0);
		}

		//Main part of the conversation  - END

		//let the main thread finalize the keep alive thread as well.
		//note: there's a fall-back mechanism in destructor of a CConversation as well.
		{
			std::lock_guard<std::recursive_mutex> lock(mPingThreadGuardian);
			setAreKeepAliveMechanicsToBeActive(false);

			if (getIsPingThreadAlive() && isPingThreadValid())
				mPingMechanicsThread.get();
		}

		//Sleep(1000);
		if (getProtocol() != eTransportType::QUIC)
		{
			closeSocket();//very important: close,free the socket AFTER the conversation marked as ended to prevent usage of the socket after underlying
			//resources have been released. UDT library doesn't check we need to check ourselves.
		}
		else
		{
			// With QUIC, we need to perform a fully asynchronous shutdown.
			// 
			// [Recall]: should anything go wrong during a 'gentle' shutdown, the Controller of CQUICConversationsServer would remain always on watch
			//		  and forcefully shutdown the connection and do cleaning up when and as needed.

			shutdownQUICConnection(false);
		}
		cleanTasks();
		//if (getProtocol() == eTransportType::UDT)
		//{
		//	mUDTSocket = 0;
		//}
		setIsThreadAlive(false);
	}
	catch (...)
	{
		closeSocket();
		cleanTasks();
	}
}
bool CConversation::isSocketFreed()
{
	//std::lock_guard<std::mutex> lock(mFieldsGuardian);

	bool wasIt = false;
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		wasIt = mSocketWasFreed;
	}

	return wasIt;
}

void CConversation::setLeadBlockInfo(const std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>>& info)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLeaderInfo = std::make_tuple(std::get<0>(info), std::get<1>(info));
}

std::vector<uint8_t> CConversation::getLeadBlockID()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return std::get<0>(mLeaderInfo);
}

std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>> CConversation::getLeadBlockInfo()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLeaderInfo;
}

bool CConversation::getIsSyncEnabled()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mFlags.exchangeBlocks;
}
bool CConversation::getAreKeepAliveMechanicsToBeActive()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mKeepAliveActive;
}
void CConversation::setAreKeepAliveMechanicsToBeActive(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mKeepAliveActive = isIt;
}
void CConversation::setIsSyncEnabled(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mFlags.exchangeBlocks = isIt;
}
// Getter for the QUIC connection handle
HQUIC CConversation::getQUICConnectionHandle() const {
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mFieldsGuardian));
	return mQUICConnectionHandle;
}

// Setter for the QUIC connection handle
void CConversation::setQUICConnectionHandle(HQUIC handle) {
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mQUICConnectionHandle = handle;
}

bool CConversation::closeSocket(){
	
	std::lock_guard<std::mutex> lock(mSocketOperationGuardian);

	// Local Variables - BEGIN
	bool alreadyFreed = false;
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		alreadyFreed = mSocketWasFreed;
	}
	HQUIC qConnHandle = getQUICConnectionHandle();
	eTransportType::eTransportType protocol = getProtocol();
	bool okToMarkQUICAsClosed = false;
	// Local Variables - END

	if (alreadyFreed)
		return true;

	// Operational Logic - BEGIN

	if (protocol == eTransportType::QUIC)
	{
		if (qConnHandle != nullptr) {
			// Gracefully shut down the QUIC connection.
			shutdownQUICConnection(true);// attempt an ABRUPT QUIC connection termination (only if the underlying QUIC connection not terminated yet)
		}
		else
		{
			okToMarkQUICAsClosed = true;// there was no QUIC connection handle to begin with
		}
	}
	else
	if (protocol == eTransportType::UDT)
	{
		UDTSOCKET s = getUDTSocket();
		if (s != 0 )
		{
			UDT::close(s);
			getNetworkManager()->getUDTServer()->incUDTSocketsClosedCount();
			setUDTSocket(0);
		}//else - must had been had been already closed
	
	}
	else
	{
		std::shared_ptr<CWebSocket> socket = getWebSocket();
		if (socket != nullptr && socket->isActive())
			socket->close();
	}
	//getState()->ping();
	mRXBuffer.clear();

	if (getProtocol() != eTransportType::QUIC || okToMarkQUICAsClosed)
	{
		// MS QUIC is fully asynchronous thus we need to wait for QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE event
		// so to prevent a situation in which CConversation gets removed while events still fire.

		// Notice that events are delivered by means of a static trampoline function.
		// The trampoline function is provided with a RAW pointer to CConversation, the corresponding CConversation
		// needs thus to prevail as long as events fire.
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		mSocketWasFreed = true;
	}
	// Operational Logic - END

	return true;
}

/// <summary>
/// AppID is optional, it is provided by the Web-UI.
/// </summary>
/// <param name="appID"></param>
/// <returns></returns>
std::shared_ptr<CDTI> CConversation::prepareDTI(uint64_t processID,std::vector<uint8_t> threadID, bool isUIAware)//for SSH Sessions similar logic is taken care through DTI-Server's createDTISession()
//[todo:CodesInChaos:low]: re-factor (restructure) 
{

	cleanUpDTIs();
	std::shared_ptr<CDTI> dti = findDTI(processID);

	if (dti != nullptr && dti->getStatus() != eDTISessionStatus::ended)//&& mDTI->getStatus()== eDTISessionStatus::alive)
		return dti;

	dti = std::make_shared<CDTI>(getBlockchainManager()->getMode(), mProtocol);

	if (!dti)
		return nullptr;

	dti->setUIProcessID(processID);
	dti->setIsSupposedToBeUIAware();
	dti->setThreadID(threadID);
	addDTI(dti);

	if (dti->initialize(shared_from_this()))
	{
		getNetworkManager()->getDTIServer()->registerDTISession(dti);
		return dti;
	}
	return nullptr;
}

std::string CConversation::getIPAddress()
{	
	std::shared_ptr<CWebSocket> socket = getWebSocket();
	std::string toRet;
	std::vector<uint8_t> temp;
	if (socket&& getProtocol() == eTransportType::WebSocket)
	{
		toRet = socket->getIPAddress();
	}
	else if(getProtocol() == eTransportType::UDT)
	{
		temp = mEndpoint->getAddress();
		toRet = std::string(temp.begin(), temp.end());
	}
	else if (getProtocol() == eTransportType::QUIC)
	{
		temp = mEndpoint->getAddress();
		toRet = std::string(temp.begin(), temp.end());
	}
	return toRet;
}


UDTSOCKET CConversation::getUDTSocket()
{
	std::lock_guard<std::mutex> lock(mSocketGuardian);
	
	return mUDTSocket;
}

void CConversation::setUDTSocket(UDTSOCKET socket)
{
	std::lock_guard<std::mutex> lock(mSocketGuardian);

	mUDTSocket = socket;
}

bool CConversation::taskChanged()
{
	std::lock_guard<std::mutex> lock (mPreviousTaskGuardian);
	//Local Variables - BEGIN
	bool changed = false;
	std::shared_ptr<CNetTask> task = getCurrentTask();
	uint64_t currentTaskID = (task != nullptr ? task->getID() : 0);
	eNetTaskState::eNetTaskState currentTaskState= (task != nullptr ? task->getState() : eNetTaskState::initial);
	//Local Variables - END

	//Operational Logic - BEGIN
	if (mPreviousTaskID == 0 && task != nullptr)//under the assumptiuon that min. task id is 1
	{
		changed = true;
	}

	if (mPreviousTaskID != currentTaskID)
		changed = true;
	else
		if (mPreviousTaskState != currentTaskState)
			changed = true;

	if (changed)
	{
		if (task != nullptr)
		{
			mPreviousTaskID = task->getID();
			mPreviousTaskState = task->getState();
		}
		else
		{
			mPreviousTaskID = 0;
			mPreviousTaskState = eNetTaskState::initial;
		}
	}
	//Operational Logic - END
	
	return changed;
}

bool CConversation::getLogDebugData()
{
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		if (!mLogDebugData)
		{
			return false;
		}
	}

	std::shared_ptr<CNetworkManager> nm = getNetworkManager();
	std::string focusedIP;

	if (nm->canEventLogAboutIP(focusedIP))
	{
		return true;
	}
	return false;
}

bool CConversation::getIsEncryptionAvailable(bool requrieSessionBasedEncryption)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	if (mSessionKey.size() > 0 || ((mRemoteSessionDescription != nullptr && mRemoteSessionDescription->getComPubKey().size() == 32) && !requrieSessionBasedEncryption))
		return true;

	return false;
}

void CConversation::setSyncToBeActive(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mFlags.synchronisationToBeActive = isIt;
}

bool CConversation::getSyncToBeActive()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mFlags.synchronisationToBeActive;
}

uint64_t CConversation::getTransmissionErrorsCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return 	mTransmissionErrors;
}

void CConversation::incTransmissionErrorsCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mTransmissionErrors++;
}

/// <summary>
/// Takes care of NetworkTask processing.
/// Tasks may be synchronous as a-synchronous.
/// When a critical tasks fails (ex. handshake), the function MAY notify the other and abort the connection.
/// </summary>
/// <param name="task"></param>
/// <param name="socket"></param>
/// <param name="webSocket"></param>
/// <param name="doBlockingProcessing"></param>
/// <returns></returns>

eNetTaskProcessingResult::eNetTaskProcessingResult CConversation::processTask(std::shared_ptr<CNetTask> task, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket, bool doBlockingProcessing)
{/*
 Extreme warning: Routing tasks are processes by the NeworkManager's task processing queue.
 Here we MIGHT process only sendNetmsg task type when it comes to NetMsg deliveries.
 sendNetMsg task is supposed to take care of all the encryption and authentication in regard to msg's body.
 */
	
	std::shared_ptr<CTools> tools = getTools();
	size_t now = tools->getTime();
	uint64_t timeOutAfter = task->getTimeoutAfter();
	uint64_t requestID = 0;

	eNetTaskProcessingResult::eNetTaskProcessingResult toRet = eNetTaskProcessingResult::error;
	BigInt tempBigInt = 0;
	if (task == nullptr)
		return toRet;

	std::shared_ptr<CNetTask> translatedToTask = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
	std::shared_ptr<CNetMsg> netMsg;
	if (task->getType() != eNetTaskType::endConversation && timeOutAfter && ((now - task->getTimeCreated()) >= timeOutAfter))//task of type endConversation cannot be aborted.
	{
		task->setState(eNetTaskState::aborted);
		return toRet = eNetTaskProcessingResult::error;
	}


	while (!(getState()->getCurrentState() == eConversationState::ended || getState()->getCurrentState() == eConversationState::ending ||
		task->getState()== eNetTaskState::completed || task->getState()== eNetTaskState::aborted))
	{
		eNetTaskType::eNetTaskType tType = task->getType();

		switch (tType)
		{
			/*
			* It often does happen that 'Network Tasks' are translated into 'Net Messages' meaning PDUs representing particular information and/or
			* data requests. Higher-level logic oftentimes issues 'Tasks', whereas the Conversation Thread associated with a particular Conversation
			* takes care of its fulfillment.
			*/

		//DSM Synchronization Tasks - BEGIN
		case eNetTaskType::requestChainProof:
			netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::chainProof, eNetReqType::request, task->getData());

			if (sendNetMsg(netMsg))
			{
				return eNetTaskProcessingResult::succeeded;
			}
			else
			{
				return eNetTaskProcessingResult::error;
			}
			break;

		case eNetTaskType::notifyChainProof:
			netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::chainProof, eNetReqType::notify, task->getData());
			
			if (sendNetMsg(netMsg))
			{
				return eNetTaskProcessingResult::succeeded;
			}
			else
			{
				return eNetTaskProcessingResult::error;
			}
			break;

		case eNetTaskType::notifyBlock:
		
			netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::block, eNetReqType::notify, task->getData());

			if (sendNetMsg(netMsg))
			{
				return eNetTaskProcessingResult::succeeded;
			}
			else
			{
				return eNetTaskProcessingResult::error;
			}
			break;

		case eNetTaskType::notifyChatMsg:

			netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::msg, eNetReqType::notify, task->getData());

			if (sendNetMsg(netMsg))
			{
				return eNetTaskProcessingResult::succeeded;
			}
			else
			{
				return eNetTaskProcessingResult::error;
			}
			break;

		case eNetTaskType::requestBlock:
			//requestID = genRequestID();//not used as of now; could be used were we to use the upper level meta-data exchange protocol.
				//see eNetTaskType::requestData  for an example of vm-meta-data bases processing.
			 netMsg = std::make_shared<CNetMsg>(eNetEntType::blockBody, eNetReqType::request, task->getData());
			  tempBigInt = task->getMetaRequestID();//instead of mixing distinct protocol fields...
			 netMsg->setExtraBytes(tools->BigIntToBytes(tempBigInt));//we should probbaly be using the vm-meta-data exchange protocol but whatever...

			 if (sendNetMsg(netMsg))
			 {
				 return eNetTaskProcessingResult::succeeded;
			 }
			 else
			 {
				 return eNetTaskProcessingResult::error;
			 }
			break;

		//DSM Synchronization Tasks - END

		case eNetTaskType::awaitOperationStatus:
			//non-blocking call YET,  the operation will keep blocking the tasks' queue until the conversation timeouts
			//OR an operationStatus netmsg is received
			return eNetTaskProcessingResult::inProgress;
			break;

		case eNetTaskType::awaitSecureSession:
			//non-blocking call YET,  the operation will keep blocking the tasks' queue until the conversation timeouts
			//OR => sessionKey is established
			if (getIsEncryptionAvailable())
			{
				return eNetTaskProcessingResult::succeeded;
			}
			return eNetTaskProcessingResult::inProgress;
			break;

		case eNetTaskType::sendNetMsg:
			if (!sendNetMsg(task->getNetMsg(), true))
			{
				return eNetTaskProcessingResult::error;
			}
			return eNetTaskProcessingResult::succeeded;
			break;
		case eNetTaskType::sendVMMetaData:
			if (requestVMMetaDataProcessing(task->getData(),doBlockingProcessing))
				return eNetTaskProcessingResult::succeeded;
			else
				return eNetTaskProcessingResult::error;
			break;
		case eNetTaskType::startConversation:
			if (getLogDebugData())
			{
				tools->logEvent(tools->getColoredString("[ Dispatching a Hello Request ] ( Stage 1 ): ", eColor::orange) + getDescription(),eLogEntryCategory::network,0);
			}
			if (requestHelloMsg(socket))//there's no error to handle over here besides the possibility of handshake being not delivered; it would be retried;
				//the other peers is supposed to initiate handshake procedure in such a case anyway, as well.
				return eNetTaskProcessingResult::succeeded;
			else
				return eNetTaskProcessingResult::error;
			break;

		case eNetTaskType::endConversation:

			if (isThreadValid() && getIsThreadAlive())
			{
				getState()->setCurrentState(eConversationState::ending);
			}
			else
			{
				getState()->setCurrentState(eConversationState::ended);
			}

			if (notifyByeMsg(socket))
			{
				
				return eNetTaskProcessingResult::succeeded;
			}
			else
				return eNetTaskProcessingResult::error;
			break;
	
	
		case eNetTaskType::downloadUpdate:
			break;
		case eNetTaskType::checkForUpdate:
			break;
		case eNetTaskType::sync:
			break;
		case eNetTaskType::sendQRIntentResponse:
			break;

	
		case eNetTaskType::notifyReceiptID:
			if (notifyReceiptIDMsg(task->getData(), socket))
				return eNetTaskProcessingResult::succeeded;
			else
				return eNetTaskProcessingResult::error;
			break;
		case eNetTaskType::notifyReceiptBody:
			break;
		case eNetTaskType::sendRAWData:
		
			if (sendRAWData(socket, task))
				toRet = eNetTaskProcessingResult::succeeded;
			else
				toRet =eNetTaskProcessingResult::error;

			if (getLogDebugData())
			{
				tools->logEvent("[SendRawData] task: " +std::string(toRet == eNetTaskProcessingResult::succeeded?tools->getColoredString("SUCCEEDED", eColor::lightGreen): tools->getColoredString("FAILED", eColor::lightPink))+ " [AbstractEndpoint]: " + getAbstractEndpoint()->getDescription(), eLogEntryCategory::network, 0);
			}
			return toRet;

			break;

		case eNetTaskType::requestData:
			//The below happens through the Meta-Data exchange sub-protocol (above the NetMsg protocol).
			//	genRequestID(); - consider using
			if (task->getState() == eNetTaskState::working)
			{
				return eNetTaskProcessingResult::inProgress;
			}
		
			if (sendNetMsg(doBlockingProcessing,eNetEntType::VMMetaData,eNetReqType::request,eDFSCmdType::unknown,std::vector<uint8_t>(),task->getData()))
			{
				task->setState(eNetTaskState::working);
				if (!doBlockingProcessing)
				{
					
					return eNetTaskProcessingResult::inProgress;
				}
				else
				{
					while (!(getState()->getCurrentState() == eConversationState::ended || getState()->getCurrentState() == eConversationState::ending ||
						task->getState() == eNetTaskState::completed || task->getState() == eNetTaskState::aborted))
					{
						if ((tools->getTime() - task->getTimeCreated()) > task->getTimeoutAfter())
						{
							task->setState(eNetTaskState::aborted);
							break;
						}
						//[todo:PauliX:low]: what's the situation in regard to timeouts on the mobile app? does it support NetTasks?

						Sleep(100);
						std::shared_ptr<CNetMsg> msg;
						if (msg = getRecentMsg())
						{
							//Abort ONLY if it's a DFS command targetting the System Thread
							if (msg->getEntityType() == eNetEntType::DFS)
							{

								std::vector<uint8_t> decryptionKey;
								if (msg->getFlags().encrypted)
								{	//do not decrypt hello-messages here. These will be demestified within their specialized handlers.
										//Reason: they might affect session key and we do not want that logic over here.

									if (msg->getFlags().boxedEncryption)
									{
										decryptionKey = Botan::unlock(getPrivKey());
									}
									else
									{
										decryptionKey = Botan::unlock(getSessionKey());
									}

									if (decryptionKey.size() == 0)
									{
										dequeMsg();
										continue;
									}

									if (!msg->decrypt(decryptionKey, getExpectedPeerPubKey()))//included signature verificaiton IF present and pubkey provided
									{
										dequeMsg();
										continue;
									}
								}

								std::shared_ptr<CDFSMsg> dfsM = CDFSMsg::instantiate(msg->getData());
								std::shared_ptr<SE::CScriptEngine> st = getSystemThread();

								if (st && dfsM &&  tools->compareByteVectors(dfsM->getThreadID(),st->getID()))
								{
									//DFS message is targetting the very same thread which was requesting data which is why we abort.
									//i.e. it is the responsibility of caller to assure that nothing disrupts the process.
									task->setState(eNetTaskState::aborted);
									return eNetTaskProcessingResult::aborted;
								}

							}
							getState()->ping();
							dequeMsg();
							//do what there's to be done with the received msg reply/process with blockchain manager etc.
							if (getState()->getCurrentState() != eConversationState::ending)//if conversation is ending we're not interested
							{
								processMsg(msg, getUDTSocket());
							}

							if (msg->getEntityType() == eNetEntType::bye)
							{
								getState()->setCurrentState(eConversationState::eConversationState::ending);
							}

						}
					}
					if (task->getState() == eNetTaskState::completed)
						return eNetTaskProcessingResult::succeeded;
					else {
						task->setState(eNetTaskState::aborted);//mark as ABORTED if was to be processed in blocking mode and failed
						return eNetTaskProcessingResult::aborted;
					}
					
				}
			}
			else
				return eNetTaskProcessingResult::error;

			break;
		default:
			return eNetTaskProcessingResult::aborted;
			break;
		}
	}
	return eNetTaskProcessingResult::aborted;
}


std::shared_ptr<COperationStatus> CConversation::prepareOperationStatus(eOperationStatus::eOperationStatus status, eOperationScope::eOperationScope scope,
	std::vector<uint8_t> ID, uint64_t reqID)
{
	std::shared_ptr<COperationStatus> st = std::make_shared<COperationStatus>(status, scope);
	if (reqID > 0)
		st->setReqID(reqID);
	if (ID.size() > 0)
		st->setID(ID);

	return st;
}


std::vector<uint8_t> CConversation::getPubKey()
{
	return mPubKey;
}
Botan::secure_vector<uint8_t> CConversation::getPrivKey()
{
	return mPrivKey;
}




bool CConversation::sendRAWData(const UDTSOCKET& socket, std::shared_ptr<CNetTask>& task)
{
	if (task == nullptr)
		return false;
	return sendBytes(task->getData());
}
conversationFlags CConversation::getFlags()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mFlags;
}

void CConversation::setFlags(conversationFlags& flags)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mFlags = flags;
}

std::shared_ptr<CNetTask> CConversation::getTaskByID(uint64_t id)
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);

	std::priority_queue<std::shared_ptr<CNetTask>, std::vector<std::shared_ptr<CNetTask>>, CCmpNetTasks> tasks = mTasks;
	std::shared_ptr<CNetTask> toRet = nullptr;
	while (!tasks.empty())
	{
		toRet = tasks.top();
		if (toRet->getID() == id)
			return toRet;
		tasks.pop();
	}
	return toRet;
}

void CConversation::cleanTasks()
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);

	std::priority_queue<std::shared_ptr<CNetTask>, std::vector<std::shared_ptr<CNetTask>>, CCmpNetTasks> emptyQueue;
	mTasks.swap(emptyQueue);
}


std::shared_ptr<CNetTask> CConversation::getTaskByMetaReqID(uint64_t id)
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);

	std::priority_queue<std::shared_ptr<CNetTask>, std::vector<std::shared_ptr<CNetTask>>, CCmpNetTasks> tasks = mTasks;
	std::shared_ptr<CNetTask> toRet = nullptr;
	
	while (!tasks.empty())
	{
		toRet = tasks.top();
		if (toRet->getMetaRequestID() == id)
			return toRet;

		tasks.pop();
	}
	return toRet;
}

std::future<void>& CConversation::getThread()
{
	std::lock_guard<std::recursive_mutex> lock(mConversationThreadGuardian);
	return mConversationThread;
}

bool CConversation::isThreadValid()
{
	std::lock_guard<std::recursive_mutex> lock(mConversationThreadGuardian);
	return mConversationThread.valid();
}

bool CConversation::isPingThreadValid()
{
	std::lock_guard<std::recursive_mutex> lock(mPingThreadGuardian);
	return mPingMechanicsThread.valid();
}
/// <summary>
/// Starts an UDT-Conversation with a given host if IPAddress!=0
//or participates in conversation if mSocket!=0. 
//The spawned thread decides whether to connect or whether to maintain connection through the provided socket.
//socket has priority if != nullptr
//Encryption and/or authentication parameters are indicated through their corresponding setters.
/// </summary>
/// <param name="IPAddress"></param>
/// <returns></returns>
bool CConversation::startUDTConversation(std::shared_ptr<ThreadPool> pool, UDTSOCKET client, std::string IPAddress)
{

	// clear state - BEGIN
	//initFields(true);

	setProtocol(eTransportType::UDT);
	refreshAbstractEndpoint();
	// clear state - END

	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	std::shared_ptr<CConversationState> state = getState();
	if (getUDTSocket() == 0 && IPAddress.size() == 0)//not an incoming connection an no target destination provided.
		return false;

	if (state->getCurrentState() != eConversationState::initial && state->getCurrentState() != eConversationState::ended)
	{
		return false;
	}

	if (getUDTSocket() == 0 && IPAddress.size() == 0)
		return false;

	setSessionKey(Botan::secure_vector<uint8_t>());

	while (getIsThreadAlive() || getIsPingThreadAlive())
	{
		end();
	}

	
	state->setCurrentState(eConversationState::initializing);
	tools->logEvent(tools->getColoredString("Starting a new UDT Conversation",eColor::lightCyan)+ " with " + IPAddress +".", eLogEntryCategory::network, 0);
	if (getThread().valid() && getIsThreadAlive())
		return false;//wait for the thread to finish executing (do not block with join() BUT at the same time DO NOT leave zombie threads)

	setIsScheduledToStart(true);
	std::lock_guard<std::recursive_mutex> l1(mPingThreadGuardian);
	std::lock_guard<std::recursive_mutex> l2(mConversationThreadGuardian);
	

	std::weak_ptr<CConversation> weak_this = shared_from_this();
	
	// start the main thread.
	mConversationThread = pool->enqueue(
		[weak_this, client, IPAddress]() {
			if (auto shared_this = weak_this.lock()) {
				shared_this->connectionHandlerThread(shared_this, client, IPAddress);//notice: no web-socket is required in case of UDT
			}
			// Optionally handle the case where weak_this is expired
		},
		ThreadPriority::HIGH
	);


	// start the keep-alive thread
	mPingMechanicsThread = pool->enqueue(
		[weak_this]() {
			if (auto shared_this = weak_this.lock()) {
				shared_this->pingMechanicsThread(shared_this);
			}
			// Optionally handle the case where weak_this is expired
		},
		ThreadPriority::NORMAL
	);


	return true;
}

// Create a new QUIC stream and add it to the active streams map
/*
* This function starts the processing of the stream by the connection. Once
  called, the stream can start receiving events to the handler passed into
  StreamOpen. If the start operation fails, the only event that will be
  delivered is QUIC_STREAM_EVENT_START_COMPLETE with the failure status code.

The first step of the start process is assigning the stream an identifier
(stream ID). The stream ID space is flow controlled, meaning the peer is able to
control how many streams the app can open (on-wire). Though, even if the peer
won't accept any more streams currently, this API (by default) allows the app to
still start the stream and assigns a local stream ID. But in this case, the
stream is just queued locally until the peer will accept it.

If the app does not want the queuing behavior, and wishes to fail instead, it
can use the QUIC_STREAM_START_FLAG_FAIL_BLOCKED flag. If there is not enough
flow control to allow the stream to be sent on the wire, then the start will
fail (via a QUIC_STREAM_EVENT_START_COMPLETE event) with the
QUIC_STATUS_STREAM_LIMIT_REACHED status.

The QUIC_STREAM_START_FLAG_INDICATE_PEER_ACCEPT flag can be used to get the
QUIC_STREAM_EVENT_PEER_ACCEPTED event to know when the stream becomes unblocked
by flow control. If the peer already provided enough flow control to accept the
stream when it was initially started, the QUIC_STREAM_EVENT_PEER_ACCEPTED event
is not delivered and the QUIC_STREAM_EVENT_START_COMPLETE's PeerAccepted field
will be TRUE. If is not initially accepted, if/once the peer provides enough
flow control to allow the stream to be sent on the wire, then the
QUIC_STREAM_EVENT_PEER_ACCEPTED event will be indicated to the app.

The stream can also be started via the QUIC_SEND_FLAG_START flag. See StreamSend
for more details.

Important - No events are delivered on the stream until the app calls
StreamStart (because of the race conditions that could occur) and it succeeds.
This means that if the parent connection is shutdown (e.g. idle timeout or peer
initiated) before calling StreamStart then the
QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE will not be delivered. So, apps that rely on
that event to trigger clean up of the stream must handle the case where
StreamStart is either not ever called or fails and clean up directly.
*/
HQUIC CConversation::createNewQUICStream() {
	std::lock_guard<std::mutex> lock(mActiveStreamsGuardian); // Safely manage concurrent access to mActiveStreams

	if (getCeaseCommunication())
		return nullptr;

	// Local Variables - BEGIN
	HQUIC StreamHandle = nullptr;
	HQUIC qConnHandle = getQUICConnectionHandle(); // Retrieves a connection handle
	const QUIC_API_TABLE* MsQuic = getQUIC(); // Gets a pointer to the MsQuic API table
	uint64_t nextStreamID = getNextStreamID();

	// Local Variables - END
	
	// Operational Logic - BEGIN
	
	incOrderedStreamCount();

	// IMPORTANT: QUIC_SEND_FLAG_FIN flag causes only SEND channel of a stram to be closed (*in a bi-directional stream*).
	//				Since we use uni-directional streams, the recipient does NOT need to call streamShutdown() which WOULD
	//				be required were we dealing with a bi-directional stream.
	// Attempt to open a new stream on the given connection
	QUIC_STATUS Status = MsQuic->StreamOpen(qConnHandle, QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL, StreamCallbackTrampoline, this, &StreamHandle);

	if (QUIC_SUCCEEDED(Status)) {
		// Successfully opened a stream, now start it
		Status = MsQuic->StreamStart(StreamHandle, QUIC_STREAM_START_FLAG_INDICATE_PEER_ACCEPT | QUIC_STREAM_START_FLAG_IMMEDIATE);
		if (QUIC_SUCCEEDED(Status)) {
			// Stream successfully started, add it to the active streams map
			mActiveStreams[nextStreamID] = StreamHandle; // Ensure mNextStreamId is properly managed
			return StreamHandle; // Return the handle to the newly opened and started stream
		}
		else {
			// If starting the stream fails, close the stream to clean up
			MsQuic->StreamClose(StreamHandle);
			decOrderedStreamCount();
		}
	}
	else
	{
		if (StreamHandle)
		{
			MsQuic->StreamClose(StreamHandle);
		}
		decOrderedStreamCount();

	}
	return nullptr; // Return nullptr if either opening or starting the stream fails

	// Operational Logic - END
}




/**
 * @brief Callback function for handling events on QUIC streams.
 *
 * This function is responsible for managing various events associated with a QUIC stream,
 * including the reception of data. One of the key events handled is `QUIC_STREAM_EVENT_RECEIVE`,
 * which is triggered when data is received over a QUIC stream. The data within a stream is
 * guaranteed to be delivered in order, but it can be split across multiple buffers within
 * the same event or across multiple events.
 *
 * In the context of `QUIC_STREAM_EVENT_RECEIVE`, the event provides an array of `QUIC_BUFFER`
 * structures, each containing a portion of the received data. The buffers are in the correct
 * sequence and must be processed sequentially to accurately reconstruct the original message.
 *
 * The primary steps in handling `QUIC_STREAM_EVENT_RECEIVE` are:
 *   1. Sequential Buffer Processing: Iterate through each `QUIC_BUFFER` in the event,
 *      processing the data in the order it is presented.
 *   2. Data Accumulation: For applications requiring the entire message before processing
 *      (e.g., reconstructing a `CNetMsg` object), accumulate the data from these buffers
 *      in a dynamic buffer or similar structure.
 *   3. Stream Closure and Completion: Upon receiving a stream closure event
 *      (e.g., `QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN`), the accumulated data can be
 *      treated as a complete message and processed accordingly.
 *   4. Handling Stream Aborts: Abrupt stream aborts (e.g., `QUIC_STREAM_EVENT_PEER_SEND_ABORTED`)
 *      should be treated as error cases, with appropriate handling for the partially
 *      received data.
 *
 * @param Stream The handle to the QUIC stream on which the event occurred.
 * @param Event A pointer to the `QUIC_STREAM_EVENT` structure containing details about
 *              the event, including the type of event and event-specific parameters.
 *
 * @return QUIC_STATUS_SUCCESS if the operation completes successfully, or an appropriate
 *         error status as needed.
 *
 * @note This function is registered as a callback in the QUIC API and should conform
 *       to the expected signature and return types.
 *
 * Usage Example:
 *     // Assuming 'Stream' is a valid HQUIC stream handle
 *     MsQuic->SetCallbackHandler(Stream, StreamCallback, Context);
 *     // The callback will now be invoked for stream events
 */
QUIC_STATUS CConversation::StreamCallback(
	HQUIC Stream,
	QUIC_STREAM_EVENT* Event
) {
	// Local Variables - BEGIN
	const QUIC_API_TABLE* MsQuic = getQUIC();
	std::string IP = getIPAddress();
	eConversationState::eConversationState convState = getState()->getCurrentState();
	std::shared_ptr<CTools> tools = getTools();
	bool logDebug = getLogDebugData();
	// Local Variables - END

	//Operational Logic - BEGIN
	switch (Event->Type) {

	case QUIC_STREAM_EVENT_PEER_ACCEPTED:
		incQUICPeerAcceptedStreamCount();

		if (logDebug) {
			tools->logEvent("QUIC Stream accepted by: " + IP + ". Stream successfully started.", eLogEntryCategory::network, 1, eLogEntryType::notification);
		}

		break;
	case QUIC_STREAM_EVENT_START_COMPLETE:
		// Handle stream start completion.
		if (QUIC_SUCCEEDED(Event->START_COMPLETE.Status)) {
			// Only increment the stream count if the start was successful.
			incStreamCount();
			if (logDebug) {
				tools->logEvent("QUIC Stream Start Complete for IP: " + IP + ". Stream successfully started.", eLogEntryCategory::network, 1, eLogEntryType::notification);
			}
		}
		else {
			// Handle start failure
			if (logDebug) {
				tools->logEvent("QUIC Stream Start Failed for IP: " + IP + ". Error: " + std::to_string(Event->START_COMPLETE.Status), eLogEntryCategory::network, 1, eLogEntryType::failure);
			}
			// Consider logging the error or taking appropriate action for the failed start operation.
		}
		break;

	case QUIC_STREAM_EVENT_RECEIVE:
		// Handle data reception on this stream.

		/*
		We correctly accumulate data in a buffer. 
		
		This is necessary because QUIC does not guarantee that the entire message will be delivered in a single event.
		Instead, data might be fragmented across multiple receive events, especially for larger messages.
		*/
		if (logDebug) {
			tools->logEvent("QUIC Stream Data Received from IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
		}
		// Accumulate data in the buffer for this stream
		for (uint32_t i = 0; i < Event->RECEIVE.BufferCount; ++i) {
			HandleStreamData(Stream, Event->RECEIVE.Buffers[i].Buffer, Event->RECEIVE.Buffers[i].Length);

		}
		// Notify MsQuic that data has been processed, allowing the sender to send more data
		MsQuic->StreamReceiveComplete(Stream, Event->RECEIVE.TotalBufferLength);
		break;

	case QUIC_STREAM_EVENT_SEND_COMPLETE:
		// Handle completion of a send operation on this stream.
		free(Event->SEND_COMPLETE.ClientContext); // deallocate memory region holding data which was already sent

		//removeStreamRXBuffer(Stream);
		if (logDebug) {
			tools->logEvent("QUIC Stream Send Complete for IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
		}
		break;

	case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
		// The peer has gracefully shut down their send side of the stream.
		if (logDebug) {
			tools->logEvent("QUIC Peer Send Stream Shutdown for IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
		}

		// We are now ready to process all of the data.
		ProcessAndRemoveStreamBuffer(Stream);

		// Since we are using uni-directional streams we do not need explicitly call streamShutdown() to shutdown our sending channel.
		// That would NEED to be the case for bi-directional streams once QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN is received.
		break;

	case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
		// The peer has aborted their send side of the stream.
		if (logDebug) {
			tools->logEvent("QUIC Peer Stream Send Aborted for IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
		}

		//MsQuic->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
		break;

	case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
		// The stream has been completely shut down.
		
		// (getFlags().isIncoming  || !Event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
			MsQuic->StreamClose(Stream);
			decStreamCount();
			decOrderedStreamCount();// always lower than the actual number of active streams. It's incremented even before stream initialization succeeds.
			removeStreamRXBuffer(Stream);// remove the buffer.
		//
		
		if (logDebug) {
			tools->logEvent("QUIC Stream Shutdown Complete for IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
		}
	
		break;

	case QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED:
		// The peer has aborted their receive side of the stream.
		if (logDebug) {
			tools->logEvent("QUIC Peer Stream Receive Aborted for IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
		}
		break;

	case QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE:
		// Local send shutdown of the stream is complete.
		if (logDebug) {
			tools->logEvent("QUIC Stream Send Shutdown Complete for IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
		}
		break;

	case QUIC_STREAM_EVENT_IDEAL_SEND_BUFFER_SIZE:
		// Adjust send buffer size as recommended for efficiency.
		if (logDebug) {
			tools->logEvent("QUIC Ideal Send Buffer Size event for IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
		}
		//TODO: for now we do not use this mode.
		
		// Adjust the stream's send buffer size to the recommended value
		if (Event->IDEAL_SEND_BUFFER_SIZE.ByteCount > 0) {
			//QUIC_STATUS SetBufferStatus = MsQuic->SetParam(Stream, QUIC_PARAM_STREAM_SEND_BUFFER_SIZE, sizeof(Event->IDEAL_SEND_BUFFER_SIZE.ByteCount), &Event->IDEAL_SEND_BUFFER_SIZE.ByteCount);
			/*if (QUIC_FAILED(SetBufferStatus)) {
				// Handle the failure to set the new buffer size
				tools->logEvent("Failed to set ideal send buffer size for QUIC Stream with IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::failure);
			}
			else {
				// Successfully set the new buffer size
				tools->logEvent("Successfully set ideal send buffer size for QUIC Stream with IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
			}*/
		}
		break;

	default:
		if (logDebug) {
			tools->logEvent("QUIC Stream Event (Unknown) for IP: " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);
		}
		break;
	}
	return QUIC_STATUS_SUCCESS;
	//Operational Logic - END
}


uint64_t CConversation::getNextStreamID()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mNextStreamId = mNextStreamId+1;
	return mNextStreamId;
}

void CConversation::HandleStreamData(HQUIC stream, const uint8_t* data, uint32_t length) {
	std::lock_guard<std::mutex> lock(mActiveStreamsGuardian); // Ensure thread safety
	mStreamRXBuffers[stream].insert(mStreamRXBuffers[stream].end(), data, data + length);
}

// In CConversation, verify these timeouts are enforced:
// - CGlobalSecSettings::getOverridedConvShutdownAfter() 
// - CGlobalSecSettings::getForcedConvShutdownAfter()

// Add to CConversation class:
bool CConversation::isSlowDataAttack() {
	// Only check WebSocket connections
	if (getProtocol() != eTransportType::WebSocket) return false;

	// Use existing state tracking
	if (!mState) return false;

	uint64_t now = CTools::getInstance()->getTime();
	uint64_t lastActivity = mState->getLastActivity();

	// Check if connection has been idle too long
	// Using existing timeout constants
	uint64_t idleTime = now - lastActivity;

	// If idle for more than half the forced shutdown time, it's suspicious
	if (idleTime > (CGlobalSecSettings::getOverridedConvShutdownAfter() / 2)) {
		// But only if the connection has been alive for a while
		// (to avoid false positives on new connections)
		if (mState->getDuration() > 60) { // Connection alive > 1 minute
			return true;
		}
	}

	// Check if connection is in a suspicious state for too long
	eConversationState::eConversationState currentState = mState->getCurrentState();

	// If stuck in connecting/initializing state for too long
	if (currentState == eConversationState::connecting ||
		currentState == eConversationState::initializing) {
		if (mState->getDuration() > 30) { // 30 seconds to complete handshake
			return true;
		}
	}

	// Check message rate using existing counters
	if (currentState == eConversationState::running) {
		// Use existing activity patterns
		// If we have very low activity on a long-running connection
		uint64_t duration = mState->getDuration();

		if (duration > 120) { // After 2 minutes
			// Check if we're getting minimal activity
			// Using the fact that getLastActivity() is updated on data
			uint64_t timeSinceLastActivity = now - lastActivity;

			// If activity is sporadic (typical Slowloris pattern)
			if (timeSinceLastActivity > 30 && timeSinceLastActivity < 60) {
				// Connection is kept alive with minimal activity
				return true;
			}
		}
	}

	return false;
}

/**
 * @brief Processes the accumulated data for a QUIC stream and finalizes message reception.
 *
 * This method is pivotal in the lifecycle of a QUIC stream within the context of message-based communication.
 * Upon the graceful shutdown of the peer's send direction on a stream (indicated by QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN),
 * it is invoked to process the accumulated data buffered throughout the stream's lifespan.
 *
 * The accumulated data is expected to form a complete message, serialized and ready for deserialization.
 * The method attempts to deserialize this data into a `CNetMsg` object - a representation of the application-level
 * message encapsulated within the data stream. Successful deserialization leads to the enqueuing of the `CNetMsg`
 * for further application-specific processing. Should deserialization fail, indicating an incomplete or malformed
 * message, the method increments a count of invalid messages, aiding in monitoring and debugging.
 *
 * Following the processing of the buffered data, the method cleans up by removing the buffer associated with the
 * stream from internal tracking structures, effectively freeing up resources and marking the stream's data handling
 * as complete. This removal is critical for maintaining the efficiency and resource usage of the application, preventing
 * memory leaks associated with orphaned or forgotten data buffers.
 *
 * @param stream The handle to the QUIC stream whose accumulated data is to be processed.
 *               This stream handle is used to identify the specific data buffer within
 *               an internal mapping of stream handles to their respective buffers.
 *
 * @note This method underscores the asynchronous, event-driven nature of QUIC stream management,
 *       where the end of data reception is explicitly signaled, necessitating a final processing
 *       step to handle the received data comprehensively.
 *
 * @note It is imperative that this method is called in response to the QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN
 *       event to ensure that data is processed only once the complete message is received, maintaining
 *       message integrity and coherence.
 */
void CConversation::ProcessAndRemoveStreamBuffer(HQUIC stream) {
	std::shared_ptr<CTools> tools = getTools();

	std::lock_guard<std::mutex> lock(mActiveStreamsGuardian);
	auto bufferIt = mStreamRXBuffers.find(stream);
	if (bufferIt != mStreamRXBuffers.end()) {
		// Process the complete message in bufferIt->second
		std::shared_ptr<CNetMsg> nMsg = CNetMsg::instantiate(bufferIt->second);

		if (nMsg) {

			if (getLogDebugData())
			{
				tools->logEvent("[QUIC] " + tools->getColoredString("Valid NetMsg received", eColor::lightGreen) + ".", eLogEntryCategory::network, 0);
			}


			// Handle the complete message
			getNetworkManager()->pingDatagramReceived(eNetworkSystem::QUIC, getTools()->stringToBytes(getIPAddress()));
			incValidMsgsCount();

			enqueueNetMsg(nMsg);
		}
		else {

			if (getLogDebugData())
			{
				tools->logEvent("[QUIC] " + tools->getColoredString("Invalid NetMsg received", eColor::lightPink) + ".", eLogEntryCategory::network, 0);
			}

			if (getInvalidMsgsCount() > 10)
			{
				end(false, false, eConvEndReason::security);
			}
		
			// Handle invalid message
			incInvalidMsgsCount();
		}

		// Remove the buffer for this stream
		mStreamRXBuffers.erase(bufferIt);
	}
}
/// <summary>
///  Notice: TX buffers are allocated per request on heap and de-allocated once data sent.
/// </summary>
/// <param name="stream"></param>
void CConversation::removeStreamRXBuffer(HQUIC stream) {

	std::lock_guard<std::mutex> lock(mActiveStreamsGuardian);

	auto bufferIt = mStreamRXBuffers.find(stream);
	if (bufferIt != mStreamRXBuffers.end()) {
		// Remove the buffer for this stream
		mStreamRXBuffers.erase(bufferIt);
	}
}


/**
 * @brief Closes a QUIC stream associated with the conversation.
 *
 * This method closes the specified QUIC stream. It can perform a graceful shutdown
 * or an abrupt shutdown based on the provided flag. A graceful shutdown allows
 * any ongoing or pending send operations to complete, whereas an abrupt shutdown
 * immediately aborts the stream.
 *
 * @param stream The QUIC stream handle to be closed.
 * @param abort If true, the stream is closed abruptly, otherwise it is closed gracefully.
 */
bool CConversation::shutdownQUICStream(HQUIC stream, bool abort, bool immediate) {

	const QUIC_API_TABLE* MsQuic = getQUIC();
	QUIC_STREAM_SHUTDOWN_FLAGS shutdownFlag = QUIC_STREAM_SHUTDOWN_FLAG_GRACEFUL; //note: QUIC_STREAM_SHUTDOWN_FLAG_ABORT

	if (abort && immediate)
	{
		shutdownFlag = QUIC_STREAM_SHUTDOWN_FLAG_IMMEDIATE | QUIC_STREAM_SHUTDOWN_FLAG_ABORT;
	}
	else if (abort)
	{
		shutdownFlag =  QUIC_STREAM_SHUTDOWN_FLAG_ABORT;

	}

	std::lock_guard<std::mutex> lock(mActiveStreamsGuardian);
	auto it = std::find_if(mActiveStreams.begin(), mActiveStreams.end(),
		[stream](const auto& pair) { return pair.second == stream; });

	if (it != mActiveStreams.end()) {
		MsQuic->StreamShutdown(stream, shutdownFlag, 0);
		mActiveStreams.erase(it);
		return true;
	}
	return false;
}


/**
 * @brief Sends data over a QUIC stream asynchronously.
 *
 * This function prepares and sends data over a given QUIC stream in an asynchronous manner. It handles memory allocation
 * for the data buffer that is to be sent and ensures that the allocated memory is managed properly throughout the
 * lifecycle of the send operation. The data to be sent is copied into a heap-allocated buffer, which is then passed
 * to the QUIC API for sending. The QUIC API takes ownership of this buffer and is responsible for its lifecycle post
 * submission for sending.
 *
 * The `Context` argument of the `QUIC_STREAM_SEND` API call is used to pass the allocated buffer to the QUIC library.
 * This allows the buffer to be accessed in the send completion callback, where it is deallocated. It is crucial that
 * the send completion callback, which is expected to handle the `QUIC_STREAM_EVENT_SEND_COMPLETE` event, correctly
 * frees this buffer to avoid memory leaks.
 *
 * The `finalize` parameter indicates whether this send operation should also close the send side of the stream
 * after sending the data. If `finalize` is true, the `QUIC_SEND_FLAG_FIN` flag is set, signaling the end of the
 * stream's send side to the peer.
 *
 * @param stream The handle to the QUIC stream over which the data is to be sent.
 * @param data A vector of bytes representing the data to be sent.
 * @param finalize A boolean flag indicating whether to finalize (close) the send side of the stream after sending the data.
 *
 * @return True if the operation was successfully initiated, false otherwise. Note that a return value of true
 * does not guarantee that the data will be successfully sent, as the operation is asynchronous. The final outcome
 * of the send operation will be reported via the send completion callback.
 *
 * @note It is the caller's responsibility to ensure that the stream remains valid until the send completion callback
 * has been invoked.
 */
bool CConversation::sendQUICData(HQUIC stream, const std::vector<uint8_t>& data, bool finalize) {
	const QUIC_API_TABLE* MsQuic = getQUIC();

	// Allocate buffer on the heap for QUIC_BUFFER and data payload
	size_t bufferSize = sizeof(QUIC_BUFFER) + data.size();
	auto* bufferRaw = static_cast<uint8_t*>(malloc(bufferSize));
	if (!bufferRaw) {
		// Log error: Failed to allocate memory for QUIC data send
		return false;
	}

	// Setup QUIC_BUFFER

	auto* quicBuffer = reinterpret_cast<QUIC_BUFFER*>(bufferRaw);
	quicBuffer->Length = static_cast<uint32_t>(data.size());
	quicBuffer->Buffer = bufferRaw + sizeof(QUIC_BUFFER);
	memcpy(quicBuffer->Buffer, data.data(), data.size());

	//Asynchronously send the data over the stream
	//here we MAY indicate that the stream is to be automatically shut down as soon as sending is complete ( QUIC_SEND_FLAG_FIN ).
	// IMPORTANT 1: no explicit closeQUICStream() invocation needed when 'finalize' flag set due to QUIC_SEND_FLAG_FIN below.
	// IMPORTANT 2: QUIC_SEND_FLAG_FIN flag causes only SEND channel of a stram to be closed (*in a bi-directional stream*).
	//				Since we use uni-directional streams, the recipient does NOT need to call streamShutdown() which WOULD
	//				be required were we dealing with a bi-directional stream.
	QUIC_STATUS status = MsQuic->StreamSend(stream, quicBuffer, 1, finalize ? QUIC_SEND_FLAG_FIN : QUIC_SEND_FLAG_NONE, quicBuffer);
	if (QUIC_FAILED(status)) {
		// Log error: QUIC StreamSend failed
		free(bufferRaw); // Clean up allocated buffer on failure
		shutdownQUICStream(stream, true); // order the stream to be shut down
		return false;
	}

	// Success: The send operation owns bufferRaw, to be freed in the send completion callback
	return true;
}



HQUIC CConversation::getRegistration()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mQUICRegistration;
}

void CConversation::setRegistration(HQUIC registration)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	 mQUICRegistration = registration;
}

/// <summary>
/// Starts an UDT-Conversation with a given host if IPAddress!=0
//or participates in conversation if mSocket!=0. 
//The spawned thread decides whether to connect or whether to maintain connection through the provided socket.
//socket has priority if != nullptr
//Encryption and/or authentication parameters are indicated through their corresponding setters.
/// </summary>
/// <param name="IPAddress"></param>
/// <returns></returns>
bool CConversation::startQUICConversation(std::shared_ptr<ThreadPool> pool, HQUIC connHandle, std::string IPAddress)
{
	std::lock_guard<std::recursive_mutex> lock2(mGuardian);
	
	// clear state - BEGIN
	//initFields(true);
	// clear state - END

	
	// Local Variables - BEGIN
	std::shared_ptr<CTools> tools = getTools();
	setProtocol(eTransportType::QUIC);
	refreshAbstractEndpoint();
	std::shared_ptr<CConversationState> state = getState();
	std::shared_ptr<CNetworkManager>  nm = getNetworkManager();
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		mQUICServer = nm->getQUICServer();
		if (!mQUICServer)
		{
			return false;
		}
		mQUICConnectionHandle = connHandle;
	}
	std::shared_ptr<CQUICConversationsServer>  server = getQUICServer();
	if (!server)
		return false;
	// Local Variables - END

	// Pre-Flight - BEGIN
	if (state->getCurrentState() != eConversationState::initial && state->getCurrentState() != eConversationState::ended)
	{
		return false;
	}
	// Pre-Flight - END
	
	// Operational Logic - BEGIN
	setRegistration(server->getRegistration());

	setSessionKey(Botan::secure_vector<uint8_t>());

	while (getIsThreadAlive() || getIsPingThreadAlive())
	{
		end();
	}

	

	state->setCurrentState(eConversationState::initializing);

	tools->logEvent(tools->getColoredString("Starting a new QUIC Conversation with ", eColor::lightCyan) + IPAddress + ".", eLogEntryCategory::network, 0);

	if (getThread().valid() && getIsThreadAlive())
		return false;//wait for the thread to finish executing (do not block with join() BUT at the same time DO NOT leave zombie threads)

	setIsScheduledToStart(true);

	// Threads - BEGIN
	std::lock_guard<std::recursive_mutex> l1(mPingThreadGuardian);
	std::lock_guard<std::recursive_mutex> l2(mConversationThreadGuardian);
	std::weak_ptr<CConversation> weak_this = shared_from_this();

	// start the main thread.
	mConversationThread = pool->enqueue(
		[weak_this,  IPAddress]() {
			if (auto shared_this = weak_this.lock()) {
				shared_this->connectionHandlerThread(shared_this, 0, IPAddress);//notice: no web-socket is required in case of UDT
			}
			// Optionally handle the case where weak_this is expired
		},
		ThreadPriority::HIGH
	);

	// start the keep-alive thread
	mPingMechanicsThread = pool->enqueue(
		[weak_this]() {
			if (auto shared_this = weak_this.lock()) {
				shared_this->pingMechanicsThread(shared_this);
			}
			// Optionally handle the case where weak_this is expired
		},
		ThreadPriority::NORMAL
	);

	// Threads - END

	return true;
	// Operational Logic - END

}


/// <summary>
/// Starts a Web-Socket conversation. The conversation becomes operational.
/// Encryption and/or authentication parameters are indicated through their corresponding setters.
/// </summary>
/// <param name="socket"></param>
/// <param name="URL"></param>
/// <returns></returns>
bool CConversation::startWebSockConversation(std::shared_ptr<ThreadPool> pool, std::weak_ptr<CWebSocket> socket, std::string URL)
{

	// clear state - BEGIN
	//initFields(true);
	// clear state - END

	if (socket.expired() && URL.size() == 0)
		return false;
	std::shared_ptr<CConversationState> state = getState();


	if (state->getCurrentState() != eConversationState::initial && state->getCurrentState() != eConversationState::ended)
	{
		return false;
	}

	if (getThread().valid())
	{
		end();
	}
	
	setProtocol(eTransportType::WebSocket);

	state->setCurrentState(eConversationState::initializing);
	refreshAbstractEndpoint();
	setIsScheduledToStart(true);
	std::weak_ptr<CConversation> weak_this = shared_from_this();

	std::lock_guard<std::recursive_mutex> l1(mPingThreadGuardian);
	std::lock_guard<std::recursive_mutex> l2(mConversationThreadGuardian);

	// initialize main thread
	mConversationThread = pool->enqueue(
		[weak_this, URL, socket]() {
			if (auto shared_this = weak_this.lock()) {
				shared_this->connectionHandlerThread(shared_this, 0, URL, socket);//notice: a web-socket needs to be passed.
			}
			// Optionally handle the case where weak_this is expired
		},
		ThreadPriority::HIGH
	);

	// initialize keep-alive thread
	mPingMechanicsThread = pool->enqueue(
		[weak_this]() {
			if (auto shared_this = weak_this.lock()) {
				shared_this->pingMechanicsThread(shared_this);
			}
			// Optionally handle the case where weak_this is expired
		},
		ThreadPriority::NORMAL
	);

	return true;
}

CConversation::~CConversation()
{
	/*
	   Ultimately the Conversation transitions into 'Ended' state only as part of a destructor of ActivitySignal destructor.
	   It is called once ActivitySignal is destroyed, which is when the main thread of CConversation exits - connectionHandlerThread().
	*/
	std::lock_guard<std::recursive_mutex> l1(mPingThreadGuardian);
	std::lock_guard<std::recursive_mutex> l2(mConversationThreadGuardian);
	std::lock_guard<std::recursive_mutex> l3(mThreadAliveGuardian);
	std::lock_guard<std::recursive_mutex> l4(mPingThreadAliveGuardian);

	// below we wait for threads to shut down (should there be a need).
	
	if (isThreadValid() && getIsThreadAlive()) {
		try {
			
			mConversationThread.get();
		}
		catch (const std::exception& e) {
			std::string error = e.what();
			getTools()->logEvent("Conversation shutdown ended with an exception: '"+ error+"'", eLogEntryCategory::network, 0, eLogEntryType::warning);
		}
	}

	if (isPingThreadValid() && getIsPingThreadAlive()) {
		try {
			mPingMechanicsThread.get();
		}
		catch (const std::exception& e) {
			std::string error = e.what();
			getTools()->logEvent("Conversation shutdown ended with an exception: '" + error + "'", eLogEntryCategory::network, 0, eLogEntryType::warning);
		}
	}

	bool alreadyClosed = isSocketFreed();

	if (alreadyClosed==false)
	{
		closeSocket();
		cleanTasks();
	}

}

bool CConversation::addTask(std::shared_ptr<CNetTask> task)
{
	if (task == nullptr)
		return false;
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);
	mTasks.push(task);
	return true;
}

std::shared_ptr<CNetTask> CConversation::getCurrentTask(bool deqeue)
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);
	std::shared_ptr<CNetTask> toRet = nullptr;
	if (!mTasks.empty())
	{
		toRet = mTasks.top();
		if (deqeue)
			mTasks.pop();
	}
	return toRet;
}

std::shared_ptr<CEndPoint> CConversation::getEndpoint()
{
	return mEndpoint;
}
void CConversation::setProtocol(eTransportType::eTransportType pType)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mProtocol = pType;
}


eTransportType::eTransportType CConversation::getProtocol()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mProtocol;
}

std::vector<uint8_t> CConversation::getID()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mID;
}


//Invoked when a new CNetMsg container is delivered from outside for processing (ex. through a web-socket).
void CConversation::enqueueNetMsg(std::shared_ptr<CNetMsg> msg)
{
	std::shared_ptr<CTools> tools = getTools();
	if (getLogDebugData())
	tools->logEvent("[" + tools->endpointTypeToString(getAbstractEndpoint()->getType()) + " Conversation]  Enqueueing NetMsg for processing." + msg->getDescription(), eLogEntryCategory::network, 0);
	//check if waiting for a QR-intent response if so ABORT.
						//no VM-related messages should be coming through once the transactions is being commuted.
						//abort what's going on, let the user know and handle incoming data
	eNetEntType::eNetEntType eType = msg->getEntityType();

	//let us check if the message type might require the VM
	//the VM is sequential and it might be currently awaiting a QRIntent-Response
	//in such a case we assume user aborted the previous commit request.
	//STILL, we give a grace period of 10 seconds, shall another GUI-dApp be non-well-behaved and ignore the pending 
	//commit.

	/*Reason: that's not a place to decide about this. DFS msg may be targeting one of the sub-threads and thus be of no destruction to the commit process
	if (eType == eNetEntType::DFS || eType == eNetEntType::VMMetaData)
	{
		std::shared_ptr<SE::CScriptEngine> se = getSystemThread();
		if (se != nullptr && se->getIsWaitingForVMMetaData())
		{//give a grace period of 10 seconds before the commit operation can be aborted

			uint64_t commitReqID = se->getMetaRequestID();
			size_t now = getTools()->getTime();
			if ((now - se->getQRWaitStartTime())>= 20)
			{
				se->setIsWaitingForVMMetaData(false);
				enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(), 0, "Commit Aborted");//todo: keep track of taskID by VM
			}
		}
	}// otherwise, the received message is VM-agnostic, we might enqueue it and do the processing right away (in accordance to its priority).
	*/
	std::lock_guard< std::mutex> lock(mNetMsgsQueueGuardian);
	mMsgs.push(msg);
}
std::shared_ptr<CNetMsg> CConversation::getRecentMsg()
{
	std::lock_guard< std::mutex> lock(mNetMsgsQueueGuardian);
	if (!mMsgs.empty())
	{
		return mMsgs.front();

	}
	else
		return nullptr;
}

void CConversation::dequeMsg()
{
	std::lock_guard<std::mutex> lock(mNetMsgsQueueGuardian);
	if (mMsgs.empty())
		return;
	mMsgs.pop();
}

uint64_t CConversation::genNewID()
{
	std::lock_guard<std::mutex> lock(sIDGenGuardian);
	return ++sLastID;
}

void CConversation::dequeTask()
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);
	if (mTasks.empty())
		return;
	mTasks.pop();
}

uint64_t CConversation::getLastLeaderNotificationOutTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastLeaderNotificationDispatchTimestamp;
}

void CConversation::pingLastLeaderNotificaitonTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastLeaderNotificationDispatchTimestamp = std::time(0);
}

const QUIC_API_TABLE* CConversation::getQUIC() 
{

	const std::shared_ptr<CQUICConversationsServer> server = getQUICServer();

	if (!server)
		return nullptr;

	return server->getQUIC();
}

uint64_t CConversation::getQUICPeerAcceptedStreamCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mQUICPeerAcceptedStreamCount;
}

void CConversation::incQUICPeerAcceptedStreamCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mQUICPeerAcceptedStreamCount = mQUICPeerAcceptedStreamCount+1;
}

uint64_t CConversation::getLastTimePingReceived()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastTimePingReceived;
}
void CConversation::pingReceived()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastTimePingReceived = std::time(0);
}
void CConversation::pingSent()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastPingSent = std::time(0);
}
uint64_t CConversation::getLastTimePingSent()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastPingSent;
}


bool CConversation::getIsScheduledToStart()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mIsScheduledToStart;
}

void CConversation::setIsScheduledToStart(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mIsScheduledToStart = isIt;;
}


CConversation::CConversation( CConversation & sibling)
{

	mUseAEADForAuth = sibling.mUseAEADForAuth;
	mUseAEADForSessionKey = sibling.mUseAEADForSessionKey;
	mAuthenticateHello = sibling.mAuthenticateHello;
	mConfiguration = sibling.mConfiguration;
	mQUICRegistration = sibling.mQUICRegistration;
	mNextStreamId = sibling.mNextStreamId;
	mActiveStreams = sibling.mActiveStreams;
	mQUICConnectionHandle = sibling.mQUICConnectionHandle;
	mCeaseCommunication = sibling.mCeaseCommunication;
	mIsScheduledToStart = sibling.mIsScheduledToStart;
	mIsPingThreadAlive = sibling.mIsPingThreadAlive;
	mPreventStartup = sibling.mPreventStartup;
	mIsThreadAlive = false;
	mDeferredTimeoutRequstedAtTimestamp = sibling.mDeferredTimeoutRequstedAtTimestamp;
	mPreviousTaskID = sibling.mPreviousTaskID;
	mKeepAliveActive = sibling.mKeepAliveActive;
	mPingMechanicsEnabled = sibling.mPingMechanicsEnabled;
	mLastPingSent = sibling.mLastPingSent;
	mEndReason = sibling.mEndReason;
	mMisbehaviorCounter = 0;
	mRecentRequestID = sibling.mRecentRequestID;
	mLastTimePingReceived = sibling.mLastTimePingReceived;
	mLastLeaderNotificationDispatchTimestamp = sibling.mLastLeaderNotificationDispatchTimestamp;
	mInvalidMsgsCount = sibling.mInvalidMsgsCount;
	totalNrOfBytesExchanged = sibling.totalNrOfBytesExchanged;
	nrOfMessagesExchanged = sibling.nrOfMessagesExchanged;
	mState = sibling.mState;
	mUDTSocket = sibling.mUDTSocket;
	mBlockchainManager = sibling.mBlockchainManager;
	mNetworkManager = sibling.mNetworkManager;
	mID = sibling.mID;
	mTools = sibling.mTools;
	mEncryptionRequired = sibling.mEncryptionRequired;
	mPeerAuthenticated = sibling.mPeerAuthenticated;
	mAbstractEndpoint = sibling.mAbstractEndpoint;
	mSettings = sibling.mSettings;
	mLogDebugData = sibling.mLogDebugData;
}


bool CConversation::getWasChallangeResponseDispatched()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mChallangeResponseDispatched;
}
std::shared_ptr<CConversationState>  CConversation::getState()
{
	std::lock_guard<std::mutex> lock(mStateGuardian);
	
	return mState;
}
void CConversation::setWasChallangeResponseDispatched(bool wasIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mChallangeResponseDispatched = wasIt;
}

Botan::secure_vector<uint8_t> CConversation::getSessionKey()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mSessionKey;
}

std::shared_ptr<CSessionDescription> CConversation::getRemoteSessionDescription()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mRemoteSessionDescription;
}

/// <summary>
/// Sets the remote description as provided by the remote peer. 
/// Optionally updated the Routing Table (default).
/// </summary>
/// <param name="desc"></param>
/// <param name="updateRT"></param>
void CConversation::setRemoteSessionDescription(std::shared_ptr<CSessionDescription> desc, bool updateRT)
{
	//Local Variables - BEGIN
	eEndpointType::eEndpointType eType;
	std::shared_ptr< CEndPoint> localAbstract;
	std::shared_ptr<CEndPoint> target;
	//Local Variables - END

	if (desc != nullptr && updateRT)
	{
	     eType = getAbstractEndpoint()->getType();
		 localAbstract = getAbstractEndpoint();
	}

	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		mRemoteSessionDescription = desc;
	}

	if (desc != nullptr && updateRT)
	{
		//Update the routing table so that the router can route to this conversation based on the remote conversation ID.
		//Note: there's yet another update to the routing table so to target this very  conversation based on conversation's local identifier
		//within refreshAbstractEndpoint().
		target = std::make_shared<CEndPoint>(desc->getConversationID(), eType, nullptr, 0);

		getNetworkManager()->getRouter()->updateRT(target, localAbstract);//mNetworkManager does never change during lifetime of the conversation
	}
}

void  CConversation::setSessionKey(Botan::secure_vector<uint8_t> key)
{
		std::shared_ptr<CTools> tools = getTools();

		if (key.size() > 0 && getLogDebugData())
		{
			std::shared_ptr<CSessionDescription> rsd = getRemoteSessionDescription();
			tools->logEvent("Secure session with peer " + tools->bytesToString(mEndpoint->getAddress()) + tools->getColoredString(" ESTABLISHED.",eColor::lightGreen)+" [SessionKey]:" 
			+ tools->base58CheckEncode(Botan::unlock(key))+" [Local ConversationID]: "+ tools->base58CheckEncode(getID())+ " [Remote ConversationID]: "+ (rsd!=nullptr?tools->base58CheckEncode(rsd->getConversationID()):"unknown"), eLogEntryCategory::network, 1);
		}

		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		mSessionKey = key;
		
}

void CConversation::setCurrentSD(std::string sdID)
{
	std::lock_guard<std::recursive_mutex> lock(mConsoleGuardian);
	mCurrentSDID = sdID;
}
void CConversation::setChangedCLIPrefix(bool set)
{
	std::lock_guard<std::mutex> lock(mPrefixChangedGuardian);
	mPrefixChanged = set;
}
void CConversation::setCurrentPath(std::string path)
{
	std::lock_guard<std::recursive_mutex> lock(mConsoleGuardian);
	mCurrentPath = path;
	setChangedCLIPrefix();
}
std::vector<uint8_t> CConversation::registerQRIntentResponse(std::shared_ptr<CQRIntentResponse> response)
{
	//std::lock_guard<std::mutex> lock(mScriptEngineGuardian);
	std::shared_ptr<SE::CScriptEngine> se = getSystemThread();
	if (se == nullptr)
		return std::vector<uint8_t>();
	return se->setQRIntentResponse(response);

}

std::vector<uint8_t> CConversation::registerVMMetaDataResponse(std::vector<std::shared_ptr<CVMMetaSection>> sections)
{
	std::shared_ptr<SE::CScriptEngine> se = getSystemThread();
	if (se == nullptr)
		return std::vector<uint8_t>();
	return se->setVMMetaDataResponse(sections);

}

std::string CConversation::getDescription()
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	std::string toRet;
	toRet += "[ID]: " + tools->base58CheckEncode(mID);
	toRet += " [Transport]: " + tools->transportTypeToString(mProtocol);
	toRet += " [HelloAuth] :" + std::string(mAuthenticateHello ? "Enabled" : "Disabled");
	toRet += " [Expected PubKey]: " + std::string(mPubKey.size()==32 ? tools->base58CheckEncode(mPubKey) : "none");
	toRet += " [PrivKey]: " + std::string(mPrivKey.size() == 32 ? tools->base58CheckEncode(Botan::unlock(mPrivKey)) : "none");
	return toRet;
}

bool CConversation::getDoSignOutgressMsgs()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return  mSignOutgressMsgs;

}
void CConversation::setDoSignOutgressMsgs(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mSignOutgressMsgs = doIt;

}


void CConversation::issueWarning()
{
	std::shared_ptr<CNetworkManager>  nm = getNetworkManager();
	std::string IP = getIPAddress();
	std::shared_ptr<CTools> tools = getTools();
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		++mWarningsIssued;
	}

	if (nm->getIsNetworkTestMode() && nm->canEventLogAboutIP(IP))
	{
		tools->logEvent("Peer " + IP + " received a " +std::to_string(mWarningsIssued)+ tools->getColoredString(" Warning", eColor::lightPink)+".", eLogEntryCategory::network, 3);

	}
	
}


/**
 * @brief Callback function for handling QUIC connection events.
 *
 * This function is the central point for processing various events related to a QUIC connection
 * within a conversation. It handles events such as connection establishment, shutdown initiated
 * by either the transport or the peer, and other critical connection-related events. This callback
 * is pivotal in managing the state and life-cycle of the QUIC connection in the context of a conversation.
 *
 * @param Connection The QUIC connection handle that triggered the event.
 * @param Event A pointer to the QUIC_CONNECTION_EVENT structure containing details about
 *              the specific connection event that occurred.
 *
 * @return Returns QUIC_STATUS_SUCCESS upon successfully handling the event, or an appropriate
 *         error code based on the situation.
 *
 * @note This function is registered with the QUIC API as the connection event callback handler.
 *       It is invoked by the QUIC library when various connection-related events occur.
 *
 * Usage:
 *     // Connection and Event are provided by the QUIC library when an event occurs.
 *     CConversation::ConnectionCallback(Connection, Event);
 */
QUIC_STATUS QUIC_API CConversation::ConnectionCallback(
	HQUIC Connection,
	QUIC_CONNECTION_EVENT* Event
) {
	// Local Variables - BEGIN
	std::string IP = getIPAddress();
	eConversationState::eConversationState convState = getState()->getCurrentState();
	std::shared_ptr<CTools> tools = getTools();
	bool logDebug = getLogDebugData();
	bool handshakeCompleted = false;
	bool peerAcknowledgedShutdown = false;
	bool appCloseInProgress = false;
	std::stringstream ss;
	const QUIC_API_TABLE* MsQuic = getQUIC();
	auto flags = getFlags();
	eConversationState::eConversationState state = getState()->getCurrentState();
	QUIC_STATUS Status = QUIC_STATUS_SUCCESS; // For SetParam calls
	// Local Variables - END


	/*
			IMPORTANT: this callback method is used for both incoming and outgoing connections.
			[Threat Model Appendix]: Both ingress and egress data.
	*/
	
	// Operational Logic - BEGIN
	switch (Event->Type) {

	case QUIC_CONNECTION_EVENT_CONNECTED:
		// The connection is successfully established.
		tools->logEvent("QUIC connection established with " + tools->getColoredString(IP, eColor::blue), eLogEntryCategory::network, 1);

		// --- BEGIN MODIFICATION: Set Idle Timeout and Keep-Alive via QUIC_SETTINGS ---
		if (MsQuic && Connection) { // Defensive check
			QUIC_SETTINGS Settings = { 0 }; // Initialize all to zero/false

			// Indicate which settings we are actually setting.
			// This is crucial for MsQuic to know which fields in the
			// QUIC_SETTINGS struct it should pay attention to.
			Settings.IsSet.IdleTimeoutMs = TRUE; // Use TRUE or 1
			Settings.IdleTimeoutMs = CGlobalSecSettings::getQuicConnectionIdleTimeoutMs();

			uint32_t keepAliveInterval = CGlobalSecSettings::getQuicKeepAliveIntervalMs();
			if (keepAliveInterval > 0) {
				Settings.IsSet.KeepAliveIntervalMs = TRUE;
				Settings.KeepAliveIntervalMs = keepAliveInterval;
			}

			// Example: If you wanted to control SendBufferingEnabled
			// Settings.IsSet.SendBufferingEnabled = TRUE;
			// Settings.SendBufferingEnabled = CGlobalSecSettings::isQuicSendBufferingEnabled() ? 1 : 0;


			// Only set parameters if at least one IsSet flag is true
			if (Settings.IsSetFlags != 0) { // Check if any field was actually set
				Status = MsQuic->SetParam(
					Connection, // Use the 'Connection' handle from the event
					QUIC_PARAM_CONN_SETTINGS, // Use this parameter ID
					sizeof(Settings),
					&Settings);

				if (QUIC_FAILED(Status)) {
					tools->logEvent("Failed to set QUIC_PARAM_CONN_SETTINGS. Error: " + std::to_string(Status), eLogEntryCategory::network, 1, eLogEntryType::failure);
				}
				else {
					if (logDebug) {
						std::string settingsApplied = "Applied QUIC settings for " + IP + ":";
						if (Settings.IsSet.IdleTimeoutMs) {
							settingsApplied += " IdleTimeoutMs=" + std::to_string(Settings.IdleTimeoutMs);
						}
						if (Settings.IsSet.KeepAliveIntervalMs) {
							settingsApplied += " KeepAliveIntervalMs=" + std::to_string(Settings.KeepAliveIntervalMs);
						}
						// Add other settings if logged
						tools->logEvent(settingsApplied, eLogEntryCategory::network, 0);
					}
				}
			}
		}
		// --- END MODIFICATION ---

		// we can now indeed mark the connection as running.
		if (getState()->getCurrentState() != eConversationState::eConversationState::ending &&
			getState()->getCurrentState() != eConversationState::eConversationState::ended)
		{
			getState()->setCurrentState(eConversationState::running);
		}
		break;

	case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
	
		// The transport has initiated a shutdown of the connection.
		if (Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status == QUIC_STATUS_CONNECTION_TIMEOUT) {

			//emit connectionFailureSig();

			if (logDebug)
				tools->logEvent("QUIC egress connection timeout for " + IP, eLogEntryCategory::network, 1, eLogEntryType::failure, eColor::lightPink);

		}
		else if (Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status == QUIC_STATUS_UNREACHABLE) {

			//semit connectionFailureSig();

			if (logDebug)
				tools->logEvent("QUIC destination unreachable for " + IP, eLogEntryCategory::network, 1, eLogEntryType::failure, eColor::lightPink);

		}
		else {
			if (logDebug)
				tools->logEvent("QUIC transport initiated shutdown for " + IP + ". Status: " + std::to_string(Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status),
					eLogEntryCategory::network, 1, eLogEntryType::failure, eColor::lightPink);
		}

		setCeaseCommunication(true);// prevent further application layer communiction attempts

		if (state != eConversationState::ended && state != eConversationState::ending)
		{
			end(false, false, eConvEndReason::QUICTransportLayer);
		}
		break;

	case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
		// The peer has initiated a shutdown of the connection.
		if (logDebug)
		tools->logEvent("QUIC shutdown initiated by peer for " + IP, eLogEntryCategory::network, 1);

		setCeaseCommunication(true);// prevent further application layer communiction attempts

		if (convState != eConversationState::ending && convState != eConversationState::ended) {
			end(false, false, eConvEndReason::otherEndTerminatedAbruptly);
		}
		break;

	case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
		// The connection has been completely shut down.
		 handshakeCompleted = Event->SHUTDOWN_COMPLETE.HandshakeCompleted; 
		 peerAcknowledgedShutdown = Event->SHUTDOWN_COMPLETE.PeerAcknowledgedShutdown; 
		 appCloseInProgress = Event->SHUTDOWN_COMPLETE.AppCloseInProgress; 


		 if (appCloseInProgress) {
		
			 if (flags.QUICConnectionClosed == false)
			 {
				 // Note: while the closing operation is inherently asynchronous as soon as ConnectionClose() returns
				 //       the connection handle needs to be assumed as INVALID.
				 //	      Consecutive events MAY fire, though.

				 flags.QUICConnectionClosed = true;
				 setFlags(flags);

				 MsQuic->ConnectionClose(Connection);// In case of an abrupt abort - clean-up connection handle without awaiting further events.
				
			 }
		 }
		// Create a single log message with all the values
		 if (logDebug)
		 {
			 ss << "QUIC connection shutdown complete for " << IP <<
				 ". HandshakeCompleted: " << (handshakeCompleted ? "true" : "false") <<
				 ", PeerAcknowledged: " << (peerAcknowledgedShutdown ? "true" : "false") <<
				 ", CoreAware: " << (appCloseInProgress ? "true" : "false");

			 tools->logEvent(ss.str(), eLogEntryCategory::network, 1);
		 }
		// Clean Up
		if (mConfiguration)
		{
			MsQuic->ConfigurationClose(mConfiguration);
			setQUICConnectionHandle(nullptr);// Safe to do so as ConfigurationClose is synchronous
		}
		{
			std::lock_guard<std::mutex> lock(mFieldsGuardian);
			mSocketWasFreed = true;
		}
		break;


	case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
		// A new stream has been started by the peer.
		// The peer has started/created a new stream. The we MUST set the
		// callback handler before returning.
		if (logDebug)
		tools->logEvent("Peer " + IP + " started new stream on a QUIC connection." , eLogEntryCategory::network, 1);

		MsQuic->SetCallbackHandler(Event->PEER_STREAM_STARTED.Stream, (void*)StreamCallbackTrampoline, this);

		break;

	case QUIC_CONNECTION_EVENT_STREAMS_AVAILABLE:
		// Notification that additional streams are available to be opened.
		if (logDebug)
		tools->logEvent("Additional QUIC streams available for " + IP, eLogEntryCategory::network, 1);
		break;

	case QUIC_CONNECTION_EVENT_RESUMED:
		// Handle connection resumption logic
		if(logDebug)
		tools->logEvent("QUIC connection resumed for " + IP, eLogEntryCategory::network, 1);
		break;

	case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED:
		// Handle receipt of a resumption ticket
		if (logDebug)
		tools->logEvent("QUIC resumption ticket received for " + IP, eLogEntryCategory::network, 1);
		break;

	case QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED:
		// Perform custom validation on the peer's certificate
		if (logDebug)
		tools->logEvent("QUIC peer certificate received for " + IP, eLogEntryCategory::network, 1);
		break;

	case QUIC_CONNECTION_EVENT_IDEAL_PROCESSOR_CHANGED:
		// Adjust processing according to the ideal processor change
		if (logDebug)
		tools->logEvent("QUIC ideal processor changed for " + IP, eLogEntryCategory::network, 1);
		break;

	case QUIC_CONNECTION_EVENT_DATAGRAM_STATE_CHANGED:
		// Handle changes in datagram support state
		if (logDebug)
		tools->logEvent("QUIC datagram state changed for " + IP, eLogEntryCategory::network, 1);
		break;

	case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED:
		// Process received datagrams
		if (logDebug)
		tools->logEvent("QUIC datagram received for " + IP, eLogEntryCategory::network, 1);
		break;

	case QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED:
		// Handle changes in the state of a sent datagram
		if (logDebug)
		tools->logEvent("QUIC datagram send state changed for " + IP, eLogEntryCategory::network, 1);
		break;
		// Add other events as needed

	default:
		if (logDebug)
		tools->logEvent("Unhandled QUIC event for " + IP, eLogEntryCategory::network, 1);
		break;
	}
	// Operational Logic - END

	return QUIC_STATUS_SUCCESS;
}


uint64_t CConversation::getWarningsCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mWarningsIssued;
}


void CConversation::clearWarnings()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mWarningsIssued = 0;
	mSecWindowSinceTimestamp = CTools::getInstance()->getTime();
}
uint64_t CConversation::genRequestID()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return ++mRecentRequestID;
}
bool CConversation::getIsThreadAlive()
{
	std::lock_guard<std::recursive_mutex> lock(mThreadAliveGuardian);
	return mIsThreadAlive;
}

void CConversation::setIsThreadAlive(bool isIt)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::recursive_mutex> lock(mThreadAliveGuardian);

	if (mIsThreadAlive == isIt)
		return;

	mIsThreadAlive = isIt;

	if (!isIt)
	{
		tools->SetThreadName("Conversation Thread ( IDLE )");
	}
	else
	{
		tools->SetThreadName("Conversation Thread ( ACTIVE )");
	}
}

bool CConversation::getIsPingThreadAlive()
{
	std::lock_guard<std::recursive_mutex> lock(mPingThreadAliveGuardian);
	return mIsPingThreadAlive;
}
void CConversation::setIsPingThreadAlive(bool isIt)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::recursive_mutex> lock(mPingThreadAliveGuardian);

	if (isIt == mIsPingThreadAlive)
		return;

	mIsPingThreadAlive = isIt;

	if (!isIt)
	{
		tools->SetThreadName("Conversation Ping Thread ( IDLE )");
	}
	else
	{
		tools->SetThreadName("Conversation Ping Thread ( ACTIVE )");
	}
}

bool CConversation::getPingMechanicsEnabled()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mPingMechanicsEnabled;
}

void CConversation::setPingMechanicsEnabled(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mPingMechanicsEnabled = isIt;
}

void CConversation::initState(std::shared_ptr<CConversation> conversation)
{
	std::lock_guard<std::mutex> lock(mStateGuardian);
	if (!mState)
	{
		mState = std::make_shared<CConversationState>(conversation);
	}

	if (conversation && !mState->getConversation())
	{
		mState->setConversation(conversation);
	}
	
}

uint64_t CConversation::getDuration()
{
	std::lock_guard<std::mutex> lock(mStateGuardian);

	uint64_t now = std::time(0);
	if (!mState)
		return 0;
	uint64_t startTime = mState->getTimeStarted();
	uint64_t endTime = mState->getTimeEnded();

	if (!startTime)
		return 0;

	if (!endTime)
	{
		return now - startTime;
	}
	else if(endTime > startTime)
	{
		return endTime - startTime;
	}

	return 0;
}

bool CConversation::getCeaseCommunication()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mCeaseCommunication;
}
void CConversation::setCeaseCommunication(bool isIt)
{
	 std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mCeaseCommunication = isIt;
}
void CConversation::setQUICShutdownInitiated(bool wasIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mQUICConnectinShutdownInitiated = wasIt;
}

bool CConversation::getQUICShutdownInitiated()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mQUICConnectinShutdownInitiated;
}

void CConversation::incStreamCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mQUICStreamCount++;
}

uint64_t CConversation::getStreamCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mQUICStreamCount;
}

void CConversation::decStreamCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	if (mQUICStreamCount)
	{
		mQUICStreamCount = mQUICStreamCount - 1;
	}
}


void CConversation::decOrderedStreamCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	if (mOrderedQUICStreamCount)
	{
		mOrderedQUICStreamCount = mOrderedQUICStreamCount - 1;
	}
}
void CConversation::incOrderedStreamCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mOrderedQUICStreamCount++;
}

uint64_t CConversation::getOrderedStreamCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mOrderedQUICStreamCount;
}
uint64_t CConversation::geAllowedStreamCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mAllowedStreamCount;
}
// Getter for total block notifications
uint64_t CConversation::getTotalBlockNotifications() {
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mTotalBlockNotifications;
}

// Getter for average block notification interval
double CConversation::getAverageNotificationInterval() {
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mAverageNotificationInterval;
}

// Method to update block notification statistics
void CConversation::updateBlockNotificationStats() {
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	// Increment the total number of notifications
	++mTotalBlockNotifications;

	// Get the current timestamp in seconds
	uint64_t now = static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::steady_clock::now().time_since_epoch())
		.count());

	// Update the average notification interval if this isn't the first notification
	if (mTotalBlockNotifications > 1) {
		uint64_t interval = now - mLastNotificationTime;
		mAverageNotificationInterval =
			((mAverageNotificationInterval * (mTotalBlockNotifications - 1)) + interval) / mTotalBlockNotifications;
	}

	// Update the last notification time
	mLastNotificationTime = now;
}

void CConversation::pingConvLocalLongCPProcessed()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastLongCPProcessed = std::time(0);
}
uint64_t CConversation::getConvLocalLongCPProcessedAt()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastLongCPProcessed;
}

void CConversation::initFields(bool justReset)
{
	mLastLongCPProcessed = 0;
	mTotalBlockNotifications = 0;
	mLastNotificationTime = 0;
	mAverageNotificationInterval = 0.0;
	//QUIC Fields - BEGIN
	mQUICStreamCount = 0;
	mOrderedQUICStreamCount = 0;
	mAuthAttempts = 0;
	mValidMsgsCount= 0;
	mQUICPeerAcceptedStreamCount = 0;
	mAllowedStreamCount = CGlobalSecSettings::getAllowedQUICStreamCount();
	mOrderedQUICStreamCount = 0;
	mQUICStreamCount = 0;
	mDeferredTimeoutRequstedAtTimestamp = 0;
	mNextStreamId = 0;
	mQUICConnectinShutdownInitiated = false;
	mConfiguration = 0;
	mQUICConnectionHandle = 0;
	mQUICRegistration = 0;
	mCeaseCommunication = false;
	//QUIC Fields - END
	mState = std::make_shared<CConversationState>();//just a STUB. make sure to call initState() right after creation.
	mIsScheduledToStart = false;
	mIsPingThreadAlive = false;
	mPreventStartup = false;
	mKeepAliveActive = true;
	mPingMechanicsEnabled = false;
	mLastPingSent = 0;
	mEndReason = eConvEndReason::none;
	mMisbehaviorCounter = 0;
	mIsThreadAlive = false;
	mSocketWasFreed = false;
	mTransmissionErrors = 0;
	mRecentRequestID = 0;
	mLastLeaderNotificationDispatchTimestamp = 0;
	mLastTimePingReceived = 0;
	mInvalidMsgsCount = 0;
	mSecWindowSinceTimestamp = 0;
	mWarningsIssued = 0;
	mLastTimeCodeProcessed = 0;
	mMisbehaviorCounter = 0;
	mUDTSocket = 0;
	mID = mTools->genRandomVector(8);
	mPreviousTaskID = 0;
	mUseAEADForSessionKey = false;
	mAuthenticateHello = true;
	mTools = CTools::getInstance();
	mSignOutgressMsgs = false;
	totalNrOfBytesExchanged = 0;
	nrOfMessagesExchanged = 0;
	mEncryptionRequired = true;
	mPeerAuthenticated = false;
	mAuthenticationRequired = false;
	mCryptoFactory = CCryptoFactory::getInstance();
	mUseAEADForAuth = false;
	CCryptoFactory::getInstance()->genKeyPair(mPrivKey, mPubKey);//ephemeral keys by default - can be replaced

	//local abstract endpoint constructed at the end of main constructor ( when an optional web socket is provided).
}
/// <summary>
/// CConversation represents an one-on-one communication with a network node.
/// It MIGHT be attached to a specific NetTask. Naturally an incoming connection would not.
/// The conversation might thus monitor the state of the task; and end itself all on its own.
/// Conversation MIGHT be encrypted. This is indicated by nmFlags within a Hello-request message.
/// Session key is stored within Conversation.
/// </summary>
/// <param name="socket"></param>
/// <param name="bm"></param>
/// <param name="nm"></param>
/// <param name="endpoint"></param>
/// <param name="task"></param>
CConversation::CConversation(UDTSOCKET socket, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm, std::shared_ptr <CEndPoint> endpoint, std::shared_ptr<CNetTask> task)
{
	initFields();

	mCustomBarID = CStatusBarHub::getInstance()->getNextCustomStatusBarID(bm->getMode());
	mEndReason = eConvEndReason::none;
	mTools = bm->getTools();
	mInvalidMsgsCount = 0;
	mIsPingThreadAlive = false;
	//assert(socket != 0);
	assertGN(bm != nullptr);
	assertGN(nm != nullptr);
	if (task != nullptr)
		addTask(task);
	mMetaGenerator = std::make_unique<CVMMetaGenerator>();
	mProtocol = eTransportType::UDT;
	mUDTSocket = socket;
	mBlockchainManager = bm;
	mSettings = bm->getSettings();
	mLogDebugData = mSettings->getLogEvents(eLogEntryCategory::network, eLogEntryType::notification) && (mSettings->getMinEventPriorityForConsole() == 0);
	mNetworkManager = nm;
	totalNrOfBytesExchanged = 0;
	nrOfMessagesExchanged = 0;
	mEndpoint = endpoint;
	refreshAbstractEndpoint();

	// sync - BEGIN
	
	setSyncToBeActive(bm->getSyncMachine()); // important: this indicates only a DESIRE. mFlags.exchangeBlocks indicated EFFECTIVE active synchronisation.
	// mFlags.exchangeBlocks = bm->getSyncMachine(); <- this would be enabled only once a secure channel established and the other peer is NOT a mobile device.

	// sync - END

}

/// <summary>
/// Constructs and/or refreshes the abstract endpoint describing this very conversation.
/// Optionally updates the Routing Table (default).
/// </summary>
/// <param name="updateRT"></param>
void CConversation::refreshAbstractEndpoint(bool updateRT)
{
	eTransportType::eTransportType transport =  getProtocol();

	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	eEndpointType::eEndpointType endpointType = eEndpointType::IPv4;

	if (transport == eTransportType::WebSocket)
	{
		endpointType = eEndpointType::WebSockConversation;
	}
	else if (transport == eTransportType::UDT)
	{
		endpointType = eEndpointType::UDTConversation;
	}
	else if (transport == eTransportType::QUIC)
	{
		endpointType = eEndpointType::QUICConversation;
	}


	mAbstractEndpoint = std::make_shared<CEndPoint>(mID, endpointType);
	std::shared_ptr<CDataRouter> router = mNetworkManager->getRouter();

	if (updateRT && router)
	{
		//update the routing table so that the router can route to this conversation based on the remote conversation ID
		std::shared_ptr<CEndPoint> target = std::make_shared<CEndPoint>(mID, mAbstractEndpoint->getType(), nullptr, 0);
		mNetworkManager->getRouter()->updateRT(target, mAbstractEndpoint);
	}
}

bool CConversation::getIsEncryptionRequired()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mEncryptionRequired;

}
void CConversation::setIsEncryptionRequired(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mEncryptionRequired = isIt;

}

bool CConversation::getIsAuthenticationRequired()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mAuthenticationRequired;

}
void CConversation::setIsAuthenticationRequired(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mAuthenticationRequired = isIt;
}

bool CConversation::getDoSigHelloAuth()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mAuthenticateHello;

}



void CConversation::setDoSigHelloAuth(bool doit)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mAuthenticateHello = doit;

}

std::shared_ptr<CEndPoint> CConversation::getAbstractEndpoint()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mAbstractEndpoint;
}

bool CConversation::getUseAEADForAuth()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mUseAEADForAuth;
}

void CConversation::setUseAEADForAuth(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mUseAEADForAuth = doIt;
}
uint64_t CConversation::getAuthAttempts()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mAuthAttempts;
}
void CConversation::incAuthAttempts()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mAuthAttempts++;
}
bool CConversation::getUseAEADForSessionKey()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mUseAEADForSessionKey;
}

void CConversation::setUseAEADForSessionKey(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mUseAEADForSessionKey = doIt;
}

void CConversation::clearAuthAttempts()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mAuthAttempts = 0;
}

CConversation::CConversation(std::weak_ptr<CWebSocket> webSocket, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm, std::shared_ptr <CEndPoint> endpoint, std::shared_ptr<CNetTask> task)
{
	initFields();
	mMetaGenerator = std::make_unique<CVMMetaGenerator>();
	setWebSocket(webSocket.lock());
	if (getWebSocket() != nullptr)
		mProtocol = eTransportType::WebSocket;
	else
		mProtocol = eTransportType::UDT;

	mTools = bm->getTools();
	
	//assert(socket != 0);
 assertGN(bm != nullptr);
 assertGN(nm != nullptr);
	if (task != nullptr)
		addTask(task);
	mUDTSocket = 0;
	mBlockchainManager = bm;
	mSettings = bm->getSettings();
	mLogDebugData = mSettings->getLogEvents(eLogEntryCategory::network, eLogEntryType::notification) && (mSettings->getMinEventPriorityForConsole() == 0);
	mNetworkManager = nm;
	totalNrOfBytesExchanged = 0;
	nrOfMessagesExchanged = 0;
	mEndpoint = endpoint;
	mState = std::make_shared<CConversationState>();
	refreshAbstractEndpoint();
}

CConversation::CConversation(HQUIC connHandle, HQUIC registration, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm, std::shared_ptr<CEndPoint> endpoint, std::shared_ptr<CNetTask> task)
{
	initFields();
	mQUICRegistration = registration;
	mMetaGenerator = std::make_unique<CVMMetaGenerator>();
	mQUICConnectionHandle = connHandle;
	mProtocol = eTransportType::QUIC;

	mTools = bm->getTools();

	//assert(socket != 0);
	assertGN(bm != nullptr);
	assertGN(nm != nullptr);
	if (task != nullptr)
		addTask(task);
	mUDTSocket = 0;
	mBlockchainManager = bm;
	mSettings = bm->getSettings();
	mLogDebugData = mSettings->getLogEvents(eLogEntryCategory::network, eLogEntryType::notification) && (mSettings->getMinEventPriorityForConsole() == 0);
	mNetworkManager = nm;
	totalNrOfBytesExchanged = 0;
	nrOfMessagesExchanged = 0;
	mEndpoint = endpoint;
	mState = std::make_shared<CConversationState>();


	refreshAbstractEndpoint();

	// sync - BEGIN
	
	setSyncToBeActive(bm->getSyncMachine()); // important: this indicates only a DESIRE. mFlags.exchangeBlocks indicated EFFECTIVE active synchronisation.

	// mFlags.exchangeBlocks = bm->getSyncMachine(); <- this would be enabled only once a secure channel established and the other peer is NOT a mobile device.

	// sync - END
}

void CConversation::setPrivKey(Botan::secure_vector<uint8_t> key)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mPrivKey = key;
}



void CConversation::setPubKey(std::vector<uint8_t> key)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mPubKey = key;
}





/// <summary>
/// Attempts to extract VMMetaData and process by internal VM.
/// On success returns true. MIGHT return a receipt ID.
/// Important: CConversation has a method named the same. We need to be keeping the two in sync.
/// Differences? DTI can process command-execution requests and launch/target specific applications whereas datagrams targetting RAW Conversations can only pass data to an instance of an underlying  VM
/// (if available at all).
/// 
/// The function is capable of routing of data to sub-threads registered with any main-vm.
/// </summary>
/// <param name="msg"></param>
/// <returns></returns>
bool CConversation::processNetMsg(std::shared_ptr<CNetMsg> msg, const std::vector<uint8_t>& receiptID, std::shared_ptr<CConversation>dataConv)
{

	if (!msg)
		return false;


	//Local Variables - BEGIN
	uint64_t serviceID = 0;
	std::string cmd;
	std::vector<uint8_t> payload;
	bool processed = false;
	std::vector<std::shared_ptr<CVMMetaEntry>> entries;
	std::vector<std::shared_ptr<CVMMetaSection>> sections;
	CVMMetaParser parser;
	std::shared_ptr<SE::CScriptEngine> targetVM;
	//Local Variables - END

	//Take Care of Decryption - BEGIN
	//Disclaimer: If a datagram is destined for a locally hosted Thread, the Thread takes care of decryption.
	//That's what happens below.

	/*That would be the case if data was end-to-end encrypted to specific instance of a DTI.
	That is the case with Log-Me-In responses.That is useful when Onion-Routing across multiple nodes
	and the network-later public key is unknown to initial peer(mobile phone). In such a case the data might be encrypted specifically
	to a given instance of a GridScript VM.

	The particular GRIDNET VM instance hosted on behalf of a given DTI, MIGHT have an ephemeral ECC key-pair generated.
	The ephemeral key-pair would be generated ONLY IF the VM expects data to be received. (ex. running Log-me-in native app).

	When not routing data across multiple full-nodes this wouldn't be needed. What this allows for is to achieve true end-to-end encryption
	and assuring anonymity against the final full-node in the chain of nodes i.e. the 'exit node'. i.e. to protect against the typical Tor-network's privacy-specic concerns.

	The network layer upon being enable to decrypt CNetMsg destined by a locally hosted DTI, would attempt to route it anyway (to the locally hosted DTI,
	which results in invocation of this very function).
	*/

	//Important: Notice that there's also processNetMsg of CDTI. The processing flow depends thus on routing entries.
	//i..e whether Thread has a routing table entry leading to it through one or another. Conversation is preferred.
	//for more check prepareAbstractEndpoint(?) of a thread.

	if (msg->getFlags().fromMobile)
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		mFlags.isMobileApp = true;
	}

	//Sub-threads' Support - BEGIN
	if (msg->getDestinationType() == eEndpointType::VM)
	{
		targetVM = mScriptEngine->getSubThreadByVMID(msg->getDestination());
	}

	if (!targetVM)
		targetVM = mScriptEngine;

	//Sub-threads' Support - END

	
	if (!targetVM)
		return false;

	targetVM->setDataDeliveryConversation(dataConv? dataConv:shared_from_this());
	if (msg->getFlags().encrypted && msg->getFlags().boxedEncryption)
	{
		if (!msg->decrypt(Botan::unlock(targetVM->getEphPrivKey().size() > 0 ? targetVM->getEphPrivKey() : getPrivKey())))//first try the VM's ephemeral private key if available (it gets cleared after any native
		{//app finishes executing) OR fall-back to the conversation's key
			
			return false;
		}
	}
	//Take Care of Decryption - END

	if (msg->getEntityType() != eNetEntType::VMMetaData)
	{

		if (msg->getFlags().encrypted && !msg->getFlags().boxedEncryption)
		{
			if (!msg->decrypt(Botan::unlock(getSessionKey())))//first try the VM's ephemeral private key if available (it gets cleared after any native
			{//app finishes executing) OR fall-back to the conversation's key

				return false;
			}
		}


		targetVM->setDataDeliveryConversation(dataConv);
		targetVM->setExternalData(msg->getData());
		return true;

	}
	//	return false;//only VM-MetaData protocol supported by the DTI interface


	sections = parser.decode(msg->getData());

	if (sections.size() == 0)
		return false;


	for (uint64_t i = 0; i < sections.size(); i++)
	{
		if (sections[i]->getType() == eVMMetaSectionType::notifications)//1)check for coreAppNotifications sections
		{//2) if present extract entries and attempt to see if scriptEngine is executing the particular app

			entries = sections[i]->getEntries();

			for (uint64_t b = 0; b < entries.size(); b++)
			{
				if ((entries[b]->getType() == eVMMetaEntryType::notification) && entries[b]->getProcessID() > 0)
				{
					//supported only by DTI. (see DTI's processNetMsg()'s implementation)

				}
				else if (entries[b]->getType() == eVMMetaEntryType::dataResponse)
				{
					//these get a receipt
					processed = true;
					const_cast<std::vector<uint8_t>&>( receiptID) = targetVM->setVMMetaDataResponse(sections, true, false);//wait for a receipt
					return true;
				}
			}
		}
		else if (sections[i]->getType() == eVMMetaSectionType::stateLessChannels)
		{
			entries = sections[i]->getEntries();

			for (uint64_t b = 0; b < entries.size(); b++)
			{
				if ((entries[b]->getType() == eVMMetaEntryType::StateLessChannelsElement) && targetVM->getIsWaitingForVMMetaData() && targetVM->getRunningAppID() == eCoreServiceID::genPool)
				{//the pool generator is waiting for response

					//handle it the entire meta data and let it take it from there
					targetVM->setVMMetaDataResponse(sections, false);
					return true;

				}
			}
		}
	}
	if (!processed && dataConv != nullptr)
	{
		dataConv->notifyOperationStatus(eOperationStatus::failure, eOperationScope::peer);//we didn't know how to process
	}

	return false;//nothing meaningful happened
}

void CConversation::setRecentOperationStatus(std::shared_ptr<COperationStatus> status)
{
	std::lock_guard<std::mutex> lock(mRecentOperationStatusGuardian);

	mRecentOperationStatus = status;
}

void CConversation::setCurrentState(eConversationState::eConversationState state)
{
	std::lock_guard<std::mutex> lock(mStateGuardian);
	getState()->setCurrentState(state);
}

bool CConversation::getIsHelloStage()
{
	if (getIsAuthenticationRequired() && !getPeerAuthenticated() || //encryption and/or authentication needs to be facilitated through NetMsg hello query/respose msgs.
		//it doesn't matter whether session encryption or per-msg AEAD is being used.
		getIsEncryptionRequired() && !getIsEncryptionAvailable())
		return true;

	return false;
}
/// <summary>
/// Process an incoming NetMsg->
/// </summary>
/// <param name="msg"></param>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::processMsg(std::shared_ptr<CNetMsg>  msg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket,  uint64_t recursionStep)
{
	//Security Checks - BEGIN
	if (recursionStep > 3)
		return false;

	if (msg == nullptr)
		return false;

	if (getProtocol() == eTransportType::UDT && socket == 0)
	{
		return false;
	}

	if (msg->getFlags().fromMobile)
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		mFlags.isMobileApp = true;
		mFlags.synchronisationToBeActive = false;
	}

	recursionStep++;

	//Security Checks - END

	//Local Variables - BEGIN
	std::shared_ptr<CTools> tools = getTools();
	std::shared_ptr<CSDPEntity> sdpMsg;
	std::shared_ptr<CDFSMsg> dfsMsg;
	CStateDomain* domain = nullptr;
	std::shared_ptr<CNetTask> task;
	std::priority_queue<std::shared_ptr<CNetTask>, std::vector<std::shared_ptr<CNetTask>>, CCmpNetTasks> tasksTemp = mTasks;
	std::shared_ptr<COperationStatus> operationStatus;
	std::shared_ptr <CDataRouter> router = getNetworkManager()->getRouter();
	uint64_t supposedReward = 0;
	bool processOnlyHello = false;
	std::vector<uint8_t> decryptionKey;
	std::shared_ptr<CNetMsg> offspringMsg;
	std::shared_ptr<CSessionDescription> sd = getRemoteSessionDescription();
	std::string logInfo;
	uint8_t test = 0;
	std::vector<uint8_t> test2;

	//Local Variables - END

	getState()->ping();
	getNetworkManager()->incEth0RX(msg->getDataSize());

	//Routing Table update - BEGIN
	if (router != nullptr)
	{
		if (!router->updateRT(msg, getAbstractEndpoint()))
		{
			//Update: Let us relax things a bit.. routing table might not get updated due to legitimate reasons...

			//tools->->logEvent("[Misbehavior]: Failed to use datagram for routing table update.", eLogEntryCategory::network, 0, eLogEntryType::notification);
			/*if (incMisbehaviorCounter())
			{
				getTools()->logEvent("[Misbehavior]: Peer deemed as malicious. Aborting conversation.", eLogEntryCategory::network,0,  eLogEntryType::notification);
				end(false);
				return false;
			}*/
		}
	}
	//Routing Table update - END

	

	logInfo = "\n---- NetMsg In " +tools->getColoredString(tools->msgEntTypeToString(msg->getEntityType())+ " " + tools->msgReqTypeToString(msg->getRequestType()), 
		eColor::orange)+ ".\n[Endpoint]:"+ getAbstractEndpoint()->getDescription()+ msg->getDescription()+"\n----\n";

	tools->logEvent(logInfo, eLogEntryCategory::network, 0, eLogEntryType::notification);

	//All messages are attempted to be routed if destination field set and if the field does not match the current node's address.
	
	//Decide if NetMsg needs routing first - BEGIN
	//all messages not destined for uncontexed processing are to be routed.
	//that means ANY messages targeting other peers OR to be processed within context of a spawned terminal/web-host

	//Note [for 'wrapper'-datagrams]: routing of these datagrams will be attempted as for any other datagrams.
	/*
	* If, however, the current, utmost onion-layer is envisioned for us (and it should be if we received it, were given such a datagram of type 'wrapper' in the first place),
	* then and only then, we will attempt decryption. The inner - just revealed onion-layer MIGHT contain another 'wrapper', which SHOULD NOT be targeting this node.
	* In such a case, routing of the embedded container will be attempted. At any point, Transmission Tokens are being collected and routing decision is assessed on per-datagram
	* basis.
	*/

	//NOTE: We *ARE* 'routing' even if destined to local Virtual Entities (webrtc conversations, web-socket conversations, DTI terminals etc).
	//IF packet is ENCRYPTED and DESTINED for a locally hosted VE - the VE NEEDS TO take care of decryption.




	/*
	IMPORTANT (for routing): The Conversation ID MIGHT (and usually would) be DIFFERENT at the two endpoints having it.
	Thus, while the destination may be specified in a variety of ways (IP/ID/public-key/address/terminalID/conversation etc.) the conversation ID available at the desired endpoint needs to be used
	for proper routing.  The Identifier of any given conversation MAY BE acquired by accessing the Remote Session's Descriptor (getRemoteSessionDescription()), after the connection has been 
	successfully negotiated (check for null at a very early stage IF negotiation has not yet finished OR failed), thus allowing for issuing of (routing requests/QR Intents) that should be targeting 
	the other endpoint rather then the local node.

	If same destination value is used in multiple entries within the routing table, the routing would proceed on highest-priority-first served basis.
	In any case, a routing table is always updated and used (based on source/destination/sequence) fields of a datagram. 
	*/
	if (getNetworkManager()->needsRouting(msg, getAbstractEndpoint()))
	{
		task = std::make_shared<CNetTask>(eNetTaskType::route,2);
		task->setOrigin(getAbstractEndpoint());//so that the routing table 'knows' the immediate, possibly 'abstract' (websock/UDT conversation) 
		task->setConversation(shared_from_this());//at least one instance already in place; the object is alive
		//^the Conversation member will be used for delivery of failure in routing when such is the case

		//source of the CNetMsg container
		task->setNetMsg(msg);
		getNetworkManager()->registerTask(task);
		//end-processing, nothing more to be done. The Msg is going to be routed away. (note this might simply mean delivery to one of the locally spawned
		//Virtual Terminals and its internal VM.
		return true;

	}
	else
	{
		//attempt to process locally below.
	}
	//Decide if NetMsg needs routing first - END

	//Security - BEGIN

	//Note: despite per-msg verification, there is also verification on the tasks'-level of processing.
	//ex. we might keep processing hello-msgs and continue with a multi-stage hello-handshake but initialization
	//of other scheduled tasks would not happen until secure session is established - IF required.


	
	//Can we process the given msg at the current state of Conversation ?
	if (getIsAuthenticationRequired() && !getPeerAuthenticated() || //encryption and/or authentication needs to be facilitated through NetMsg hello query/respose msgs.
		//it doesn't matter whether session encryption or per-msg AEAD is being used.
		getIsEncryptionRequired() && !getIsEncryptionAvailable())
		
	{
		processOnlyHello = true;
		incAuthAttempts();

		//authentication required for anything else than a hello message
		//attempt to authenticate with the current Peer
		if (getAuthAttempts() > 15)
		{
			
			tools->logEvent("[Misbehavior]: Excessive authorization attempts.", eLogEntryCategory::network,0, eLogEntryType::failure);
		
			//misbehaved peer
			getState()->setCurrentState(eConversationState::ending);
			return false;
		}
	
		
	}
	else
	{
		//authentication was not expected.
	}
	//Decrypt AND verify signature - BEGIN

		/*
		* [ WARNING ]: Do *NOT* decrypt Hello-msgs over here.
		* Reason: These are decrypted within their specialized handlers.
		* These handlers are able to perform additional processing like establishing session secrets based on AEAD containers.
		*/
	if (msg->getEntityType() != eNetEntType::hello)//WARNING!^
	{
		if (msg->getFlags().encrypted)
		{	//do not decrypt hello-messages here. These will be demystified within their specialized handlers.
				//Reason: they might affect session key and we do not want that logic over here.

			if (msg->getFlags().boxedEncryption)
			{
				decryptionKey = Botan::unlock(getPrivKey());
			}
			else
			{
				decryptionKey = Botan::unlock(getSessionKey());
			}

			if (decryptionKey.size() == 0)
			{
				return false;//session key unavailable
			}

			if (!msg->decrypt(decryptionKey, getExpectedPeerPubKey()))//included signature verification IF present and pub-key provided
				return false;
		}
	}
	
		if (msg->getSig().size()==64 && getExpectedPeerPubKey().size()==32)// note that we usually do NOT rely on per-each-container strong signature authentication.
		{//sig had been already verified during decryption IF encrypted
			if (!msg->verifySig(getExpectedPeerPubKey()))
				return false;
		}
		
	
		//Decrypt AND verify signature - END

		if (processOnlyHello && msg->getEntityType() != eNetEntType::hello)
		{
		
			tools->logEvent("[Misbehavior]: At this stage allowed only to process Hello Msgs (" + tools->msgEntTypeToString(msg->getEntityType()) + " received).",
				eLogEntryCategory::network,0,eLogEntryType::failure);

			if (msg->getEntityType() != eNetEntType::ping)
			{
				notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit);

				if (incMisbehaviorCounter())
				{
					tools->logEvent("Peer " + getIPAddress() + " deemed as malicious. Aborting conversation.", "Security", eLogEntryCategory::network, 1, eLogEntryType::notification);
					endInternal(false, eConvEndReason::security);
					return false;
				}
			}
	
			return false;
	    }

	//Security - END


	bool doSwarmAuth = false;
	switch (msg->getRequestType())
	{
	//	[20.12.20] no explicit routing decisions made by data-originator (ex. mobile)
	//full-node decides autonomously on its own, always.
	//case eNetReqType::eNetReqType::route:

	case eNetReqType::eNetReqType::route:
		//do NOTHING with incoming route requests. Explicit routing intent for external sources is currently disabled.
		//full-node decides autonomously what to do, based on the destination field.

		//[22.02.23]: we now employ explicit 'route' requests when WebRTC signaling datagrams are disseminated across core nodes.
		//so that we know that the datagram arrived from another Core node, and so that we know how to treat it.

		
		switch (msg->getEntityType())
		{
			//WebRTC Swarms Support - BEGIN
			case eNetEntType::Swarm:

			sdpMsg = CSDPEntity::instantiate(msg->getData());
			
			if (sdpMsg == nullptr)
			{
				
				tools->logEvent("Invalid Wrapper received from " + getEndpoint()->getDescription(),"SECURITY", eLogEntryCategory::network, 1, eLogEntryType::warning);
				issueWarning();
				return false;

			}

			sdpMsg->setHopCount(msg->getHops());
		

			handleProcessSDPMsg(sdpMsg, socket, getWebSocket());

			break;
			//WebRTC Swarms Support - END
		}
	
		break;

	
	case eNetReqType::eNetReqType::notify:
		switch (msg->getEntityType())
		{

		case eNetEntType::ping:
			pingReceived();
			break;
		case eNetEntType::sessionInfo:
			//we got here so no routing was needed so supposedly it's log-me-in response
			if (mScriptEngine->getRunningAppID() == eCoreServiceID::logMeIn)
			{
				mScriptEngine->setExternalData(msg->getData());
			}
			
			break;

		case eNetEntType::wrapper://(tunneling)

			//there should be a valid, decrypted by now, BER-encoded CNetMsg container within.
			if (msg->hasData())
			{
				offspringMsg = CNetMsg::instantiate(msg->getData());
			}

			if (!offspringMsg)
			{
				tools->logEvent("[Misbehavior]: Invalid Wrapper received from " + getEndpoint()->getDescription(), eLogEntryCategory::network, 0);

				notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit);
				
				if (incMisbehaviorCounter())
				{
					tools->logEvent("Peer " + getIPAddress() + " deemed as malicious. Aborting conversation.", "Security", eLogEntryCategory::network, 1, eLogEntryType::notification);
					endInternal( false, eConvEndReason::security);
				
				}
				return false;
			}


			//test2 = offspringMsg->getImage(false);
			//test2 = mCryptoFactory->getSHA2_256Vec(offspringMsg->getData());
			test = *(reinterpret_cast<uint8_t*>(&offspringMsg->getFlags()));

			//now, once we have unveiled the encapsulated datagram, let us process it as usually
			//all the incentivization checks should have been completed by now.
			//Note: there should be just a single onion layer destined for the local node. Anyway we allow for up to 3 recursions (in case  someone realy insisted
			//on onion routing across web-socket conversations/web-rtc swarms hosted by the local node).
			return processMsg(offspringMsg, socket, webSocket, recursionStep);//todo: reconsider recursion in this place
			//there's a safety counter allowing for no more than 3 recursion steps, checked in function's header.
			
			break;

		case eNetEntType::OperationStatus:
			operationStatus = COperationStatus::instantiate(msg->getData());
			if (operationStatus == nullptr)
				return false;

			setRecentOperationStatus(operationStatus);

			if (operationStatus->getID().size() > 0)
			{
				while (!tasksTemp.empty())
				{
					task = tasksTemp.top();
					if (task && task->getType() == eNetTaskType::awaitOperationStatus && operationStatus->getReqID() == task->getMetaRequestID())
					{
						task->setState(eNetTaskState::completed);
						break;
					}


					tasksTemp.pop();
				}
			}

			if (operationStatus->getReqID() && operationStatus->getID().size())
			{
				if (operationStatus->getStatus() == eOperationStatus::failure)//block id and request-id should be both present
				{
				
					markBlockRequestAsAborted(operationStatus->getReqID());//introduce flags indicating what type of object the requst is related to or employ the VM-meta-data exchange protocol?
				//markBlockRequestAsAborted(operationStatus->getID());
				}
			}
			//else
			//	emit transactionRegistrationFailedSig();

			//mark the 'awaiting receiptID' task as completed if there is one
			//note, this implicitly assumes that to be completed task, the one currently being resolved is the one with highest priority
		
			break;
		case eNetEntType::QRIntentResponse://deprecated, remove [20.12.20]
			handleProcessQRIntentResponseMsg(msg, socket, getWebSocket());

		case eNetEntType::longestPath://remove?
			handleNotifyLongestPathMsg(msg, socket, getWebSocket());
			break;
		case eNetEntType::chainProof://another peer claims to have the heaviest history of events.
			//chain-Proof allows for autonomous verification of such a  claim. No additional data needed.
			//chain-proof thus is comprised of a sequence of block-headers.

			handleNotifyChainProofMsg(msg, socket, getWebSocket());
			break;
		
		case eNetEntType::block:

			handleNotifyNewBlockMsg(msg, socket, getWebSocket());//the handler decides whether we are interested in a particular block; requests a chain-proof if we are.
			break;
		case eNetEntType::blockBody:
			//only available as 'process' action. (not 'notify').
			
			//handleNotifyNewBlockMsg(msg, socket, getWebSocket());
			break;
		case eNetEntType::msg:

			handleNotifyNewChatMsg(msg, socket, getWebSocket());
			break;
		case eNetEntType::sec:
			break;
		case eNetEntType::transaction:
			handleNotifyTransactionMsg(msg, socket);
			break;
		case eNetEntType::verifiable:

			break;
		case  eNetEntType::hello:

			if (getLogDebugData())
				tools->logEvent(tools->getColoredString("[ Handling a Hello-Notification (Stage 3) ]: ", eColor::orange)+msg->getDescription() , eLogEntryCategory::network, 0);

			if (!handleNotifyHelloMsg(msg, socket, getWebSocket()))
			{
				//abort, authentication must have failed
				getState()->setCurrentState(eConversationState::ending);
				notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit);
			}

			break;

		case eNetEntType::bye:
			handleNotifyByeMsg(msg, socket, getWebSocket());
			break;
		}     
		break;
	case eNetReqType::eNetReqType::process:
		switch (msg->getEntityType())
		{
		case eNetEntType::Kademlia:
			handleProcessKademliaMsg(msg, socket);
			break;
		case eNetEntType::QRIntentResponse://deprecated, remove [20.12.20]
			handleProcessQRIntentResponseMsg(msg, socket);
			break;
		case eNetEntType::longestPath:
			handleProcessLongestPathMsg(msg, socket);
			break;
		case eNetEntType::chainProof:
			
			handleProcessChainProofMsg(msg, socket);
			break;
		case eNetEntType::block:
		
			handleProcessBlockMsg(msg, socket);
			break;
		case eNetEntType::blockBody:
			if (!handleProcessBlockBodyMsg(msg, socket))
			{
				issueWarning();
			}
			break;
		case eNetEntType::msg:
			break;
		case eNetEntType::sec:
			break;
		case eNetEntType::transaction:
			handleProcessTransactionMsg(msg, socket);
			break;
		case eNetEntType::verifiable:
			handleProcessVerifiableMsg(msg, socket);
			break;
		case eNetEntType::VMMetaData:
			handleProcessVMMetaDataMsg(msg);
			break;
		}
		break;
	case eNetReqType::eNetReqType::request:

		switch (msg->getEntityType())
		{

		case eNetEntType::hello:

			/*
				Discussion: It is fine for both peers to exchange
				Hello-Requests, under the assumption that ephemeral key-pairs
				are generated once per session and thus results of any further
				Diffie-Hellman key-exchanges would remain the same. New key-pair
				would be generated once a new connection is established. With
				that said, for efficiency, only one peer shall generate a
				hello-request (the peer receiving an incoming connection).

				It is also important for ONLY the incoming peer to generate this datagram first
				since otherwise we open ourselves up to a race condition in which the one party would
				assume session key to be already available (and send payload) while the other renegotiates.

				Further, we should NOT treat consecutive handshake attempts as
				erroneous and/or as misbehaved. The protocol DOES support
				renegotiation of connection parameters for already established
				communication sessions.

				- Recall that data-exchange might be - :
				1) session-encrypted (symmetric stream cipher, with key
				   established through ECDH)
				2) per message ECIES-encrypted
				3) no-encryption

				- Additionally, we support following authentication modes - :
				  1)no authentication 2)StrongAuth - When public key is A Priori
				  known to verifier a) Session-Based - one time strong
				  authentication during hello-handshake based on strong EC
				  signatures. Note: for full verification, the expected public
				  key needs to be provided BEFORE a connection attempt is made.
				  On server side, the public key can be compared against what
				  available within the decentralized state-machine. Any
				  consecutive datagrams are authenticated under the strength of
				  Poly1305. b) Per CNetMsg - strong signature based
				  authentication on per message basis. Additionally provides
				  full accountability on per-message basis. ECC signature is
				  attached to each and every datagram. Should be avoided until
				  really needed due to performance and efficiency reasons.
				  3)WeakAuth - for when the public key is not knows beforehand
				  to verifier, yet authentication data is provided

				- Additionally, there are two ways to facilitate authentication
				  -
				+ strong signatures (256bit security)
				+ AEAD-container/(Poly1305+ECDH) In  the latter authentication
				  is based on public key provided within AEAD container and on
				  the fact that the resulting ECDH secret, and thus given
				  Poly1305 token securing given data could be generated only by
				  owner of a given private key. The algorithm offers 128bits of
				  security (Poly1305), which is still quite a lot, but due to
				  half of crypto-strength when compared with prior,a 'WeakAuth'
				  naming convention is used.
			*/
			// if (getWasChallangeResponseDispatched())
			// {
				 //misuse attempt; abort
			 //    getState()->setCurrentState(eConversationState::ending);

		   //      break;
			 //}

			//if (getWasChallangeResponseDispatched())
			//{
				//misuse attempt; abort
			//	getState()->setCurrentState(eConversationState::ending);
			//	break;
			//}
			
			if (!handleRequestHelloMsg(msg, socket, getWebSocket()))
			{
				notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit);
				getState()->setCurrentState(eConversationState::ending);
			}
			else
			{
				setWasChallangeResponseDispatched();//attempt made is enough to assume we did it.
			}

			break;
		case eNetEntType::longestPath:
			handleRequestLongestPathMsg(msg, socket);
			break;
		case eNetEntType::chainProof:
			handleRequestChainProofMsg(msg, socket);
			break;
		case eNetEntType::block:
			handleBlockRequestMsg(msg, socket);
			break;
		case eNetEntType::blockBody:
			handleBlockBodyRequestMsg(msg, socket);
			break;
		case eNetEntType::msg:
			break;
		case eNetEntType::sec:
			break;
		case eNetEntType::transaction:
			handleTransactioneRequestMsg(msg, socket);
			break;
		case eNetEntType::verifiable:
			handleVerifiableRequestMsg(msg, socket);
			break;
		case eNetEntType::DFS:

			 dfsMsg = CDFSMsg::instantiate(msg->getData());


			 //Thread Commit Abort Check - Begin
			 //That covers the System-Thread only.
			 if (msg->getEntityType() == eNetEntType::DFS || msg->getEntityType() == eNetEntType::VMMetaData)
			 {
				 std::shared_ptr<SE::CScriptEngine> se = getSystemThread();
				 if (dfsMsg && se != nullptr && se->getIsWaitingForVMMetaData() && tools->compareByteVectors(dfsMsg->getThreadID(),se->getID()))//ONLY protects the System Thread
				 {//give a grace period of 20 seconds before the Global Commit operation can be aborted/disturbed

					 size_t now = tools->getTime();
					 if ((now - se->getQRWaitStartTime()) >= 20)
					 {
						 se->setIsWaitingForVMMetaData(false);
						 enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(), 0, "Commit Aborted");//todo: keep track of taskID by VM
					 }
				 }
			 }// otherwise, the received message is VM-agnostic, we might enqueue it and do the processing right away (in accordance to its priority).
			 //Thread Commit Abort Check - End


			 if (dfsMsg == nullptr) {
				 getState()->setCurrentState(eConversationState::ending);
			 }
			else
			{
				if (!getBlockchainManager()->getIsReady())
				{
					return false;
				}
				handleProcessDFSMsg(dfsMsg, socket, getWebSocket());
			}
			break;

		case eNetEntType::Swarm:

			sdpMsg = CSDPEntity::instantiate(msg->getData());
			if (sdpMsg == nullptr)
				getState()->setCurrentState(eConversationState::ending);

			handleProcessSDPMsg(sdpMsg, socket, getWebSocket());
			
			break;
		}
		break;
	default:
		break;

	}
	return false;
}

/// <summary>
/// Requests the longest (in terms of PoW) path from the other peer.
/// Provides 3 possible sync-points to the other peer. The data is provided as a sequence of vectors within the data field.
/// If, no sync points are provided the other node is supposed to provide a full path (starting from its genesis block).
/// The other node is supposed to provide a longest path starting from one of the requested sync-points.
/// </summary>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::requestLongestPath(UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{
	//Notice:
	/*
	* Longest Path - sequence of block IDs
	* Chain-proof - sequence of the actual Block Headers.
	*/
	return false;
}

/// <summary>
/// Provides a response with a sequence of blockIDs within the data field.
/// </summary>
/// <param name="netmsg"></param>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::handleRequestLongestPathMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	//Notice:
	/*
	* Longest Path - sequence of block IDs
	* Chain-proof - sequence of the actual Block Headers.
	*/

	//1) try to find any of the provided sync points

	//2) check if those sync-points lead to the local assumed history of events

		//if so - delivery Longest Path starting at that point
		//if not - deliver entire Longest Path (optimize)

	//3 if no sync-points are part of the proclaimed history of events - deliver the entire Longest Path.
	return false;
}

/// <summary>
/// Requests the longest (in terms of PoW) ChainProof (sequence of headers) from the other peer.
/// Provides 3 possible sync-points to the other peer. The data is provided as a sequence of vectors within the data field.
/// If no sync points are provided the other node is supposed to provide a ChainProof (sequence of headers starting from the genesis block).
/// The other node is supposed to provide a sequence of headers starting from one of the requested sync-points (blockIDs).
/// </summary>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::requestChainProof(UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	//currently handled immediately upon reception of a block header.
	return false;
}

/// <summary>
/// Provides the best known ChainProof to the other node.
/// </summary>
/// <param name="netmsg"></param>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::handleRequestChainProofMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	//Local Variables - BEGIN
	const uint64_t largeDataThreshold = 1000000;
	std::shared_ptr<CNetTask> task;
	std::vector<std::vector<uint8_t>> checkpoints, proof;
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	std::shared_ptr<CTools> tools = getTools();
	std::string ip = getIPAddress();
	std::vector<uint8_t> packedProof;
	bool isPartialProof = false;
	//Local Variables - END

	tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " processing chain-proof request from " + ip +".", eLogEntryCategory::network, 1, eLogEntryType::notification);
	
	if (bm->getSyncPercentage() < 95)// optimization. Do not allow ourselves to get suffocated by chain-proof requests
	{								 // when those requests regard both old periods of time and when we are occupied with syncing ourselves.
		tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " neglecting chain-proof request from " + ip + ".", eLogEntryCategory::network, 1, eLogEntryType::warning);
		return false;
	}

		tools->BERVectorToCPPVector(netmsg->getData(), checkpoints);
		if (checkpoints.size())
		{
			
			/// Key-Block identifiers, present within checkpoints are  ordered from the latest, to the oldest
			/*As perceived by the other peer. Thus checkpoints represents its known leading block and the last identifier MIGHT
			* represent the Genesis Block.
			* 
			* Since we want the produced chain-proof to be of shortest length, we attempt generating chain-proof starting from the most recent block,
			* yet again - as known by the other peer.
			*/
			for (uint64_t i = 0;i < checkpoints.size(); i++)
			{//attempt to retrieve a chain-proof for any of the provided checkpoints

				// key blocks in checkpoints are ordered from latest to the earliest.
				// [ SECURITY ]: this is not expected to cause excessive congestion - a double buffer is used for data retrieval.
				if (bm->getChainProofForBlock(eChainProof::verifiedCached, proof, checkpoints[i]))
				{
					isPartialProof = true;
					tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " constructed partial chain-proof for " + ip + ".", eLogEntryCategory::network, 1, eLogEntryType::notification, eColor::lightGreen);
					break;
				}
			}
		}

		if (!proof.size())
		{//retrieve the bm chain-proof
			//Notice: if no checkpoints were provided, the peer would provide its entire version.
		
			
			/*if (!bm->getChainProofForBlock(eChainProof::verified, proof))
			{
				tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + "  unable to prepare chain-proof for " + ip + ".", eLogEntryCategory::network, 1, eLogEntryType::failure, eColor::cyborgBlood);
				notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit);
				return false;
			}

			tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)  + "  constructed full chain-proof for " + ip + ".", eLogEntryCategory::network, 1, eLogEntryType::notification, eColor::orange);
			*/

			//^ we do not do the above anymore. It is computationally expensive. Instead when full chain-proof is needed we simply retrieve the packed vector from Cold Storage and deliver as is.
			packedProof = bm->getChainProofPacked(eChainProof::verified);

			if (!packedProof.size())
			{
				tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + "  unable to prepare chain-proof for " + ip + ".", eLogEntryCategory::network, 1, eLogEntryType::failure, eColor::cyborgBlood);
				notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit);
				return false;
			}
		}
		


		task = std::make_shared<CNetTask>(eNetTaskType::notifyChainProof, 1);

		if (packedProof.size()> largeDataThreshold ||((proof.size() * 300) > largeDataThreshold))
		{
			requestDeferredTimeout(false);
		}

		task->setData(isPartialProof?tools->BERVector(proof): packedProof);
		return addTask(task);
		
}

/// <summary>
/// Make a request for the current block header.
/// </summary>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::requestLatestBlockHeader(UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

/// <summary>
/// Handles request for the header of the Leader Block.
/// </summary>
/// <param name="netmsg"></param>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::handleRequestLatestBlockHeader( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}
/// <summary>
/// Gets called in response to a Hello-Request (hello param).
/// The function CAN authenticate the other peer on-sight IF expected public key is known IF the other peer used useAEADpubKeyForSession flag.
/// Otherwise, if authentication required, the function will initiate a challenge-response signature-based procedure.
/// Facilitates the second part of a Hello-handshake, the received Hello Request needs to be provided as parameter.
/// Within mData-conversation ID (from local perspective)
/// Within extraBytes - either challenge to be signed, or signature itself (specified by eNetReq type of the Hello msg)
/// IF, the function return FALSE, the conversation shall be aborted.
/// 
/// </summary>
/// <param name="socket"></param>
/// <param name="webSocket"></param>
/// <returns></returns>
bool CConversation::notifyHelloMsg(std::shared_ptr<CNetMsg> helloRequest, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{					//     ^----- this effectively is the Hello Request message handler.

	/*
		IMPORTANT: each Hello Message Handler needs to take care of DECRYPTION on its own. Standard pre-processing is not applies to these.
	*/

	/*
	The received Hello Request may:
		- session encrypted
		- direct-ECIES encrypted 
		- non-encrypted at all.
	*/

	/*
		IMPORTANT: for a NEW session key to be generated, the other peer MUST have delivered Session Description encapsulating the Communication Public Key.

		[ Typical Usage Scenario ]: Typically we would rely on DH-based session key derived based on Communication Private/Public key-pairs - with the public key of the other party received
						   from the other peer as part of Hello Request.

		[ Race Conditions ]: it is fine for both peers to receive Hello Requests as part of the same connection. 
							Both parties would arrive at the very same session key - based on the most recent handshake.
	*/

	/*
	[Meeting 03.01.20] - we eliminate one round trip, peer B is to reply with SessionDescription instead of ConversationID within the data field.
	Hello-response is to be ECIES-box encrypted to node's A public key so that node A can get to know peer's B public key.
	Any consecutive messages are to be encrypted with established secret i.e. in session mode.

	Message's flags indicate the current encryption mode used.

	//The Hello-Request MIGHT be ECIES-box encrypted (initial one - if A knew our public key) OR already session encrypted IF already session key established and the other peer
	//wants to verify our identity using the signature challenge response or simply to renegotiate shared secret.
	*/
	if (helloRequest == nullptr)
		return false;
	//[meeting 20.12.20] conversation ID shall be dispatched to peer within Hello Msg [DEPRECATED: use CSessionDescription for that].
	//The ID might be then used for WebSocket/UDT clients and by full-node itself to route for particular web-socket sessions.
	//Another usage scenario: QR-intent might target particular web-socket session and the full-node may route accordingly once CNetMsg is received from a mobile application.

	//Local Variables - BEGIN
	std::shared_ptr<CNetMsg>  msg = std::make_shared<CNetMsg>(eNetEntType::hello, eNetReqType::notify);
	std::vector<uint8_t> data = helloRequest->getData();
	Botan::secure_vector<uint8_t> sessionKey = getSessionKey();
	std::shared_ptr<CSessionDescription> sd;
	std::vector<uint8_t> expectedPeerPub = getExpectedPeerPubKey();
	std::vector<uint8_t> sendersPubKey;
	conversationFlags cFlags = getFlags();
	Botan::secure_vector<uint8_t> sessionKeyAEADOut, sessionKeyAEADIn, sessionKeyDH;
	std::vector<uint8_t> decryptionKey;
	Botan::secure_vector<uint8_t> establishedSessionKey;
	std::shared_ptr<CTools> tools = getTools();
	std::vector<uint8_t> sig;
	bool logDebug = getLogDebugData();
	bool AEADBoxArmed = false;// was Poly1305 authentication used? If not, we CANNOT use this for authentication of the other peer.
	//Local Variables - END

		if (logDebug)
			tools->logEvent("Preparing a Hello-Notification.", eLogEntryCategory::network, 0, eLogEntryType::notification);

	/*
	When hello-request is received it is assumed that the other peer wants to renegotiate session-key. As such, we forget the current key and establish a new secret based on
	the provided public key within received SessionDescription. As such, the hello request may be either session or direct-ECIES encrypted or no encryption may be used at all.
	
	The PREVIOUS session key MUST BE forgotten AS-SOON-AS the hello-request is possibly decrypted.

	Note that in order to thwart middleman being able reset connection between two peers after successful network-layer address spoofing,
	(the WiFi-like susceptibility to authentication attacks) the protocol needs to operate in full-authenticated mode where sender is verified through a cryptographic signature.
	*/

		
			// NOTICE: the entire Hello Request datagram MIGHT be encrypted (i.e. when re-keying).
			
			// Session-Key based on a Public Key found within of AEAD container - BEGIN
			if (helloRequest->getFlags().encrypted)
			{
				if (msg->getFlags().boxedEncryption)
				{// ECIES encrypted
					decryptionKey = Botan::unlock(getPrivKey());
				}
				else
				{// encrypted with an already well established(?) Session Key
					decryptionKey = Botan::unlock(sessionKey);
				}

				if (decryptionKey.size() == 0)
				{
					return false; //session key unavailable
				}
				// [ Session Key Derivation 1 ]: derive session key from a public key found within of an AEAD container.
				// Here, it is attempted to establish a session key based on an already delivered AEAD container (within or Hello Request).
				// note that session key can be derived also from AEAD construct during outgoing data disparagement [ Session Key Derivation 3 ].
				if (helloRequest->getFlags().useAEADpubKeyForSession)
				{
					//Note: this would result in an AEAD-in session key only if Hello Request was already encrypted through ECIES (ChaCha20c25519).
					if (!helloRequest->decrypt(decryptionKey, std::vector<uint8_t>(), sessionKeyAEADIn, sendersPubKey, AEADBoxArmed))//Note: usage of sendersPubKey below for authentication (to prevent signature-based authentication)
						return false;
				}
			}

			// Session Description - BEGIN
			if (helloRequest->getFlags().sessionDescription && data.size() > 0)
			{
				sd = CSessionDescription::instantiate(data); // attempt to retrieve Session Description

				if (sd == nullptr)
				{
					if (logDebug)
					{
						issueWarning();// since invalid data received, under the assumption that SessionInfo is required within Hello-Request.
						if (logDebug)
						tools->logEvent(tools->getColoredString("Peer provided INVALID SessionInfo", eColor::lightPink), eLogEntryCategory::network, 0, eLogEntryType::failure, eColor::lightPink);
					}
					return false;//terminate connection;
				}

				if (logDebug)
				{
					if (sd)
					{
						if (logDebug)
						tools->logEvent(tools->getColoredString("[ Received HR SessionInfo ]: ", eColor::orange) + sd->getDescription(), eLogEntryCategory::network, 0);
					}

				}
			}
			else
			{
				issueWarning();

				if(logDebug)
				tools->logEvent(tools->getColoredString("Peer has not provided HR SessionInfo", eColor::cyborgBlood), eLogEntryCategory::network, 0,eLogEntryType::failure,eColor::lightPink);
				return false;//terminate connection;

			}

			// Session Description - END
			
			//Forget the session key - BEGIN
			
			// if the other party requested new handshake, the current session key is to be DESTROYED.
			// even if it means no session key available.
			//sessionKey.clear();
			//Forget the session key - END

			//Session-Key based on a Public Key found within of AEAD container - END

			//Note: Session Description might be about a lot more than session key derivation. 
			//	    We thus store the Session Description EVEN if session key is to be derived implicitly from the received AEAD container.
			//      Note the difference between session based on AEAD-in and AEAD-out.
		

			setRemoteSessionDescription(sd);// notice that Remote Session Description is set twice right when Hello Request received and once received (is this correct?)
			if (sd)
			{// if remote Session Description is available we MAY use strong authentication IF the other peer's public key has been made available as part of the Session Description.
		     // Notice that there are two public keys 1) the communication-layer public key (used by ECIES) and 2) the peer's currenty identity-related public key.
				// For authentication purposes we shall use the latter.


				// we fetch the key from remoteSessionDescription as setExpectedPeerPubKey() is meant for session-wide per-msg strong auth only.
				//setExpectedPeerPubKey(sd->getPubKey());// this has nothing to do with session key. It is used to authenticate remote peer through an explicitly signed challange-response mechanism.
				// the signed challange would be verified to match the public key in CSessionDescription delivered earlier thorugh a Hello Request.
			}

			if (!helloRequest->getFlags().useAEADpubKeyForSession)
			{//only if we do NOT want to rely on session key derived from the AEAD-in container.
				//Session Key based on an explicit Public Key found within of Session Description - BEGIN

					// [ Session Key Derivation 2]: derive session key by performing Diffie-Hellman handshake between Public Key in Session Description and the Communication Private Key.

					// attempt to prepare session-key based on a Public Key from within of Session Description.
				if (sd->getComPubKey().size() == 32)
				{
					sessionKeyDH = mCryptoFactory->ECDH(sd->getComPubKey(), getPrivKey());
				}

				//Session Key based on explicit public key within Session Description - END
			}

			//If session key established, any future CNetMsgs would be encrypted with endChaCha20() instead of ECIES.

			// [Important]: the new session key will be set AFTER the message is sent, for the session-based encryption to take effect afterwards.

		

		//Initial pre-Authentication - BEGIN
		// Note: this is 'only' to AUTHENTICATE the peer. This is NOT involved in key-establishment.
		//if expected public key is known and the delivered one does not match then abort right now

		if (getIsAuthenticationRequired())
		{
			// [IMPORANT]: IF useAEADpubKeyForSession is set then we have verified the other peer under the strength of the other peer's 
			// private key and of the Poly1305 authentication function. In such a case no additional challenge-response signature based verification is required. 
			sendersPubKey = (helloRequest->getFlags().useAEADpubKeyForSession && AEADBoxArmed && helloRequest->getFlags().encrypted)? sendersPubKey : sd->getComPubKey();

			/*
				Authentication can take place in two ways:
				1) challenge-response, signature of random data generated by the other peer.
				2) if peer used its Communication Private Key for ECIES/AEAD-box - we can authenticate it under the presumed strength of its private key and the Poly1305 construct.
			*/
			if (expectedPeerPub.size() > 0)
			{
				if (sd == nullptr || !tools->compareByteVectors(expectedPeerPub, sendersPubKey))
					return false;
			}

			if (helloRequest->getFlags().authenticated && helloRequest->getFlags().encrypted && helloRequest->getFlags().useAEADpubKeyForSession && helloRequest->getFlags().encrypted)
			{
				{
					std::lock_guard<std::mutex> lock(mFieldsGuardian);
					mPeerAuthenticatedAsPubKey = sendersPubKey;// [IMPORTANT]: the authenticated identity is also kept track of, at the level of CScriptEngine (available through the log-me-in mechanics). Notice dti->setLoggedInIDToke()
					mPeerAuthenticated = true;//authenticated decryption succeeded
				}
				
				if (logDebug)
					tools->logEvent(tools->getColoredString("Remote peer is now authenticated.", eColor::lightGreen) + msg->getDescription(), eLogEntryCategory::network, 0, eLogEntryType::notification);
			}
		}
	
		//pre-Authentication - END

		//challenge/response authentication - BEGIN (optional if extraBytes> 0 - means other peers wants us to provide a signature)
		//authenticate ourselves in the eyes of the other peer.
		//this can also be achieved by using useAEADpubKeyForSession for outgress response

		//as per the 03.01.21 meeting, we've moved challenge data from extraBytes field to a dedicated field within SessionData
		//thus the entire thing gets encrypted mitigating/thwarting association of peers with themselves based on Session-handshakes.

		if (getDoSigHelloAuth()&& sd->getChallangeData().size() > 0)//sign the challenge data
		{
			sig = CCryptoFactory::getInstance()->signData(sd->getChallangeData(), getPrivKey());
			//challenge response will be made operational few lines below once session description gets initialized
		}
		
		//challenge/response authentication - END


   msg->setSourceType(eEndpointType::IPv4);//compare against with what a connection was attempted to be made with, at Web-Nodes.No: easy to spoof
	
	//msg->setData(getID()); DEPRECATED [meeting 03.01]

	
	//Generate Local Session Description - BEGIN
	CSessionDescription lsd(*sd);
	lsd.setConversationID(getID());
	lsd.setComPubKey(getPubKey());
	lsd.setChallangeData(sig);//challenge response made operational
	msg->setData(lsd.getPackedData());
	//Generate Local Session Description - END

	//Update Flags - BEGIN
	nmFlags mFlags = msg->getFlags();
	mFlags.sessionDescription = true;
	
	if (cFlags.localIsMobile)
	{
		mFlags.fromMobile = true;
	}

	//make flags effective
	msg->setFlags(mFlags);
	//Update Flags - END
	
	//Authentication and Encryption - BEGIN


	// [Rationale]: we employ a dedicated call to authAndEncryptMsg() instead of imposing a need of relying on implicit, internal sendMsg() encryption
	//			  so to be able to already set the session ket is AEAD-by-product session key is to be established.
	// IMPORTANT: below, we invoke sendNetMsg() with 'doPreprocessing' flag set.
	if (!authAndEncryptMsg(msg, sessionKeyAEADOut)) // Hello Response will be ECIES-box encrypted since session key is not set yet (since it is before this very function returns).
	{										 
		if(logDebug)
		tools->logEvent("AuthEnc failed for "+getIPAddress()+": "+ msg->getDescription(), eLogEntryCategory::network, 0, eLogEntryType::failure, eColor::lightPink);
		return false;//session encryption cannot be used yet since we assume A not to know B's public key as of yet. need to deliver.
	}

	// Proclaim New Session Key - BEGIN
	// Session Key is assumed based on requirements dictated by the other peer.
	std::string sessionKeyType;
	if (getUseAEADForSessionKey())
	{
		if (sessionKeyAEADIn.empty() == false)
		{
			sessionKeyType = "AEAD-In";
			establishedSessionKey = sessionKeyAEADIn;

		}
		else if (sessionKeyAEADOut.empty() == false)
		{
			sessionKeyType = "AEAD-out";
			establishedSessionKey = sessionKeyAEADOut;
		}
		else
		{
			if (logDebug)
				tools->logEvent("Unable to establish AEAD-based session key.", eLogEntryCategory::network, 0, eLogEntryType::failure);

			return false;
		}
	}
	else if (sessionKeyDH.empty() == false)
	{
		sessionKeyType = "DH-ECC";
		establishedSessionKey = sessionKeyDH;
	}
	else
	{
		tools->logEvent("Unable to establish Diffie-Hellman-based session key.", eLogEntryCategory::network, 0, eLogEntryType::failure);
	}
	// Proclaim New Session Key - END

	
	//Authentication and Encryption - END

	setSessionKey(establishedSessionKey);//set session key BEFORE dispatching data to avoid race conditions.

	bool sent = sendNetMsg(msg, true, false);
	//									 ^--- *DO NOT* attempt pre-precessing. Handshake stage has dedicated Auth routine above.
	//											Todo: Since  we might be in the process of re-keying and session key MIGHT be already well established.
	//												  This might then count as 'session continuation', or 're-keying' instead of a brand new session.								          

	if (sent) {//we want the first response to use direct ECIES IF session key had not been established already before function's invocation. 
	//otherwise the other peer wouldn't be able to decrypt sessionDescription and get to know out public key were we to encrypt with session key

		if (logDebug)
			tools->logEvent("----- [ "+tools->getColoredString("Established ", eColor::lightGreen) + // notice how priority --v is used to throttle DEBUG messages (if enabled).
				tools->getColoredString(sessionKeyType, eColor::blue)+" Session Key."+" ] -----", eLogEntryCategory::network, 3, eLogEntryType::notification);
	}
	else
	{
		setSessionKey(Botan::secure_vector<uint8_t>()); // revert the session key.

		if (logDebug)
			tools->logEvent("Failed to send:" + msg->getDescription(), eLogEntryCategory::network, 0, eLogEntryType::failure);
	}

	return sent;
}

/// <summary>
/// The function is responsible for preparing and delivering a Hello Request as per the Conversation's required properties (enc/auth etc).
/// That's the first part of a (possibly)two-step handshake (if auth/encryption enabled).
/// Note that node B MIGHT issue an additional hello-request even after session has been established IF it wills to verify node's B identity using the challenge-response signature method.
/// 
/// </summary>
/// <param name="socket"></param>
/// <param name="webSocket"></param>
/// <returns></returns>
bool CConversation::requestHelloMsg(UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{
	//[meeting 20.12.20] conversation ID shall be dispatched to peer within
	//Hello Msg the ID might be then used for WebSocket/UDT clients and by
	//full-node itself to route for particular web-socket sessions. another
	//usage scenario: QR-intent might target particular web socket session and
	//the full-node may route accordingly once CNetMsg received from a
	//mobile-app. [morning meeting 29.12.20]: the Hello msg is to facilitate a
	//challenge-response authentication mechanism, thus we distinguish between a
	//query and notification(response). for security, the fact of already having
	//responded to challenge SHOULD be remembered by the Conversation object.


	/*
			[ Methods Involved ]: 
				- requestHelloMsg() - both peers execute, as part of the startConversation asynchronous NetTask. Local CSessionDescription is provided to remote peer.

									// [ IMPORTANT ]: see Dual Nature of CSessionDescription in sessionDescription.h
									  [ CSessionDescription Handling]: - there are two public keys. 1) the Ephemeral Communication Layer Public Key (possibly used by ECIES)
																				                   2) the Peer's Identity Public Key - basically a public key corresponding to user's wallet.
																		 Now, a peer MUST provide 1) while the 2) is OPTIONAL, meaning a peer MAY provide.
																		 [ Use Case ]: the mobile app provides 2) to authenticate itself during Strong-Auth so that a corresponding routing table entry
																			           in a Core node could be created. There usually is little need for mutual strong-authentication in between of Core nodes.



			    - notifyHelloMsg()  - takes care of Hello Request processing as well as of CSessionDescription.
				- handleRequestHelloMsg() - basically a wrapper around notifyHelloMsg().
				- handleNotifyHelloMsg()  - handleNotifyHelloMsg() once completed, the DH handshake is concluded, session is established.

			    Notice that dependently on session-key establishment method used, or whether re-keying, peers would be arriving at session key at different stages.
	*/

	//Local Variables - BEGIN
	std::shared_ptr<CNetMsg>  msg = std::make_shared<CNetMsg>(eNetEntType::hello, eNetReqType::request);
	CSessionDescription sessionDescription; // IMPORTANT: see  Dual Nature of CSessionDescription in sessionDescriptin.h
	std::vector<uint8_t> packed;
	std::vector<uint8_t> challange;
	Botan::secure_vector<uint8_t> sessionKey;//(optional)
	bool logDebug = getLogDebugData();
	std::shared_ptr<CTools> tools = getTools();
	//Local Variables - END

	if (logDebug)
		tools->logEvent("Preparing a Hello-Request.", eLogEntryCategory::network, 0, eLogEntryType::notification);

	//msg.setSource(tools->stringToBytes(mNetworkManager->autoDetectPublicIP()));
	msg->setSourceType(eEndpointType::IPv4);//compare against with what a connection was attempted to be made with, at Web-Nodes.

	//Prepare Session Description - BEGIN
	// [ IMPORTANT ]: see Dual Nature of CSessionDescription in sessionDescription.h

	sessionDescription.setConversationID(getID()); // (possibly) used by routing.
	sessionDescription.setComPubKey(getPubKey()); // set an ephemeral public key to-be-used by our Diffie-Hellman construct.

	// Peer Authentication (optional) - BEGIN
	// [ IMPORTANT ]: see Dual Nature of CSessionDescription in sessionDescription.h
	if (getDoSigHelloAuth())
	{	// enable for another peer to authenticate itself.
		challange = tools->genRandomVector(12);
		setRequestedChallange(challange);

		sessionDescription.setChallangeData(challange);// the challenge to be signed (when mutual peer authentication enabled).
	} 
	// Peer Authentication (optional) - END

	msg->setData(sessionDescription.getPackedData()); // mandatory

	//Prepare Session Description - END

	//Update flags - BEGIN
	nmFlags flags = msg->getFlags();
	flags.sessionDescription = true;
	msg->setFlags(flags);
	//Update flags - END
   
	//Authentication and Encryption (AEAD) - BEGIN
	//							    v---- when AEAD-container session key establishment enabled. We can get session key right now. (STEP R1).
	if (!authAndEncryptMsg(msg, sessionKey, true))//encryption of an outgress hello request is not required (occurs when making an outgress connection). 
	{										//^- indicates that encryption is not required. The function would NOT return false if encryption fails.
		if (logDebug)
			tools->logEvent("AuthEnc failed.", eLogEntryCategory::network, 0, eLogEntryType::failure);

		return false;//of remote peer is delivered through SessionData within peer's hello request, thus our hello request (if we will to Authenticate remote peer) would be ECIES encrypted in that case)
	}
	//Authentication and Encryption (AEAD) - END

	// Update Session Key  - BEGIN // (optional) only if target's public key known, encryption and sessionKeFromAEADBox enabled (thus kind of exotic)
	if (msg->getFlags().boxedEncryption && getUseAEADForSessionKey())// Note: this is REQUIREd public key of the destination.
	{
		if (sessionKey.size()>0)//v---- ONLY if AEAD-container session key to be used here we activate the key (STEP R2).
			setSessionKey(sessionKey);
	}
	// Update Session Key  - END

	return sendNetMsg(msg, true, false);
	//								^-  do not do any pre-processing (any additional encryption etc.).
}

bool CConversation::isToEnd()
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);
	// Temporary copy of the priority queue for iteration
	auto tempQueue = mTasks;

	// Iterate through the temporary queue
	while (!tempQueue.empty()) {
		auto task = tempQueue.top();
		tempQueue.pop(); // Move to the next element

		if (task->getType() == eNetTaskType::endConversation) {
			return true; // Found an endConversation task
		}
	}

	return false; // No endConversation task found
}


/**
 * @brief Sends data over a network using different transport protocols (QUIC, UDT, WebSocket).
 *
 * This function is responsible for sending a given byte array (`packed`) over the network using the protocol specified by the conversation.
 * It supports QUIC, UDT (UDP-based Data Transfer), and WebSocket protocols. The function is protocol-agnostic, meaning it automatically
 * adapts to the underlying transport protocol used by the conversation.
 *
 * For QUIC, there is an option to send data over a dedicated QUIC stream. The 'useDedicatedQUICStream' flag determines whether a new QUIC
 * stream is created for each message (`true`), or if an existing stream is reused (`false`). This is particularly useful for message
 * delimitation in QUIC, as it allows each message to be sent independently.
 *
 * In the case of UDT and WebSocket, the function utilizes their respective mechanisms to send data.
 *
 * @param packed The byte array containing the data to be sent.
 * @param pingStatus Indicates whether to ping the state (i.e., update the last active timestamp).
 * @param useDedicatedQUICStream When set to `true`, a new QUIC stream is created for sending the data,
 *                               effectively delimiting it as a distinct message. If `false`, an existing QUIC stream is reused.
 *
 * @return `true` if the data was sent successfully; `false` otherwise.
 *
 * @note If the conversation is marked to cease communication or if there are any protocol-specific errors
 *       (e.g., socket issues in UDT or WebSocket), the function will return `false`.
 */

bool CConversation::sendBytes(std::vector<uint8_t>& packed, bool pingStatus, bool useDedicatedQUICStream)
{

	// Pre-Flight - BEGIN
	if (getCeaseCommunication())
		return false;
	std::shared_ptr<CNetworkManager> nm = getNetworkManager();
	if (getMisbehaviorsCount() > CGlobalSecSettings::getUDTDataTransitErrorWarningThreshold())
	{//cease community with the peer (connection would be shut-down elsewhere).
		std::string IP = getIPAddress();
		getTools()->logEvent(IP + " exceeded acceptable data-transit error threshold.", "Security", eLogEntryCategory::network,1, eLogEntryType::warning);
		return false;//security check should anything go wrong (part of the brain got stuck etc. Do not attempt communication);
	}
	if (nm->getIsNetworkTestMode())
	{
		std::shared_ptr<CTools> tools = getTools();
		eNetworkSystem::eNetworkSystem ns = eNetworkSystem::QUIC;
		eTransportType::eTransportType tp = getProtocol();
		switch (tp)
		{
		case eTransportType::SSH:
			ns = eNetworkSystem::SSH;
			break;
		case eTransportType::WebSocket:
			ns = eNetworkSystem::WebSocket;
			break;
		case eTransportType::UDT:
			ns = eNetworkSystem::UDT;
			break;
		case eTransportType::local:
			break;
		case eTransportType::HTTPRequest:
			ns = eNetworkSystem::WebServer;
			break;
		case eTransportType::HTTPConnection:
			ns = eNetworkSystem::WebServer;
			break;
		case eTransportType::Proxy:
			ns = eNetworkSystem::HTTPProxy;
			break;
		case eTransportType::HTTPAPIRequest:
			ns = eNetworkSystem::WebServer;
			break;
		case eTransportType::QUIC:
			ns = eNetworkSystem::QUIC;

			break;
		default:
			break;
		}

		nm->pingDatagramSent(ns, tools->stringToBytes(getIPAddress()));
	}
	// Pre-Flight - END

	// Local Variables - BEGIN
	bool socketClosed = false;
	bool logDebug = getLogDebugData();
	std::shared_ptr<CTools> tools = getTools();
	// Local Variables - END

	// Operational Logic - BEGIN
	std::lock_guard<std::mutex> lock(mSendGuardian);
	if (getCeaseCommunication())
		return false;

	socketClosed = isSocketFreed();
	eConversationState::eConversationState state = getState()->getCurrentState();
	if (socketClosed || state == eConversationState::unableToConnect || state == eConversationState::ended)
	{
		return false;
	}

	if(pingStatus)
	getState()->ping();

	// QUIC - BEGIN
	if (getProtocol() == eTransportType::QUIC)
	{
		HQUIC streamHandle = nullptr;
		if (useDedicatedQUICStream) {
			streamHandle = createNewQUICStream();

			if (!streamHandle) {
				// Handle error in stream creation
				std::shared_ptr<CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
				addTask(newTask);
				tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " QUIC connection was lost.", eLogEntryCategory::network, 0, eLogEntryType::failure);

				return false;
			}
		}
		else {
			// Use an existing stream if available, or create a new one
			std::lock_guard<std::mutex> lock(mActiveStreamsGuardian);

			if (!mActiveStreams.empty()) {
				streamHandle = mActiveStreams.begin()->second; // Use the first available stream
			}
			else {
				streamHandle = createNewQUICStream();
				if (!streamHandle)
					return false;
			}
		}

		// Send the data over the QUIC stream
		if (!sendQUICData(streamHandle, packed, useDedicatedQUICStream? true : false)) {
			return false;
		}
		else
		{
			getNetworkManager()->incEth0TX(packed.size());
		}

		return true;
		// QUIC - END
	}
	// UDT - BEGIN
	else if (getProtocol() == eTransportType::UDT)
	{
		UDTSOCKET us = getUDTSocket();

		if (!us)
		{
			tools->logEvent("---- There was an attempt to send data over an empty UDT socket! [Conversation]: " + getAbstractEndpoint()->getDescription(), eLogEntryCategory::network, 1,eLogEntryType::failure, eColor::cyborgBlood);
			return false;
		}
		if (getLogDebugData())
		{
			tools->logEvent("---- Sending " + std::to_string(packed.size()) + " RAW-bytes over UDT. [Conversation]: " + getAbstractEndpoint()->getDescription(), eLogEntryCategory::network, 0);
		}

		int res = UDT::sendmsg(us, reinterpret_cast<char*>(packed.data()), static_cast<int>(packed.size()), -1, true);

		if(res != UDT::ERROR && res == packed.size())
		{
			getNetworkManager()->incEth0TX(packed.size());
			return true;
		}
		else
		{
			if (res == UDT::ERROR)
			{
				UDT::ERRORINFO errorInfo = UDT::getlasterror();
			
				tools->logEvent(tools->getColoredString("[TX UDT Error]:", eColor::lightPink)+ getAbstractEndpoint()->getDescription()+ tools->getColoredString("@"+getIPAddress(), eColor::blue) + tools->getColoredString(" Reason: ", eColor::orange) + errorInfo.getErrorMessage(), eLogEntryCategory::network, 1, eLogEntryType::failure);
			
				
					if (errorInfo.getErrorCode() == CUDTException::ECONNLOST)
					{
						 setCeaseCommunication(true);
							std::shared_ptr<CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
							addTask(newTask);
							tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " connection was lost.", eLogEntryCategory::network, 0, eLogEntryType::failure);
						

							//getNetworkManager()->banIP(getIPAddress());
							//tools->logEvent("Anti-DDOS mechanics triggered for " + getIPAddress() + ".", "Security", eLogEntryCategory::network, 1, eLogEntryType::warning);

					}
					else if (errorInfo.getErrorCode() == CUDTException::EINVSOCK)
					{
						setCeaseCommunication(true);
						std::shared_ptr<CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
						addTask(newTask);
						tools->logEvent(tools->getColoredString("[Requesting Conversation Exit]: ", eColor::lightPink) + getAbstractEndpoint()->getDescription() + tools->getColoredString(" Reason: ", eColor::orange) + " invalid UDT socket.", eLogEntryCategory::network, 0, eLogEntryType::failure);
					}


			}
		}
	}
	// UDT - END

	else	
	{// Web-Sockets - BEGIN
		if (getLogDebugData())
		{
			tools->logEvent("---- Sending " + std::to_string(packed.size()) + " RAW-bytes over a Web-Socket. [Conversation]: " + getAbstractEndpoint()->getDescription(), eLogEntryCategory::network, 0);
		}
		std::shared_ptr<CWebSocket> wSock = getWebSocket();

		if (!wSock)
			return false;
		

			uint64_t bytesSentCount = static_cast<uint64_t>(wSock->sendBytes(packed));
			getNetworkManager()->incEth0TX(bytesSentCount);
		
		return true;
		// Web-Sockets - END
	}
	if (socketClosed || state == eConversationState::ending)
	{//do not count as error if connection MIGHT be no longer available (closer by the other peer abruptly etc.)
		incTransmissionErrorsCount();
		tools->logEvent(tools->getColoredString("[Network]: ", eColor::cyborgBlood) + " ERROR: ---- Sending " + std::to_string(packed.size()) + " RAW-bytes. [Conversation]: " + getAbstractEndpoint()->getDescription(), eLogEntryCategory::network, 0, eLogEntryType::failure);
	}
	// Operational Logic - END

	return false;
}

bool CConversation::handleRequestHelloMsg(std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{
	/*
		IMPORTANT: each Hello Message Handler needs to take care of DECRYPTION on its own. Standard pre-processing is not applies to these.
	*/

	if (getLogDebugData())
	{
		std::shared_ptr<CTools> tools= getTools();
		tools->logEvent(tools->getColoredString("[ Handling a Hello-Request (Stage 2) ]:", eColor::orange) + netmsg->getDescription(), eLogEntryCategory::network, 0);
	}
	//provide signature and respond with hello notify
	return notifyHelloMsg(netmsg, socket, webSocket);
}
/// <summary>
/// This method gets executed in response to a Hello-Notification (which had been sent in response to a Hello-Request)
/// 
/// If peer authentication is to be performed, first the expected public key needs to be set by calling setPeerPubKey().
/// </summary>
/// <param name="netmsg"></param>
/// <param name="socket"></param>
/// <param name="webSocket"></param>
/// <returns></returns>
bool CConversation::handleNotifyHelloMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	if (netmsg == nullptr)
		return false;

	/*
		IMPORTANT: each Hello Message Handler needs to take care of DECRYPTION on its own. Standard pre-processing does not apply to these.
	*/

	//Local Variables - BEGIN
	bool publicKeyFromRemoteSessionInfo = false;
	std::vector<uint8_t> expectedPubKey = getExpectedPeerPubKey();


	// Remote Session Description Auth Support ( Part 1 ) - BEGIN
	// [ IMPORTANT ]: see Dual Nature of CSessionDescription in sessionDescription.h
	std::shared_ptr<CSessionDescription> sd = getRemoteSessionDescription();

	if (sd)
	{
		// [ IMPORTANT ]: see Dual Nature of CSessionDescription in sessionDescription.h

		if (sd->getPubKey().size() == 32)
		{// if delivered by remote peer this overrides session-wide per-datagram strong-auth.
			// we need to verify siganture of the challange present within the Session Description first.
			expectedPubKey = sd->getPubKey();
			publicKeyFromRemoteSessionInfo = true;
		}
	}
	// Remote Session Description Auth Support ( Part 1 ) - END

	bool authenticated = false;
	bool encryptionAuthenticated = false;
	std::vector<uint8_t> challange, sig, boxedPubKey;
	bool authTried = false;
	Botan::secure_vector<uint8_t> sessionKey = getSessionKey();
	Botan::secure_vector<uint8_t> proposedAEADSessionKey;

	std::shared_ptr<CTools> tools = getTools();
	std::vector<uint8_t> pk = getExpectedPeerPubKey();
	std::vector<uint8_t> decryptionKey;
	bool logDebug = getLogDebugData();
	//Local Variables - END

	//Operational Logic - BEGIN

	if (!netmsg->getFlags().sessionDescription)
		return false;//current version of the Protocol requires SessionData proposal to be made


	if (getIsEncryptionRequired() && !netmsg->getFlags().encrypted)
		return false;
	
		// Decryption (and possibly session-key proposal derivation if useAEADpubKeyForSession flag set) - BEGIN
		
		//Note, if enabled, the AEAD-established session key will be made operational a the end of the invocation, not here.
		//the Hello response SHOULD be ECIES-box encrypted
		
		//Session-Key based on a Public Key within AEAD container - BEGIN
		if (netmsg->getFlags().encrypted)
		{
			// derive session key from a Public Key within AEAD container
			
			if (netmsg->getFlags().boxedEncryption)
			{
				decryptionKey = Botan::unlock(getPrivKey());
			}
			else if(sessionKey.size() == 32)
			{
				decryptionKey = Botan::unlock(sessionKey);
				
			}
			else
			{
				if (logDebug)
				tools->logEvent("Peer " + tools->bytesToString(mEndpoint->getAddress())+" used session encryption but no session secret established yet.",
					eLogEntryCategory::network, 0, eLogEntryType::failure, eColor::lightPink);
				return false;
			}

			//             v---- would select decryption method based on flags present within CNetMsg.
			if (!netmsg->decrypt(decryptionKey, pk, proposedAEADSessionKey, boxedPubKey, encryptionAuthenticated))//session key will be derived below using pubKey within SessionDescription
			{
				if (logDebug)
				tools->logEvent("Datagram decryption from " + tools->bytesToString(mEndpoint->getAddress()) + " failed.",
					eLogEntryCategory::network, 0, eLogEntryType::failure);

				return false;
			}
		
		
		//Session-Key based on Public Key within AEAD container - END

		//Decryption - END

		// Session Description - BEGIN
		if (netmsg->getFlags().sessionDescription && netmsg->getData().size() > 0)
		{
			// [ IMPORTANT ]: see Dual Nature of CSessionDescription in sessionDescription.h
			sd = CSessionDescription::instantiate(netmsg->getData());
		}

		if (sd == nullptr)
		{
			if(logDebug)
				tools->logEvent("Hello-Notify with missing SessionDescription received.", eLogEntryCategory::network, 0 ,eLogEntryType::failure, eColor::lightPink);

			if (incMisbehaviorCounter())
			{
				tools->logEvent("Peer " + getIPAddress() + " deemed as malicious. Aborting conversation.", "Security", eLogEntryCategory::network, 1, eLogEntryType::failure);
				endInternal(false, eConvEndReason::security);
				return false;
			}

			return false;//terminate connection;
		}
		if (logDebug)
		tools->logEvent("[ Received HN SessionInfo ]:" + sd->getDescription(), eLogEntryCategory::network, 0);

		setRemoteSessionDescription(sd);

		// Session Description - END

		//Signature based session Authentication - BEGIN 

	//[ challenge-response based mechanism in the context of multiple hello-requests ]
	// The ephemeral private/public key pairs used would result in the same shared secret as its lifetime is Conversation-wide (during consecutive phalanges)
		challange = getRequestedChallange();
		sig = sd->getChallangeData();
		nmFlags flags = netmsg->getFlags();

		if (encryptionAuthenticated)
		{
			/* We can authenticate by verifying that the other peer is in possession of a private key which corresponds to a given public key. Now,
			* we either:
			* 1) check if the authenticated public key is the EXPECTED ONE (needs to be specified before the connection starts)
			*/
			if (sig.size() == 64)
			{
				std::vector<uint8_t> pubKeyForAuth;
				bool doingStrong = false;
				if (expectedPubKey.size() != 32 && sd->getComPubKey().size() == 32)
				{
					if (logDebug)
					tools->logEvent("Valid PubKey for " + tools->bytesToString(mEndpoint->getAddress()) + " NOT known. Commencing with WeakSigAuth..",
						eLogEntryCategory::network,0);
					pubKeyForAuth = sd->getComPubKey();
				}
				/** 2) simply check if the provided signature matches the public key within session description.
				*
				* The 2nd option provided NO additional security let alone for accountability. It can be compared to providing of a self signed certificate.
				* Any consecutive datagrams are expected to be protected with symmetric encryption and Poly1305 authentication code anyway (keyed by session key).
				*/

				//prefer strong, signature based authentication if available
				else if (expectedPubKey.size() == 32)// Notice: the public key would be delivered as part of CSessionDescription contianer as part of Hello Request
				{
					// [ IMPORTANT ]: see Dual Nature of CSessionDescription in sessionDescription.h
					if (logDebug)
					tools->logEvent("Valid PubKey for " + tools->bytesToString(mEndpoint->getAddress()) + " IS KNOWN. Commencing with StrongSigAuth..",
						eLogEntryCategory::network, 0);
					doingStrong = true;
					pubKeyForAuth = expectedPubKey; // we expect the signature to match the previously delivered public key (as part of Session Description - during a Hello Request).

					if (publicKeyFromRemoteSessionInfo)
					{// only if public key was retrieved from Remote Session Info ( would be no point to make an entry based on either an ephemeral public key or non-authenticated Address)

						// Remote Session Description Auth Support ( Part 2 ) - BEGIN
						std::vector<uint8_t> effectiveAddress;
						if (!mCryptoFactory->genAddress(expectedPubKey, effectiveAddress))
						{
							return false; // invalid data
						}

						std::vector<uint8_t> claimedAddress = getRemoteSessionDescription()->getAddress();
						if (claimedAddress.empty()==false && tools->compareByteVectors(effectiveAddress, claimedAddress) == false)
						{
							return false; // invalid data
						}


						std::shared_ptr<CEndPoint> peerEndpoint = std::make_shared<CEndPoint>(effectiveAddress, eEndpointType::PeerID); // ephemeral endpoint representing remote peer by its Public Key
						peerEndpoint->setPubKey(expectedPubKey);// notice: we store both the wallet address and the Public Key within of the routing table. Still, the address is the wallet identifier. 
																// DUI would usually route to a wallet address.
						getNetworkManager()->getRouter()->updateRT(peerEndpoint, getAbstractEndpoint(), 0, eRouteKowledgeSource::sessionHandshake);// a routing table entry establishing a link between current conversation and the remote peer.

						// Remote Session Description Auth Support ( Part 2 ) - END
					}


				}
				if (pubKeyForAuth.size() == 32)
				{
					authTried = true;
					authenticated = CCryptoFactory::getInstance()->verifySignature(sig, challange, pubKeyForAuth);//the integrity of data inside is always assured through Poly1305. It does make sense to authenticate
						//(through signature) only if we have A priori knowledge/expectations in regard to the other peer's public key.
					if (logDebug)
					tools->logEvent(tools->getColoredString(std::string(doingStrong?"StrongSigAuth": "WeakSigAuth"), eColor::blue)+ " for " + tools->bytesToString(mEndpoint->getAddress()) + " "+ std::string(authenticated ? tools->getColoredString("SUCCEDED", eColor::lightGreen) : tools->getColoredString("FAILED", eColor::lightPink)),
						eLogEntryCategory::network, 0);
				}
			}

			// a little bit less of strength below:
			if (getUseAEADForAuth() && !authenticated && encryptionAuthenticated && expectedPubKey.size() == 32 && boxedPubKey.size()==32 && flags.useAEADForAuth && flags.encrypted && flags.boxedEncryption)
			{
				//decryption together with Poly-1305 must have succeeded by now thus we MAY assume node as authenticated IF public key within the AEAD container matched the expected one
				if (tools->compareByteVectors(expectedPubKey, boxedPubKey))
					authenticated = true;

				authTried = true;
			}
		}

		if (getIsAuthenticationRequired() && !authenticated)//return error only if sig-based authentication REQUIRED
		{
			if (logDebug)
			tools->logEvent("Error: Authentication with " + tools->bytesToString(mEndpoint->getAddress()) + " FAILED. Connection will abort.",
				eLogEntryCategory::network, 0,eLogEntryType::failure, eColor::lightPink);
			return false;
		}

		if (authenticated)
		{
			if (logDebug)
			tools->logEvent("Connection with " + tools->bytesToString(mEndpoint->getAddress()) + tools->getColoredString(" AUTHENTICATED.", eColor::lightGreen),
				eLogEntryCategory::network, 0);

			{
				std::lock_guard<std::mutex> lock(mFieldsGuardian);
				mPeerAuthenticatedAsPubKey = expectedPubKey;//IMPORTANT: the authenticated identity is also kept track of, at the level of CScriptEngine (available through the log-me-in mechanics). Notice dti->setLoggedInIDToke()
				mPeerAuthenticated = true;//from now on we might process other messages than hello messages.
			}
		}
		else if (authTried)
		{
			if (logDebug)
			tools->logEvent("Authentication of " + tools->bytesToString(mEndpoint->getAddress()) + " FAILED, yet AUTH is set as not obligatory.",
				eLogEntryCategory::network, 0, eLogEntryType::failure, eColor::lightPink);
		}
		else
		{
			if (logDebug)
			tools->logEvent("Authentication of " + tools->bytesToString(mEndpoint->getAddress()) + " was not attempted.",
				eLogEntryCategory::network, 0);
		}

		//Signature based session Authentication - END

		// Session-Key Establishment and Proclamation  - BEGIN
		// Notice: when AEAD-container bases session mode enables, the session key has been established already but not proclaimed.
	
		if (sessionKey.size() == 0)//if the session-key has not been established as of yet
		{
			if (!getUseAEADForSessionKey())
			{
				//Session Key based on an explicit public key within Session Description - BEGIN
				//attempt to prepare session-key based on pub-key within session description.
				if (sd->getComPubKey().size() == 32)
				{
					sessionKey = mCryptoFactory->ECDH(sd->getComPubKey(), getPrivKey());
					setSessionKey(sessionKey);//can be now made operational.
				}
				//Session Key based on an explicit public key within Session Description - END
			}
			else if(proposedAEADSessionKey.size()>0)
			{
				//use session key based on AEAD container
				setSessionKey(proposedAEADSessionKey);
			}
		}

		// Session-Key Establishment and Proclamation  - END

		//if session key established, any future CNetMsgs would be encrypted with endChaCha20() instead of ECIES.

		//[Important]: the session key will be set AFTER the message is sent, for the session-based encryption to take effect afterwards.

	}
	else
		return false;//encrypted or not, there MUST be a SessionDecription inside; 

	//Operational Logic - END

	return true;
}
bool CConversation::notifyReceiptIDMsg(std::vector<uint8_t> receiptID, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{

	CNetMsg msg(eNetEntType::receiptID, eNetReqType::notify);
	msg.setData(receiptID);
	std::vector packed = msg.getPackedData();
	return sendBytes(packed);
}

bool CConversation::notifyByeMsg(UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	CNetMsg msg(eNetEntType::bye, eNetReqType::notify);
	std::vector packed = msg.getPackedData();
	return sendBytes(packed);
}

bool CConversation::handleNotifyByeMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	getState()->setCurrentState(eConversationState::ending);
	return true;
}



/// <summary>
/// Notifies peer about a new block.
/// </summary>
/// <param name="block"></param>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::notifyNewBlock(std::shared_ptr<CBlockHeader> block, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	if(!block)
	return false;

	return true;
}

bool CConversation::handleNotifyNewChatMsg(std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{

	//Local Variables - BEGIN
	std::vector<uint8_t> data = netmsg->getData();
	std::shared_ptr<CNetworkManager> nm = getNetworkManager();
	std::shared_ptr<CStateDomainManager> sdm = getBlockchainManager()->getStateDomainManager();
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::vector<uint8_t> hash = cf->getSHA2_256Vec(data);
	std::shared_ptr<CTools> tools = getTools();
	std::string IP = getIPAddress();
	std::vector<uint8_t> receiptID;
	std::shared_ptr<CIdentityToken> idTokenSource;
	CStateDomain * issuer;
	std::vector<uint8_t> pubKeySource;
	std::shared_ptr<CChatMsg> msg;
	bool autoBan = true;
	std::vector<uint8_t> sourceID;
	std::shared_ptr<CDTIServer> dtiServer;
	std::shared_ptr< CNetTask> task;
	bool authenticated = true;

	if (nm)
	{
		dtiServer = nm->getDTIServer();
	}
	std::vector<std::shared_ptr<CDTI>> terminals;
	//Local Variables -  END

	//support of the already seen objects awareness- BEGIN
	if (nm->getWasDataSeen(hash, IP, autoBan))
	{

		tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) + " redundant chat-msg received from " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);

		if (autoBan)
		{//if still true that means that the IP was banned and we should terminate ASAP
			std::shared_ptr< CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
			addTask(newTask);
		}
		return false;
	}
	nm->sawData(hash);

	if (data.size() > 50000000)
	{
		tools->logEvent("Excessive data received from " + IP, eLogEntryCategory::network, 1, eLogEntryType::warning, eColor::lightPink);
		issueWarning();
		return false;
	}

	//support of the already seen objects awareness - END
	
	//Operational Logic - BEGIN
	msg = CChatMsg::instantiate(netmsg->getData());

	if (!msg)
	{
		tools->logEvent("Invalid chat-msg received..", eLogEntryCategory::network, 1, eLogEntryType::warning, eColor::lightPink);
		issueWarning();
		return false;
	}
	
	sourceID = msg->getSourceID();

	issuer = sdm->findByID(sourceID);

	if (issuer == nullptr)
	{
		authenticated = false;
		//tools->logEvent("Invalid chat-msg received, unknown issuer..", eLogEntryCategory::network, 1, eLogEntryType::warning, eColor::lightPink);
		//issueWarning();
		//return;
	}
	if (issuer)
	{
		idTokenSource = issuer->getIDToken();
	}
	
	if (authenticated && idTokenSource == nullptr)
	{
		authenticated = false;
	}

	if (idTokenSource)
	{
		pubKeySource = idTokenSource->getPubKey();
	}

	if (authenticated &&  pubKeySource.size()!=32)
	{
		authenticated = false;
	}


	if (authenticated && !msg->verifySignature(pubKeySource))
	{
		authenticated = false;
	}

	std::string msgTxt = tools->bytesToString(msg->getData());

	if (!msgTxt.size())
	{
		issueWarning();
		return true;
	}


	std::string visibleID = !authenticated ? tools->bytesToString(msg->getSourceID()) : idTokenSource->getFriendlyID();
	tools->eraseSubStr(visibleID, u8"🔒");

	if (authenticated)
	{
		visibleID += tools->getColoredString(u8"🔒", eColor::orange);
	}

	if (dtiServer)
	{
		dtiServer->broadcastMsgToAll(msgTxt, visibleID );
	}


	std::vector<std::shared_ptr<CConversation>> convs = nm->getAllConversations(true, true, true, true);


	//retransmit

	for (uint64_t i = 0; i < convs.size(); i++)
	{
		task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
		task->setNetMsg(netmsg);//IMPORTANT: simply re-transmit; no modifications so that loops in network propagation are mitigated.

		tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) +
			"notifying peer " + convs[i]->getIPAddress() + " about a chat msg. ", eLogEntryCategory::network, 0);

		convs[i]->addTask(task);
	}
	return true;

	//Operational Logic - END
}

/// <summary>
/// Checks whether the remote node has a block available.
/// </summary>
/// <param name="blockInfo"></param>
/// <returns></returns>
eBlockAvailabilityConfidence::eBlockAvailabilityConfidence CConversation::hasBlockAvailable(const std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>> checkedBlockInfo)
{
	// Local Variables - BEGIN
	eBlockAvailabilityConfidence::eBlockAvailabilityConfidence assessment = eBlockAvailabilityConfidence::unavailable;
	bool availableInChain = false;
	std::unique_lock<std::mutex> localChainProofLocked;
	// Local Variables - END

	// Pre-flight Checks - BEGIN
	if (std::get<0>(checkedBlockInfo).size() != 32)
	{
		return eBlockAvailabilityConfidence::unavailable;
	}
	std::shared_ptr<CBlockHeader> lookedFor = std::get<1>(checkedBlockInfo);
	// Pre-flight Checks - END

	// Local Variables - BEGIN
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	uint64_t leaderHeight = bm->getCachedHeight();
	uint64_t heaviestLeaderhHeight = bm->getCachedHeaviestHeight();
	bool traverseFromEnd = true;
	std::vector<uint8_t> checkedBlockID = std::get<0>(checkedBlockInfo);

	std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>> remoteLeaderInfo;
	// Local Variables - END

	// Operational Logic - BEGIN

	// 1) Get the remote peer's leader information:
	remoteLeaderInfo = getLeadBlockInfo();
	std::shared_ptr<CBlockHeader> remoteLeader = std::get<1>(remoteLeaderInfo);
	std::vector<uint8_t> remoteLeaderID = std::get<0>(remoteLeaderInfo);

	// 2) Checkpoints' Support - BEGIN
	// We do not want to sync with target Heaviest Chain Proofs
	// that are "shorter" than the latest obligatory checkpoint.
	std::shared_ptr<CBCheckpoint> ob = bm->getLatestObligatoryCheckpoint();
	if (!remoteLeader || (ob && (ob->getHeight() > remoteLeader->getHeight())))
	{
		return  eBlockAvailabilityConfidence::unavailable;
	}
	// Checkpoints' Support - END

	// 3) Basic validity checks on remote leader
	// If the leading block info size is not 32 or if the leading block is null, return false. 
	// This implies that the other peer has not notified us yet.
	if (remoteLeaderID.size() != 32 || remoteLeader == nullptr)
	{
		// The other peer has not notified us properly yet
		return  eBlockAvailabilityConfidence::unavailable;
	}

	// 4) If the block we want is higher than the remote's known leader, the peer is behind
	// If the block info is not null, compare its height to the leading block's height. 
	// If it's higher, the other peer is in the past, return false.
	if (lookedFor && (lookedFor->getHeight() > remoteLeader->getHeight()))
	{
		// The other peer still is in the past
		return  eBlockAvailabilityConfidence::unavailable;
	}

	// 5) Quick optimization: If we are checking exactly the remote leader's block
	// At this point we:
	// - Check if the other peer's leading block is within our heaviest chain-proof.
	// - Verify whether the other peer's leading block is AFTER the block we are after 
	//   (this means the other node is synced at least up to that point)
	// - If so, we assume the other node has the block available
	if (getTools()->compareByteVectors(remoteLeaderID, checkedBlockID))
	{
		return  eBlockAvailabilityConfidence::available; // The peer clearly has its own leader block
	}

	// 6) Conversation Local Chain-Proof analysis - BEGIN
	// We cache the last known valid chain-proof for this conversation.
	// Notice: remote peer might had delivered only a partial chain-proof and we have reconstructed the full chain-proof.
	// Conversation stores a fully qualified (full) chain-proof. That would be > 10MB per conversation.
	// Yet still, we need this to better mitigate network eclipsing attacks and other attacks 
	// leading to wasted processing power and synchronization speed.
	localChainProofLocked = std::unique_lock<std::mutex>(mLocalChainProofGuardian);

	bool isCloserToGenesis = false;
	if (lookedFor && remoteLeader &&
		(static_cast<double>(lookedFor->getHeight()) < (static_cast<double>(remoteLeader->getHeight()) / 2.0)))
	{
		isCloserToGenesis = true;
	}

	// 7) Attempt your standard "isBlockInChainProof" checks using either
	// the global heaviest chain or the conversation's local chain-proof:
	if (mLocalChainProof.empty() && !remoteLeaderID.empty())
	{
		if (!isCloserToGenesis)
		{
			// Check if [checkedBlockID] is before or in the path to [remoteLeaderID]
			availableInChain = bm->isBlockInChainProof(
				remoteLeaderID,
				eChainProof::heaviest,
				checkedBlockID,
				/*includeDescendants=*/true,
				/*includeAncestors=*/true
			);
		}
		else
		{
			availableInChain = bm->isBlockInChainProof(
				checkedBlockID,
				eChainProof::heaviest,
				std::vector<uint8_t>(),
				/*includeDescendants=*/false,
				/*includeAncestors=*/true
			);
		}

	}
	else if (!mLocalChainProof.empty() && !remoteLeaderID.empty())
	{
		if (!isCloserToGenesis)
		{
			// Here we specifically use the conversation's chain proof
			 availableInChain = bm->isBlockInChainProof(
				remoteLeaderID,
				eChainProof::fullTemporary,
				checkedBlockID,
				/*includeDescendants=*/true,
				/*includeAncestors=*/true,
				mLocalChainProof
			);
		}
		else
		{
			availableInChain = bm->isBlockInChainProof(
				checkedBlockID,
				eChainProof::fullTemporary,
				std::vector<uint8_t>(),
				/*includeDescendants=*/false,
				/*includeAncestors=*/true,
				mLocalChainProof
			);
		}

		
	}
	// Conversation Local Chain-Proof analysis - END
	// ---- Fallback Logic - BEGIN ----
	// If the standard check concluded "not available" (i.e., we didn't find the
	// block in the local chain-proof with respect to the remote leader), we can
	// apply a heuristic fallback. Since partial chain proofs are always verified
	// first, we trust that the remote's tip is at least *somewhat* valid.
	//
	// Rationale:
	//   - If the remote is significantly ahead of our local chain tip,
	//     it is likely they have the block in question (especially if
	//     it is deep in the chain).
	//   - This helps avoid scenarios where (due to checkpoint or version mismatch)
	//     we strictly can't find the remote leader in our local chain-proof,
	//     but the peer might actually have all older blocks.
	if (!availableInChain)
	{

		// Fuzzy Test - BEGIN
		if (remoteLeader)
		{
			if (remoteLeader->getHeight() >= heaviestLeaderhHeight)
			{
				// if remote node has newer blocks than locally proclaimed by heaviest chain proof, then it's likely
				// (though not certain) that the remote peer might be in possession of a block which is looked after.
				assessment = eBlockAvailabilityConfidence::possiblyAvailable;
			}
		}
		// Fuzzy Test - END
	}
	else
	{
		assessment = eBlockAvailabilityConfidence::available;
	}
	// ---- Fallback Logic - END ----

	return assessment;
	// Operational Logic - END

	// Note: Previous faster implementation based on absolute indexes:
	// return bm->isBlockInChainProof(remoteLeader, eChainProof::heaviest, std::get<1>(checkedBlockInfo), true);
}

void CConversation::setEndReason(eConvEndReason::eConvEndReason reason)
{	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mEndReason = reason;

}

eConvEndReason::eConvEndReason CConversation::getEndReason()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mEndReason;
}

uint64_t CConversation::getCustomBarID()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mCustomBarID;
}

std::vector<std::vector<uint8_t>> CConversation::getLocalChainProof()
{
	std::lock_guard<std::mutex> lock(mLocalChainProofGuardian);
	return  mLocalChainProof;
}

void CConversation::setLocalChainProof(const std::vector<std::vector<uint8_t>>& proof)
{
	std::lock_guard<std::mutex> lock(mLocalChainProofGuardian);
	mLocalChainProof.clear();
    mLocalChainProof = proof;
}

void CConversation::clearLocalChainProof()
{
	std::lock_guard<std::mutex> lock(mLocalChainProofGuardian);
	mLocalChainProof.clear();
}


/// <summary>
/// Used when requesting large dynamic data over UDT datagrams.
/// </summary>
/// <returns></returns>
bool CConversation::requestDeferredTimeout(bool requestedByUs)
{
	std::shared_ptr<CTools> tools = getTools();
	std::string ip = getIPAddress();

	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mDeferredTimeoutRequstedAtTimestamp = std::time(0);

	tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) +(requestedByUs?("expecting large data delivery from " + ip ):(ip+ +" requested a "+tools->getColoredString(" large data delivery", eColor::lightPink))) +
		".", eLogEntryCategory::network,requestedByUs? 1:4, requestedByUs?eLogEntryType::notification: eLogEntryType::warning);

	return true;
}

uint64_t CConversation::getDeferredTimeoutRequestedAt()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mDeferredTimeoutRequstedAtTimestamp;
}

uint64_t CConversation::getUDTTimeout()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	if (mDeferredTimeoutRequstedAtTimestamp && (std::time(0) - mDeferredTimeoutRequstedAtTimestamp) < DEFFERRED_TIMEOUT_DURATION)
	{
		return DEFFERRED_TIMEOUT_DURATION;
	}
	else
	{
		return CGlobalSecSettings::getKillUDTSyncConversationAfterLackOfPing();
	}
}

bool CConversation::getPreventStartup()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mPreventStartup;
}

void CConversation::preventStartup()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mPreventStartup = true;
}

/// <summary>
/// Handles an incoming block HEADER. No actual block expected.
/// </summary>
/// <param name="netmsg"></param>
/// <param name="socket"></param>
/// <param name="webSocket"></param>
/// <returns></returns>
bool CConversation::handleNotifyNewBlockMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{


	// Update block notification statistics
	updateBlockNotificationStats();

	//Preliminaries - BEGIN
	std::shared_ptr<CNetworkManager>  nm = getNetworkManager();
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();

	if (!nm || !bm)
	{
		return false;
	}
	if (nm->getIsNetworkTestMode())
	{
		return false;
	}
	//Preliminaries - END

	if (!getSyncToBeActive())
		return false;


	// we've received a header that means that node wants to participate in Sync sub-protocol.
	setIsSyncEnabled(true);// important: we NEED to account for empty datagrams! as these may be coming from infant nodes.
	conversationFlags flags = getFlags();
	//check if we are in possession of that block already
	
	/*
	Note: there's an assumption that nodes notify ONLY about the most recent best block.
	Under such an assumption other nodes schedule their downloads, more efficiently.

	Example:  Node_A would check if the block it is looking for, contained on its local Heaviest Chain-Proof
	is leading to the best block already notified by Node_B. Only then, would it decide to attempt to fetch that block from Node_B.
	In other words, Node_A caches the recently notified block identifier from Node_B, within the CConversation object, 
	and before attempting to download from Node_B, it would first call hasBlockAvailable() on a CConversation with Node_B.
	hasBlockAvailable() would then check getRecentLeadBlock() for the block identifier recently notified about and traverse mHeaviestChainProof
	, going backwards - to see if identifier returned by getRecentLeadBlock() is present AFTER the identifier of a block Node_A attempts to download.

	Thanks to such an approach, nodes would NOT attempt to download blocks from nodes that might not even have them available locally.
	Yet again, that stems from an  assumption that nodes broadcast notification about already verified leading blocks only and that the blocks nodes managed to verify 
	are available in their cold storage.
	*/
	//Local Variables - BEGIN
	std::string error;
	eBlockInstantiationResult::eBlockInstantiationResult res;
	CBlockHeader::eBlockHeaderInstantiationResult resH;
	std::vector<uint8_t> headerBER = netmsg->getData();

	
	bool isBlockAvailable = false;
	std::vector<uint8_t> blockID;
	std::shared_ptr<CTools> tools = getTools();
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::shared_ptr<CNetTask> task;
	bool alreadyKnown = false;
	std::string ip = getIPAddress();
	bool logDebug = getLogDebugData();
	std::vector<uint8_t> heaviestLeadingBlockID;
	int64_t overallBlockDiff = 0;
	uint64_t longCPWaitInterval = 60 * 10;
	uint64_t GenesisCPRequestInterval = 60;

	std::shared_ptr<CBlockHeader> hbl;
	std::shared_ptr<CBlockHeader> header;
	std::shared_ptr<CBlock> cachedLeader;
	uint64_t now = std::time(0);

	if (nm)
	{
		bm = nm->getBlockchainManager();
	}
	if (!bm)
		return true;


	// SECURITY - BEGIN
	if (nm)
	{
		if (!nm->canProcessBlockNotification(ip, true))
		{
			if (logDebug)
			{
				tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " won't process " + 
					tools->getColoredString("new block notification ", eColor::lightCyan) + " from " + ip + ". Would happen too frequently..", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::lightGreen);
			}
			return true;
		}
	}
	// SECURITY - END

	std::vector<uint8_t> leaderBlockID;
	if (bm)
	{
		cachedLeader = bm->getCachedLeader();
		if (cachedLeader)
			leaderBlockID = cachedLeader->getID();  //bm->getLeaderID();
	}
	uint64_t farAwayCPRequetedTS = bm->getFarAwayCPRequestedTimestamp();
	uint64_t localHeight = cachedLeader ? cachedLeader->getHeader()->getHeight() : 0;
	uint64_t localKeyHeight = cachedLeader ? cachedLeader->getHeader()->getKeyHeight() : 0;
	//Local Variables - END
	
	//Operational Logic - BEGIN
	heaviestLeadingBlockID = bm->getHeaviestChainProofLeadBlockID();
	
	if (logDebug)
	{
		tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " processing " + tools->getColoredString("new block notification ", eColor::lightCyan) + " from " + ip + ".", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::lightGreen);
	}


	if (headerBER.size() == 0)
	{
		//It's OK!!! nodes indicate willingness of participation in DSM protocol by sending an empty header.
		// 
		//tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " empty block-header received from " + ip, eLogEntryCategory::network, 10, eLogEntryType::warning, eColor::lightPink);
		//issueWarning();
		return true;
	}
	blockID = bm->getCryptoFactory()->getSHA2_256Vec(headerBER);

	// Phase 1 - Remote Leader Awareness - BEGIN

	header = CBlockHeader::instantiate(headerBER, resH, error, false, getBlockchainManager()->getMode());

	setLeadBlockInfo(std::make_tuple(blockID, header));
	// Phase 1 - Remote Leader Awareness - END
	
	// Phase 2-A - Is Processing Needed - BEGIN
	isBlockAvailable = bm->isBlockAvailableLocally(blockID); //use extremely fast Robin Hood look-up tables.
	// Main Line: we need to check whether we have the block in Cold Storage.
	//			  if we already do - we are NOT to peruse this any further.

	std::shared_ptr<CBlockHeader> hbh = bm->getHeaviestChainProofLeader();

	if (localHeight && isBlockAvailable && bm && (hbh && hbh->getHeight() >= header->getHeight()))
	{
		//@CodesInChaos - do NOT trust response from a single peer. That makes the system overly prone to eclipsing attacks.
		// ex. all it would take for the local system to believe that it is already synchronized with the network, was for it to connect with itself.
		//if (tools->compareByteVectors(blockID, leaderBlockID))
		//{
			//bm->setSyncPercentage(100);
		//}

		if (logDebug) {
			tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " the received block-header from " + ip + " is already known.", eLogEntryCategory::network, 0, eLogEntryType::notification);
		}
		alreadyKnown = true;
		return false;
	}
	// Phase 2-A - Is Processing Needed - END

	// Phase 2-B - Do We Consider Following  - BEGIN
	//Optimizations - BEGIN
	if (header == nullptr)
	{
		tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " the received block header from " + ip + " is invalid.", eLogEntryCategory::network, 0, eLogEntryType::warning, eColor::lightPink);
		issueWarning();
		return false;
	}
	uint64_t headerHeight = header->getHeight();
	uint64_t cachedLeaderHeight = cachedLeader ? cachedLeader->getHeader()->getHeight() : 0;

	if (nm->wasChainProofForBlockProcessed(blockID))
	{
		if (logDebug)
		{
			tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " won't request chain-proof from " + ip + " its world-view is already known.", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::orange);
		}
		return false;
	}

	if (tools->compareByteVectors(heaviestLeadingBlockID, blockID))
	{
		if (logDebug)
		{
			tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " won't request chain-proof from " + ip + " - heaviest block already proclaimed.", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::orange);
		}
		return false;
	}

	//Only Bootstrap nodes first - BEGIN
	//that is to mitigate network eclipsing attacks.
	uint64_t tryingBootstrapNodesSince = nm->getRetrievingCPFromBootstrapNodesSince();
	bool onlyBootstrapNodesAllowedToDeliver = true;

	if (tryingBootstrapNodesSince == 0)
	{
		nm->pingRetrievingCPFromBootstrapNodesSince();
		tryingBootstrapNodesSince = std::time(0);
	}

	if ((std::time(0) - tryingBootstrapNodesSince) > (60 * 5))
	{
		onlyBootstrapNodesAllowedToDeliver = false;
	}

	std::vector<std::shared_ptr<CEndPoint>>  bootstrapNodes = nm->getBootstrapNodes();

	std::vector<std::string> bootstrapNodesStrs;

	for (uint64_t i = 0; i < bootstrapNodes.size(); i++)
	{
		bootstrapNodesStrs.push_back(tools->bytesToString(bootstrapNodes[i]->getAddress()));
	}

	if (onlyBootstrapNodesAllowedToDeliver && !tools->checkStringContained(ip, bootstrapNodesStrs))
	{
		tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " won't request chain-proof from " + ip + " - for now considering only Bootstrap nodes.", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::orange);
		return false;
	}
	//Only Bootstrap nodes first - END

	if (cachedLeader)
	{	// we do not want to request chain-proofs for blocks which are too far away into the future (from our local perspective) too often.
		// both verification and exchange of such chain-proof imposes significant overheads. 
		// Notice: the checkpoints used during chain-proofs' requests are based on the Verified Chain-Proof (not the Heaviest Chain-Proof)
		//		   and the former may be considerably far behind the latter.

		//Blocks In The Future - BEGIN

		bool isStuck = getBlockchainManager()->getIsSyncStuck();

		if (isStuck)
		{
			longCPWaitInterval = 60;
		}
		if (headerHeight > cachedLeaderHeight &&
			((headerHeight - cachedLeaderHeight) > 100))
		{
			if ((std::time(0) - farAwayCPRequetedTS) < longCPWaitInterval)
			{
				if (logDebug)
				{
					tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " won't request chain-proof from " + ip + " - node too far away into the future.", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::orange);
				}
				return false;
			}
			else
			{
				tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " requesting a " + tools->getColoredString("Far-Away Chain-Proof", eColor::lightCyan) + " from " + ip + ".", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::orange);
				bm->pingFarAwayCPRequestedTimestamp();
				requestDeferredTimeout(true);
			}
		}
		//Blocks In The Future - END

		// Too far in the past - BEGIN
		if (cachedLeaderHeight > 1000 && (headerHeight < (cachedLeaderHeight - 1000)))
		{
			if (logDebug)
			{
				tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " won't request chain-proof from " + ip + " - node is too far in the past.", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::lightPink);
			}
			return false;
		}
		// Too far in the past - END

	}
	else
	{
		// no leader known, we are thus requesting lots of data (as of 9/10/23 the Heaviest Chain-Proof is > 10MB)
		if ((std::time(0) - farAwayCPRequetedTS) < GenesisCPRequestInterval)
		{
			if (logDebug)
			{
				tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " won't request chain-proof from " + ip + " - Genesis-Chain requested recently.", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::orange);
			}
			return false;
		}
		else
		{
			requestDeferredTimeout(true);
			tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " requesting a " + tools->getColoredString("Genesis Chain-Proof", eColor::lightCyan) + " from " + ip + ".", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::orange);
			bm->pingFarAwayCPRequestedTimestamp();
		}
		
	}
	
	//Optimizations - END
	// Phase 2-B - Do We Consider Following  - END

	// Execution - BEGIN
	 overallBlockDiff = (int64_t)header->getHeight() - (int64_t)localHeight;

	if (!isBlockAvailable || overallBlockDiff)
	{//newer block OR into the past but not older than a threshold OR no block available locally at all.

		//Request Chain-Proof - BEGIN
		task = std::make_shared<CNetTask>(eNetTaskType::requestChainProof, 1);
		std::vector<std::vector<uint8_t>> checkpoints;
		

		// Prepare Synchronisation Point Porposals - BEGIN
		// [ Rationale ]: so that remote peer does not need to deliver (and so that we do not need to process) a very long chain proof.
		
		if (mNetworkManager->getCPCheckpoints(checkpoints))
		{
			std::stringstream hcpReport;
			// Logging - BEGIN
			std::shared_ptr<CBlockHeader> keyLeader;
			if (!checkpoints.empty())
			{
				eBlockInstantiationResult::eBlockInstantiationResult ir;
				std::stringstream syncPointsReport;
				syncPointsReport << tools->getColoredString( "[ Synchronization Points Analysis ]",  eColor::lightCyan)<<" proposed to " + getIPAddress()<< " - BEGIN \n";
				// Local Heaviest Chain Proof Info - BEGIN
				{
					
					hcpReport << "\n -  Local Heaviest Chain-Proof Status - BEGIN";

					// Get key leader info
					 keyLeader = bm->getHeaviestChainProofKeyLeader();

					if (keyLeader) {
						hcpReport << "\n[ Key Leader ]"
							<< "\n    Height: " << keyLeader->getHeight()
							<< "\n    Key Height: " << keyLeader->getKeyHeight()
							<< "\n    Type: " << (keyLeader->isKeyBlock() ? "Key Block" : "Data Block")
							<< "\n    Timestamp: " << tools->timeToString(keyLeader->getSolvedAtTime(), true,true)
							<< "\n    Public Key: " << tools->base58CheckEncode(keyLeader->getPubKey())
							<< "\n    Parent: " << tools->base58CheckEncode(keyLeader->getParentID())
							<< "\n    ID: " << tools->base58CheckEncode(keyLeader->getHash());
					}
					else {
						hcpReport << "\n[ Key Leader ]    Not available";
					}

					// Get lead block info
					std::vector<uint8_t> leadBlockID = bm->getHeaviestChainProofLeadBlockID();
					if (!leadBlockID.empty()) {
						eBlockInstantiationResult::eBlockInstantiationResult ir;
						std::shared_ptr<CBlock> leadBlock = bm->getBlockByHash(leadBlockID, ir);
						if (leadBlock && leadBlock->getHeader()) {
							hcpReport << "\n[ Lead Block ]"
								<< "\n    Height: " << leadBlock->getHeader()->getHeight()
								<< "\n    Key Height: " << leadBlock->getHeader()->getKeyHeight()
								<< "\n    Type: " << (leadBlock->getHeader()->isKeyBlock() ? "Key Block" : "Data Block")
								<< "\n    Timestamp: " << tools->timeToString(leadBlock->getHeader()->getSolvedAtTime(), true,true)
								<< "\n    Parent: " << tools->base58CheckEncode(leadBlock->getHeader()->getParentID())
								<< "\n    ID: " << tools->base58CheckEncode(leadBlockID);
						}
						else {
							hcpReport << "\n[ Lead Block ]    Unable to instantiate (ID: "
								<< tools->base58CheckEncode(leadBlockID) << ")";
						}
					}
					else {
						hcpReport << "\n[ Lead Block ]    Not available";
					}

					hcpReport << "\n -  Local Heaviest Chain-Proof Status - END";
					}


				// Local Heaviest Chain Proof Info - END

				syncPointsReport << hcpReport.str();

				syncPointsReport  <<  tools->getColoredString("\n - Sync Points - BEGIN \n ", eColor::blue);
				
				// Helper function for block info
				auto appendBlockInfo = [&](const std::shared_ptr<CBlock>& block, const std::vector<uint8_t>& hash, const char* label) {
					if (block && block->getHeader()) {
						syncPointsReport << "\n[ " << label << " ]"
							<< "\n    Height: " << block->getHeader()->getHeight()
							<< "\n    Key Height: " << block->getHeader()->getKeyHeight()
							<< "\n    Type: " << (block->getHeader()->isKeyBlock() ? "Key Block" : "Data Block")
							<< "\n    Timestamp: " << tools->timeToString(block->getHeader()->getSolvedAtTime(), true, true)
							<< "\n    Parent: " << tools->base58CheckEncode(block->getHeader()->getParentID())
							<< "\n    ID: " << tools->base58CheckEncode(hash);
					}
					else {
						syncPointsReport << "\n[ " << label << " ]"
							<< "\n    Unable to instantiate block"
							<< "\n    ID: " << tools->base58CheckEncode(hash);
					}
					};

				// Get and report first block
				std::shared_ptr<CBlock> blockStart = bm->getBlockByHash(checkpoints[0], ir);
				appendBlockInfo(blockStart, checkpoints[0],
					checkpoints.size() == 1 ? "Single Checkpoint (Only Point)" : "First Checkpoint");

				// Get and report second block if available
				if (checkpoints.size() > 1) {
					std::shared_ptr<CBlock> blockSecond = bm->getBlockByHash(checkpoints[1], ir);
					appendBlockInfo(blockSecond, checkpoints[1], "Second Checkpoint");
				}

				// Get and report last block if more than 2 checkpoints
				if (checkpoints.size() > 2) {
					std::shared_ptr<CBlock> blockEnd = bm->getBlockByHash(checkpoints[checkpoints.size() - 1], ir);
					appendBlockInfo(blockEnd, checkpoints[checkpoints.size() - 1], "Last Checkpoint");
				}
				syncPointsReport << tools->getColoredString("\n - Sync Points - END \n ", eColor::blue);

				// Add summary information - BEGIN
				syncPointsReport << "\n\n[ Summary ]"
					<< "\n    Total Checkpoints: " << checkpoints.size();

				if (blockStart && blockStart->getHeader() && checkpoints.size() > 1) {
					std::shared_ptr<CBlock> blockEnd = bm->getBlockByHash(checkpoints[checkpoints.size() - 1], ir);
					if (blockEnd && blockEnd->getHeader()) {
						uint64_t startHeight = blockStart->getHeader()->getHeight();
						uint64_t endHeight = blockEnd->getHeader()->getHeight();
						uint64_t startTime = blockStart->getHeader()->getSolvedAtTime();
						uint64_t endTime = blockEnd->getHeader()->getSolvedAtTime();

						// Safe height difference calculation
						if (startHeight  >= endHeight) {
							uint64_t heightDiff = startHeight - endHeight;
							syncPointsReport << "\n    Height Range: " << heightDiff << " blocks"
								<< "\n    Start Height: " << startHeight
								<< "\n    End Height: " << endHeight;
						}
						else {
							syncPointsReport << "\n    Height Range: Invalid (start height < end height)"
								<< "\n    Start Height: " << startHeight
								<< "\n    End Height: " << endHeight;
						}

						// Safe time difference calculation and formatting
						syncPointsReport << "\n    Start Time: " << tools->timeToString(startTime, true, true)
							<< "\n    End Time: " << tools->timeToString(endTime, true, true);

						if (startTime  >= endTime) {
							uint64_t timeDiff = startTime  - endTime;
							syncPointsReport << "\n    Time Range: " << tools->secondsToFormattedString(timeDiff);
						}
						else {
							syncPointsReport << "\n    Time Range: Invalid (start time < end time)";
						}
					}
				}
				// Add summary information - END

				// Warnings - BEGIN
				if (blockStart && blockStart->getHeader() && keyLeader) {
					uint64_t startHeight = blockStart->getHeader()->getHeight();
					uint64_t leaderHeight = keyLeader->getHeight();
					uint64_t startTime = blockStart->getHeader()->getSolvedAtTime();
					uint64_t leaderTime = keyLeader->getSolvedAtTime();

					bool hasCriticalWarning = false;
					std::stringstream warningReport;
					warningReport << "\n[ Warning Analysis ]";

					// Check if first checkpoint is ahead of current leader
					if (startHeight > leaderHeight) {
						hasCriticalWarning = true;
						warningReport << "\n    [CRITICAL] First checkpoint height (" << startHeight
							<< ") is ahead of current leader height (" << leaderHeight << ")"
							<< "\n    Height difference: " << (startHeight - leaderHeight) << " blocks"
							<< "\n    Time difference: " << tools->secondsToFormattedString(startTime - leaderTime)
							<< "\n    First checkpoint time: " << tools->timeToString(startTime, true, true)
							<< "\n    Current leader time: " << tools->timeToString(leaderTime, true, true);
					}

					// Add warning section to main report only if we have warnings
					if (hasCriticalWarning) {
						syncPointsReport << tools->getColoredString(warningReport.str(), eColor::cyborgBlood);

						// Issue dedicated warning log
						tools->logEvent(
							tools->getColoredString("[CRITICAL SYNC WARNING] ", eColor::cyborgBlood) +
							"First checkpoint is ahead of current leader by " +
							std::to_string(startHeight - leaderHeight) + " blocks. " +
							"This may indicate a significant synchronization issue.",
							eLogEntryCategory::network,
							10,  // High priority
							eLogEntryType::warning,
							eColor::cyborgBlood
						);
					}
					else {
						syncPointsReport << "\n[ Warning Analysis ]"
							<< "\n    No critical warnings detected";
					}
				}
				// Warnings - END

				syncPointsReport << tools->getColoredString("\n[ Synchronization Points Analysis] - END", eColor::lightCyan);



				tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + syncPointsReport.str(),
					eLogEntryCategory::network, 1, eLogEntryType::notification);
			}
			// Logging - END

			task->setData(tools->BERVector(checkpoints));
		}
		else
		{
			if ((std::time(0) - farAwayCPRequetedTS) < GenesisCPRequestInterval)
			{
				if (logDebug)
				{
					tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " unable to prepare synchronisaiton points. Won't request chain-proof from " 
						+ ip + " - Genesis-Chain requested recently.",
						eLogEntryCategory::network, 1, eLogEntryType::notification);
				}
				return false;
			}
			else {
				// we're effectively requesting a full chain-proof
				bm->pingFarAwayCPRequestedTimestamp();
			}
		}
		// Prepare Synchronisation Point Porposals - END

		tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + " block header received from " + 
			ip + " is of interest. "+ tools->getColoredString("Requesting chain-proof now..", eColor::orange),
			eLogEntryCategory::network, 3, eLogEntryType::notification, eColor::lightGreen);
		addTask(task);
		//Request Chain-Proof - END
	}

	return true;
	// Execution - END
	//Operational Logic - END
}


/// <summary>
/// Handles an incoming block notification from the network.
/// Processes full block bodies (data or key blocks) and validates them before queuing.
/// Ensures blocks are properly instantiated, verified, and were previously requested
/// through the Heaviest Chain Proof protocol.
/// </summary>
/// <param name="netmsg">Network message containing the block data</param>
/// <param name="socket">UDT socket connection identifier</param>
/// <param name="webSocket">Web socket connection, if applicable</param>
/// <returns>True if block was successfully processed and queued, false otherwise</returns>
bool CConversation::handleNotifyNewBlockBodyMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,
	std::shared_ptr<CWebSocket> webSocket)
{
	// Pre-Flight Checks - BEGIN
	if (!getIsSyncEnabled())
		return false;

	std::shared_ptr<CTools> tools = getTools();
	std::vector<uint8_t> BERBlock = netmsg->getData();

	if (BERBlock.size() == 0 || BERBlock.size() > CGlobalSecSettings::getMaxDataBlockSize())
		return false;
	// Pre-Flight Checks - END

	// Block Instantiation - BEGIN
	eBlockInstantiationResult::eBlockInstantiationResult iResult;
	std::string errorInfo;
	std::shared_ptr<CBlock> block = CBlock::instantiateBlock(true, BERBlock, iResult,
		errorInfo, mBlockchainManager->getMode());

	if (iResult == eBlockInstantiationResult::Failure || !block)
	{
		tools->writeLine(tools->getColoredString(
			"[Error]: unable to instantiate the received block.",
			eColor::cyborgBlood));
		return false;
	}
	// Block Instantiation - END

	// Security Validation - BEGIN
	std::vector<uint8_t> confirmedBlockID = block->getID();
	// Notice: Block is rejected if effective header hash differs from expected
	if (!mBlockchainManager->isBlockHunted(confirmedBlockID))
	{
		// delivery node would be penalized once thus method returns
		return false;
	}
	// Security Validation - END

	// Block Processing - BEGIN
	// Process only blocks referenced within Heaviest Chain Proof
	// These must have been ordered by Network Manager in current session
	return mBlockchainManager->pushBlock(block, false);
	// Block Processing - END
}

/// <summary>
/// Called whenever the other peer provides a Chain-Proof.
/// </summary>
/// <param name="netmsg"></param>
/// <param name="socket"></param>
/// <param name="webSocket"></param>
/// <returns></returns>
bool CConversation::handleNotifyChainProofMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	if (!getIsSyncEnabled())
		return false;

	uint64_t longCPSize = 100;
	std::shared_ptr<CTools> tools = getTools();
	//Preliminaries - BEGIN
	std::shared_ptr<CNetworkManager>  nm = getNetworkManager();
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	if (!nm || !bm)
	{
		return false;
	}
	if (nm->getIsNetworkTestMode())
	{
		return false;
	}

	if (bm->getDoNotProcessExtrernalChainProofs())
	{
		tools->logEvent(tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) + tools->getColoredString(" external chain-proof processin diabled.",
			eColor::cyborgBlood), eLogEntryCategory::localSystem, 10, eLogEntryType::notification);

		return false;
	}
	//Preliminaries - END

	//Local Variables - BEGIN
	std::string IP = getIPAddress();
	std::vector<std::vector<uint8_t>> chainProof;
	std::vector<std::vector<uint8_t>> resultingCompleteChainProof;
	uint64_t resultingTotalPow=0;
	uint64_t blocksScheduledForDownload = 0;
	std::vector<std::vector<uint8_t>> localChainProof = getLocalChainProof();
	std::vector<uint8_t> bytes = netmsg->getData();
	//Local Variables - END

	//Operational Logic - BEGIN

	if (netmsg->getDataSize() >= ((double)CGlobalSecSettings::getMaxUDTNetworkPackageSize() * (double)0.9))
	{
		tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) +  tools->getColoredString("excessive chain-proof received from " + IP, eColor::cyborgBlood), eLogEntryCategory::network, 4, eLogEntryType::warning);
		issueWarning();
	}

	///Check if chain proof was processed recently - BEGIN
	if (!localChainProof.empty() && bm->wasChainProofRecentlyProcessed(bytes, true))// we need local per-conversation chain-proof always so that we know from whom to download.
	{
		tools->logEvent(tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) + "the received Chain-Proof was processed recently, neglecting...", eLogEntryCategory::network, 1, eLogEntryType::notification);
		return true;
	}
	///Check if chain proof was processed recently - END

	if (!tools->BERVectorToCPPVector(bytes, chainProof))
	{
		tools->logEvent(tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) + tools->getColoredString("invalid Chain-Proof bytes received.", eColor::cyborgBlood),  eLogEntryCategory::localSystem, 10, eLogEntryType::notification);
		notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit);
		issueWarning();
		return false;
	}

	// [ Security ] Long Chain-Proof Received Throttling - BEGIN
	uint64_t now = std::time(0);
	uint64_t castCPProcessedAt = getConvLocalLongCPProcessedAt();

	if (chainProof.size() > longCPSize)
	{
		if (
			(now >= castCPProcessedAt
				&& (now - castCPProcessedAt) < CGlobalSecSettings::getMinLongCPConvLocalProcessingInterval()))
		{
			tools->logEvent(tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) +
				"excessively long Chain-Proosf received from " + getIPAddress(),
				eLogEntryCategory::localSystem,1, eLogEntryType::notification);

			issueWarning();
			return false;
		}
		
	}

	// [ Security ] Long Chain-Proof Received Throttling - END

	//wait for the mutex to be locked mHeaviestPathGuardian of blockchain-manager
	//keep checking if conversations shutdown was ordered if so - abort
	bool heaviestPathLocked = false;
	

	// CVE_901 - network black-holing through a  chain-proof flood
	// Define a reasonable timeout for chain proof acquisition
	const uint64_t CHAIN_PROOF_LOCK_TIMEOUT_MS = 3000; // 3 seconds max wait
	uint64_t startTime = tools->getTime();
	uint64_t waitTime = 20; // Start with smaller wait time
	bool timeoutOccurred = false;

	
	do {
		// Check if we should abort
		if (getState()->getCurrentState() == eConversationState::ending) {
			return false;
		}

		// Attempt to get the lock
		heaviestPathLocked = bm->attemptToLockHeaviestPath();

		if (heaviestPathLocked) {
			break; // Successfully acquired the lock
		}

		// Check for timeout
		if ((tools->getTime() - startTime) > CHAIN_PROOF_LOCK_TIMEOUT_MS) {
			timeoutOccurred = true;
			break;
		}

		// Progressive backoff to reduce contention
		Sleep(waitTime);

		// Increase wait time exponentially, up to a reasonable max
		waitTime = min(waitTime * 2, static_cast<uint64_t>(500));

	} while (!heaviestPathLocked);

	// Handle timeout case
	if (timeoutOccurred) {
		tools->logEvent(tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) +
			"Timed out waiting for heaviest path lock. Will try again later.",
			eLogEntryCategory::network, 1, eLogEntryType::warning);

		// Notify peer that we're busy (optional)
		notifyOperationStatus(eOperationStatus::interrupted, eOperationScope::dataTransit);
		return false;
	}

	// Continue with chain proof processing if lock was acquired...

	tools->logEvent("Preliminary analysis of the received chain-proof", "Chain-Proof", eLogEntryCategory::localSystem,2, eLogEntryType::notification, eColor::orange);
	/*
	if (!mBlockchainManager->getChainProofCumulativePoW(chainProof, resultingTotalPow))
	{
		
		issueWarning();
		tools->logEvent(tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) + tools->getColoredString(" invalid chain-proof received. Warning issued.", eColor::cyborgBlood), eLogEntryCategory::localSystem, 1,
			eLogEntryType::notification);
		if (heaviestPathLocked)
		{
			heaviestPathLocked = false;
			mBlockchainManager->unlockHeaviestPath();
		}
		return false;
	}
	
	if (!resultingTotalPow)
	{//even partial chain-proofs need to hold PoW.
		issueWarning();
		tools->logEvent(tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) + tools->getColoredString(" invalid chain-proof received (no PoW). Warning issued.", eColor::cyborgBlood), eLogEntryCategory::localSystem, 1,
			eLogEntryType::notification);
		if (heaviestPathLocked)
		{
			heaviestPathLocked = false;
			mBlockchainManager->unlockHeaviestPath();
		}
		return false;
	}*/

	//Analyze the received chain-proof and see its relation with the local best known.
	eChainProofValidationResult::eChainProofValidationResult result = eChainProofValidationResult::invalidGeneral;


	if (chainProof.size() > longCPSize)
	{
		pingConvLocalLongCPProcessed();//update last time when a long chain was allowed through (conversation scope).
	}
	// Logging of Received Chain-Proof - BEGIN
	if (!chainProof.empty()) {
		CBlockHeader::eBlockHeaderInstantiationResult ebhrFirst, ebhrSecond, ebhrLast;
		std::string errFirst, errSecond, errLast;
		std::stringstream report;
		std::shared_ptr<CBlockHeader> lastHeader;
		report << "[ Received Chain-Proof Analysis ] from: "<< getIPAddress() <<"\n";

		// Track first key block position
		int64_t firstKeyBlockPos = -1;

		// Instantiate first header
		std::shared_ptr<CBlockHeader> firstHeader = CBlockHeader::instantiate(chainProof[0], ebhrFirst, errFirst, false, bm->getMode());
		if (firstHeader) {
			report << "[ First Block ]  Height: " << firstHeader->getHeight()
				<< " Key Height: " << firstHeader->getKeyHeight()
				<< " Type: " << (firstHeader->isKeyBlock() ? "Key" : "Data")
				<< " Timestamp: " << firstHeader->getSolvedAtTime()
				<< " Parent: " << tools->base58CheckEncode(firstHeader->getParentID())
				<< " ID: " << tools->base58CheckEncode(CCryptoFactory::getInstance()->getSHA2_256Vec(chainProof[0]));

			if (firstHeader->isKeyBlock()) {
				firstKeyBlockPos = 0;
			}
		}
		else {
			report << "[ First Block ]  Unable to instantiate header. Error: " << errFirst;
		}

		// Examine second block if available
		if (chainProof.size() > 1) {
			std::shared_ptr<CBlockHeader> secondHeader = CBlockHeader::instantiate(
				chainProof[1], ebhrSecond, errSecond, false, bm->getMode());

			if (secondHeader) {
				report << "\n[ Second Block ] Height: " << secondHeader->getHeight()
					<< " Key Height: " << secondHeader->getKeyHeight()
					<< " Type: " << (secondHeader->isKeyBlock() ? "Key" : "Data")
					<< " Timestamp: " << secondHeader->getSolvedAtTime()
					<< " Parent: " << tools->base58CheckEncode(secondHeader->getParentID())
					<< " ID: " << tools->base58CheckEncode(CCryptoFactory::getInstance()->getSHA2_256Vec(chainProof[1]));

				if (firstKeyBlockPos == -1 && secondHeader->isKeyBlock()) {
					firstKeyBlockPos = 1;
				}
			}
			else {
				report << "\n[ Second Block ] Unable to instantiate header. Error: " << errSecond;
			}
		}

		// If first key block wasn't in first two positions, find it
		if (firstKeyBlockPos == -1) {
			for (size_t i = 2; i < chainProof.size(); i++) {
				CBlockHeader::eBlockHeaderInstantiationResult ebhr;
				std::string err;
				std::shared_ptr<CBlockHeader> header = CBlockHeader::instantiate(
					chainProof[i], ebhr, err, false, bm->getMode());
				if (header && header->isKeyBlock()) {
					firstKeyBlockPos = i;
					report << "\n[ First Key Block Found ] at position: " << i
						<< " Height: " << header->getHeight()
						<< " Key Height: " << header->getKeyHeight()
						<< " Timestamp: " << header->getSolvedAtTime()
						<< " Parent: " << tools->base58CheckEncode(header->getParentID())
						<< " ID: " << tools->base58CheckEncode(CCryptoFactory::getInstance()->getSHA2_256Vec(chainProof[i]));
					break;
				}
			}
		}

		// Report if no key block was found
		if (firstKeyBlockPos == -1) {
			report << "\n[ Warning ] No key block found in chain proof!";
		}

		// Examine last block
		if (chainProof.size() > 1) {
			lastHeader = CBlockHeader::instantiate(
				chainProof[chainProof.size() - 1], ebhrLast, errLast, false, bm->getMode());

			if (lastHeader) {
				report << "\n[ Last Block ]   Height: " << lastHeader->getHeight()
					<< " Key Height: " << lastHeader->getKeyHeight()
					<< " Type: " << (lastHeader->isKeyBlock() ? "Key" : "Data")
					<< " Timestamp: " << lastHeader->getSolvedAtTime()
					<< " Parent: " << tools->base58CheckEncode(lastHeader->getParentID())
					<< " ID: " << tools->base58CheckEncode(CCryptoFactory::getInstance()->getSHA2_256Vec(chainProof[chainProof.size() - 1]));
			}
			else {
				report << "\n[ Last Block ]   Unable to instantiate header. Error: " << errLast;
			}
		}

		// Add statistical information
		report << "\n[ Chain Stats ]"
			<< "\n    Total Elements: " << chainProof.size()
			<< "\n    First Key Block Position: " << (firstKeyBlockPos >= 0 ? std::to_string(firstKeyBlockPos) : "Not Found")
			<< "\n    Chain Start Height: " << (firstHeader ? std::to_string(firstHeader->getHeight()) : "Unknown")
			<< "\n    Chain End Height: " << (chainProof.size() > 1 && lastHeader ? std::to_string(lastHeader->getHeight()) :
				(firstHeader ? std::to_string(firstHeader->getHeight()) : "Unknown"));

		tools->logEvent(tools->getColoredString("[Chain-Proof Analysis]: ", eColor::lightCyan) + report.str(),
			eLogEntryCategory::network, 1, eLogEntryType::notification);
	}
	// Logging of Received Chain-Proof - END

	// Chain Proof Validation - BEGIN
	// todo: consider making this asynchronous - for instance scheduling for async processing with CBlockchainManager
	//		 This would require peer inofmration to be passed along so that final security penalty could be imposed if needed.
	eChainProofUpdateResult::eChainProofUpdateResult  res = bm->analyzeAndUpdateChainProofExt(chainProof, result, resultingCompleteChainProof, resultingTotalPow, blocksScheduledForDownload, bytes, getCustomBarID(), false);//do NOT update Cold Storage. It would be updated later on.
	// Chain Proof Validation - END

	// Local Conversation Scope Chain Proof Actuation - BEGIN
	if (res != eChainProofUpdateResult::invalidData && res != eChainProofUpdateResult::Error)
	{
		//store chain-proof locally to avoid network eclipsing attacks but also wasted storage and processing.
		setLocalChainProof(resultingCompleteChainProof); //  this takes lots of RAM (20+ MB)
	}
	// Local Conversation Scope Chain Proof Actuation - END	

	if (res != eChainProofUpdateResult::updated && res != eChainProofUpdateResult::updatedLocalBestKnown)
	{
		bm->clearFarAwayCPRequestedTimestamp(); // clear timer so that a long chain-proof may be requested from other nodes

		// at same time conversatoin local timer is kept track of through pingConvLocalLongCPProcessed() so that a long chain-proof wouldn't be processed from this node anytime soon.

	}
	else
	{
		nm->forceSyncCheckpointsReferesh(); // force synchronisation checpoints to be refreshed
	}

	if (heaviestPathLocked)
	{
		heaviestPathLocked = false;
		bm->unlockHeaviestPath();
	}
	
	// Reporting - BEGIN
	switch (res)
	{
	case eChainProofUpdateResult::updated:
		tools->logEvent(
			tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) +
			"Chain proof updated. The resulting cumulative PoW is " +
			std::to_string(resultingTotalPow) +
			" Blocks scheduled for download: " +
			tools->getColoredString(std::to_string(blocksScheduledForDownload), eColor::lightCyan),
			eLogEntryCategory::localSystem, 10, eLogEntryType::notification);
		break;

	case eChainProofUpdateResult::updatedLocalBestKnown:
		tools->logEvent(
			tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) +
			tools->getColoredString("Heaviest Chain Proof updated.", eColor::lightGreen) +
			" The resulting cumulative PoW is " + std::to_string(resultingTotalPow) +
			" Blocks scheduled for download: " +
			tools->getColoredString(std::to_string(blocksScheduledForDownload), eColor::lightCyan),
			eLogEntryCategory::localSystem, 10, eLogEntryType::notification);
		break;

	case eChainProofUpdateResult::totalDiffLower:
		tools->logEvent(
			tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) +
			tools->getColoredString("No need to update the history of events.", eColor::orange),
			eLogEntryCategory::localSystem, 1, eLogEntryType::notification);
		break;

	case eChainProofUpdateResult::noCommonPointFound:
		tools->logEvent(
			tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) +
			tools->getColoredString("No common point in the history of events found.", eColor::cyborgBlood),
			eLogEntryCategory::localSystem, 1, eLogEntryType::warning);
		break;

	case eChainProofUpdateResult::invalidData:
		tools->logEvent(
			tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) +
			tools->getColoredString("Invalid chain-proof received. Reason: '", eColor::lightPink) +
			mTools->CPValidationResultToString(result)+
			"' Warning issued to " + tools->bytesToString(getEndpoint()->getAddress()) + ".",
			eLogEntryCategory::network, 1, eLogEntryType::notification);

		// Punish peer
		issueWarning();
		break;

	case eChainProofUpdateResult::Error:
		tools->logEvent(
			tools->getColoredString("[Chain Proof]: ", eColor::lightCyan) +
			tools->getColoredString("Received chain-proof caused undefined error. Warning issued.", eColor::cyborgBlood),
			eLogEntryCategory::network, 1, eLogEntryType::warning);

		// Issue warning for the peer causing unexpected results
		issueWarning();
		break;

	default:
		break;
	}

	// Reporting - END

	
	//Operational Logic - END
	return true;
}

bool CConversation::handleNotifyLongestPathMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}
bool CConversation::handleProcessSDPMsg(std::shared_ptr<CSDPEntity> sdpMsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{

	//preliminaries
	if (sdpMsg == nullptr)
		return false;

	//Local Variables - BEGIN
	std::shared_ptr<CIdentityToken> token;
	std::vector<uint8_t> swarmID;
	std::shared_ptr<CWebRTCSwarm> swarm;
	CStateDomain* domain = nullptr;
	bool doSwarmAuth = false;//todo: allow to set
	sdpMsg->setConversation(shared_from_this());
	swarmID = sdpMsg->getSwarmID();
	std::shared_ptr<CNetworkManager> nm = getNetworkManager();
	std::shared_ptr<CWebSocketsServer> wss;

	if (nm)
	{
		wss = nm->getWebSocketsServer();
	}

	//Local Variables - END

	//Security - BEGIN
	if (doSwarmAuth)
	{
		if (sdpMsg->getSourceID().size() == 32)
		{
			domain = getBlockchainManager()->getStateDomainManager()->findByPubKey(sdpMsg->getSourceID());
		}
		else
		{
			domain = getBlockchainManager()->getStateDomainManager()->findByFriendlyID(CTools::getInstance()->bytesToString(sdpMsg->getSourceID()));
		}

		if (domain == nullptr)
			return false;

		token = domain->getIDToken();

		if (token == nullptr)
			return false;
		if (token->getConsumedCoins() == 0)
			return false;
		std::vector<uint8_t> pubKey = token->getPubKey();

		if (pubKey.size() != 32)
			return false;
		if (!sdpMsg->validateSignature(pubKey))
			return false;
	}
	//Security - END

	//Operational Logic - BEGIN
	if (!wss)
	{
		return false;
	}
	//Critical Section - BEGIN
	wss->lockSwarms(true);
	if (swarmID.size() > 0) {
		swarm = wss->getSwarmByID(swarmID);
	}
	else
	{
		swarm = wss->getRandomSwarm();
	}
	
	if (swarm == nullptr)
	{
		//create instance of a particular Swarm on this full-node.
		//the node will exchange/route data for this Swarm through inter-full-node Kademlia communication sub-system.
		swarm = wss->createSwarmWithID(swarmID);
	}
	wss->lockSwarms(false);
	//Critical Section - FALSE

	if (swarm != nullptr)
	{
		if(isSDPRoutable(sdpMsg))
		swarm->addSDPEntity(sdpMsg);//there it will be processed further
		else
		{
			localSDPProcessing(sdpMsg);
		}
	}

	return true;
	//Operational Logic - END

}

/// <summary>
/// The function does processing of SDP messages which are not intended for processing by the Swarm's Controller.
/// Oftentimes these might include messages which ARE NOT intended to affect the rest of the Swarm.
/// </summary>
/// <param name="sdp"></param>
/// <returns></returns>
bool CConversation::localSDPProcessing(std::shared_ptr<CSDPEntity> sdp)
{
	if (sdp == nullptr)
		return false;
	switch (sdp->getType())
	{
	case eSDPEntityType::pingFullNode:
		getState()->ping();
		break;
	default:
		break;
	}

	return true;
}

/// <summary>
/// Note, SDP might require additional (re)processing by Swarm's Controller though.
/// </summary>
/// <param name="sdp"></param>
/// <returns></returns>
bool CConversation::isSDPRoutable(std::shared_ptr<CSDPEntity> sdp)
{
	if (sdp == nullptr)
		return false;

	if (sdp->getType() == eSDPEntityType::pingFullNode)
		return false;

	return true;

}
/// <summary>
/// Takes care of DFS Messages' processing.
/// The messages are processed by the main VM by default, one associated with the Conversation.
/// The DFS-datagram MAY specify another VM for processing by providing its identifier and setting the appropriate dfsFlag.
/// </summary>
/// <param name="msg"></param>
/// <param name="socket"></param>
/// <param name="webSocket"></param>
/// <returns></returns>
bool CConversation::handleProcessDFSMsg(std::shared_ptr<CDFSMsg> msg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{

	//There are a couple of ways this could go.
	//1) The UI dApp might be willing to spawn a VM of its own. This would not affect the session-main VM when errors occur its state wouldn't be WIPED
	//2) The UI dApp might CHOOSE to suspend throws on error by specifiec tghe NO_THROW flag in CDFSMfg. Such a flag would be then set in the VM and unset once first DFS command gets executed.
	//In such a case on error(i.e. path traversal fails / file read/write fails). The state of the VM would persist. A standard (NON-DFS) COperationStatus would be delivered to caller if reachable.
	//3) By default in case of an error, the VM issues sig-abort and caller is notified through DFS-Error message containing further information. Such an error would be shown next to the Magic Button
	//in Web-UI wheb hovered. The sandbox's state would be lost.
	if (msg == nullptr)
		return false;


	//Local Variables - BEGIN
	const eDFSCmdType::eDFSCmdType mType = msg->getType();
	eDFSCmdType::eDFSCmdType DFSResponseType = eDFSCmdType::eDFSCmdType::unknown;
	eNetReqType::eNetReqType NETMsgResponseType = eNetReqType::eNetReqType::notify;

	//response type will be determined based on the received query
	//if processing by VM is successful, a response containing BER-Metadata will be generated
	//witht he same response-ID as ID of the incoming qeuery.
	std::shared_ptr<CTools> tools = getTools();
	std::shared_ptr<CDFSMsg> dfsMsg = nullptr;
	std::shared_ptr<CNetMsg> netMsg = nullptr;
	std::shared_ptr<CNetTask> task = nullptr;
	std::shared_ptr<CQRIntentResponse> qrResp= nullptr;
	std::string cmd;
	std::vector<uint8_t> retVec;
	BigInt  ERGUsed = 0;
	uint64_t topMostValue, currentKeyHeight = 0;
	std::string base58B;
	std::vector<uint8_t> dataToIncInBank1,dataToIncInBank2;
	std::shared_ptr<CDTI> dti;
	std::shared_ptr<SE::CScriptEngine> mainVM;
	std::shared_ptr<SE::CScriptEngine> targetThread;
	bool markAsReady = true;
	

	/*
		IMPORTANT: By default, before each thread 'begins', ' flag writes_only -set' is set so that data-reads ex. 'ls' or 'less' do not makt it into the final source-code.
	*/

	//Local Variables - END


	//Prepare VM - BEGIN
		//client did not init the VM-subsystem
		//let us do this for him/her right now
		

		if (!prepareVM(true, nullptr, msg->getRequestID(), msg->getThreadID()))
			enqeueDFSErrorMsg("Initializing requested thread failed.");

		mainVM = getSystemThread();

		if (!mainVM)
			return false;

		//Sub-threads Support- BEGIN
		if (msg->getFlags().subThreadProcessing)
		{
			targetThread = mainVM->getSubThreadByVMID(msg->getThreadID());
		}
		else
		{
			//the message is supposed to be processed by the system-thread
			targetThread = mainVM;
		}

		if (!targetThread)
		{
			enqeueDFSErrorMsg("Initialization of the requested DFS-thread failed.");
			return false;
		}
		//Sub-threads Support- END

	//Prepare VM - END
		//std::shared_ptr<SE::CScriptEngine> se = getScriptEngine();

	//Requested DFS-commands are translated on-the-fly to coresponding #GridScript instructions.
	//During their execution the VM's output will be muted and BER-Metadata generation will be turned on.
	//The resulting meta-data containing any kind of expected results will be encapsulated within a Response.
	
	std::vector<uint8_t> assetID;
	std::string shortAssetID, fullPath;
	std::string fileName, mainDir;
	bool ommitScriptProcessing = false;
	switch (mType)
	{
	case eDFSCmdType::fileCertRequest:
		//a file certificate is being requested
		ommitScriptProcessing = true;
		assetID = msg->getData1();

		if (assetID.size() != 32)
		{
			return false;
		}
		
		fileName = tools->base58Encode(std::vector<uint8_t> (assetID.begin(), assetID.begin() + 12)) + ".der";
		mainDir = getBlockchainManager()->getSolidStorage()->getMainDataDir();
	    fullPath = mainDir + CGlobalSecSettings::getCertsDirPath() + "\\" + fileName;

		dataToIncInBank2 = tools->readFileEx(fullPath);
		dataToIncInBank1 = assetID;

		if (dataToIncInBank2.size())
		{
			DFSResponseType = eDFSCmdType::eDFSCmdType::fileCert;
		}
		else
		{
			enqeueDFSErrorMsg("File certificate does not exist.", msg->getRequestID());
		}
		break;
	case eDFSCmdType::fileCert:
		ommitScriptProcessing = true;
		//a supposedly requested file certificate just arrived

		//check if we were awaiting data 

		//if not - issue a warning
		issueWarning();
		//store the certificate

		//if yes - request data as needed

		break;
	case eDFSCmdType::fileChunkRequest:
		ommitScriptProcessing = true;
		//a chunk of a file is being requested by another peer


		//check if dynamic data or file request

		//[Dynamic] - check for a locally available CCertificate containers


		//[File] - check for a corresponding CCertificate in NetworkManager(?)
		break;

	case eDFSCmdType::fileChunk:
		ommitScriptProcessing = true;
		//chunk of a file just arrived


		//check if we were awaiting data by checking  locally available CCertificate containers

		//if not - issue a warning
		issueWarning();
		//if yes - fill-in the data in pending container


		//check if we need more data and if so - issue a request
		break;
	case eDFSCmdType::init://DEPRECATED, the VM gets auto-prepare above.
		
		break;
	case eDFSCmdType::ready://ready-indicator
		break;
	case eDFSCmdType::error://error - more info in mData1
		break;
	case eDFSCmdType::exists://check if file exists
		break;
	case eDFSCmdType::enterDir:
		cmd = "cd '" + tools->bytesToString(msg->getData1())+"'";//+ " \n ls";
		DFSResponseType = DFSResponseType = eDFSCmdType::eDFSCmdType::directoryContent;
		break;
	case eDFSCmdType::createDir:
		if (!targetThread->getTransactionBegan())
			cmd += "sct " + getBlockchainManager()->getDefaultRealmName() + " \n flag writes_only -set \r bt";//notice: BT would take care of CDing into current path on its own
		cmd += " mkdir '" + tools->bytesToString(msg->getData1())+ "' \n "+ (markAsReady?" \r rt ":"")+"ls";
		DFSResponseType = DFSResponseType = eDFSCmdType::eDFSCmdType::directoryContent;
		break;
	case eDFSCmdType::readDir://list directory content
		cmd = "ls";
		DFSResponseType = eDFSCmdType::eDFSCmdType::directoryContent;
		break;
	case eDFSCmdType::readFile://retrieve file ; path in mData1
		cmd = "less '" + tools->bytesToString(msg->getData1())+"'";
		DFSResponseType = eDFSCmdType::eDFSCmdType::fileContent;//embedded within VM Meta-Data
		//todo: generate appropariate response
		break;
	case eDFSCmdType::writeFile://write file; path in mData1, content in mData2
		//the data will be stored as binary within the DS, still it needs to make it through as base58 encoded-data
		//within the transaction's source code. In the future we might be generating #GridScript-VM byte-code right from here.
		if (!targetThread->getTransactionBegan())
			cmd += "sct testnet \n flag writes_only -set \r bt";//notice: BT would take care of CDing into current path on its own
	
		//cmd += "setVar " + tools->bytesToString(msg->getData1()) + " " + tools->base58CheckEncode(msg->getData2()) + " -b";
		cmd += " cat -b64 '" + tools->base64Encode(msg->getData2());
		cmd+= +"' > '" + tools->bytesToString(msg->getData1()) + (markAsReady ? "' \r rt " : "");
		break;
	case eDFSCmdType::rename: // rename file / dir.pathA in mData1, pathB in mData2
		break;
	case eDFSCmdType::copy://copy file/dir. pathA in mData1, pathB in mData2
		break;
	case eDFSCmdType::unlink:
		break;
	case eDFSCmdType::search://find file/dir. Regex in mData1
		break;
	case eDFSCmdType::touch:
		break;
	case eDFSCmdType::sessionStat:
		break;
		//the following is used to REQUEST a commit.
	case eDFSCmdType::requestCommit: //request QRIntent
		if (!targetThread->hasCommitableCode())
		{
			enqeueDFSErrorMsg("Nothing to commit!", msg->getRequestID());
			return true;
		}
		cmd = "ct";
		DFSResponseType = eDFSCmdType::eDFSCmdType::commitSuccess;
		//store the QR-code within VM's meta-data and deliver in response
		break;
	case eDFSCmdType::QRIntentData://serialized QRIntent in mData1
		break;
	case eDFSCmdType::sync:
		cmd = "sync";
		DFSResponseType = eDFSCmdType::eDFSCmdType::syncSuccess;
		break;

		//The following is used to process and register QR-Intent *RESPONSE* locally
	case eDFSCmdType::commit://provide serialized QRIntentResponse in mData1
		qrResp = CQRIntentResponse::instantiate(msg->getData1());

		if (qrResp != nullptr)
		{
			retVec = targetThread->setQRIntentResponse(qrResp);
			if (retVec.size() == 0)
				enqeueDFSErrorMsg("Could not register QR-Response", msg->getRequestID());
			else
				enqeueDFSOk("",msg->getRequestID());
		}
		else
		{
			enqeueDFSErrorMsg("Invalid QR-Response", msg->getRequestID());
		}
		//DFSResponseType = eDFSCmdType::eDFSCmdType::QRIntentData;
		break;
	case eDFSCmdType::fileContent:// file path in mData1, content in mData2
		break;
	case eDFSCmdType::directoryContent://sequence of vectors in mData1
		break;
	case eDFSCmdType::enterDirLS:
		cmd = "cd '" + tools->bytesToString(msg->getData1())+ "' \n ls";
		DFSResponseType  = eDFSCmdType::eDFSCmdType::directoryContent;
		break;
	default:
		break;
	}

	//execute command by the VM - BEGIN
	if (msg->getFlags().suspendThrows)
	{
		targetThread->setRegN(REG_SUPPRESS_DFS_THROW);
	}
	else
	{
		targetThread->clearRegN(REG_SUPPRESS_DFS_THROW);
	}

	if (ommitScriptProcessing)
	{
		std::shared_ptr<CDFSMsg>  dfsMsg = std::make_shared<CDFSMsg>(DFSResponseType);
		dfsMsg->setRequestID(msg->getRequestID());
		dfsMsg->setData1(dataToIncInBank1);//DFS contains VMMetaData in the first data bank. Thus file content and/or directory listings
		//are encoded using VMMetaData-compatible sections/elements.
		if (dataToIncInBank2.size() > 0)
			dfsMsg->setData2(dataToIncInBank2);
		std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::DFS, NETMsgResponseType);
		netMsg->setData(dfsMsg->getPackedData());
		std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
		task->setNetMsg(netMsg);
		task->setData(netMsg->getPackedData());
		addTask(task);
		return true;
	}

	// Determine if this command requires detached (non-blocking) processing
	// Commands that wait for external data (like authentication) must use detached processing
	// to avoid blocking the WebSocket thread
	bool requiresDetachedProcessing = false;
	if (mType == eDFSCmdType::requestCommit)
	{
		// The 'ct' (commit) command requires authentication from browser via VM Meta Data protocol
		// If executed in blocking mode, the WebSocket thread will be blocked and unable to receive auth data
		// Result: DEADLOCK. Solution: use detached processing (spawns native thread, WebSocket stays responsive)
		requiresDetachedProcessing = true;
	}

	CReceipt r;

	if (requiresDetachedProcessing)
	{
		// Non-Conversation Blocking processing - BEGIN

		// This will spawn a new native thread.
		// The thread would be alive only during processing of the just received code-package.

		// The Decentralized Thread would keep track of all of the attached native threads and keep the REG_HAS_DETACHED_THREAD
		// registry flag set until all the attached native threads terminate. This is performed autonomously.
		// processScript() knows it's executing from a dedicated thread thanks to an appropriate parameter being set.

		size_t nativeThreadsCount = targetThread->getNativeThreadCount();

		if (!nativeThreadsCount)
		{
			// Only 1 native thread allowed per VM / Decentralized Thread.
			targetThread->addNativeThread(std::make_shared<std::thread>(std::bind(&SE::CScriptEngine::processScript, targetThread,
				cmd,
				std::vector<uint8_t>(),
				topMostValue,
				ERGUsed,
				currentKeyHeight,
				false,  // resetVM
				false,  // isTransaction
				std::vector<uint8_t>(),  // receiptID
				false,  // resetERGUsage
				true,   // GenerateBERMetadata
				true,   // muteDuringProcessing
				true,   // isCmdExecutingFromGUI
				false,  // isGUICommandLineTerminal
				msg->getRequestID(),  // metaRequestID - CRITICAL: used by notifyCommitStatus()
				true,   // resetOnException
				true,   // detachedProcessing - indicates execution from dedicated thread
				false   // excuseERGusage
			)));
		}
		else
		{
			// Usually additional incoming requests are not expected, Caller is expected to wait.
			// To assure responsiveness we shut-down any current processing.
			// Thus, we effectively assume the disruption being the caller's fault - not having awaited prior result.
			// Processing loop within any #GridScript command/app are supposed to react to these notifications and shut down immediately.
			targetThread->quitCurrentApp();
			targetThread->abortCurrentVMTask();
		}
		// Non-Conversation Blocking processing - END

		// Return immediately - WebSocket thread is now free to receive auth data
		// Results will be delivered asynchronously by the native thread via DFS messages
		// (commitSuccess, commitAborted, or error sent by notifyCommitStatus())
		return true;
	}
	else
	{
		// Conversation-blocking processing
		// No other datagrams could be handled until the processing by the thread below is finished.
		// Maintain backward compatibility for commands that don't need external data
		r = targetThread->processScript(
			cmd,
			std::vector<uint8_t>(),
			topMostValue,
			ERGUsed,
			currentKeyHeight,
			false,  // resetVM
			false,  // isTransaction
			std::vector<uint8_t>(),  // receiptID
			false,  // resetERGUsage
			true,   // GenerateBERMetadata
			true,   // muteDuringProcessing
			true,   // isCmdExecutingFromGUI
			false,  // isGUICommandLineTerminal
			0,      // metaRequestID (not needed for synchronous commands)
			false   // indicate non-detached processing
			// excuseERGusage defaults to false
		);
	}
	//execute command by the VM - BEGIN

	if (r.getResult() == eTransactionValidationResult::invalid)
	{
		std::string error = "";
		if (r.getLog().size() > 0)
		{
			//[todo:Major:medium]: check-out log-running in receipts, when handed to UI-sessions.
			
				error = r.getRenderedLog(3, getProtocol() == eTransportType::WebSocket?"<br/>":"\r\n",15);
			

			enqeueDFSErrorMsg(error, msg->getRequestID());
		}
		targetThread->clearErrorFlag();

	}
	else
	{
		if (mType == eDFSCmdType::commit)
		{
			dataToIncInBank2 =tools->stringToBytes(tools->base58CheckEncode(r.getGUID()));
		}
		//Response - BEGIN
		//here response to the requested Decentralized-File-System (DFS) action is delivered.
		//The type of CNetMSG container was set previously, above -> and so the type of DFS-Response.
		//ex. eDFSCmdType::readDir would result in a response of type eDFSCmdType::directoryContent
		if (DFSResponseType == eDFSCmdType::syncSuccess && r.getResult() == eTransactionValidationResult::invalid)
		{
			notifyOperationStatus(eOperationStatus::failure, eOperationScope::VM, targetThread->getID(), msg->getRequestID(), "Synchronization Failed");
			return false;
		}

		std::shared_ptr<CDFSMsg>  dfsMsg = std::make_shared<CDFSMsg>(DFSResponseType);
		dfsMsg->setRequestID(msg->getRequestID());
		dfsMsg->setData1(r.getBERMetaData());//DFS contains VMMetaData in the first data bank. Thus file content and/or directory listings
		//are encoded using VMMetaData-compatible sections/elements. 
		if(dataToIncInBank2.size()>0)
		dfsMsg->setData2(dataToIncInBank2);
		std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::DFS, NETMsgResponseType);
		netMsg->setData(dfsMsg->getPackedData());
		std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
		task->setNetMsg(netMsg);
		task->setData(netMsg->getPackedData());
		addTask(task);
		//Response - END

		//enqeueDFSOk();
	}
	//execute command by the VM - END

	return false;
}

void CConversation::unmuteVM()
{
	if (mScriptEngine == nullptr)
		return;
	mScriptEngine->setTextualOutputEnabled(true);
	mScriptEngine->setGenerateBERMetaData(false);
}

void CConversation::muteVM()
{
	if (mScriptEngine == nullptr)
		return;
	mScriptEngine->setTextualOutputEnabled(false);
	mScriptEngine->setGenerateBERMetaData();
}

bool CConversation::sendNetMsgRAW(bool doItNOW, eNetEntType::eNetEntType eNetType, eNetReqType::eNetReqType eNetReqType, std::vector<uint8_t> RAWbytes, bool pingStatus)
{
	return sendNetMsg(doItNOW,eNetType, eNetReqType, eDFSCmdType::unknown, std::vector<uint8_t>(), RAWbytes, pingStatus);
}

/// <summary>
/// Sends a netmsg container.
/// If nothing is to be changed within the container i.e. the container has been already fully-prepared encrypted or no encryption desired on purpose etc. 
/// Then, and only then, the 'doPreProcessing' flag should be set to False explicitly.
/// </summary>
/// <param name="msg"></param>
/// <param name="doItNow"></param>
/// <param name="doPreProcessing"></param>
/// <returns></returns>
bool CConversation::sendNetMsg(std::shared_ptr<CNetMsg> msg, bool doItNow, bool doPreProcessing)
{	
	if (msg == nullptr)
		return false;

	//Local variables - BEGIN
	std::shared_ptr<CNetTask> task;
	std::shared_ptr<CTools> tools = getTools();
	std::vector<uint8_t> bytes;// = msg->getPackedData();
	uint64_t size = 0;
	nmFlags mFlags = msg->getFlags();
	Botan::secure_vector<uint8_t> sk;
	conversationFlags cFlags = getFlags();
	std::string logInfo;
	//Local variables - END
	
	//Operational Logic - BEGIN

	if (doPreProcessing && doItNow)//do not do pre-processing if data is NOT to be shipped out right now - it would be done later on.
	{
		// Flags - BEGIN
		if (mFlags.authenticated == false) // do not break existing authentication
		{
			if (cFlags.localIsMobile)
			{
				mFlags.fromMobile = true;
			}

			msg->setFlags(mFlags);

		}
		// Flags - END
		
		//Authentication and encryption - BEGIN

		if (!authAndEncryptMsg(msg, sk, true))
		{
			tools->logEvent("EncAuth FAILED for: " + msg->getDescription(), eLogEntryCategory::network, 0, eLogEntryType::failure);
			return false;//abort
		}
		//Authentication and encryption - END
	}
	
	//Logging - BEGIN
	logInfo = "\n---- NetMsg Out " + tools->getColoredString(tools->msgEntTypeToString(msg->getEntityType())+ " "+
		tools->msgReqTypeToString(msg->getRequestType()), eColor::blue) + " " + std::string(doItNow ? "Now" : "Enqueued") + "\n [Endpoint]:" + getAbstractEndpoint()->getDescription() + msg->getDescription() + "\n----\n";

	tools->logEvent(logInfo, eLogEntryCategory::network, 0, eLogEntryType::notification);
	//Logging - END

	if (!doItNow)
	{
     	task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
		task->setNetMsg(msg);
		return addTask(task);
	}
	else
	{
		bytes = msg->getPackedData();
		size = bytes.size();
		return sendBytes(bytes);
	}
	//Operational Logic - END

	return false;
}

/// <summary>
/// Sends a CNetMsg container through the underlying transport layer.
/// Note: if doItNOW specified, data would be delivered synchronously during function's invocation. That would also omit and get ahead over any existing 
/// tasks within the network-tasks' processing queue. Which might lead to problems at the early stage of connection (like when the Conversation is set to require encryption
/// yet no peer's public-key nor symmetric key(session secret) is known as of yet).
/// 
/// </summary>
/// <param name="doItNOW"></param>
/// <param name="eNetType"></param>
/// <param name="eNetReqType"></param>
/// <param name="dfsCMDType"></param>
/// <param name="dfsData1"></param>
/// <param name="NetMSGRAWbytes"></param>
/// <returns></returns>
bool CConversation::sendNetMsg(bool doItNOW, eNetEntType::eNetEntType eNetType, eNetReqType::eNetReqType eNetReqType, eDFSCmdType::eDFSCmdType dfsCMDType,
	std::vector<uint8_t> dfsData1, std::vector<uint8_t> NetMSGRAWbytes, bool pingStatus)
{
	//Local variables - BEGIN
	std::shared_ptr<CTools> tools = getTools();
	std::shared_ptr<CDFSMsg>  dfsMsg;
	std::shared_ptr<CNetTask> task;
	std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetType, eNetReqType);
	std::vector<uint8_t> bytes;
	std::string logInfo;
	//Local variables - END

	//Operational Logic - BEGIN
	if (dfsCMDType != eDFSCmdType::unknown)
	{
		dfsMsg = std::make_shared<CDFSMsg>(dfsCMDType);
		dfsMsg->setData1(dfsData1);
		netMsg->setData(dfsMsg->getPackedData());
	}
	else
	{
		netMsg->setData(NetMSGRAWbytes);
	}

	//Authentication and encryption - BEGIN
	if (!authAndEncryptMsg(netMsg))
	{
		if (getLogDebugData())
			tools->logEvent("AuthEnc failed.", eLogEntryCategory::network, 0, eLogEntryType::failure);
		return false;//abort
	}

	//Authentication and encryption - END
	bytes = netMsg->getPackedData();

	//Logging - BEGIN
	logInfo = "\n---- NetMsg Out " + std::string(doItNOW ? "Now" : "Enqued") + "\n [Endpoint]:" + getAbstractEndpoint()->getDescription() + netMsg->getDescription()+"\n----\n";

	tools->logEvent(logInfo, eLogEntryCategory::network, 0, eLogEntryType::notification);
	//Logging - END

	if (!doItNOW)
	{
		task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
		task->setNetMsg(netMsg);
		return addTask(task);
	}
	else
	return sendBytes(bytes, pingStatus);
	//Operational Logic - END
}


/// <summary>
/// Authenticates and encrypts a CNetMsg as per the current CConversation's properties.
/// [IMPORTANT]: this method does NOT use the provided 'sessionKey' for encryption. INSTEAD, the establishes session key is RETURNED.
/// The function retrieves autonomously the already establishes session key OR the Session Info delivered by another peer and DECISED which method of encryption to use.
/// 
///		The function prefers session based encryption over ECIES encryption.
/// Thus:
/// - when session key already established it would be REUSED and returned.
/// - when session key NOT established, it would attempt to retrieve public key of the other peer from session data and compute session key which would be returned.
/// 
/// In any case, the provided session key is ALWAYS discarded.
/// 
/// ANY msg's non enc/auth-related (the function takes care of these) flags should be set by now since they get authenticated through encryption as well.
/// Any modifications to CNetMsg afterwards will make its authentication fail.
/// </summary>
/// <param name="msg"></param>
/// <returns></returns>
bool CConversation::authAndEncryptMsg(std::shared_ptr<CNetMsg> msg, const Botan::secure_vector<uint8_t> &sessionKey, bool doNotRequireEncryption)
{

	/*
	 [Notice]: if a message is already encrypted, and if it is already
	           authenticated, the function basically does almost nothing, BUT
	           for the ability to introduce a strong ECC signature if explicitly
	           required by the conversation's parameters. The only reason why
	           this method does NOT terminate on sight upon seeing an already
	           encrypted and authenticated (through Poly-1309) datagram.

	           'doNotRequireEncryption' argument - whether encryption is *REQUIRED*.

	           If encryption is REQUIRED and encryption fails - FALSE is returned.

	           If explicit signature is REQUIRED and signature fails - FALSE is returned.

	           Otherwise, TRUE is returned.
	*/

	if (msg == nullptr)
		return false;

	//Local Variables - BEGIN  (WARNING - order IMPORTANT)
	nmFlags flags = msg->getFlags();
	bool canEncrypt = false;
	bool encrypted = false;
	bool authenticated = false;
	bool alreadyEncrypted = flags.encrypted;

	if (alreadyEncrypted)
	{
		encrypted = true;
		canEncrypt = false;//PREVENT DOUBLE ENCRYPTION
	}

	if (msg->getSig().empty() == false)
		return false;


	// we want to PREVENT double encryption or double authentication or double signature.
	if (alreadyEncrypted && flags.authenticated && (msg->getSig().empty() == false || getDoSignOutgressMsgs() == false))// encrypted, authentcated, signature already present OR not required.
		return true;//no error, nothing to do.

	bool alreadyAuthenticated = flags.authenticated;

	std::shared_ptr<CTools> tools = getTools();
	std::shared_ptr<CSessionDescription> sd = getRemoteSessionDescription();

	//*DO NOT* attempt the below  since it creates a RACE CONDITION  on the external 'sessionKey' variable provided here by reference.
	//(const_cast<Botan::secure_vector<uint8_t>&>(sessionKey)).clear(); //a 'get away' for gcc and clang compiler, which do not allow for default non-const reference parameters
	//(const_cast<Botan::secure_vector<uint8_t>&>(sessionKey)).insert(sessionKey.begin(), t.begin(), t.end());
	
	//std::vector<uint8_t> data = msg->getData();// Avoid mem-copies
	bool msgHasData = msg->hasData(); //limit the number of invocations.
	bool effectiveEncryptionReq = getIsEncryptionRequired() && !doNotRequireEncryption && msgHasData && !alreadyEncrypted;// AVOID double-encryption.
	Botan::secure_vector<uint8_t> privateKey = getPrivKey();
	bool doDirectECIES = false;
	bool doSign = getDoSignOutgressMsgs() && privateKey.size() == 32;
	bool logDebugData = getLogDebugData();
	
	//Local Variables - END

	//UPDATE FLAGS - BEGIN
	//Note: update flags BEFORE the message container is possibly SIGNED *OR* encrypted (entire message gets authenticated)
	if (!alreadyEncrypted && effectiveEncryptionReq)
	{
		//1) [ Session Based ] First try to encrypt with SessionKey if available.

		//[Reason]: better performance and less network overhead.
		//[Discussion]: with a session-based encryption, authentication is as strong as the initially established DH secret for the session's duration.
		//Whereas, with session-less communication and with ECIES for each message - security is established anew for each CNetMsg.
		if (sessionKey.size() == 32)
		{
			// session based communication is to be used.
			doDirectECIES = false;
			canEncrypt = true;
		}

		//2) [ Session-Less ECIES ] Then, try ECIES encryption (only if public key of remote note is already available).
		else if (sd != nullptr && sd->getComPubKey().size() == 32)
		{
			doDirectECIES = true;
			canEncrypt = true;
		}

		flags.encrypted = canEncrypt;//set right now because we cannot later on. Mark as encrypted if already encrypted.

		flags.boxedEncryption = doDirectECIES && effectiveEncryptionReq;//todo: re-factor both types are AEAD-boxed;it's just whether session key is used for symmetric encryption of encrypted to public key(direct ECIES)
		flags.session = !doDirectECIES && effectiveEncryptionReq;
	}

		if (msg->getSig().empty() && getDoSignOutgressMsgs() && getPubKey().size() == 32)// do not double-sign.
			flags.authenticated = true;
		
		flags.useAEADpubKeyForSession = getUseAEADForSessionKey();//RECONSIDER

		if (!alreadyAuthenticated)
		{
			//thus we assume the operation will succeed, VERIFY at the end abort on error
			if (doSign || getUseAEADForAuth())
				flags.authenticated = true;//Note: signature-based authentication imposes Non-Repudiation, use ONLY when needed.
			//authentication between and in the eyes of just the two parties is maintained ANYWAY through:
			//Session: Poly1305 auth-code during DH-secret establishment and thus throughout the entire session under secret key's secrecy.
			//random IV generated for each message.
			//Non-Session based-communication: there's Poly1305 within each AEAD container.
			//Thus, do NOT use signature UNTIL non-repudiation IS desirable.

		}
		msg->setFlags(flags); // all flags need to be set right now.
		/* - NO MODIFICATION TO FLAGS ALLOWED BELOW, EVER - */

	//UPDATE FLAGS - END

		//Pre-Operational Logic - BEGIN
		if (effectiveEncryptionReq && !canEncrypt)
		{
			if (logDebugData)
			{
				tools->logEvent("Encryption required but keys not available.", eLogEntryCategory::network, 0, eLogEntryType::failure);
				/* 
				[Note]: it might happen so that sendNetMsg was invoked forcefully (with encryption required and  sendNow parameter set), but the 
				encryption capabilities have not been yet initialized (like when peer's pubKey is not yet known or session key not yet established through ECDH,
				in such a case data dispatch would fail. We do not want retransmission attempts.
				*/
				return false;
			}
		}
		//Pre-Operational Logic - END

	//Operational Logic - BEGIN

	if (effectiveEncryptionReq && msgHasData)
	{
		//ENCRYPTION - BEGIN

		if (!doDirectECIES)
		{// session base encryption

			// [Note]: the function below only RETURNS the 1) about to-be-established ECIES Session Key OR 2) the already established Session Key.
			// Thus the 'sessionKey' value provided   v---  it is always DISCARDED.
			encrypted = msg->encrypt(Botan::unlock(sessionKey), false); // boxed stream cipher instead of ECIES
		}
		else
		{
			// key availability verified in pre-logic
			encrypted = msg->encrypt(sd->getComPubKey(), true, sessionKey); //do ECIES against the recipient's public key.
		}
	}

	if (effectiveEncryptionReq && !encrypted && !doNotRequireEncryption)
	{
		if(logDebugData)
		tools->logEvent("Encryption of a datagram for " + tools->bytesToString(mEndpoint->getAddress()) + tools->getColoredString(" FAILED (Required).", eColor::cyborgBlood),
			eLogEntryCategory::network, 0);
		return false;//error
	}
	//ENCRYPTION - END

	//Authentication - BEGIN
	//[Discussion]: ECIES encryption is authenticated by default. The sessionKey was established during a DH-key exchange during which recipient got to know our public key.
	//Thus there's no need to authenticate further under strength of the encryption. We do NOT want non-repudiation on per datagram basis, thus signatures per message are NOT welcome.
	//With all that said, the Hello-Notify message contains a signature of challenge data-contained within extraBytes of the CNetMsg.
	//Further non-repudiation can be ENABLED with setDoSignOutgressMsgs()
	if (getDoSignOutgressMsgs())
	{
		if (logDebugData)
			tools->logEvent("Preparing an explicit signature.", eLogEntryCategory::network, 0);

		if (!msg->sign(privateKey))
		{
			if (logDebugData)
			tools->logEvent("Signature preparation failed.", eLogEntryCategory::network, 0,eLogEntryType::failure);
			return false;
		}
		authenticated = true;
	}
	//Authentication - END

	//Operational Logic - END

	if (!authenticated && doSign)
		return false; //something went wrong when signing

	return true;//return true even with no enc/auth was required. false indicated an error.
}



/*
* Prepares the main VM attached to conversation.
* Additional VMs may be spawned as threads through #GridScript.
*/
bool CConversation::prepareVM(bool reportStatus, std::shared_ptr<CDTI> dti, uint64_t requestID, std::vector<uint8_t> vmID, uint64_t processID)
{
	std::lock_guard<std::mutex> lock(mScriptEngineGuardian);

	std::future<bool> future = std::async(std::launch::async, [&]() {
	//Local Variables - BEGIN
	//bool isTargettingSystemThread = false;
	std::shared_ptr<CTools> tools = getTools();
	//Local Variables - END
	std::shared_ptr<CTransactionManager> tm = nullptr;
	std::shared_ptr<SE::CScriptEngine> se = nullptr;

	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		tm = mTransactionManager;
		se = mScriptEngine;
	}
	//[DTI support of Decentralized Processing Threads]
	/*
		As discussed with The Old Wizard on [01.04.22], the DTI interface now ended up providing a general-purpose apparatus for facilitating
		user<-> full-node data-request-response queries.

		As of now, sub-threads of the main thread are NOT guaranteed to inherit DTI instance of the System thread.
		+ check out how the initial log-me-in is performed (over Web-UI)
		[PauliX]: requestQRLogon() is called by VMContext, rendering QR Intent, client side. 
		+ allow for ad-hoc, on-demand initialization of DTI sub-system for any sub-thread when needed (DTI sessions do timeout), also these consume lots of resources.
		+ we upgrade dataQueryMethods of DTI to allow for specification of process' IDs (for in-UI-dApp data queries, besides global queries that display full-screen).
		  that is to allow for in-ui-dApp data-query Message-Boxes.
		+ this further employs the concept separate DTIs per UI dApp (this has been used by UI Terminal dApp since long).
	*/

	//First need to prepare both the System-thread and public read-only data-thread- BEGIN
	//presence of the main-VM is obligatory for sub-threads.
	//other threads are then available and committable from the main VM.
	if (tm == nullptr && se == nullptr)
	{
		std::vector<uint8_t> dataThreadID = tools->genRandomVector(8);// it is generated A-Piori so that we may dispatch notifications even BEFORE the thread is contructed.

		if (reportStatus)
		{
			sendNetMsgRAW(true, eNetEntType::VMStatus, eNetReqType::notify, tools->getSigBytesFromNumber(eVMStatus::initializing));	
		}
		//Note:here an owned (by CConversation) instance of Transaction Manager is instantiated, boarding an owned (by TM) instance of a Thread as well.
		//The Thread does NOT own the Transaction Manager, instead its kept track of only through a weak_ptr. 
		// 
		//The above ^ - stays in contrast with sub-threads- as in case of these, the ownership hierarchy is inverted - the specific instance of a Thread owns Transaction Manager
		//through a shared_pointer, thus preventing is from being destroyed until the specific instance of a Thread gets out of scope (i.e. until the CConvesation holding the System-thread
		//gets out of scope (i.e. is removed on disconnection etc.) which is when the Conversation's destructor would fire, causing its private system-thread to get out of scope, causing its
		//destructor to fire, which in turn would cause its mThreads container holding all of the sub-threads to get out of scope, causing their respective destructors to fire.. finally causing their
		//private shared_pointers to Transaction Manager getting out of scope as well and memory being freed.
	
		tm = std::make_shared<CTransactionManager>(eTransactionsManagerMode::Terminal, mBlockchainManager, nullptr, "", false, mBlockchainManager->getMode(), false, true, false, dti, true); // doBlockFormation=false, createDetachedDB=true, doNOTlockChainGuardian=false

		tm->initialize();//no external Thread in use. It will auto spawn new VM.
		se = tm->getScriptEngine();
		{
		    std::lock_guard<std::mutex> lock(mFieldsGuardian);
			mTransactionManager = tm;
			mScriptEngine = se;
		}
		//se->setID(tools->stringToBytes("system")); as discussed with TheOldWizard [02.09.21] the threads' IDs should be UNIQUE that also should hold true for system and data threads and any other threads as well.
		//the parties of interest are notified through the VM-Meta-Data protocol about the thread's properties. The recipient may look-up the notificaiton-encapsulated vmFlags
		//to check for additional data (whether it is a System Thread, a Data Thread etc.).

		//Initialize the System Thread- BEGIN
		if (reportStatus)
		{
			SE::vmFlags flags;
			flags.privateThread = true;
			flags.isThread = false;//i.e. a System thread.
			notifyVMStatus(eVMStatus::initializing, flags, 0 , se->getID(), std::vector<uint8_t>(), getID(), true, requestID);
		}
		se->setCommitTargetEx(eBlockchainMode::TestNet);
		se->setIsGUITerminalAttached();
		se->setTextualOutputEnabled(false);
		se->setConversation(shared_from_this());
		se->reset();

		se->refreshAbstractEndpoint(); //allow for routing to the System Thread
	
		se->setTextualOutputEnabled();
		se->setAllowLocalSecData(false);//do NOT allow for access to local sec-data store over remote sessions

		//by default, for now.
		//se->setTerminalMode(true);
		se->setERGLimit(CGlobalSecSettings::getSandBoxERGLimit());
		se->setOutputView(eViewState::eViewState::GridScriptConsole);
		se->enterSandbox();
		tm->setIsReady();

		if (reportStatus)
		{
			//only sent by the main VM
			sendNetMsgRAW(true, eNetEntType::VMStatus, eNetReqType::notify, tools->getSigBytesFromNumber(eVMStatus::ready));
			
			SE::vmFlags flags;
			notifyVMStatus(eVMStatus::ready, flags, 0,  se->getID(), std::vector<uint8_t>(), getID(), true, requestID);//used by sub-threads as well
		}
		//Initialize the System Thread- END
		
		//Initialize the Data Thread- BEGIN
		
		SE::vmFlags flagsD;
		flagsD.isDataThread = true;
		flagsD.isThread = true;
		flagsD.privateThread = false; //other may access freely. (multiple applications)
		flagsD.nonCommittable = true;//code within the Data-thread MAY NOT be committed. code from it would never be collected by the system thread.
		if (reportStatus)
		{
			notifyVMStatus(eVMStatus::initializing, flagsD, 0, dataThreadID, std::vector<uint8_t>(), getID(), true, requestID);
		}

		//within the below, a new DPT would be instantiated, along with a dedicated instance of Transactions Manager
		std::shared_ptr<SE::CScriptEngine> dataT;

		

		
		try {
		
			dataT = se->startThreadExC(dataThreadID, 0, 0, dti, flagsD);
		}
		catch (...)
		{
			dataT = nullptr;
		}

		//waiting for thread to start...

		//Initialize the Data Thread- END
		if (reportStatus)
		{
			if (!dataT)
			{
				SE::vmFlags flags;
				notifyVMStatus(eVMStatus::limitReached, flags, 0, vmID, std::vector<uint8_t>(), getID(), true, requestID);
			}
			else
			{
				notifyVMStatus(eVMStatus::ready, dataT->getFlags(), 0, dataT->getID(), std::vector<uint8_t>(), getID(), true, requestID);
			}
		}

	}//First need to the two initial threads- END

	//if (vmID.size() == 0)
		//return false; <= can't do that. The vmID MGIHT be actually EMPTY if we're targetting by processID.
	//if vmID is empty it's suffices to ensure that the System Thread is operational (already done).

	//Check if it's the System Thread itself - BEGIN
	//Note: all the other threads are sub-threads of the System-thread.
	if (vmID.size() > 0 && tools->compareByteVectors(vmID, se->getID()))
		return true;
	//Check if it's the System Thread itself - END


	//Second, prepare the sub-thread if requested - BEGIN
	if (vmID.size() > 0)
	{
		if (!se->isThreadRunning(vmID))
		{
			se->startThreadExC(vmID,requestID, processID, dti);
		}
	}
	else
	{
		//se->setThreadNumID(processID); //no process can own the System thread
	}
	//Second, prepare the sub-thread if requested - END

	if (tm == nullptr || se==nullptr)
	return false;
	else return true;
	});

	std::future_status status;
	eTransportType::eTransportType protocol = getProtocol();
	std::shared_ptr<CNetMsg> pingMsg;
	bool ready = false;
	do {
		switch (status = future.wait_for(500ms); status) {
		case std::future_status::deferred:
			ready = false;
			break;
		case std::future_status::timeout:
			//would be called on each wait_for()

			//dispatch keep-alive to the web-client.
			//outgress ping - BEGIN
			switch (protocol)
			{
			case eTransportType::SSH:
	
				break;
			case eTransportType::WebSocket:
				
				//pingMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::ping, eNetReqType::notify);
				//if (sendNetMsg(pingMsg, true))
				//{
				//	pingSent();
				//}

				break;
			case eTransportType::UDT:
				
			///	pingMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::ping, eNetReqType::notify);
				//if (sendNetMsg(pingMsg, true))
				//{
				//	pingSent();
				//}

				break;
			case eTransportType::local:
				break;
			default:
				break;
			}
			//outgress ping - END
			break;
		case std::future_status::ready:
			ready = true;
			break;
		}
	} while (status != std::future_status::ready);

	bool toRet = future.get();
	return toRet;
}

void CConversation::enqeueDFSErrorMsg(std::string errorMsg, uint64_t requestID)
{
	std::shared_ptr<CDFSMsg>   dfsMsg = std::make_shared<CDFSMsg>(eDFSCmdType::error);
	dfsMsg->setRequestID(requestID);
	dfsMsg->setData1(getTools()->stringToBytes(errorMsg));
	std::shared_ptr<CNetMsg> netMsg   = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::DFS, eNetReqType::notify);
	netMsg->setData(dfsMsg->getPackedData());
	std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
	task->setNetMsg(netMsg);
	addTask(task);
}

bool CConversation::enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::eVMMetaProcessingResult status, std::vector<uint8_t> BERMetaData,  uint64_t reqID, std::string message)
{
	mMetaGenerator->reset();
	mMetaGenerator->beginSection(eVMMetaSectionType::notifications);
	mMetaGenerator->addGridScriptResult(status, BERMetaData, reqID, message);
	mMetaGenerator->endSection();


	std::vector<uint8_t> meta = mMetaGenerator->getData();

	if (meta.size() == 0)
		return false;

	std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::VMMetaData, eNetReqType::notify);
	netMsg->setData(meta);
	std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
	task->setNetMsg(netMsg);
	addTask(task);
	return true;
}

bool CConversation::enqeueThreadOperationStatus(eThreadOperationType::eThreadOperationType oType, std::vector<uint8_t> threadID,  uint64_t reqID)
{
	mMetaGenerator->reset();
	mMetaGenerator->beginSection(eVMMetaSectionType::notifications);
	mMetaGenerator->addThreadOperationStatus(oType, threadID);
	mMetaGenerator->endSection();


	std::vector<uint8_t> meta = mMetaGenerator->getData();

	if (meta.size() == 0)
		return false;

	std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::VMMetaData, eNetReqType::notify);
	netMsg->setData(meta);
	std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
	task->setNetMsg(netMsg);
	addTask(task);
	return true;
}

void CConversation::enqueBlockBodyRequest(std::vector<uint8_t>& blockID, uint64_t reqID)
{
	std::lock_guard<std::mutex> lock(mPendingBlockBodyRequestsGuardian);
	mPendingBlockBodyRequests.push_back(std::make_tuple(blockID,std::time(0), reqID));
}

/// <summary>
/// Checks if block request is pending; automatically deques request on a timeout.
/// </summary>
/// <param name="blockID"></param>
/// <returns></returns>
bool CConversation::isRequestForBlockPending(std::vector<uint8_t>& blockID)
{
	if (blockID.size() == 0)
		return false;
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mPendingBlockBodyRequestsGuardian);
	
	for (uint64_t i = 0; i < mPendingBlockBodyRequests.size(); i++)
	{
		if (tools->compareByteVectors(std::get<0>(mPendingBlockBodyRequests[i]), blockID))
		{
			if ((std::time(0) - std::get<1>(mPendingBlockBodyRequests[i])) < CGlobalSecSettings::getMaxTimeBlockHunted())//a 1 minute timeout
			{
				return true;
			}
			else
			{
				tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) +
					" retrieval of block  " + tools->base58CheckEncode(std::get<0>(mPendingBlockBodyRequests[i])) + " from " + getIPAddress() + " timed out..", eLogEntryCategory::network, 3, eLogEntryType::failure, eColor::lightPink);
				mPendingBlockBodyRequests.erase(mPendingBlockBodyRequests.begin() + i);
				return false;
			}
		}
	}
	return false;
}

bool CConversation::markBlockRequestAsCompleted(std::vector<uint8_t>& blockID)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mPendingBlockBodyRequestsGuardian);

	
	for (uint64_t i = 0; i < mPendingBlockBodyRequests.size(); i++)
	{
		if (tools->compareByteVectors(std::get<0>(mPendingBlockBodyRequests[i]), blockID))
		{
			tools->logEvent(tools->getColoredString("[DSM-sync Conversation]:", eColor::lightCyan) +
				" marking request for block  " + tools->base58CheckEncode(blockID) + " from " + getIPAddress() + " as " + tools->getColoredString("completed.", eColor::lightGreen), eLogEntryCategory::network, 3, eLogEntryType::notification);

			mPendingBlockBodyRequests.erase(mPendingBlockBodyRequests.begin() + i);
			return true;
		}
	}

	return false;
}

bool CConversation::markBlockRequestAsCompleted(uint64_t requestID)
{
	if (!requestID)
	{
		return false;
	}

	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mPendingBlockBodyRequestsGuardian);

	
	for (uint64_t i = 0; i < mPendingBlockBodyRequests.size(); i++)
	{
		if (std::get<2>(mPendingBlockBodyRequests[i])== requestID)
		{
			tools->logEvent(tools->getColoredString("[DSM-sync Conversation]:", eColor::lightCyan) +
				" marking request for block request ID " + std::to_string(requestID) + " from " + getIPAddress() + " as " + tools->getColoredString("completed.", eColor::lightGreen), eLogEntryCategory::network, 1, eLogEntryType::notification);

			mPendingBlockBodyRequests.erase(mPendingBlockBodyRequests.begin() + i);
			return true;
		}
	}

	return false;
}

bool CConversation::markBlockRequestAsAborted(uint64_t requestID)
{
	if (!requestID)
	{
		return false;
	}

	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mPendingBlockBodyRequestsGuardian);

	
	for (uint64_t i = 0; i < mPendingBlockBodyRequests.size(); i++)
	{
		if (std::get<2>(mPendingBlockBodyRequests[i]) == requestID)
		{
			tools->logEvent(tools->getColoredString("[DSM-sync Conversation]:", eColor::lightCyan) +
				" marking request for block request ID " + std::to_string(requestID) + " from " + getIPAddress() + " as " + tools->getColoredString("aborted.", eColor::cyborgBlood), eLogEntryCategory::network, 0, eLogEntryType::failure);

			mPendingBlockBodyRequests.erase(mPendingBlockBodyRequests.begin() + i);
			return true;
		}
	}

	return false;
}

size_t CConversation::getPendingBlockBodyRequestsCount()
{
	std::lock_guard<std::mutex> lock(mPendingBlockBodyRequestsGuardian);
	return mPendingBlockBodyRequests.size();
}



bool CConversation::notifyVMStatus(eVMStatus::eVMStatus status, SE::vmFlags &flags,uint64_t processID, std::vector<uint8_t> vmID, std::vector<uint8_t> terminalID, std::vector<uint8_t> conversationID, bool doItNow, uint64_t inRespToReqID)
{
	mMetaGenerator->reset();
	mMetaGenerator->beginSection(eVMMetaSectionType::notifications);
	mMetaGenerator->addVMStatus(status, flags, processID, vmID, terminalID, conversationID.size()>0? conversationID:getID(), inRespToReqID);
	mMetaGenerator->endSection();

	std::vector<uint8_t> meta = mMetaGenerator->getData();

	if (meta.size() == 0)
		return false;

	std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::VMMetaData, eNetReqType::notify);
	netMsg->setData(meta);
	return sendNetMsg(netMsg, doItNow);
}

bool CConversation::enqeueVMMetaTerminalDataMsg(std::vector<uint8_t> message, eTerminalDataType::eTerminalDataType dType, uint64_t reqID, uint64_t appID)
{
	mMetaGenerator->reset();
	mMetaGenerator->beginSection(eVMMetaSectionType::notifications);
	mMetaGenerator->addTerminalData(message, std::vector<uint8_t>(), dType, reqID, appID);
	mMetaGenerator->endSection();
	std::vector<uint8_t> meta = mMetaGenerator->getData();

	if (meta.size() == 0)
		return false;

	std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::VMMetaData, eNetReqType::notify);
	netMsg->setData(meta);
	std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
	task->setNetMsg(netMsg);

	addTask(task);
	return true;

}
	bool CConversation::enqeueVMMetaUIAppDataMsg(std::vector<uint8_t> message , uint64_t reqID, uint64_t appID, std::vector<uint8_t> vmID)
	{
		mMetaGenerator->reset();
		mMetaGenerator->beginSection(eVMMetaSectionType::notifications);
		mMetaGenerator->addAppData(message, std::vector<uint8_t>(), reqID, appID,vmID);
		mMetaGenerator->endSection();
		std::vector<uint8_t> meta = mMetaGenerator->getData();

		if (meta.size() == 0)
			return false;

		std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::VMMetaData, eNetReqType::notify);
		netMsg->setData(meta);
		std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
		task->setNetMsg(netMsg);

		addTask(task);
		return true;
	}

void CConversation::enqeueDFSOk(std::string additionalInfo, uint64_t reqID)
{
	enqeueDFSOk(getTools()->stringToBytes(additionalInfo));
}

void CConversation::enqeueDFSOk(std::vector<uint8_t> additionaData, uint64_t reqID )
{
	std::shared_ptr<CDFSMsg>  dfsMsg = std::make_shared<CDFSMsg>(eDFSCmdType::ready);
	dfsMsg->setData1(additionaData);
	dfsMsg->setRequestID(reqID);
	std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::DFS, eNetReqType::notify);
	netMsg->setData(dfsMsg->getPackedData());
	std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
	task->setNetMsg(netMsg);
	addTask(task);
}

std::shared_ptr<SE::CScriptEngine> CConversation::getSystemThread()
{
	std::lock_guard<std::mutex> lock(mScriptEngineGuardian);
		
	return mScriptEngine;
}

/// <summary>r
/// Orders conversation's finalization.
/// IMPORTANT: this is to be called outside of the Conversation's Thread (future) ONLY. It's a public method.
/// Support both synchronous and asynchronous modes of operation.
/// </summary>
/// <param name="waitTillDone">The (async)chronous mode indicator.</param>
/// <returns></returns>
bool CConversation::end(bool waitTillDone, bool notifyPeer, eConvEndReason::eConvEndReason endReason)
{
	if (!waitTillDone && getState()->getCurrentState() == eConversationState::ending)
		return true;

	bool isFocusedOnNode = getNetworkManager()->getIsFocusingOnIP(getIPAddress());
	preventStartup();

	setEndReason(endReason);
	setAreKeepAliveMechanicsToBeActive(false);//thread responsible for keep-alive mechanics is to shut-down.
	getTools()->logEvent("Ordering a " +std::string(waitTillDone?"Blocking":"Non-Blocking") + " conversation's shut-down. [Conversation]: "+ getAbstractEndpoint()->getDescription(), eLogEntryCategory::network, 0, eLogEntryType::notification);
	
	if (notifyPeer)
	{
		std::shared_ptr< CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
		addTask(newTask);
	}
	getState()->setCurrentState(eConversationState::eConversationState::ending);

	if (getProtocol() == eTransportType::QUIC)
	{
		shutdownQUICConnection(notifyPeer==false?true:false);// attempt a graceful vs abrupt QUIC connection termination.
	}

	cleanUpDTIs(true);

	if (getIsThreadAlive() == false)
	{
		{
			std::lock_guard<std::recursive_mutex> lock(mConversationThreadGuardian);
			try
			{
				getState()->setCurrentState(eConversationState::eConversationState::ending);//just to be on the safe side (duplicate yet, thread is started before it is assigned to mConversationThreadGuardian)
				if (waitTillDone && mConversationThread.valid())// waitTillDone also here to support rare occasions isAlive would be set after the above check.
				{
					// we allow for a race condition on 'isThreadAlive' to improve performance.

					mConversationThread.get();
				}
			}
			catch (const std::future_error& e)
			{
			}

			try {
				if (waitTillDone && mPingMechanicsThread.valid())
					mPingMechanicsThread.get();
			}
			catch (const std::future_error& e)
			{
			}
		}


		if (isSocketFreed() == false)
		{

			closeSocket();
			cleanTasks();
		}
		getState()->setCurrentState(eConversationState::eConversationState::ended);
		return true;
	}

	if (isThreadValid())//don't attempt to make thread wait upon itself to shut-down
	{
		while (waitTillDone && getState()->getCurrentState() != eConversationState::eConversationState::ended)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));//Warning: look out not to attempt to join itself; that's precisely what joinable() checks against.

		}
		{
			std::lock_guard<std::recursive_mutex> lock(mConversationThreadGuardian);

			if (waitTillDone && isThreadValid())
				mConversationThread.get();
		}
		//this->getState()->setCurrentState(eConversationState::ended);//so to make sure that the below thread exists as well. <= *DO NOT* only thr thread can report on its status.
		//the below thread exits as a result of 
		setAreKeepAliveMechanicsToBeActive(false);

		{
			std::lock_guard<std::recursive_mutex> lock(mPingThreadGuardian);
			if (waitTillDone && isPingThreadValid())
				mPingMechanicsThread.get();
		}
	}
	else 
	{
		if (isSocketFreed()==false)
		{
			closeSocket();
			cleanTasks();
			Sleep(100);
		}
		getState()->setCurrentState(eConversationState::ended);
	}
	

	
	return true;
}

/// <summary>
/// Ends conversation. Important: to be called ONLY from within of the conversation's thread (future). Otherwise use 'end()'
/// </summary>
/// <param name="waitTillDone"></param>
/// <param name="notifyPeer"></param>
/// <param name="endReason"></param>
/// <returns></returns>
bool CConversation::endInternal(bool notifyPeer, eConvEndReason::eConvEndReason endReason)
{
	//IMPORTANT
	preventStartup();
	setEndReason(endReason);
	getTools()->logEvent("Ordering an internal conversations's shut-down. [Conversation]: " + getAbstractEndpoint()->getDescription(), eLogEntryCategory::network, 1, eLogEntryType::notification);
	
	if (notifyPeer)
	{
		std::shared_ptr< CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
		addTask(newTask);
	}
	getState()->setCurrentState(eConversationState::eConversationState::ending);

	cleanUpDTIs(true);

	if (getIsThreadAlive() == false)
	{
		setAreKeepAliveMechanicsToBeActive(false);//thread responsible for keep-alive mechanics is to shut-down.

		//this->getState()->setCurrentState(eConversationState::ended);//so to make sure that the below thread exists as well.
		if (isPingThreadValid())
			mPingMechanicsThread.get();//same as 'join()' but works for a 'future'.

		if (isSocketFreed() == false)
		{
			closeSocket();
			cleanTasks();
		}
		// [ IMPORTANT ]: only when main controller is down do we assume the conversation as already concluded.
		getState()->setCurrentState(eConversationState::eConversationState::ended);
		return true;
	}

	if (isSocketFreed() == false)
	{
		closeSocket();
		Sleep(100);//just in case (UDT-library level race conditions if any).
	}

	return true;
}

uint64_t CConversation::getNrOfBytesExchanged()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return totalNrOfBytesExchanged;
}

/// <summary>
/// Sets the requested challange i.e. the challange-data which we expect to receive the signature of.
/// </summary>
/// <param name="challange"></param>
void CConversation::setRequestedChallange(std::vector<uint8_t> challange)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mRequestedChallange = challange;
}

std::vector<uint8_t> CConversation::getRequestedChallange()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mRequestedChallange;
}

std::vector<uint8_t> CConversation::getExpectedPeerPubKey()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mExpectedPeerPubKey;
}

/// <summary>
/// Usually the public key delivered as part of CSessionDescription during the initial handshake.
/// </summary>
/// <param name="pubKey"></param>
void CConversation::setExpectedPeerPubKey(std::vector<uint8_t> pubKey)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mExpectedPeerPubKey = pubKey;
}

bool CConversation::getPeerAuthenticated()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mPeerAuthenticated;
}


void CConversation::setPeerAuthenticated(bool wasIT)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mPeerAuthenticated = wasIT;
}
void CConversation::setPeerAuthenticatedAsPubKey(const std::vector<uint8_t> &pubKey)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mPeerAuthenticatedAsPubKey = pubKey;
}

std::vector<uint8_t> CConversation::getPeerAuthenticatedAsPubKey()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mPeerAuthenticatedAsPubKey ;
}


uint64_t CConversation::getNrOfMsgsExchanged()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return nrOfMessagesExchanged;
}


bool CConversation::requestBlock(std::vector<uint8_t> blockID, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket, bool doItNow)
{
	if (!blockID.size())
		return false;
	uint64_t requestID = genRequestID();
	std::shared_ptr<CNetTask>task = std::make_shared<CNetTask>(eNetTaskType::requestBlock,1, requestID);//notice how meta-data-exchange sub-protocol request ID is used right over here
	//even though the VM-Meta-Data container is not actually being used. That's not really correct but lets say that it would minimize processing overheads..
	//the remote peer would use the ID to associate with request when replying with Operation Status (ex. should the requested block be unavailable at the remote peer).
	
	task->setData(blockID);
	addTask(task);
	enqueBlockBodyRequest(blockID, requestID);

	return true;
}

bool CConversation::handleBlockRequestMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

/// <summary>
/// Notice; this function is a handler of the NetMsg exchange protocol. It does NOT involve the Meta-Data-Exchange sub-system.
/// For any more sophisticated operations do consider using the latter.
/// </summary>
/// <param name="netmsg"></param>
/// <param name="socket"></param>
/// <param name="webSocket"></param>
/// <param name="doItNow"></param>
/// <returns></returns>
bool CConversation::handleBlockBodyRequestMsg(std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket, bool doItNow)
{
	if (!netmsg)
		return false;
	std::shared_ptr<CTools> tools = getTools();

	tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) +" processing block-body request from "+ getIPAddress(), eLogEntryCategory::network, 0, eLogEntryType::notification);
	//Local Variables - BEGIN
	std::vector<uint8_t> id = netmsg->getData();
	std::vector<uint8_t> extraBytes = netmsg->getExtraBytes();//where the request id should be
	BigInt requestID = extraBytes.size()?tools->BytesToBigInt(extraBytes):0;//we should probably be using the vm-meta-data exchange protocol but whatever...
	std::vector<uint8_t> blockData;
	std::shared_ptr<CNetMsg> msg;
	eBlockInstantiationResult::eBlockInstantiationResult result = eBlockInstantiationResult::Failure;
	//Local Variables - END

	if (id.size() != 32)
		return false;
	
	blockData = mBlockchainManager->getSolidStorage()->getBlockDataByHash(id);

	if (blockData.size() == 0)
	{
		tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) + getIPAddress()+ " requested an unknown block.", eLogEntryCategory::network, 1, eLogEntryType::notification);
		return notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit, id, static_cast<uint64_t>(requestID));//notifty the other peer that the block is not available.
	}
	notifyOperationStatus(eOperationStatus::success, eOperationScope::dataTransit, id, static_cast<uint64_t>(requestID));
	//Dispatch Block to the other Peer - BEGIN
	msg = std::make_shared<CNetMsg>(eNetEntType::blockBody, eNetReqType::process);
	msg->setData(blockData);

	return sendNetMsg(msg, doItNow);
	//Dispatch Block to the other Peer - END

}


bool CConversation::requestTransaction(std::vector<uint8_t> transactionID, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::handleTransactioneRequestMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::requestVerifiable(std::vector<uint8_t> verifiableID, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::handleVerifiableRequestMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::requestReceipt(std::vector<uint8_t> receiptID, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::handleReceiptRequestMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::requestVMMetaDataProcessing(std::vector<uint8_t> VMMetaDataBytes, bool doItNow)
{
	if (VMMetaDataBytes.size() == 0)
		return false;

	std::shared_ptr<CNetMsg> msg  =std::make_shared<CNetMsg>(eNetEntType::VMMetaData, eNetReqType::request);
	msg->setData(VMMetaDataBytes);
	
	return sendNetMsg(msg, doItNow);
}



/// <summary>
/// Intructs the other node to process our longestPath.
/// </summary>
/// <param name="path"></param>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::processLongestPath(std::vector<std::vector<uint8_t>> path, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{

	return false;
}

/// <summary>
/// Extracts a sequence of blockIDs from within the message.
/// The sequence *supposedly* represent the longest path.
/// A chainProof is required to validate this.
/// </summary>
/// <param name="netmsg"></param>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::handleProcessLongestPathMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}
/// <summary>
/// Intructs the other node to process a chainProof;
/// </summary>
/// <param name="path"></param>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::processChainProof(std::vector<std::vector<uint8_t>> path, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}
/// <summary>
/// Extracts the chainProof from within the data field; if found to represent a better path than the one already known
/// the current mBestKnownNetChainProof wihin the BlockchainManager is updated.
/// </summary>
/// <param name="path"></param>
/// <param name="socket"></param>
/// <returns></returns>
bool CConversation::handleProcessChainProofMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::processLatestBlockHeader(std::shared_ptr<CBlockHeader> header, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::handleProcessLatestBlockHeaderMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::processBlock(std::shared_ptr<CBlock> block, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

bool CConversation::handleProcessBlockMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	//we currently rely solely on 'notifications' for block-headers



	return false;
}
// Network Block Processing Methods - BEGIN

/// <summary>
/// Processes a block received in response to a block request.
/// Validates block integrity, ensures it was actually requested,
/// and handles the block through the blockchain manager.
/// Immediately returns false for invalid blocks, triggering remote node penalties.
/// </summary>
/// <param name="netmsg">Network message containing the block data</param>
/// <param name="socket">UDT socket connection identifier</param>
/// <param name="webSocket">Web socket connection, if applicable</param>
/// <returns>False if block is invalid or not requested, triggering remote node penalty; True if processed</returns>
bool CConversation::handleProcessBlockBodyMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,
	std::shared_ptr<CWebSocket> webSocket)
{
	// Pre-Flight Checks - BEGIN
	if (!getIsSyncEnabled())
		return false;
	// Pre-Flight Checks - END

	// Local Variables - BEGIN
	std::vector<uint8_t> blockBytes = netmsg->getData();
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	std::shared_ptr<CTools> tools = getTools();
	// Local Variables - END

	// Block Instantiation - BEGIN
	eBlockInstantiationResult::eBlockInstantiationResult result = eBlockInstantiationResult::Failure;
	std::string errorInfo;
	std::shared_ptr<CBlock> block = CBlock::instantiateBlock(true, blockBytes, result,
		errorInfo, bm->getMode());

	if (!block || !block->getHeader())
	{
		tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) +
			getIPAddress() + " provided invalid block data.",
			eLogEntryCategory::network, 5, eLogEntryType::warning, eColor::lightPink);
		issueWarning();
		return false;  // Trigger penalty for invalid block data
	}
	// Block Instantiation - END

	// Request Validation - BEGIN
	std::vector<uint8_t> blockID = block->getHeader()->getHash();
	if (!isRequestForBlockPending(blockID))
	{
		tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) +
			getIPAddress() + " attempted to deliver a not-requested block.",
			eLogEntryCategory::network, 5, eLogEntryType::warning, eColor::lightPink);
		return false;  // Trigger penalty for unrequested block
	}
	// Request Validation - END

	// Block Processing - BEGIN
	if (!bm->pushBlock(block))
	{
		tools->logEvent(tools->getColoredString("[DSM Sync]: ", eColor::lightCyan) +
			getIPAddress() + tools->getColoredString(" delivered block ", eColor::lightGreen) +
			tools->base58Encode(blockID),
			eLogEntryCategory::network, 5, eLogEntryType::warning, eColor::lightPink);
		notifyOperationStatus(eOperationStatus::failure, eOperationScope::peer, blockID);
	}
	else
	{
		bm->removeBlockFromHuntedList(blockID);
		markBlockRequestAsCompleted(blockID);
		notifyOperationStatus(eOperationStatus::success, eOperationScope::peer, blockID);
	}
	// Block Processing - END

	return true;
}

// Network Block Processing Methods - END
bool CConversation::handleProcessQRIntentResponse(std::shared_ptr <CQRIntentResponse> response, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	//mobile client has provided a QRIntentResponse
    //now find the proper terminal which has been waiting for a response
	if (response == nullptr || socket==0)
		return false;
	std::vector<uint8_t> receiptID= getNetworkManager()->getDTIServer()->registerQRIntentResponse(response);
	std::shared_ptr<CNetTask>task = std::make_shared<CNetTask>(eNetTaskType::notifyReceiptID);
	task->setData(receiptID);
	addTask(task);
	return true;
}
bool CConversation::handleProcessKademliaMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{
	//CEndPoint  ep (getAbstractEndpoint())
	//getNetworkManager()->getKademliaServer()->processKademliaDatagram(netmsg->getData(),)
	//if (resp != nullptr)
	//{
	//	return handleProcessQRIntentResponse(resp, socket);
	//}
	return false;
}


bool CConversation::handleProcessQRIntentResponseMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	std::shared_ptr<CQRIntentResponse> resp = CQRIntentResponse::instantiate(netmsg->getData());

	if (resp != nullptr)
	{
		return handleProcessQRIntentResponse(resp,socket);
	}
	return false;
}

bool CConversation::processTransaction( CTransaction& transaction, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}
bool CConversation::notifyTXReceptionResult(bool success, uint64_t reqID, std::string receiptID)
{

		std::shared_ptr<CDFSMsg>  dfsMsg = std::make_shared<CDFSMsg>(eDFSCmdType::commitSuccess);
		dfsMsg->setRequestID(reqID);
		//dfsMsg->setData1(r.getBERMetaData());
		if (receiptID.size() > 0)
			dfsMsg->setData2(getTools()->stringToBytes(receiptID));

		std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::DFS, eNetReqType::eNetReqType::notify);
		netMsg->setData(dfsMsg->getPackedData());
		std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);
		task->setData(netMsg->getPackedData());
		addTask(task);
		return true;

	return false;
}

bool CConversation::handleProcessTransactionMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	std::shared_ptr<CTools> tools = getTools();
	std::vector<uint8_t> data = netmsg->getData();
	std::shared_ptr<CNetworkManager> nm = getNetworkManager();
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::vector<uint8_t> hash = cf->getSHA2_256Vec(data);
	std::string IP = getIPAddress();
	bool autoBan = true;

	if (!bm->getIsReady() || CSettings::getIsGlobalAutoConfigInProgress())
	{
		tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) + " neglecting TX received from " + IP +", node is still booting up..", eLogEntryCategory::network, 1, eLogEntryType::notification);
		return true;
	}
	
	//support of the already seen objects awareness- BEGIN
	if (nm->getWasDataSeen(hash,IP, autoBan))
	{
	
		tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) + " redundant object received from "+ IP, eLogEntryCategory::network, 1, eLogEntryType::notification);

		if (autoBan)
		{//if still true that means that the IP was banned and we should terminate ASAP
			std::shared_ptr< CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
			addTask(newTask);
		}
		return false;
	}
	nm->sawData(hash);

	//support of the already seen objects awareness - END

	CTransaction* t = CTransaction::instantiate(data, getBlockchainManager()->getMode());
	
	if (!t)
		return false;

	std::vector<uint8_t> receiptID;
	if (t == nullptr)
	{
		notifyTXReceptionResult(false,netmsg->getSourceSeq());
	}
	CTransaction tx = *t;
	delete t;
	if (!tools->validateTransactionSemantics(tx) || !tx.isValid())
	{
		notifyTXReceptionResult(false, netmsg->getSourceSeq());
	}
	//here we get the Formation Flow manager since it's responsible for blocks' formation.
	if (!getBlockchainManager()->getFormationFlowManager()->registerTransaction(tx, receiptID))
	{
		notifyTXReceptionResult(false, netmsg->getSourceSeq());
	}
	notifyOperationStatus(eOperationStatus::success, eOperationScope::peer, receiptID);
	notifyTXReceptionResult(true,  netmsg->getSourceSeq(), tools->base58CheckEncode(receiptID));
	return true;
}

bool CConversation::handleNotifyTransactionMsg(std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket)
{

	//Local Variables - BEGIN
	std::vector<uint8_t> data = netmsg->getData();
	std::shared_ptr<CNetworkManager> nm = getNetworkManager();
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::vector<uint8_t> hash = cf->getSHA2_256Vec(data);
	std::shared_ptr<CTools> tools = getTools();
	std::string IP = getIPAddress();
	std::vector<uint8_t> receiptID;
	bool autoBan = true;
	//Local Variables -  END
	
	//support of the already seen objects awareness- BEGIN
	if (nm->getWasDataSeen(hash, IP, autoBan))
	{

		tools->logEvent(tools->getColoredString("[DSM-sync]:", eColor::lightCyan) + " redundant object received from " + IP, eLogEntryCategory::network, 1, eLogEntryType::notification);

		if (autoBan)
		{//if still true that means that the IP was banned and we should terminate ASAP
			std::shared_ptr< CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 10);//assign highest priority
			addTask(newTask);
		}
		return false;
	}
	nm->sawData(hash);

	//support of the already seen objects awareness - END

	tools->logEvent("Handling a TX notification..", eLogEntryCategory::network, 1, eLogEntryType::notification,eColor::orange);
	CTransaction* t = CTransaction::instantiate(data, getBlockchainManager()->getMode());
	if (!t)
		return false;

	
	if (t == nullptr)
	{
		notifyTXReceptionResult(false, netmsg->getSourceSeq());
	}
	CTransaction tx = *t;
	delete t;
	if (!tools->validateTransactionSemantics(tx) || !tx.isValid())
	{
		tools->logEvent("Invalid TX received..", eLogEntryCategory::network, 1, eLogEntryType::warning, eColor::lightPink);
		notifyTXReceptionResult(false, netmsg->getSourceSeq());
		issueWarning();
	}

	//here we get the Formation Flow manager since it's responsible for blocks' formation.
	if (!getBlockchainManager()->getFormationFlowManager()->registerTransaction(tx, receiptID))
	{
		tools->logEvent("Failed to register the received TX..", eLogEntryCategory::network, 1, eLogEntryType::failure, eColor::lightPink);
		notifyTXReceptionResult(false, netmsg->getSourceSeq());
	}
	notifyOperationStatus(eOperationStatus::success, eOperationScope::peer, receiptID);
	notifyTXReceptionResult(true, netmsg->getSourceSeq(), tools->base58CheckEncode(receiptID));
	return true;
}

bool CConversation::processVerifiable(CVerifiable verifiable, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}

std::shared_ptr<SE::CScriptEngine> CConversation::getTargetVMByID(std::vector<uint8_t> id)
{
	//Local Variables - BEGIN
	//bool isTargettingSystemThread = false;
	std::shared_ptr<CTools> tools = getTools();
	//Local Variables - END

	//Operational Logic - BEGIN

	//if (tools->compareByteVectors(id, tools->stringToBytes("system")))
	//{
	//	isTargettingSystemThread = true;
	//}

	std::shared_ptr<SE::CScriptEngine> se = getSystemThread();
	if (se == nullptr)
		return nullptr;//main VM required

	//if (isTargettingSystemThread)
	//	return se;

	if (id.size() == 0)
		return se;//if ID not provided return main VM

	if (tools->compareByteVectors(id, se->getID()))
	{
		return se;
	}

	return se->getSubThreadByVMID(id);//manage to find the processing thread
	//Operational Logic - END
}

/// <summary>
/// The function takes care of VMMetaDataProcessing. 
/// Note: the function does all the processing with a detached, newly spawned VM.
/// If, MetaData is targeting a VM spawned for a remote DTI session then it should be routed
/// to it using CDataRouter.
/// </summary>
/// <param name="msg"></param>
/// <returns></returns>
bool CConversation::handleProcessVMMetaDataMsg(std::shared_ptr<CNetMsg> msg)
{
	//prepareVM(true, nullptr, msg->, msg->getThreadID());// att ALL cases we DO NEED a vm right now. even for empty msg.
	if (msg->getData().size() == 0)
	{
		
		return true;
	}
	std::shared_ptr<CTools> tools = getTools();
	//Local vars - BEGIN
	CReceipt r;
	std::vector<std::string> log = r.getLog();
	std::string finalMessage = log.size() > 0 ? log[0] : "";
	std::shared_ptr<CDTI> dti;
	CVMMetaParser parser;
	bool res = true;
	std::vector<std::shared_ptr<CVMMetaSection>>  sections = parser.decode(msg->getData());
	std::vector<std::shared_ptr<CNetTask>> affectedTasks;
	uint64_t inRespToReqID = 0;
	std::vector<std::vector<uint8_t>>  dataFields;
	std::shared_ptr<CNetTask> task;
	std::vector<std::shared_ptr<CVMMetaEntry>> entries;
	uint64_t topMostValue, currentKeyHeight = 0;
	BigInt ERGUsed = 0;
	std::shared_ptr<SE::CScriptEngine> se = getSystemThread();
	eTerminalDataType::eTerminalDataType terminalDType = eTerminalDataType::input;
	std::shared_ptr<CDTI> targetDTI;
	std::shared_ptr<SE::CScriptEngine> targetVM;
	eThreadOperationType::eThreadOperationType toType = eThreadOperationType::newThread;
	std::string code;
	bool targettingGUITerminal = false;
	eVMMetaCodeExecutionMode::eVMMetaCodeExecutionMode execMode = eVMMetaCodeExecutionMode::RAW;
	//Local vars - END

	if (sections.size() == 0)
		return false;

	//Thread Commit Abort Check - Begin
			//That covers the System-Thread only.
	if (msg->getEntityType() == eNetEntType::DFS || msg->getEntityType() == eNetEntType::VMMetaData)
	{
		std::shared_ptr<SE::CScriptEngine> se = getSystemThread();

		if (sections.size()>0 && sections[0]->getEntries().size()>0 && se != nullptr && se->getIsWaitingForVMMetaData() && tools->compareByteVectors(sections[0]->getEntries()[0]->getVMID(), se->getID()))//ONLY protects the System Thread
		{//give a grace period of 20 seconds before the Global Commit operation can be aborted/disturbed

			size_t now = tools->getTime();
			if ((now - se->getQRWaitStartTime()) >= 20)
			{
				se->setIsWaitingForVMMetaData(false);
				enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(), 0, "Commit Aborted");//todo: keep track of taskID by VM
			}
		}
	}// otherwise, the received message is VM-agnostic, we might enque it and do the processing right away (in accordance to its priority).
	//Thread Commit Abort Check - End

	std::vector<uint8_t> targetVMID;
	//Notify tasks about arrived responses to Data-Queries - BEGIN
	for (uint64_t i = 0; i < sections.size(); i++)
	{
		// Requests - BEGIN
		if (sections[i]->getType() == eVMMetaSectionType::requests)
		{
			entries = sections[i]->getEntries();
			for (uint64_t a = 0; a < entries.size(); a++)
			{
				//Reset fields - BEGIN
				targetVM = nullptr;
				dataFields = entries[a]->getFields();
				targetDTI = nullptr;
				targetVMID = entries[a]->getVMID();
				//Reset fields - END

				//Prepare the VM/Thread - BEGIN

				if (!se || (!(dataFields.size() > 0 && entries[a]->getType() == eVMMetaEntryType::threadOperation &&
					static_cast<eThreadOperationType::eThreadOperationType>(tools->bytesToUint64(dataFields[0]) == eThreadOperationType::freeThread))))
				{//do not attempt to instantiate the thread if it's about to be shut down anyway.
					if (!prepareVM(true, nullptr, entries[a]->getReqID(), targetVMID, entries[a]->getProcessID()))
						continue;
				}

				se = getSystemThread();
				if (!se)
					return false;
				//Prepare the VM/Thread - END
				bool detachedProcessing = false;

				switch (entries[a]->getType())
				{
				case eVMMetaEntryType::threadOperation:
					if (dataFields.size() == 0)
					{
						return false;
					}

					toType = static_cast<eThreadOperationType::eThreadOperationType>(tools->bytesToUint64(dataFields[0]));

					switch (toType)
					{
					case eThreadOperationType::newThread:
						notifyOperationStatus(eOperationStatus::success, eOperationScope::VM, entries[a]->getVMID(), entries[a]->getReqID());//we're here so the operation must have already succeeded.
						//nothing to do; the thread had been just spawned implicitly if resources allowed with an ad-hoc notification delivered in any case.
						break;
					case eThreadOperationType::freeThread:
						if (mScriptEngine->freeThreadExC(entries[a]->getVMID(), entries[a]->getReqID(), true))
						{
							notifyOperationStatus(eOperationStatus::success, eOperationScope::VM, entries[a]->getVMID(), entries[a]->getReqID());
						}
						else
						{//most likely the thread did not exist
							notifyOperationStatus(eOperationStatus::failure, eOperationScope::VM, entries[a]->getVMID(), entries[a]->getReqID());
						}

						break;
					default:
						break;
					}

					break;


				case eVMMetaEntryType::GridScriptCode:

					if (dataFields.size() < 3)
						return false;

					execMode = static_cast<eVMMetaCodeExecutionMode::eVMMetaCodeExecutionMode>(tools->bytesToUint64(dataFields[0]));
					//processID at [1]
					code = tools->bytesToString(dataFields[2]);

					/*
					* ------- Decentralized Processing Threads - Main rationale ------:
					*
					* The overall rationale is to improve process' isolation, improve multi-tasking support and to allow for data retrieval even during pending data-commits,
					invoked by other applications. Even during locks imposed onto the system-thread.
					IMPORTANT:  the tryLockCommit() mechanics ensures synchronization through blocking any other app from executing DFS commands or
					initiating the commit-procedure. It is way less costly than spawning of a separate Decentralized Processing Thread, which involes full-node's resources
					and is subject to limits imposed by the given full-node's Operator.
					*
					*
						+ single UI dApp may have multiple VMs/threads attached,-> we need to allow targeting to each of these
						+ for the sub-threads to be available, the main Conversation-related VM needs to be made available first.
						At the bottom of everything, all the sub-threads are spawned through it, and registered with it.

						The main VM (system-thread),- it MAY manage lifetime of its offsprings, also through user-invokable #GridScript commands.
						+ caller MIGHT NOT be aware of particular VMs and/or threads
						+ whenever no specifics are provided, the processing is to be taking place at the 'data' named-thread (read-only).

						+ [DFS datagrams] are thread aware, NOT process-aware. Process-awareness needs to be achieved through request IDs.
						Apps need to sign-up for incoming responses and invoke this.addVMMetaRequestID() with particular request-IDs,- so
						that responses are not omitted when encountered within the processing queue, once they arrive.

						+ The [VM-Meta-Data] entries are BOTH process-aware (through processIDs) and thread/VM-aware (through VM-IDs).
						That allows caller to target by any of these.

						Note: targeting by processID currently is supported for DTIs only.

						+ multiple applications may be accessing the same thread/VM - IF thread is not marked as PRIVATE through vmFlags.
						+ threads may be distributed across multiple nodes/full-nodes, that happens at the discretion of the CVMContext.
						+ new thread may be requested anytime by invoking the start-thread ('st' - in-line ID optional) #GridScript instruction on the main VM or by calling CVMContext.createThread()
						from the JavaScript context. In any case, the result is delivered asynchronously through VM-Meta-Data.
						+ the Decentralized Processing Thread/VM is attempted to be spawned when first time queried for (either explicitly - through CVMContext.createThread()
						or implicitly, by issuing any VM-Meta-Data / DFS command targeting a specific named-thread).
						+ spawning of the thread MAY fail should limits specified by the Operator be exceeded. In such a case, an appropriate VM-Status-Notificaiton is delivered,
						the caller MAY expect.
						+ when ready-thread ('rt') instruction is invoked on a thread through #GridScript, the thread is marked as committable.
						+ particular atomic threads MAY be committed directly to the decentralized state-machine by invoking 'ct' in their scope.
						+ when commit-transaction ('ct') is invoked on the system-thread, the commitThread() sub-routine of the main-thread attempts to collect
						thread-codes from all the offspring-threads, the ones marked as committable, concatenate the source codes, compile them as one and commit to the decentralized state-machine.


					--------------------------------------------------------------------

					----- Decentralized Processing Threads---
					^-----------------Rules of Thumb---------------^
					+ thread manipulation mechanics are available both from #GridScript and through the VM Meta-Data protocol (eVMMetaEntryType::threadOperation). VM-Status meta-data are always generated if conversation is available
					from #GridScript. OperationStatus meta-data are generated only if thread manipulation invoked through VM-Meta-Data.
					+ the global 'data'-thread MAY be used EVEN during pending commits.
					+ the data thread MAY NOT become committable.
					+ thread MAY be  marked as committable during a pending commit, however its processing is not guaranteed.
					+ whenever no thread/VMid is specified,- the default,- main 'data' read-only processing thread is assumed.
					+ wherever threadID is specified but not available - the thread is spawned.
					+ code-targeting through thread-ID has higher priority than targeting through process-ID. The threadID is thus preferred.
					+ the global 'data'-thread IS to be expected to be shared across applications.
					+ the 'system' thread MAY NOT be accessed directly by user-mode UI dApps. Use CVMContext.proposeCode() instead. Further additional rules apply, see below.
					+ By default invocation of CVMContext.commmit() attempts to commit ALL commitable threads (including the system-thread).
					+ private threads should be accessed in eVMMetaCodeExecutionMode.RAW mode

					+ thread marked as committable can only be either committed or cleared. Consecutive instructions would NOT affect  the already formulated code
					(not guaranteed in case of a critical error).
					+ use CVMContext.freeThread() providing the thread's ID if thread is not to-be-needed anymore.
					+ IF, commitment of a user-defined thread is required call CVMContext.commmit() providing the particular thread's ID, OR execute 'ct' instruction directly
					on the thread.
					+ when parallel isolated/thread-safe data commitments are NOT needed; when willing to commit,- use tryLockCommit() instead of a new DPT.
					Do this:
					   - BEFORE proposing code through CVMContext.proposeCode(). The method would fail without code analysis if commitLock is active.
					   - BEFORE invoking CVMContext.commmit()
					   - WHENEVER *DURING* altering of your own 'committable' Decentralized Processing Thread which EVER was marked as committable.
					   Note: ALWAYS invoke freeCommitLock() whenever you're done with any of the above (manipulating your own commitable thread); otherwise
					   the system-thread would be locked. that is until a timeout occurs.

					Always use a separate DPT when you want to keep invoking instructions on the decentralized state-machine IF/WHEN:
					1) other commitment (single or multi-threaded) is already pending
					2) you want to formulate code/transaction that is entirely separate from the main VM or to be committed separately on your own discrection.
					Note: the thread will become (auto)-commitable, in bulk, along with the other threads,- once marked as ready through the 'ready-thread' instruction.
					You may commit code manually ommiting the 'ready-thread' instruction and proceeding directly with 'commit-thread' instead.
					3) you want your code executed on multiple / different nodes on the network or resources on the current node are busy

					---IMPORTANT NODES---
					Note 1: By default, the GRIDNET OS Web-Context spawns TWO threads:
						- the 'system' named-thread, which is supposed to be accessed ONLY in the eVMMetaCodeExecutionMode.UI processing mode
						and is assumed as 'committable' at all times. Any other threads are children of this very thread.
							Purpose: any UI-related operations that *ARE* to affect the decentralized state-machine and are to become part of the final transaction.
							Do *NOT*: invoke data-read operations on this thread IF results are not required by code running on the decentralized state-machine.
							otherwise, data-read operations ARE permitted.
						- the 'data' named-thread, which is to accommodate data-*READ* operations and *IS* to be shared across applications.
						Note: results of data-read operations will be delivered through the VM-Meta-Data protocol bearing unique requestID provided when invoking the operation.

					Note 2: User-mode applications are NOT allowed to operate directly on the System-Thread. They may however PROPOSE atomic instructions through a call to
					CVMContext.proposeCode(), which would return TRUE if the operation succeeded.
					-Above all-:
						+ do *NOT* propose code that would alter PREVIOUS contents of the stack-frame.
						+ do *NOT* perform data-read operations that are not required as input to code running on the decentralized state-machine.

					IMPORTANT: IF for any reason a call to CVMContext.proposeCode() should fail, the user is REQUIRED to spawn a new Decentralized Thread of his own
					through a call to CVMContext.createThread(), providing an appropriate threadID. The result of spawning of the new thread would be delivered through a
					VM-Meta-Data's VM-Status message. Once and if the thread is ready user may proceed over there.

					----------------------------------------------------

						The getTargetVMByID() utility function returns thread by its ID and main-thread if no ID provided.

					* Processing depending on selected modes:
					* 1) eVMMetaCodeExecutionMode::RAW - No code pre-processing is taking place. Command Executor not available.
					   Code executed either by the main VM - in case processID  AND threadID eual to 0, OR by the specialized UI-app' instance specific attached VM
					IF  vmID > 0. In the latter case, in this mode, the code is executed directly by the underlying VM (ommiting the Command Executor). This might be desirable
					* still note that due to presumed non-availability of Commands' Executor, some pre- and after- processing wouldn't be happening (like the in-shell reported current directory might become out-of sync).
					*
					  2) eVMMetaCodeExecutionMode::GUI - Code-pre processing taking place. VM is automatically put into transaction formulation mode if not already in it.
					  In this mode, by default, the code is processed by the main UI-VM attached to Conversation.
					  3) eVMMetaCodeExecutionMode::GUITerminal - supports both processIDs and thread IDs. In this mode, code is processed through the Command Executor, thus all of the shell pre- and after- processings
					  ARE effective. In this mode the code is supposed to target an specific instance of the VM attached to a specific UI-Terminal dApp.


					  //NOTE: as Discussed with the OldWizard on 18.08.21 we do not allow for routing to VMs per process IDs since a single process might be having access/ owning multiple threads
					  and select the firts or random one would be too risky. Better for developers to be more specific than to allow for implicit assumptions/facts inferring - in this case.
					  Thus a dictionary proposal by CodesInChaos [GIP 76.1] has been turned down as of now.
					  Only routig per process ID to DTis through VMMetaCodeExecutionMode::GUITerminal mode is allowed as these are assumed to handle only a single VM thread.

					*/

					//Preperations - BEGIN
					//The target VM/Decentralized Thread is selected and code processing (optional) takes place.
					//Note: the target VM is assumed to have been prepared by now.

					switch (execMode)
					{
					case eVMMetaCodeExecutionMode::RAWDetached:
						detachedProcessing = true;
					case eVMMetaCodeExecutionMode::RAW:
						//the VM should have been implicitly prepared already through a call to prepareVM above().
						//if due to resource limits etc. it failed we wouldn't be here.
						//nullptr checks to follow later on anyway
						targetVM = getTargetVMByID(entries[a]->getVMID()); //need to be explicit over here. If no ID provided, main VM assumed. 

						break;
					case eVMMetaCodeExecutionMode::GUI:
						//Pre-formatting ensuring the VM is in transaction-formulation state (if not already) and that code targets the proper REALM is assured.
						//In this mode, the operations are supposed to affect the decentralized state-machine through actions performed by user within the UI of GRIDNET OS.
						//All actions are ALWAYS assumed to be committable once performed.
						//additional checks and pre-processing MIGHT be taking here in the future.

						targetVM = getTargetVMByID(entries[a]->getVMID()); //need to be explicit over here. If no ID provided, main VM assumed. 

						if (!targetVM)
							break;//the VM should have been prepared already

						if (!targetVM->getTransactionBegan())
						{
							code = "sct test \n bt " + code;
						}

						break;

					case eVMMetaCodeExecutionMode::GUITerminal:
						targettingGUITerminal = true;
						//we're targeting a specific instance spawned for a specific UI-served DTI Terminal
						//all code is to be processed through the Commands' Executor.
						//Thus, the processID NEEDS to be specified. 

						if (entries[a]->getProcessID() == 0)
						{
							enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(), entries[a]->getReqID(), "No process ID specified.");
							return false;
						}

						targetDTI = findDTI(entries[a]->getProcessID());


						if (!targetDTI) {
							//for some reason the DTI is not available. Notify caller.
							enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(), entries[a]->getReqID(), "Unable to initialize the DTI.");
							return false;
						}

						break;
					default:
						break;
					}
					//Preperations - END

					if (targetDTI)
					{
						targetVM = targetDTI->getScriptEngine();//will be routed to DTI anyway (it's prioritized).
					}

					if (!targetVM)
					{
						tools->logEvent("An attempt to access a nulled VM from UI session.", eLogEntryCategory::VM, 1, eLogEntryType::failure);
						notifyOperationStatus(eOperationStatus::failure, eOperationScope::VM, std::vector<uint8_t>(), entries[a]->getReqID());
						return false;
					}

					//check if code already running
					if (targetVM->isRegSet(REG_EXECUTING_INSTRUCTION) || targetVM->isRegSet(REG_EXECUTING_PROGRAM))
					{
						//in order to maximize responsivness, we 1) abort any current code execution as it is assumed to be the responsibility of caller (VMContext) to watch what's being requested
						targetVM->requestAppAbort();
						//2) deliver a notifcation
						notifyOperationStatus(eOperationStatus::failure, eOperationScope::VM, targetVM->getID(), entries[a]->getReqID());
						return false;
					}

					//Code Processing - BEGIN

					if (targetDTI)
					{//execute command by the DTI - BEGIN
						if (!targetDTI->executeCode(code))
						{
							enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(), entries[a]->getReqID(), "Can't handle code at this moment.");
						}
						else return  true;
					}//execute command by the DTI - END

					else if (targetVM)
					{//execute command by the VM - BEGIN
						if (!detachedProcessing)
						{
							//Conversation-blocking processing
							//no other datagrams could be handled until the processing by the thread below is finished.

							r = targetVM->processScript(code, std::vector<uint8_t>(), topMostValue, ERGUsed, currentKeyHeight,
								false, false, std::vector<uint8_t>(), false, true, true, true, targettingGUITerminal,
								entries[a]->getReqID(),
								false // indicate non-detached processing
							);
						}
						else
						{
							//Non-Conversation Blocking processing - BEGIN

							//This will spawn a new native thread.
							//The thread would be alive only during processing of the just received code-package.

							//The Decentralized Thread would keep track of all of the attached native threads and keep the REG_HAS_DETACHED_THREAD
							//registry flag set until all the attached native threads terminate. This is performed autonomously.
							//processScript() knows it's executing from a dedicated thread thanks to an appropriate parameter being set.

							size_t nativeThreadsCount = targetVM->getNativeThreadCount();


							if (!nativeThreadsCount)
							{
								//only 1 native thread allowed per VM / Decentralized Thread.
								targetVM->addNativeThread(std::make_shared<std::thread>(std::bind(&SE::CScriptEngine::processScript, targetVM,
									code, std::vector<uint8_t>(), topMostValue, ERGUsed, currentKeyHeight, false, false, std::vector<uint8_t>(),
									false, true, true, true, targettingGUITerminal, entries[a]->getReqID(), false,
									true, // indicate detached processing
									false
								)));
							}
							else
							{
								// Usually additional incoming requests are not expected, Caller is expected to wait.
								// To assure responsiveness we shut-down any current processing.
								// Thus, we effectively assume the disruption being the caller's fault - not having awaited prior result.
								// Processing loop within any #GridScript command/app are supposed to react to these notifications and shut down immediately.
								targetVM->quitCurrentApp();
								targetVM->abortCurrentVMTask();
							}
							//Non-Conversation Blocking processing - END

						}
						//execute command by the VM - END
					}
					else
					{
						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(), entries[a]->getReqID(), "Can't handle code at this moment.");
						return false;
					}

					//Code Processing - END

					//following reciept analysis only for RAW VM-processing
					log = r.getLog();
					finalMessage = log.size() > 0 ? log[0] : "";
					//notify caller about the results  - BEGIN
					if (r.getResult() == eTransactionValidationResult::valid)
					{
						//[todo:vega4]: update web-ui to handle the additional ber-meta-data field
						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::success, targetVM ? targetVM->getRecentBERMetaData() : se->getRecentBERMetaData(), entries[a]->getReqID(), r.translateStatus() + (finalMessage.size() ? (": " + finalMessage) : ""));
					}
					else
					{
						
						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, targetVM ? targetVM->getRecentBERMetaData() : se->getRecentBERMetaData(), entries[a]->getReqID(), r.translateStatus() +(finalMessage.size() ? (": "+ finalMessage):""));
					}
					//notify caller about the results - END
					break;

				case eVMMetaEntryType::preCompiledTransaction:
				{
					// Handle pre-compiled, locally signed transactions from Web UI dApp
					// This enables trustless transaction generation where bytecode compilation
					// and signing happens in the browser without trusting the node

					// Validate data fields
					if (dataFields.size() < 1)
					{
						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(),
							entries[a]->getReqID(), "Missing transaction data");
						break;
					}

					// Extract BER-encoded transaction from first data field
					std::vector<uint8_t> txBytes = dataFields[0];

					// Get target blockchain
					std::shared_ptr<CBlockchainManager> targetBlockchain = getBlockchainManager();

					if (!targetBlockchain)
					{
						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(),
							entries[a]->getReqID(), "Blockchain manager not available");
						break;
					}

					// Deserialize transaction
					CTransaction* tx = CTransaction::instantiate(txBytes, targetBlockchain->getMode());

					if (tx == nullptr)
					{
						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(),
							entries[a]->getReqID(), "Invalid transaction format");
						break;
					}

					CTransaction t = *tx;
					delete tx;

					// Pre-validate transaction using TransactionManager
					// This performs comprehensive validation including ERG checks, signature verification, nonce, etc.
					uint64_t currentKeyHeight = targetBlockchain->getKeyHeight();
					std::shared_ptr<CTransaction> transPtr = std::make_shared<CTransaction>(t);
					std::shared_ptr<CTransactionDesc> txDesc = nullptr; // Can be null for basic validation

					eTransactionValidationResult validationResult = targetBlockchain->getFormationFlowManager()->preValidateTransaction(
						transPtr,
						currentKeyHeight,
						txDesc
					);

					// Check validation result - only proceed if 'valid'
					if (validationResult != eTransactionValidationResult::valid)
					{
						std::string errorMsg = "Transaction validation failed: ";
						switch (validationResult)
						{
						case eTransactionValidationResult::unknownIssuer:
							errorMsg += "Unknown issuer";
							break;
						case eTransactionValidationResult::insufficientERG:
							errorMsg += "Insufficient ERG";
							break;
						case eTransactionValidationResult::ERGBidTooLow:
							errorMsg += "ERG bid too low";
							break;
						case eTransactionValidationResult::invalidSig:
							errorMsg += "Invalid signature";
							break;
						case eTransactionValidationResult::invalidNonce:
							errorMsg += "Invalid nonce";
							break;
						case eTransactionValidationResult::pubNotMatch:
							errorMsg += "Public key mismatch";
							break;
						case eTransactionValidationResult::noIDToken:
							errorMsg += "No ID token";
							break;
						case eTransactionValidationResult::noPublicKey:
							errorMsg += "No public key";
							break;
						case eTransactionValidationResult::invalidBalance:
							errorMsg += "Invalid balance";
							break;
						case eTransactionValidationResult::invalidBytecode:
							errorMsg += "Invalid byte-code";
							break;
						case eTransactionValidationResult::received:
							errorMsg += "Received - awaiting validation";
							break;
						default:
							errorMsg += "Unknown error";
							break;
						}

						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(),
							entries[a]->getReqID(), errorMsg);
						break;
					}

					// Set issuing information for reporting
					t.setIssuingEndpoint(getAbstractEndpoint());
					t.setIssuingConversation(shared_from_this());

					// Register transaction and get receipt ID
					std::vector<uint8_t> receiptID;
					if (!targetBlockchain->getFormationFlowManager()->registerTransaction(t, receiptID))
					{
						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(),
							entries[a]->getReqID(), "Failed to register transaction");
						break;
					}

					// Log success
					mTools->logEvent("Pre-compiled transaction registered. Receipt: " +
						mTools->base58CheckEncode(receiptID), eLogEntryCategory::VM, 0, eLogEntryType::notification);

					// Publish transaction to network if not local-only mode
					
						if (targetBlockchain->getNetworkManager() != nullptr)
						{
							CDataPubResult pubResult = targetBlockchain->getNetworkManager()->publishTransaction(t);

							if (pubResult.getResult() != eDataPubResult::success)
							{
								mTools->logEvent("Warning: Transaction registered locally but network publish failed",
									eLogEntryCategory::VM, 0, eLogEntryType::warning);
							}
							else
							{
								mTools->logEvent("Transaction published to network successfully",
									eLogEntryCategory::VM, 0, eLogEntryType::notification);
							}
						}
					


					// Send success response with receipt ID
					// Receipt ID is sent back to Web UI dApp for transaction tracking
					std::vector<uint8_t> responseData;

					// Create response with receipt ID
					std::shared_ptr<CVMMetaGenerator> metaGen = std::make_shared<CVMMetaGenerator>();

					// Prepare data types and fields for addDataResponse
					std::vector<eDataType::eDataType> dataTypes;
					dataTypes.push_back(eDataType::bytes); // Receipt ID is bytes

					std::vector<std::vector<uint8_t>> responseFields;
					responseFields.push_back(receiptID);

					metaGen->addDataResponse(dataTypes, responseFields, entries[a]->getReqID(),
						entries[a]->getProcessID(), entries[a]->getVMID());

					responseData = metaGen->getData();
					enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::success, responseData, entries[a]->getReqID());

					break;
				}

				}
			}
		}// Requests - END

		// Notifications - BEGIN
		if (sections[i]->getType() == eVMMetaSectionType::notifications)//notification-section MIGHT contain detaResponse entries
		{
			 entries = sections[i]->getEntries();

			for (uint64_t a = 0; a < entries.size(); a++)
			{
				dataFields = entries[a]->getFields();

				switch (entries[a]->getType())
				{
				case eVMMetaEntryType::dataResponse:
					 inRespToReqID = 0;

					if ((inRespToReqID = entries[a]->getReqID()) > 0)
					{
						//provide task with data it was awaiting
						// dataFields = entries[a]->getFields();
						task = getTaskByMetaReqID(inRespToReqID);

						//check if a task with such a *META* RequestID actually exists=> if so proceed with filling awaited-data-parts.
						//the tasks needs to be in a 'working' state

						if (task != nullptr && task->getState()== eNetTaskState::working)
						{
							affectedTasks.push_back(task);
							if (task == nullptr)
								return false;

							for (uint64_t b = 0; b < dataFields.size(); b++)
							{
								task->addResultDataEntry(dataFields[b]);
							}
						}
						else if (task == nullptr)
						{
							// FALLBACK: No task found - this happens when detached native threads
							// (e.g., from DFS commit) are waiting for auth data.
							// Deliver data directly to the VM that's waiting, similar to how DTI handles it.
							// The VM's setVMMetaDataResponse() will match by metaRequestID and unblock
							// any waiting GridScript code (e.g., commitThreads() waiting for QRIntentAuth response).

							std::shared_ptr<SE::CScriptEngine> systemThread = getSystemThread();
							if (systemThread != nullptr)
							{
								// Get all VMs in this conversation (system, data, and user-defined threads)
								std::vector<std::shared_ptr<SE::CScriptEngine>> allThreads = systemThread->getThreads(false);

								// Iterate through all VMs to find the one waiting for this data
								bool delivered = false;
								for (size_t i = 0; i < allThreads.size() && !delivered; i++)
								{
									if (allThreads[i] != nullptr && allThreads[i]->getIsWaitingForVMMetaData())
									{
										// Found the waiting VM - deliver auth data directly
										allThreads[i]->setVMMetaDataResponse(sections, true, false);
										delivered = true;
									}
								}

								// If no waiting VM found among threads, check system thread itself
								if (!delivered && systemThread->getIsWaitingForVMMetaData())
								{
									systemThread->setVMMetaDataResponse(sections, true, false);
								}
							}
						}

					}
					break;
				
				case eVMMetaEntryType::terminalData:

					//[todo:PauliX:urgent]: it might happen that the DTI session is closed due to a timeout. The input/output handling threads are thus killed.
					/*=> the UI-based Terminal session becomes unresponsive, BUT, the spawned VM is still ALIVE.
					* Investigate possible aproaches to resumption of processing for current and new instances of UI Terminals.
					* + isn't the VM left in an invalid state?
					* + can it be re-used?
					* + [FOCUS] could we 'simply' restart/respawn the DTI on user-input? Could it happen right here?
					* [todo:PauliX,Mike:urgent]: research the feasibility of 'detached UI Terminals' as discussed with TheOldWizard on 12.07.21
					*     +'detach' command as discussed? ETA?
					*	  + seperate instances of VM per Terminal Window after 'detach' command exectuted?
					*     + each UI terminal attached to UI VM by default? (currently it is as such)
					*	  + make only first instance stick to the UI VM?
					*     + make the UI control attachment? [todo:Mike:urgent] could we introduce a fancy UI element for this?
					* 
					[todo:CodesInChaos:urgent] Please do assess costs in <15 hours.
					*/

					//Validation - BEGIN
					if (dataFields.size() < 2)//require just 2 fields in a 'typical' case, require 3 fields in case of window-dimensions
						return false;

					if (dataFields[0].size() == 0 || dataFields[1].size() == 0)
						return false;
					//Validation - END

					if (!prepareVM(true,nullptr,entries[a]->getReqID(),entries[a]->getVMID(),entries[a]->getProcessID()))//prepare the main VM
						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(), entries[a]->getReqID(), "Unable to initialize the VM.");

					dti = prepareDTI(entries[a]->getProcessID(), entries[a]->getVMID(), true);

					if(!dti)
						enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::failure, std::vector<uint8_t>(), entries[a]->getReqID(),"Unable to initialize the DTI.");
					
					//getSystemThread()->setRemoteTerminal(dti); //this has nothing to do with the System Thread
					dataFields = entries[a]->getFields();

					terminalDType = static_cast<eTerminalDataType::eTerminalDataType>(dataFields[0][0]);

					 switch (terminalDType)
					 {
					 case eTerminalDataType::input:
						 //received keyCode might comprise more than a single Byte
						 for (uint64_t i = 0; i < dataFields[1].size(); i++)//iterate over the data1 data-bank
						 {//there might have been multiple bytes present (an extended key-code).
							 dti->enqueInputChar(dataFields[1][i]);
						 }
						 break;
					 case eTerminalDataType::output:
						 return false;
						 break;
					 case eTerminalDataType::windowDimensions:
						 if (dataFields.size() < 3)
							 return false;//height is missing
						
							 //Do not return false when the below fails as that would trigger autonomous firewall ban-when threshold reached.
							  dti->setTerminalWidth(tools->bytesToUint64(dataFields[2]));
							  dti->setTerminalHeight(tools->bytesToUint64(dataFields[1]));
						
						 break;
					 default:
						 return false;
						 break;
					 }
					break;

				
				default:
					break;
				}

			}
		}
		// Notifications - END
	}
	//Notify tasks about arrived responses to Data-Queries - END

	// Update state of tasks awaiting data - BEGIN
	for (uint64_t i = 0; i < affectedTasks.size(); i++)
	{
		if (affectedTasks[i]->getType() == eNetTaskType::requestData && affectedTasks[i]->getRequiredMetaFieldsToReceive() <= affectedTasks[i]->getResultDataEntries().size())
		{
			affectedTasks[i]->setState(eNetTaskState::completed);
		}
	}
	// Update state of tasks awaiting data - END
	
	return true;//TODO: consider processing VMMeta-Requests processing on full-node (full node replies to requsts is it viable?)
}
/// <summary>
/// Notifies about the status of a given (possibly implicit if reqID not specified) operation.
/// To be used scarcely, not to impose overheads, at best only when recipient does await it.
/// </summary>
/// <param name="status"></param>
/// <param name="scope"></param>
/// <param name="ID"></param>
/// <param name="reqID"></param>
/// <returns></returns>
bool  CConversation::notifyOperationStatus(eOperationStatus::eOperationStatus status, eOperationScope::eOperationScope scope,
	std::vector<uint8_t> ID, uint64_t reqID, std::string notes)
{
	std::shared_ptr<CNetMsg>  msg = std::make_shared<CNetMsg>(eNetEntType::OperationStatus, eNetReqType::notify);
	std::shared_ptr<COperationStatus> st = prepareOperationStatus(status, scope, ID, reqID);
	std::shared_ptr<CTools> tools = getTools();
	st->setNotes(notes);

	if (getLogDebugData())
	{
		tools->logEvent("Notifying "+tools->endpointTypeToString(getAbstractEndpoint()->getType())+ " peer about Operation Status: "+ st->getDescription(), eLogEntryCategory::network, 0);
	}

	msg->setSource(getBlockchainManager()->getCurrentID());
	msg->setSourceType(eEndpointType::PeerID);

	msg->setData(st->getPackedData());

	return sendNetMsg(msg, true);
}
std::shared_ptr<CDTI> CConversation::findDTI(uint64_t appID)
{
	std::lock_guard<std::mutex> lock(mDTISetGuardian);
	
	for (uint64_t i = 0; i < mDTIs.size(); i++)
	{
		if (mDTIs[i]->getUIProcessID() == appID)
			return mDTIs[i];
	}
	return nullptr;
}
void CConversation::addDTI(std::shared_ptr<CDTI> dti)
{
	std::lock_guard<std::mutex> lock(mDTISetGuardian);
	mDTIs.push_back(dti);
}
void CConversation::cleanUpDTIs(bool forceShutdown)
{
	std::lock_guard<std::mutex> lock(mDTISetGuardian);

	std::vector<std::shared_ptr<CDTI>>::iterator it = mDTIs.begin();

	while (it != mDTIs.end())
	{
		if(forceShutdown)
			(*it)->requestStatusChange(eDTISessionStatus::ended);

		if ((*it)->getStatus() == eDTISessionStatus::ended)
			it = mDTIs.erase(it);
		else
			++it;
	}

}
size_t CConversation::getLastTimeCodeProcessed()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastTimeCodeProcessed;
}
void CConversation::setLastTimeCodeProcessed(size_t time)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastTimeCodeProcessed = time;
}

std::shared_ptr<CConversation> CConversation::getConvForWebSocket(std::weak_ptr<CWebSocket> webSocket, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm, std::shared_ptr<CEndPoint> endpoint, std::shared_ptr<CNetTask> task)
{
	std::shared_ptr< CConversation> conv = std::make_shared<CConversation>(webSocket, bm, nm, endpoint, task);
	conv->initState(conv);

	convHolder* holder = new convHolder();
	reinterpret_cast<convHolder*>(holder)->conv = conv;
	if (std::shared_ptr<CWebSocket> socket = webSocket.lock())
	{

		socket->setOnEventCallback([](//TRAMPOLINE
			struct mg_connection*, int ev,
			void* ev_data, void* fn_data) {//[todo:CodesInChaos:high] fn_data might become null after the smart-pointer pointing to the conversation gets deltec
				//Solution: fn_data points to a data-structure (convHolder) containing a weak-ptr to the Conversation.
				//convHolder is allocate on heap and needs to be de-allocated when the web-socket connection is released.

				if (!fn_data)
				{
					return;
				}

				std::shared_ptr<CConversation> conv = reinterpret_cast<convHolder*>(fn_data)->conv.lock();
				//note: the above might return nullptr since the Conversation might have ended by now.
				if (!conv)
				{
					return;
				}
				std::shared_ptr<CTools> tools = conv->getTools();

				bool logDataEnabled = conv->getLogDebugData();

				if (ev == MG_EV_CLOSE)
				{
					if (conv->getLogDebugData())
						tools->logEvent("[WebSockets]: Close request from a web-socket connected to " + CTools::getInstance()->bytesToString(conv->getEndpoint()->getAddress()), eLogEntryCategory::network, 0);
					conv->end(false, false, eConvEndReason::otherEndTerminatedAbruptly);//do not notify peer, socket is already closed.
					if (fn_data)
					{
						delete fn_data;
					}
				}
				else if (ev == MG_EV_WS_MSG) {
					// Got web-socket frame. Received data is wm->data. Echo it back!
					struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;


					std::shared_ptr<CNetMsg> nMsg = CNetMsg::instantiate(CTools::getInstance()->ConvertArrayToVector(const_cast<void*>(reinterpret_cast<const void*>(wm->data.ptr)), wm->data.len));


					if (logDataEnabled)
						tools->logEvent("[WebSockets]: Data received through a web-socket.", eLogEntryCategory::network, 0);

					if (nMsg == nullptr)
					{
						conv->incInvalidMsgsCount();
					
						if (logDataEnabled)
							tools->logEvent("[WebSockets]: Invalid data received,", eLogEntryCategory::network, 0);

					}
					else
					{
						conv->enqueueNetMsg(nMsg);
					}

				}


			}, holder);
	}
	else
	{
		return nullptr;
	}
	return conv;
}

uint64_t CConversation::getInvalidMsgsCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mInvalidMsgsCount;
}

void CConversation::incInvalidMsgsCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mInvalidMsgsCount++;
}

uint64_t CConversation::getValidMsgsCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mValidMsgsCount;
}

void CConversation::incValidMsgsCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mValidMsgsCount++;
}

size_t CConversation::getSecWindowSinceTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mSecWindowSinceTimestamp;
}
std::shared_ptr<CWebSocket> CConversation::getWebSocket()
{
	std::lock_guard<std::mutex> lock(mWebSocketGuardian);
	return mWebSocket;
}
void CConversation::setWebSocket(std::shared_ptr<CWebSocket> socket)
{	
	std::lock_guard<std::mutex> lock(mWebSocketGuardian);
	mWebSocket = socket;
}


std::shared_ptr<CTools> CConversation::getTools()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mTools;
}
std::shared_ptr<CBlockchainManager> CConversation::getBlockchainManager()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mBlockchainManager;
}
std::shared_ptr<CNetworkManager> CConversation::getNetworkManager()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mNetworkManager;
}
bool CConversation::incMisbehaviorCounter()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mMisbehaviorCounter++;
	if (mMisbehaviorCounter > CGlobalSecSettings::getMaxConvMisbehaviorsCount())
		return true;
	return false;
}
uint64_t CConversation::getMisbehaviorsCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mMisbehaviorCounter;
}
bool CConversation::handleProcessVerifiableMsg( std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket )
{
	return false;
}