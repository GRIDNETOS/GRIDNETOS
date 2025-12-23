#pragma once

#include "stdafx.h"
#include <shared_mutex>

#include "IManager.h"
#include "enums.h"
#include "robin_hood.h"
#include <queue>
#include "Block.h"
#include "keyChain.h"

class CBlockVerificationResult;
class CTests;
//class CBlock;
class CSolidStorage;
class COperatorSecurityInfo;
class CStateDomainManager;
class CWorkManager;
class CTrieDB;
class CSettings;
class CBreakpointFactory;
class CKeyChain;
class COCLEngine;
class CNetworkManager;
class CTransactionManager;
class CCryptoFactory;
class CTransaction;
class CReceipt;
class CGlobalSecSettings;
class ThreadPool;
class CBlockHeader;
class CVerifiable;
class CGridScriptCompiler;
class CBCheckpoint;
#include "SearchResults.hpp"
class CBreakpoint;
class CBreakpointFactory;
class CSearchFilter;

// Data structure to hold operator security information


// Constants - BEGIN
constexpr size_t VALID_BLOCK_ID_LENGTH = 32;
// Constants - END
struct ThreadTimeEstimation {
	double emaBlocksPerSecond = 0.0;
	bool firstEstimation = true;
	std::chrono::steady_clock::time_point lastReportTime;

	ThreadTimeEstimation() : lastReportTime(std::chrono::steady_clock::now()) {}

	// Update EMA with the latest blocks processed and time elapsed
	void update(double blocksProcessed, double timeElapsedSeconds, double alpha = 0.2) {
		if (timeElapsedSeconds > 0 && blocksProcessed > 0) {
			double blocksPerSecond = blocksProcessed / timeElapsedSeconds;
			if (firstEstimation) {
				emaBlocksPerSecond = blocksPerSecond;
				firstEstimation = false;
			}
			else {
				emaBlocksPerSecond = alpha * blocksPerSecond + (1.0 - alpha) * emaBlocksPerSecond;
			}
		}
	}
};

//Global Dial Bars - BEGIN
#define DIFFICULTY_DIAL_BAR_ID 995
#define MINING_DIAL_BAR_ID 899 // IS_LEADER, nr_of_mined_blocks, total_reward_GNC,processedTXlocally, processed_TX_GLOBAL, Mem_ppol_size, mem_pool_req_#
#define SYNC_BAR_ID 898
#define HARDWARE_BAR_ID 897
#define OPENCL_DIAGNOSTICS_BAR_ID 896
#define OPENCL_COMPILED_DEVICES_COUNT_ID 895
#define OPENCL_COMPILING_DEVICES_SINCE_BAR_ID 894

struct OperatorActivity {
	size_t blocksMined = 0;
	size_t blocksMinedDuringDifficultyDecrease = 0;
	size_t blocksMinedDuringDifficultyIncrease = 0;
	size_t blocksMinedDuringAnomalousIntervals = 0;
	std::vector<uint64_t> solvedTimes;
	uint64_t lastReportedAtHeight = 0; // To avoid duplicate reports
};
/**
*
*  This is [Level 2]  is CTXInfo (Core-only container containing additional pointers to raw Level 1 data).
*  This container maintains ownership of Level 3 container and holds references to detached  (copies, not part of MPT) of Level 1 containers.
*
   [ Level 1 ] - This is a BER - encoded VM Meta Data exchange protocol component.
*			   Compatible implementation in JavaScript.

[Level 1]  are raw CTransaction and CReceipt containers.
*/
class CTXInfo
{
public:
	CTXInfo(
		std::shared_ptr<CTransaction> transaction = nullptr,
		std::shared_ptr<CReceipt> receipt = nullptr,
		std::shared_ptr <CTransactionDesc> description = nullptr
	)
		: mTransaction(transaction)
		, mReceipt(receipt)
		, mDescription(description)
	{}

	// Getters and setters (including the new value field)
	std::shared_ptr<CTransaction> getTransaction() const { std::shared_lock lock(mGuardian); return mTransaction; }
	std::shared_ptr<CReceipt> getReceipt() const { std::shared_lock lock(mGuardian); return mReceipt; }
	
	void setTransaction(std::shared_ptr<CTransaction> transaction) { std::unique_lock lock(mGuardian);  mTransaction = transaction; }
	void setReceipt(std::shared_ptr<CReceipt> receipt) { std::unique_lock lock(mGuardian); mReceipt = receipt; }
	std::shared_ptr<CTransactionDesc> getDescription() { std::unique_lock lock(mGuardian); return mDescription;  }
	void setDescription(std::shared_ptr<CTransactionDesc> description) { std::unique_lock lock(mGuardian); mDescription = description; }

private:



	std::shared_ptr<CTransactionDesc> mDescription;
	std::shared_ptr<CTransaction> mTransaction;
	std::shared_ptr<CReceipt> mReceipt;
	mutable std::shared_mutex mGuardian;

};

//Global Dial Bars - END
namespace SE {
	class CScriptEngine;
}
struct WarningInfo {
	std::string text;
	std::string source;
	uint64_t timestamp;
};
class CBlockchainManager :public  std::enable_shared_from_this<CBlockchainManager>, public IManager
{
private:

	ReverseSemaphore mLocalBlockInPipeline;// [ Rationale ]: to cope with a Schrodinger's Cat effect after a TX was taken from mem-pool, block formed but the block was not processed yet.
	//				 during this period a transaction is both processed and not-processed. 
	


	// Memory Pool Cache - BEGIN
	robin_hood::unordered_map<std::vector<uint8_t>, std::shared_ptr<CTransactionDesc>> mMemPoolTXMetaCache;
	uint64_t mMaxMemPoolCacheSize;
	mutable std::shared_mutex mMemPoolCacheMutex;
	// Memory Pool Cache - END
	bool mSynchronizationPaused;
	bool mDoNotProcessExternalChainProofs;
	// Operators Security Assessment - BEGIN

/*
* 
*						- [ USAGE EXAMPLES ] - 
* 
// Assume blockchainManager is an instance of CBlockchainManager

// Process each block in chronological order
for (uint64_t height = 0; height <= blockchainManager.getLatestBlockHeight(); ++height) {
    std::shared_ptr<CBlock> block = blockchainManager.getBlockAtHeight(height);
    if (block) {
        blockchainManager.updateSecAnalysis(block);
    }
}

// Retrieve the security report
auto securityReport = blockchainManager.getSecurityReport();

// Display the report
for (const auto& opInfo : securityReport) {
    std::cout << "Operator ID: " << opInfo.operatorID << std::endl;
    std::cout << "Timestamp Manipulation Count: " << opInfo.timestampManipulationCount << std::endl;
    std::cout << "PoW Wave Attack Count: " << opInfo.powWaveAttackCount << std::endl;
    std::cout << "Detailed Reports:" << std::endl;
    for (const auto& report : opInfo.detailedReports) {
        std::cout << " - " << report << std::endl;
    }
    std::cout << std::endl;
}

// Retrieve report for a specific operator
std::string operatorID = "operator123";
OperatorSecurityInfo opInfo = blockchainManager.getSecurityReportForOperator(operatorID);

// Display operator's security report
std::cout << "Operator ID: " << opInfo.operatorID << std::endl;
std::cout << "Timestamp Manipulation Count: " << opInfo.timestampManipulationCount << std::endl;
std::cout << "PoW Wave Attack Count: " << opInfo.powWaveAttackCount << std::endl;
std::cout << "Detailed Reports:" << std::endl;
for (const auto& report : opInfo.detailedReports) {
    std::cout << " - " << report << std::endl;
}

// Perform a quick PoW wave assessment over the recent 50 key blocks
blockchainManager.analyzeRecentPoWWaveAttacks(50);

	---

# Security Analysis in CBlockchainManager

## Overview

The `CBlockchainManager` class includes sophisticated security analysis mechanisms designed to detect and report on potential malicious activities within the blockchain. Specifically, it targets:

1. **Timestamp Manipulation Attacks**: Where node operators or transaction issuers manipulate timestamps to gain unfair advantages or disrupt network consensus.

2. **Proof-of-Work (PoW) Wave Attacks**: Where mining nodes with significant hashing power manipulate their mining activity patterns to destabilize the network's difficulty adjustment mechanism.

This documentation provides an in-depth explanation of the mechanics, rationale, and implementation details of the security analysis methods integrated into the `CBlockchainManager`.

---

## Attacks Detected

### 1. Timestamp Manipulation Attacks

**Description**: In a timestamp manipulation attack, a node operator or transaction issuer intentionally sets incorrect timestamps in blocks or transactions. This can lead to:

- **Unfair Advantages**: Such as pre-mining or reordering transactions.
- **Consensus Disruption**: Causing inconsistencies in the blockchain's chronological order.

**Impact**:

- **Blockchain Integrity**: Incorrect timestamps can compromise the trustworthiness of the blockchain data.
- **Network Performance**: May lead to forks or delays in transaction confirmations.

### 2. Proof-of-Work (PoW) Wave Attacks

**Description**: In a PoW wave attack, a mining node with significant hashing power intermittently participates in mining. The node mines blocks rapidly after a period of inactivity, causing:

- **Difficulty Adjustment Lag**: The network's difficulty algorithm may not adjust quickly enough to the sudden change in mining power.
- **Resource Strain on Other Nodes**: Other miners struggle to keep up with the increased difficulty when the attacking node withdraws.

**Impact**:

- **Network Stability**: Fluctuations in block times and difficulty adjustments can destabilize the network.
- **Mining Fairness**: Creates an uneven playing field, disadvantaging honest miners.

---

## Security Analysis Methods

### Overview

The security analysis comprises two main methods:

1. **`updateSecAnalysis(const std::shared_ptr<CBlock>& block)`**: Updates the security analysis with each new block appended to the blockchain.

2. **`getSecurityReport()`**: Retrieves a comprehensive report of operators suspected of malicious activities, sorted by severity.

---

### 1. `updateSecAnalysis(const std::shared_ptr<CBlock>& block)`

#### **Purpose**

- To incrementally analyze new blocks for signs of timestamp manipulation and PoW wave attacks.
- To maintain up-to-date security statistics without reprocessing the entire blockchain.

#### **Mechanics**

- **Timestamp Manipulation Detection**:
  - Analyzes data blocks (blocks containing transactions).
  - For each transaction, computes the difference between the block's confirmed timestamp and the transaction's unconfirmed timestamp.
  - Flags transactions where the absolute time difference exceeds a threshold (e.g., 3 hours).
  - Updates the operator's security info if suspicious transactions are found.

- **PoW Wave Attack Detection**:
  - Focuses on key blocks (blocks containing proof-of-work and difficulty adjustments).
  - Maintains a mining record per operator, storing key block heights and their solved times.
  - Uses a sliding window of recent key blocks to analyze mining patterns.
  - Detects patterns where an operator shows periods of inactivity followed by rapid mining of blocks.

#### **Rationale**

- **Efficiency**: By analyzing blocks as they are added, the system avoids the computational overhead of reprocessing the entire chain.
- **Timeliness**: Immediate detection allows for quicker responses to potential attacks.
- **Incremental Updates**: Maintains ongoing statistics, improving the accuracy of the analysis over time.

#### **Detailed Steps**

1. **Check Block Validity**:
   - Ensures the provided block is not null.
   - Determines if the block is a key block or data block.

2. **Timestamp Manipulation Analysis** (for data blocks):
   - Retrieves the operator ID from the block header or parent key block.
   - Obtains the block's transaction metadata (`CBlockDesc` and `CTransactionDesc`).
   - Iterates over each transaction:
     - Calculates the time difference.
     - Checks if the difference exceeds the threshold.
     - Counts suspicious transactions.
   - If suspicious transactions are found:
     - Updates the operator's `timestampManipulationCount`.
     - Records a detailed report explaining the findings.

3. **PoW Wave Attack Analysis** (for key blocks):
   - Retrieves the operator ID and mining time.
   - Updates the operator's mining records with the new key block information.
   - Analyzes recent mining intervals using a sliding window.
   - Detects significant deviations from the expected key-block interval.
   - If suspicious patterns are detected:
     - Updates the operator's `powWaveAttackCount`.
     - Records a detailed report explaining the findings.

#### **Examples**

- **Timestamp Manipulation**:
  - If a transaction claims to have been issued 5 hours before it was included in a block, and the threshold is 3 hours, it is flagged as suspicious.

- **PoW Wave Attack**:
  - An operator mines no blocks for several hours (long interval), then suddenly mines multiple blocks in quick succession (short intervals), indicating possible manipulation.

---

### 2. `getSecurityReport()`

#### **Purpose**

- To provide a comprehensive, sorted report of operators involved in suspicious activities.
- To facilitate monitoring and auditing of the network's security.

#### **Mechanics**

- Aggregates security information collected during the incremental analysis.
- Sorts operators based on the total number of offenses (timestamp manipulation and PoW wave attacks).
- Presents detailed reports for each operator, including counts and specific incidents.

#### **Rationale**

- **Transparency**: Allows stakeholders to identify and address potential malicious actors.
- **Prioritization**: Sorting helps focus on the most severe cases first.
- **Accountability**: Encourages operators to maintain honest behavior due to potential scrutiny.

#### **Detailed Steps**

1. **Lock Data Structures**:
   - Uses shared locks to ensure thread-safe access to the security map.

2. **Collect Operator Info**:
   - Iterates over the `operatorSecurityMap`.
   - Gathers each operator's security statistics and detailed reports.

3. **Sort Operators**:
   - Calculates the total offenses for each operator.
   - Sorts the list in descending order of total offenses.

4. **Return Report**:
   - Provides the sorted vector of `OperatorSecurityInfo` structures for further processing or display.

#### **Examples**

- An operator with 5 timestamp manipulation offenses and 2 PoW wave attack offenses will be ranked higher than one with 3 total offenses.

---

## Data Structures

### `OperatorSecurityInfo`

#### **Fields**

- `operatorID`: The unique identifier of the operator (e.g., public key hash).
- `timestampManipulationCount`: The number of suspicious timestamp discrepancies detected.
- `powWaveAttackCount`: The number of potential PoW wave attack patterns detected.
- `detailedReports`: A vector of strings containing detailed descriptions of each suspicious incident.

#### **Purpose**

- To encapsulate all security-related information for a given operator.
- To facilitate easy aggregation, sorting, and reporting of security data.

---

## Supporting Methods and Members

### `analyzeBlockForTimestampManipulation(const std::shared_ptr<CBlock>& block)`

#### **Purpose**

- To analyze a single data block for potential timestamp manipulation attacks.

#### **Mechanics**

- Extracts the operator ID and transaction metadata.
- Computes time differences for each transaction.
- Flags transactions exceeding the threshold.
- Updates security information accordingly.

### `analyzeChainForPoWWaveAttacks(const std::shared_ptr<CBlock>& newBlock)`

#### **Purpose**

- To analyze key blocks incrementally for signs of PoW wave attacks.

#### **Mechanics**

- Updates the operator's mining records with the new key block.
- Uses a sliding window to focus on recent activity.
- Computes intervals between mining events.
- Detects patterns of irregular mining activity.

### Data Members

- `operatorSecurityMap`: Stores security information for each operator.
- `operatorMiningRecords`: Keeps track of mining times and key block heights per operator.
- `securityMapMutex` and `miningRecordsMutex`: Ensure thread-safe access to shared data.

---

## Rationale Behind Detection Techniques

### Timestamp Manipulation Detection

- **Threshold Setting**: The 3-hour threshold balances the need to detect significant discrepancies while allowing for minor time variations due to network delays or clock differences.
- **Per-Operator Analysis**: By attributing suspicious transactions to operators, the system can identify nodes that consistently exhibit malicious behavior.

### PoW Wave Attack Detection

- **Targeted Interval Usage**: Utilizing the network's expected key-block interval allows the detection mechanism to adapt to the designed pace of block production.
- **Sliding Window**: Focusing on recent blocks ensures that the analysis is relevant to current network conditions.
- **Threshold Factor**: Comparing intervals against a multiple of the expected interval helps identify significant deviations without being overly sensitive to normal variations.

---

## Examples and Scenarios

### Scenario 1: Operator Engaging in Timestamp Manipulation

- **Behavior**: An operator includes transactions with timestamps significantly older than the block's timestamp.
- **Detection**:
  - The system computes the time difference for each transaction.
  - Transactions exceeding the threshold are counted.
  - The operator's `timestampManipulationCount` is incremented.
- **Report**:
  - The operator appears in the security report with detailed incidents.
  - Stakeholders can investigate the operator's activity further.

### Scenario 2: Operator Performing PoW Wave Attack

- **Behavior**: An operator stops mining for an extended period and then mines several key blocks rapidly.
- **Detection**:
  - The system records the operator's mining times.
  - Intervals between mining events are analyzed.
  - Significant deviations from expected intervals trigger a flag.
- **Report**:
  - The operator's `powWaveAttackCount` increases.
  - Detailed reports specify the key block heights and times involved.

---

## Thread Safety and Performance Considerations

- **Concurrent Access**: Shared and unique locks are used to protect data structures from race conditions in multi-threaded environments.
- **Incremental Updates**: By updating analyses with each new block, the system avoids the overhead of full-chain reprocessing.
- **Data Structure Efficiency**: Unordered maps and vectors are used for fast lookups and iterations.

---

## Conclusion

The security analysis methods integrated into `CBlockchainManager` provide robust mechanisms for detecting and reporting on two critical types of attacks in proof-of-work blockchains. By analyzing operator behaviors and transaction patterns, the system enhances the network's resilience and integrity.

These tools are essential for maintaining trust in the blockchain and ensuring that all participants adhere to fair and honest practices.

---
	*/
	// Internal methods
	

/**
	 * @brief Analyzes a single block for timestamp manipulation attacks.
	[ Purpose ]:
	  - Analyzes a single data block for potential timestamp manipulation attacks.

	[ Behavior ]:
	  - Checks if the block is a data block (not a key block).
	  - Retrieves the operator ID from the block header or parent key block.
	  - Iterates over each transaction in the block:
	  - Computes the time difference between the unconfirmed and confirmed timestamps.
	  - Flags transactions where the absolute time difference exceeds 3 hours (10,800 seconds).
	  - Updates the OperatorSecurityInfo for the operator if suspicious transactions are found.

	[ Additional Info ]:
	 * This method is called internally by updateSecAnalysis and should not be called directly.
	 * It examines the transactions within a data block to detect significant discrepancies between
	 * unconfirmed and confirmed timestamps, which may indicate manipulation.
	 *
	 * @param block The block to analyze.
	 */
/*
* 
* 


*/
	void analyzeBlockForTimestampManipulation(const std::shared_ptr<CBlock>& block);
	void analyzeChainForPoWWaveAttacks(const std::shared_ptr<CBlock>& block);

	void updateHistoricalGroupActivity(double groupActivityRatio);

	void updateHistoricalIndividualActivity(double individualActivityRatio);

	double getHistoricalIndividualActivityMean() ;

	double getHistoricalIndividualActivityStdDev() ;

	double calculateStandardDeviation(const std::vector<uint64_t>& intervals, double mean) const;

	double getHistoricalGroupActivityMean() ;

	double getHistoricalGroupActivityStdDev() ;

	// Data structures
	std::unordered_map<std::string, std::shared_ptr<COperatorSecurityInfo>> operatorSecurityMap; // Maps operator IDs to their security info
	std::shared_mutex securityMapMutex;

	// Additional data structures for PoW wave attack detection
	std::deque<std::shared_ptr<CBlock>> recentKeyBlocks;   // Sliding window of recent key blocks
	std::unordered_map<std::string, OperatorActivity> operatorActivities; // Cumulative operator activities
	std::shared_mutex dataMutex; // Mutex to protect access to shared data structures
	// Operators Security Assessment - END
	
	// USDT Price - BEGIN
	mutable std::shared_mutex mPriceGuardian;
	mutable BigInt mCachedAttoPerUSD;
	mutable uint64_t mLastPriceCheck = 0;
	mutable BigInt mDailyOpenAttoPerUSD;
	mutable uint64_t mLastDailyReset = 0;

	const uint64_t PRICE_CACHE_TIMEOUT = 60; // Cache timeout in seconds
	const uint64_t SECONDS_IN_DAY = 86400;   // Number of seconds in a day
	const double MAX_DAILY_FLUCTUATION = 0.10; // 10% max daily fluctuation
	const BigInt ATTO_PER_GNC = BigInt("1000000000000000000");
	// USDT Price - END
	// Liveness - BEGIN
	mutable std::shared_mutex mLivenessGuardian;
	mutable eLivenessState::eLivenessState mCachedLiveness = eLivenessState::noLiveness;
	mutable uint64_t mLastLivenessCheck = 0;
	const uint64_t LIVENESS_CACHE_TIMEOUT = 60; // Cache timeout in seconds
	// Liveness - END

	uint64_t mLastTimeBlockCacheCleared;
	mutable std::recursive_mutex mBlockCacheFlatGuardian;
	// Detached Cache - BEGIN
	uint64_t mBlockUnorderedCacheMaxSize;


	 /** @brief Shared mutex to protect access to the block unordered cache. */
	mutable std::shared_mutex mBlockUnorderedCacheMutex;

	/**
	 * @brief Map from block IDs to pairs of shared pointers of blocks and list iterators.
	 *
	 * The iterator points to the block's position in the LRU order list.
	 */
	robin_hood::unordered_map<std::vector<uint8_t>,
		std::pair<std::shared_ptr<CBlock>, std::list<std::vector<uint8_t>>::iterator>> mBlockUnorderedCacheMap;

	/**
	 * @brief List to keep track of LRU order.
	 *
	 * Most recently used blocks are at the back of the list.
	 */
	std::list<std::vector<uint8_t>> mBlockUnorderedCacheOrder;

	// Detached Cache - END
	 
	// Historical data structures and mutexes
	std::deque<double> historicalGroupActivity;
	size_t historicalWindowSize = 100; // Example window size
	std::mutex historicalDataMutex;

	// Variables for Welford's algorithm with exponential decay for group activity
	double groupActivityMean = 0.0;
	double groupActivityM2 = 0.0;
	double groupActivityWeight = 0.0;
	size_t groupActivityCount = 0;
	// Variables for Welford's algorithm with exponential decay for individual activity
	double individualActivityMean = 0.0;
	double individualActivityM2 = 0.0;
	double individualActivityWeight = 0.0;
	std::mutex individualHistoricalDataMutex;

	void setIsBlockMetaDataAvailable(bool isIt=true);
	void setIsTXMetaDataAvailable(bool isIt = true);
	std::shared_mutex mSharedGuardian;
public:



	bool getIsBlockMetaDataAvailable();
	bool getIsTXMetaDataAvailable();
	// Block ID -> Block Height Cache - BEGIN
		 /**
		 * @brief Adds a block to the BlockID to Height map.
		 * @param blockID The unique identifier of the block. Must be 32 bytes long.
		 * @param height The height of the block in the chain.
		 * @return True if the block was successfully added, false if the blockID is invalid.
		 */
	bool addToBlockIDHeightMap(const std::vector<uint8_t>& blockID, uint64_t height);

	/**
	 * @brief Removes a block from the BlockID to Height map.
	 * @param blockID The unique identifier of the block to remove. Must be 32 bytes long.
	 * @return True if the block was found and removed, false if not found or blockID is invalid.
	 */
	bool removeFromBlockIDHeightMap(const std::vector<uint8_t>& blockID);

	/**
	 * @brief Retrieves the height of a block given its ID.
	 * @param blockID The unique identifier of the block. Must be 32 bytes long.
	 * @return The height of the block, or UINT64_MAX if not found or blockID is invalid.
	 * Notice: getBlockIDAtDepth() provides reverse functionality.
	 */
	bool getBlockHeightFromBlockIDCache(const std::vector<uint8_t>& blockID, uint64_t& blockHeight) const;

	/**
	 * @brief Synchronizes the BlockID to Height map with the flat cache.
	 * This method should be called after significant changes to the blockchain structure.
	 * [ WARNING ]: this method requires in-RAM cache to be populated beforehand.
	 */
	void syncBlockIDHeightMap();

	/**
	 * @brief Updates the BlockID to Height map when a new block is added to the chain.
	 * This method should be called whenever a new block is added to the blockchain.
	 * @param block The newly added block.
	 * @return True if the update was successful, false otherwise.
	 * [ WARNING ]: this method requires in-RAM cache to be populated beforehand.
	 */
	bool updateBlockIDHeightMapOnNewBlock(const std::shared_ptr<CBlock>& block);


	/// <summary>
	/// Gets the last Core version spotted on chain.
	/// Thread-safe read access using shared mutex.
	/// Initially set to current Core version until blockchain processing begins.
	/// </summary>
	/// <returns>Last spotted Core version number</returns>
	uint64_t getLastSpottedCoreVersion();


	/// <summary>
	/// Updates the last Core version spotted on chain.
	/// Thread-safe write access using shared mutex.
	/// Called during block processing to track highest observed version.
	/// </summary>
	/// <param name="version">Core version from processed block</param>
	void setLastSpottedCoreVersion(uint64_t version);

	/// <summary>
	/// Updates the last Core version spotted on chain.
	/// Thread-safe write access using shared mutex.
	/// Called during block processing to track highest observed version.
	/// </summary>
	/// <param name="version">Core version from processed block</param>

	void clearBlockIDHeightMap();

	/**
	 * @brief Updates the BlockID to Height map when the blockchain is reorganized.
	 * This method should be called after a blockchain reorganization.
	 * @param newChainTip The new tip of the blockchain after reorganization.
	 * [ WARNING ]: this method requires in-RAM cache to be populated beforehand.
	 */
	void updateBlockIDHeightMapOnReorg(const std::shared_ptr<CBlock>& newChainTip);
	// Block ID -> Block Height Cache - END

	// Flat Cache Support - BEGIN

	/**
	 * Flat Cache Rationale and Management
	 * ===================================
	 *
	 * Purpose:
	 * The flat cache system, comprising mBlockCacheFlat and mKeyBlockCacheFlat, is designed
	 * to provide extremely fast lookups for blocks based on their height or key-height.
	 * This caching mechanism significantly improves performance for height-based queries,
	 * which are common operations in blockchain systems.
	 *
	 * Structure:
	 * 1. mBlockCacheFlat: A vector of weak_ptr<CBlock> indexed by block height.
	 * 2. mKeyBlockCacheFlat: A vector of weak_ptr<CBlock> indexed by key-height (for key blocks only).
	 *
	 * Performance Benefits:
	 * - O(1) time complexity for block retrieval by height or key-height.
	 * - Avoids traversing the two-way connected blockchain list for height-based queries.
	 * - Enables efficient range queries and blockchain analysis operations.
	 *
	 * Memory Management:
	 * The flat caches use std::weak_ptr to reference blocks. This design choice offers several advantages:
	 * 1. Automatic Memory Management: Weak pointers do not prevent blocks from being deallocated
	 *    when they're no longer needed elsewhere in the system.
	 * 2. Consistency: The cache automatically reflects the current state of the blockchain.
	 *    If a block is removed from the main blockchain structure, its corresponding weak_ptr
	 *    in the cache will expire.
	 * 3. No Explicit Cleanup: Due to the use of weak pointers, there's no need for explicit
	 *    cache invalidation or cleanup operations when blocks are removed from the blockchain.
	 *
	 * Integration with Blockchain Management:
	 * The flat caches work in tandem with the main two-way connected blockchain list:
	 * - When new blocks are added to the blockchain, they are also added to the flat cache(s).
	 * - The external block management mechanics (operating on the two-way connected list)
	 *   ensure the validity of blocks referenced in the flat caches.
	 * - No additional synchronization is needed between the flat caches and the main blockchain
	 *   structure due to the use of weak pointers.
	 *
	 * Maintenance:
	 * The only maintenance required for the flat caches is truncation, which should be performed
	 * after setting a new blockchain leader (i.e., after a reorganization):
	 * - Use the truncateFlatCache method to adjust the size of the caches.
	 * - This ensures that the flat caches only contain blocks that are part of the current
	 *   valid blockchain.
	 *
	 * Thread Safety:
	 * Operations on the flat caches are protected by a mutex (mBlockCacheFlatMutex) to ensure
	 * thread-safe access and modifications.
	 *
	 * In summary, the flat cache system provides a high-performance, low-maintenance solution
	 * for block lookups, leveraging C++'s memory management features to maintain consistency
	 * with the underlying blockchain structure.
	 */

	 /**
	 * @brief Retrieve a block from the flat cache.
	 *
	 * @param index The index of the block to retrieve.
	 * @param isDepth If true, the index is treated as depth, otherwise as height.
	 * @param isKeyHeight If true, the index is treated as a key height.
	 * @param chain The chain proof to use for calculations.
	 * @return std::shared_ptr<CBlock> The retrieved block, or nullptr if not found.
	 */
	std::shared_ptr<CBlock> getBlockFlatCache(
		size_t index,
		bool isDepth,
		bool isKeyHeight,
		eChainProof::eChainProof chain = eChainProof::verifiedCached
	);

	/**
	 * @brief Retrieve a range of blocks from the flat cache.
	 *
	 * @param start The starting index.
	 * @param end The ending index.
	 * @param isDepth If true, indices are treated as depths, otherwise as heights.
	 * @param isStartKeyHeight If true, the start index is treated as a key height.
	 * @param isEndKeyHeight If true, the end index is treated as a key height.
	 * @param chain The chain proof to use for calculations.
	 * @return std::vector<std::shared_ptr<CBlock>> Vector of retrieved blocks.
	 */
	std::vector<std::shared_ptr<CBlock>> getBlockRangeFlatCache(
		size_t start,
		size_t end,
		bool isDepth,
		bool isStartKeyHeight,
		bool isEndKeyHeight,
		eChainProof::eChainProof chain = eChainProof::verifiedCached
	);

	void clearFlatCache(bool releaseMemory = true);

	/**
	 * @brief Set a block in the flat cache(s).
	 *
	 * This method attempts to insert a block into the flat cache(s). For regular blocks,
	 * it inserts into mBlockCacheFlat. For key blocks, it inserts into both mBlockCacheFlat
	 * and mKeyBlockCacheFlat.
	 *
	 * IMPORTANT CONSTRAINTS:
	 * 1. The method does NOT support out-of-order block injection.
	 * 2. Any provided block needs to be either at the boundary of the cache or form a contiguous chain.
	 * 3. For key blocks, it must have an immediate existing neighbor in terms of key-height in mKeyBlockCacheFlat.
	 * 4. For all blocks, it must have an immediate existing neighbor in terms of height in mBlockCacheFlat.
	 *
	 * BEHAVIOR:
	 * - If inserting into an empty cache, the cache will be resized to accommodate the block's height/key-height.
	 * - For non-empty caches, the block must be adjacent to at least one existing block.
	 * - If inserting a key block fails in either cache, the operation is reverted in both caches.
	 *
	 * EDGE CASES:
	 * - Inserting the Genesis Block (height 0) is allowed in an empty cache.
	 * - Inserting at the end of the cache is allowed if it's exactly one height/key-height greater than the last block.
	 *
	 * NOTE: For full reordering, clearFlatCache() must be called first, followed by repopulation of the entire flat cache.
	 *
	 * @param block The block to be set. Must not be nullptr.
	 * @return eSetBlockCacheResult::eSetBlockCacheResult indicating the result. Result may be translated with CTools::getSetBlockResultDescription()
	 */
	eSetBlockCacheResult::eSetBlockCacheResult setBlockFlatCache(std::shared_ptr<CBlock> block);

	/**
	 * @brief Clear an entry in the flat cache.
	 *
	 * @param index The index of the entry to clear.
	 * @param isDepth If true, the index is treated as depth, otherwise as height.
	 * @param isKeyHeight If true, the index is treated as a key height.
	 * @param chain The chain proof to use for calculations.
	 */
	void clearEntryFlatCache(
		size_t index,
		bool isDepth,
		bool isKeyHeight,
		eChainProof::eChainProof chain = eChainProof::verifiedCached
	);

	/**
	 * @brief Validate the integrity of the flat cache.
	 *
	 * @return bool True if the cache is valid, false otherwise.
	 */
	bool validateFlatCache() const;

	/**
	 * @brief Truncates the flat cache to a specified size.
	 *
	 * This method reduces the size of the main flat cache (mBlockCacheFlat) to the specified new size.
	 * Optionally, it can also adjust the key block cache (mKeyBlockCacheFlat) to maintain consistency
	 * with the truncated main cache.
	 *
	 * @param newSize The desired new size for the main flat cache. If this value is greater than or
	 *                equal to the current size of mBlockCacheFlat, no truncation occurs.
	 *
	 * @param truncateKeyCache If true, the method will also adjust the size of mKeyBlockCacheFlat
	 *                         to ensure it only contains key blocks that are still present in the
	 *                         truncated main cache. If false, mKeyBlockCacheFlat remains unchanged.
	 *
	 * Behavior:
	 * 1. Always truncates mBlockCacheFlat to newSize if newSize is smaller than the current size.
	 * 2. If truncateKeyCache is true:
	 *    - Scans the truncated mBlockCacheFlat to find the highest key block still present.
	 *    - Resizes mKeyBlockCacheFlat to contain only key blocks up to this height.
	 *    - If no key blocks are found in the truncated range, mKeyBlockCacheFlat is resized to 0.
	 *
	 * Thread safety:
	 * This method is thread-safe. It uses a mutex (mBlockCacheFlatMutex) to ensure exclusive access
	 * during the truncation process.
	 *
	 * Edge cases:
	 * - If newSize >= current size of mBlockCacheFlat, no changes are made to either cache.
	 * - If mBlockCacheFlat becomes empty after truncation, mKeyBlockCacheFlat will also become empty
	 *   if truncateKeyCache is true.
	 * - If truncateKeyCache is false, mKeyBlockCacheFlat may contain references to blocks no longer
	 *   in mBlockCacheFlat after truncation.
	 *
	 * Note:
	 * After calling this method with truncateKeyCache set to false, the key block cache may be
	 * inconsistent with the main cache. It's the caller's responsibility to handle this situation
	 * if needed.
	 */
	void truncateFlatCache(size_t newSize, bool truncateKeyCache = true);

	/**
	 * @brief Get the size of the flat cache.
	 *
	 * @param getKeyCache If true, return the size of the key block cache instead of the regular cache.
	 * @return size_t The size of the specified cache.
	 */
	size_t getFlatCacheSize(bool getKeyCache = false) const;
	// Flat Cache Support - END
	void setIsRestartNeeded(bool isIt = true);
	uint64_t getLastTimeBlockCacheCleaned();
	void pingLastTimeBlockCacheCleaned();
	std::string getDefaultRealmName();
	std::shared_ptr<CGridScriptCompiler> getCompiler();
	eBlockchainMode::eBlockchainMode getMode();
	uint64_t getKnownStateDomainsCount();
	uint64_t getForksCount();
	static enum eBlockProcessingResult { appended, justSaved, discarded, error };
	std::string translateVerificationResult(eBlockVerificationResult::eBlockVerificationResult res);
	std::shared_ptr<CTools> getTools();
	bool getIsValidPointer();
	void invalidatePointer();
	bool pushBlock(std::shared_ptr<CBlock> block, bool byLocalScheduler = false);
	bool pushBlock(std::vector<uint8_t> BERBlock, bool byLocalScheduler = false);
	bool getBlockQueueLength(uint64_t& length, bool waitForData = true);
	uint64_t getBlockQueueMaxLength();
	void setBlockQueueMaxLength(uint64_t size);
	bool wasBlockProcessed(std::vector<uint8_t> blockID);
	std::vector<CVerifiable> getProofsOfFraufForLeader(std::vector<uint8_t> pubKey);
	std::shared_ptr<CBlock> getLatestDataBlockForKeyLeader(std::vector<uint8_t> pubKey, uint64_t maxSearch = 1000, bool useColdStorage = true);

	//Proof-of-Fraud related mechanics
	bool logBlock(std::shared_ptr<CBlock> block, std::vector<uint8_t> keyBlockID = std::vector<uint8_t>());
	eFraudCheckResult::eFraudCheckResult checkForFraud(std::shared_ptr<CBlock> blockProposal, CVerifiable& proofOfFraud, Botan::secure_vector<uint8_t> rewardeesPrivKey = Botan::secure_vector<uint8_t>());
	void updateStatysticsToFile(bool memPoolStatsOnly = false);
	bool getJustAppendedLeader();
	uint64_t getCachedHeaviestHeight();
	void setCachedHeaviestHeight(uint64_t height);
	uint64_t getCachedHeight(bool keyHeight = false);
	void setCachedHeight(uint64_t value, bool keyHeight = false);
	bool doIhaveAChanceOfWinning();
	uint64_t getCachedBlockQueueLength();
	void  setCachedBlockQueueLength(uint64_t length);

	// Declaration of member functions for insertion, query and removal of black-listed block identifiers
	bool blacklistBlock(const std::vector<uint8_t>& blockIdentifier, uint64_t seconds = 0);
	bool isBlacklisted(const std::vector<uint8_t>& blockIdentifier);
	void unblacklistBlock(const std::vector<uint8_t>& blockIdentifier);
	void incLocalTotalRewardBy(BigInt value);
	BigInt getLocalTotalReward();
	uint64_t getLocallyMinedKeyBlocks();
	void incLocallyMinedKeyBlocks();
	uint64_t getLocallyMinedDataBlocks();
	void incLocallyMinedDataBlocks(); void incTXProcessedLocally(uint64_t by = 1);
	uint64_t getReportedDifficulty();
	void logWarning(const std::string& text, const std::string& source = "");
	void clearWarnings();
	uint64_t getTXProcessedLocally();
	std::vector<WarningInfo> CBlockchainManager::getWarnings();
	static void missionAbort();
	static bool getIsMissionAbort();

	// Receipts Cache - BEGIN

	/**
	 * @brief Set the maximum size of the receipts cache.
	 * @param size The new maximum size in bytes.
	 * @throws std::invalid_argument if size is 0.
	 */
	void setMaxReceiptsCacheSize(size_t size);

	/**
	* @brief Forcefully clears the entire receipts cache.
	*
	* This method immediately removes all entries from the cache,
	* resets the cache size to zero, and updates the last cleanup time.
	* It's thread-safe and can be called from any context where
	* immediate cache clearance is required.
	*
	* @note This operation may be expensive if the cache is large,
	*       as it needs to acquire an exclusive lock on the cache.
	*/
	void forceClearReceiptsCache() noexcept;

	size_t receiptsHotCacheCleanupK(bool force = false);

	/**
	 * @brief Gets the maximum size of the receipts cache.
	 */
	size_t getMaxReceiptsCacheSize();

	size_t getMaxBlockchainCacheSize();

	/**
	 * @brief Get the current size of the receipts cache.
	 * @return The current size in bytes.
	 */
	size_t getReceiptsCacheSize() const noexcept;

	/**
	* @brief Retrieve a receipt from the cache.
	* @param id The unique identifier of the receipt.
	* @return A shared pointer to the receipt if found, nullptr otherwise.
	*/
	std::shared_ptr<CReceipt> getReceiptFromHotCache(const std::vector<uint8_t>& id) const;


	/**
	 * @brief Add a receipt to the cache.
	 * @param receipt A shared pointer to the receipt.
	 * @throws std::invalid_argument if receipt is nullptr.
	 */
	void addReceiptToHotCache(const std::shared_ptr<CReceipt>& receipt);

	/**
	* @brief Perform a cleanup of the receipts cache, removing old entries.
	* @param force If true, performs a cleanup regardless of the time since the last cleanup.
	* @return The number of entries removed from the cache.
	*/
	size_t receiptsHotCacheCleanup(bool force = false);
	// Receipts Cache - END


	std::shared_ptr<CBreakpointFactory> getBreakpointFactory();
	ExclusiveWorkerMutex mHeaviestPathGuardian;
	ExclusiveWorkerMutex mHeaviestPathDoubleGuardian;//The point of double-buffering is to allow for efficient responses to the outside world
	ExclusiveWorkerMutex mVerifiedPathDoubleGuardian;//but also for swift reporting.
	ExclusiveWorkerMutex mVerifiedPathGuardian;
	ExclusiveWorkerMutex mCacheOperationGuardian; // may be either read or write (both shared and unqiue locks supported).
private:
	// Stats - BEGIN
	mutable std::shared_mutex mStatsDataGuardian;
	BigInt mRecentTotalRewardCache;
	uint64_t mRecentTotalRewardTimestamp ;
	double mNetworkUtilizationCache;
	uint64_t mNetworkUtilizationTimestamp;
	uint64_t mBlockSizeCache;
	uint64_t mBlockSizeTimestamp;
	uint64_t mAverageBlockTimeCache;
	uint64_t mAverageKeyBlockTimeCache;
	uint64_t mAverageBlockTimeTimestamp;
	uint64_t mAverageKeyBlockTimeTimestamp ;
	const uint64_t CACHE_VALIDITY_PERIOD = 600; // 10 minutes in seconds
	// Stats - END

	uint64_t mLastSpottedCoreVersion;
	mutable std::shared_mutex mBlockIDMapGuardian;
	// Receipts Cache - BEGIN
	struct CacheEntry {
		std::shared_ptr<CReceipt> receipt;
		size_t size;
		std::chrono::steady_clock::time_point lastAccessed;
	};

	mutable std::shared_mutex mReceiptsCacheMutex;
	mutable robin_hood::unordered_map<std::vector<uint8_t>, CacheEntry> mReceiptsCache;
	mutable std::atomic<size_t> mReceiptsCacheSize;
	std::atomic<size_t> mMaxReceiptsCacheSize;
	std::atomic<size_t> mMaxBlockchainCacheSize;
	mutable std::deque<std::vector<uint8_t>> mReceiptsCacheOrder;
	mutable std::chrono::steady_clock::time_point mLastCacheCleanup;

	void removeOldestReceipt();
	size_t estimateReceiptSize(const CReceipt& receipt) const noexcept;
	// Receipts Cache - END
	static bool sMissionAbort;
	static std::mutex sFieldsGuardian;
	bool mTXMetaDataAvailable;
	bool mDoNotProcessExternalBlocks;
	bool mBlockMetaDataAvailable;
	robin_hood::unordered_map<uint64_t, WarningInfo> mWarnings;
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t> mBlockID_BlockHeightMap;
	uint64_t mLocallyMinedKeyBlocks;
	uint64_t mLocallyMinedDataBlocks;
	uint64_t mTXProcessedLocally;
	BigInt mTotalLocalReward;
	bool mJustAppendedLeader;
	std::weak_ptr<CBlock> mDeepestBlockInCache;
	std::vector<std::weak_ptr<CBlock>> mBlockCacheFlat, mKeyBlockCacheFlat;
	std::recursive_mutex mDiffCoefficientsGuardian;
	uint64_t mVerifiedChainProofTotalDiff;//cached
	uint64_t mHeaviestChainProofTotalDiff;//cached
	uint64_t mCurrentHeight;//cache
	uint64_t mCurrentKeyHeight;//cache
	uint64_t mHeaviestHeight;//cache
	bool isBlockInProcessingQeueue(std::vector<uint8_t>& blockID);
	std::mutex mStatusChangeGuardian;
	void addBlockToProcessedBlocks(std::vector<uint8_t> blockID);
	uint64_t mBlockQueueMaxLength;
	mutable std::mutex mFieldsGuardian;
	mutable std::shared_mutex mSharedFieldsGuardian;
	std::mutex mVerifiedKeyBlocksIndexGuardian;
	std::mutex mHeaviestKeyBlocksIndexGuardian;
	std::mutex mWarningsGuardian;
	std::mutex mBlacklistedBlocksGuardian;
	std::shared_ptr<CBlockHeader> mHeaviestChainProofLeaderKeyBlock, mHeaviestChainProofLeaderBlock;
	std::vector<uint8_t> mHeaviestChainProofLeadingBlockID;
	std::shared_ptr<CBlock> popBlock();
	std::mutex mBlockLimitGuardian;
	std::recursive_mutex blockQueueGuardian;
	std::mutex processedBlockIDsQueueGuardian;
	std::deque<std::shared_ptr<CBlock>> mBlockQueue;
	uint64_t mCachedBlockQueueLength;

	//std::priority_queue<std::shared_ptr<CBlock>, std::vector<std::shared_ptr<CBlock>>, CompareBlocksByDiff> mBlockQueue;

	std::vector<std::vector<uint8_t>> mProcessedBlockIDsQueue;
	std::mutex mIsValidPointerGuardian;
	bool mIsValidPointer;
	bool mSkipLocalDataAnalysis;
	bool mNetworkTestingMode;
	CBlockVerificationResult doCommonVerification(std::shared_ptr<CBlock> block);
	CBlockVerificationResult doKeyPartVerification(std::shared_ptr<CBlock> block, CBlockVerificationResult res);
	bool immuniseAgainstBlock(const std::shared_ptr<CBlock>& block, const uint64_t blacklistDurationSeconds = 60 * 15);
	bool setEffectivePerspective(const std::vector<uint8_t> perspective, const std::vector<uint8_t>& blockID);

	CBlockVerificationResult doDataPartVerification(std::shared_ptr<CBlock> block, CBlockVerificationResult);
	std::shared_ptr<CBreakpointFactory> mBreakpointFactory;
	
	bool mIsNumberOfStateDomainsFresh;
	uint64_t mCustomStatusBarFlashedTimestamp;
	std::string mCustomStatusBarText;
	bool mTestTrieAfterBlockProcessing;
	bool registerReceiptWithinCache(const CReceipt& rec, const std::vector<uint8_t>& blockID, bool useHotStorage = true);
	uint64_t mMaxReceiptsIndexCacheSize;
	uint64_t mNrOfCryticalErrors;

	size_t mMaxNrOfOverallBlocksToKeepInMemory;
	size_t mMaxNrOfFullBlocksToKeepInMemory;
	size_t mMaxNrOfPrunedBlocksToKeepInMemory;//with pruned DBs

	//synchronization constructs:
	static std::mutex sFactoryProductionLockGuardian;
	std::recursive_mutex mLiveStateDBGuardian;
	std::recursive_mutex mLeaderGuardian;
	ExclusiveWorkerMutex mChainGuardian; // todo: we would rather transition towards a shared_mutex and prevent nested locks
	std::recursive_mutex mHuntedBlocksGuardian;
	std::recursive_mutex mGuardian;
	std::recursive_mutex mStatusGuardian;
	std::mutex mProcessingStatusGuardian;
	std::mutex mModeGuardian;
	uint64_t mLastHeaviestChainproofSync = 0;
	std::mutex mToolsGuardian;
	const double DEFAULT_DIFFICULTY = 1.0;
	const double PROHIBITIVE_DIFFICULTY = double((uint64_t)(-1));

	std::recursive_mutex mMinERGPriceGuardian;
	std::recursive_mutex mReceiptsGuardian;
	std::recursive_mutex mBlockProcessingGuardian;
	std::recursive_mutex mLocalBlockAvailabilityGuardian;
	bool mVitalsMonitorRunning;
	bool mVitalsMinitorIsToBeRunning;
	std::thread mController, mVitalsMonitor;
	arith_uint256 mMinDifficulty;
	std::shared_ptr<CTools> mTools;
	std::shared_ptr<CCryptoFactory> mCryptoFactory;
	static bool mAlreadyInstantiated;
	std::shared_ptr<CGridScriptCompiler> mCompiler;
	std::shared_ptr<CNetworkManager> mNetworkManager;
	CSolidStorage* mSolidStorage;
	bool mRestartNeeded;
	//SE::CScriptEngine * mScriptEngine;
	std::shared_ptr<CTransactionManager> mLiveTransactionsManager;// used to deliver information to the outside environment 
	std::shared_ptr <CTransactionManager> mVerificationFlowTransactionsManager;// to participate in Flow mechanics (for block verification only)
	std::shared_ptr <CTransactionManager> mBlockFormationFlowManager; //to make Flow mechanics of Block Validation and Formation separate.
	std::shared_ptr <CTransactionManager> mTerminalTransactionsManager;//available to Terminal services only
	//std::shared_ptr<CStateDomainManager> mDomainManager;
	std::shared_ptr<CWorkManager> mWorkManager;
	CTests* mTests;

	std::shared_ptr<CSettings> mSettings;

	uint64_t mForksCount;
	uint64_t mDiscardedKeyBlocksCount;
	uint64_t mDiscardedDataBlocksCount;

	uint64_t mJustSavedKeyBlocksCount;
	uint64_t mJustSavedDataBlocksCount;

	uint64_t mAssumedAsLeaderDataBlocksCount;
	uint64_t mAssumedAsLeaderKeyBlocksCount;

	uint64_t mAverageKeyBlockInteval;
	uint64_t mAverageDataBlockInteval;
	std::mutex mStatisticsDataGuardian;

	//paths - sequence of block IDs
	std::vector <std::vector<uint8_t>> mVerifiedPath, mVerifiedPathDouble;
	std::vector < std::vector<uint8_t>> mHeaviestPath, mHeaviestPathDouble;

	//chain-proofs - sequences of block headers
	std::vector <uint64_t> mHeaviestChainProofCumulativePoWs;//optimization - protected by same mutex as the one below
	//^- each entry in the sequence represents cumulative PoW up to the point represented by index within the vector.

	std::vector <std::vector<uint8_t>> mHeaviestChainProof;//todo: introduce double buffering of heaviest chain proof?
	std::vector <std::vector<uint8_t>> mVerifiedChainProof, mVerifiedChainProofDouble;
	uint64_t mSynchronizationPercentage;

	//largest cumulated PoW constitute this global chainProof. The corresponding block MIGHT NOT be available locally.
	//it is the task of the NetworkManager to keep everything in sync.
	//responses to inquiries about the latest block are generated based on this data structure.

	CTrieDB* mLiveStateDB;//NOT a Flow DB. Contains only data commited to cold storage.always stable and valid.
	//see Transaction's Manager mFlowDB.
	// Important: the above DB is pruned most of the time but for the initial initialization during bootstrap sequence.
	//			- For any public access the DB of LIVE-Transaction Manager is used most of the time.
	//			- For transactions' processing (during a Flow) a Flow-Transaction Manager is used.
	//		     - For high throughout search queries we employ Peristen DB of LIVE Transaction Manager


	bool mLocalNodeIsLeaderCached = false;
	bool mIsTestNet = false;

	// ============================================================================
	// Phantom Leader Mode - BEGIN
	// ============================================================================
	// [ Purpose ]: Phantom Leader Mode enables transaction processing and data block
	//              formation debugging even when the local node is NOT the current Leader.
	//              Blocks formed in Phantom Mode are NOT broadcasted to the network.
	//              This mode is intended for diagnosing transaction processing issues
	//              such as transaction withholding attacks or software bugs.
	// [ Security ]: This mode can only be enabled from local Terminal with security
	//               credentials. It does NOT affect real blockchain state or consensus.
	// ============================================================================
	bool mPhantomLeaderModeEnabled = false;
	std::mutex mPhantomLeaderModeGuardian;
	uint64_t mPhantomBlocksFormedCount = 0;
	uint64_t mPhantomTransactionsProcessedCount = 0;
	uint64_t mLastPhantomBlockFormationTime = 0;
	// Phantom Leader Mode - END
	// ============================================================================

	//std::vector<uint8_t> mLeaderID; <= this got removed. there's no need to store the BlockLeader ID separately.
	//current leader is dictated by the last header  in a chainProof or the last Hash of block header within the LongestPath
	std::shared_ptr<CBlock> mCachedLeader, mCachedKeyLeader;//CANNOT be used during a Flow.
	std::shared_ptr<CBlock> mLeader; //this can be either the current key-block or regular block
	std::shared_ptr<CBlock> mKeyLeader; // this is the current leading key-block (might not be the top of the chain)

	CBlockVerificationResult validateBlock(std::shared_ptr<CBlock> block);



	CBlockVerificationResult validateBlock(std::vector<uint8_t> packedBERBlock);

	size_t mLastTimeValidBlockProcessed;

	void mControllerThreadF();

	void processHeaviestChainProof(eBlockchainMode::eBlockchainMode mode, uint64_t& justPushedFromHeaviestChainProof, uint64_t& blocksScheduleForDownload, std::shared_ptr<CTools>& tools);



	std::vector<std::shared_ptr<CBlock>> blocksForProcessing;
	static bool sTestNetInstanceInited, sLiveInstanceInited, sTestSandBoxInstanceInited, sLIVESandBoxInstanceInited, sLocalTestsInstanceInited;

	static std::recursive_mutex  sStaticPointersGuardian;
	static std::shared_ptr<CBlockchainManager> sTestNetInstance;
	static std::shared_ptr<CBlockchainManager> sLiveInstance;
	static std::shared_ptr<CBlockchainManager> sTestSandBoxInstance;
	static std::shared_ptr<CBlockchainManager> sLIVESandBoxInstance;
	static std::shared_ptr<CBlockchainManager> sLocalTestsInstance;
	eManagerStatus::eManagerStatus mStatus;
	eBlockProcessingStatus::eBlockProcessingStatus mProcessingStatus;
	eManagerStatus::eManagerStatus mStatusChange;

	eManagerStatus::eManagerStatus getStatusChange();
	void setStatusChange(eManagerStatus::eManagerStatus status);
	void updateVersionToFile();
	bool mIsSynced;
	std::mutex mHeaviestChainProofTotalDifficultyGuardian;
	std::mutex mVerifiedChainProofTotalDifficultyGuardian;
	uint64_t mHeaviestChainProofTotalDifficulty;
	uint64_t mVerifiedChainProofTotalDifficulty;
	std::unordered_map<std::vector<uint8_t>, std::chrono::system_clock::time_point> mBlacklistedBlocks;

	//^ explained in:https://talk.gridnet.org/t/proactive-resilience-enhancing-gridnet-core-in-response-to-unconventional-mining-practices/240
	// We need to support a case in which node is being suffocated by a heaviest chain proof which is invalid by the current implementation
	// while yet still there is no checkpoint added to the system by the team.
	// the software need to be fully autonomous
	robin_hood::unordered_map<std::vector<uint8_t>, std::shared_ptr<CReceipt>> mReceipts;// high speed cache for receipts
	robin_hood::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>> mReceiptsBlocksIndex;
	//robin_hood::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>> mKeyBlockIDsAtHeights;//hashed height=>blockID
	std::vector< std::vector<uint8_t>> mVerifiedKeyBlockIDsAtHeights, mHeaviestKeyBlockIDsAtHeights;
	std::mutex mBlockProcessingDelaysByBlockIDGuardian;
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t> mBlockProcessingDelaysByBlockID;//used for testing ONLY.
	uint64_t mHotStorageBlockCacheSize;
	robin_hood::unordered_map<std::vector<uint8_t>, bool> mLocalBlockAvailability;
	eBlockchainMode::eBlockchainMode mMode;
	std::recursive_mutex mIsReadyGuardian;
	void setBlockProcessingStatus(eBlockProcessingStatus::eBlockProcessingStatus status);
	eBlockProcessingStatus::eBlockProcessingStatus getBlockProcessingStatus();
	uint64_t mKeyBlockFormationLimit;
	uint64_t mDataBlockFormationLimit;
	std::mutex mAchievablePerspectiveAtIndexGuardian;
	uint64_t mAchievablePerspectiveAtIndex; //points to an index within mHeaviestChainProof whose perspectives is achievable locally.
	uint64_t mTotalPoWAvailableAtAchievableIndex;
	uint64_t mDefaultDerouteDelay;


	bool mIssueHeaviestChainProofReset;
	std::mutex mIssueHeaviestChainProofResetGuardian;
	bool resetHeaviestChainProof();
	uint64_t mCommitedHeaviestChainProofBlocksCount;
	std::mutex mCommitedHeaviestChainProofBlocksCountGuardian;
	std::vector<uint8_t> mLocalPeerID;
	std::mutex mLocalPeerIDGuardian;
	std::vector<uint8_t> getLocalPeerId();//INTERNAL use only; getCurrentID() for extrinsic usage
	void setLocalPeerId(std::vector<uint8_t> id);
	uint64_t mLastTimeStatsToFile;
	void pingLastTimeStatsToFile();
	uint64_t getLastTimeStatsToFile();
	std::mutex mRecentlyProcessedChainProofsGuardian;
	uint64_t mRecentlyProcessedChainProofsClearedTimestamp;
	std::unordered_map<std::vector<uint8_t>, uint64_t> mRecentlyProcessedChainProofs;
	bool mAllVitalsLookingGood;
	size_t mLastVitalsRefresh;
	std::string mVitalsStrCache;
	bool mSyncStateMachine;

	// ============================================================================
	// Transaction Withholding Detection (Liveness Monitoring) - BEGIN
	// ============================================================================
	// [ Purpose ]: Detects potential transaction withholding attacks where Leaders
	//              produce key blocks but fail to process pending transactions.
	//              Issues warnings when transactions remain stuck in mem-pool across
	//              multiple key block epochs.
	// [ Threshold ]: Warning issued if transactions wait for >= 3 key blocks.
	// ============================================================================
	std::mutex mTxWithholdingDetectionGuardian;
	uint64_t mKeyHeightAtLastTxWithholdingCheck = 0;
	uint64_t mLastTxWithholdingWarningTime = 0;
	uint64_t mTxWithholdingWarningCount = 0;
	static constexpr uint64_t TX_WITHHOLDING_KEY_BLOCK_THRESHOLD = 3;
	static constexpr uint64_t TX_WITHHOLDING_WARNING_COOLDOWN_SEC = 60;
	// Transaction Withholding Detection - END
	// ============================================================================
	uint64_t mProcessingLongChainProof;
	std::vector<std::shared_ptr<CBCheckpoint>> mCheckpoints;
	void clearCheckpoints();

	void loadCheckpoints();

	std::mutex mCheckpointsGuardian;
	uint64_t mVPInColdStorageBehindCount;
	uint64_t mBPM, mAPM;
	uint64_t mLastBPMCleaned;
	uint64_t mFarAwayCPRequestedTimestamp;
	uint64_t mLastControllerLoopRun;
	bool mSyncIsStuck;

	//Hot Storage Block stats - BEGIN
	std::vector<uint64_t> mProcessedBlockTimestamps;
	std::vector<uint64_t> mAppendedBlockTimestamps;
	//Hot Storage Block stats - END


	//ColdStorage Sync Timestamps - BEGIN
	uint64_t mVerfiedChainCSSyncTimestamp;
	uint64_t mHeaviestChainCSSyncTimestamp;
	uint64_t mCachedFilledInStatusBarID = 0;
	//ColdStorage Sync Timestamps - END
	bool mOperatorLeadingAHardFork;
	uint64_t mVerfiedSyncedWithCSAtHeight;
	bool mLeadingAlternativeHistory;
	
	// Transactions Count Daily Stats - BEGIN

	/** @brief Mutex to protect access to recent transactions. */
	mutable std::shared_mutex mRecentTransactionsGuardian;

	/** @brief The maximum number of recent transactions to keep. */
	uint64_t mMaxRecentTransactions = 10000;

	/** @brief The recent transactions cache. */
	std::deque<std::shared_ptr<CTXInfo>> mRecentTransactions;

	/** @brief A hash map to quickly access transactions by their hash. */
	robin_hood::unordered_map<std::vector<uint8_t>, std::shared_ptr<CTXInfo>> mRecentTransactionsMap;

	/** @brief A hash map to quickly access transactions by their receipt ID. */
	robin_hood::unordered_map<std::vector<uint8_t>, std::shared_ptr<CTXInfo>> mRecentTransactionsByReceiptID;


	// Transactions Count Daily Stats - BEGIN

	/**
	 * @brief Structure to cache transaction counts for date ranges.
	 *
	 * This structure stores the start and end dates, the transaction count,
	 * and the last time the cache was updated. It helps in providing quick
	 * responses for transaction count queries over specific date ranges.
	 */
	struct TransactionCountCache {
		uint64_t startDate;
		uint64_t endDate;
		uint64_t count;
		uint64_t lastUpdated;
	};

	/** @brief Mutex to protect access to the transaction count cache. */
	mutable std::shared_mutex mDailyStatsCacheMutex;

	/** @brief Duration for which the transaction count cache is valid (6 hours in seconds). */
	const uint64_t CACHE_DURATION = 6 * 60 * 60;

	/** @brief Cache for transaction counts over date ranges. */
	std::vector<TransactionCountCache> mTransactionCountCache; // Notice: this case is filled-in only on per-request basis

	// Transactions Count Daily Stats - END


	// Transactions Count Daily Stats - END




public:


	// Operators Security Assessment - BEGIN
	/**
	 * @brief Updates the security analysis with a new block.
	 *
	 * This method should be called iteratively for each block in the blockchain, ideally in chronological order
	 * from the genesis block to the latest block. It performs the following analyses:
	 * - Detects timestamp manipulation attacks by analyzing transaction timestamps within data blocks.
	 * - Detects Proof-of-Work (PoW) wave attacks, including colluding operators exploiting difficulty adjustments.
	 *
	 * @param block The new block to analyze.
	 */
	void updateSecAnalysis(const std::shared_ptr<CBlock>& block);

	/**
	 * @brief Retrieves the security report with optional filtering criteria.
	 *
	 * This method returns a vector of shared pointers to `COperatorSecurityInfo` objects,
	 * representing the security analysis results for operators. It accepts optional parameters
	 * to filter the report based on the operators' activity within a specified depth (block height range)
	 * and minimum offense counts.
	 *
	 * @param depth Optional. When greater than zero, only operators active within the last 'depth' blocks are included.
	 *              Default is 0, which includes all operators.
	 * @param minTotalOffenses Optional. The minimum total offenses (timestamp manipulation count + PoW wave attack count)
	 *                         an operator must have to be included in the report. Default is 0.
	 * @param minTimestampManipulationCount Optional. The minimum timestamp manipulation offenses an operator must have.
	 *                                      Default is 0.
	 * @param minPowWaveAttackCount Optional. The minimum PoW wave attack offenses an operator must have. Default is 0.
	 * @return A vector of shared pointers to `COperatorSecurityInfo` objects that meet the specified criteria,
	 *         sorted in descending order of total offenses.
	 */
	std::vector<std::shared_ptr<COperatorSecurityInfo>> CBlockchainManager::getSecurityReport(
		uint64_t depth = 0,
		uint64_t minTotalOffenses = 0,
		uint64_t minTimestampManipulationCount = 0,
		uint64_t minPowWaveAttackCount = 0);

	/**
	* @brief Retrieves the security report for a specific operator.
	*[ Purpose ]:
		- Retrieves the security report for a specific operator.

	[ Behavior ]:
		- Acquires a shared lock on securityMapMutex for thread-safe access.
		- Searches for the operator in operatorSecurityMap.
		- Returns the operator's security information if found.
	[ Notes ]:
		- Allows for targeted analysis of individual operators.
		- Useful for monitoring specific entities within the network.

	* @param operatorID The Base58Check-encoded ID of the operator.
	* @return OperatorSecurityInfo containing the security information of the operator.
	*         If the operator is not found, an OperatorSecurityInfo with zero counts is returned.
	*/
	std::shared_ptr<COperatorSecurityInfo> getSecurityReportForOperator(const std::string& operatorID);

	
	// Operators Security Assessment - END

	// Transactions Count Daily Stats - BEGIN

	/*
	*           ---- [ Mechanics Description] ---- BEGIN
	* Overview
The caching mechanism is designed to provide extremely fast look-ups of metadata (CRecentTXInfo) related to transactions. It achieves this by maintaining the following data structures:

mRecentTransactions: A deque that stores CRecentTXInfo objects in order of recency. New transactions are added to the front, and the oldest transactions are at the back.
mRecentTransactionsMap: A hash map (robin_hood::unordered_map) that maps transaction hashes to CRecentTXInfo pointers. This allows O(1) look-ups of transaction metadata using the transaction hash.
mRecentTransactionsByReceiptID: A hash map that maps receipt IDs to CRecentTXInfo pointers. This allows O(1) look-ups of transaction metadata using the receipt ID.

[ - Operational  Details - ]
[ Adding Transactions ]
When a transaction is added via addRecentTransaction:

Check for Existence: The method checks if the transaction already exists in mRecentTransactionsMap.
[ Update Existing Entry ] :
+ If it exists, the transaction metadata is updated.
The entry is moved to the front of mRecentTransactions to reflect its recency.
+If the receipt ID has changed, mRecentTransactionsByReceiptID is updated accordingly.
Add New Entry:
+If it does not exist, a new CRecentTXInfo object is created.
- The new entry is added to the front of mRecentTransactions.
- The transaction hash and receipt ID are added to the respective hash maps.

 [ Maintaining Cache Size ] :

If adding a new transaction causes the cache to exceed mMaxRecentTransactions, the oldest transaction is removed from the back of mRecentTransactions and both hash maps.

[ Synchronization Between Data Structures ] 
All modifications to the deque and hash maps are performed within the same critical section protected by mRecentTransactionsGuardian. This ensures thread safety and keeps the data structures in sync.

Whenever a transaction is added, updated, or removed, corresponding changes are made to all data structures.
Sanity checks are performed to verify that the sizes of the deque and hash maps are consistent.

[ Lookup Operations ] 
- By Transaction Hash:
The mRecentTransactionsMap allows quick retrieval of transaction metadata using the transaction hash.
- By Receipt ID:
The mRecentTransactionsByReceiptID allows quick retrieval using the receipt ID, which is computed based on the transaction hash and blockchain mode.

[ Cache Initialization ]
The initializeRecentTransactions method populates the cache by traversing recent blocks and collecting transactions.
It ensures that duplicates are not added by checking mRecentTransactionsMap before adding a transaction.
The method populates mRecentTransactions, mRecentTransactionsMap, and mRecentTransactionsByReceiptID in sync.

					---- [ Mechanics Description] ---- BEGIN
	*/


	/**
* @brief Updates the transactions cache with a pre-computed transaction info.
*
* This method updates or adds a pre-computed CTXInfo object to the cache.
* It maintains the size constraints and updates all relevant lookup structures.
*
* @param txInfo The pre-computed transaction info to add/update in cache.
* @param unsafe If true, skips mutex locking (use only when thread safety is handled externally).
*/
	void updateTransactionCache(const std::shared_ptr<CTXInfo>& txInfo, bool unsafe=false);

	void performTransactionsCacheSanityChecks();

	/**
 * @brief Adds or updates a transaction in the recent transactions cache.
 *
 * This method adds a new transaction or updates an existing one in the recent transactions cache.
 * It maintains synchronization between the deque `mRecentTransactions` and the hash maps
 * `mRecentTransactionsMap` and `mRecentTransactionsByReceiptID`. The transaction is moved to
 * the front of the deque to reflect its recency.
 *
 * @param transaction The transaction to add or update (must not be null).
 * @param receipt The receipt associated with the transaction.
 * @param block The block containing the transaction (used for metadata generation).
 * @param genMetaData If true, generates detailed metadata for the transaction.
 * @param unsafe If true, skips mutex locking for thread-unsafe but potentially faster operation.
 *               Use with caution and only when thread safety is guaranteed by the caller.
 *
 * @note When `unsafe` is false (default), the method uses mutex locking to ensure thread safety.
 *       When `unsafe` is true, no mutex locking is performed, which can improve performance
 *       in single-threaded contexts but may lead to data races in multi-threaded environments.
 *
 * @warning Calling this method with `unsafe=true` in a multi-threaded environment without
 *          proper external synchronization can lead to data races and undefined behavior.
 *
 * The method performs the following operations:
 * 1. Generates metadata if `genMetaData` is true.
 * 2. Updates an existing transaction or adds a new one to the cache.
 * 3. Maintains the cache size, removing the oldest transaction if necessary.
 * 4. Performs sanity checks on the cache state (regardless of the `unsafe` flag).
 *
 * @throws No exceptions are thrown. Errors are logged using the mTools logger.
 */
	void CBlockchainManager::addTransactionToCache(
		const std::shared_ptr<CTransaction>& transaction,
		const std::shared_ptr<CReceipt>& receipt,
		const std::shared_ptr<CBlock>& block,
		bool genMetaData,
		bool unsafe = false
	);

/**
  * @brief Retrieves transaction information by receipt ID.
  *
  * This method allows fast retrieval of transaction metadata using the receipt ID.
  * It returns a shared pointer to the `CRecentTXInfo` object if found, or `nullptr` if not found.
  *
  * @param receiptID The receipt ID of the transaction.
  * @return A shared pointer to `CRecentTXInfo` if found, or `nullptr` if not found.
  */
	std::shared_ptr<CTXInfo> getTransactionInfoByReceiptID(const std::vector<uint8_t>& receiptID);

	/**
	 * @brief Retrieves recent transactions for pagination.
	 *
	 * This method provides paginated access to the recent transactions cache.
	 * It returns a vector of recent transactions corresponding to the specified
	 * page number and page size.
	 *
	 * @param page The page number (0-based index).
	 * @param size The number of transactions per page.
	 * @return A vector of recent transactions for the specified page.
	 */
	std::vector<std::shared_ptr<CTXInfo>> getRecentTransactionsInfoPage(uint64_t page, uint64_t size);


	// CBlockchainManager.h

/**
 * @brief Retrieves a specific range of recent transactions from the blockchain.
 *
 * This method provides paginated access to the recent transactions stored in the blockchain manager.
 * It uses a thread-safe approach to access the shared transaction data.
 *
 * Pagination Mechanism:
 * 1. Range Calculation:
 *    - Start Index: Provided as a parameter
 *    - End Index: Min(Start Index + Count, Total Transactions)
 *    - Ensures that we don't exceed the total number of stored transactions
 *
 * 2. Thread Safety:
 *    - Uses a shared lock to allow concurrent reads of the transaction data
 *    - Ensures data consistency when accessed from multiple threads
 *
 * 3. Efficiency:
 *    - Pre-allocates memory for the result vector to avoid reallocation
 *    - Uses vector::assign for efficient range copying
 *
 * 4. Edge Cases:
 *    - Returns an empty vector if the start index is out of range
 *    - Correctly handles requests that exceed the available transaction count
 *
 * @param startIndex The index of the first transaction to retrieve (0-based).
 * @param count The maximum number of transactions to retrieve.
 * @return std::vector<std::shared_ptr<CTXInfo>> A vector of CTXInfo pointers representing the requested transactions.
 *         May return fewer than 'count' transactions if the end of the list is reached.
 *         Returns an empty vector if startIndex is out of range.
 */
	std::vector<std::shared_ptr<CTXInfo>> getRecentTransactionsInfo(uint64_t startIndex, uint64_t count);


	/**
	 * @brief Sets the maximum number of recent transactions to keep.
	 *
	 * This method adjusts the maximum size of the recent transactions cache.
	 * If the new maximum size is smaller than the current cache size, it
	 * removes the oldest transactions to comply with the new limit.
	 *
	 * @param max The maximum number of recent transactions to keep.
	 */
	void setMaxRecentTransactions(uint64_t max);

	/**
	 * @brief Clears the recent transactions cache.
	 *
	 * This method clears all entries from the recent transactions cache and
	 * the associated hash maps used for quick look-ups.
	 */
	void clearRecentTXCache();

	
	// /END/ Recent Transactions Methods


/**
 * @brief Gets the transaction count for a specific date.
 *
 * This method counts the number of transactions that occurred on the specified date only.
 * It utilizes a cache to store transaction counts for specific dates to improve performance
 * on subsequent calls. The count includes all transactions that occurred from the start
 * (00:00:00) to the end (23:59:59) of the specified date.
 *
 * @param date The date in UNIX timestamp format (seconds since epoch).
 * @return The total number of transactions that occurred on the specified date.
 */

	uint64_t getTransactionCountOnDate(
		uint64_t date,
		size_t numThreads =1 ,
		std::shared_ptr<ThreadPool> threadPool = nullptr,
		uint64_t cacheDelayDays = 2,
		bool useCache = true);

	/**
	 * @brief Gets the transaction count since a given date.
	 *
	 * This method counts the number of transactions that occurred between the
	 * specified start date and the current time. It utilizes a cache to store
	 * transaction counts for date ranges to improve performance on subsequent calls.
	 *
	 * @param startDate The start date in UNIX timestamp format (seconds since epoch).
	 * @return The total number of transactions from the start date to the current time.
	 */
	uint64_t getTransactionCountSinceDate(uint64_t startDate);
	// Transactions Count Daily Stats - END


	/**
	 * @brief Determines the current liveness state of the blockchain system.
	 *
	 * This method assesses the liveness of the blockchain based on the timestamp
	 * of the last solved block (leader block). It returns an appropriate liveness state
	 * (noLiveness, lowLiveness, mediumLiveness, highLiveness) based on the time interval
	 * since the last leader block was solved.
	 *
	 * Liveness states:
	 * - noLiveness: No recent leader block has been found, or more than 3 hours have passed
	 *               since the last block was solved. This may indicate a serious network issue.
	 * - lowLiveness: The leader block was solved between 30 minutes and 1 hour ago. This indicates
	 *                the network is experiencing delays and may have potential issues.
	 * - mediumLiveness: The leader block was solved between 10 and 30 minutes ago. The network is still
	 *                   operating but with minor delays in block production.
	 * - highLiveness: The leader block was solved within the last 10 minutes. This indicates
	 *                 the network is functioning optimally.
	 *
	 * @return eLivenessState Enum representing the current liveness state of the blockchain system:
	 *         - eLivenessState::noLiveness: No blocks or long delay since last block.
	 *         - eLivenessState::lowLiveness: Leader block solved between 30 minutes and 1 hour ago.
	 *         - eLivenessState::mediumLiveness: Leader block solved between 10 and 30 minutes ago.
	 *         - eLivenessState::highLiveness: Leader block solved in the last 10 minutes.
	 *
	 * @note This method introduces basic time-based heuristics to monitor the blockchain's performance
	 *       and health, allowing detection of block production slowdowns or failures. Thresholds can
	 *       be adjusted based on the specific blockchain requirements.
	 */
	eLivenessState::eLivenessState CBlockchainManager::getLiveness(bool allowCached = true);

	double generateNewPrice(double floor, double ceiling, double previousPrice) const;

	void resetDailyPrice();

	BigInt generateNewAttoPerUSD(double floorUSDPrice=5, double ceilingUSDPrice=10) const;

	/**
	* @brief Get the current GNC/USDT price (stub implementation).
	*
	* This method provides a simulated GNC/USDT price for testing and development purposes.
	* It is a stub implementation that will be replaced with real-time data from live exchanges
	* in the future.
	*
	* Functionality:
	* 1. Caching: The price is cached and refreshed at most once per minute (configurable) to
	*    reduce computation overhead.
	* 2. Daily Reset: The daily open price is reset every 24 hours to simulate a new trading day.
	* 3. Price Fluctuation: The price fluctuates randomly but is constrained to a maximum of
	*    10% change from the daily open price, simulating realistic market behavior.
	* 4. Thread Safety: The method is thread-safe, allowing multiple readers with efficient
	*    locking mechanisms.
	*
	* Price Generation Rules:
	* - The price is always between the specified floor and ceiling values.
	* - Intraday price changes are limited to �10% from the daily open price.
	* - A new random price is generated only when the cache expires.
	*
	* @param floor The minimum possible price (default: 5.0 USDT).
	* @param ceiling The maximum possible price (default: 10.0 USDT).
	*
	* @return The current simulated GNC/USDT price as a double.
	*
	* @note This is a stub implementation. In the production version, this method will be
	*       replaced with one that fetches real-time price data from live cryptocurrency exchanges.
	*
	* @warning Do not use this method for actual trading or financial decisions. The prices
	*          generated are simulated and do not reflect real market conditions.
	*/
	
	BigInt getGNCUSDTprice(double floorUSDPrice = 5.0, double ceilingUSDPrice = 10.0);

	/**
	* @brief Get the current price of 1 GNC in USD.
	*
	* This method calculates the current price of 1 GNC in USD based on the
	* cached Atto per USD value. It does not require any new member fields
	* and uses the existing cached data.
	*
	* @return The current price of 1 GNC in USD as a double.
	*
	* @note This is based on the stub implementation and does not reflect
	*       real market conditions.
	*/
	double getCurrentGNCPriceInUSD() const;

	std::shared_ptr<CSearchResults> searchBlocks(const std::string& query, uint64_t size, uint64_t page, const CSearchFilter& filter);
/**
 * @brief Creates a detailed description of a blockchain block including optional transaction details.
 *
 * This method generates a comprehensive block description by extracting and processing information
 * from a given block. It handles both key blocks and data blocks, and can optionally include
 * detailed transaction information. The method attempts to reuse pre-computed transaction meta-data
 * when available, falling back to real-time computation when necessary.
 *
 * Processing Steps:
 * 1. Basic block information extraction (height, time, miner ID, etc.)
 * 2. Optional transaction processing (when includeTXdetails is true and not a key block):
 *    - Attempts to retrieve pre-computed transaction meta-data
 *    - Falls back to real-time transaction description creation if pre-computed data unavailable
 * 3. Block reward calculation and compilation of all block statistics
 *
 * @param block The blockchain block to process. Must be a valid shared pointer to a CBlock object.
 *              If null, the method returns nullptr.
 *
 * @param includeTXdetails Boolean flag controlling transaction detail inclusion:
 *                         - true: Include detailed transaction information (may increase processing time)
 *                         - false: Omit transaction details for faster processing
 *
 * @return std::shared_ptr<CBlockDesc> A shared pointer to a CBlockDesc object containing:
 *         - Block ID (base58 encoded)
 *         - Block type (key block or data block)
 *         - Key height and block height
 *         - Timestamp (solved at time)
 *         - Miner's ID (base58 encoded)
 *         - Difficulty level
 *         - Parent block ID (base58 encoded)
 *         - Energy usage statistics (used and limit)
 *         - Block nonce
 *         - Block rewards (current and total)
 *         - Transaction statistics (count of receipts, transactions, and verifiables)
 *         - Detailed transaction data (if requested and available)
 *         Returns nullptr if the input block is invalid.
 * 
 * 
 * [ When Security Analysis Enabled ]
 * Performs a security assessment to detect potential Proof-of-Work (PoW) wave attacks by analyzing timestamp discrepancies between block confirmation times and transaction issuance times.
 *
 * This assessment aims to identify if the block-confirming node or transaction originators are manipulating timestamps, which can be indicative of malicious behavior like PoW wave attacks.
 *
 * **Mechanics:**
 *
 * 1. **Data Collection:**
 *    - For each transaction in the block, collect the following:
 *      - **Confirmed Timestamp (`confirmedTimestamp`):** The time when the block containing the transaction was confirmed, as set by the node operator.
 *      - **Unconfirmed Timestamp (`unconfirmedTimestamp`):** The timestamp included within the transaction data structure by the transaction issuer.
 *      - **Sender Address (`sender`):** The originator of the transaction.
 *
 * 2. **Time Difference Calculation:**
 *    - Compute the time difference (`timeDiff`) for each transaction:
 *      ```
 *      timeDiff = confirmedTimestamp - unconfirmedTimestamp
 *      ```
 *    - A positive `timeDiff` indicates that the transaction was issued before the block was confirmed.
 *    - A negative `timeDiff` suggests the transaction claims to have been issued after the block confirmation time, which is suspicious.
 *
 * 3. **Suspicion Criteria:**
 *    - A transaction is considered **suspicious** if the absolute value of `timeDiff` exceeds **3 hours** (10,800 seconds):
 *      ```
 *      if (abs(timeDiff) > 10800) => Transaction is suspicious
 *      ```
 *
 * 4. **Per-Sender Aggregation:**
 *    - Group transactions by their `sender` to analyze on a per-originator basis.
 *    - For each sender:
 *      - Collect all `timeDiff` values.
 *      - Calculate the **average time difference** (`avgTimeDiff`):
 *        ```
 *        avgTimeDiff = sum(timeDiffs) / number of transactions
 *        ```
 *
 * 5. **Statistical Analysis:**
 *    - **Unique Senders Analysis:**
 *      - Identify the set of unique senders (`uniqueSenders`) and those with suspicious transactions (`suspiciousSenders`).
 *    - **Mean and Standard Deviation:**
 *      - Calculate the mean (`mean`) and standard deviation (`stdDev`) of `avgTimeDiff` among suspicious senders:
 *        ```
 *        mean = sum(avgTimeDiffs) / number of suspicious senders
 *        variance = sum((avgTimeDiff - mean)^2) / number of suspicious senders
 *        stdDev = sqrt(variance)
 *        ```
 *    - **Proportion of Suspicious Senders:**
 *      - Compute the proportion of suspicious senders (`proportionSuspiciousSenders`):
 *        ```
 *        proportionSuspiciousSenders = number of suspicious senders / total unique senders
 *        ```
 *
 * 6. **Assessment Decision:**
 *    - **Node Operator Suspicion:**
 *      - If `proportionSuspiciousSenders` is **greater than 50%** and `stdDev` is **less than 10% of the mean**, it suggests that the node operator may be manipulating block timestamps.
 *      - This is because a high proportion of senders have similar suspicious time differences, indicating a systemic issue likely caused by the node.
 *    - **Transaction Issuer Suspicion:**
 *      - If the above condition is not met, it is more likely that the suspicious activity originates from individual transaction issuers.
 *
 * 7. **Recording Results:**
 *    - **Block-Level Analysis Hints:**
 *      - Use `blockDesc->addAdditionalAnalysisHint()` to append messages to the block's extended metadata.
 *      - Include:
 *        - Summary of the number of suspicious senders.
 *        - Indication of whether the node operator or transaction issuers are likely at fault.
 *        - Statistical metrics (mean, standard deviation, proportion).
 *    - **Transaction-Level Analysis Hints:**
 *      - For each suspicious transaction, use `txDesc->addAdditionalAnalysisHint()` to note the specific time difference and sender involved.
 *
 * **Results Availability:**
 *
 * - The assessment results are made available as part of the block's additional metadata (`mAdditionalAnalysisHints` in `CBlockDesc`).
 * - Individual transactions also contain analysis hints in their metadata (`mAdditionalAnalysisHints` in `CTransactionDesc`).
 *
 * **Example of Analysis Hints:**
 *
 * - *"Suspicious time differences detected in 3 out of 5 unique senders."*
 * - *"High proportion of senders (60%) have similar average time differences (mean: 12,000s, std dev: 800s). Node operator may be manipulating the block timestamp."*
 * - *"Sender: Alice, Average Time Difference: 12,500 seconds."*
 * - *"Transaction from sender Bob has suspicious time difference of -13,000 seconds."*
 *
 * **Thread Safety and Performance:**
 *
 * - The assessment operates without shared mutable data, ensuring thread safety.
 * - The algorithm runs in linear time relative to the number of transactions and unique senders.
 *
 * **Error Handling:**
 *
 * - All exceptions are caught and logged, preventing the assessment from crashing the system.
 * - In case of errors, appropriate messages are added to the `errorMessage` parameter.
 *
 * **Usage:**
 *
 * - The security assessment is integrated into the `createBlockDescription` method.
 * - It is enabled by default but can be toggled via the `doSecAssessment` parameter.
 *
 * **Purpose:**
 *
 * - To enhance the integrity of the blockchain by detecting and flagging potential malicious activities.
 * - To aid in auditing and monitoring by providing detailed analysis within block and transaction metadata.
 *
 * @note Transaction meta-data processing includes error handling and logging for failed
 *       transaction description creation or null description cases.
 *
 * @warning Processing detailed transaction information can significantly increase the method's
 *          execution time, especially for blocks with many transactions.
 */
	std::shared_ptr<CBlockDesc> createBlockDescription(const std::shared_ptr<CBlock>& block, bool includeTXdetails=true, bool updateGlobalTXcache=true, const std::string& errorMessage="", bool doSecAssessment=true);
	bool matchesBlockFilter(const std::shared_ptr<CBlockDesc>& blockDesc, const std::string& query, const CSearchFilter& filter);
	std::shared_ptr<CSearchResults> searchTransactions(const std::string& query, uint64_t size, uint64_t page, const CSearchFilter& filter);

	/**
	 * @brief Determines if a transaction matches the given search criteria and filter.
	 *
	 * This method checks if a transaction description matches the provided search query
	 * and conforms to the specified search filter. It supports both standard and custom
	 * (arbitrary) search flags.
	 *
	 * @param txDesc A shared pointer to the CTransactionDesc object to be checked.
	 * @param query The search query string to match against transaction properties.
	 * @param filter The CSearchFilter object specifying which transaction properties to search.
	 *
	 * @return true if the transaction matches the search criteria and filter, false otherwise.
	 *
	 * @note The method first checks if the TRANSACTIONS standard flag is set in the filter.
	 *       If custom flags are set, it checks only the specified transaction properties.
	 *       If no custom flags are set, it performs a general search on key transaction properties.
	 *       All string comparisons are case-insensitive.
	 *
	 * @see CSearchFilter
	 * @see CTransactionDesc
	 * @see CTools::findStringIC
	 */

	bool matchesTransactionFilter(CSearchResults::ResultData& resultData, const std::string& query, const CSearchFilter& filter);
	CSearchResults::ResultData createTransactionDescription(std::shared_ptr<CTransaction>  tx, std::shared_ptr<CReceipt> receipt = nullptr, uint64_t confirmedTimetamp = 0, bool includeDetails = false, std::shared_ptr<CBlockHeader> blockHeader = nullptr, const std::string & errorMessage="");



		/**
		 * @brief Get transaction metadata from memory pool cache
		 * @param receiptID The 32-byte receipt identifier
		 * @return ResultData containing the transaction description or nullptr if not found
		 * @throws std::invalid_argument If receipt ID is invalid
		 */
		CSearchResults::ResultData getMemPoolTXMeta(const std::vector<uint8_t>& receiptID);

		/**
		 * @brief Add transaction metadata to memory pool cache
		 * @param receiptID The 32-byte receipt identifier
		 * @param txDesc Transaction description to cache
		 * @return bool True if added successfully, false if cache is full
		 * @throws std::invalid_argument If parameters are invalid
		 */
		bool addMemPoolTXMeta(const std::vector<uint8_t>& receiptID, std::shared_ptr<CTransactionDesc> txDesc);

		/**
		 * @brief Add transaction metadata to memory pool cache from ResultData variant
		 * @param receiptID The 32-byte receipt identifier
		 * @param resultData ResultData variant potentially containing CTransactionDesc
		 * @return bool True if added successfully, false if invalid data or cache is full
		 * @throws std::invalid_argument If receipt ID is invalid
		 */
		bool addMemPoolTXMeta(const std::vector<uint8_t>& receiptID, const CSearchResults::ResultData& resultData);

		/**
		 * @brief Remove transaction metadata from memory pool cache
		 * @param receiptID The 32-byte receipt identifier
		 * @return bool True if removed, false if not found
		 * @throws std::invalid_argument If receipt ID is invalid
		 */
		bool removeMemPoolTXMeta(const std::vector<uint8_t>& receiptID);

		/**
		 * @brief Set maximum size of memory pool cache
		 * @param size New maximum cache size
		 * @throws std::invalid_argument If size is 0
		 */
		void setMaxMemPoolCacheSize(uint64_t size);

		/**
		 * @brief Get current maximum size of memory pool cache
		 * @return uint64_t Current maximum cache size
		 */
		uint64_t getMaxMemPoolCacheSize() const;

		/**
		 * @brief Get current size of memory pool cache
		 * @return uint64_t Current number of entries in cache
		 */
		uint64_t getCurrentMemPoolCacheSize() const;

	/**
	 * @brief Retrieves or generates the address associated with a transaction.
	 *
	 * This method determines the address for a given transaction based on its public key
	 * and issuer information. It performs various validation checks and can optionally
	 * verify the existence of the issuer in the Merkle Patricia Trie (MPT).
	 *
	 * The address is determined through the following process:
	 * 1. If both public key and issuer are present in the transaction, it generates
	 *    an address from the public key and verifies it matches the issuer.
	 * 2. If an explicit 32-byte public key is present, it uses this to generate the address.
	 * 3. If only an issuer is present, it attempts to retrieve the public key from the
	 *    associated IDToken in the StateDomain.
	 * 4. If verification is required, it checks for the existence of the issuer in the MPT.
	 *
	 * @param trans A shared pointer to the CTransaction object to process.
	 * @param requireVerification A boolean flag indicating whether to verify the issuer's
	 *                            existence in the MPT. Defaults to false.
	 *
	 * @return std::vector<uint8_t> The generated or retrieved address as a byte vector.
	 *         Returns an empty vector if any validation checks fail or if a valid
	 *         address cannot be determined.
	 *
	 * @throws May throw exceptions from underlying calls to mCryptoFactory or
	 *         mStateDomainManager methods. Callers should handle these appropriately.
	 *
	 * @note This method performs several validation checks and will return an empty
	 *       vector in case of any validation failures, including:
	 *       - Mismatch between generated address and provided issuer
	 *       - Unknown issuer (when verification is required)
	 *       - Missing or invalid IDToken
	 *       - Absence of a valid 32-byte public key
	 *
	 * @warning This method assumes that mCryptoFactory and mStateDomainManager are
	 *          properly initialized and accessible. Ensure these dependencies are
	 *          set up before calling this method.
	 */
	std::vector<uint8_t> CBlockchainManager::getTransactionAddress(std::shared_ptr<CTransaction> trans, bool requireVerification = false);
	

	std::shared_ptr<CBlock> getDeepestBlockInCache() const;

	void setDeepestBlockInCache(std::shared_ptr<CBlock> block);

	bool addCheckpoint(std::shared_ptr<CBCheckpoint> checkpoint);
	std::vector<std::shared_ptr<CBCheckpoint>> getCheckpoints();
	std::shared_ptr<CBCheckpoint> getLatestObligatoryCheckpoint();
	bool getIsOperatorLeadingAHardFork();
	void setIsOperatorLeadingAHardFork(bool isHe = true);
	void setIsForkingAlternativeHistory(bool isIt = true);
	bool getIsForkingAlternativeHistory();
	std::vector<uint8_t> getBlockIDAtDepth(uint64_t depth);
	uint64_t getCacheBarID();

	uint64_t getChainProofCSSyncTime(eChainProof::eChainProof which);
	void pingChainProofCSSyncTime(eChainProof::eChainProof which);
	bool cutHeaviest(uint64_t height, bool fromConsole);

/**
 * @brief Resets the current state of the local instance of the GRIDNET decentralized state machine.
 *
 * This function performs a comprehensive clearing of the in-memory blockchain state of the local instance.
 *  [ IMPORTANT ]: the aim is thus not to make cut in Hot Storage data structures but to wipe away EVERYTHING and resume anew.
 *				   Even when a cut is made, all the caches are rebuild from Genesis Block.
 *
 *  [ Rationale ]: to avoid having to keep in mind of every miniscule caching detail.
 *
 * By wiping the current state, it readies the GRIDNET Core to start a fresh synchronization and revalidation
 * process, retracing the entire history of events from the Genesis Block onwards. This ensures that any
 * corrupted or out-of-sync state is discarded, allowing the machine to rebuild its state from trusted cold storage.
 *
 * Specifically, the function:
 *   - Clears the hot storage block cache.
 *   - Resets the current and key leader pointers.
 *   - Clears chain paths and chain proofs.
 *   - Sets the synchronization percentage back to 0.
 *
 * It's crucial to understand that while the in-memory state is wiped clean, this method does not remove or
 * alter any blocks stored in cold storage. As such, historical data remains intact and ready for resynchronization.
 *
 * @note This operation acquires multiple locks to ensure thread safety during the resync process. Be cautious
 *       of potential deadlock scenarios in multi-threaded environments.
 *
 * @warning Before initiating a resync, ensure that there are no ongoing operations or transactions that might
 *          be disrupted. Once the resync process begins, the state will be cleared, potentially impacting ongoing tasks.
 */
	bool resync(bool cutHeaviestCP = true, uint64_t height = 0, bool fromConsole = false);
	static uint64_t getForcedResyncSequenceNumber(eBlockchainMode::eBlockchainMode mode);
	uint64_t getVPInColdStorageBehindCount();
	void	incVPInColdStorageBehindCount();
	void	clearVPInColdStorageBehindCount();
/**
 * @brief Gets the total number of recent on-chain transactions.
 *
 * This method returns the total number of recent transactions stored in the cache.
 *
 * @return The total number of recent on-chain transactions.
 */
	uint64_t getTotalRecentTransactionsCount();
	bool setPaidToMinerEffective(const BigInt& paidToMiner, const std::vector<uint8_t>& blockID);
	std::vector<uint8_t> getEffectivePerspective(const std::vector<uint8_t>& blockID);
	bool truncateKeyBlockIDsFromKeyHeight(uint64_t height, eChainProof::eChainProof chainProof);
	std::vector<std::shared_ptr<CBCheckpoint>>  activateCheckpoints(eChainProof::eChainProof chainProofType, const std::vector < std::vector<uint8_t>>& chainproof = std::vector < std::vector<uint8_t>>());
	uint64_t getActiveCheckpointsCount();
	std::shared_ptr<CBCheckpoint> getCurrentCheckpoint(std::shared_ptr<CBlockHeader> blockHeader);
	bool isCheckpointed(std::shared_ptr<CBlockHeader> blockHeader, std::vector<std::shared_ptr<CBCheckpoint>> = std::vector<std::shared_ptr<CBCheckpoint>>());
	bool getIsProcessingLongChainProof();
	void setIsProcessingLongChainProof(bool isIt = true);
	bool markChainProofAsProcessed(const std::vector<uint8_t>& chainProofBytes);
	bool wasChainProofRecentlyProcessed(const std::vector<uint8_t>& chainProofBytes, bool marAsProcessed = false);
	bool cleanUpRecentlyProcessedChainProofs(uint64_t pruneEverySec = 1024);
	std::shared_ptr<CBlock> getKeyBlockAtKeyBlockHeight(uint64_t keyHeight, bool useColdStorage);
	void incCommitedHeaviestChainProofBlocksCount(uint64_t incBy = 1);
	uint64_t getCommitedHeaviestChainProofBlocksCount();
	void enterMaintenanceMode();
	void exitMaintenanceMode();
	void issueHeaviestChainProofReset();
	bool isHeaviestChainProofResetToBeMade();
	bool deRouteBlocksToHeaviestChain(uint64_t keyBlocksNr, uint64_t dataBlocksNr, bool resetHeaviestChainproof = true, bool AffectColdStorage = true);
	void setDefaultDeRouteDelay(uint64_t forHowLongSec);
	uint64_t getDefaultDeRouteDelay();
	bool delayProcessingOfBlock(std::shared_ptr<CBlock> block, uint64_t delayInSec = 0);
	bool delayProcessingOfBlock(std::vector<uint8_t> blockID, uint64_t delayInSec = 0);
	uint64_t getBlockProcessingDelay(std::vector<uint8_t> blockID, bool remove = false);
	uint64_t getTotalPoWAvailableAtAcivableIndex();
	void setTotalPoWAvailableAtAcivableIndex(uint64_t pow);
	uint64_t getAchievablePerspectiveAtIndex();
	void setAchievablePerspectiveAtIndex(uint64_t index);
	uint64_t getHeaviestChainProofTotalDifficulty();
	std::shared_ptr<CBlockHeader> getHeaviestChainProofKeyLeader();
	std::shared_ptr<CBlockHeader> getHeaviestChainProofLeader();
	void setHeaviestChainProofLeader(std::shared_ptr<CBlockHeader> header);
	void setHeaviestChainProofKeyLeader(std::shared_ptr<CBlockHeader> header);
	void setHeaviestChainProofLeadBlockID(const std::vector<uint8_t>& header);
	std::vector<uint8_t> getHeaviestChainProofLeadBlockID();
	uint64_t getVerifiedChainProofTotalDifficulty();
	void setHeaviestChainProofTotalDifficulty(uint64_t diff);
	void setVerifiedChainProofTotalDifficulty(uint64_t diff);
	void setKeyBlockFormationLimit(uint64_t limit = 0);
	uint64_t getKeyBlockFormationLimit();


	void setDataBlockFormationLimit(uint64_t limit);
	uint64_t getDataBlockFormationLimit();
	bool getEpochReward(std::shared_ptr<CBlock> fromPerspectiveOf, BigInt& reward, BigInt& rewardVerifiedUpTilNow, eEpochRewardCalculationMode::eEpochRewardCalculationMode mode);
	BigInt analyzeDomainBalances();
	bool verifyChainProofSegment(const std::vector<std::vector<uint8_t>>& chainProof, std::atomic<uint64_t>& highestChainHeightAtomic, std::atomic<uint64_t>& highestKeyBlockHeightAtomic, std::atomic<uint64_t>& cumulativePoWAtomic, std::atomic<bool>& verificationFailed, size_t startIndex, size_t endIndex);
	bool verifyBlockHeader(const std::shared_ptr<CBlockHeader> currentHeader, std::atomic<uint64_t>& highestChainHeightAtomic, std::atomic<uint64_t>& highestKeyBlockHeightAtomic, std::atomic<uint64_t>& cumulativePoWAtomic, std::atomic<bool>& verificationFailed, size_t startIndex, size_t i);
	bool verifyChainProofMultithreaded(const std::vector<std::vector<uint8_t>>& chainProof, uint64_t& cumulativeDiff, bool validateAgainsLocalData, bool flashCurrentCount, const std::shared_ptr<CBlockHeader>& keyLeader, const bool& containsGenesisBlock, const std::vector<uint64_t>& PoWs, const std::shared_ptr<CBlockHeader>& blockLeader, uint64_t startFromPosition, bool optimize);
	/**
	 * @brief Retrieves the previous key block in the blockchain for a given block.
	 *
	 * This method traverses the blockchain backward from the given block to find the previous key block.
	 * It can use both hot storage (cache) and cold storage to retrieve the block, depending on the
	 * parameters and availability.
	 *
	 * @param block The starting block from which to search for the previous key block.
	 * @param omitHardForkBlocks If true, hard fork blocks (blocks with nonce == 0) are skipped.
	 * @param chain The chain proof type to use for block retrieval. Defaults to verified chain.
	 * @param allowHotStorageCache If true, allows using hot storage cache for block retrieval.
	 *
	 * @return A shared pointer to the previous key block if found, nullptr otherwise.
	 *
	 * @note If allowHotStorageCache is true and the chain is verified or verifiedCached,
	 *       the method first attempts to find the block in hot storage. If unsuccessful or not allowed,
	 *       it falls back to cold storage using the specified chain.
	 *
	 * @warning This method assumes thread-safety is handled by the caller if used in a multi-threaded context.
	 * @warning If the input block is the genesis block (key height 0), the method will return nullptr
	 *          as there are no previous blocks.
	 */
	std::shared_ptr<CBlock> CBlockchainManager::getKeyBlockForBlock(
		std::shared_ptr<CBlock> block,
		bool omitHardForkBlocks = false,
		eChainProof::eChainProof chain = eChainProof::verified,
		bool allowHotStorageCache = true);
	std::shared_ptr<CBlock> getLatestKeyBlock(std::vector<uint8_t> fromPerspectiveoOfBlockID = std::vector<uint8_t>());
	std::shared_ptr<CBlock> getLatestKeyBlock(std::shared_ptr<CBlock> fromPerspectiveOf = nullptr);
	std::vector<std::shared_ptr<CBlock>> getRecentKeyBlocks(uint64_t howMany = 1, std::vector<uint8_t> fromPerspectiveoOfBlockID = std::vector<uint8_t>());
	std::vector<std::shared_ptr<CBlock>> getRecentKeyBlocks(uint64_t howMany = 1, std::shared_ptr<CBlock> fromPerspectiveoOf = nullptr);
	std::vector<uint8_t> getPubKeyOfRecentKeyBlockOwner();
	std::vector<uint8_t> getCurrentID(bool getCached = true);
	bool amITheLeader(CKeyChain chain, CKeyChain& leaderAtChain);

	bool isStateDomainOwned(std::vector<uint8_t> id, const CKeyChain& chain = CKeyChain(false));

	std::shared_ptr<CNetworkManager> getNetworkManager();
	void updateLiveTransactionManagerStatistics(bool keyBlock);
	bool addBlockToHuntedList(std::vector<uint8_t>& blockID, std::shared_ptr<CBlockHeader> header);
	uint64_t getNumberOfHuntedBlocks(bool doAutoRemoval = true);
	bool isBlockHunted(std::vector<uint8_t>& blockID);
	bool removeBlockFromHuntedList(std::vector<uint8_t>& blockID);
	//Blockchain internal-pointer queries
	CTests* getTestsEngine();
	std::shared_ptr<CWorkManager> getWorkManager();
	bool isNrOfStateDomainsFresh();
	bool isTestNet();
	bool isBlockInChainProof(std::shared_ptr<CBlockHeader> header, eChainProof::eChainProof chain = eChainProof::heaviestCached);
	bool isBlockInChainProof(const std::vector<uint8_t>& blockIdentifier, uint64_t height, eChainProof::eChainProof chain = eChainProof::heaviestCached);
	std::shared_ptr<CReceipt> findReceipt(std::vector<uint8_t> GUID, bool checkIfOnChain = true, bool blockNeeded = true, std::shared_ptr<CBlock>& block = std::make_shared<CBlock>(), std::vector<uint8_t> blockID = std::vector<uint8_t>());
	std::shared_ptr<CTransaction> findTransaction(std::vector<uint8_t> receiptID, CReceipt& receipt, const uint64_t& confirmedtTimestamp = 0, const uint64_t& timestamp = 0, bool checkIfOnChain = true, const std::vector<uint8_t>& blockID = std::vector<uint8_t>());
	bool reportCryticalError(std::string description = "");
	uint64_t getNrOfErrors();

	void pingLastLivenessCheck();

	uint64_t getLastLivenessCheck() const;

	

	std::shared_ptr<CBlock> findBlockInCache(std::vector<uint8_t> id);

	size_t getMaxNrOfPrunedBlocksToKeepInMemory();
	size_t getMaxNrOfFullBlocksToKeepInMemory();
	size_t getMaxOverallBlocksToKeepInMemory();
	void setMaxNrOfPrunedBlocksToKeepInMemory(size_t count);

	//Blockchain information utilities
	BigInt getCoinbaseRewardForKeyHeight(uint64_t height);
	std::vector<uint8_t> getPerspective();
	uint64_t getBlockCacheLength(bool optimize=true);
	std::vector<std::shared_ptr<CBlock>> getRecentUncles();
	uint64_t calculateRewardForUncleBlock(std::shared_ptr<CBlock>block);

	static std::shared_ptr<CBlockchainManager> getInstance(eBlockchainMode::eBlockchainMode whichOne, bool initIfNull = true, bool waitTillReady = true);

	bool wasBlockRetrievalSuccessful(eBlockInstantiationResult::eBlockInstantiationResult res);

	//Blockchain statistical-augumentation utilities
	void updateMinDifficultyCoefficients(bool lockIt = true);
	void updateMinERGPrice();


	//Blockchain statistical-infromation query utilities
	double getMinDIfficultyForBlock(std::vector<uint8_t> blockID, uint64_t now, bool& ok);
	void updateDataBlockInterval();
	double getMinDIfficultyForBlockSMA(std::shared_ptr<CBlock> current, uint64_t now, bool useOptimization, bool& ok, std::shared_ptr<CBlockchainManager> bm = nullptr);
	double getMinDIfficultyForBlock(std::shared_ptr<CBlock> current, uint64_t now, bool useOptimization, bool updateLIVECoefficients, bool& ok, std::shared_ptr<CBlockchainManager> bm = nullptr, const uint64_t& EMA = 0, const uint64_t& SMA = 0, double forcedAlpha = 0, double forcedStepFactor = 0, const uint64_t& alphaZeroedAtStep = 0);
	std::vector<std::tuple< std::vector<uint8_t>, std::shared_ptr<CBlockHeader>, uint64_t >> mHuntedBlocks;

	bool blockFromHeader(std::vector<uint8_t> headerBytes, std::shared_ptr<CBlock>& block, bool instantiateTries = true);
	bool blockFromHeader(std::shared_ptr<CBlockHeader>& header, std::shared_ptr<CBlock>& block, bool instantiateTries = true);

	bool prepareCache(bool cacheReceipts = true, const std::vector<uint64_t> PoWs = std::vector<uint64_t>());

/**
 * @brief Prepares the Block Transaction Meta Data Cache using a multi-threaded approach.
 *
 * This method initializes and populates the block transaction meta-data cache by iterating
 * through each block in the blockchain. It employs a multi-threaded, batched processing approach
 * to efficiently handle large blockchains.
 *
 * The sole purpose of this method is to generate and set block descriptions for each block,
 * thereby updating the global TX cache.
 *
 * @return bool Returns true if meta-data cache preparation was successful, false otherwise.
 *
 * @note This method is thread-safe, using mutex locks to protect shared resources.
 *       It's designed to handle large blockchains efficiently, with built-in limits to prevent excessive memory usage.
 *
 * @warning This method can be resource-intensive and may take a considerable amount of time for large blockchains.
 *          Ensure adequate system resources are available before calling this method.
 */
	bool prepareBlockTXMetaDataCache(uint32_t numThreads=8);
	/**
	 * @brief Prepares the blockchain cache using a multi-threaded approach.
	 *
	 * This method initializes and populates the blockchain cache, optionally including transaction receipts.
	 * It employs a multi-threaded, batched processing approach to efficiently handle large blockchains.
	 *
	 * [ IMPORTANT ]: This method is NOT expected to perform validation of the existing cache members.
	 *				Example: the setLeader() method is responsible for making a cut in cache, at an appropriate spot, when appending a new Leading Block.
	 *				Thus, from a strucural perspective the solely aim of this method is ensuring that:
	 *				 - blockchain cache is of an appropriate depth.
	 *				 - it contains a sufficient amount of unfolded and pruned Blocks (in regards to corresponding Merkle Patricia Tries).
	 *				 - the receipts cache is filled up.
	 *				 - hardware capabilities of the underlying platform are fully utilised (CPU core)
	 *			      - time-left is properly estimated and reported.
	 *				 - limits steming from hardware limitations (number of CPU cores) and Operator set limits (RAM storage, cache length) are respected.
	 *				 - the cache is produced starting with the currently proclaimed Leading Block.
	 *
	 *
	 *
	 * @details The caching process is divided into two main phases:
	 *
	 * Phase 1: Block and Receipt Caching
	 * - 1A: Parallel Block Retrieval and Initial Processing
	 *   - Retrieves blocks in batches, with each batch processed by multiple threads.
	 *   - Caches full blocks up to 'maxFullBlocksToKeep', then switches to pruned blocks.
	 *   - Processes and caches transaction receipts if 'cacheReceipts' is true.
	 *   - Continues until 'maxPrunedBlocksToKeep' or 'maxBlockchainCacheSize' is reached.
	 *
	 * - 1B: Block Connection
	 *   - Connects blocks within each batch to form a doubly-linked list structure.
	 *   - Links the current batch with the previous batch using 'lastBatchFinalBlock'.
	 *
	 * Phase 2: Extended Receipt Caching (if necessary)
	 * - Continues processing blocks solely for receipt caching if the receipt cache isn't full after Phase 1.
	 * - Uses a similar batched, multi-threaded approach as Phase 1A, but doesn't cache the blocks themselves.
	 *
	 * Throughout both phases, the method employs several key mechanisms:
	 * 1. Batch Processing: Divides work into manageable batches for efficient multi-threading.
	 * 2. Dynamic Thread Management: Adjusts active threads based on workload.
	 * 3. Adaptive Caching: Switches between full and pruned block caching based on configured limits.
	 * 4. Progress Tracking: Uses Exponential Moving Average (EMA) for time estimation and progress reporting.
	 * 5. Memory Management: Monitors and limits cache size for both blocks and receipts.
	 * 6. Error Handling: Uses atomic flags for thread-safe error signaling.
	 *
	 * @param cacheReceipts Boolean flag to indicate whether to cache transaction receipts.
	 * @param PoWs Vector of Proof of Work difficulties for each block (used for total difficulty calculation).
	 * @param flashCurrentCount Boolean flag to control frequent progress updates.
	 * @param numThreads Number of threads to use for parallel processing.
	 *
	 * @return bool Returns true if cache preparation was successful, false otherwise.
	 *
	 * @note This method is thread-safe, using mutex locks to protect shared resources.
	 *       It's designed to handle large blockchains efficiently, with built-in limits to prevent excessive memory usage.
	 *       The method includes optional verification steps (controlled by preprocessor flags) to ensure cache integrity.
	 *
	 * @warning This method can be resource-intensive and may take a considerable amount of time for large blockchains.
	 *          Ensure adequate system resources are available before calling this method.
	 */
	bool CBlockchainManager::prepareCacheMT(bool cacheReceipts, const std::vector<uint64_t>& PoWs = std::vector<uint64_t>(), bool flashCurrentCount = true, uint32_t numThreads = 8, std::shared_ptr<ThreadPool> threadPool = nullptr);


// Stats - BEGIN

/**
 * @brief Gets the total block reward over a specified time period.
 *
 * Calculates the sum of all block rewards for blocks created within
 * the specified number of seconds from the current time.
 *
 * @param seconds The time period in seconds to consider (default: 24 hours).
 * @param allowCache Whether to use cached values if available (default: true).
 * @return BigInt The total block reward in the specified time period.
 */
	BigInt getRecentTotalReward(uint64_t seconds = 86400, bool allowCache = true);

/**
 * @brief Gets the current network utilization.
 *
 * This is a placeholder implementation that returns a random value
 * between 0.01 and 0.05 to simulate network utilization.
 *
 * @param allowCache Whether to use cached values if available (default: true).
 * @return uint64_t A value between 0.01 and 0.05 representing network utilization.
 */
	double getNetworkUtilization(bool allowCache = true);

/**
 * @brief Gets the average block size in bytes over a specified time period.
 *
 * Calculates the average size of blocks created within the specified
 * number of seconds from the current time. Uses cached block data when available.
 *
 * @param seconds The time period in seconds to consider (default: 24 hours).
 * @param allowCache Whether to use cached values if available (default: true).
 * @return uint64_t The average block size in bytes.
 */
	uint64_t getBlockSize(uint64_t seconds = 86400, bool allowCache = true);

/**
 * @brief Gets the average time between blocks over a specified time period.
 *
 * Calculates the average time between consecutive blocks within the specified
 * number of seconds from the current time. Can be restricted to only key blocks.
 *
 * @param seconds The time period in seconds to consider (default: 24 hours).
 * @param onlyKey Whether to consider only key blocks (default: false).
 * @param allowCache Whether to use cached values if available (default: true).
 * @return uint64_t The average time between blocks in seconds.
 */
	uint64_t getAverageBlockTime(uint64_t seconds = 86400, bool onlyKey = false, bool allowCache = true);

/**
 * @brief Performs security analysis on all blocks in the blockchain.
 *
 * This method iteratively processes each block from height 0 to the current height,
 * performing security analysis using updateSecAnalysis. Due to the dependent nature
 * of block security analysis, this process is for not not attempted to be parallelized.
 *
 * @return bool Returns true if security analysis completed successfully, false otherwise.
 */
	bool prepareInitialBlockchainSecurityAnalysis();

// Stats - END

/**
 * @brief Clears the in-memory block cache starting from the leader block.
 *
 * This function navigates the main chain in reverse, starting from the `mLeader`
 * block and using the `getParent()` method. It clears the cache references by
 * invoking `setParent(nullptr)` and `setNext(nullptr)` on each block in the chain.
 * This ensures that the entire cached chain is decoupled and the memory references
 * are broken, facilitating garbage collection.
 *
 * The function acquires necessary mutex locks at the beginning to ensure
 * thread safety during the clearing process.
 *
 * @warning Ensure that there are no ongoing operations relying on the cached
 * blocks before calling this function, as it will break the in-memory chain
 * linkage.
 */
	void clearBlockCache();
	// PoWs are optional (good for initial bootstrap performance).
	void getHuntedBlockIDs(std::vector < std::vector<uint8_t>>& data);
	void getHuntedBlocks(std::vector<std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>>>& blockData);
	bool setPerspective(std::vector<uint8_t> perspectiveID);

	bool getLocalNodeIsLeaderCached();

	void setLocalNodeIsLeaderCached(bool isIt = true);

	// ============================================================================
	// Phantom Leader Mode Public Interface - BEGIN
	// ============================================================================
	/// @brief Checks if Phantom Leader Mode is currently enabled.
	/// @return true if Phantom Leader Mode is active, false otherwise.
	/// @note Phantom Leader Mode allows transaction processing simulation without
	///       broadcasting blocks to the network. Useful for debugging tx withholding.
	bool getIsPhantomLeaderModeEnabled();

	/// @brief Enables or disables Phantom Leader Mode.
	/// @param enable Set to true to enable, false to disable.
	/// @note This mode should only be enabled from local Terminal with proper credentials.
	///       When enabled, node acts as if it were the Leader but blocks are NOT broadcasted.
	void setPhantomLeaderModeEnabled(bool enable);

	/// @brief Returns the count of phantom blocks formed during this session.
	/// @return Number of phantom blocks formed since mode was enabled.
	uint64_t getPhantomBlocksFormedCount();

	/// @brief Increments the phantom blocks formed counter.
	void incPhantomBlocksFormedCount();

	/// @brief Returns the count of transactions processed in phantom mode.
	/// @return Number of transactions processed in phantom blocks.
	uint64_t getPhantomTransactionsProcessedCount();

	/// @brief Adds to the phantom transactions processed counter.
	/// @param count Number of transactions to add.
	void addPhantomTransactionsProcessedCount(uint64_t count);

	/// @brief Resets all phantom mode statistics.
	void resetPhantomModeStats();

	/// @brief Gets the timestamp of the last phantom block formation.
	/// @return Unix timestamp of last phantom block, 0 if none formed.
	uint64_t getLastPhantomBlockFormationTime();

	/// @brief Updates the last phantom block formation timestamp.
	void pingLastPhantomBlockFormationTime();

	/// @brief Generates a detailed report of the last phantom block formation.
	/// @param block The phantom block that was formed.
	/// @param receipts Vector of receipts from processing.
	/// @param processingTimeMs Time taken to process in milliseconds.
	/// @return Formatted string report suitable for terminal display.
	std::string generatePhantomBlockReport(
		std::shared_ptr<CBlock> block,
		const std::vector<CReceipt>& receipts,
		uint64_t processingTimeMs);
	// Phantom Leader Mode Public Interface - END
	// ============================================================================

	// Unordered  / Detached Block Cache - BEGIN
	// 
	
	/**
	 * @brief Removes a block from the unordered cache by its ID.
	 * @param blockID The 32-byte vector representing the block's ID.
	 *
	 * This method removes a block from the unordered cache, if it exists.
	 * It ensures that both the map and the order list remain consistent.
	 */
	void removeBlockFromUnorderedCache(const std::vector<uint8_t>& blockID);
	// Rationale: to serve an extremely fast cache between the Network and Block Processing layer.
	 /**
	 * @brief Adds a block to the unordered cache.
	 * @param block The shared pointer to the block to be added.
	 */
	void addBlockToUnorderedCache(const std::shared_ptr<CBlock>& block);

	/**
	 * @brief Retrieves a block from the unordered cache by its ID.
	 * @param blockID The 32-byte vector representing the block's ID.
	 * @return A shared pointer to the block if found; nullptr otherwise.
	 */
	std::shared_ptr<CBlock> getBlockFromUnorderedCache(const std::vector<uint8_t>& blockID);

	/**
	 * @brief Sets the maximum number of blocks to keep in the unordered cache.
	 * @param maxSize The maximum cache size.
	 */
	void setBlockUnorderedCacheMaxSize(size_t maxSize);

	/**
	 * @brief Gets the current number of blocks in the unordered cache.
	 * @return The current cache size.
	 */
	size_t getBlockUnorderedCacheSize() const;

	void setDoNotProcessExtrernalChainProofs(bool doNotProcessExtChainProofs);

	bool getDoNotProcessExtrernalChainProofs();

	bool getIsSynchronizationPaused();

	void setIsSynchronizationPaused(bool isIt);

	// Unordered  / Detached Block Cache - END
	void mVitalsMonitoringThreadF();

	// ============================================================================
	// Transaction Withholding Detection Public Interface - BEGIN
	// ============================================================================
	/// @brief Performs transaction withholding detection check.
	/// @details Analyzes mem-pool transactions against key block progression.
	///          Issues warning if transactions are stuck for >= 3 key blocks.
	/// @param currentKeyHeight The current key block height.
	/// @param memPoolTxCount Number of transactions currently in mem-pool.
	/// @param oldestTxKeyHeight Key height when oldest mem-pool tx was added.
	/// @return true if potential withholding detected, false otherwise.
	bool checkForTransactionWithholding(
		uint64_t currentKeyHeight,
		uint64_t memPoolTxCount,
		uint64_t oldestTxKeyHeight);

	/// @brief Gets the count of transaction withholding warnings issued.
	/// @return Number of warnings issued during this session.
	uint64_t getTxWithholdingWarningCount();

	/// @brief Resets the transaction withholding detection state.
	void resetTxWithholdingDetection();
	// Transaction Withholding Detection Public Interface - END
	// ============================================================================

	bool processBreakpoints(std::shared_ptr< CBlock> block, eBreakpointState::eBreakpointState state);

	bool syncChainProofsWithColdStorage(bool forceIt = false);

	size_t getLastTimeValidBlockProcessed();

	void pingLastTimeValidBlockProcessed();

	uint64_t getAverageSMAKeyBlockInterval();

	uint64_t getJustSavedKeyBlocksCount();
	uint64_t getJustSavedDataBlocksCount();

	void incJustSavedKeyBlocksCount();
	void incJustSavedDataBlocksCount();

	uint64_t getDiscardedKeyBlocksCount();
	uint64_t getDiscardedDataBlocksCount();

	void incDiscardedKeyBlocksCount();
	void incDiscardedDataBlocksCount();

	uint64_t getAssumedAsLeaderKeyBlocksCount();
	uint64_t getAssumedAsLeaderDataBlocksCount();

	void incAssumedAsLeaderKeyBlocksCount();
	void incAssumedAsLeaderDataBlocksCount();

	void setAverageSMAKeyBlockInterval(uint64_t interval);
	uint64_t getAverageSMADataBlockInterval();
	void setAverageDataBlockInterval(uint64_t interval);

	void setVitalsGood(bool areThey);
	bool getAreVitalsGood();
	std::string getVitalsDescription(bool colorize = true);
	bool getDisableDataAnalysis();
	void setEnableDataAnalysis(bool doIt);
	bool getInNetworkTestingMode();
	void setInNetworkTestingMode(bool doIt);
	uint64_t getFarAwayCPRequestedTimestamp();
	void clearFarAwayCPRequestedTimestamp();
	void pingFarAwayCPRequestedTimestamp();
	bool getIsVitalsMonitorRunning();
	void setIsVitalsMonitorRunning(bool isIt);
	void setIsVitalsMonitorToBeRunning(bool isIt);
	bool getIsVitalsMonitorToBeRunning();
	uint64_t getLastControllerLoopRun();
	void pingtLastControllerLoopRun();
	uint64_t getVerifiedSyncedWithCSAtHeight();
	void setVerfiedSyncedWithCSAtHeight(uint64_t height);
	bool getIsRestartNeeded();
	CBlockchainManager(eBlockchainMode::eBlockchainMode mode, std::shared_ptr<CWorkManager> workManager);
	bool postInstantiationPreparations();
	void setTestTrieAfterEachRound(bool doIt = true);
	void lockChainGuardian();
	void unlockChainGuardian();
	void waitTillBlockProcessingDone();
	/**
	 * @brief Verifies the integrity of the BlockID to height map mechanism.
	 *
	 * This method performs a comprehensive check to ensure consistency between:
	 * 1. The BlockID to height map (mBlockID_BlockHeightMap)
	 * 2. The flat cache for normal blocks (mBlockCacheFlat)
	 *
	 * The integrity check includes:
	 * - Comparing the size of mBlockID_BlockHeightMap with mBlockCacheFlat
	 * - Verifying that each entry in mBlockID_BlockHeightMap corresponds to a valid block in mBlockCacheFlat
	 * - Ensuring that the block IDs match between the map and the actual blocks in the cache
	 *
	 * @return bool True if the integrity check passes (all checks succeed), false otherwise.
	 *
	 * @note This method provides progress updates through log events and updates a status bar.
	 *       Progress is reported every 5% of completion.
	 *
	 * @note This method is thread-safe. It uses appropriate locks to ensure exclusive access
	 *       to mBlockID_BlockHeightMap and mBlockCacheFlat during the verification process.
	 *
	 * @note In case of integrity violations, detailed error messages are logged using the
	 *       CTools logging mechanism. These logs can be used for diagnostics and debugging.
	 *
	 * @warning This method may take a considerable amount of time for large blockchain datasets.
	 *          It should be called during maintenance periods or when consistency checks are necessary.
	 *
	 * @see mBlockID_BlockHeightMap, mBlockCacheFlat
	 */
	bool verifyBlockIDMapIntegrity();
	bool verifyLocalDataAvailability(eChainProof::eChainProof chainproof = eChainProof::heaviest);

	bool getSyncMachine();

	void pingCustomStatusBarShown();


	void incAPM();
	void incBPM();
	uint64_t getAPM();
	uint64_t getlastBlockStatsCleaned();
	uint64_t getBPM();
	void cleanBlockStats();
	void setDoNotProcessExtrernalBlocks(bool doNotProcessExtBlocks=true);
	bool getDoNotProcessExtrernalBlocks();
	CBlockchainManager(const CBlockchainManager& sibling);

	//There's a Blockchain Manager for each network.("Live/Test-Net/Local")
	//Further, each Blockchain Manager has 3 Transaction Managers.
	std::shared_ptr<CTransactionManager> getLiveTransactionsManager();//used to represent the current 'Live' state of the given network ("Live/Test-Net/Local") i.e. there's a 'Live; TM for each network.
	std::shared_ptr<CTransactionManager> getVerificationFlowManager();//for verifying incoming blocks
	std::shared_ptr<CTransactionManager> getFormationFlowManager();//for block formation
	//used for Flow processing. The purpose is to separate local processing from network processing.
	std::shared_ptr<CTransactionManager> getTerminalTransactionsManager();// this one is used by the Local Terminal only. yet anather separation of processing Logic.
	std::shared_ptr<CSettings>  getSettings();
	std::shared_ptr<CCryptoFactory> getCryptoFactory();
	void exit(bool waitForConfirmation = false);
	~CBlockchainManager();

	CSolidStorage* getSolidStorage();
	CTrieDB* getStateTrieP();
	CTrieDB  getStateTrie();

	uint64_t getChainProofSizeInBytes(eChainProof::eChainProof whichOne);
	uint64_t getPathCacheSizeInBytes(eChainProof::eChainProof whichOne);
	std::shared_ptr<CStateDomainManager>  getStateDomainManager(bool persistentSDM=false);
	eBlockProcessingResult processBlock(std::vector<uint8_t> BERBlock);
	uint64_t getMinersRewardLockPeriod(std::shared_ptr<CBlock> block);
	eBlockProcessingResult processBlock(std::shared_ptr<CBlock> block, std::vector<std::string>& log);

	void blockProcessingCleanUp(std::shared_ptr<CBlock>& block, bool freeTries = true);

	std::shared_ptr<CBlock> getCachedLeader(bool keyBlockOnly = false);

	/**
 * @brief Retrieves the next key block in the blockchain for a given block.
 *
 * This method traverses the blockchain forward from the given block to find the next key block.
 * It can use both hot storage (cache) and cold storage to retrieve the block, depending on the
 * parameters and availability.
 *
 * @param block The starting block from which to search for the next key block.
 * @param omitHardForkBlocks If true, hard fork blocks (blocks with nonce == 0) are skipped.
 * @param chain The chain proof type to use for block retrieval. Defaults to verified chain.
 * @param allowHotStorageCache If true, allows using hot storage cache for block retrieval.
 *
 * @return A shared pointer to the next key block if found, nullptr otherwise.
 *
 * @note If allowHotStorageCache is true and the chain is verified or verifiedCached,
 *       the method first attempts to find the block in hot storage. If unsuccessful or not allowed,
 *       it falls back to cold storage using the specified chain.
 *
 * @warning This method assumes thread-safety is handled by the caller if used in a multi-threaded context.
 */
	std::shared_ptr<CBlock> getNextKeyBlockForBlock(
		std::shared_ptr<CBlock> block,
		bool omitHardForkBlocks = false,
		eChainProof::eChainProof chain = eChainProof::verified,
		bool allowHotStorageCache = true);

	void  setCachedLeader(std::shared_ptr<CBlock> leader);
	uint64_t getHeight();
	uint64_t getKeyHeight();
	//recent block
	std::vector<uint8_t> getLeaderID(bool keyBlockOnly = false);
	std::shared_ptr<CBlock> getLeader(bool keyBlockOnly = false);

	bool syncChainProofToColdStorage(eChainProof::eChainProof which);
	bool setLeader(std::shared_ptr<CBlock> block, size_t heaviestChainProofSize = 0, const std::vector<std::string> &log = std::vector<std::string>());
	void setJustAppendedLeader(bool doIt);
	uint64_t getDepth();
	uint64_t getChainProofLength(eChainProof::eChainProof chain);
	// Indexes - BEGIN
	void setBlockIDAtKeyHeight(uint64_t height, std::vector<uint8_t> blockID, eChainProof::eChainProof chainProof = eChainProof::verified);
	std::vector<uint8_t> getBlockIDAtHeight(uint64_t height);
	std::vector<uint8_t> getBlockIDAtKeyHeight(uint64_t height, eChainProof::eChainProof chainProof = eChainProof::verified);
	std::vector<uint8_t> getBlockIDAtKeyDepth(uint64_t keyDepth, eChainProof::eChainProof chainProof = eChainProof::verified);
	bool getTotalBlockRewardEffective(const std::vector<uint8_t>& blockID, BigInt& totalReward);

	bool setTotalBlockRewardEffective(const BigInt& totalReward, const std::vector<uint8_t>& blockID);
	bool getPaidToMinerEffective(const std::vector<uint8_t>& blockID, BigInt& paidToMiner);
	// Indexes - END

	std::shared_ptr<CBlock> getBlockAtHeight(uint64_t height, bool isKeyHeight = false, eChainProof::eChainProof chain = eChainProof::verified);

	// Current and Solid-Storage indirect - BEGIN
	std::shared_ptr<CBlock> getBlockAtDepth(uint64_t depth, bool isKeyDepth = false, eChainProof::eChainProof chain = eChainProof::verified);
	// Current and Solid-Storage indirect - END

	// Direct Solid Storage - BEGIN
	std::shared_ptr<CBlock> getBlockAtDepthSSDirect(size_t depth, eChainProof::eChainProof chain = eChainProof::verified, const bool& instantiateTrees = true, const uint64_t& bytesCount = 0);
	std::shared_ptr<CBlock> getBlockAtHeightSSDirect(size_t height, eChainProof::eChainProof chain = eChainProof::verified, const bool& instantiateTrees = true, const uint64_t& bytesCount = 0);
	// Direct Solid Storage - END

	std::shared_ptr<CBlockHeader> getHeaderAtHeight(size_t height, eChainProof::eChainProof chain);
	std::shared_ptr<CBlock> getBlockByHash(std::vector<uint8_t> hash, eBlockInstantiationResult::eBlockInstantiationResult& result, bool instantiateTries = true, const uint64_t& bytesCount = 0, bool enableFastCash = true);

	//settings
	bool isBlockAvailableLocally(std::vector<uint8_t>& blockID, bool useCacheOnly = false, const bool& basedOnCache = false);
	void markBlockLocalAvailability(std::vector<uint8_t> blockID, bool available = true, bool report = false);
	//Block synchronization utilites
	std::vector <std::vector<uint8_t>> getPath(eChainProof::eChainProof whichOne = eChainProof::eChainProof::verified, bool recalculate = false, bool muteConsole = false, bool forceReload = false);
	std::vector <std::vector<uint8_t>> getChainProof(eChainProof::eChainProof whichOne = eChainProof::eChainProof::verified, bool recalculate = false, bool forceReload = false);
	std::vector<uint8_t> getChainProofPacked(eChainProof::eChainProof whichOne);
	bool isBlockInChainProof(const std::vector<uint8_t>& blockID, eChainProof::eChainProof chain = eChainProof::heaviest, const std::vector<uint8_t>& proceededByBlock = std::vector<uint8_t>(), bool traverseFromEnd = true, bool allowForCache = false, const std::vector<std::vector<uint8_t>>& externalChainProof = std::vector<std::vector<uint8_t>>());
	bool isBlockInChainProof(const std::shared_ptr<CBlockHeader> block, eChainProof::eChainProof chain, const std::shared_ptr<CBlockHeader> proceededByBlock = nullptr, bool allowForCache = true);
	bool shouldIDig();
	//Blockchain crypto information-query utilities
	//bool getChainProofCumulativePoW(const std::vector<std::vector<uint8_t>>& chainProof, uint64_t& cumulativeDiff, const std::vector<uint64_t>& PoWs = std::vector<uint64_t>());
	bool getChainProofForBlock(eChainProof::eChainProof whichChainProof, std::vector <std::vector<uint8_t>>& proof, std::vector<uint8_t> blockIDA = std::vector<uint8_t>(), std::vector<uint8_t> blockIDB = std::vector<uint8_t>());
	eChainProofValidationResult::eChainProofValidationResult verifyChainProof(const std::vector <std::vector<uint8_t>>& chainProof, uint64_t& cumulativeDiff, bool validateAgainsLocalData = true, bool flashCurrentCount = false, const  std::shared_ptr<CBlockHeader>& keyLeader = nullptr, const bool& containsGenesisBlock = false, const std::vector<uint64_t>& PoWs = std::vector<uint64_t>(), const  std::shared_ptr<CBlockHeader>& blockLeaeder = nullptr, uint64_t startFromPosition = 0, bool optimize = false, bool requireCheckpoints = false, uint64_t barID = 100, bool needToKnowWhereBroken = true, const uint64_t& validUpUntil = 0);
	eChainProofValidationResult::eChainProofValidationResult verifyChainProofMT(const std::vector <std::vector<uint8_t>>& chainProof, uint64_t& cumulativeDiff, bool validateAgainsLocalData = true, bool flashCurrentCount = false, const  std::shared_ptr<CBlockHeader>& keyLeader = nullptr, const bool& containsGenesisBlock = false, const std::vector<uint64_t>& PoWs = std::vector<uint64_t>(), const  std::shared_ptr<CBlockHeader>& blockLeaeder = nullptr, uint64_t startFromPosition = 0, bool optimize = false, bool requireCheckpoints = false, uint64_t barID = 100, const uint64_t& validUpUntil = 0, uint32_t numThreads = 9);
	bool getPathForBlock(eChainProof::eChainProof whichChainProof, std::vector<std::vector<uint8_t>>& path, std::vector<uint8_t> blockIDA = std::vector<uint8_t>(), std::vector<uint8_t> blockIDB = std::vector<uint8_t>());
	//bool verifyPath(std::vector<std::vector<uint8_t>> path);
	void setSyncIsStuck(bool isIt = true);
	bool getIsSyncStuck();
	//any external chain-proof. *NOT* for the mVerifiedChainProof.
	eChainProofUpdateResult::eChainProofUpdateResult analyzeAndUpdateChainProof(std::vector<std::vector<uint8_t>> subProof, eChainProofValidationResult::eChainProofValidationResult& result, std::vector<std::vector<uint8_t>>& chain, uint64_t& totalDiffOfNewChainProof, const uint64_t& blocksScheduledForDownloadCount = 0, const std::vector<uint8_t>& receivedPackedChainProof = std::vector<uint8_t>(), uint64_t barID = 999, bool doColdStorage = true, const std::vector<std::vector<uint8_t>>& resultingCompleteChainProof = std::vector<std::vector<uint8_t>>());
	void updateHeaviestPathLeaders();
	//only for the heaviest known chain proof. not the verified one.
	eChainProofUpdateResult::eChainProofUpdateResult updateHeaviestChainProof(std::vector<std::vector<uint8_t>> subProof, uint64_t& totalDiffOfNewChainProof);

	eChainProofCommitResult::eChainProofCommitResult commitHeaviestChainProofBlocks(uint64_t& nrOfPushedBlocks, uint64_t& nrOfBlocksScheduleForDownload);
	eChainProofUpdateResult::eChainProofUpdateResult analyzeAndUpdateChainProofExt(std::vector<std::vector<uint8_t>> subChain, eChainProofValidationResult::eChainProofValidationResult & result, std::vector<std::vector<uint8_t>>& resultingCompleteChainProof, uint64_t& totalDiffOfNewChainProof, const uint64_t& blocksScheduledForDownloadCount = 0, const std::vector<uint8_t>& receivedPackedChainProof = std::vector<uint8_t>(), uint64_t barID = 999, bool doColdStorage = true);
	uint64_t getSyncPercentage();
	void setSyncPercentage(uint64_t percentage);
	bool attemptToLockHeaviestPath();
	void unlockHeaviestPath();
	//local chain-proofs

	//update based on a new leader
	bool updatePathWithLeader(std::shared_ptr<CBlock> block, eChainProof::eChainProof which, bool updatePathInColdStorage = true);

	//explicitly set with provided data
	bool setPath(std::vector<std::vector<uint8_t>> path, eChainProof::eChainProof whichOne, bool onlyColdStorage = false, bool doColdStorage = true);
	bool setChainProof(const std::vector<std::vector<uint8_t>>& proof, eChainProof::eChainProof whichOne, bool onlyColdStorage = false, const std::vector<uint8_t>& receivedPackedChainProof = std::vector<uint8_t>(), bool doColdStorage = true);

	//Blockchain memory-information utilities
	size_t getTransactionMemPoolSize();

	/**
	 * @brief Cleans up the blockchain memory cache by pruning and removing blocks.
	 *
	 * This method manages the blockchain's in-memory cache by:
	 * 1. Keeping a specified number of blocks unpruned (with full Merkle tries loaded).
	 * 2. Pruning additional blocks (freeing Merkle tries) up to a specified limit.
	 * 3. Removing excess blocks beyond both unpruned and pruned limits.
	 *
	 * The cleanup process traverses the blockchain from the most recent block (leader)
	 * towards older blocks, making decisions based on configured limits.
	 *
	 * @details
	 * - Unpruned blocks: Keeps Merkle tries loaded for quick access.
	 * - Pruned blocks: Frees Merkle tries to save memory while retaining block data.
	 * - Removed blocks: Prepares blocks for deletion when beyond both unpruned and pruned limits.
	 *
	 * The method respects three main limits:
	 * 1. Maximum number of unpruned blocks (full blocks with Merkle tries).
	 * 2. Maximum number of pruned blocks (blocks with Merkle tries freed).
	 * 3. Overall maximum number of blocks to keep in memory.
	 *
	 * @note This method is thread-safe, using mutex locks to protect shared resources.
	 *
	 * @warning This operation can potentially break the continuity of the blockchain
	 * in memory if blocks are removed. Ensure that critical operations are not in progress
	 * when calling this method.
	 *
	 * @throws Potential exceptions from underlying operations like loadTries() or freeTries().
	 *
	 * @see getMaxNrOfFullBlocksToKeepInMemory()
	 * @see getMaxNrOfPrunedBlocksToKeepInMemory()
	 * @see getMaxOverallBlocksToKeepInMemory()
	 */
	void CBlockchainManager::cleanCache();


	uint64_t getHotStorageCacheLength(bool recalculate = false);
	uint64_t previousIterationHSCachelength;
	uint64_t previousIterationBlockchainHeight;
	// Inherited via IManager
	virtual void stop() override;

	virtual void pause() override;

	virtual void resume() override;

	virtual eManagerStatus::eManagerStatus getStatus() override;


	// Inherited via IManager
	virtual void setStatus(eManagerStatus::eManagerStatus status) override;


	// Inherited via IManager
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status) override;

	virtual eManagerStatus::eManagerStatus getRequestedStatusChange() override;

};