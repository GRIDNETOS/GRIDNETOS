#pragma once
#include <vector>
#include <mutex>
#include "ScriptEngine.h"
#include "enums.h"
#include <ixwebsocket/IXWebSocket.h>
#include "NetTask.h"
#include <queue>

class CDTIServer;
class CFileSystemServer;
class CConversationState;
class CBlockchainManager;
class CTransactionManager;
class CNetTask;
class CNetworkManager;

class CFileSystemSession : public  std::enable_shared_from_this<CFileSystemSession>
{
private:
	void dequeTask();
	//eDFS
	eDFSTaskProcessingResult::eDFSTaskProcessingResult processFileSystemTask(std::shared_ptr<CNetTask> task);

	std::priority_queue<std::shared_ptr<CNetTask>, std::vector<std::shared_ptr<CNetTask>>, CCmpNetTasks> mTasks;
	std::mutex mQueueGuardian;
	std::thread mConversationThread;
	bool addTask(std::shared_ptr<CNetTask> task);
	std::shared_ptr<CNetTask> getCurrentTask(bool deqeue=false);
	void FileSystemSessionHandlerThreadF(std::shared_ptr<ix::WebSocket> socket); //handle a private connection in a thread
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	std::shared_ptr<CConversationState> mState;
	//internals 
	std::mutex mFieldsGuardian;
	size_t mLastWarningTimestamp;
	void mControllerThreadF();
	size_t mSessionStartedTimeStamp;
	size_t mLastInteractionTimeStamp;
	std::string mClientIP;

	//identification
	std::vector<uint8_t> mID;
	std::vector<uint8_t> mDTISessionID;
	std::shared_ptr<CDTI> mDTI;
	//#GridScript engine
	std::shared_ptr<SE::CScriptEngine> mScriptEngine;//will be shared across a DTI session if active
	//thus intermediary/not commited results within the GUI will be available within the Decentralized Terminal Interface

	eBlockchainMode::eBlockchainMode mBlockchainMode;
	//external servers
	std::shared_ptr <CDTIServer> mDTIServer;
	std::shared_ptr <CFileSystemServer> mFileSystemServer;
	void setReady(bool isReady = true);

	//path
	void setCurrentSD(std::string sdID);
	std::string getCurrentSD();
	void setCurrentPath(std::string path);
	std::string getCurrentPath();
	eDFSSessionStatus::eDFSSessionStatus mStatus;
	std::shared_ptr<CTransactionManager> mTransactionManager;
	//std::shared_ptr<SE::CScriptEngine> mScriptEngine;

public:
	CFileSystemSession(std::shared_ptr<CNetworkManager> nm,std::vector<uint8_t> DTISessionID= std::vector<uint8_t>());
	std::vector<uint8_t> getID();
	size_t getLastActivityTimestamp();

	bool CFileSystemSession::end(bool waitTillDone=true);

	std::shared_ptr<CConversationState> getState();

	//statystics
	uint64_t getWarningsCount();

	//DTI Server Integration
	bool attachDTISession(std::vector<uint8_t> mDTISessionID);
	std::vector<uint8_t> getDTIServerID();

	//data-commits
	std::vector<uint8_t> registerQRIntentResponse(std::shared_ptr<CQRIntentResponse> response);
	eFSCommitResult::eFSCommitResult getCommitResult(std::vector<uint8_t> receiptID);

	//client info
	std::string getClientIP();


	

};