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
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include "enums.h"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include "IManager.h"
#include <future>

#include <kademlia\session.hpp>
#include <kademlia\first_session.hpp>
//#include <kademlia\engine.hpp>
//#include <kademlia/src/kademlia/engine.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>


namespace kademlia
{
	namespace detail
	{
		template< typename UnderlyingSocketType >
		class engine;



	}
}

class CEndPoint;
class CConversation;
class CDTI;
class CTools;
class CQRIntentResponse;
class CBlockchainManager;
class CNetworkManager;
//using socket_type =  boost::asio::ip::udp::socket;
//using engine_type = kademlia::detail::engine< socket_type >;
/// <summary>
/// Kademlia Server(Client - P2P) takes care of managing decentralized node discovery .
/// The lifeteime of Server/Client is controlled by the NetworkManager but all of its internal operations are
/// fully independent.
/// </summary>
class CKademliaServer : public  std::enable_shared_from_this<CKademliaServer>, public IManager
{
public:
	void setCurrentBootstrapNode(std::shared_ptr<CEndPoint> nooe);
	std::shared_ptr<CEndPoint> getCurrentBootstrapNode();
	static void setActiveKadEngine(kademlia::detail::engine<boost::asio::ip::udp::socket>* engine);
	static kademlia::detail::engine<boost::asio::ip::udp::socket>* getActiveKadEngine();
	//static bool notifyPendingUDPSockets(std::vector<uint8_t> &data, kademlia::detail::ip_endpoint endpoint);
	//static void addPendingUDPSocket(kademlia::detail::message_socket<boost::asio::ip::udp::socket>*);
	bool processKademliaDatagram(std::vector<uint8_t> bytes, const CEndPoint& endpoint);
private:

	static std::mutex sActiveKadEngineGuardian;
	static kademlia::detail::engine<boost::asio::ip::udp::socket>* sActiveKadEngine;


	kademlia::endpoint mIPv4Endpoint;
	std::mutex mCurrentBootstrapNodeGuardian;
	std::vector<std::shared_ptr<CEndPoint>> mKnownPeers;
	bool mIsBootstrapNode;
	std::shared_ptr<CEndPoint> mCurrentBootstrapNode;
	//std::vector<CEndPoint> mBootstrapPeers;
	std::future<std::error_code> mKademiliaMainLoopResult;
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	eManagerStatus::eManagerStatus mStatus;
	std::thread mControllerThread;
	eManagerStatus::eManagerStatus mStatusChange;
	std::recursive_mutex mStatusGuardian, mGuardian;
	std::mutex mFieldsGuardian;
	bool mShutdown;
	std::thread mKademliaServerThread;
	kademlia::session* mKademliaSession;
	kademlia::first_session* mKademliaBootstrapSession;
	kademlia::first_session* getBootstrapSession();
	kademlia::session* getSession();

	void KademliaServerThreadF();
	bool getIsBootstreapNode();
	bool initializeServer();
	std::shared_ptr<CNetworkManager> getNetworkManager();
	std::recursive_mutex mPeersGuardian;
	uint64_t cleanupPeers();

	uint64_t doCleaningUp(bool forceKillAllSessions = false);

	size_t mLastTimeCleaned;
	size_t mMaxSessionInactivity;
	size_t mMaxWarningsCount;
	std::shared_ptr<CTools> mTools;
	std::mutex mToolsGuardian;
	std::shared_ptr<CTools> getTools();
	size_t mMaxSessions;
	std::mutex mStatusChangeGuardian;
	std::shared_ptr<CNetworkManager> mNetworkManager;
	std::shared_ptr<CEndPoint> findPeerByIP(std::string IP, bool updateLastTimeSeen = false);
public:
	void refreshPeers();
	std::vector<std::shared_ptr<CEndPoint>> getPeers();
	CKademliaServer(bool isBootstrapNode, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm);
	~CKademliaServer();

	size_t getLastTimeCleanedUp();
	void setLastTimeCleanedUp(size_t timestamp = 0);

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