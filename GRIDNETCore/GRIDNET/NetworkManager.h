#pragma once
#include "stdafx.h"

#include "IManager.h"
#include <future>
#include <kademlia\session.hpp>
#include <kademlia\first_session.hpp>
#include "NetworkDevice.h"
#include "NetworkNode.h"
#include "enums.h"
#include <unordered_set>
#include "DataPubResult.h"
#include "NetTask.h"
#include "robin_hood.h"
#include <queue>
#include "msquic.h"

#define WIN32 1
typedef int UDTSOCKET;
#include <mutex>
#include "conversation.h"
class CBlockchainManager;
class CTransaction;
class CVerifiable;
class CDataPubResult;
class CDTIServer;
class CConnTracker;
class CUDTConversationsServer;
class CKademliaServer;
class CDataRouter;
class CWebSocketsServer;
class CCORSProxy;
class CWWWServer;
class CFileSystemServer;
class ThreadPool;
class CQUICConversationsServer;
#include "WFPFirewallModule.h"
/// <summary>
/// Represent the current overall state of the GRIDNET Core node.
/// The state is persistent across p2p conversations.
/// </summary>
namespace eNetPeerState
{
	enum eNetPeerState
	{
		syncing,
		operational,
		silent
	};
}
class CNetMsg;
class CBlock;
class NetworkDevice;
class CNetTask;
class CChatMsg;

class ConnectionAttemptTimestamps {

public:
	 ConnectionAttemptTimestamps(uint64_t QUICTimestamp = 0, uint64_t UDTTimestamp = 0);
	 ConnectionAttemptTimestamps(eTransportType::eTransportType transport, uint64_t timestamp);
	 uint64_t getLastQUICAttempt();
	 uint64_t getLastUDTAttempt();
	 void pingLastQUICAttempt();
	 void pingLastUDTAttempt();

private:
	std::mutex mFieldsGuardian;
	uint64_t mLastQUICAttempt;
	uint64_t mLastUDTAttempt;
	
};
/// <summary>
/// Network Manager is responsible for maintaining contact with other GRIDNET Core and light-node clients.
/// The connectivity is comprised of two sub-systems. The Kademlia node-discovery sub-system, as well as the UDT direct-P2P data exchange protocol.
/// In all cases, the GRIDNET ORM/DRM protocols are used for data exchange. These protocols reward intermediaries of a data exchange for each byte
/// they help to disseminate.
/// 
/// Direct communications are done with CConversation. (the class takes care of the client Socket within a thread of its own).
/// 
/// The Blockchain Managers maintains the current chainProof (sequence of blockHeaders), packedChainProof (sequence of encoded block headers), 
/// longestPath (sequence of blockIDs), as well as the representation of the current blockchain path available by accessing descendants of the mLeader block
/// (this is a kind of a block-cache which is pruned from time to time). The reason for maintaining a couple of lists is to enable for a better performance at the
/// cost of increased memory-usage, as compared to a naive approach with blocks stored solely by the RocksDB engine.
/// 
/// **BLOCK-EXCHANGE** - BEGIN
/// The basic work-flow of the networking sub-system is as follows:
/// 1) Discover nearby nodes through Kademlia
/// All the other communication takes place through the UDT protocol.
/// 2) Periodically query these nodes for the latest block they know of
///		2-1) Nodes respond with notification containing the latest known header producing a chain with largest cumulated PoW 
///			(they look at their local best known chainProof)
///		2-2) The querying node looks onto the response and decided by looking at the provided header if it might be interested in the given block
///		2-3) Since the repose is NOT trusted, if the node is interested in the block, it queries the node for a chainProof. It provides the 3 fall-back 
///     block blockIDs with which the chainProof should begin with. The last fall-back blockID is the ID of the Genesis block.
///		2-4) The other node responds with a chainProof. The chain-proof consists of a sequence of block headers. With which the local node can
///		verify the validity of the path and so of the to-be-downloaded blocks.
/// 3) the retrieved-best known chainProof is stored locally within the BlockchainManager.
/// 4) The Network Manager continuously attempts to update the currently best known chainProof and to download the corresponding known blocks.
/// 5) The Network Manager notifies other nodes about the block it has just mined by providing the header.
/// 6) Other nodes that see the header, update their best known chainProof.
/// **BLOCK-EXCHANGE** - END
/// 
/// </summary>
class CNetworkManager : public IManager, public std::enable_shared_from_this<CNetworkManager>
{

public:
	std::vector<std::string> getWhitelistedIPs();
	bool processUnsolicitedDatagram(uint64_t protocolType, std::vector<uint8_t> bytes, const CEndPoint &endpoint);
	bool getIsKademliaOperational();
	void setIsKademliaOperational(bool isIt = true);
	bool notifyAboutNewBlock(std::shared_ptr<CBlockHeader> header);
	bool isBlockCurrentlyBeingFetched(std::vector<uint8_t>& blockID);
	uint64_t getWasDataSeen(const std::vector<uint8_t>& hash ,const std::string& receivedFrom="", const bool& autoBan = true, const std::string& target = "", bool allowSingleOutgressLoop=false);
	void sawData(const std::vector<uint8_t>& hash, const std:: string& remoteAddress="");
	std::vector<std::string> getDomainIPs(const std::string& domain);
	std::vector<std::shared_ptr<CConversation>> getConversations(conversationFlags flags, bool fetchUDTConversations = true, bool onlyActive = true, bool connecting=false, bool fetchQUICConversations=true);

	void markChainProofForLeadBlockProcessed(std::vector<uint8_t> blockIID);
	bool wasChainProofForBlockProcessed(std::vector<uint8_t> blockIID);
	bool forgetReceivedChainProofs();
	uint64_t eliminateDuplicates( std::vector<std::shared_ptr<CConversation>> &dsmConvs, uint64_t allowedDuplicates=1, bool bePolite=true);
	
	uint64_t mLastDSMQualityReport;

	uint64_t getLastDSMQualityReportTimestamp();
	void pingDSMQualityReport();
	void handlePortInUse(unsigned short port, eFirewallProtocol::eFirewallProtocol protocol);
	bool initWebServer();
	bool isIPAddressLocal(std::vector<uint8_t> ip);
	void pingDatagramReceived(eNetworkSystem::eNetworkSystem netSystem, std::vector<uint8_t> address);
	void pingInvalidDatagramReceived(eNetworkSystem::eNetworkSystem netSystem, std::vector<uint8_t> address);
	void pingInvalidDatagramReceivinitializeWFPFirewall();
	void pingDatagramSent(eNetworkSystem::eNetworkSystem netSystem, std::vector<uint8_t> address);
	void clearCounters(uint64_t& unsolicitedRX, uint64_t& validKADRX, uint64_t& validUDTRX, uint64_t& invalidRX, uint64_t& KADTX, uint64_t& UDTTX, uint64_t& validQUICRX, uint64_t& QUICTX);
	void cleanKernelModeRules(bool wipeAll=true);
	bool getKeepChatExchangeAlive();
	bool getIsChatExchangeAlive();
	void setKeepChatExchangeAlive(bool isIt);
	void setIsChatExchangeAlive(bool isIt);
	std::shared_ptr<ThreadPool> getThreadPool();
	bool canProcessBlockNotification(const std::string & ip, bool markAnAttemptIfOK=true);
	bool getIsBootstrapNode(std::shared_ptr<CEndPoint> ep);
	bool getIsBootstrapNode(const std::string& IP);
	uint64_t updateLocalDNSBootstrapNodes(const std::vector<std::string>& newNodes);
    std::vector<std::string> getDNSBootstrapNodes();
	double estimateNetworkHashrate(double difficulty);
	uint64_t estimateMiningTime(double localHashrate, double difficulty);
private:
	// HTTP DDOS / Slowloris Mitigation - BEGIN
	std::mutex mConcurrentHTTPGuardian;
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t> mConcurrentHTTPConnections;
	// HTTP DDOS / Slowloris Mitigation - END

	// Windows Filtering Platform - BEGIN
	std::shared_ptr<CWFPFirewallModule> mWFPFirewall;
	std::mutex mWFPFirewallGuardian;
	std::atomic<bool> mWFPFirewallEnabled{ true };
	std::thread mWFPMaintenanceThread;
	std::atomic<bool> mKeepWFPMaintenanceAlive{ true };
	uint64_t mLastWFPCleanup{ 0 };
	uint64_t mLastWFPStatsReport{ 0 };
	// Windows Filtering Platform - END

	uint64_t mLastKernelModeFirewallCleanUp;
	uint64_t mLastTimeNetworkSpecificReported;
	uint64_t mLastTimeBootstrapDNSUpdate;
	std::vector<std::string> mDNSBootstrapNodes;
	std::shared_ptr<ThreadPool> mThreadPool;
	bool mKeepChatExchangeAlive;
	bool mChatExchangeAlive;
	uint64_t mLastControllerLoopRun;
	uint64_t mLastDSMLoopRun;
	std::mutex mNetworkModeGuardian;
	std::mutex mStatisticsGuardian;
	bool mNetworkTestMode;
	uint64_t mLastAnalysisStatusBarMode;
	uint64_t mLastAnalysisStatusBarModeSwitched;
	std::unordered_set<std::vector<uint8_t>> mReceivedChainproofsFor;//blocks for which chain-proofs have been already received
	std::mutex mBMGuardian;
	std::vector<std::vector<uint8_t>> mWhitelist;
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t> mIPsRedundantObjectsReceived;
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t> mSeenTXs;
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<uint64_t, uint64_t>> mBannedIPs; // IP, till_when, REJECT_SCREENS_SHOWN_COUNT
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t> mLastAllowedBlockNotificationPerIP;//IP <->timestamp
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t> mAttackReportedFromWhen;//IP <->timestamp
	//Network Analysis - BEGIN
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>,uint64_t>>  mUnsolicitedDatagramsReceived; // P_HASH->[IP,number of datagrams] WARNING: these might be say Kademlia datagrams but have not been processed yet by the KAD sub-system.
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> mValidKademliaDatagramsReceived; // P_HASH->[IP,number of datagrams]
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> mInvalidDatagramsReceived; // P_HASH->[IP,number of datagrams]; overall for all protocols.
	
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> mValidUDTDatagramsReceived; //P_HASH->[IP,number of datagrams]
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> mValidQUICDatagramsReceived; //P_HASH->[IP,number of datagrams]
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> mUDTDatagramsSent; // P_HASH->[IP,number of datagrams]
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> mQUICDatagramsSent; // P_HASH->[IP,number of datagrams]

	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> mKademliaDatagramsSent; // IP_HASH->[IP,number of datagrams]
	//Network Analysis - END

	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t> mBonusSlotsPerIP;
	robin_hood::unordered_map<std::vector<uint8_t>, // <- IP address hash (or other protocol layer identifier - network layer invariant, notice a payload entry at index 3 contains string(byte-vector) representation)
		std::tuple<uint64_t, uint64_t, uint64_t, std::vector<uint8_t>, uint64_t,uint64_t>> mConnectionsPerWindowIPs;//IP Address_Hash=> tuple{counter, lastAttemptTimestamp, http_counter, NETWORK_ID, last_seen, HTTP_API_REQUESTS}
	std::mutex mBannedIPsGuardian;
	std::mutex mSeenTXsGuardian;
	std::mutex mIPsRedundantObjectsReceivedGuardian;
	std::mutex mBonusSlotsGuardian;
	std::mutex mConnectionsPerWindowIPsGuardian;
	uint64_t mRouterCleanUpInterval;
	uint64_t mLastRouterCleanup;
	uint64_t mLastConversationsCleanUp;
	bool mKademlialNetworkingEnabled;
	bool mUDTNetworkingEnabled;
	bool mQUICNetworkingEnabled;
	bool mDTINetworkingEnabled;
	std::shared_ptr<CConnTracker> mConnTracker;
	bool mFileSystemNetworkingEnabled;
	bool mWebNetworkingEnabled;
	bool mWebSocketsNetworkingEnabled;
	std::vector <std::string> mLocalIPAddresses;
	std::set<CNetworkDevice, CompareNetworkDevice> mNetworkDevices;
	std::set<CNetworkNode, CompareNetworkDevice> mNetworkNodes;

	size_t mLocalPort;
	size_t mMaxNrOfIncConnecinftions;
	//std::vector<std::shared_ptr<CConversation>> mConversations;
	//std::mutex mConversationsGuardian;
	size_t mMaxNrOfOutConnections;
	size_t mConnectionTimout;
	
	std::thread mController,mChatThread;
	std::thread mKademliaInitThread; // To manage the Kademlia initialization thread
	bool initializeSubSystems();

	void initializeKademlia(std::shared_ptr<CKademliaServer>& mKademliaServer, bool mIsBootstrapNode,  std::shared_ptr<CNetworkManager> sharedThis);



	void mControllerThreadF();

	std::vector<uint8_t> getCachedAutoDetectedPublicIP();

	

	void exchangeChatMessagesThreadF();

	void DSMSyncThreadF();
	std::vector<CNetMsg*> mPendingMsgs;
	bool mIsBootstrapNode;;
	bool mKeepRunning = true;
	uint64_t mLastTimeBlocksScheduled;

	std::shared_ptr<CTools> mTools;
	std::string mPublicIP, mLocalIP;
	//bool mIsTestnet;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	std::shared_ptr<CBlockchainManager>mBlockchainManager;
	std::recursive_mutex mBootstrapPeersGuardian;
	bool mAllowOnlyBootstrapNodes;
	std::vector<std::shared_ptr<CEndPoint>> mBootstrapPeers, mForcedBootstrapPeers;
	std::future<std::error_code> mKademiliaMainLoopResult;
	eManagerStatus::eManagerStatus mStatus;
	eManagerStatus::eManagerStatus mStatusChange;

	std::mutex mBlockNotificationsGuardian;
	std::mutex mTasksQueueGuardian;
	std::mutex mFieldsGuardian;
	std::mutex mKademliaGuardian;
	std::mutex mCheckpointsGuardian;
	std::string  mFocusOnIP;
	uint64_t mLoopbackRX;
	uint64_t mLoopbackTX;
	uint64_t mEth0RX;
	uint64_t mEth0TX;
	std::vector<CNetTask> mNetTasks;
	//time-points

	size_t mLastTimeSyncProcedureStart;
	size_t mLastTimeSyncProcedurEnd;
	std::priority_queue<std::shared_ptr<CNetTask>, std::vector<std::shared_ptr<CNetTask>>, CCmpNetTasks> mTasks;
	void cleanupPeers();
	uint64_t lastGeneratedTaskID;
	std::shared_ptr<CDTIServer> mDTIServer;
	std::shared_ptr<CUDTConversationsServer> mUDTServer;
	std::shared_ptr<CQUICConversationsServer> mQUICServer;

	std::mutex mHeaderNotificationsQueueGuardian;
	std::queue<std::shared_ptr<CBlockHeader>> mHeaderNotificationsQueue;
	std::queue<std::shared_ptr<CChatMsg>> mChatNotificationQueue;
	std::mutex mChatNotificationQueueGuardian;
	std::shared_ptr<CKademliaServer> mKademliaServer;

	std::shared_ptr<CFileSystemServer> mFileSystemServer;
	std::shared_ptr<CWWWServer> mWWWServer;
	std::shared_ptr<CCORSProxy> mCORSProxy;
	
	std::shared_ptr<CWebSocketsServer> mWebSocketsServer;
	std::vector<uint8_t> mCachedAutoDetectedPublicIP;
	std::mutex mCachedAutoDetectedPublicIPGuardian;
	bool mInitializeDTIServer;
	bool mInitializeUDTServer;
	bool mInitializeQUICServer;
	bool mInitializeWebSocketsServer;
	bool mInitializeFileServer;
	bool mInitializeWebServer;
	bool mInitializeCORSProxy;
	bool mInitializeKademlia;
	std::mutex mLinkStatusGuardian;
	bool mPublicLinkEnabled;
	std::mutex mStatusChangeGuardian;
	std::mutex mLocalIPPoolGuardian;
	std::mutex mIPAttackingReportedAtGuardian;
	std::size_t mLastTimeChatMsgsRouted;
	void addLocalIPAddr(std::string ipAddr);
	std::shared_ptr<CChatMsg> dequeChatMsgNotification();

	std::shared_ptr<CChatMsg> getHeadChatMsgNotification();
	bool mFirewallKernelModeIntergation;
	std::vector<uint8_t> mLocalID;
	std::mutex mLocalIDGuardian;
	std::shared_ptr<CDataRouter> mRouter;
	std::mutex mBootstrapNodesGuardian;
	uint64_t mCurrentBootstrapNodeIndex;
	size_t mCheckpointsRefreshedTimestamp;
	std::vector<std::vector<uint8_t>> mCheckpoints;
	std::shared_ptr<CBlockHeader> dequeHeaderNotification();
	bool enqueHeaderNotification(std::shared_ptr<CBlockHeader> header);
	//bool abortNetTaskOnFaulure;
	uint64_t mMyCustomStatusBarID = 0;
	uint64_t mTryingToGetGenesisCPFromBootstrapNodesSince = 0;
	bool mConnectivityOptimal;
	uint64_t mLastConnectivityReportTimestamp;
	robin_hood::unordered_map<std::string, std::shared_ptr<ConnectionAttemptTimestamps>> mLastOutgoingConnectionAttempts;
	std::vector< eTransportType::eTransportType> mSyncTransports;
	uint64_t mLastConnectivityImprovementAttempt;
public:
	bool connectNode(const std::string& IP, eTransportType::eTransportType transportType);
	bool connectNode(std::shared_ptr<CEndPoint> endpoint, eTransportType::eTransportType transportType);
	void focusOnIP(const std::string& ip);
	bool getIsFocusingOnIP(const std::string& ip = "");
	bool getIsFocused();
	bool canEventLogAboutIP(const std::string& ip);
	void enableSyncTransport(eTransportType::eTransportType transport);
	void disableSyncTransport(eTransportType::eTransportType transport);

	bool isTransportEnabledForSync(eTransportType::eTransportType transport);

	bool getIsFirewallKernelModeIntegrationEnabled();
	void setIsFirewallKernelModeIntegrationEnabled(bool isIt=true);

	std::shared_ptr<CConnTracker> getConnTracker();
	uint64_t getLastConnectivityReportTimestamp();
	void pingLastConnectivityReportTimestamp();

	bool getIsConnectivityOptimal();
	void setIsConnectivityOptimal(bool isIt = true);
	void pingRetrievingCPFromBootstrapNodesSince();
	uint64_t getRetrievingCPFromBootstrapNodesSince();

	uint64_t getMyCustomStatusBarID();
	void initialize();
	void setIsNetworkTestMode(bool isIt);
	void getCounters(uint64_t& unsolicitedRX, uint64_t& validKADRX, uint64_t& validUDTRX, uint64_t& invalidRX, uint64_t& KADTX, uint64_t& UDTTX, uint64_t &validQUICRX, uint64_t &QUICTX);
	void clearStatistics();
	bool getIsNetworkTestMode();
	bool getHasForcedBootstrapNodesAvailable();
	bool enqueChatMsgNotification(std::shared_ptr<CChatMsg> header);

/**
 * @brief Generates and caches synchronization checkpoints referencing valid key blocks.
 *
 * This method constructs a set of synchronization checkpoints (each represented as a vector
 * of bytes) based on the current blockchain state. Checkpoints are generated at fixed depths
 * (e.g., depths 0, 10, and 100 if available) as well as at additional depths computed using a
 * fragmentation parameter. These checkpoints are intended to reference valid KEY-blocks only.
 *
 * The generated checkpoints are cached and reused for up to 180 seconds unless the cache is empty.
 *
 * The method returns false in any of the following cases:
 * - The heaviest key leader or current key leader is missing.
 * - The blockchain is empty (i.e., currentHeaviestKeyHeight is 0).
 * - A computed checkpoint (via the fragmentation loop) is either empty or is a duplicate of the previous checkpoint.
 * - No checkpoints are generated.
 *
 * Detailed log entries are produced for each failure or significant event.
 *
 * @param[out] checkpoints A vector to be populated with checkpoint vectors (each a vector of bytes) representing valid key block IDs.
 * @return true if checkpoints are successfully generated (or retrieved from cache) and assigned; false otherwise.
 */
	bool getCPCheckpoints(std::vector<std::vector<uint8_t>>& checkpoints);
	void forceSyncCheckpointsReferesh();
	std::shared_ptr<CKademliaServer> getKademliaServer();
	std::vector<std::shared_ptr<CConversation>> getAllConversations(bool includeWeb=true, bool includeUDT=true, bool includeQUIC= true, bool onlyAlive=false);
	bool getPublicLinkStatus();
	void setPublicLinkStatus(bool value);
	std::shared_ptr<CDataRouter> getRouter();
	std::string getInterfacesDescription(std::string newLine);
	uint64_t getRouterCleanupInterval();
	void setRouterCleanupInterval(uint64_t interval);
	uint64_t getLastRouterCleanup();
	void setLastRouterCleanup(uint64_t timestamp);
	std::vector<uint8_t> getLocalID();
	std::shared_ptr<CBlockchainManager> getBlockchainManager();
	eBlockchainMode::eBlockchainMode getBlockchainMode();
	std::shared_ptr<CUDTConversationsServer> getUDTServer();
	std::shared_ptr<CQUICConversationsServer> getQUICServer();
	std::shared_ptr<CCORSProxy> getWebProxy();
	std::shared_ptr<CWebSocketsServer> getWebSocketsServer();
	std::shared_ptr<CWWWServer> getWWWServer();
	std::shared_ptr<CFileSystemServer> getFileSystemServer();
	std::string getPublicIP();
	std::string getLocalIP();

/**
 * @brief Generates a comprehensive report of all network connections and attempts.
 *
 * This method compiles a detailed report of both incoming and outgoing connection data,
 * including information from mConnectionsPerWindowIPs, mLastOutgoingConnectionAttempts,
 * and the CConnTracker. It provides a holistic view of the network's current state and recent activity.
 *
 * The report includes the following information for each IP address:
 * - Last connection attempt timestamp
 * - Total connection duration within the current time window
 * - Connection count
 * - Remaining allowed connection time in the current window
 * - Time when connection limits will reset
 * - Current incoming connection status
 * - General and HTTP-specific connection attempt counts
 * - Active WebSocket , UDT  and QUIC connection counts
 * - Timestamp of last activity
 *
 * @param rows [out] A vector of string vectors, where each inner vector represents
 *                   a row in the report. The first row contains the column headers.
 *
 * @note This method is thread-safe. It uses a lock guard on mFieldsGuardian to ensure
 *       thread safety when accessing shared resources.
 *
 * @warning This method may take a non-trivial amount of time to execute, especially
 *          if there are many active or recent connections. It should not be called
 *          in performance-critical code paths.
 *
 * @see CConnTracker, CWebSocketsServer, CUDTConversationsServer
 */
	void generateComprehensiveConnectionReport(std::vector<std::vector<std::string>>& rows, bool useColors=true);
	std::string getConnectionStatusString(eIncomingConnectionResult::eIncomingConnectionResult status);
	void clearConnectionAttempts();
	void registerConversation(std::shared_ptr<CConversation> conversation);
	bool canAttemptConnection(const std::string& ip, eTransportType::eTransportType protocol, bool markConnectionAttempt=false);
	void storeConnectionAttempt(const std::string& ip, eTransportType::eTransportType protocol);
	bool isBonusSlotAllowedForIP(std::string ip);
	std::string getAnalysisReport(std::string newLine="\n");
	bool clearDataSet();
	void clearSeenTXs();
	bool whitelist(std::vector<uint8_t> address);
	bool unWhitelist(std::vector<uint8_t> address);
	bool getIsWhitelisted(std::vector<uint8_t> address);
	bool pardonPeer(std::vector<uint8_t> address);
	void getStateTable(std::vector<std::vector<std::string>>& rows, bool onlyBlocked=false, bool hideAddresses=false);
	bool banIP(std::string ip, size_t time=0, bool useKernelMode = true);
	bool hasResourcesForConnection(eTransportType::eTransportType transport);
	void pingBlockedAccessNotified(const std::string& ip);
	bool canShowBlockedAccessScreen(const std::string& ip);
	/**
	* @brief Validates whether a connection from a given IP address is allowed.
	*
	* This method implements comprehensive security checks including rate limiting,
	* connection counting, automatic banning for suspected attacks, and resource availability.
	* It supports multiple connection types (HTTP, WebSocket, UDT, QUIC) with appropriate
	* thresholds and tracking mechanisms for each.
	*
	* @param ip The IP address attempting to connect (IPv4 format)
	* @param autoBan Whether to automatically ban IPs that exceed rate limits
	* @param transport The type of transport/connection being attempted
	* @param sendNotifications Whether to send notifications to existing connections about blocks
	* @param incrementCounters Whether to increment connection counters (default: true).
	*                         Set to false when only checking without recording a new attempt.
	*
	* @return eIncomingConnectionResult indicating whether the connection is allowed:
	*         - allowed: Connection permitted
	*         - limitReached: Connection/rate limits exceeded
	*         - DOS: Denial of Service detected, IP banned
	*         - insufficientResources: Server lacks resources
	*         - onlyBootstrapNodesAllowed: Non-bootstrap node rejected in restricted mode
	*
	* @note This method is thread-safe and uses internal mutex locks.
	* @note Local and whitelisted IPs bypass all security checks.
	* @note The incrementCounters parameter prevents double-counting when the same
	*       connection triggers multiple checks (e.g., HTTP upgrade to WebSocket).
	*
	* @see banIP(), isIPAddressLocal(), getIsWhitelisted(), hasResourcesForConnection()
	*/
	eIncomingConnectionResult::eIncomingConnectionResult isConnectionAllowed(
		std::string ip,
		bool autoBan=true,
		eTransportType::eTransportType transport = eTransportType::UDT,
		bool sendNotifications = false,
		bool incrementCounters = true
	);

	bool incrementWebSocketConnection(const std::string& ip);
	void clearHTTPConnectionCounter(const std::string& ip);
	void decrementWebSocketConnection(const std::string& ip);
	std::shared_ptr<CDTIServer> getDTIServer();
	void decrementHTTPConnection(const std::string& ip);
	uint64_t getHTTPConnectionsCount(const std::string& ip);
	bool incrementHTTPConnection(const std::string& ip);
	uint64_t getConnectionAttemptsFrom(std::string& IP, eTransportType::eTransportType transport, uint64_t seconds=180);
	std::vector <std::string>  getLocalIPAdresses();
	uint64_t getLastAttacksReportedFrom(const std::string& ip, bool reportIt=true);
	bool needsRouting(std::shared_ptr<CNetMsg> msg, std::shared_ptr<CEndPoint> receivedby);
	void performWFPMaintenance();
	void reportWFPStatistics();
	~CNetworkManager();
	std::mutex mNetworkStatsGuardian;

	uint64_t getEth0RX();
	uint64_t getEth0TX();
	uint64_t getLoopbackRX();
	uint64_t getLoopbackTX();
	void incEth0RX(uint64_t value);
	void incEth0TX(uint64_t value);
	void incLoopbackRX(uint64_t value);
	void incLoopbackTX(uint64_t value);
	size_t getLastTimeChatMsgsRouted();
	void pingLastTimeChatMsgsRouted();

	// Public WFP Firewall Methods  - BEGIN
	bool initializeWFPFirewall();
	void shutdownWFPFirewall();
	std::shared_ptr<CWFPFirewallModule> getWFPFirewall();
	void setWFPFirewallEnabled(bool enabled);
	bool isWFPFirewallEnabled() const;
	void processIncomingPacketWFP(const std::string& sourceIP, uint16_t protocol, 
	                              uint16_t destPort, uint32_t packetSize);
	void getWFPStatistics(std::vector<std::vector<std::string>>& rows);
	void configureWFPThresholds(const DosThresholds& thresholds);
	// Public WFP Firewall Methods  - END

	uint64_t getLastTimeControllerThreadRun();
	void pingLastTimeControllerThreadRun();
	bool getMaintainConnectivityOnlyWithBootstrapNodes();
	uint64_t getLastTimeDSMControllerThreadRun();
	std::vector<std::shared_ptr<CEndPoint>> getBootstrapNodes(bool fetchFromDNS=false, bool onlyEffective=true);
	void pingDNSBooststrapUpdate();
	uint64_t getDNSBooststrapUpdateTimestamp();
	uint64_t getLastConversationsCleanUp();
	void pingLastConversationsCleanUp();
	uint64_t getLastKernelModeFirewallCleanUpTimestamp();
	void pingLastKernelModeFirewallCleanUpTimestamp();
	void pingLastConnectivityImprovement();
	uint64_t getLastConnectivityImprovement();
	uint64_t getLastNetworkSpecifcsReported();
	void pingLastNetworkSpecifcsReported();
	bool getAllowConnectivityOnlyWithBootstrapNodes();
	void setAllowConnectivityOnlyWithBootstrapNodes(bool doIt = true);
	void pingLastTimeDSMControllerThreadRun();
	CNetworkManager(std::shared_ptr<CBlockchainManager> blockchainManager, bool isBootStrapNode = false, bool initializeDTIServer = false, bool initializeUDTServer = false, bool initializeKademlia = false, bool initializeFileServer = false,
		bool initializeWebServer = false, bool initializeWebSocketsServer = false, bool initCORSproxy = false, bool initializeQUICServer=false, bool useQUICForSync=true, bool useUDTForSync = true);
	


	void reloadBootstrapNodes();

	bool getIsBootstrapNode();

	bool addAndSetBootstrapNode(std::string IP);

	std::shared_ptr<CEndPoint> getBootstrapNode(bool tossNewOne=false);
	std::shared_ptr<CNetTask> createTask(eNetTaskType::eNetTaskType  type, uint64_t priority = 1, bool registerTask = false, std::vector<uint8_t>data = std::vector<uint8_t>());
	std::string autoDetectPublicIP();
	std::string autoDetectLocalIP();
	bool registerTask(std::shared_ptr< CNetTask> task);
	std::shared_ptr<CNetTask> getCurrentTask(bool deqeue = true);

	eNetTaskProcessingResult::eNetTaskProcessingResult processTask(std::shared_ptr<CNetTask> task);
	void dequeTask();
	void processNetTasks();
	std::shared_ptr<CNetTask> getTaskByMetaReqID(uint64_t id);
	//std::shared_ptr<CNetworkManager> getInstance();
	std::shared_ptr< CNetTask> getTaskByID(uint64_t id);
	std::shared_ptr<CTools> getTools();
	void setInitializeDTIServer(bool doIt = true);
	void setInitializeUDTServer(bool doIt);
	void setInitializeQUICServer(bool doIt);
	void setInitializeFileServer(bool doIt);
	void setInitializeWebSocketsServer(bool doIt);
	void setInitializeWebServer(bool doIt);
	void setInitializeCORSProxy(bool doIt);
	bool getInitializeDTIServer();
	bool getInitializeUDTServer();
	bool getInitializeQUICServer();
	bool getInitializeFileServer();
	bool getInitializeWebServer();
	bool getInitializeCORSProxy();
	bool getInitializeWebSocketsServer();
	bool getInitializeKademlia();
	static enum dataQueryResultStatus { dataDelivered, noData, invalidQuery, staleData, otherQueryResult };


	typedef struct dataRequestResult {
		CNetworkManager::dataQueryResultStatus status;
		std::vector<uint8_t> providersID;
		std::vector<uint8_t> sig;
		std::vector<uint8_t> info;
	} dataQueryResult;



	//data publishing - notifications (blocks) and uploads(transactions)
	CDataPubResult publishBlock(std::shared_ptr<CBlock> block); //only the header
	CDataPubResult publishTransaction(CTransaction transaction);//transaction's body
	CDataPubResult publishVerifiable(CVerifiable transaction);//verifiable's body

	CDataPubResult publishLeaderInfo(CVerifiable transaction);//only the header

	//emergency stuff - requires GRIDNET Team's private key inside of the Sec field.(signature verified by other peers)
	//the aim of these function IS NOT to provide centralization of the network, but rather to improve security and rigidness during
	// the initial phase. In the long run, most these functions are to be disabled. Note that these functions do not allow
	//the GRIDNET Team to manipulate with specific transactions etc; probably the most influential command is
	//revertToBlock() which cases the entire network to revert to a specific block height (this would be used in case a major
	//bug is discovered).
	CDataPubResult publishUpdateAvailable(CVerifiable transaction);//verifiable's body
	CDataPubResult revertToBlock(std::vector<uint8_t> blockID);//verifiable's body
	CDataPubResult banIPs(std::vector<uint8_t> blockID);

	bool checkForUpdate();
	std::vector<uint8_t> fetchLatestVersion();

	// Inherited via IManager
	virtual void stop() override;

	virtual void pause() override;

	virtual void resume() override;

	virtual eManagerStatus::eManagerStatus getStatus() override;

	virtual void setStatus(eManagerStatus::eManagerStatus status) override;


	// Inherited via IManager
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status) override;

	virtual eManagerStatus::eManagerStatus getRequestedStatusChange() override;

};
