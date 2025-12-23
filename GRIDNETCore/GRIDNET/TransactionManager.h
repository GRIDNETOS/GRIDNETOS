#pragma once
#include "stdafx.h"

#include <map>
#include <unordered_set>
#include <atomic>
#include "IManager.h"
#include <thread>
#include "enums.h"
#include <shared_mutex>
enum eTransactionCommitResult { success, failure };
enum eTransactionValidationResult { valid, invalid, unknownIssuer,insufficientERG,ERGBidTooLow,pubNotMatch,noIDToken,noPublicKey,invalidSig,invalidBalance,incosistentData,validNoTrieEffect,invalidSacrifice, invalidNonce, forkedOut, received, invalidBytecode};
enum eTransactionSortingAlgorithm { feeHighestFirst, recentFirst, feesHighestFirstNonce };
enum eBlockFormationResult { emptyRegularBlock, blockFormed, invalidTransaction, notEnoughTransactions,aborted,unknownLeader,noKeyChain,coinMintProblem,initial,PoWFailed,recentKeyBlockUnavailable,notALeader,leaderNotAvailableInCS,awaitingGenesisKeyBlock, cantCalculatePastEpochReward};
enum eBlockFormationStatus { idle, forming, enqueuingForProcessing,processing};
enum TriggerType {sourceDomainID // Represents a trigger type of a domain/account address
		// Other trigger types can be added here in the future
	};
class CTransactionDesc;
class CWorkManager;
class CBlockchainManager;
class CReceipt;
class CGlobalSecSettings;
class CBlock;
class CVerifier;
class CVerifiable;
class CSettings;
class CTools;
class CLinkContainer;
class CDTI;
class CObjectThrottler;


class CProcessVerifiablesRAII final
{
public:
	CProcessVerifiablesRAII(std::stringstream& logRef, std::shared_ptr<CTools> toolsRef)
		: mLogRef(logRef), mToolsRef(toolsRef)
	{
	}

	~CProcessVerifiablesRAII()
	{
		// This is called automatically no matter how the function returns
		if (mToolsRef)
		{
			mToolsRef->writeLine(mLogRef.str());
		}
	}

private:
	std::stringstream& mLogRef;
	std::shared_ptr<CTools> mToolsRef;
};
class CEndFlowRAII final
{
public:
	CEndFlowRAII(std::stringstream& logRef, std::shared_ptr<CTools> toolsRef)
		: mLogRef(logRef), mToolsRef(toolsRef)
	{
	}

	~CEndFlowRAII()
	{
		// This runs automatically on every return path from endFlow(...)
		if (mToolsRef)
		{
			// Dump the entire debugLog content
			mToolsRef->writeLine(mLogRef.str());
		}
	}

private:
	std::stringstream& mLogRef;
	std::shared_ptr<CTools>  mToolsRef;
};

class CProcessTransactionRAII final
{
public:
	CProcessTransactionRAII(std::stringstream& logRef, std::shared_ptr<CTools> toolsRef)
		: mLogRef(logRef), mToolsRef(toolsRef)
	{
	}

	~CProcessTransactionRAII()
	{
		// This will be called automatically on every return path
		// from the function that creates a CProcessTransactionRAII.
		if (mToolsRef)
		{
			// Dump the entire debugLog content
			mToolsRef->writeLine(mLogRef.str());
		}
	}

private:
	std::stringstream& mLogRef;
	std::shared_ptr<CTools> mToolsRef;
};


namespace SE {
	class CScriptEngine;
}

class CCryptoFactory;
class CTrieDb;
class CStateDomainManager;
class CTransaction;
class CTransactionManager :public IManager,public  std::enable_shared_from_this<CTransactionManager>
{
	/*
	Warning: Note that the manager needs to be marked externally as ready in order to assure proper operation and ability to shut-down.
	*/
public: 
	uint64_t getLastTimeMemPoolAdvertised();
	uint64_t getLastControllerLoopRun();
	uint64_t getPowCustomBarID();
private:

	// ACID Flow Tranasctions' Stack - BEGIN

	/// State Trie DB stack depth counters - BEGIN

	/// Tracks the total number of State Trie DB snapshots across all transactions in the current flow.
	/// This counter is incremented with each DB snapshot and decremented only when stepping back.
	/// When block processing completes successfully:
	/// - Value equals the number of successful transactions (each adds one final snapshot)
	/// - Failed/reverted transactions are not included in the final count
	/// Used to verify proper state progression across an entire block of transactions.
	uint64_t mInFlowTotalTXDBStackDepth;

	/// Tracks State Trie DB snapshots for the current transaction only.
	/// Reset to 0 before processing each new transaction.
	/// During transaction processing:
	/// - Incremented with each intermediary DB snapshot
	/// - When transaction commits: Final value is 1 (the committed state)
	/// - When transaction reverts: Final value is 0 (all states discarded)
	/// Used for stepping back through states of the current transaction.
	uint64_t mInFlowTXDBStackDepth;

	/// State Trie DB stack depth counters - END

	/**
	* @brief Increments both state stack depth counters
	*
	* Called when creating a new State Trie DB snapshot during transaction processing.
	* Increments:
	* - mInFlowTotalTXDBStackDepth: Tracking total snapshots across all transactions
	* - mInFlowTXDBStackDepth: Tracking snapshots for current transaction
	*
	* [IMPORTANT]: Each successful transaction adds exactly one snapshot to the final
	* total count, representing its committed state.
	*
	* Thread-safe: Protected by mFieldsGuardian recursive mutex
	*/
	void incInFlowTXDBStackDepth();

	/**
	* @brief Steps back through State Trie DB snapshots
	*
	* Decrements both counters when reverting to a previous state:
	* - mInFlowTotalTXDBStackDepth: Always decremented (tracks total snapshots)
	* - mInFlowTXDBStackDepth: Decremented only if > 0 (tracks current transaction)
	*
	* This maintains consistency as both counters reference the same underlying
	* stack of State Trie DB snapshots from different perspectives.
	*
	* Thread-safe: Protected by mFieldsGuardian recursive mutex
	* @pre mInFlowTotalTXDBStackDepth must be > 0
	*/
	void decInFlowTXDBStackDepth();

	/**
	* @brief Prepares for new transaction by resetting current transaction depth
	*
	* [IMPORTANT]: Call ONLY before starting a new transaction.
	* Sets mInFlowTXDBStackDepth = 0 while preserving mInFlowTotalTXDBStackDepth.
	*
	* After transaction completion:
	* - Successful commit: mInFlowTXDBStackDepth = 1 (committed state)
	* - Revert/failure: mInFlowTXDBStackDepth = 0 (all states discarded)
	*
	* Thread-safe: Protected by mFieldsGuardian recursive mutex
	*/
	void resetCurrentTXDBStackDepth();

	/**
	* @brief Provides access to State Trie DB stack depth counters
	*
	* Returns either:
	* - Total depth (getTotalDepth=true): Number of snapshots across all transactions,
	*   equals number of successful transactions when block processing completes
	* - Current transaction depth (getTotalDepth=false): Number of snapshots for
	*   current transaction only, 0-N during processing, 0 or 1 after completion
	*
	* Thread-safe: Protected by mFieldsGuardian recursive mutex
	* @param getTotalDepth Selects which counter to return
	* @return Selected stack depth counter value
	*/
	uint64_t getInFlowTXDBStackDepth(bool getTotalDepth = true);


	// ACID Flow Tranasctions' Stack - END

	std::shared_ptr<CObjectThrottler> mThrottler;
	uint64_t mProcessedTransactions = 0;
	bool mHardForkBlockAlreadyDispatched;
	std::shared_ptr<CObjectThrottler> getThrottler();

	uint64_t mPoWCustomBarID =0;
	void pingtLastControllerLoopRun();
	void pingLastTimeMemPoolAdvertised();
	uint64_t mLastControllerLoopRun;
	std::uint64_t mLastTimeMemPoolAdvertised;
	std::mutex mStatusChangeGuardian;
	std::vector<uint8_t> debugInitial, debugAfterT, debugAfterV, debugFinal;
	uint64_t mDataBlocksGeneratesUsingSamePubKey;
	uint64_t mKeyBlocksGeneratesUsingSamePubKey;
	uint64_t mNewPubKeyEveryNKeyBlocks;
	uint64_t mCycleBackIdentiesAfterNKeysUsed;
	std::mutex mBlockchainModeGuardian;
	bool mSynchronizeBlockProduction;
	std::mutex mSynchronizeBlockProductionGuardian;
	uint64_t mBlockProductionHaltedTillTimeStamp;
	bool mMinTransactionPerDataBlock = 0;//0 - unlimited. if less a data-block wouldn't be formed
	bool mInformedNotLeader;
	bool mNotifiedWaitingForTransactions;
	//std::mutex mWaitingForFlowGuardian;
	std::mutex mBlockProductionHaltedTillTimeStampGuardian;
	std::mutex mIsInFlowGuardian;
	std::mutex mInitialStatusQueryGuardian;
	//bool mDoNotLockChainGuardian;
	bool mInKeyBlockFlow;
	uint64_t mKeyBlocksFormedCounter;
	uint64_t mDataBlocksFormedCounter;
	eTransactionsManagerMode::eTransactionsManagerMode mMode;
	std::mutex mNotificationsGuardian;
	std::mutex mStatisticsCounterGuardian;
	std::mutex mBlocksCounterGuardian;
	std::mutex mModeGuardian;
	std::recursive_mutex mFieldsGuardian;
	std::recursive_mutex mStatDataGuardian;


	
	uint64_t mRequiredMemPoolObjectsCached;
	uint64_t mTXsPendingInMemPoolCached;
	uint64_t mProcessedVerifiables;
	std::vector<uint8_t> mForcedMiningParentBlockID;
	uint32_t mAllowedInterFlowSteps = 500;//how many steps can we go backwards (larger values require more RAM) and increase the number of memory operations.
	//std::vector<CReceipt*> mReceiptsToBeIncluded;
	//std::vector<CTrieNode*> markedForDeletion;
	uint64_t mAverageERGBid;
	bool mDoBlockFormation;  // Unified control for both key and data block formation
	eBlockFormationStatus mKeyBlockFormationStatus;
	eBlockFormationStatus mRegBlockFormationStatus;
	bool mAnonimityModeOn;
	bool mReadyForNextBlock;

	std::string mMinersKeyChainID;
	std::vector<uint8_t> mCurrentMiningTaskID ;
	//std::vector<CVerifiable> mBeneficiaries;
	std::shared_ptr<CCryptoFactory>  mCryptoFactory;
	std::shared_ptr<CWorkManager> mWorkManager;
	std::recursive_mutex mGuardian;
	std::mutex mThreadManagementMutex;
	std::atomic<bool> mPersistentDBUpdateInProgress{ false };
	uint64_t mLastDevReportMode;//0 - mhps; 1 - kernel exec time.
	uint64_t mLastDevReportModeSwitch;
	//std::mutex mSettingUpFlowGuardian;
	//std::mutex mBlockReadyGuardian;
	std::recursive_mutex mFlowGuardian;//locked ONLY during entire lifetime of an accepted flow (after mInFlow=true); used to prevent other threads from aborting the flow they dont own.
	//the check is made through locking the above recursive mutex. (this would block on another thread) before the isInfLow() is executed.
	std::recursive_mutex mFlowProcessingGuardian;
	std::recursive_mutex mCurrentDBGuardian;
	//std::mutex mTransactionProsessingGuardian;
	//bool mIsTestDB = false;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	bool mIsLIVEDBDetached;
	std::recursive_mutex  mMemPoolGuardian;
	std::recursive_mutex  mFlowStateDBGuardian;
	std::recursive_mutex  mLiveStateDBGuardian;
	std::vector<std::shared_ptr<CTrieNode>> mMemPool;

	// Phantom Mode Transaction Tracking - BEGIN
	// Tracks transaction IDs that have been processed in phantom mode to avoid
	// reprocessing the same transactions indefinitely. Uses string representation
	// of TX IDs for efficient O(1) lookup in unordered_set.
	mutable std::mutex mPhantomProcessedTxGuardian;
	std::unordered_set<std::string> mPhantomProcessedTxIDs;
	// Phantom Mode Transaction Tracking - END

	// Phantom Mode Processing Report - BEGIN
	// Stores detailed entries for each transaction processed in phantom mode.
	// Used by 'chain -phantom report' to display processing history.

	/// @brief Entry for a single transaction processed in phantom mode.
	/// @note Defined here (before use) to avoid forward declaration issues.
	struct PhantomTxReportEntry
	{
		std::vector<uint8_t> receiptID;      // Receipt ID (from receipt.getGUID())
		uint64_t processedAt;                // Unix timestamp when processed
		eTransactionValidationResult result; // Processing result (enum)
		std::string resultText;              // Human-readable result (from translateStatus())
		BigInt ergUsed;                      // ERG consumed
		BigInt ergPrice;                     // ERG price
		std::vector<std::string> logEntries; // Log entries from receipt (GridScript output, errors, etc.)
	};

	mutable std::mutex mPhantomReportGuardian;
	std::vector<PhantomTxReportEntry> mPhantomReportEntries;
	// Phantom Mode Processing Report - END

	std::vector<uint8_t> mCurrentFlowObjectID;
	uint64_t mTXsRevertedCount;
	uint64_t mTXsCleanedCount;
	bool popCurrentTXDBs();
	bool popDB(size_t nrOfDBs=1);
	void pushDB(CTrieDB* db, bool isTXDB=false, const std::vector<uint8_t> & objectID = std::vector<uint8_t>());

/**
 * @brief Completely reverts all effects of the current transaction
 *
 * Reverts all changes made during the transaction processing, including:
 * - GridScript execution effects
 * - Transaction fees
 * - State changes in the Merkle Patricia Trie
 * - Any other side effects stored in the state stack
 *
 * The reversion process iteratively steps back through the state stack
 * until all intermediate states are unwound. Each step reverts one atomic
 * change that was part of the transaction. The process continues until
 * the state stack is empty (mInFlowTXDBStackDepth reaches 0), indicating
 * a complete reversion to the pre-transaction state.
 *
 * @note This is typically called when a transaction fails or needs to be
 *       rolled back to maintain database consistency
 *
 * @return The number of atomic steps that were reverted during the process
 */
	bool revertTX();
	std::string genThreadName(bool isBlockFactoryThread=false,bool keyBlocks=true);
	CTrieDB * mFlowStateDB;//temporary stateDatabase for transaction processing; it might contain different 
	//root node that the official one in CBlockchainManager
	CTrieDB * mLiveStateDB;// pointer to the Live, production State Trie.
	CTrieDB * mCurrentStateDB;
	std::vector<std::thread> mCleanUps;
	CTrieDB * mBackupStateDB;

	

	// Persistent DB - BEGIN
	// Double Buffering - BEGIN
	CTrieDB* mPersistentStateDB;  // Main persistent DB. Maintains a complete in-memory state trie, never pruned
	CTrieDB* mPersistentStateDBDouble; // Update buffer
	mutable std::shared_mutex mPersistentDBSwapMutex;         // Protect buffer swaps
	// Double Buffering - END

	bool mKeepPersistentDB;
	bool mPersistenStateDBPopulated;
	std::shared_ptr<CStateDomainManager> mPersistentStateDomainManager;
	std::mutex mDataBlockFormationGuardian;
	std::shared_ptr<CStateDomainManager>  mStateDomainManager;
	uint64_t mPersistentDBUpdateInterval; // Interval in seconds for persistent DB updates
	uint64_t mLastPersistentDBUpdate;     // Timestamp of last persistent DB update
	std::recursive_mutex mPersistentDBGuardian;
	// Persistent DB - END

	size_t mUtilizedBlockSize; 
	BigInt mUtilizedERG;
	BigInt mTotalGNCFees;
	BigInt mTreasuryTax;
	BigInt mLastActualTransferredValue; // Stores actual value transferred in last processTransaction() call
	BigInt mBalanceBeforeTransaction; // Stores balance before transaction processing
	uint64_t mLastKeyBlockFormationAttempt;
	std::shared_ptr<SE::CScriptEngine> mScriptEngine;
	std::shared_ptr<CBlockchainManager> mBlockchainManager;

	//mempool
	CVerifier * mVerifier;
	CGlobalSecSettings * mGlobalSecSettings;
	uint64_t mInFlow;//is transaction flow in progress>
	std::vector<uint8_t> mBackupStateRootID;
	std::vector<std::vector<uint8_t>> mInFlowInterPerspectives;//contains roots of StateTrie after each transactions is processed.
	std::vector<CTrieDB*> mInFlowInterDBs;
	//thus it is possible to step back one step.
	std::vector<std::shared_ptr<CTransaction>> mInFlowTransactions;
	std::vector<std::shared_ptr<CVerifiable >> mInFlowVerifiables;
	std::vector<std::shared_ptr<CLinkContainer>> mInFlowLinks;//Links to be created during a Flow.
	std::string mNamePrefix;
	std::mutex mNamePrefixGuardian;
	std::mutex mInFlowLinksGuardian;
	bool addLinkToFlow(std::shared_ptr<CLinkContainer> link);

	void clearInFlowLinks();

	std::thread mController;
	std::thread mKeyBlockFactoryT;
	std::thread mRegBlockFactoryT;

	// Thread Health Monitoring - BEGIN
	// Heartbeat timestamps to detect externally killed threads
	std::atomic<uint64_t> mKeyBlockThreadHeartbeat{ 0 };
	std::atomic<uint64_t> mDataBlockThreadHeartbeat{ 0 };
	static constexpr uint64_t THREAD_HEARTBEAT_TIMEOUT_SECONDS = 60; // Consider thread dead if no heartbeat for 60 seconds
	void pingKeyBlockThreadHeartbeat();
	void pingDataBlockThreadHeartbeat();
	bool isKeyBlockThreadAlive();
	bool isDataBlockThreadAlive();
	void checkAndRestartBlockFormationThreads();
	// Thread Health Monitoring - END

	void mControllerThreadF();
	void keyBlockMinerThread();
	void manufactureBlocks(bool keyBlock);
	std::shared_ptr<CBlockchainManager> getBlockchainManager();
	void dataBlockMinerThread();
	bool  mUpdatePerspectiveAfterBlockProcessed;
	std::shared_ptr<CTools> mTools;
	std::shared_ptr<CTools> getTools();
	//transaction validation
	BigInt mMaxERGUsage;
	CReceipt processTransaction(std::vector<uint8_t> BERpackedTransaction,uint64_t keyHeight);

	std::vector<uint8_t> getCurrentFlowObjectID();
	void setCurrentFlowObjectID(const std::vector<uint8_t>& id);
	
	CReceipt processTransaction(CTransaction & trans,uint64_t blockHeight, std::vector<uint8_t> &confirmedDomainID, std::vector<uint8_t> receiptID= std::vector<uint8_t>(), const uint64_t &nonce = 0, std::shared_ptr<CReceipt> existingReceipt = nullptr, std::shared_ptr<CBlock> existingBlock = nullptr, bool excuseERGusage=false, bool enforceFailure = false);
	bool saveSysValue(bool doInSandbox,eSysDir::eSysDir dir , std::vector<uint8_t> key, std::vector<uint8_t> value,uint64_t &cost, bool revertPath=true);
	CReceipt processVerifiable(CVerifiable ver,  BigInt &rewardValidatedUpToNow, BigInt& rewardIssuedToMinerAfterTAX,  std::vector<uint8_t> receiptID = std::vector<uint8_t>(), std::shared_ptr<CBlock> proposal = 0);
	//size_t mMaxBlockSize;

	eBlockFormationResult formBlock(std::shared_ptr<CBlock>& block,bool formKeyBlock, CKeyChain chain, std::vector<uint8_t> forcedParentID=std::vector<uint8_t>(), bool phantomMode=false);
	
	bool powLoop(std::shared_ptr<CBlock> proposal);
	bool mInSandbox;
	void cleanMemPool();
	//std::vector<CReceipt*> mPendingReceipts;
	void deleteMemPoolObject(int &i);
	CTransaction genSimpleTransaction(size_t ammount, std::vector<uint8_t> source, std::vector<uint8_t> destination, std::vector<uint8_t> sig);
	eManagerStatus::eManagerStatus mStatus;
	eManagerStatus::eManagerStatus mStatusChange;
	std::vector<uint8_t> mNextMiningPerspective;
	uint64_t mDifficultyMultiplier;
	bool mWasForcedPerspectiveProcessed;
	bool mWasForcedDifficultyMultiplierProcessed;
	std::recursive_mutex mReceiptsGuardian;
	std::map<std::vector<uint8_t>, std::vector<uint8_t>> mTransactionsReceiptsIndex;
	uint64_t mMaxReceiptsIndexCacheSize;
	bool registerTransactionWithReceiptID(const CTransaction & trans, std::vector<uint8_t> receiptID);
	bool registerVerifiablenWithReceiptID(const CVerifiable & trans, std::vector<uint8_t> receiptID);
	bool addToMemPool(std::shared_ptr<CTrieNode> object);
	bool isObjectInMemPool(std::shared_ptr<CTrieNode> object);
	void setBlockFormationStatus(bool keyBlock,eBlockFormationStatus status);
	size_t mLastTimeMemPoolCleared;
	uint64_t mIsSettingUpFlow;
	bool getWasForcedPerspectiveProcessed();
	void setWasForcedPerspectiveProcessed(bool was=true);

	uint64_t mFraudulantDataBlocksToGeneratePerKeyBlock;
	uint64_t mFraudulantDataBlocksOrdered;
	uint64_t mProofsOfFraudProcessed;
	BigInt mTotalValueOfPenalties;
	BigInt mTotalValueOfPoFRewards;
	uint64_t mNewFraudsDetected;
	uint64_t mFraudsDetectedButAlreadyRewarded;
	uint64_t mNrOfAttemptsToSpendLockedAssets;
	uint64_t mNrOfTransactionsFailesDueToOutofERG;
	uint64_t mNrOfTimesTransactionsFailedDueToInsufficientFunds;
	uint64_t mNrOfTransactionsWithInvalidNonceValues;
	uint64_t mNrOfTimesDeferredAssetsReleased;
	//
	uint64_t mNrOfTTransactionsFailedDueToInvalidEnvelopeSig;
	uint64_t mNrOfTTransactionsFailedDueToInvalidNonce;
	uint64_t mNrOfTransactionsFailedDueToUnknownIssuer;
	uint64_t mNrOfInvalidTransactionNotIncludedInBlock;
	std::weak_ptr<CDTI> mDTI;
	std::mutex mDTIGuardian;
	std::shared_ptr<CDTI> getDTI();
	bool mPastGracePeriod;

	uint64_t getTXsRevertedCount();
	uint64_t getTXsCleanedCount();
	void incTXsRevertedCount();
	void incTXsCleanedCount();
public:

	// Thread Health Status - public accessors
	bool getIsKeyBlockThreadAlive();
	bool getIsDataBlockThreadAlive();

	// Persistent DB accessors
	void setKeepPersistentDB(bool keep = true);
	bool getKeepPersistentDB();
	void setPersistentDBUpdateInterval(uint64_t seconds);
	uint64_t getPersistentDBUpdateInterval();
	std::shared_ptr<CStateDomainManager> getPersistentStateDomainManager();

	void setWasPersistentDBPopulated();

	bool getWasPersistentDBPopulated();


	std::vector<uint8_t> getCurrentMinigTaskID();
	void setCurrentMiningTaskID( std::vector<uint8_t>& id);
	void setIsPastGracePeriod(bool isIt = true);
	bool getIsPastGracePeriod();
	void setTXInMemPoolCached(uint64_t value);
	uint64_t getTXInMemPoolCached();

	void setReqMemPoolObjectsCached(uint64_t value);
	uint64_t getLastKeyBlockFormationAttempt();
	void pingLastKeyBlockFormationAttempt();
	uint64_t getReqMemPoolObjectsCached();
	uint64_t advertiseMemPoolContents();
	bool initialize(std::shared_ptr<SE::CScriptEngine> se=nullptr);
	void setNamePrefix(std::string name);
	uint64_t getNrOfTimesDeferredAssetsReleased();
	bool isLinkInFlow(std::shared_ptr<CLinkContainer> link);
	void setFraudulantDataBlocksToGeneratePerKeyBlock(uint64_t nr);
	uint64_t getFraudulantDataBlocksToGeneratePerKeyBlock();
	uint64_t getKeyBlocksGeneratesUsingSamePubKey();
	void incKeyBlocksGeneratesUsingSamePubKey();
	void incFraudulantDataBlocksOrdered();
	void incNrOfTimesDeferredAssetsReleased();
	uint64_t getFraudulantDataBlocksOrdered();
	void incNewFraudsDetected();
	uint64_t getNewFraudsDetected();
	void incNrOfTimesTransactionsFailedDueToInsufficientFunds();
	uint64_t getNrOfTimesTransactionsFailedDueToInsufficientFunds();
	void incFraudsDetectedButAlreadyRewarded();
	uint64_t getFraudsDetectedButAlreadyRewarded();
	void incTotalValueOfFraudPenaltiesBy(BigInt val);
	BigInt getTotalValueOfFraudPenalties();
	void incTotalValueOfPoFRewardsBy(BigInt val);
	BigInt getTotalValueOfPoFRewards();
	void incNrOfTransactionsFailesDueToOutofERG();
	uint64_t getNrOfTransactionsFailesDueToOutofERG();
	void incNrOfTransactionsFailesDueToInvalidEnvelopeSig();
	uint64_t getNrOfTransactionsFailesDueToInvalidEnvelopeSig();
	void incNrOfTransactionsFailesDueToInvalidNonce();
	uint64_t getNrOfTransactionsFailesDueToInvalidNonce();
	void incNrOfTransactionsFailesDueToUnknownIssuer();
	uint64_t getNrOfTransactionsFailesDueToUnknownIssuer();
	void incNrOfTransactionsFailesDueToNotIncludedInBlock();
	uint64_t getNrOfTransactionsFailesDueToNotIncludedInBlock();
	void incNrOfAttemptsToSpendLockedAssets();
	uint64_t getNrOfAttemptsToSpendLockedAssets();
	void incProofsOfFraudProcessed();
	uint64_t getProofsOfFraudProcessed();
	void incNrOfTransactionsWithInvalidNonceValues();
	uint64_t getNrOfTransactionsWithInvalidNonceValues();
	void resetKeyBlocksGeneratesUsingSamePubKey();
	uint64_t getDataBlocksGeneratedUsingSamePubKey();
	void incDataBlocksGeneratesUsingSamePubKey();
	void resetDataBlocksGeneratesUsingSamePubKey();
	void setNewPubKeyEveryNKeyBlocks(uint64_t blockCount);
	uint64_t getNewPubKeyEveryNKeyBlocks();
	void setCycleBackIdentiesAfterNKeysUsed(uint64_t keysCount);
	uint64_t getCycleBackIdentiesAfterNKeysUsed();
	eBlockchainMode::eBlockchainMode getBlockchainMode();
	bool getSynchronizeBlockProduction();
	void setSynchronizeBlockProduction(bool doIt = true);
	bool haltBlockProductionFor(size_t timestamp);
	bool isBlockProductionHalted();
	size_t getBlockProductionResumptionTimestamp();
	std::vector<uint8_t> getForcedMiningBlockID();
	bool getInformedWaitingForTransactions();
	void setInformedWaitingForTransactions(bool didThat = true);
	bool getDoBlockFormation();
	void setDoBlockFormation(bool doIt=true, bool resetThreads=true);
	eTransactionsManagerMode::eTransactionsManagerMode getMode();
	uint64_t getTransactionsCounter();
	uint64_t getVerifiablesCounter();
	void incProcessedVerifiablesCounter();
	uint64_t getFormedKeyBlocksCounter();
	uint64_t getFormedDataBlocksCounter();
	void incFormedKeyBlocksCounter();
	void incFormedDataBlocksCounter();
	bool getIsInFlow();
	void setIsInFlow(bool is = true);
	void setIsSettingUpFlow(bool is = true);
	bool getIsSomebodyElseSettingUpFlow();
	bool registerTransaction(const CTransaction& trans, std::vector<uint8_t>& receiptID, bool genMetaData=true);
	

	bool registerVerifiable(const CVerifiable & ver, std::vector<uint8_t>& receiptIDToRet);

	std::vector<uint8_t> getReceiptIDForTransaction(const CTransaction & trans);
	std::vector<uint8_t> getReceiptIDForTransaction(std::vector<uint8_t> transactionID,std::shared_ptr<CBlock> block=nullptr);
	std::vector<uint8_t> getReceiptIDForVerifiable(const CVerifiable & trans);
	std::vector<uint8_t> getReceiptIDForVerifiable(std::vector<uint8_t> verifiableID, std::shared_ptr<CBlock> block = nullptr);
	std::vector<uint8_t> getForcedMiningPerspective();
	bool forceMiningPerspectiveOfNextBlock(std::vector<uint8_t> ID= std::vector<uint8_t>());
	bool forceOneTimeParentMiningBlockID(std::vector<uint8_t> ID = std::vector<uint8_t>());
	std::shared_ptr<SE::CScriptEngine> getScriptEngine();
	CTrieDB* getFlowDB();
	CTrieDB* getLiveDB();
	std::shared_ptr<CWorkManager> getWorkManager();
	std::shared_ptr<CStateDomainManager>  getStateDomainManager();
	void processUncleBlocks(std::vector <std::shared_ptr<CBlock>>  &blocks);
	bool processTransactions(std::vector<std::shared_ptr<CTransaction>>& transactionsToBeProcessed, std::vector<CReceipt> &receipts,bool markAsProcessed=true, std::shared_ptr<CBlock> proposal=NULL,bool addToBlock=false,bool verifyAgainstBlock=false, bool processBreakpoints=false);
	bool processVerifiables(std::vector<std::shared_ptr<CVerifiable>> &verifiablesToBeProcessed, std::vector<CReceipt> &receipts,  BigInt &rewardValidatedUpToNow , BigInt& rewardIssuedToMinerAfterTAX, bool markAsProcessed=true, std::shared_ptr<CBlock> proposal=NULL, bool addToBlock=false, bool verifyAgainstBlock=false);
	void processReceipts(std::vector<CReceipt > &receiptsToBeProcessed, std::shared_ptr<CBlock> proposal);
	std::vector<uint8_t> getPerspective(int flowPerspective=-1);
	bool setPerspective(std::vector<uint8_t> perspectiveID);
	std::shared_ptr<CTrieNode> findMemPoolObjectByID(std::vector<uint8_t> id, eObjectSubType::eObjectSubType objectType);
	std::shared_ptr<CTransaction> findTransactionByID(std::vector<uint8_t> id);
	std::shared_ptr<CVerifiable> findVerifiableByID(std::vector<uint8_t> id);
	BigInt getAverageERGBid();
	CSettings * mSettings;
	void abortKeyBlockFormation();
	void abortDataBlockFormation();
	enum eDBTransactionFlowResult { success,failure};
	bool startFlow(std::vector<uint8_t> initialPerspective = std::vector<uint8_t>(), bool abortOnLock = false);
	eDBTransactionFlowResult endFlow(bool commit,bool doInitialCleanUp=true, bool updateKnownStateDomains = true,const std::string &errorMsg=std::string(), 
		const std::vector<CReceipt> &receipts = std::vector<CReceipt>(),const std::vector<uint8_t>&finalPerspective= std::vector<uint8_t>(),
		bool abort=false,std::shared_ptr<CBlock> blockProposal= NULL,
		bool doTrieVerification=false,
		const BigInt & finalTotalBlockRewardEffective = BigInt(0),
		const BigInt &finalPaidBlockRewardEffective = BigInt(0)
		);
	void cleanupFlow(bool cleanInFlowTransaction,bool revert=true, bool cleanBackupDB = true);
	bool stepBack(size_t nrOfSteps);
	bool abortFlow();
	bool setOneTimeDiffMultiplier(uint64_t multiplier);
	bool setDiffMultiplier(uint64_t multiplier);
	bool getIsRemoteTerminal();
	bool getHardForkBlockAlreadyDispatched();
	void setHardForkBlockAlreadyDispatched(bool wasIt=true);
	CTransactionManager(eTransactionsManagerMode::eTransactionsManagerMode mode, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CWorkManager> cwm,std::string minersKeyChainID,bool anonimityModeON,eBlockchainMode::eBlockchainMode instance,bool doBlockFormation=true,bool createDetachedDB=false,bool  doNOTlockChainGuardian = false,std::shared_ptr<CDTI> DTI=nullptr,bool muteConsole=false, bool keepPersistentDB = false);
	bool updateBlocksGeneratedWithSamePubKey();
	~CTransactionManager();

	uint64_t getUnprocessedTransactionsCount();

	// ============================================================================
	// Transaction Withholding Detection Support - BEGIN
	// ============================================================================
	/// @brief Gets the timestamp of when the oldest unprocessed transaction was received.
	/// @return Unix timestamp of the oldest unprocessed TX, or 0 if mem-pool is empty.
	/// @note Used by vitals monitoring to detect transaction withholding attacks.
	uint64_t getOldestUnprocessedTransactionTime();

	/// @brief Gets statistics about mem-pool transactions for withholding detection.
	/// @param oldestTxTime Output: timestamp of oldest unprocessed TX.
	/// @param txCount Output: number of unprocessed transactions.
	/// @return true if stats were retrieved successfully.
	bool getMemPoolWithholdingStats(uint64_t& oldestTxTime, uint64_t& txCount);
	// Transaction Withholding Detection Support - END
	// ============================================================================

	// ============================================================================
	// Phantom Mode Transaction Tracking - BEGIN
	// ============================================================================
	// [ Purpose ]:
	//   Tracks transactions that have been processed in Phantom Leader Mode.
	//   Since phantom mode doesn't mark transactions as processed (to keep them
	//   in mem-pool for real processing later), we need a separate tracking
	//   mechanism to avoid reprocessing the same transactions indefinitely.
	//
	// [ Behavior ]:
	//   - Transactions are added to this set after phantom block formation
	//   - Only transactions that WOULD have been marked as processed in normal
	//     mode are added (i.e., successfully included in the phantom block)
	//   - During phantom block formation, transactions in this set are skipped
	//   - The set is cleared when phantom mode is disabled or reset
	// ============================================================================

	/// @brief Marks a transaction as having been processed in phantom mode.
	/// @param txID The transaction ID to mark as phantom-processed.
	void markAsPhantomProcessed(const std::vector<uint8_t>& txID);

	/// @brief Checks if a transaction has already been processed in phantom mode.
	/// @param txID The transaction ID to check.
	/// @return true if the transaction was already phantom-processed.
	bool isPhantomProcessed(const std::vector<uint8_t>& txID) const;

	/// @brief Clears all phantom-processed transaction tracking.
	/// @note Called when phantom mode is disabled or reset.
	void clearPhantomProcessedTransactions();

	/// @brief Gets the count of transactions tracked as phantom-processed.
	/// @return Number of unique transactions phantom-processed this session.
	size_t getPhantomProcessedTransactionCount() const;

	// Phantom Mode Transaction Tracking - END
	// ============================================================================

	// ============================================================================
	// Phantom Mode Processing Report - BEGIN
	// ============================================================================
	// [ Purpose ]:
	//   Maintains a detailed report of transactions processed in phantom mode.
	//   This report provides insights into what would have happened if the node
	//   were the actual leader, helping diagnose transaction withholding issues.
	// ============================================================================

	/// @brief Adds an entry to the phantom processing report.
	/// @param receiptID Receipt ID (from receipt.getGUID())
	/// @param result Processing result
	/// @param ergUsed ERG consumed
	/// @param ergPrice ERG price per unit
	/// @param resultText Human-readable result text (from translateStatus())
	/// @param logEntries Log entries from receipt (GridScript output, errors, etc.)
	void addPhantomReportEntry(
		const std::vector<uint8_t>& receiptID,
		eTransactionValidationResult result,
		BigInt ergUsed,
		BigInt ergPrice,
		const std::string& resultText = "",
		const std::vector<std::string>& logEntries = std::vector<std::string>());

	/// @brief Gets the current phantom processing report entries.
	/// @return Vector of report entries (copy for thread safety).
	std::vector<PhantomTxReportEntry> getPhantomReport() const;

	/// @brief Clears the phantom processing report.
	void clearPhantomReport();

	/// @brief Gets summary statistics from the phantom report.
	/// @param totalTxs Output: total transactions processed
	/// @param successCount Output: successful transactions
	/// @param failCount Output: failed transactions
	/// @param totalErgUsed Output: total ERG consumed
	void getPhantomReportSummary(
		size_t& totalTxs,
		size_t& successCount,
		size_t& failCount,
		BigInt& totalErgUsed) const;

	// Phantom Mode Processing Report - END
	// ============================================================================

	bool processBreakpoints(const CTransaction& trans, const std::vector<uint8_t>& receiptID, uint64_t keyHeight, eBreakpointState::eBreakpointState state, std::shared_ptr<CReceipt> existingReceipt = nullptr, std::shared_ptr<CBlock> existingBlock = nullptr);

	eBlockFormationStatus getBlockFormationStatus(bool keyBlock);

	eTransactionValidationResult preValidateTransaction(std::shared_ptr<CTransaction> trans, uint64_t keyHeight, std::shared_ptr<CTransactionDesc> txDesc ) const;

	void doPerspectiveSync(bool doIt=true);
	bool getDoPerspectiveSync();
	bool addTransactionToFlow(std::shared_ptr<CTransaction> trans);
	bool addVerifiableToFlow(std::shared_ptr<CVerifiable>  trans);
	void untriggerTransactions(TriggerType triggerType, const std::vector<uint8_t>& triggerID);
	//transaction commit
	//transactions retrieval
	std::vector<std::shared_ptr<CTransaction>> getUnprocessedTransactions(eTransactionSortingAlgorithm alg, bool ommitTriggering = true, const std::vector<uint8_t> &tiggerID = std::vector<uint8_t>());
	std::vector<std::shared_ptr<CVerifiable>> getUnprocessedVerifiables(eTransactionSortingAlgorithm alg);
	bool removeTransactionFromMemPool(std::vector<uint8_t> id);

	size_t getMemPoolSize();
	void doBlockFormation(bool yesOrNo = true);

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

