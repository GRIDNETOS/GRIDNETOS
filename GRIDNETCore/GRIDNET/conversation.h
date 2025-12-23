#pragma once
#include "stdafx.h"
#include "ScriptEngine.h"
#include <queue>
#include "NetTask.h"
#include "msquic.h"
#include <future>
///#include <ixwebsocket/IXWebSocket.h>
#define DEFFERRED_TIMEOUT_DURATION 60*5
#include "CDFSMsg.h"
typedef int UDTSOCKET;
class CTools;
namespace eNetMsgResult
{
	enum eNetMsgResult
	{
		valid,
		invalid

	};
}
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

class CCertificate;
class CSDPEntity;
class CTools;
class CBlockchainManager;
class ThreadPool;
class CNetworkManager;
class CBlock;
class CBlockHeader;
class CVerifiable;
class CConversationState;
class CNetMsg;
class CEndPoint;
class CDTI;
class CQRIntentResponse;
class CTransactionManager;
class CVMMetaGenerator;
class CVMMetaSection;
class CSessionDescription;
class CCryptoFactory;
class COperationStatus;
class CSettings;
class CWebSocket;
class CQUICConversationsServer;
struct conversationFlags
{
	bool exchangeBlocks : 1;// whether synchronization if EFFECTIVELY  operational.
	bool isMobileApp : 1;
	bool isIncoming : 1;
	bool QUICConnectionClosed : 1;
	bool QUICConnectionShutDown : 1;
	bool synchronisationToBeActive : 1; // where synchonization is the TARGET activity. (it would be enabled only once encryption becomes operational and target is not a mobile device).
	bool localIsMobile : 1;
	bool reserved6 : 1;

	conversationFlags(const conversationFlags& sibling) {
		std::memcpy(this, &sibling, sizeof(conversationFlags));
	}

	conversationFlags()
	{
		std::memset(this, 0, sizeof(conversationFlags));
	}
};
struct ConversationContext {
	std::shared_ptr<CConversation> conversation;
};

class CConversation: public std::enable_shared_from_this<CConversation>
{
public: 
	bool isSlowDataAttack();
private:

	uint64_t mTotalBlockNotifications;         // Total number of received block notifications
	uint64_t mLastNotificationTime;            // Timestamp of the last notification (in seconds)
	double mAverageNotificationInterval;       // Average block notification interval (in seconds)

	// QUIC-specific member fields - BEGIN

	// Map to hold buffers for each stream
	std::map<HQUIC, std::vector<uint8_t>> mStreamRXBuffers;
	HQUIC mConfiguration;  // QUIC configuration object
	HQUIC mQUICRegistration;
	HQUIC mQUICConnectionHandle = 0;  // QUIC connection handle
	uint64_t mTXBufferSize = 0;
	uint64_t mRXBufferSize = 0;
	std::unordered_map<uint64_t, HQUIC> mActiveStreams; // Map of active QUIC streams
	uint64_t mNextStreamId;   // Next stream identifier
	std::mutex mActiveStreamsGuardian;  // Mutex for stream map access
	// QUIC-specific member fields - END
	uint64_t getNextStreamID();
	// QUIC-specific methods - BEGIN

	// Method to handle stream data
	void HandleStreamData(HQUIC stream, const uint8_t* data, uint32_t length);
	
	// Method to process and remove buffer when stream is closed
	void ProcessAndRemoveStreamBuffer(HQUIC stream);

	void removeStreamRXBuffer(HQUIC stream);

	HQUIC createNewQUICStream();
	QUIC_STATUS StreamCallback(HQUIC Stream, QUIC_STREAM_EVENT* Event);
	bool shutdownQUICStream(HQUIC stream, bool abort = false, bool immediate = false);
	bool sendQUICData(HQUIC stream, const std::vector<uint8_t>& data, bool finalize=false);
	HQUIC getRegistration();
	void setRegistration(HQUIC registration);
	// QUIC-specific methods - END

	bool mIsScheduledToStart;
	uint64_t mCustomBarID = 0;
	//uint64_t mRedundantObjectsReceived; kept track by Network Manager (per IP address)
	conversationFlags mFlags;//used to specify the main purpose, type of peer (mobile or not) etc.
	bool sendNetMsgRAW(bool doItNOW, eNetEntType::eNetEntType eNetType, eNetReqType::eNetReqType eNetReqType, std::vector<uint8_t> RAWbytes = std::vector<uint8_t>(), bool pingStatus = true);
	bool mIsThreadAlive;
	bool mIsPingThreadAlive;
	bool mCeaseCommunication;
	uint64_t mQUICStreamCount = 0;
	uint64_t mOrderedQUICStreamCount = 0;
	uint64_t mAllowedStreamCount;
	uint64_t mQUICPeerAcceptedStreamCount;

public: 

	// Getter for total block notifications
	uint64_t getTotalBlockNotifications();

	// Getter for average block notification interval
	double getAverageNotificationInterval();

	// Method to update block notification statistics
	void updateBlockNotificationStats();

	uint64_t getQUICPeerAcceptedStreamCount();
	void  incQUICPeerAcceptedStreamCount();
	const QUIC_API_TABLE* getQUIC() ;
	std::recursive_mutex mDestructionGuardian;
	uint64_t getLastTimePingReceived();
	void pingReceived();
	void pingSent();
	uint64_t getLastTimePingSent();
	conversationFlags getFlags();
	void setFlags(conversationFlags& flags);
	std::shared_ptr<CNetTask>  getTaskByID(uint64_t id);
	void cleanTasks();
	std::shared_ptr<CNetTask> getTaskByMetaReqID(uint64_t id);
	std::future<void>& getThread();
	bool isThreadValid();
	bool isPingThreadValid();
	bool startUDTConversation(std::shared_ptr<ThreadPool> pool, UDTSOCKET client, std::string IPAddress = "");

	bool startQUICConversation(std::shared_ptr<ThreadPool> pool, HQUIC connHandle, std::string IPAddress);

	bool startWebSockConversation(std::shared_ptr<ThreadPool> pool, std::weak_ptr<CWebSocket> socket, std::string URL = "");
	~CConversation();
	bool addTask(std::shared_ptr<CNetTask> task);
	std::shared_ptr<CNetTask> getCurrentTask(bool deqeue = false);
	std::shared_ptr<CEndPoint> getEndpoint();
	void setProtocol(eTransportType::eTransportType pType);
	eTransportType::eTransportType getProtocol();
	std::vector<uint8_t> getID();
	eNetTaskProcessingResult::eNetTaskProcessingResult processTask(std::shared_ptr<CNetTask> task, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr,bool doBlockingProcessing=false);
	

	std::vector<uint8_t> getPubKey();
	Botan::secure_vector<uint8_t> getPrivKey();
	bool getDoSigHelloAuth();
	bool getUseAEADForAuth();
	void setUseAEADForAuth(bool doIt = true);
	bool getUseAEADForSessionKey();
	void setUseAEADForSessionKey(bool doIt = true);

	void clearAuthAttempts();

	uint64_t getAuthAttempts();
	void incAuthAttempts();
	void setDoSigHelloAuth(bool doIt = true);
	std::shared_ptr<CEndPoint> getAbstractEndpoint();//constructed on initialization only
bool notifyOperationStatus(eOperationStatus::eOperationStatus status, eOperationScope::eOperationScope scope = eOperationScope::dataTransit,
		std::vector<uint8_t> ID = std::vector<uint8_t>(), uint64_t reqID = 0, std::string notes="");
	std::shared_ptr<CDTI> findDTI(uint64_t appID);
	void addDTI(std::shared_ptr<CDTI> dti);
	void cleanUpDTIs(bool forceShutdown = false);
	bool enqeueVMMetaTerminalDataMsg(std::vector<uint8_t> message, eTerminalDataType::eTerminalDataType dType = eTerminalDataType::output, uint64_t reqID = 0, uint64_t appID=0);
	bool enqeueVMMetaUIAppDataMsg(std::vector<uint8_t> message,uint64_t reqID = 0, uint64_t appID=0,std::vector<uint8_t> vmID = std::vector<uint8_t>());
	
	size_t getLastTimeCodeProcessed();
	void setLastTimeCodeProcessed(size_t time = 0);
	static std::shared_ptr<CConversation> getConvForWebSocket(std::weak_ptr<CWebSocket> webSocket, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm, std::shared_ptr <CEndPoint> endpoint, std::shared_ptr<CNetTask> task);
	uint64_t getInvalidMsgsCount();
	void incInvalidMsgsCount();
	uint64_t getValidMsgsCount();
	void incValidMsgsCount();
	std::shared_ptr<CDTI> prepareDTI(uint64_t processID=0, std::vector<uint8_t> threadID = std::vector<uint8_t>(), bool isUIAware=false);

	std::string getIPAddress();
	bool enqeueThreadOperationStatus(eThreadOperationType::eThreadOperationType oType, std::vector<uint8_t> threadID, uint64_t reqID=0);
	void enqueBlockBodyRequest(std::vector<uint8_t>& blockID, uint64_t reqID);
	bool isRequestForBlockPending(std::vector<uint8_t>& blockID);
	bool markBlockRequestAsCompleted(std::vector<uint8_t>& blockID);
	bool markBlockRequestAsCompleted(uint64_t requestID);
	bool markBlockRequestAsAborted(uint64_t requestID);
	size_t getPendingBlockBodyRequestsCount();
	bool requestBlock(std::vector<uint8_t> blockID, UDTSOCKET socket=0, std::shared_ptr<CWebSocket> webSocket = nullptr, bool doItNow = false);
	bool getIsEncryptionAvailable(bool requrieSessionBasedEncryption=false);
	void setSyncToBeActive(bool isIt = true);
	bool getSyncToBeActive();
	uint64_t getTransmissionErrorsCount();
	void incTransmissionErrorsCount();
	bool isSocketFreed();
	void setLeadBlockInfo( const std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>> &info);
	std::vector<uint8_t> getLeadBlockID();
	std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>> getLeadBlockInfo();
	eBlockAvailabilityConfidence::eBlockAvailabilityConfidence hasBlockAvailable(const std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>> blockInfo);

	void setEndReason(eConvEndReason::eConvEndReason reason);
	eConvEndReason::eConvEndReason getEndReason();
	uint64_t getCustomBarID();
	void clearLocalChainProof();
	std::vector <std::vector<uint8_t>> getLocalChainProof();
	void setLocalChainProof(const std::vector <std::vector<uint8_t>>& proof);
	bool requestDeferredTimeout(bool requestedByUs=true);
	uint64_t getDeferredTimeoutRequestedAt();
	uint64_t getUDTTimeout();
	bool getPreventStartup();
	void preventStartup();
	std::recursive_mutex mThreadAliveGuardian;
	std::recursive_mutex mPingThreadAliveGuardian;
	void initState(std::shared_ptr<CConversation> conversation=nullptr);
	uint64_t getDuration();
	static std::string checkUDTError(int optRes);
	std::shared_ptr<CNetworkManager> getNetworkManager();
	bool getIsSyncEnabled();
private:
	uint64_t mLastLongCPProcessed; // conversation local
	bool mPreventStartup;
	bool mQUICConnectinShutdownInitiated;
	std::shared_ptr<CQUICConversationsServer> getQUICServer();
	void shutdownQUICConnection(bool abrutply=false);
	void setLocalIsMobile(bool isIt);
	std::shared_ptr<CQUICConversationsServer> mQUICServer;
	std::vector<uint8_t> mRXBuffer;
	uint64_t mDeferredTimeoutRequstedAtTimestamp;
	std::vector <std::shared_ptr<CCertificate>> mPendingObjects;
	std::vector<uint8_t> mPendingDataRequestIDs;
	std::mutex mSendGuardian;
	std::mutex mLocalChainProofGuardian; 
	std::vector <std::vector<uint8_t>> mLocalChainProof;
	bool mPingMechanicsEnabled;
	eConvEndReason::eConvEndReason mEndReason;
	uint64_t mMisbehaviorCounter;
	std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>> mLeaderInfo;
	uint64_t mTransmissionErrors;
	std::mutex mWebSocketGuardian;
	bool sendNetMsg(bool doItNOW, eNetEntType::eNetEntType eNetType, eNetReqType::eNetReqType eNetReqType, eDFSCmdType::eDFSCmdType dfsCMDType = eDFSCmdType::unknown,
		std::vector<uint8_t> dfsData1 = std::vector<uint8_t>(), std::vector<uint8_t> NetMSGRAWbytes = std::vector<uint8_t>(), bool pingStatus = true);
	uint64_t mLastTimePingReceived;
	std::mutex mPendingBlockBodyRequestsGuardian;
	std::vector <std::tuple<std::vector<uint8_t>,uint64_t, uint64_t>> mPendingBlockBodyRequests;//blockID,time, reqID
	std::shared_ptr<CWebSocket> getWebSocket();
	void setWebSocket(std::shared_ptr<CWebSocket> socket);
	uint64_t mInvalidMsgsCount;
	uint64_t mValidMsgsCount;
	std::mutex mStateGuardian;
	size_t getSecWindowSinceTimestamp();
	size_t mSecWindowSinceTimestamp;
	uint64_t mWarningsIssued;
	size_t mLastTimeCodeProcessed;
	std::mutex mDTISetGuardian;
	std::shared_ptr<CTools> getTools();
	std::shared_ptr<CBlockchainManager> getBlockchainManager();
	uint64_t mMisbehaviourCounter;
	bool incMisbehaviorCounter();
	uint64_t getMisbehaviorsCount();
	//UDTSOCKET mSocket;
	std::mutex mPreviousTaskGuardian;
	uint64_t mPreviousTaskID;
	eNetTaskState::eNetTaskState mPreviousTaskState;
	std::mutex mSocketGuardian;
	std::recursive_mutex mGuardian;
	UDTSOCKET getUDTSocket();
	void setUDTSocket(UDTSOCKET socket);
	bool taskChanged();
	bool mLogDebugData;
	bool getLogDebugData();
	std::shared_ptr<CSettings> mSettings;

	uint64_t  mAuthAttempts = 0;
	bool mUseAEADForAuth = false;
	bool mUseAEADForSessionKey = false;
	bool mAuthenticateHello = false;
	std::shared_ptr<CCryptoFactory> mCryptoFactory;
	//Guardians - BEGIN

	std::vector<uint8_t> mPubKey;
	Botan::secure_vector<uint8_t> mPrivKey;
	uint64_t mLastPingSent;
	std::uint64_t mLastLeaderNotificationDispatchTimestamp;
	std::mutex mFieldsGuardian, mSocketOperationGuardian;

	std::mutex mTasksQueueGuardian;
	std::mutex mNetMsgsQueueGuardian;
	std::mutex mScriptEngineGuardian;
	//std::mutex mDTIGuardian;
	static std::mutex  sIDGenGuardian;
	//Guardians - END
	
	
	bool getAreKeepAliveMechanicsToBeActive();
	void  setAreKeepAliveMechanicsToBeActive(bool isIt = true);
	void setIsSyncEnabled(bool isIt = true);
	HQUIC getQUICConnectionHandle() const;
	void setQUICConnectionHandle(HQUIC handle);
	bool closeSocket();
	std::shared_ptr<CTools> mTools;
	std::shared_ptr<CTransactionManager> mTransactionManager;
	//std::shared_ptr<CDTI> mDTI;
	std::vector<std::shared_ptr<CDTI>> mDTIs;


	std::shared_ptr<SE::CScriptEngine> mScriptEngine;// the 'system' thread
	std::shared_ptr<CEndPoint> mAbstractEndpoint;//represents the very conversation

	void enqueueNetMsg(std::shared_ptr<CNetMsg> msg);

	std::shared_ptr<CNetMsg> getRecentMsg();

	void dequeMsg();

	eTransportType::eTransportType mProtocol;
	static uint64_t  genNewID();

	std::vector<uint8_t> mID;


	static uint64_t sLastID;
	std::priority_queue<std::shared_ptr<CNetTask>,std::vector<std::shared_ptr<CNetTask>>, CCmpNetTasks> mTasks;
	std::queue<std::shared_ptr<CNetMsg>> mMsgs;

	
	void dequeTask();

	uint64_t getLastLeaderNotificationOutTimestamp();

	void pingLastLeaderNotificaitonTimestamp();

	
	
	uint64_t totalNrOfBytesExchanged;
	uint64_t nrOfMessagesExchanged;
	std::shared_ptr<CConversationState> mState;
	UDTSOCKET  mUDTSocket;
	std::unique_ptr<CVMMetaGenerator> mMetaGenerator;
	std::shared_ptr<CWebSocket> mWebSocket;
	std::vector<uint8_t> mFragmentBuffer;
	bool mWaitingForRest = false;
	uint64_t mRecentRequestID;
	std::shared_ptr<CEndPoint> mEndpoint;
	std::recursive_mutex mConversationThreadGuardian, mPingThreadGuardian;
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	std::future<void> mConversationThread, mPingMechanicsThread;
	std::shared_ptr<CNetworkManager> mNetworkManager;
	void pingMechanicsThread(std::shared_ptr<CConversation> self);
	
	void connectionHandlerThread(std::shared_ptr<CConversation> self,UDTSOCKET socket,std::string IPAdrress="", std::weak_ptr<CWebSocket> webSocket= std::weak_ptr<CWebSocket>()); //handle a private connection in a thread


	bool sendRAWData(const UDTSOCKET& socket, std::shared_ptr<CNetTask>& task);


	//=============================== NetMsg reqeusts and handlers ==========================

	bool processMsg( std::shared_ptr<CNetMsg> msg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr, uint64_t recursionStep=0);

	//==============Request Msgs and Processing of Requests msgs
	bool requestLongestPath(UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool handleRequestLongestPathMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool requestChainProof(UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleRequestChainProofMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool requestLatestBlockHeader(UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleRequestLatestBlockHeader( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);


	bool handleBlockRequestMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleBlockBodyRequestMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr, bool doItNow = false);
	
	//these might be used by light-clients:
	bool requestTransaction(std::vector<uint8_t> transactionID, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleTransactioneRequestMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool requestVerifiable(std::vector<uint8_t> verifiableID, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleVerifiableRequestMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool requestReceipt(std::vector<uint8_t> receiptID, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleReceiptRequestMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool requestVMMetaDataProcessing(std::vector<uint8_t> VMMetaDataBytes, bool doItNow=false);
	
	//=============Process Msgs and Handling of Process Msgs
	bool processLongestPath(std::vector<std::vector<uint8_t>> path, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleProcessLongestPathMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool processChainProof(std::vector<std::vector<uint8_t>> path, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleProcessChainProofMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool processLatestBlockHeader(std::shared_ptr<CBlockHeader> header, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleProcessLatestBlockHeaderMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool processBlock(std::shared_ptr<CBlock> block, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleProcessBlockMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool handleProcessBlockBodyMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool handleProcessQRIntentResponse(std::shared_ptr <CQRIntentResponse> response, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleProcessKademliaMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket=nullptr);
	bool handleProcessQRIntentResponseMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	//these might be used by light-clients:
	bool processTransaction( CTransaction & transaction, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool notifyTXReceptionResult(bool success, uint64_t reqID, std::string receiptID="");
	bool handleProcessTransactionMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleNotifyTransactionMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool processVerifiable(CVerifiable verifiable, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	std::shared_ptr<SE::CScriptEngine> getTargetVMByID(std::vector<uint8_t> id);
	bool handleProcessVMMetaDataMsg(std::shared_ptr<CNetMsg> msg);
	bool handleProcessVerifiableMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);


	//Notification Msgs and Processing of Notification Msgs
	bool notifyHelloMsg(std::shared_ptr<CNetMsg> hello,UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool requestHelloMsg(UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool isToEnd();
	bool sendBytes(std::vector<uint8_t>& packed, bool pingStatus=true, bool useDedicatedQUICStream=true);
	bool handleRequestHelloMsg(std::shared_ptr<CNetMsg>  netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleNotifyHelloMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool notifyReceiptIDMsg(std::vector<uint8_t> receiptID, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool notifyByeMsg(UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleNotifyByeMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool notifyNewBlock(std::shared_ptr<CBlockHeader> block, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleNotifyNewChatMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool handleNotifyNewBlockMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleNotifyNewBlockBodyMsg(std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleNotifyChainProofMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);
	bool handleNotifyLongestPathMsg( std::shared_ptr<CNetMsg> netmsg, UDTSOCKET socket,std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool handleProcessSDPMsg(std::shared_ptr<CSDPEntity> msg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);

	bool localSDPProcessing(std::shared_ptr<CSDPEntity> sdp);

	bool isSDPRoutable(std::shared_ptr<CSDPEntity> sdp);

	bool handleProcessDFSMsg(std::shared_ptr<CDFSMsg> msg, UDTSOCKET socket, std::shared_ptr<CWebSocket> webSocket = nullptr);

	void unmuteVM();
	void muteVM();

	


	
	
	void enqeueDFSErrorMsg(std::string errorMsg, uint64_t requestID = 0);

	bool enqeueVMMetaProcessingResultMsg(eVMMetaProcessingResult::eVMMetaProcessingResult status, std::vector<uint8_t> BERMetaData, uint64_t reqID, std::string message="");


	void enqeueDFSOk(std::string additionalInfo="", uint64_t reqID=0);

	void enqeueDFSOk(std::vector<uint8_t> additionaData, uint64_t reqID = 0);
	std::mutex mPrefixChangedGuardian;
	std::string mCurrentSDID;
	std::recursive_mutex  mConsoleGuardian;
	std::string mCurrentPath;
	bool mPrefixChanged;
	std::vector<uint8_t> mRequestedChallange;
	bool mChallangeResponseDispatched;
	bool mKeepAliveActive;
	std::vector<uint8_t> mPeerPubKey;
	bool mPeerAuthenticated;
	std::vector<uint8_t> mPeerAuthenticatedAsPubKey;
	Botan::secure_vector<uint8_t> mSessionKey;
	bool mSocketWasFreed;
	bool mEncryptionRequired;
	bool mAuthenticationRequired;
	bool mSignOutgressMsgs;
	

	void clearWarnings();
	uint64_t genRequestID();

	void setIsThreadAlive(bool isIt = true);
	bool getPingMechanicsEnabled();
	void setPingMechanicsEnabled(bool isIt);
	std::mutex mQUICShutdownGuardian;
	bool getCeaseCommunication();
	void setCeaseCommunication(bool isIt=true);
	void setQUICShutdownInitiated(bool wasIt=true);
	bool getQUICShutdownInitiated();
	void incStreamCount();
	uint64_t getStreamCount();
	void decStreamCount();
	void decOrderedStreamCount();
	void incOrderedStreamCount();
	uint64_t getOrderedStreamCount();
	uint64_t geAllowedStreamCount();
	void pingConvLocalLongCPProcessed();
	uint64_t getConvLocalLongCPProcessedAt();
	void initFields(bool justReset=false);
	std::shared_ptr<CSessionDescription> mRemoteSessionDescription;
	bool authAndEncryptMsg(std::shared_ptr<CNetMsg> msg, const Botan::secure_vector<uint8_t>&sessionKey = Botan::secure_vector<uint8_t>(), bool doNotRequireEncryption=false);

	bool sendNetMsg(std::shared_ptr<CNetMsg> msg, bool doItNow = true, bool doPreProcessing=true);
	std::vector<uint8_t>  mExpectedPeerPubKey; //Usually the public key delivered as part of CSessionDescription during the initial handshake

	void setRecentOperationStatus(std::shared_ptr<COperationStatus> status);
	void setCurrentState(eConversationState::eConversationState state);
	std::shared_ptr<COperationStatus> mRecentOperationStatus;
	std::mutex mRecentOperationStatusGuardian;
	bool getIsHelloStage();
	bool endInternal(bool notifyPeer = true, eConvEndReason::eConvEndReason = eConvEndReason::none);
public:
	QUIC_STATUS QUIC_API ConnectionCallback(_In_ HQUIC Connection, _Inout_ QUIC_CONNECTION_EVENT* Event);
	uint64_t getWarningsCount();
	bool getIsThreadAlive();

	bool getIsPingThreadAlive();
	void setIsPingThreadAlive(bool isIt);
	bool notifyVMStatus(eVMStatus::eVMStatus status, SE::vmFlags& flags, uint64_t processID = 0, std::vector<uint8_t> vmID =std::vector<uint8_t>(), std::vector<uint8_t> terminalID = std::vector<uint8_t>(), std::vector<uint8_t> conversationID = std::vector<uint8_t>(), bool doItNow=true,uint64_t inRespToReqID = 0);
	bool processNetMsg(std::shared_ptr<CNetMsg> msg, const std::vector<uint8_t>& receiptID = std::vector<uint8_t>(), std::shared_ptr<CConversation>dataConv = nullptr);
	void issueWarning();
	std::string getDescription();
	bool getDoSignOutgressMsgs();
	void setDoSignOutgressMsgs(bool doIt = true);
	std::shared_ptr<CSessionDescription> getRemoteSessionDescription();
	void setRemoteSessionDescription(std::shared_ptr<CSessionDescription> desc, bool updateRT=true);
	void setPrivKey(Botan::secure_vector<uint8_t> key);
	void setPubKey(std::vector<uint8_t> key);
	bool getIsAuthenticationRequired();
	void setIsAuthenticationRequired(bool isIt = true);
	void refreshAbstractEndpoint(bool updateRT = true);
	bool getIsEncryptionRequired();
	void setIsEncryptionRequired(bool isIt = true);

	
	Botan::secure_vector<uint8_t> getSessionKey();
	void setSessionKey(Botan::secure_vector<uint8_t> key);

	bool getPeerAuthenticated();
	void setPeerAuthenticated(bool wasIT=true);
	void setPeerAuthenticatedAsPubKey(const std::vector<uint8_t>& pubKey);
	std::vector<uint8_t> getPeerAuthenticatedAsPubKey();
	std::vector<uint8_t> getExpectedPeerPubKey();
	void setExpectedPeerPubKey(std::vector<uint8_t> pubKey);
	void setPeerPubKey(std::vector<uint8_t> pubKey);//used for signature verification
	std::vector<uint8_t> getRequestedChallange();
	void setRequestedChallange(std::vector<uint8_t> challange);
	bool getWasChallangeResponseDispatched();
	void setWasChallangeResponseDispatched(bool wasIt = true);

	bool prepareVM(bool reportStatus = true, std::shared_ptr<CDTI> dti = nullptr, uint64_t requestID = 0, std::vector<uint8_t> vmID = std::vector<uint8_t>(), uint64_t processID=0);
	std::shared_ptr <SE::CScriptEngine> getSystemThread();
	bool end(bool waitTillDone = true, bool notifyPeer = true, eConvEndReason::eConvEndReason = eConvEndReason::none);
	uint64_t getNrOfBytesExchanged();
	uint64_t getNrOfMsgsExchanged();

	std::shared_ptr<COperationStatus>  prepareOperationStatus(eOperationStatus::eOperationStatus status, eOperationScope::eOperationScope scope = eOperationScope::dataTransit,
	std::vector<uint8_t> ID = std::vector<uint8_t>(), uint64_t reqID = 0);
	bool getIsScheduledToStart();
	void setIsScheduledToStart(bool isIt);
	CConversation( CConversation &sibling);
	std::shared_ptr<CConversationState> getState();
	void setCurrentSD(std::string sdID);
	void setChangedCLIPrefix(bool set = true);
	void setCurrentPath(std::string path);
	std::vector<uint8_t> registerQRIntentResponse(std::shared_ptr<CQRIntentResponse> response);
	std::vector<uint8_t> registerVMMetaDataResponse(std::vector<std::shared_ptr<CVMMetaSection>> sections);
	CConversation(UDTSOCKET  socket,std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm, std::shared_ptr <CEndPoint> endpoint, std::shared_ptr<CNetTask> task=nullptr);
	CConversation(std::weak_ptr<CWebSocket> webSocket, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm, std::shared_ptr <CEndPoint> endpoint, std::shared_ptr<CNetTask> task = nullptr);
	CConversation(HQUIC connHandle, HQUIC registration, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm, std::shared_ptr <CEndPoint> endpoint, std::shared_ptr<CNetTask> task = nullptr);
	

// ConnectionCallbackTrampoline is a static member function of the CConversation class.
// It acts as a bridge (or "trampoline") between the C-style callback mechanism used by
// the MS QUIC library and the C++ member function callback mechanism.
//
// The key reason for using a trampoline here is to enable the MS QUIC library, which
// expects a simple function pointer for callbacks, to invoke member functions on CConversation
// instances. The trampoline achieves this by taking advantage of the 'Context' parameter
// provided by MS QUIC callbacks, which allows passing arbitrary user data (in this case,
// a pointer to a CConversation instance) through the callback.
//
// The trampoline function does the following:
// 1. Receives a 'Context' parameter from MS QUIC, which it knows is a pointer to a
//    CConversation instance (because we set it up that way when registering the callback).
// 2. Casts this 'Context' back to a CConversation pointer.
// 3. Invokes the appropriate member function on the CConversation instance, effectively
//    bridging the gap between the C-style callback and C++ member function.
//
// This approach allows each CConversation instance to handle its callbacks in a way
// that maintains object-oriented design principles and access to instance-specific state.

	static QUIC_STATUS QUIC_API ConnectionCallbackTrampoline(
		HQUIC Connection,
		void* Context,
		QUIC_CONNECTION_EVENT* Event)
	{
		auto* conversation = reinterpret_cast<CConversation*>(Context);
		return conversation->ConnectionCallback(Connection, Event);
	}

	static QUIC_STATUS QUIC_API StreamCallbackTrampoline(
		HQUIC Stream,
		void* Context,
		QUIC_STREAM_EVENT* Event
	) {

		auto conversation = static_cast<CConversation*>(Context);
		return conversation->StreamCallback(Stream, Event);
	}

	
	//high-level NetMsg processing - the processing might take place within an UDT Private Conversation
	// or the message might be delivered through Kademlia P2P sub-system.
	
	

};
