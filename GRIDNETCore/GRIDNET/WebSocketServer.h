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
#include <libssh/libssh.h>
#include <libssh/server.h>
#include "IManager.h"
#include "mongoose/mongoose.h"

class CWebSocket;
class CConversation;
class CBlockchainManager;
class CNetworkManager;
class CWebSocketResponse;
class CWebRTCSwarm;
class ThreadPool;

static void log_via_log_file(const void* buf, size_t len, void* userdata);

/// <summary>
/// Implements a Web-Socket server. The server manages WebSockets. The incoming/outgoing commands have no state. 
//	Sessions CAN be maintained through underlying 'protocols' accessed through the WebSocket server. Decomposition and processing of such sessions SHOULD BE
//	managed by the corresponding protocol's server. Still, the WebSockets server comes with an auto-firewall capabilities, tracking insensitivity of queries per IP.
/// </summary>
class CWebSocketsServer : public  std::enable_shared_from_this<CWebSocketsServer>, public IManager
{
public:
	CWebSocketsServer(std::shared_ptr<ThreadPool> pool,std::shared_ptr<CBlockchainManager> bm, bool initSwarmMechanics=true);
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

	//commands
	//std::shared_ptr<CWebSocketResponse> processInput(std::vector<uint8_t> input);

	std::vector<std::shared_ptr<CConversation>> getConversationsByIP(std::vector<uint8_t> IP);
	std::vector<std::shared_ptr<CConversation>> getAllConversations();
	bool registerConversation(std::shared_ptr<CConversation> conversation);
	std::shared_ptr<CConversation> getConversationByID(std::vector<uint8_t> id);
	std::shared_ptr<CWebRTCSwarm>  createSwarmWithID(std::vector<uint8_t> id);
	std::shared_ptr<CWebRTCSwarm> getRandomSwarm();
	void lockSwarms(bool doIt);
	std::shared_ptr< CWebRTCSwarm> getSwarmByID(std::vector<uint8_t> id);
	bool getSwarmMechanicsEnabled();
	void setSwarmMechanicsEnabled(bool setIt = true);
	uint64_t mLastSwarmsTick ;
	uint64_t mMaxActiveSwarmsCounts;
	uint64_t mMaxSwarmInactivityThreshold;
	bool sendNetMsgRAW(std::shared_ptr<CWebSocket> socket, eNetEntType::eNetEntType eNetType, eNetReqType::eNetReqType eNetReqType, std::vector<uint8_t> RAWbytes);
	bool sendNetMsg(std::shared_ptr<CWebSocket> socket, eNetEntType::eNetEntType eNetType, eNetReqType::eNetReqType eNetReqType, eDFSCmdType::eDFSCmdType dfsCMDType, std::vector<uint8_t> dfsData1, std::vector<uint8_t> NetMSGRAWbytes);
	std::shared_ptr<ThreadPool> getThreadPool();

private:
	//Mongoose Elements - BEGIN
	std::shared_ptr<CTools> getTools();
	//Mongoose Elements - END
	//Mongoose Settings - BEGIN
	std::string mDebugLevel;
	std::string mRootDir;
	std::string mListeningAddress;
	std::string mEnableHexdump;
	std::string mSsPattern;
	//Mongoose Settings - END

	//Mongoose Callbacks - BEGIN
	//Mongoose Callbacks - END


	std::shared_ptr<CNetworkManager> getNetworkManager();
	std::mutex mLastAliveListingGuardian;
	uint64_t getLastAliveListing();
	uint64_t mLastAliveListingTimestamp;
	void lastAlivePrinted();
	std::vector<std::shared_ptr<CWebRTCSwarm>> mSwarms;
	std::recursive_mutex mSwarmsGuardian;
	uint64_t cleanConversations();
	uint64_t cleanSwarms();
	void  makeSwarmsTick();

	uint64_t doCleaningUp(bool forceKill = false);
	bool addSwarm(std::shared_ptr<CWebRTCSwarm> swarm);
	//CMD-processing
	
	std::recursive_mutex mConversationsGuardian;
	std::vector<std::shared_ptr<CConversation>> mConversations;
	std::shared_ptr<ThreadPool> mThreadPool;
	size_t mMaxWarningsCount;
	//size_t mMaxSessions;
	bool mSwarmMechanicsEnabled;
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	eManagerStatus::eManagerStatus mStatus;
	std::thread mControllerThread;
	eManagerStatus::eManagerStatus mStatusChange;
	std::recursive_mutex mStatusGuardian, mGuardian;
	std::mutex mFieldsGuardian;
	bool mShutdown;
	std::thread mWebSocketServerThread;
	void WebSocketServerThreadF();
	
	bool initSwarmMechanics();
	bool initializeServer();
	size_t mLastTimeCleaned;
	std::shared_ptr<CTools> mTools;
	std::mutex mStatusChangeGuardian;
	std::weak_ptr<CNetworkManager> mNetworkManager;
};
