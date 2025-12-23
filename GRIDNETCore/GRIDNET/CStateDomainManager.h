#pragma once
#include "SolidStorage.h"
#include "TrieDB.h"
#include "StateDomain.h"
#include "IdentityToken.h"
#include "robin_hood.h"
#include "enums.h"
/// <summary>
/// The State Domain Manger provides state domain managment capabilities.
/// It takes the Global State DB and an option whether to use the test database or the live one.
/// 
/// Each member method takes an optional perspectiveID (the StateDB root node hash) argument.
/// 
/// If no perspectiveID is specified, the latest version of the Global State DB will be used.
/// (CBlockchainManager contains pointer to the same state DB).
/// 
/// Naturally, a change to a StateDomain results in a change to the Root Hash of the global Trie.
/// Thus, each method returns which add or modifies a State Domain, takes perspectiveID as a reference,
/// and returns the RootHash of the GlobalTrieDB just after the change occured.
/// 
/// Each member method is secured by a Mutex. Additionally each method of the TrieDB is protected by a recursive mutex.
/// </summary>
class CTransactionManager;
class CDomainDesc;
class CSearchResults;
class CSearchFilter;
/**
* @brief The CStateDomainManager provides thread-safe access to state domains within a Merkle Patricia Trie.
*        This documentation details proper usage patterns for accessing the manager, especially important
*        in the context of the persistent state DB which maintains a complete in-memory trie.
*
* @details The Persistent State DB keeps a complete Merkle Patricia Trie in RAM at all times, providing
*          fast access to historical states. It employs a double-buffering mechanism where:
*          1. Main buffer (mPersistentStateDB) - actively used for queries
*          2. Update buffer (mPersistentStateDBDouble) - used during state updates
*
*          This design allows for atomic updates while maintaining availability. However, it requires careful
*          synchronization to prevent:
*          - Access to inconsistent state during updates
*          - Updates while domains are being read
*          - Premature deletion of state data
*
* @section Usage Checking Manager Readiness
* First, always verify the manager is ready to handle requests:
*
* @code
* auto manager = transactionManager->getPersistentStateDomainManager();
* if (!manager->isReady()) {
*     // Manager not ready - could be updating, not initialized, etc.
*     return;
* }
* @endcode
*
* @section Usage Safe Access Patterns
*
* @subsection Pattern1 Using RAII Lock Guard (Preferred)
* Safest approach, automatically handles release:
* @code
* auto manager = transactionManager->getPersistentStateDomainManager();
* if (!manager->isReady()) return;
*
* {
*     std::lock_guard<CStateDomainManager> lock(*manager);
*     // Safe to access manager and its DB here
*     auto domain = manager->findByID(someID);
*     // Process domain...
* } // Lock automatically released here
* @endcode
*
* @subsection Pattern2 Manual Notification
* For more complex scenarios requiring manual control:
* @code
* auto manager = transactionManager->getPersistentStateDomainManager();
* if (!manager->isReady()) return;
*
* manager->notifyDataInUse();
* try {
*     // Safe to use manager and its DB here
*     auto domain = manager->findByID(someID);
*     // Process domain...
*
*     manager->notifyDataReleased();
* }
* catch(...) {
*     manager->notifyDataReleased(); // Ensure release on error
*     throw;
* }
* @endcode
*
* @section Internals Synchronization Mechanism
* The manager uses a ReverseSemaphore (through CTrieDB's DataGuardian) to track active users.
* During updates:
* 1. Update process waits for all users to finish (counter reaches 0)
* 2. Updates occur in double buffer
* 3. Pointers atomically swapped
* 4. Old buffer becomes new update buffer
*
* This ensures:
* - Zero downtime for reads
* - Consistent state views
* - No data loss during updates
* - Thread-safe access
*
* @warning Never hold locks longer than necessary. Acquire lock, get required data,
*          release lock, then process data. This allows other threads to access the DB
*          and prevents blocking state updates unnecessarily.
*
* @note The persistent state DB is meant to maintain a complete state trie in memory
*       at all times. This enables fast historical queries but requires careful memory
*       management and synchronization to prevent corruption or inconsistent views.
*/
class CStateDomainManager : public std::enable_shared_from_this<CStateDomainManager>
{
public://TODO allow for accesing StateDomain from the perspective of a specific StateDb root node.
	//i.e. so to be able to access prior states of a given StateDomain .

	
	CStateDomainManager(CTrieDB * globalStateTrie, eBlockchainMode::eBlockchainMode blockchainMode, CTransactionManager *tManager = NULL);
	std::vector<std::shared_ptr<CDomainDesc>> getAllCachedDomainMetadata() const;
	size_t getAllCachedDomainMetadataCount() const;
	BigInt analyzeTotalDomainBalances();
	~CStateDomainManager();
	CStateDomainManager(const CStateDomainManager& sibling);
	void setKnownStateDomainIDs(std::vector<std::vector<uint8_t>> IDs);
	void appendKnownDomainIDs(std::vector<std::vector<uint8_t>> IDs);
	CTrieDB *getDB();
	std::shared_ptr<CSearchResults> searchDomains(const std::string& query, uint64_t size, uint64_t page, const CSearchFilter& filter);
	std::shared_ptr<CDomainDesc> createDomainDescription(CStateDomain* domain, bool includeSecData=false);
	uint64_t getDomainPerspectivesCount(const std::vector<uint8_t>& domainID);
/**
 * @brief Calculates the total transfers and transaction counts for a specific domain.
 *
 * This method computes the total amount of GNC received and sent, as well as the number of
 * incoming and outgoing transactions for a given domain. It processes the transaction history
 * stored in the domain's TXStats container, validating each transaction against the blockchain.
 *
 * @param domainID A vector of uint8_t representing the unique identifier of the domain.
 *
 * @return A tuple containing four elements:
 *         1. BigInt: Total amount of GNC received (in Attos)
 *         2. BigInt: Total amount of GNC sent (in Attos) 
 *         3. uint64_t: Number of incoming transactions (events when value > 0)
 *		  4. uint64_t: Number of outgoing transactions (events when value < 0)
 *		  5. BigInt: Total Genesis Rewards
 *	      6. BigInt: Total GNC Mined 
 *
 * @note This method ensures proper handling of edge cases, including:
 *       - Non-existent blockchain manager
 *       - Non-existent domain
 *       - Data integrity issues in the TXStats container
 *       - Invalid transactions (which are subtracted from the totals)
 *
 * @warning This method assumes that the TXStats container follows a specific format
 *          with 4 fields per transaction. Any deviation from this format will be
 *          treated as a data integrity error.
 *
 * @see CStateDomain, CBlockchainManager, CReceipt
 */
	std::tuple<BigInt, BigInt, uint64_t, uint64_t, BigInt, BigInt> CStateDomainManager::getDomainTransferTotals(const std::vector<uint8_t>& domainID);
	bool matchesDomainFilter(const std::shared_ptr<CDomainDesc>& domainDesc, const std::string& query, const CSearchFilter& filter);
	
	/**
	 * @brief Returns the total supply of GNC (Green Network Coin) by iterating through all available State Domains (accounts) in the system.
	 *
	 * This method computes the total supply of GNC by summing up the balances of all State Domains that are not classified as special domains
	 * (e.g., "ICOFund", "TheFund"). Additionally, it can optionally exclude locked assets from the total supply based on the parameter passed.
	 *
	 * The method first checks if a cached value is available and allowed. If so, and if the cached value is less than 5 minutes old, it returns
	 * the cached value. Otherwise, it proceeds with the full calculation.
	 *
	 * The method first checks if the Blockchain Manager is available. If not, it returns 0 as an error condition. Then, it iterates through
	 * all known domain IDs, skipping domains that are considered "special" or whose domain objects are null or unavailable.
	 *
	 * Locked assets can either be included or subtracted from the total supply depending on the `includeLockedAssets` parameter. If locked
	 * assets are to be excluded, the method ensures that the subtraction does not cause an underflow (i.e., the total supply will not become
	 * negative).
	 *
	 * After calculation, the result is cached for future use.
	 *
	 * @param includeLockedAssets A boolean value indicating whether locked assets should be included in the total supply.
	 *                            - If true, the total supply includes both locked and unlocked assets.
	 *                            - If false, locked assets are subtracted from the total supply, but not lower than 0.
	 * @param allowCached A boolean value indicating whether the use of cached values is permitted.
	 *                    - If true, the method may return a cached value if it's available and recent.
	 *                    - If false, the method will always perform a full calculation.
	 *
	 * @return BigInt The total supply of GNC across all non-special domains, optionally excluding locked assets.
	 *                Returns 0 if the Blockchain Manager is unavailable or in case of any other critical errors.
	 *
	 * @note Edge Case Handling:
	 *       - If the Blockchain Manager is unavailable, the method returns 0.
	 *       - If a domain object is null or cannot be found, it is skipped without affecting the final total.
	 *       - Special domains (e.g., "ICOFund", "TheFund") are excluded from the total supply calculation.
	 *       - In case locked assets exceed the total supply, the total supply is set to 0 (no negative supply).
	 *       - If a cached value is used, it may be up to 5 minutes old.
	 */
	BigInt getTotalSupply(bool includeLockedAssets = true, bool allowCached = true, bool useMetaData = false) ;
	void pingLastTotalSupplyCheck() const;
	void clearLastTotalSupplyCheck() const;
	uint64_t getLastTotalSupplyCheck() const;
	bool isInFlow();
	//Look-ups
	CStateDomain * findByID(std::vector<uint8_t> id,bool verifyDomainID=true, bool syncDomains=true, bool supportFlow=true);
	CStateDomain * findByPubKey(std::vector<uint8_t> pubKey);
	CStateDomain * findByIDToken(CIdentityToken id);
	std::vector<std::vector<uint8_t>> getKnownDomainIDs(bool includeThoseFromCurrentFlow = false) ;

	uint64_t notifyDataInUse();

	uint64_t notifyDataReleased();

	void lock();

	void unlock();

	//State-Domain deployment
	std::vector<std::vector<uint8_t>> getDomainsCreatedDuringFlow();
	bool create(std::vector<uint8_t> &perspective, CStateDomain** createdDomain,bool inSandbox,const  std::vector<uint8_t> &id = std::vector<uint8_t>());
	bool setPerspective(std::vector<uint8_t> perspective);
	void cleanUpFlowData();
	//Updates
	bool update(CStateDomain domain, std::vector<uint8_t> &perspective,bool inSandBox=false);
	bool enterFlow();
	CStateDomain* findByFriendlyID(std::string friendlyID);
	std::shared_ptr<CTokenPool> findTokenPoolByID(std::vector<uint8_t> id, CStateDomain**owner);
	bool exitFlow(bool updateKnownStateDomainIDs=true);
	std::vector<CStateDomain*> mInFlowDomainInstances;
	bool deleteDomainBody(std::vector<uint8_t> ID);
	bool unregisterDomain(std::vector<uint8_t> ID);
	bool registerDomain(CStateDomain * domain);
	std::recursive_mutex mGuardian;
	std::recursive_mutex mInFlowGuardian;
	mutable std::shared_mutex mSharedGuardian;
	mutable std::recursive_mutex mCurrentDBGuardian;
	size_t getKnownDomainsCount(bool includeThoseCreatedDuringTheFlow = false);

	bool getIsDataInHotStorage();

	bool getIsDomainMetaDataAvailable();

	


private:

	void setIsDomainMetaDataAvailable(bool isIt = true);
	bool mDomainMetaDataAvailable;
	// Meta-data cache - BEGIN
	robin_hood::unordered_map<std::vector<uint8_t>, std::shared_ptr<CDomainDesc>> mDomainMetadataCache;
	mutable std::shared_mutex mMetadataCacheMutex;

	// Update status tracking
	std::atomic<bool> mMetadataUpdateInProgress{ false };
	std::atomic<uint64_t> mMetadataUpdateProgress{ 0 };
	std::atomic<uint64_t> mDomainsMetadataProcessed{ 0 };
	std::atomic<uint64_t> mLastUpdateTimestamp{ 0 };
	std::atomic<uint64_t> mTotalDomainsToUpdate{ 0 };

	// Thread pool for updates
	std::shared_ptr<ThreadPool> mThreadPool;
	static const uint64_t DEFAULT_CHUNK_SIZE = 100;

	// Internal methods

	void domainMetaDataUpdateProgressChanged(uint64_t progress, uint64_t total);
	public:

		bool doDomainMetadataUpdate(bool forceUpdate = false,
			size_t numThreads = std::thread::hardware_concurrency(),
			std::shared_ptr<ThreadPool> threadPool = nullptr);
		bool isMetadataUpdateInProgress() const;
		uint64_t getLastUpdateTimestamp() const;
		uint64_t getUpdateProgress() const;
		std::shared_ptr<CDomainDesc> getCachedDomainMetadata(const std::vector<uint8_t>& domainID) const;
	// Meta-data cache - END

	private:

	mutable std::shared_mutex mCachedTotalSupplyMutex;
	mutable BigInt mCachedTotalSupply;
	mutable uint64_t mLastTotalSupplyCheck;

	std::vector<std::vector<uint8_t>> mDomainsCreatedDuringFlow;
	std::vector<std::vector<uint8_t>> mKnownStateDomainIDs;//these are independent from values cached by the underlying TrieDB
	//thus the Trie needs to be tested only Once. Then; new domains are added to mDomainsCreatedDuringFlow during the flow,
	// and once the Flow was successful ( exitFlow called with updateKnownStateDomainIDs set to True); then the new identifiers
	//are copied over to the mKnownStateDomainIDs. Other objects can rely on the values provided by the appropiate TransactionsManager's (Live/Flow)
	// State Domain Manager
	//size_t mKnownStateDomainsCount;
	mutable std::mutex mKnownDomainsGuardian;
	CTransactionManager * mTransactionsManager;
	
	std::shared_ptr<CTools> mTools;////shared so that it can be copied over fast in a copy constructor without need for re-initialization
	CTrieDB * mCurrentDB;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	bool mInFlow;
};


