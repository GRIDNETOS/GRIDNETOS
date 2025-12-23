#include "Block.h"
#include "BlockHeader.h"
#include "BlockchainManager.h"
#include "BlockVerificationResult.h"
#include "Receipt.h"
#include "Verifiable.h"
#include <mutex>
#include "BlockDesc.hpp"
//std::mutex CBlock::mGeneratorLock;
/// <summary>
/// Can be used to instantiate a block from its BER-serialized counterpart.
/// </summary>
/// <param name="blockBER"></param>
/// <param name="result"></param>
/// <param name="errorInfo"></param>
/// <param name="useTestDB"></param>
/// <returns></returns>
std::shared_ptr<CBlock> CBlock::instantiateBlock(bool pupulateTries,const std::vector<uint8_t> & blockBER, eBlockInstantiationResult::eBlockInstantiationResult &result, std::string &errorInfo, eBlockchainMode::eBlockchainMode blockchainMode)
{

	Botan::BER_Object obj;
	eBlockInstantiationResult::eBlockInstantiationResult bResult;
	try {
		std::shared_ptr<CBlock> block = CBlock::newBlock(nullptr, bResult, blockchainMode, true);//true for key block is a stub here will be filled below based on data
		if (block)
		{
			block->setSize(blockBER.size());
		}
		CBlockHeader::eBlockHeaderInstantiationResult resultHeader;
		std::vector<uint8_t> headerData;
		std::string mErrorInfo;
		std::vector<uint8_t> tempID;

		bool isKey = false;
		Botan::BER_Decoder dec1 = Botan::BER_Decoder(blockBER);
		Botan::BER_Object ob= dec1.get_next_object();

		if (ob.class_tag != Botan::ASN1_Tag::PRIVATE)
			return nullptr;
		if (ob.type_tag == eBERObjectType::eBERObjectType::keyBlock)
			isKey = true;
		else if (ob.type_tag == eBERObjectType::eBERObjectType::regularBlock)
			isKey = false;
		else
			return nullptr;//unknown type

		Botan::BER_Decoder dec2 = Botan::BER_Decoder(ob.value);
		dec2.decode(block->mVersion);
		std::vector<uint8_t> claimedTransactionRootID, claimedReceiptsRootID, claimedVerifiablesRootID;
		switch (block->mVersion)
		{
		case 1:
			obj = dec2.get_next_object();
			if (obj.type_tag != Botan::ASN1_Tag::OCTET_STRING)
			{
				result = eBlockInstantiationResult::eBlockInstantiationResult::Failure;
				errorInfo = mErrorInfo;
				return nullptr;
			}
			headerData = Botan::unlock(obj.value);
			std::shared_ptr<CBlockHeader> header = nullptr;

			header = CBlockHeader::instantiate(headerData, resultHeader, mErrorInfo, true, blockchainMode);
			if (header == nullptr)
			{
				result = eBlockInstantiationResult::eBlockInstantiationResult::headerInstantationFailure;
				errorInfo = mErrorInfo;
				return nullptr;

			}
			block->setBlockHeader(header);
		
			
				if (header != nullptr && !header->isKeyBlock() && resultHeader == CBlockHeader::eBlockHeaderInstantiationResult::OKRootsFromSS ||
					resultHeader == CBlockHeader::eBlockHeaderInstantiationResult::OKVriginRoots)
				{
					tempID = CCryptoFactory::getInstance()->getSHA2_256Vec(headerData);

					block->getHeader()->setBlock(block);
					result = eBlockInstantiationResult::eBlockInstantiationResult::OK;
					if (pupulateTries)
					{
						if (!block->getHeader()->loadTries(blockBER))
						{
							resultHeader == CBlockHeader::eBlockHeaderInstantiationResult::couldNotPopulateTries;
							result = eBlockInstantiationResult::eBlockInstantiationResult::Failure;
						}

						block->mTransactionIDs.clear();
						std::vector <std::vector<uint8_t>> transactionIDs = header->getTransactionsDB()->getKnownNodeIDs();
						for (int i = 0; i < transactionIDs.size(); i++)
						{
							block->mTransactionIDs.push_back(transactionIDs[i]);
						}

						block->mVerifiablesIDs.clear();
						std::vector <std::vector<uint8_t>> verifiablesIDs = header->getVerifiablesDB()->getKnownNodeIDs();
						for (int i = 0; i < verifiablesIDs.size(); i++)
						{
							block->mVerifiablesIDs.push_back(verifiablesIDs[i]);
						}

						block->mReceiptsIDs.clear();
						std::vector <std::vector<uint8_t>> receiptsIDs = header->getReceiptsDB()->getKnownNodeIDs();
						for (int i = 0; i < receiptsIDs.size(); i++)
						{
							block->mReceiptsIDs.push_back(receiptsIDs[i]);
							//todo:remove the below
							//CReceipt rec;
							//if (!block->getReceipt(receiptsIDs[i], rec))
							// assertGN(false);
						}

					}
				}
				else
				{
					result = eBlockInstantiationResult::eBlockInstantiationResult::Failure;
					errorInfo = mErrorInfo;
					return nullptr;
				}
			

		}//end switch version

		block->mTemporalID = tempID;
		return block;
	}
	catch (...)
	{
		bResult = eBlockInstantiationResult::eBlockInstantiationResult::Failure;
		return nullptr;
	}
}

/// <summary>
/// attempts to instantiate a block. returns nullptr on error.
//the block might not get successfully instatnitated due to missing data etc.
/// </summary>
std::shared_ptr<CBlock> CBlock::newBlock(std::shared_ptr<CBlock> parent, eBlockInstantiationResult::eBlockInstantiationResult & result,
	eBlockchainMode::eBlockchainMode blockchainMode, 
	bool keyBlock, bool miningNewBlock)
{
	//std::lock_guard<std::mutex> lock(mGeneratorLock);
	std::shared_ptr<CBlock>block = std::make_shared<CBlock>();
	
	switch (blockchainMode)
	{
	case eBlockchainMode::LIVE:
		block->isTestNet = false;
		break;
	case eBlockchainMode::TestNet:
		block->isTestNet = true;
		break;
	case eBlockchainMode::LIVESandBox:
		block->isTestNet = false;
		break;
	case eBlockchainMode::TestNetSandBox:
		block->isTestNet = true;
		break;
	case eBlockchainMode::LocalData:
		block->isTestNet = true;
		break;
	default:
		break;
	}
	block->mBlockchainMode = blockchainMode;
	block->mVersion = 1;
	block->mIsPrepared = false;
	block->mBlockHeader = nullptr;
	block->mNext = nullptr;
	block->mCf = CCryptoFactory::getInstance();
	CBlockHeader::eBlockHeaderInstantiationResult headerResult;
	
	block->mBlockHeader =  CBlockHeader::newBlockHeader(block, blockchainMode, keyBlock, headerResult);
	if (miningNewBlock)
	{
		block->mBlockHeader->imposeCoreMarking();
	}
	if (block->mBlockHeader == nullptr)
	{
		result = eBlockInstantiationResult::eBlockInstantiationResult::Failure;
		return nullptr;
	}
	//set state TrieRoot
	if (parent != nullptr && parent->getHeader() != nullptr) {

		block->getHeader()->setHeight(parent->getHeader()->getHeight() + 1);
		block->mBlockHeader->setParent(parent,true);
		block->mBlockHeader->setPerspective(parent->mBlockHeader->getPerspective(eTrieID::state), eTrieID::state);// notice: this perspective in parent block MIGHT not be effective after hard forks;
		//thus on bondary of hard forks we need to account for effective parspective during flow process.
	}
	if (parent != nullptr)
	{    //NOTE: do *NOT* do this. the parent might NOT be willing to be modified at all.
		//this might lead to modification of the leader block for instance if direct pointer
		//to it was requested. and a new block is formed based on it.

		//parent->setNext(block);
		
	}
	result = eBlockInstantiationResult::eBlockInstantiationResult::OK;
	return block;
}

///The function removes the circular relationship between the Block and its header
//Warning: the Block should NOT be accessed after this is done.
void CBlock::prepareForRemoval()
{
	if (getHeader() != nullptr)
	{
		getHeader()->setParent(nullptr);
	
		getHeader()->setBlock(nullptr);
	}
	mPartOfMainCache = false;
	setNext(nullptr);
	//the header itself will be removed within the Block's destructor

}
bool CBlock::setBlockHeader(std::shared_ptr<CBlockHeader> header)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	
	 if (header != nullptr)
	 {
		 mBlockHeader = header;
	 }

	return true;
}

std::vector<uint8_t> CBlock::getID(bool refresh)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	 std::lock_guard<std::mutex> lock2(mFieldsGuardian);
	

	 if (refresh || mID.empty())
	 {
		 if (mBlockHeader != nullptr)
			 if (mBlockHeader->getPackedData(mID))
			 {
				 mID =  mCf->getSHA2_256Vec(mID);
			 }
	 }
	 return mID;


}

uint32_t CBlock::getTransactionsCount()
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mTransactionIDs.size();
}

uint32_t CBlock::getVerifiablesCount()
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mVerifiablesIDs.size();
}

bool CBlock::prepare()
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);

    mIsPrepared = true;
	return true;
}

/// <summary>
/// Sorts transactions based on their identifiers.
/// </summary>
void CBlock::sortTransactions()
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::sort(mTransactionIDs.begin(), mTransactionIDs.end(), [](std::vector<uint8_t> lhs, std::vector<uint8_t> rhs) {
		
		return Botan::BigInt(lhs.data(), lhs.size()) > Botan::BigInt(rhs.data(), rhs.size());
	});
}



bool CBlock::getVerifiable(std::vector<uint8_t> id, CVerifiable & verifiable)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//std::vector<uint8_t> rootHash = this->mBlockHeader->getPerspective(verifiables);
	return this->mBlockHeader->getVerifiable(id, verifiable);
}

/// <summary>
/// IMPORTANT: never call this method alone. Call clearBlockCache() instead. Additional adjacent cachces need to be accounted for as well.
/// Blocks cannot contain hash of next block in its formal definition, as its hash would need to depend on it.
/// Thus, a temporary pointer to next block is maintained. Note there might be multiple offsprings.
/// Only one forked structure is maintained through pointer for simplicity. Other possible blocks are contained within TrieDB.
/// </summary>
/// <returns></returns>
std::shared_ptr<CBlock> CBlock::getNext(bool getTemp)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	 if(!getTemp)
    return mNext;
	 else return mTempNext;
}

void CBlock::freePath (bool tempPathOnly, std::shared_ptr<CTools> tools)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	
	if (tools)
	{
		tools->logEvent("Cleaning up blockchain cache..");
	}
	

	if (getHeader() == nullptr)
	{
		if (tools)
		{
			tools->logEvent("WARNING: invalid block with NULL header encountered; aborting the CleanUp procedure");
		}
		return;
	}
	CTools::writeLineS("Cleaning up the parent path for block" + CTools::getTools()->base58CheckEncode(getHeader()->getHash()));
	std::shared_ptr<CBlock> currentBlock = getHeader()->getParentPtr();
	std::shared_ptr<CBlock>next = nullptr;

	size_t alreadyTraversed = 0;
	size_t freed = 0;
	bool cut = false;
	eBlockInstantiationResult::eBlockInstantiationResult res;
	while (currentBlock != nullptr)
	{
		if (currentBlock->getHeader() == nullptr)
		{
			if (tools)
			{
				tools->logEvent("WARNING: invalid block with NULL header encountered; aborting the CleanUp procedure");
			}
		
			return;
		}
		next = currentBlock->getHeader()->getParentPtr();

		if (!tempPathOnly)
		{
			currentBlock->prepareForRemoval();
			currentBlock = next;
		}
		if (mTempNext != nullptr)
			mTempNext->prepareForRemoval();
		
		alreadyTraversed++;
	}
	if (tools)
	{
		tools->logEvent("Freed " + std::to_string(freed) + " blocks");
	}
	
}
/// <summary>
/// Returns just a pointer to parent IF available.
/// </summary>
/// <returns></returns>
std::shared_ptr<CBlock> CBlock::getParentPtr()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mBlockHeader != nullptr)
	{
		return mBlockHeader->getParentPtr();
	}
	 return nullptr;
}
/// <summary>
/// Extreme caution:
/// - assumption that block header exists
/// - assumption that block won't be mdified
/// USAGE: ONLY when entire chain locked.
/// </summary>
/// <returns></returns>
std::shared_ptr<CBlock> CBlock::getParentPtrK()
{
		return mBlockHeader->getParentPtrK();
}

bool  CBlock::setNext(std::shared_ptr<CBlock> nxt,  bool isTemp)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	 if (!isTemp)
		 mNext = nxt;
	 else
		 mTempNext = nxt;
	return true;
}



///Gets a total amount of work performed on the chain represented by this Block.
//Blocks might be fetched either from the Live block cache OR from the ChainProof cache OR they might be fetched directly from
//Cold Storage (slowest) IF not available within one of the HotStorage(RAM) cache data stores.
//
//Because blocks might be fetched directly from the Live Block Cache, and because a list-data structure is formed
//during the traversal;  a temporary Next pointer is used to store the consecutive Next-Blocks;
// a call to freePath(true) is made on the top-most Block (this) to free the temporary analyzed sub-chain.
//only next-pointers need to be cleared since the parent-pointer is NOT set during the traversal.
unsigned long long CBlock::getTotalWorkDone(CBlockVerificationResult &result)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	unsigned long long totalDiff = 0;
	getTotaWorkDoneInt(result, totalDiff);
	//freePath(true);
	return totalDiff;
}

void CBlock::setReceivedAt(size_t time)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mRecaivedAt = time;
}

void CBlock::setReceivedNow()
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	 mRecaivedAt = std::time(0);
}

size_t CBlock::getReceivedAt()
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mRecaivedAt;
}

bool CBlock::setReceivedFrom(CNetworkNode * node, bool now)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (node == nullptr)
		return false;
	mReceivedFrom = node;
	if (now)
		setReceivedAt(std::time(0));

	mReceivedFrom = node;
	return true;
}


//used to provide a pre-computed hash of block's header directly from its body
std::vector<uint8_t> CBlock::getHotStorageID()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> temp;
	if (mTemporalID.size() == 32)
		return mTemporalID;

	if (mBlockHeader != nullptr)
	{
		if(mBlockHeader->getPackedData(temp))
		mTemporalID = mCf->getSHA2_256Vec(temp);
	}
	return mTemporalID;
}

void CBlock::setMainCacheMember(bool is)
{
	mPartOfMainCache = is;
}

bool CBlock::isMainCacheMember()
{
	return mPartOfMainCache;
}

bool CBlock::getScheduledByLocalScheduler()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mScheduledByLocalScheduler;
}

void CBlock::setScheduledByLocalScheduler(bool wasIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	mScheduledByLocalScheduler = wasIt;
}

bool CBlock::getDoTurboFlow()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mDoTurboFlow;
}

void CBlock::setDoTurboFlow(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mDoTurboFlow = doIt;
}

void CBlock::setIsCheckpointed(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mIsCheckpointed = isIt;
}

bool CBlock::getIsCheckpointed()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mIsCheckpointed;
}

bool CBlock::getIsLeadingAHardFork()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mBlockLeadingAHardFork;
}

void CBlock::setIsLeadingAHardFork(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mBlockLeadingAHardFork = isIt;
}

uint64_t CBlock::getSize()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mSize;
}
void CBlock::setSize(uint64_t size)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	 mSize = size;
}


/// <summary>
/// Returns the total, cumulative amount of Proof-of-Work performed on the chain to which the block
/// belongs and with which the chain ends.
/// The function is compatible with the block-caching mechanism i.e. it takes use of the fact that a certaina amount
/// of blocks is cached on the main-chain and sets appropriate temporary fields within blocks.
/// NOTE: The function DOES NOT rebuild the cache. Blocks might be fetched from Cold Storage but permanent pointers
/// among blocks WOULD NOT be set. That's the aim of BM's prepareCache().
/// </summary>
/// <param name="result"></param>
/// <param name="totalDiff"></param>
void  CBlock::getTotaWorkDoneInt(CBlockVerificationResult & result, unsigned long long &totalDiff)
{
	//LOCAL VARIABLES - BEGIN
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	eBlockInstantiationResult::eBlockInstantiationResult res;
	std::shared_ptr<CBlockHeader> currentHeader = getHeader();
	std::shared_ptr<CBlock> current = shared_from_this();// getHeader()->getParent(res, true, false);//this is NOT the current node(This); rather its the 'current parent'
	std::shared_ptr<CBlock> previous;// the 'Next' Block; - an ofspring from the currentParent; but a 'previous' during parsing - we parse from This towards the Genesis block
	std::shared_ptr<CBlock> first;
	//LOCAL VARIABLES - END

	if (mBlockHeader==nullptr)
		return;
	
	//let us check if I know my own total difficulty if so, assume it as the intemediaryDiff and return

	if (getHeader()->getTotalDiffField() > 0)//uses the fact that if at lest one block knows the value that it must had taken into account
	{//all the previous values as well.
		  totalDiff = currentHeader->getTotalDiffField();
		  result = CBlockVerificationResult(
			  eBlockVerificationResult::eBlockVerificationResult::valid,
			  mCf->getSHA2_256Vec(getID()), getHeader()->getParentID());
		  return;
	}
	
	//find the earliest previous block which does not know its total difficulty
	while (current != nullptr && currentHeader!=nullptr)
	{
		if (previous != nullptr)
		{
			//make sure the TEMPORARY pointer to the nextBlock is valid' we'll use it while going back
			//we CANNOT override the true current next pointer as the block might have been fetched from the Live Chain Cache!
			current->setNext(previous,true);
		}

		if (currentHeader->getTotalDiffField() != 0)
			break;

		previous = current;
		current = current->getParent(res, true, false);
		if (current != nullptr)
			currentHeader = current->getHeader();
		else
			currentHeader = nullptr;
		
	}
	 first = current;

	if (current == nullptr && previous == nullptr)
	{
		this->getHeader()->setTotalDiffField(getHeader()->getDifficulty());

		if (!isGenesis())
			result = CBlockVerificationResult(
				eBlockVerificationResult::eBlockVerificationResult::unknownBlockOnPath, getID());
		else
			result = CBlockVerificationResult(
				eBlockVerificationResult::eBlockVerificationResult::valid, getID());

		totalDiff = this->getHeader()->getTotalDiffField();
		return;
	}

	if (current == nullptr && ((previous!=nullptr && previous->isGenesis()==false) ))
	{
		result = CBlockVerificationResult(
			eBlockVerificationResult::eBlockVerificationResult::unknownBlockOnPath,
			getID());
		return;
	}

	if (current == nullptr)
	{
		current = previous;
		currentHeader = current->getHeader();
	}

	if (currentHeader->getTotalDiffField() == 0 && current->isGenesis())
		currentHeader->setTotalDiffField(current->getHeader()->getDifficulty());

	//update total diff by going all the way back. Now in the direction from the Gensis to This.
	
	do
	{
		if (current->getNext(true) != nullptr)
			current->getNext(true)->getHeader()->setTotalDiffField(current->getHeader()->getTotalDiffField() + current->getNext(true)->getHeader()->getDifficulty());
		previous = current;
		
		current = current->getNext(true);
		
		previous->setNext(nullptr, true);//clear the dependancy
		if (!previous->isMainCacheMember())
			previous->prepareForRemoval();

	} while (current != nullptr);


	totalDiff = this->getHeader()->getTotalDiffField();
	result = CBlockVerificationResult(
		eBlockVerificationResult::eBlockVerificationResult::valid,
		getID());
	return;
}

std::shared_ptr<CBlock> CBlock::getParent(eBlockInstantiationResult::eBlockInstantiationResult& res, bool useColdStorage , bool instantiateTries,bool muteConsole, std::shared_ptr<CBlockchainManager> bm)
{

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mBlockHeader != nullptr)
	{
	return mBlockHeader->getParent(res, useColdStorage, instantiateTries,muteConsole,bm);
	}
	else
	{
		res = eBlockInstantiationResult::eBlockInstantiationResult::Failure;
		return nullptr;
	}
	return nullptr;
}

/// <summary>
/// the function is to traverse the entire database in search for offsprings.
/// Warning: Slow. if explicit database search is selected the process would be EXTREMELY slow.
/// </summary>
/// <returns></returns>
std::shared_ptr<CBlock> CBlock::getAllPossibleOffsprings(bool doExplicitColdStorageSearch)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return nullptr;
}


bool CBlock::addTransaction(CTransaction transaction)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	 if (mBlockHeader == nullptr || mBlockHeader->isKeyBlock())
		 return false;
	if (!transaction.isValid())
		transaction.prepare();
	if (!transaction.isValid())
		return false;

	if (mBlockHeader == nullptr)
		return false;
	this->mTransactionIDs.push_back(transaction.getHash());
	bool result = this->mBlockHeader->addTransaction(transaction);

    this->mIsPrepared = false;
	return result;
}
bool  CBlock::addVerifiable(CVerifiable verifiable)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mBlockHeader == nullptr)
		return false;
	if (mBlockHeader == nullptr)
		return false;
	std::vector<uint8_t> h = verifiable.getHash();
	this->mVerifiablesIDs.push_back(h);
	if (!this->mBlockHeader->addVerifiable(verifiable))
	{
		return false;
	}

	this->mIsPrepared = false;
	return true;
}

bool CBlock::addReceipt(CReceipt receipt)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	 if (mBlockHeader == nullptr )
		 return false;

	 if (mBlockHeader == nullptr)
		 return false;
	this->mReceiptsIDs.push_back(receipt.getHash());
	if (!this->mBlockHeader->addReceipt(receipt))
		return false;

	this->mIsPrepared = false;
	return true;
}


/// <summary>
/// The function iterates through the Transaction Trie looking for a transaction with a given ID.
/// The Transaction Trie is reconstructed when unboxing packed Block using sorted list of transactions.
/// This might also be considered as an additional integrity validation check.
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
bool   CBlock::getTransaction(std::vector<uint8_t> id, CTransaction & transaction)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//std::vector<uint8_t> rootHash = this->mBlockHeader->getPerspective(transactions);
	return this->mBlockHeader->getTransaction(id, transaction);
}

bool CBlock::getReceipt(std::vector<uint8_t> id, CReceipt& receipt)
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//std::vector<uint8_t> rootHash = this->mBlockHeader->getPerspective(receipts);
	 if (this->mBlockHeader == nullptr)
	 {
		 return false;
	 }
	return this->mBlockHeader->getReceipt(id,receipt);
}

std::vector<CReceipt> CBlock::getReceipts()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<CReceipt> receipts;
	for (int i = 0; i < mReceiptsIDs.size(); i++)
	{
		CReceipt rec(mBlockchainMode);//todo:verify if mBlockchainMode field is properly initialized
		if (getReceipt(mReceiptsIDs[i], rec))
			receipts.push_back(rec);
	}
	return receipts;
}

std::vector<CTransaction> CBlock::getTransactions()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<CTransaction> transactions;
	for (int i = 0; i < mTransactionIDs.size(); i++)
	{
		CTransaction t;
		if (getTransaction(mTransactionIDs[i], t))
			transactions.push_back(t);
	}
	return transactions;
}

std::shared_ptr<CBlockDesc> CBlock::getDescription() const
{
	std::shared_lock lock(mSharedGuardian);
	return mDescription;
}

bool CBlock::setDescription(std::shared_ptr<CBlockDesc> description)
{
	if (!description) {
		return false;
	}
	std::unique_lock lock(mSharedGuardian);
	mDescription = description;
	return true;
}

bool CBlock::hasDescription() const
{
	std::shared_lock lock(mSharedGuardian);
	return mDescription != nullptr;
}

void CBlock::clearDescription()
{
	std::unique_lock lock(mSharedGuardian);
	mDescription.reset();
}

std::vector<CVerifiable> CBlock::getVerifiables()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
		std::vector<CVerifiable> transactions;
		for (int i = 0; i < mVerifiablesIDs.size(); i++)
		{
			CVerifiable ver;
			if (getVerifiable(mVerifiablesIDs[i], ver))
				transactions.push_back(ver);
		}
		return transactions;
}

CBlock::~CBlock()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mBlockHeader != nullptr)
	{
		//CTools::writeLineS("Self-destructing..","Block at height "+ std::to_string(mBlockHeader->getHeight()));
		mBlockHeader = nullptr;
	}

 
}

bool CBlock::validate()
{
	if (mBlockHeader == nullptr)
		return false;

	return true;
}

void CBlock::setHeader(std::shared_ptr<CBlockHeader> header)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mBlockHeader = header;
}

CBlock::CBlock()
{
	mSize = 0;
	mProducedLocally = false;
	mBlockLeadingAHardFork = false;
	mIsCheckpointed = false;
	mScheduledByLocalScheduler = false;
	mDoTurboFlow = false;
	mPartOfMainCache = true;
	isTestNet  =false;
	mNext = nullptr;
	mCf = CCryptoFactory::getInstance();
	mVersion = 1;
	mBlockHeader = nullptr;
	mRecaivedAt = 0;
	mReceivedFrom = nullptr;
}

CBlock::CBlock(const CBlock& other)
{
	this->mSize = other.mSize;
	this->mProducedLocally = other.mProducedLocally;
	this->mBlockLeadingAHardFork = other.mBlockLeadingAHardFork;
	this->mIsCheckpointed = other.mIsCheckpointed;
	this->mScheduledByLocalScheduler = other.mScheduledByLocalScheduler;
	this->mDoTurboFlow = other.mDoTurboFlow;
	this->mTemporalID = other.mTemporalID;
	this->mRecaivedAt = other.mRecaivedAt;
    this->mVersion = other.mVersion;
    this->mIsPrepared = other.mIsPrepared;
    this->mBlockHeader = std::make_shared<CBlockHeader>(*other.mBlockHeader);
    this->mTransactionIDs = other.mTransactionIDs;
    this->mReceiptsIDs = other.mReceiptsIDs;
	this->mVerifiablesIDs = other.mVerifiablesIDs;
	this->mReceivedFrom = other.mReceivedFrom;
    this->mNext = nullptr;
	this->isTestNet = other.isTestNet;
	this->mCf = other.mCf;
	this->mDescription = other.mDescription;
	
	//this->mTools = other.mTools;

}

void CBlock::setProducedLocally(bool wasIt)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mProducedLocally = wasIt;
}


bool CBlock::getProducedLocally()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mProducedLocally;
}

/// <summary>
/// Encodes Block as well as Block Header into one data bundle.
/// </summary>
/// <returns></returns>
bool  CBlock::getPackedData(std::vector<uint8_t> &BERData, bool includeSig)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	bool errorEncountered = false;
	CReceipt rec(mBlockchainMode);//verify value
	CTransaction trans;
	if (mBlockHeader == nullptr)
		return false;//header is required

	if (!mBlockHeader->validate())
		return false;//block header needs to be valid
	bool isKey = mBlockHeader->isKeyBlock();
	std::vector<uint8_t> headerPackedData;

	Botan::DER_Encoder enc1 = Botan::DER_Encoder().start_cons(static_cast<Botan::ASN1_Tag>(isKey?eBERObjectType::eBERObjectType::keyBlock: eBERObjectType::eBERObjectType::regularBlock), Botan::ASN1_Tag::PRIVATE)
        .encode(mVersion);

    if (mBlockHeader != nullptr)
	{
		 if(! mBlockHeader->getPackedData(headerPackedData,includeSig))
			return false;

		enc1 = enc1.encode(headerPackedData, Botan::ASN1_Tag::OCTET_STRING);
	}
	else
		enc1 = enc1.encode_null();

	//if (!mBlockHeader->isKeyBlock())
	//{
		//if it's NOT a key block-> we can supply some more data such as the Transactions, Verifiables etc..
		if (!mBlockHeader->isKeyBlock())
		{
			enc1 = enc1.start_cons(Botan::ASN1_Tag::SEQUENCE);
			//no transactions within a key-block
			for (int i = 0; i < mTransactionIDs.size(); i++)
			{
				if (mTransactionIDs[i].size() != 32)
					continue;
				
				if (!getTransaction(mTransactionIDs[i], trans))
					return false;
				enc1 = enc1.encode(trans.getPackedData(), Botan::ASN1_Tag::OCTET_STRING);
			}
			enc1 = enc1.end_cons();
		}

		enc1=enc1.start_cons(Botan::ASN1_Tag::SEQUENCE);

		for (int i = 0; i < mReceiptsIDs.size(); i++)
		{
			if (mReceiptsIDs[i].size() != 32)
				return false;
			
			if (getReceipt(mReceiptsIDs[i], rec) == false)
			{
				return false;
			}
			enc1 = enc1.encode(rec.getPackedData(), Botan::ASN1_Tag::OCTET_STRING);
		}

		enc1 = enc1.end_cons().start_cons(Botan::ASN1_Tag::SEQUENCE);

		for (int i = 0; i < mVerifiablesIDs.size(); i++)
		{
			if (mVerifiablesIDs[i].size() != 32)
				return false;
			CVerifiable ver;
			if (!getVerifiable(mVerifiablesIDs[i], ver))
				return false;

			enc1 = enc1.encode(ver.getPackedData(), Botan::ASN1_Tag::OCTET_STRING);
		}

		enc1 = enc1.end_cons();

	//}
	enc1 = enc1.end_cons();
	BERData= enc1.get_contents_unlocked();
	return true;

}

std::shared_ptr<CBlockHeader> CBlock::getHeader()
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mBlockHeader;
}

/// <summary>
/// Returns true ONLY for the first *KEY-BLOCK*.
/// The function won't return true for the 2nd data-block following the former, i.e. the block which contains the actual transactions.
/// </summary>
/// <returns></returns>
bool CBlock::isGenesis()
{	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	bool mToRet = false;
	 eBlockInstantiationResult::eBlockInstantiationResult res;

	if (this->getHeader() != nullptr  && this->getHeader()->isKeyBlock()&&this->getHeader()->getHeight()==0&& this->getHeader()->getParent(res) == nullptr)//TODO: verify hash!
		mToRet= true;
	else mToRet = false;

	return mToRet;
}

std::vector <std::vector<uint8_t>>  CBlock::getTransactionsIDs()
{
	 std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mTransactionIDs;
}

std::vector<std::vector<uint8_t>> CBlock::getVerifiablesIDs()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mVerifiablesIDs;
}

std::vector<std::vector<uint8_t>> CBlock::getReceiptsIDs()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mReceiptsIDs;
}
