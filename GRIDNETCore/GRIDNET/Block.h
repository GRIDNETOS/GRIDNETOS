#pragma once
#ifndef BLOCK_H
#define BLOCK_H
#pragma once
#include "stdafx.h"
#include "transaction.h"
#include "CryptoFactory.h"
#include "EEndPoint.h"
#include "NetworkNode.h"
#include "SolidStorage.h"
#include "transactionmanager.h"
#include "enums.h"

class CBlockDesc;
class CBlockHeader;
class CBlockVerificationResult;


class CBlock :public std::enable_shared_from_this<CBlock>
{
public: 
	void setHeader(std::shared_ptr<CBlockHeader> header);
	CBlock(); //use instantiateBlock() instead
	CBlock(const CBlock& other);

	void setProducedLocally(bool wasIt=true);

	bool getProducedLocally();
	
	~CBlock();
	bool validate();
	void prepareForRemoval();
	static std::shared_ptr<CBlock> instantiateBlock(bool instantiateTries,const std::vector<uint8_t> & blockBER, eBlockInstantiationResult::eBlockInstantiationResult &result,std::string &errorInfo,eBlockchainMode::eBlockchainMode blockchainMode);
	static std::shared_ptr<CBlock> newBlock(std::shared_ptr<CBlock> parent, eBlockInstantiationResult::eBlockInstantiationResult &result, eBlockchainMode::eBlockchainMode blockchainMode,bool keyBlock, bool miningNewBlock=false);
	bool getPackedData(std::vector<uint8_t> &BERData,bool includeSig=true);
	bool setBlockHeader(std::shared_ptr<CBlockHeader> header);
	std::vector<uint8_t> getID(bool refresh=false);
	uint32_t getTransactionsCount();
	uint32_t getVerifiablesCount();
	std::shared_ptr<CBlockHeader> getHeader();
	bool isGenesis();
	std::vector < std::vector<uint8_t>> getTransactionsIDs();
	std::vector < std::vector<uint8_t>> getVerifiablesIDs();
	std::vector < std::vector<uint8_t>> getReceiptsIDs();
	bool addTransaction(CTransaction transaction);
	bool addReceipt(CReceipt receipt);
	bool addVerifiable(CVerifiable verifiable);

	bool  getTransaction(std::vector<uint8_t> id,CTransaction&transaction);
	bool getReceipt(std::vector<uint8_t> id,CReceipt & receipt);
	std::vector<CReceipt> getReceipts();
	std::vector<CTransaction> getTransactions();

	// Meta Data - BEGIN
/**
	 * @brief Gets the block's metadata description.
	 *
	 * Thread-safe read-only access to the block's metadata using shared lock.
	 *
	 * @return std::shared_ptr<CBlockDesc> The block's metadata description.
	 *         May return nullptr if metadata hasn't been generated yet.
	 */
	std::shared_ptr<CBlockDesc> getDescription() const;

	/**
	 * @brief Sets the block's metadata description.
	 *
	 * Thread-safe modification of metadata using unique lock.
	 * If metadata already exists, it will be replaced.
	 *
	 * @param description The new metadata description to set.
	 * @return bool Returns true if the description was set successfully.
	 */
	bool setDescription(std::shared_ptr<CBlockDesc> description);

	/**
	 * @brief Checks if the block has metadata description.
	 *
	 * Thread-safe read-only check using shared lock.
	 *
	 * @return bool Returns true if metadata exists, false otherwise.
	 */
	bool hasDescription() const;

	/**
	 * @brief Clears the block's metadata description.
	 *
	 * Thread-safe modification using unique lock.
	 * Useful when metadata needs to be regenerated or during cleanup.
	 */
	void clearDescription();
	// Meta Data - END

	std::vector<CVerifiable> getVerifiables();

	bool getVerifiable(std::vector<uint8_t> id,CVerifiable &verifiable);
	//navigation
	std::shared_ptr<CBlock> getNext(bool getTemp=false);
	void freePath(bool tempPath=false, std::shared_ptr<CTools> tools = nullptr);
	std::shared_ptr<CBlock> getParentPtr();
	std::shared_ptr<CBlock> getParentPtrK();
	std::shared_ptr<CBlock> getParent(eBlockInstantiationResult::eBlockInstantiationResult& res,bool useColdStorage=true,bool instantiateTries=true,bool muteConsole=false, std::shared_ptr<CBlockchainManager> bm=nullptr);
	std::shared_ptr<CBlock> getAllPossibleOffsprings(bool doExplicitColdStorageSearch=false);
	bool setNext(std::shared_ptr<CBlock> nxt,bool isTemp=false);
	
	unsigned long long getTotalWorkDone(CBlockVerificationResult &result);
	void setReceivedAt(size_t time);
	void setReceivedNow();
	size_t getReceivedAt();

	bool setReceivedFrom(CNetworkNode *node, bool now = true);
	
	std::vector<uint8_t> getHotStorageID();
	void setMainCacheMember(bool is = true);
	bool isMainCacheMember();

	bool getScheduledByLocalScheduler();
	void setScheduledByLocalScheduler(bool wasIt = true);


	bool getDoTurboFlow();
	void setDoTurboFlow(bool doIt = true);


	void setIsCheckpointed(bool isIt = true);
	bool getIsCheckpointed();
	bool getIsLeadingAHardFork();
	void setIsLeadingAHardFork(bool isIt = true);
	uint64_t getSize();
	void setSize(uint64_t size);
private:
	// Meta Data Support - BEGIN
	mutable std::shared_mutex mSharedGuardian;
	std::shared_ptr<CBlockDesc> mDescription;
	// Meta Data Support - END
	std::vector<uint8_t> mID;
	uint64_t mSize;
	bool mBlockLeadingAHardFork;
	bool mIsCheckpointed;
	std::mutex mFieldsGuardian;
	bool mScheduledByLocalScheduler;
	bool mDoTurboFlow;
	bool mProducedLocally;
	bool mPartOfMainCache;
	static std::mutex mGeneratorLock;
	std::vector<uint8_t> mTemporalID;//not serialized
	std::recursive_mutex mGuardian;
	void getTotaWorkDoneInt(CBlockVerificationResult &result, unsigned long long &intemediaryDiff);
	bool isTestNet;
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	//std::vector<CTransaction*> retrievedTransactions;
	std::shared_ptr<CBlock> mNext;
	std::shared_ptr<CBlock> mTempNext;
	//CTools *mTools;
    std::shared_ptr<CCryptoFactory>  mCf;
	bool prepare();
    bool mIsPrepared;
    size_t mVersion;
    std::shared_ptr<CBlockHeader> mBlockHeader;
	void sortTransactions();
	std::vector <std::vector<uint8_t>> mTransactionIDs;//stores the transaction identifiers. used to retrieve transaction from TrieDB
	std::vector <std::vector<uint8_t>> mReceiptsIDs;//stores the receipts identifiers. used to retrieve receipts from TrieDB
	std::vector <std::vector<uint8_t>> mVerifiablesIDs;//stores the veriiables identifiers. used to retrieve veriiables from TrieDB
	size_t mRecaivedAt;
	CNetworkNode * mReceivedFrom;
	
};
class CompareBlocksByDiff
{
public:
	bool operator() (std::shared_ptr<CBlock> b1, std::shared_ptr<CBlock>b2)
	{
		return b1->getHeader()->getDifficulty() > b2->getHeader()->getDifficulty();
	}
};

#endif
