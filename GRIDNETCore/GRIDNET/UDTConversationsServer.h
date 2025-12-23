#pragma once
#include "stdafx.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>
#include <limits>  
#include <stdlib.h>
#include <algorithm>  

#include "arith_uint256.h"
#include "uint256.h"
#include <iostream>
#include <cctype>
#include <boost/multiprecision/cpp_int.hpp> ////vcpkg install boost-multiprecision --triplet x64-windows
#include <boost/multiprecision/cpp_dec_float.hpp>
#include "enums.h"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include "IManager.h"

class ThreadPool;
class CConversation;
class CDTI;
class CTools;
class CQRIntentResponse;
class CBlockchainManager;




/// <summary>
/// UDT Conversations Server takes care of managing UDT Conversations .
/// The lifetime of  Server is controlled by the Network Manager but all of its internal operations are
/// fully independent.
/// </summary>
/// 
/// 
class CUDTConversationsServer : public  std::enable_shared_from_this<CUDTConversationsServer>, public IManager
{
private:


	std::mutex mLastAliveListingGuardian;
	uint64_t mLastAliveListingTimestamp;
	uint64_t getLastAliveListing();
	std::shared_ptr<CTools> getTools();
	void setLastConnectionsReport(const std::string& report);
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	UDTSOCKET mUDTServerSocket;
	eManagerStatus::eManagerStatus mStatus;
	std::thread mControllerThread;
	eManagerStatus::eManagerStatus mStatusChange;
	std::recursive_mutex mStatusGuardian, mGuardian;
	std::mutex mFieldsGuardian;
	std::mutex mNetworkManagerGuardian;
	std::recursive_mutex mConversationsGuardian;
	uint64_t mUDTSocketsClosedCount;
	bool mShutdown;
	std::thread mUDTServerThread;
	void UDTConversationServerThreadF();
	bool bootUpUDTServerSocket();
	uint64_t mLastConversationCleanup;
	void killServerSocket();
	bool initializeServer();
	std::vector<std::shared_ptr<CConversation>> mConversations;
	uint64_t doCleaningUp(bool forceKillAllSessions = false);
	uint64_t cleanConversations();
	void lastAlivePrinted();
	size_t mLastTimeCleaned;
	size_t mMaxSessionInactivity;
	size_t mMaxWarningsCount;
	std::shared_ptr<CTools> mTools;

	size_t mMaxSessions;
	size_t getActiveSessionsCount();
	std::mutex mStatusChangeGuardian;
	std::shared_ptr<CNetworkManager> mNetworkManager;
	void *(*mUnsolicitedDataCallback) (std::vector<uint8_t>& bytes, std::vector<uint8_t>& senderID, void* fn_data);
	std::shared_ptr<ThreadPool> mThreadPool;
	std::string mLastConnectionsReport;
public:
	std::string getLastConnectionsReport();

	std::shared_ptr<ThreadPool> getThreadPool();
	std::shared_ptr<CNetworkManager> getNetworkManager();
	bool addConversation(std::shared_ptr<CConversation> convesation);
	~CUDTConversationsServer();
	std::vector<std::shared_ptr<CConversation>> getAllConversations();
	std::shared_ptr<CConversation> getConversationByID(std::vector<uint8_t> id);
	std::shared_ptr<CConversation> getConversationByIP(std::vector<uint8_t> IP);
	size_t getConversationsCountByIP(std::vector<uint8_t> IP);
	size_t getMaxSessionsCount();

	size_t getLastTimeCleanedUp();
	uint64_t getUDTSocketsClosedCount();
	void incUDTSocketsClosedCount();
	void setLastTimeCleanedUp(size_t timestamp = 0);
	CUDTConversationsServer(std::shared_ptr<ThreadPool> pool, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm);
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
};

void unsolicitedDatagramCallback(uint64_t protocolType, std::vector<uint8_t> &data, std::vector<uint8_t>& senderID , uint64_t srcPort, void* fn_data);
void validDatagramCallback(uint64_t protocolType, std::vector<uint8_t>& senderID, uint64_t srcPort, void* fn_data);