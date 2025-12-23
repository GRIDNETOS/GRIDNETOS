#include "KademliaServer.h"
#include "CGlobalSecSettings.h"
#include "BlockchainManager.h"
#include "NetworkManager.h"
#include "DataRouter.h"

#define FORGET_PEERS_AFTER_NOT_SEEN_FOR_SEC  1800 //30 minutes

#include "EEndPoint.h"
#include <kademlia\engine.hpp>
kademlia::detail::engine<boost::asio::ip::udp::socket>* CKademliaServer::sActiveKadEngine;
std::mutex  CKademliaServer::sActiveKadEngineGuardian;

void CKademliaServer::setCurrentBootstrapNode(std::shared_ptr<CEndPoint> node)
{
	std::lock_guard<std::mutex> lock(mCurrentBootstrapNodeGuardian);
	getTools()->logEvent(getTools()->getColoredString("[Kademlia]:",eColor::lightCyan)+" Setting current bootstrap node to : " + node->getDescription(),eLogEntryCategory::network,5);
	mCurrentBootstrapNode = node;
}

std::shared_ptr<CEndPoint> CKademliaServer::getCurrentBootstrapNode()
{
	std::lock_guard<std::mutex> lock(mCurrentBootstrapNodeGuardian);
	return mCurrentBootstrapNode;
}

void CKademliaServer::setActiveKadEngine(kademlia::detail::engine<boost::asio::ip::udp::socket>* engine)
{
	std::lock_guard<std::mutex> lock(sActiveKadEngineGuardian);
	sActiveKadEngine = engine;
}

kademlia::detail::engine<boost::asio::ip::udp::socket>* CKademliaServer::getActiveKadEngine()
{
	std::lock_guard<std::mutex> lock(sActiveKadEngineGuardian);
	return sActiveKadEngine;
}



bool CKademliaServer::processKademliaDatagram(std::vector<uint8_t> bytes, const CEndPoint& endpoint)
{
	//std::lock_guard<std::mutex> lock(mFieldsGuardian);

	kademlia::detail::ip_endpoint point;

	std::vector<uint8_t> ad = endpoint.getAddress();
	std::string ipStr = std::string(ad.begin(), ad.end());
	point.address_ =  boost::asio::ip::address::from_string(ipStr);
	point.port_ = endpoint.getPort();//EXTREMELY IMPORTANT: use source port reported within the packet as the source might be behind a NAT. i.e. do not use ->  CGlobalSecSettings::getDefaultPortNrPeerDiscovery(eBlockchainMode::TestNet);
	//when a packet is sent to the source port, the NAT (if present on the data-path), it would perform destination-port translation and in the end the destination port number would end-up being 443 (as seen by the client 
	//which is behind a NAT. Thus two mechanisms are possibly here at plat - source address/port number translation and destination address/port number translation - if nat present.
	//if NAT is not present, then the source port number would correspond to port 443 anyway. We can thus recognize whether  a client is behind a NAT simply by looking if the source port number is equal to 443 or not.
	kademlia::detail::engine<boost::asio::ip::udp::socket> * engine = CKademliaServer::getActiveKadEngine();

	if (engine)
	{
		engine->handle_new_messageEx(bytes, point);
		return true;
	}


	return false;
}

kademlia::first_session* CKademliaServer::getBootstrapSession()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mKademliaBootstrapSession;
}

kademlia::session* CKademliaServer::getSession()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mKademliaSession;
}

/// <summary>
/// Main Thread of the Kademlia-based peer-discovery sub-system.
/// </summary>
void CKademliaServer::KademliaServerThreadF()
{
	getTools()->SetThreadName("Kademlia Server");

	//accept an incoming connection
	while (getStatus() == eManagerStatus::eManagerStatus::running)
	{
		cleanupPeers();
		refreshPeers();
		Sleep(10000);
	}
}

bool CKademliaServer::getIsBootstreapNode()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);;
	return mIsBootstrapNode;

}
/// <summary>
/// Initialized Kademlia Server.
/// </summary>
/// <returns></returns>
bool CKademliaServer::initializeServer()
{
	bool valueFromStorage = false;
	std::shared_ptr<CEndPoint>  node= mNetworkManager->getBootstrapNode();

	if (!node)
	{
		getTools()->writeLine("Unable to initialize due to no Bootstrap nodes provided.");
		return false;
	}
	setCurrentBootstrapNode(node);
	


	kademlia::endpoint const initial_peer(getTools()->bytesToString(node->getAddress()), static_cast<uint16_t>(node->getPort()));
	//start UDT begin
	// Automatically start up and clean up UDT module.

	kademlia::endpoint const seedPeerIPv4{ "0.0.0.0",  static_cast<uint16_t>(node->getPort()) };
	kademlia::endpoint const seedPeerIPv6{ "::1", static_cast<uint16_t>(node->getPort()) };
	std::string minersID = mBlockchainManager->getSettings()->getMinersKeyChainID();
	CKeyChain chain(false);
	assertGN(mBlockchainManager->getSettings()->getCurrentKeyChain(chain, false));
	std::vector<uint8_t> netID = CTools::getInstance()->genRandomVector(32);//chain.getPubKey(); <- this is no more to allow for multiple nodes with same identity.
	assertGN(netID.size() == 32);
	std::shared_ptr<CTools> tools = CTools::getInstance();
	if (getIsBootstreapNode())
	{
		getTools()->writeLine("I'm a Peer-Discovery bootstrap-node; hello Devs!");
		getTools()->writeLine("Bootstrapping Peer Discovery Service at " +tools->getColoredString( mIPv4Endpoint.address() + ":" + mIPv4Endpoint.service(), eColor::lightCyan));
		mKademliaBootstrapSession = new kademlia::first_session(mNetworkManager, mIPv4Endpoint);
		getTools()->writeLine("Launching the Kademlia-thread");
		mKademiliaMainLoopResult = std::async(std::launch::async
			, &kademlia::first_session::run, mKademliaBootstrapSession);
	}
	else
	{
		getTools()->writeLine("Initializing Kademlia-based node discovery..");
		getTools()->writeLine("Bootstrapping Peer Discovery Service at " + tools->getColoredString(mIPv4Endpoint.address() + ":" + mIPv4Endpoint.service(), eColor::lightCyan));
		std::stringstream ss;
		ss << "Bootstrap peer is set to: " << initial_peer;
		getTools()->writeLine(ss.str());
		ss.str(std::string());
	
		mKademliaSession = new kademlia::session(mNetworkManager, initial_peer, netID, mIPv4Endpoint);

		getTools()->writeLine("Launching the Kademlia thread..");
		mKademiliaMainLoopResult = std::async(std::launch::async
			, &kademlia::session::run, mKademliaSession);

		uint32_t discTime = 60;
		getTools()->writeLine("I'll wait for " + std::to_string(discTime) + " (sec) for peers' discovery..");
		uint32_t previousPeerCount = 0;
		std::lock_guard<std::recursive_mutex> lock(mPeersGuardian);
		while (discTime > 0)
		{
			cleanupPeers();
			refreshPeers();
			if (previousPeerCount != mKnownPeers.size())
			{
				previousPeerCount = mKnownPeers.size();
				getTools()->writeLine("Nr. of discovered peers: " + std::to_string(previousPeerCount));

			}
			discTime--;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		getTools()->writeLine("Total number of discovered GRIDNET Core Peers: " + std::to_string(mKnownPeers.size()));
	}

	//let us not forget about an Overwatch, shall we..
	mControllerThread = std::thread(&CKademliaServer::mControllerThreadF, this);

	return true;
}

std::shared_ptr<CNetworkManager> CKademliaServer::getNetworkManager()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	
	return mNetworkManager;
}

/// <summary>
/// Carries our a clen-up procedure.
/// MUST NOT be called from the outside.
/// </summary>
/// <returns></returns>
uint64_t CKademliaServer::cleanupPeers()
{

	size_t currentTime = std::time(0);
	uint64_t cleanedUpCount = 0;
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::recursive_mutex> lock(mPeersGuardian);
	

	auto it = mKnownPeers.begin();
	while (it != mKnownPeers.end())
	{
		if ((currentTime - ((*it))->getLastTimeSeen()) > FORGET_PEERS_AFTER_NOT_SEEN_FOR_SEC)
		{
			tools->writeLine(tools->getColoredString("Forgetting about peer ",eColor::orange) + tools->bytesToString((*it)->getAddress()) + ", as it was not seen recently..");
			it = mKnownPeers.erase(it);
			cleanedUpCount++;
		}
		else
		{
			++it;
		}

	}
	return cleanedUpCount;
}


/// <summary>
/// Initiates the cleaning-up procedure.
/// </summary>
/// <param name="forceKillAllSessions"></param>
/// <returns></returns>
uint64_t CKademliaServer::doCleaningUp(bool forceKillAllSessions)
{
    return cleanupPeers();
}


/// <summary>
/// Refresh the currently known peers.
/// </summary>
void CKademliaServer::refreshPeers()
{
	//Local Variables - BEGIN
	std::shared_ptr<CDataRouter> router = getNetworkManager()->getRouter();
	std::vector<std::shared_ptr<CEndPoint>> discoveredPeers;
	kademlia::session* session = getSession();
	std::shared_ptr<CTools> tools = getTools();
	kademlia::first_session* bootstrapSession = getBootstrapSession();
	std::vector<uint8_t> currentIP;
	assertGN(session != nullptr || bootstrapSession != NULL);

	if (getIsBootstreapNode())
		discoveredPeers = bootstrapSession->getPeers();
	else
		discoveredPeers = session->getPeers();

	//Local Variables - END

	//Operational Logic - BEGIN
	std::lock_guard<std::recursive_mutex> lock(mPeersGuardian);

	for (int i = 0; i < discoveredPeers.size(); i++)
	{
		//update the main, global routing table with the peer (entry would be either refreshed or created).
		router->updateRT(discoveredPeers[i], discoveredPeers[i], 0, eRouteKowledgeSource::Kademlia);
		
		currentIP = discoveredPeers[i]->getAddress();

		if (!findPeerByIP(tools->bytesToString(currentIP), true))
		{
			tools->logEvent(tools->getColoredString("New Peer discovered: ", eColor::lightGreen) + tools->getColoredString(tools->bytesToString(currentIP),eColor::lightCyan) + " !", eLogEntryCategory::network, eLogEntryType::notification);
			//update the global routing table, treat Kademlia peers as from within a flat-network.
			//getNetworkManager()->getRouter()->updateRT(discoveredPeers[i], discoveredPeers[i], eRouteKowledgeSource::Kademlia);
			mKnownPeers.push_back(discoveredPeers[i]);
		}
	}
	tools->logEvent(tools->getColoredString("[Kademlia: Reports]",eColor::lightCyan)+
		" - aware of " + tools->getColoredString(std::to_string(discoveredPeers.size() + 1) + " nodes.", eColor::lightGreen),  eLogEntryCategory::network, 1, eLogEntryType::notification);
	//Operational Logic - END
}

/// <summary>
/// Retrieve the currently known peers.
/// </summary>
/// <returns></returns>
std::vector<std::shared_ptr<CEndPoint>> CKademliaServer::getPeers()
{
	std::lock_guard<std::recursive_mutex> lock(mPeersGuardian);
	std::vector<std::shared_ptr<CEndPoint>> toRet;
	//perform a deep copy

	for (uint64_t i = 0; i < mKnownPeers.size(); i++)
	{
		toRet.push_back(mKnownPeers[i]);
	}
	return toRet;
}


std::shared_ptr<CTools> CKademliaServer::getTools()
{
	std::lock_guard<std::mutex> lock(mToolsGuardian);
	return mTools;
}

/// <summary>
/// Retrieve a peer, provided its IP address.
/// </summary>
/// <param name="IP"></param>
/// <param name="updateLastTimeSeen"></param>
/// <returns></returns>
std::shared_ptr<CEndPoint> CKademliaServer::findPeerByIP(std::string IP, bool updateLastTimeSeen)
{
	std::lock_guard<std::recursive_mutex> lock(mPeersGuardian);
	for (int i = 0; i < mKnownPeers.size(); i++)
	{
		if ((getTools()->bytesToString(mKnownPeers[i]->getAddress()).compare(IP) == 0))
		{
			mKnownPeers[i]->ping();
			return mKnownPeers[i];
		}
	}
	return nullptr;
}


/// <summary>
/// Constructor of the Kademlia-protocol Agent.
/// </summary>
/// <param name="isBootstrapNode"></param>
/// <param name="bm"></param>
/// <param name="nm"></param>
CKademliaServer::CKademliaServer(bool isBootstrapNode, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm)
{//the below cannot be "0.0.0.0",at least on windows. There would be a deadlock situation.
	mIPv4Endpoint = kademlia::endpoint{ "0.0.0.0",  static_cast<uint16_t>(443) };//the socket is NOT supposed to receive data
	////directly from the network, data packets are to traverse through the UDT socket instead. the socket IS used for outgress datagrams.
	mBlockchainMode = bm->getMode();
	mShutdown = false;
	mStatusChange = eManagerStatus::initial;
	mStatus = eManagerStatus::initial;
	mKademliaSession = nullptr;
	mKademliaBootstrapSession = nullptr;
	mLastTimeCleaned = 0;
	mMaxSessionInactivity = 0;
	mMaxSessions = 500;
	mBlockchainManager = bm;
	mNetworkManager = nm;
	mIsBootstrapNode = isBootstrapNode;
	mTools = bm->getTools();
	mMaxWarningsCount = 10;
	mLastTimeCleaned = 0;
	mMaxSessionInactivity = 60;
	mMaxWarningsCount = 3;
	mMaxSessions = 1000;
}

CKademliaServer::~CKademliaServer()
{
	if (!mIsBootstrapNode && mKademliaSession != NULL)
	{
		mKademliaSession->abort();
		auto failure = mKademiliaMainLoopResult.get();
	}
	else if (mIsBootstrapNode && mKademliaBootstrapSession != NULL)
	{
		mKademliaBootstrapSession->abort();
		auto failure = mKademiliaMainLoopResult.get();
	}

	if(mKademliaServerThread.joinable())
	mKademliaServerThread.join();

	if (mControllerThread.joinable())
		mControllerThread.join();
}

size_t CKademliaServer::getLastTimeCleanedUp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastTimeCleaned;
}

void CKademliaServer::setLastTimeCleanedUp(size_t timestamp)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastTimeCleaned = timestamp != 0 ? (timestamp) : (getTools()->getTime());
}

/// <summary>
/// Initializes the Kademlia sub-system.
/// </summary>
/// <returns></returns>
bool CKademliaServer::initialize()
{
	return initializeServer();
}

/// <summary>
/// Stops the Kademlia Agent.
/// </summary>
void CKademliaServer::stop()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	//firs ensure no new session come-in
	if (getStatus() == eManagerStatus::eManagerStatus::stopped)
		return;
	getTools()->writeLine("Aborting Kademlia main loop.");
	if (!mIsBootstrapNode && mKademliaSession != NULL)
	{
		mKademliaSession->abort();
		auto failure = mKademiliaMainLoopResult.get();
		mKademliaSession = nullptr;
	}
	else if (mIsBootstrapNode && mKademliaBootstrapSession != NULL)
	{
		mKademliaBootstrapSession->abort();
		auto failure = mKademiliaMainLoopResult.get();
		mKademliaBootstrapSession = nullptr;
	}
	mStatusChange = eManagerStatus::eManagerStatus::stopped;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::stopped)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CKademliaServer::mControllerThreadF, this);

	while (getStatus() != eManagerStatus::eManagerStatus::stopped && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	if (mControllerThread.joinable() && std::this_thread::get_id() != mControllerThread.get_id())
		mControllerThread.join();

	//then clean-up data structures / free memory

	doCleaningUp(true);//kill all active ones

	getTools()->writeLine("Kademlia Server killed;");
}

/// <summary>
/// Pauses the Kademlia peer-discovery service.
/// </summary>
void CKademliaServer::pause()
{
	if (getStatus() == eManagerStatus::eManagerStatus::paused)
		return;

	mStatusChange = eManagerStatus::eManagerStatus::paused;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::paused)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CKademliaServer::mControllerThreadF, this);

	while (getStatus() != eManagerStatus::eManagerStatus::paused && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	doCleaningUp(true);

	getTools()->writeLine("Kademlia Server paused;");
}

/// <summary>
/// Resumes the Kademlia-based peer-discovery apparatus.
/// </summary>
void CKademliaServer::resume()
{
	if (getStatus() == eManagerStatus::eManagerStatus::running)
		return;
	mStatusChange = eManagerStatus::eManagerStatus::running;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::running)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CKademliaServer::mControllerThreadF, this);


	while (getStatus() != eManagerStatus::eManagerStatus::running && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	getTools()->writeLine("Kademlia Server resumed;");
}

/// <summary>
/// Main controller thread of the Kademlia-based, decentralized peer-discovery service.
/// </summary>
void CKademliaServer::mControllerThreadF()
{
	std::string tName = "Kademlia-Server Controller";
	getTools()->SetThreadName(tName.data());
	setStatus(eManagerStatus::eManagerStatus::running);
	bool wasPaused = false;
	if (mStatus != eManagerStatus::eManagerStatus::running)
		setStatus(eManagerStatus::eManagerStatus::running);
	uint64_t justCommitedFromHeaviestChainProof = 0;

	mKademliaServerThread = std::thread(&CKademliaServer::KademliaServerThreadF, this);

	while (mStatus != eManagerStatus::eManagerStatus::stopped)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		wasPaused = false;
		doCleaningUp();
		if (mStatusChange == eManagerStatus::eManagerStatus::paused)
		{
			setStatus(eManagerStatus::eManagerStatus::paused);
			mStatusChange = eManagerStatus::eManagerStatus::initial;

			while (mStatusChange == eManagerStatus::eManagerStatus::initial)
			{

				if (!wasPaused)
				{
					getTools()->writeLine("My thread operations were freezed. Halting..");
					if (mKademliaServerThread.native_handle() != 0)
					{
						while (!mKademliaServerThread.joinable())
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
						mKademliaServerThread.join();
					}

					wasPaused = true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		if (wasPaused)
		{
			getTools()->writeLine("My thread operations are now resumed. Commencing further..");
			mKademliaServerThread = std::thread(&CKademliaServer::KademliaServerThreadF, this);
			mStatus = eManagerStatus::eManagerStatus::running;
		}


		if (mStatusChange == eManagerStatus::eManagerStatus::stopped)
		{
			mStatusChange = eManagerStatus::eManagerStatus::initial;
			mStatus = eManagerStatus::eManagerStatus::stopped;

		}

	}
	doCleaningUp(true);//kill all the connections; do not allow for Zombie-connections
	setStatus(eManagerStatus::eManagerStatus::stopped);
}

/// <summary>
/// Retrieves service status.
/// </summary>
/// <returns></returns>
eManagerStatus::eManagerStatus CKademliaServer::getStatus()
{
	std::lock_guard<std::recursive_mutex> lock(mStatusGuardian);
	return mStatus;
}


/// <summary>
/// Sets the current state.
/// Must not be called from the outside.
/// </summary>
/// <param name="status"></param>
void CKademliaServer::setStatus(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::recursive_mutex> lock(mStatusGuardian);
	if (mStatus == status)
		return;
	mStatus = status;
	switch (status)
	{
	case eManagerStatus::eManagerStatus::running:

		getTools()->writeLine("I'm now running");
		break;
	case eManagerStatus::eManagerStatus::paused:
		getTools()->writeLine(" is now paused");
		break;
	case eManagerStatus::eManagerStatus::stopped:
		getTools()->writeLine("I'm now stopped");
		break;
	default:
		getTools()->writeLine("I'm now in an unknown state;/");
		break;
	}
}

/// <summary>
/// Asks for a transision to a given state.
/// </summary>
/// <param name="status"></param>
void CKademliaServer::requestStatusChange(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	mStatusChange = status;
}

/// <summary>
/// Requests the currently requested state-transition (which MIGHT not yet be effective).
/// </summary>
/// <returns></returns>
eManagerStatus::eManagerStatus CKademliaServer::getRequestedStatusChange()
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	return mStatusChange;
}
