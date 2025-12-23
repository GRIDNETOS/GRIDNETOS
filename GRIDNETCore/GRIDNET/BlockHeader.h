#ifndef BLOCK_HEADER_H
#define BLOCK_HEADER_H

//#include "stdafx.h"

#include "enums.h"
#include "TrieDB.h"

class CBlock;
class CReceipt;
class CTools;
class CTransaction;
class CVerifiable;
class CBlockchainManager;
enum eTrieID
{
	state,
	transactions,
	receipts,
	verifiables
};
/// <summary>
/// A block header.
/// A block header constitutes also a serialized key-block.
/// i.e. it is serialized into a 'key-block' if marked as such.
/// In a key-block there's only a stub block body when deserialized.
/// </summary>
class CBlockHeader
{

private:
	std::vector<uint8_t> mCachedOperatorID;
	size_t mHeight;
	size_t mKeyHeight;//sequentional number related to key-blocks only
	size_t mCoreVersion;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	bool mAvailableLocally;
	bool mIsTestNet;
	bool mIsKeyBlock;
	size_t mVersion;

	size_t mNonce;
	size_t mTarget;
	size_t mSolvedAt; // time since UNIX epoch
	bool mTriesLoaded;//  false if at least one of the tries gets unloaded (receipts, transactions, verifiables).

	//signature 
	std::vector<uint8_t> mSignature;
	std::vector<uint8_t> mPublicKey;

	//uncleBlocks
	std::vector<uint8_t> mUncleBlocksHash;

	//Merkle-Patricie Tries - IDs:
	std::vector<uint8_t> mInitiallStateRootID;
	std::vector<uint8_t> mFinalStateRootID;
	std::vector<uint8_t> mTransactionsRootID;
	std::vector<uint8_t> mReceiptsRootID;
	std::vector<uint8_t> mVerifiablesRootID;
	std::vector<uint8_t> mLogsBloom;
	std::vector<uint8_t> mExtraData;

	//erg-costs
	BigInt mErgLimit;
	BigInt mErgUsed;

	BigInt mTotalRewardEffective; // Hard Forks could have changed. Data in a block may be no longer Effective.
	BigInt mTotalReward;// Epoch 1 + Epoch 2 Reward (Excluding: Tax, Fraud Report Rewards,processing of any verifiables other than Miner's Rewards Verifiable).
	BigInt mPaidToLeader;
	BigInt mPaidToLeaderEffective;// Hard Forks could have changed. Data in a block may be no longer Effective.
	//the same value needs to be within the Leader's Verifiable within the blocks body. it is checked, both values are signed.

	//std::vector<uint8_t> mBeneficiary; - it was redundant with mPublicKey; we have getBefeniciary() to convert now instead
	
	std::vector<CVerifiable*> mBeneficiaries;
	size_t mNrOfTransactions;
	size_t mNrOfVerifiables;
	size_t mNrOfReceipts;
	//size_t mBlockReward; 

	// Merkle-Patricia Tries - DBs:
	//CTrieDB * mStateDB; //stores State Domains
	CTrieDB* mTransactionsDB; //stores individual Transactions (influencing state of the StateDB above)
	CTrieDB* mReceiptsDB; // stores receipts containing results of the transactions contained in the transactionsDB
	CTrieDB* mVerifiablesDB; // stores Verifiables (i.e. objcets representing proofs of certain events). These are verified by top-most state domain (core level) and might be verified by child state domains.

	CSolidStorage* mSs;
	std::weak_ptr<CBlock> mBlock;
	std::shared_ptr<CTools> mTools;
	std::shared_ptr<CCryptoFactory>  mCf;

	std::vector<uint8_t> beneficiariesHash;
	std::vector<uint8_t> mParentBlockID;    // this CAN change during PoW computation
	std::vector<uint8_t> mParentKeyBlockID; // this cannot change during Proof of Work computation
	std::shared_ptr<CBlock> mParentBlock;//used for efficient blockchain traversal.
	std::recursive_mutex mGuardian;
	std::shared_mutex mSharedGuardian;
	std::recursive_mutex mParentGuardian; // needs to be recursice since:
	/*
	* 1) the value may be set from nullptr to an actual pointer anytime while the block is in cache.
	* 2) a higher level API call getParent() may invoke lower level cache mechachin which may also require lock on parent (pointer)
	* 3) due to the dynamic and chaotic nature of adjecant matter we gladly play the overhead with a recursive mutex.
	*/
	unsigned long long mTotalWorkDone;//temp variable; we could never store total PoW in a block as individual nodes cannot trust such values;
	//cumulative PoW needs to verified on one's own.
	//static std::mutex mGeneratorLock;
	std::vector<uint8_t> mHashTemp;
public:

	void setCachedOperatorID(const std::vector<uint8_t>& id);
	std::vector<uint8_t> getCachedOperatorID();



	size_t getPackedTarget();
	bool validate(bool requireSig = true);
	std::vector<uint8_t> getParentKeyBlockID();
	bool  setParentKeyBlockID(std::vector<uint8_t> id);
	bool isKeyBlock();
	bool isHardForkBlock();
	uint64_t getTotalDiffField();
	void setTotalDiffField(uint64_t diff);
	std::vector<uint8_t> getMerkleProof(std::vector<uint8_t>, eTrieID trie);
	bool getReceiptForVerifiable(std::vector<uint8_t> proofID, eVerifiableType::eVerifiableType verType, CReceipt& rec, bool pruneDBAfter);
	bool getReceiptForTransaction(std::vector<uint8_t> transactionID, CReceipt& rec, bool pruneDBAfter = false);
	bool isAvailableLocally();
	void markLocalAvailability(bool available = true);
	ColdStorageProperties properties;
	enum eBlockHeaderInstantiationResult {
		OK,//used when creating new header 
		OKRootsFromSS,//used when deserializing a BER-packed header
		OKVriginRoots,//used when deserializing a BER-packed header
		parentMissing,
		stateDBFailure,
		transactionsDBFailure,
		receiptsDBFailure,
		failure,
		couldNotPopulateTries,
		invalidBERData
	};

	std::vector<uint8_t> getMinersID();
	bool forceRootNodesStorage();
	void setPubSig(std::vector<uint8_t> pub, std::vector<uint8_t> sig = std::vector<uint8_t>());
	void setPubKey(std::vector<uint8_t> pub);
	bool setBlock(std::shared_ptr<CBlock> block);
	eBlockInfoRetrievalResult::eBlockInfoRetrievalResult update();
	~CBlockHeader();
	size_t getCoreVersion();
	bool setCoreVersion(size_t version);
	void imposeCoreMarking();
	CBlockHeader(eBlockchainMode::eBlockchainMode blockchainMode, bool isKeyBlock);
	std::vector<CVerifiable*> getBeneficiaries();
	CBlockHeader(const CBlockHeader&);
	void setPackedTarget(uint32_t target);
	CTrieDB* getTransactionsDB();
	CTrieDB* getReceiptsDB();
	CTrieDB* getVerifiablesDB();
	static std::shared_ptr<CBlockHeader> newBlockHeader(std::shared_ptr<CBlock>block, eBlockchainMode::eBlockchainMode blockchainMode, bool isKeyBlock, eBlockHeaderInstantiationResult& result);
	static std::shared_ptr<CBlockHeader> instantiate(const std::vector<uint8_t>& headerBER, const eBlockHeaderInstantiationResult& result = eBlockHeaderInstantiationResult::OK, const std::string& errorInfo = "", bool loadRootsFromSS = true, eBlockchainMode::eBlockchainMode blockchainMode = eBlockchainMode::eBlockchainMode::TestNet);
	bool getPackedData(std::vector<uint8_t>& data, bool includeSig = true);
	void getPubSig(std::vector<uint8_t>& sig, std::vector<uint8_t>& pub);
	std::vector<uint8_t> getPubKey();
	bool addTransaction(CTransaction trans);
	bool addReceipt(CReceipt receipt);
	bool addVerifiable(CVerifiable verifiable);
	bool getTransaction(std::vector<uint8_t> transactionID, CTransaction& transaction);
	bool getReceipt(std::vector<uint8_t> receiptID, CReceipt& receipt);
	bool areTriesLoaded(bool verify = false);
	void setTriesLoaded(bool mark = true);
	bool loadTries();
	bool loadTries(std::vector<uint8_t> BEREncodedBlock);
	bool getVerifiable(std::vector<uint8_t> verifiableID, CVerifiable& verifiable);
	bool solvedNow();
	size_t getVersion();
	size_t getHeight();
	void  setHeight(size_t height);
	void setKeyHeight(size_t height);
	size_t getKeyHeight();
	size_t getNonce();
	void setNonce(size_t nonce);
	double getDifficulty();
	void freeTries();
	size_t getSolvedAtTime();
	void setSolvedAtTime(size_t);
	std::shared_ptr<CBlock> getParent(eBlockInstantiationResult::eBlockInstantiationResult& result, bool useColdStorage = true, bool instantiateTries = true, bool muteConsole = false, std::shared_ptr<CBlockchainManager> bm = nullptr);
	std::shared_ptr<CBlock> getParentPtr();
	std::shared_ptr<CBlock> getParentPtrK();
	void freeParent();

	std::vector<uint8_t> getHash(bool allowCached = false);

	std::vector<uint8_t> getUncleBlocksHash();

	bool setTrieRoot(CTrieNode* stateRoot, eTrieID id);

/// <summary>
/// Sets a Perspective for the specified trie. For state trie, can accept either Trie or Block Perspective
/// based on whether parentBlockHash is provided. 
/// 
/// 	[ IMPORTANT ]: starting from Core 1.6.5 the perspective provided for State Trie ( eiher initial or final)
///				   MUST be the Block Perspective. (POSTPONED).
/// 
/// For other tries, always expects Trie Perspective.
/// </summary>
/// <param name="id">The 32-byte Perspective to set</param>
/// <param name="trieid">Which trie to set the perspective for</param>
/// <param name="isFinal">For state trie, whether this is final or initial state</param>
/// <param name="parentBlockHash">Optional parent block hash. If provided for state trie, id is treated as Block Perspective</param>
/// <returns>True if perspective was set successfully, false otherwise</returns>
	bool setPerspective(std::vector<uint8_t> stateRootID, eTrieID id, bool isFinal=true, const std::vector<uint8_t>& parentBlockHash = std::vector<uint8_t>());



/// <summary>
/// Gets the block's Perspective Identifier of a given Trie, optionally combining with parent block hash.
/// When parent block hash is provided, returns Block Perspective (Trie Perspective + parent influence).
/// When no parent hash is provided, returns pure Trie Perspective.
/// The operation combining parent hash with Trie Perspective is reversible.
/// </summary>
/// <param name="trie">state/transactions/receipts/verifiables Trie</param>
/// <param name="getFinal">If true, returns final state perspective for state trie</param>
/// <param name="parentBlockHash">Optional parent block hash to get Block Perspective</param>
/// <returns>32-byte perspective hash</returns>
	std::vector<uint8_t> getPerspective(eTrieID trie, bool getFinal = true, const std::vector<uint8_t>& parentBlockHash= std::vector<uint8_t>());

	std::vector<uint8_t> getLogsBloom();
	bool setLogsBloom(std::vector<uint8_t> bloom);

	std::vector<uint8_t> getExtraData();
	bool setExtraData(std::vector<uint8_t> data);

	BigInt getErgLimit();
	bool setErgLimit(BigInt ergLimit);

	BigInt getErgUsed();
	bool setErgUsed(BigInt ergUsed);
	void setTotalBlockReward(BigInt reward, bool updateOnlyEffective = false);

	void setPaidToMiner(BigInt GNCPaid, bool updateOnlyEffective = false);


	/*
	  Epoch 1 + Epoch 2 Reward (including taxes and excluding any custom Verifiabels such as Proof of Fraud).
	  see getPaidToMiner() for an amount which was actually credited in the end to Operator.
	  By default, an Effective value is returned (one not necessarily present within serialzied block).
	  Rationale: the value could have been overriden by a Hard Fork.
	*/
	BigInt getTotalBlockReward(bool getEffectiveValue=true);


	/*
	  Epoch 1 + Epoch 2 Reward (EXCLUDING taxes and INCLUDING any custom Verifiabels such as Proof of Fraud).
	  see getPaidToMiner() for an amount which was actually credited in the end to Operator.
	  By default, an Effective value is returned (one not necessarily present within serialzied block).
	  Rationale: the value could have been overriden by a Hard Fork.
	*/
	BigInt getPaidToMiner(bool getEffectiveValue = true);


	std::vector<uint8_t> getParentID();
	bool setParentHash(std::vector<uint8_t> blockID);

	void setParent(std::shared_ptr<CBlock> parent, bool onlyHash = false);
	void setParentPtr(std::shared_ptr<CBlock> parent);
	uint64_t getNrOfParentsInMem();
	size_t getNrOfTransactions();
	size_t getNrOfVerifiables();
	size_t getNrOfReceipts();
	void   setNrOfTransactions(size_t nr);
	void   setNrOfVerifiables(size_t nr);
	void   setNrOfReceipts(size_t nr);

	/// <summary>
	/// The amount is computed dynamically based on Miner Reward Verifiable field found in block.
	/// IMPORTANT: Tries need to be unfolfed for this to work.
	///			   mTotalRewards field in block's header needs to match this value anyway.
	/// </summary>
	/// <param name="reward"></param>
	/// <returns></returns>
	eBlockInfoRetrievalResult::eBlockInfoRetrievalResult getRewardInMiningVerifiable(BigInt& reward);

};

#endif
