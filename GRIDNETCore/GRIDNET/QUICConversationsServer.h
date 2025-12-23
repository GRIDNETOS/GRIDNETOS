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
#include "msquic.h"

class ThreadPool;
class CConversation;
class CDTI;
class CTools;
class CQRIntentResponse;
class CBlockchainManager;


/// <summary>
/// QUIC Conversations Server takes care of managing QUIC Conversations .
/// The lifetime of  Server is controlled by the Network Manager but all of its internal operations are
/// fully independent.
/// </summary>
/// 
/// 
class CQUICConversationsServer : public  std::enable_shared_from_this<CQUICConversationsServer>, public IManager
{
private:


	std::mutex mLastAliveListingGuardian;
	uint64_t mLastAliveListingTimestamp;
	uint64_t getLastAliveListing();
	std::shared_ptr<CTools> getTools();
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	HQUIC mListenerHandle; // Handle for the MS-QUIC listener
	HQUIC mRegistration;   // MS-QUIC registration object
	HQUIC mConfiguration;  // MS-QUIC configuration object
	eManagerStatus::eManagerStatus mStatus;
	std::thread mControllerThread;
	eManagerStatus::eManagerStatus mStatusChange;
	std::recursive_mutex mStatusGuardian, mGuardian;
	std::mutex mFieldsGuardian;
	std::mutex mNetworkManagerGuardian;
	std::recursive_mutex mConversationsGuardian;
	bool mShutdown;
	std::thread mQUICServerThread;
	std::string getCertPath();
	std::string getKeyPath();
	void QUICConversationServerThreadF();
	bool bootUpQUICServerSocket();
	uint64_t mLastConversationCleanup;
	CSolidStorage* mSS;
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
	std::string mLastConnectionsReport;
	size_t mMaxSessions;
	size_t getActiveSessionsCount();
	std::mutex mStatusChangeGuardian;
	std::shared_ptr<CNetworkManager> mNetworkManager;
	void* (*mUnsolicitedDataCallback) (std::vector<uint8_t>& bytes, std::vector<uint8_t>& senderID, void* fn_data);
	std::shared_ptr<ThreadPool> mThreadPool;
	std::string mCertPath, mPrivKeyPath;
	const QUIC_API_TABLE* mMsQuic;

public:

	std::string getLastConnectionsReport();
	void setLastConnectionsReport(const std::string& report);
	const QUIC_API_TABLE* getQUIC() const;

	QUIC_STATUS QUIC_API ServerListenerCallback(_In_ HQUIC Listener, _Inout_ QUIC_LISTENER_EVENT* Event);
	void HandleListenerStop();
	std::string describeConnectionResult(eIncomingConnectionResult::eIncomingConnectionResult result);
	bool initializeQUICRegistration(QUICExecutionProfile::QUICExecutionProfile profile);
	bool initializeQUICConfiguration(bool isServer, HQUIC& configuration, uint64_t allowedStreamCount);
	bool StartQuicServer(const std::string& ipAddress);
	HQUIC getRegistration();
	std::shared_ptr<ThreadPool> getThreadPool();
	std::shared_ptr<CNetworkManager> getNetworkManager();
	bool addConversation(std::shared_ptr<CConversation> convesation);
	~CQUICConversationsServer();
	std::vector<std::shared_ptr<CConversation>> getAllConversations();
	std::shared_ptr<CConversation> getConversationByID(std::vector<uint8_t> id);
	std::shared_ptr<CConversation> getConversationByIP(std::vector<uint8_t> IP);
	size_t getConversationsCountByIP(std::vector<uint8_t> IP);
	size_t getMaxSessionsCount();

	size_t getLastTimeCleanedUp();
	void setLastTimeCleanedUp(size_t timestamp = 0);
	CQUICConversationsServer(std::shared_ptr<ThreadPool> pool, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm);
	bool initialize();
	// Inherited via IManager
	virtual void stop() override;
	void shutdownQUIC();
	virtual void pause() override;
	virtual void resume() override;
	void mControllerThreadF();
	virtual eManagerStatus::eManagerStatus getStatus() override;
	bool dispatchKademliaDatagram(const std::vector<uint8_t>& data, const std::string& destinationIP, uint16_t destinationPort);
	virtual void setStatus(eManagerStatus::eManagerStatus status) override;

	// Inherited via IManager
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status) override;
	virtual eManagerStatus::eManagerStatus getRequestedStatusChange() override;
};

static QUIC_STATUS QUIC_API ServerListenerTrampoline(
	_In_ HQUIC Listener,
	_In_opt_ void* Context,
	_Inout_ QUIC_LISTENER_EVENT* Event
) {
	auto* server = static_cast<CQUICConversationsServer*>(Context);
	return server->ServerListenerCallback(Listener, Event);
}

