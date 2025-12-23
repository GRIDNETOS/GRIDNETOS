#pragma once
#include "stdafx.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>
#include <limits>  
#include <stdlib.h>
#include <algorithm>  
#include "botan_all.h"
#include "uint256.h"
#include <iostream>
#include <cctype>
#include "enums.h"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include "IManager.h"


class CBlockchainManager;
class CNetworkManager;
class CFileSystemSession;
class CWebSocketsServer;

class CFileSystemServer : public  std::enable_shared_from_this<CFileSystemServer>, public IManager
{
public:
	CFileSystemServer(std::shared_ptr<CBlockchainManager> bm);
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
	std::shared_ptr<CFileSystemSession> getSessionByID(std::vector<uint8_t> id);
	std::shared_ptr<CFileSystemSession> getSessionByIP(std::string IP);
	size_t getMaxSessionsCount();
	void setLastTimeCleanedUp(size_t timestamp = 0);

	//commands
	eFSCmdResult::eFSCmdResult processCmd(std::vector<uint8_t> sessionID, eDFSCmdType::eDFSCmdType cType, std::vector<uint8_t> inputData,
		std::vector<uint8_t> &outputData);

private:
	//CMD-processing

	//JavaScript / WebSocket-compatibe response serialization

	std::vector<std::shared_ptr<CFileSystemSession>> mSessions;
	uint64_t cleanSessions();
	size_t mLastTimeCleaned;
	size_t mMaxSessionInactivity;
	size_t mMaxWarningsCount;
	size_t mMaxSessions;

	
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	eManagerStatus::eManagerStatus mStatus;
	std::thread mControllerThread;
	eManagerStatus::eManagerStatus mStatusChange;
	std::mutex mStatusGuardian, mGuardian;
	std::mutex mFieldsGuardian;
	std::mutex mSessionsGuardian;
	bool mShutdown;
	std::thread mFileSystemServerThread;
	void fileSystemServerThreadF();
	bool initializeServer();
	uint64_t doCleaningUp(bool forceKillAllSessions = false);
	std::shared_ptr<CTools> mTools;
	std::mutex mStatusChangeGuardian;
	std::shared_ptr<CNetworkManager> mNetworkManager;
	std::shared_ptr<CWebSocketsServer> mWebSocketsServer;
};
