#ifndef SOLID_STORAGE_H
#define SOLID_STORAGE_H


/*
Recently we've updated to most recent branch of RockDB 9.*

1) vcpkg remove rocksdb:x64-windows
2) vcpkg install rocksdb:x64-windows
3) in file cache.h during compilation error replace relevant lines with:

-----------------------------------------------------------
Status UpdateTieredCache(
	const std::shared_ptr<Cache>& cache, int64_t total_capacity = -1,
	double compressed_secondary_ratio = (std::numeric_limits<double>::max)(),
	TieredAdmissionPolicy adm_policy = TieredAdmissionPolicy::kAdmPolicyMax);
	} // namespace ROCKSDB_NAMESPACE

-----------------------------------------------------------

Rationale: we need to scope max() macro
*/
#include "stdafx.h"
#include "IdentityToken.h"
#include "sqlite3.h"
#include <iostream>
#include <ios>
#include <shared_mutex>
#include "rocksdb/db.h"
#include "BlockHeader.h"
#include "enums.h"
#include "CryptoFactory.h"
 static sqlite3 *sqlDB;
 static std::string DBName = "GRIDNET";
 static std::string DBIdentTokensTable = "IdentityTokens";
 static std::string DBName2 = "GRIDNET";
 static std::string DBName3 = "GRIDNET";
 static std::string DBName4 = "GRIDNET";
 //used to distinguish data types in the db
 static std::string H_BLOCK_HEADER_PREFIX = "blHd_";//followed by [blockHeight] | _
 static std::string H_BLOCK_PREFIX = "BL_";//followed by [blockHeight] | _
 static std::string H_LATEST_BLOCK_HEIGHT_ = "maxBLHeight";
 static std::string H_NODE_HASH = "n_";
 static std::string H_LINK_PREFIX = "L_";
 static std::string H_TRIE_ROOT_HASH = "N_";
 static std::string H_ACCOUNT_STORAGE_PREFIX = "AS_";
 static std::string H_MAIN_PUB_KEY = "mainPubKey";
 static std::string H_MAIN_PRIV_KEY = "mainPrivKey";
 static std::string H_MAIN_PRIV_KEY_NONCE = "mainPrivKeyNonce";
 static std::string H_REWARD = "reward";
 static std::string H_BLOCK_LOG_ENTRY = "BL_";
 //followed by chain name
 static std::string H_USER_CHAIN = "usrChain_";
 static std::string H_MIN_DIFF = "minDiff_";
 static std::string H_MIN_SYNC = "minSync_";
 static std::string H_MIN_ERG_BID = "minERGBid";
 static std::string H_CLIENT_DEF_ERG_BID = "clientDefERGBid";
 static std::string H_CLIENT_TOTAL_ERG_LIMIT= "clientToalERGLimit";
 static std::string H_CHECKPOINTS_COUNT = "checkpointsCount";
 static std::string H_CLIENT_DEF_TRANS_ERG_LIMIT = "clientDefTransERGLimit";
 class CBlock;
 class CLinkContainer;



class CSolidStorage
{
private:
	// Cache data structures:
	struct CacheValue {
		std::vector<uint8_t> data; // The stored value
	};
	std::mutex mBlockchainModeGuardian;
	//static variables
	static bool mWasSQLDatabaseInitialized;
	static bool staticVirginityDetector;
	static bool  mWereRocksDBsDatabasesInitialized;

	eBlockchainMode::eBlockchainMode mBlockchainMode;
	static std::mutex sStaticInstancesGuardian;
	static CSolidStorage * sLIVEInstancePointer;
	static CSolidStorage * sLIVESandBoxInstancePointer;

    static CSolidStorage * sTestNetInstancePointer;
	static CSolidStorage * sTestNetSandBoxInstancePointer;
	static CSolidStorage * sLocalTestsInstancePointer;
	//LIVE-NET
	static rocksdb::DB* sLIVEBlockchainDB; //contains Blockchain blocks
	static rocksdb::DB* sLIVEStateTrieDB; //contains a Global State Trie
	static rocksdb::DB* sLIVEStagedStateTrieDB;//pruned, Staged version of the Global State-Trie
	//TEST-NET
	static rocksdb::DB* sTestNetBlockchainDB;
	static rocksdb::DB* sTestNetStateTrieDB;
	static rocksdb::DB* sTestNetStagedStateTrieDB;

	//LOCAL-TESTS
	static rocksdb::DB* sLocalTestBlockchainDB;
	static rocksdb::DB* sLocalTestStateTrieDB;
	static rocksdb::DB* sLocalTestStagedStateTrieDB;

	//local object specific variables:
	bool mAbortAllWrites;
    sqlite3 *mSQLDB;// not used now.
	rocksdb::DB* mStateTrieDB;
	rocksdb::DB* mStagedStateTrieDB;
	rocksdb::DB* mBlockchainDB;
	//std::mutex mStateTrieGuardian;


	//CSolidStorage constitutes a wrapper/intermediary around any of the following ROCKS-DB instances.
	//(ROCKS-DB permits multi-threaded access to DBs for 'basic' operations)

	// Example: TestNetInstancePointer of CSolidStorage will point to the same  rocksdb::DB as the sTestNetSandBoxInstancePointer
	//with the exception that sTestNetSandBoxInstancePointer instance will have the mAbortAllWrites set to TRUE.

	//The choice between Blockchain-DB and Sate-Trie DB is made on the level of particular SolidStorage functions.
	//At the higher level there are two CSolidStorage instances for both the LIVE and Test-Net - 4 in total.
	// Two: one the write-enabled CSolidStorage and the second- where writes are disabled. No matter what.
	// Note: the Flow mechanics, additionally protects the data-access layer and extends the SandBox capabilities.
	//The mechanisms are NOT redundant. Flows allow for high-level transactional commits.
	//RAW CSolidStorage with writes disabled allows for low-level SandBox environment.
	//Example: The Decentralized Terminal is coupled with a TransactionManager whose GridScript instance talks to the SandBoxed SolidStorage

	

	//from time to time the entire current state of the mStateTrieDB is copied to mStagedStateTrieDB.
	//the HDD-content of the mStateTrieDB is then WIPED. All the irrelevant data such as copies of very old nodes are now lost. 
	std::shared_ptr<CTools> mTools;
	

    std::shared_ptr<CCryptoFactory> mCf;

    std::vector<uint8_t> mLatestBlockID;

    size_t mRecentBlocksCacheSize;
    std::vector<std::shared_ptr<CBlock>> mRecentBlocksCache;
	bool mIsTestDB = false;
	static bool initialiseRocksDBs();
	static rocksdb::DB** getDatabaseReference(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType type);
	static bool initializeDatabase(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType, rocksdb::DB** dbRef, const HardwareSpecs& specs);
	bool initializeSQLDB();
	bool doesSQLTableExist(std::string tableName);
	bool createIdentityTokenTable();

	// Links Cache - BEGIN
	
	 // Hash and equality for 32-byte vectors
	 /// <summary>
	/// Hash functor for 32-byte keys in the robin_hood map.
	/// Since keys are guaranteed to be 32 bytes and cryptographically random,
	/// we simply use the first 8 bytes as a 64-bit integer for hashing.
	/// </summary>
	struct KeyHash {
		std::size_t operator()(const std::vector<uint8_t>& key) const {
			// key is exactly 32 bytes
			uint64_t hashVal = 0;
			static_assert(sizeof(hashVal) == 8, "uint64_t should be 8 bytes");
			std::memcpy(&hashVal, key.data(), sizeof(hashVal));
			return static_cast<std::size_t>(hashVal);
		}
	};

	/// <summary>
	/// Equality functor: compares two 32-byte keys for equality.
	/// </summary>
	struct KeyEq {
		bool operator()(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) const {
			return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
		}
	};



	std::string prefixFromType(eLinkType::eLinkType type);
	std::vector<uint8_t> makeCacheKey(eLinkType::eLinkType type, const std::vector<uint8_t>& ID);
	void insertIntoLinkCache(const std::vector<uint8_t>& key, const std::vector<uint8_t>& value);

	// Hard-coded max size for the cache:
	static constexpr size_t MAX_CACHE_SIZE = 50000;
	// Synchronization for cache access
	std::shared_mutex mCacheMutex;
	// The in-RAM cache
	robin_hood::unordered_flat_map<std::vector<uint8_t>, CacheValue, KeyHash, KeyEq> mCache;

	// To manage eviction we keep track of insertion order
	std::deque<std::vector<uint8_t>> mCacheInsertionOrder;
	mutable  std::shared_mutex mDataAccessGuardian;
	// Links Cache - END

	bool mSystemAvailable;
	std::mutex mFieldsGuardian, mToolsGuardian;
	static std::string mMainDataDir;
	std::string mUIPackageDir;
	bool mUIDataAvailable;
	bool mUIUpdateInProgress;
	std::thread mUIUpdateThread;
	static std::string mKernelCacheDir;
	std::shared_ptr<CTools> getTools();
	mutable std::mutex mBlocksMutex;
	static void configureRocksDBOptions(rocksdb::Options& options, const HardwareSpecs& specs);
	std::atomic<uint64_t> mCleanupIntervalSeconds;
	std::atomic<uint64_t> mLastCleanupTime;
public:
	// Default cleanup interval is 1 hour
	static const uint64_t DEFAULT_CLEANUP_INTERVAL_SECONDS = 3600;

	// Getters and setters for cleanup interval
	uint64_t getCleanupIntervalSeconds() const;
	void setCleanupIntervalSeconds(uint64_t seconds);

	// Check if cleanup is needed
	bool isCleanupNeeded() const;

	// Main cleanup method
	void performCleanup();
	void periodicMemoryCheck();
	void setUIDataAvailable(bool isIt = true);
	bool getUIDataAvailable();
	void setIsSystemAvailable(bool isIt = true);
	bool getIsSystemAvailable();
	bool saveStringToFile(std::string data, std::string fileName);
	bool readStringFromFile(std::string& data, std::string fileName);
	std::vector<std::string> getAllFilesInDir(std::string dirPath);
	bool saveToUTF8File(std::wstring data, std::string fileName);
	bool readFromUTF8File(std::wstring& data, std::string fileName);
	eBlockchainMode::eBlockchainMode getBlockchainMode();
	CSolidStorage(const CSolidStorage& sibling);
	bool isTestDB();
	CSolidStorage(eBlockchainMode::eBlockchainMode mode, bool init=true);
	~CSolidStorage();
	bool saveLink(std::shared_ptr<CLinkContainer> link);
	/// <summary>
		/// Saves a link to persistent storage and optionally to in-RAM cache.
		/// If allowHotStorage is true, the entry is also cached in RAM.
		/// </summary>
		/// <param name="ID">The identifier vector (32 to 35 bytes). If exactly 32 bytes, it's considered already SHA-256 hashed.</param>
		/// <param name="valueP">The value associated with this ID.</param>
		/// <param name="type">The link type, used to differentiate key namespaces.</param>
		/// <param name="allowHotStorage">If true, also store/retrieve the link from RAM cache. Default is true.</param>
		/// <returns>True if saved successfully, false otherwise.</returns>
	bool saveLink(std::vector<uint8_t> ID, std::vector<uint8_t> valueP, eLinkType::eLinkType type, bool allowHotStorage = true);

	/// <summary>
	/// Loads a link from persistent storage or from in-RAM cache if allowed.
	/// If allowHotStorage is true, try RAM cache first. On a miss, load from DB and cache it.
	/// </summary>
	/// <param name="keyP">The identifier vector originally used to save the link.</param>
	/// <param name="valueP">Output parameter for the retrieved value.</param>
	/// <param name="type">The link type.</param>
	/// <param name="allowHotStorage">If true, attempt caching. Default is true.</param>
	/// <returns>True if the link is found, false otherwise.</returns>
	bool loadLink(std::vector<uint8_t> keyP, std::vector<uint8_t>& valueP, eLinkType::eLinkType type, bool allowHotStorage = true);

	bool addBlockToCache(std::shared_ptr<CBlock> block);
	std::vector<std::shared_ptr<CBlock>> getCachedBlocks();

	std::vector<uint8_t>  getValue(std::string key);
	std::vector<uint8_t>  getValue(std::vector<uint8_t> key);
	bool saveValue(std::vector<uint8_t> key, std::vector<uint8_t> value);

	bool saveValue(std::string key, std::string value);
	bool saveValue(std::string key, std::vector<uint8_t> value);
	bool deleteValue(std::vector<uint8_t> key);
	bool saveByteVectors(std::string key, const std::vector < std::vector<uint8_t>> &vectors);
	bool saveByteVectors(std::vector<uint8_t> key, const std::vector < std::vector<uint8_t>> &vectors);

	std::vector < std::vector<uint8_t>> loadByteVectors(std::string key);
	std::vector < std::vector<uint8_t>> loadByteVectors(std::vector<uint8_t> key);

	static CSolidStorage * getInstance(eBlockchainMode::eBlockchainMode mode);

	std::string getMemUsageReport();
	//Trie-Node solid-storage manipulation
	bool saveNode(CTrieNode *node,std::string prefix);
	bool saveNode(std::vector<uint8_t> id,std::vector<uint8_t> BER_TrieNode, std::string prefix="" );
	bool getAbortAllWrites();
	bool deleteNode(std::vector<uint8_t> nodeID, std::string prefix = "");
	std::vector<uint8_t> loadNode(std::vector<uint8_t> hash, std::string prefix="");

	double getTotalRAMUsageGB(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType);

	double getTotalDiskUsageGB(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType);

	bool releaseMemory(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType);

	bool generateDatabaseReport(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType, std::string& report);

	std::vector<std::string> unzip(std::string const& zipFile, std::string const& path, std::string const& password);

	static bool destroyDatabase(const std::string& dbPath, rocksdb::Options& options);

	static bool destroySpecificDatabase(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType);

	bool destroyDatabasesForMode(eBlockchainMode::eBlockchainMode mode);

	bool destroyData();

	std::string getDebugUIDir();

	std::string getUIDir();

	bool checkUIDataAvailable();

	bool getIsUIUpdateInProgress();
	
	bool updateUIPackage(bool initial = true, bool justCheckForPackage = false);

	void UIUpdateExited();

	void updateUIPackageThreadF(std::string appDataDir);

	static bool setupDataDir(std::string& path);

	static std::string getMainDataDir();

	static void setMainDataDir(const std::string& dir);
	static void setKernelCacheDir(const std::string& dir);
	std::string getKernelCacheDir();
	static bool initialiseSolidStorage();
	//Trie root manipulation: Note: currently it acts the same as above but with automatic instantiation.
	bool saveCurrentTrieRoot(CTrieNode *node,std::string prefix);
	CTrieNode * loadNode(std::string prefix,std::vector<uint8_t> rootHasH);

	bool readBinaryFromFile(std::string fileName,std::vector<uint8_t>& data );

	//blocks
	bool saveBlock(std::vector<uint8_t> BER_block, std::vector<uint8_t> parentHash);

	bool saveBlock(std::shared_ptr<CBlock> block);

	std::vector<uint8_t> getBlockDataByHash(std::vector<uint8_t> hash);

	std::shared_ptr<CBlock> getBlockByHash(std::vector<uint8_t> hash, eBlockInstantiationResult::eBlockInstantiationResult& mResult, bool instantiateTries=true, const uint64_t &bytesCount=0, bool loadEffectiveData=true);
	bool getTotalBlockRewardEffective(const std::vector<uint8_t>& blockID, BigInt& totalReward);
	bool setTotalBlockRewardEffective(const BigInt& totalReward, const std::vector<uint8_t>& blockID);
	bool getPaidToMinerEffective(const std::vector<uint8_t>& blockID, BigInt& paidToMiner);
	bool setPaidToMinerEffective(const BigInt& paidToMiner, const std::vector<uint8_t>& blockID);
	bool checkIfBlockInStorage(std::vector<uint8_t> hash);

	static bool closeAllDBs(bool freeMemory=true);


	HardwareSpecs assessHardware();

};

#endif

rocksdb::ReadOptions getOptimizedReadOptions();
