#pragma once

#include "stdafx.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>
#include <limits>  
#include <stdlib.h>
#include <algorithm>  

#include "uint256.h"
#include <iostream>
#include <cctype>
#include "enums.h"
#include "IManager.h"
#include "mongoose/mongoose.h"
#include <mutex>

class ThreadPool;
class CConversation;
class CBlockchainManager;
class CNetworkManager;
class CStateDomainManager;
class CSolidStorage;
class CICENode;

/// <summary>
/// Implements a WWW Server. The server manages access to the Web-UI for remote peers. The incoming/outgoing commands have no state. 
//	Sessions CAN be maintained through underlying 'protocols' accessed through the WWW Server. Decomposition and processing of such sessions SHOULD BE
//	managed by the corresponding protocol's server. Still, the WWW server comes with an auto-firewall capabilities, tracking insensitivity of queries per IP.
/// </summary>
static  std::string convertToLinuxFormat(const std::string& filePath);
static void log_via_log_file(const void* buf, size_t len, void* userdata);
static struct mg_fd* p_gn_open(const char* pathA, int flags);
static void p_gn_close(struct mg_fd *fd);
static size_t p_gn_read(void *fp, void *buf, size_t len);
static size_t p_gn_write(void *fp, const void *buf, size_t len);
static size_t p_gn_seek(void* fp, size_t offset);
static int p_gn_stat(const char* pathA, size_t* size, time_t* mtime);
static void p_gn_list(const char* dir, void (*fn)(const char*, void*), void* userdata);

class CWWWServer : public  std::enable_shared_from_this<CWWWServer>, public IManager
{

private:
	void configureWebSocketBuffers(struct mg_connection* c);

	struct HTTPConnectionState {
		uint64_t connectTime;
		uint64_t lastCompleteMessageTime;
		uint64_t currentRequestStartTime;
		bool hasPartialRequest;
		size_t partialRequestSize;
		uint32_t completedRequests;

		// web-socket - BEGIN
		uint64_t upgradeStartTime;
		bool handshakeComplete;
		uint64_t emptyFrameCount;        // Add this: Per-connection empty frame counter
		uint64_t lastEmptyFrameTime;     // Add this: Track timing of empty frames
		// web-socket - END

		HTTPConnectionState() :
			connectTime(0),
			lastCompleteMessageTime(0),
			currentRequestStartTime(0),
			hasPartialRequest(false),
			upgradeStartTime(0),
			handshakeComplete(false),
			partialRequestSize(0),
			completedRequests(0),
			emptyFrameCount(0),          // Initialize new fields
			lastEmptyFrameTime(0) {
		}
	};


	std::mutex mHTTPStateGuardian;
	std::unordered_map<mg_connection*, HTTPConnectionState> mHTTPStates;

	// Slowloris protection methods - BEGIN

	// HTTP protective fields
	std::recursive_mutex mHTTPConnectionsGuardian;
	//std::unordered_map<mg_connection*, HTTPConnectionInfo> mHTTPConnections;

	// Web-Sockets - BEGIN
	

	void trackWebSocketUpgrade(mg_connection* c);
	void completeWebSocketUpgrade(mg_connection* c);
	void removeWebSocketUpgrade(mg_connection* c);
	void enforceWebSocketUpgradeTimeouts();
	// Web-Sockets - END

	

	// HTTP - BEGIN

	void enforceHTTPTimeouts();
	// HTTP - END

	// the following callback fires even before Mongoose accepts an incoming connection
	static bool mongoosePreAcceptCallback(const struct sockaddr* addr, socklen_t addr_len, void* user_data);
	// Slowloris protection methods - END

public:
	void pingLastMongooseEvent();
	uint64_t getLastMongooseEvent();
	void testPathSanitization();
	CWWWServer(std::shared_ptr<ThreadPool> pool, std::shared_ptr<CBlockchainManager> bm);
	std::string getRootDir();
	~CWWWServer();
	bool initialize();

	// Inherited via IManager
	virtual void stop() override;
	virtual void pause() override;
	virtual void resume() override;
	void mControllerThreadF();
	virtual eManagerStatus::eManagerStatus getStatus() override;
	virtual void setStatus(eManagerStatus::eManagerStatus status) override;

	// Inherited via IManager
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status) override;
	virtual eManagerStatus::eManagerStatus getRequestedStatusChange() override;
	size_t getLastTimeCleanedUp();
	size_t getActiveSessionsCount();
	size_t getMaxSessionsCount();
	void setLastTimeCleanedUp(size_t timestamp = 0);

	bool getIsMaintenanceMode();

	void setIsMaintenanceMode(bool isIt=true);
	std::shared_ptr<CTools> getTools();
	std::shared_ptr<ThreadPool> getThreadPool();
	//commands
	std::vector<std::shared_ptr<CConversation>> getConversationsByIP(std::vector<uint8_t> IP);
	bool registerConversation(std::shared_ptr<CConversation> conversation);
	std::shared_ptr<CConversation> getConversationByID(std::vector<uint8_t> id);
	void setUIDebugMode(bool isIt = true);

	bool getUIDebugModeActive();

	void setIsRemoteUIDebugMode(bool isIt);
	bool getIsRemoteUIDebugModeActive();
	std::vector <std::shared_ptr<CICENode>> getICENodes();
	void setICENodes(std::vector <std::shared_ptr<CICENode>> nodes);
	void addICENode(std::shared_ptr<CICENode> node);

	void setThreadPool(std::shared_ptr<ThreadPool> pool);

private:
	bool mRemoteUIDebugMode;
	uint64_t mLastMongooseEvent;
	std::shared_ptr<ThreadPool> mThreadPool;
	std::string mUIDirectory;
	std::mutex mMongooseEventTimestampLock;
	CSolidStorage* mSS;
	bool prepareCertificate();
	struct mg_tls_opts mOpts;
	void updateRootDir();
	mg_http_serve_opts mDecentralizedServeOpts, mServeOpts;
	//Mongoose Elements - BEGIN
	bool mMaintenanceMode;
	std::string mAppRootDir;
	bool mDebugMode;
	std::vector <std::shared_ptr<CICENode>> mKnownICENodes;
	void serveSysPage(struct mg_connection* c, int ev, void* ev_data, eSysPage::eSysPage page);
	bool serveJSFile(mg_connection* c, int ev, void* ev_data);
	bool serveMainPage(mg_connection* c, int ev, void* ev_data);
	struct mg_mgr mMongooseMgr;
	struct mg_connection* mMongooseConnection;
	//Mongoose Elements - END
	//Mongoose Settings - BEGIN
	void redirectToHttps(struct mg_connection* c, int ev, void* ev_data, void* fn_data);
	void redirectToURL(struct mg_connection* c, std::string url, bool isTemporal =true);
	std::string mDebugLevel;
	//std::string mRootDir;
	std::string mRootDirDec;
	std::string mCertDir;
	std::string mListeningAddressTLS;
	std::string mListeningAddressRAW;
	std::string mCertPath, mPrivKeyPath;


	std::string mEnableHexdump;
	std::string mSsPattern,mSsPatternDec;
	//Mongoose Settings - END
	std::string getKnownNodesHTMLString();
	std::string getKnownICENodesHTMLString();

	//RESTFul API - BEGIN

	void handleGetVersion(mg_connection* c, mg_http_message* hm);
	void handleGetHeight(mg_connection* c, mg_http_message* hm);
	void handleGetBalance(mg_connection* c, mg_http_message* hm, std::string & clientIP);
	void handleGetBlock(mg_connection* c, mg_http_message* hm);
	void handleGetTX(mg_connection* c, mg_http_message* hm);
	void handleGenAddress(mg_connection* c, mg_http_message* hm);
	void handleSendTX(mg_connection* c, mg_http_message* hm);
	std::shared_ptr<CTransaction> prepareTX(uint64_t nonce, Botan::secure_vector<uint8_t>& privKey, std::vector<uint8_t>& pubKey, std::vector<uint8_t>& issuer, const std::vector<uint8_t>& byteCode, BigInt ERGLimit, uint64_t ERGPrice);
	std::shared_ptr<CTransaction> prepareTX(uint64_t nonce, Botan::secure_vector<uint8_t>& privKey, std::vector<uint8_t>& pubKey, std::vector<uint8_t>& issuer, const std::string & sourceCode, BigInt ERGLimit, uint64_t ERGPrice);
	void handleProcessCode(mg_connection* c, mg_http_message* hm);
	std::shared_ptr<CTransaction> prepareTX(uint64_t nonce, Botan::secure_vector<uint8_t> &privKey,std::vector<uint8_t> &pubKey, std::vector<uint8_t>& issuer, std::string recipient, BigInt value, BigInt ERGLimit, uint64_t ERGPrice);
	void errorResponse(mg_connection* c, int status, const std::string& errorDescription);
	mg_mgr * getMongooseMgr();
	//RESTFul API - END
	// 
	//Mongoose Callbacks - BEGIN
	void cb(struct mg_connection* c, int ev, void* ev_data, void* fn_data);
	void handleCheckAddress(mg_connection* c, mg_http_message* hm);
	bool serveVMFile(struct mg_connection* c, std::string path);
	//Mongoose Callbacks - END
	std::shared_ptr<CNetworkManager> getNetworkManager();
	std::mutex mLastAliveListingGuardian;
	uint64_t getLastAliveListing();
	uint64_t mLastAliveListingTimestamp;
	void lastAlivePrinted();

	uint64_t cleanConversations();
	uint64_t doCleaningUp(bool forceKill = false);

	std::recursive_mutex mConversationsGuardian;
	std::vector<std::shared_ptr<CConversation>> mConversations;

	size_t mMaxWarningsCount;
	size_t mMaxSessions;

	std::string mRootDir;
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	std::shared_ptr<CStateDomainManager> mStateDomainManager;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	eManagerStatus::eManagerStatus mStatus;
	std::thread mControllerThread;
	eManagerStatus::eManagerStatus mStatusChange;
	std::recursive_mutex mStatusGuardian, mGuardian;
	std::mutex mFieldsGuardian;
	bool mShutdown;
	std::shared_ptr<CBlockchainManager> getBlockchainManager();
	std::shared_ptr<CStateDomainManager> getStateDomainManager();
	std::thread mWWWServerThread;
	void WWWServerThreadF();
	
	bool initializeServer();
	size_t mLastTimeCleaned;
	std::shared_ptr<CTools> mTools;
	std::mutex mStatusChangeGuardian;
	std::weak_ptr<CNetworkManager> mNetworkManager;
};
