#include "StateDomain.h"
#include "TrieNode.h"
#include <cassert>
#include "keyChain.h"
#include "BlockchainManager.h"
#include <shared_mutex>
#include "CStateDomainManager.h"
#include "transactionmanager.h"
#include "TrieDB.h"
#include "AsyncMemCleaner.h"
#include "TokenPool.h"

bool CStateDomain::verifyStatus()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return false;
}

bool CStateDomain::createDirectory(std::string name, std::vector<uint8_t>& newPerspective, uint64_t& cost, std::string path, std::string currentDir, bool inSandbox, bool hidden, std::shared_ptr<CAccessToken> accessToken, std::shared_ptr<CSecDescriptor> secDescriptor)
{
	CDataTrieDB dir(mBlockchainMode, name);
	if (name.size() == 0)
		return false;

	std::vector<uint8_t> hash = CCryptoFactory::getInstance()->getSHA2_256Vec(mTools->stringToBytes(name));
	if (saveValueDB(accessToken, hash, dir, eDataType::eDataType::directory, newPerspective, cost, inSandbox, hidden, path, currentDir, nullptr, false, false, true, secDescriptor))
		return true;
	return false;
}

bool CStateDomain::removeElement(std::string name, std::vector<uint8_t>& newPerspective, uint64_t& cost, std::string path, std::string currentDir, bool inSandbox, bool hidden, std::shared_ptr<CAccessToken> accessToken, std::shared_ptr<CSecDescriptor> secDescriptor)
{
	if (name.size() == 0)
		return false;

	std::vector<uint8_t> hash = CCryptoFactory::getInstance()->getSHA2_256Vec(mTools->stringToBytes(name));
	if (saveValueDB(accessToken, hash, nullptr, eDataType::eDataType::noData, newPerspective, cost, inSandbox, hidden, path, currentDir, nullptr, false, false, true, secDescriptor))
		return true;
	return false;
}

bool CStateDomain::updateDirectory(CDataTrieDB dir, std::vector<uint8_t>& newPerspective, uint64_t& cost, std::string path, std::string currentDir, bool inSandbox, bool hidden, std::shared_ptr<CAccessToken> accessToken)
{
	std::vector<uint8_t> hash = CCryptoFactory::getInstance()->getSHA2_256Vec(mTools->stringToBytes(dir.getName()));
	if (saveValueDB(accessToken, hash, dir, eDataType::eDataType::directory, newPerspective, cost, inSandbox, hidden, path, currentDir, nullptr, false, false, true, nullptr))
		return true;
	return false;
}


/// <summary>
/// Prepares a path within the decentralized file-system, provided a valid URI.
/// Sets permissions to each created directory, according to a Security Descriptor, if one provided.
/// Otherwise, by default, permissions would be inherited from parent directory.
/// </summary>
/// <param name="modifiedDirectories"></param>
/// <param name="path"></param>
/// <param name="currentDir"></param>
/// <param name="pathEnd"></param>
/// <param name="cost"></param>
/// <param name="invalidateNodes"></param>
/// <param name="inSandbox"></param>
/// <param name="secToken"></param>
/// <param name="inheritPermissions"></param>
/// <returns></returns>
bool CStateDomain::preparePath(std::shared_ptr<CAccessToken> accessToken, std::vector<CDataTrieDB*>& modifiedDirectories, std::string& path, CDataTrieDB* currentDir, CDataTrieDB** pathEnd, uint64_t& cost, bool invalidateNodes, bool inSandbox, std::shared_ptr<CSecDescriptor> secToken, bool inheritPermissions)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::string currentDirStr;
	if (path.size() == 0)
		return false;

	if (path.size() != 0)
	{

		//Local Variables - BEGIN
		std::vector < std::string> dirs;
		std::string fileName;
		std::string dirpath;
		bool isAbsolutePath = false;
		std::shared_ptr<CSecDescriptor> secDesc;
		std::vector<uint8_t> currentDirID;
		std::string currentDirName;
		//Local Variables - END

		if (!mTools->parsePath(isAbsolutePath, path, fileName, dirs, dirpath))
			return false;

		if (isAbsolutePath)
		{
			//state-domain cannot process an absolute path (state-domain ID in root)
			//thus we'll treat the path as relative
			path = std::string(path.begin() + 1, path.end());
			if (!mTools->parsePath(isAbsolutePath, path, fileName, dirs, dirpath))
				return false;
		}


		if (currentDirStr.size() == 0)
		{
			if (dirs.size() > 0 && dirs[0] == ".")
				dirs.erase(dirs.begin());
		}
		else
		{
			if (dirs.size() > 0 && dirs[0] == ".")
				dirs[0] = currentDirStr;
		}
		modifiedDirectories.push_back(currentDir);

		bool updateSecDesc = false;
		for (int i = 0; i < dirs.size(); i++)
		{

			//Security - BEGIN
			secDesc = currentDir->getSecDescriptor();
			if (secDesc && !currentDir->getSecDescriptor()->getIsAllowed(accessToken, mStateDomainManager, updateSecDesc))
			{//todo: take updateSecDesc into account?
				//this should never happen after update 1 (no default allowence)
				return false;
			}
			//Security - END

			currentDir->invalidateNode();

			std::vector<uint8_t> subdirHash = CCryptoFactory::getInstance()->getSHA2_256Vec(mTools->stringToBytes(dirs[i]));
			CDataTrieDB* foundSubDir = static_cast<CDataTrieDB*>(currentDir->findNodeByFullID(subdirHash));

			if (foundSubDir == nullptr)
			{
				CDataTrieDB subDir(mBlockchainMode, dirs[i]);

				//Security - BEGIN
				if (secToken != nullptr) {//prefer explicit security descriptor over an explicit, inherited one
					subDir.setSecDescriptor(secToken);//explicit security descriptor provided; let's use it
				}
				else if (inheritPermissions)
				{
					if (secDesc)
					{
						subDir.setSecDescriptor(std::make_shared<CSecDescriptor>(secDesc));
					}
				}
				else
				{
					//no explicit security token would be available for this object
				}
				//Security - END

				cost += CGlobalSecSettings::getPricePerColdStorageByte() * dirs[i].size();
				//create the sub-dir
				if (currentDir->addNodeExt(mTools->bytesToNibbles(subdirHash), &subDir, inSandbox, true) != CTrieDB::eResult::added)
					assertGN(false);//should not happen
				currentDir = static_cast<CDataTrieDB*>(currentDir->findNodeByFullID(subdirHash));

				if (currentDir == nullptr)
					assertGN(false);//should not happen
				modifiedDirectories.push_back(currentDir);

			}
			else
			{
				currentDir = foundSubDir;
				modifiedDirectories.push_back(currentDir);
			}
			if (dirs.size() == i + 1)

			{
				*pathEnd = currentDir;
				return true;
			}

		}
	}
	return true;
}

std::vector<uint8_t> CStateDomain::getAddress(bool prettyPrint)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> toRet = mAddress;
	if (prettyPrint && toRet.size() > 0)
	{
		for (int i = toRet.size() - 1; toRet[i] == 0; i--)
			toRet.pop_back();
	}
	return toRet;

}
std::vector<uint8_t> CStateDomain::getStoragePerspective()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mStorageDB->getPerspective();
}

void CStateDomain::syncPerspectiveFieldFromDB()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mStoragePerspective = mStorageDB->getPerspective();
}
bool CStateDomain::setStoragePerspective(std::vector<uint8_t> perspective)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (perspective.size() != 32)
		return false;
	if (!mStorageDB->setPerspective(perspective))
		return false;
	mStoragePerspective = perspective;
	return false;
}
CKeyChain* CStateDomain::getKeyChain()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mKeyChain;
}
/// <summary>
/// Changes the State Domain's view to be from a given Perspective (from the perspective of a given decentralised StateDB's root node)
/// Note: disable refreshInnerData when this could cause infinite recursion.
/// </summary>
/// <param name="perspectiveID"></param>
/// <param name="refreshInnerData"></param>
/// <returns></returns>
bool CStateDomain::setPerspective(std::vector<uint8_t> perspectiveID, bool refreshInnerData)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> prevAdr = getAddress();
	if (perspectiveID.size() != 32)
		return false;
	if (mLocalStateDB == NULL)
	{
		mLocalStateDB = new CTrieDB(CGlobalSecSettings::getStateDBID(), CSolidStorage::getInstance(mBlockchainMode), perspectiveID, true, refreshInnerData, refreshInnerData);//changed to ommit copying over of known nodes //new CTrieDB(CBlockchainManager::getInstance(mUseTestDB)->getStateTrie());
		//the content of the global Flow Trie DB might get changed during this operation thus we need to sync access
		//to the global trie.
		mLocalStateDB->setFlowStateDB(false);
	}




	if (mStorageDB == NULL)
	{
		mStorageDB = new CDataTrieDB(mBlockchainMode);
		mStorageDB->setParentDomain(this);
	}

	if (std::memcmp(mLocalStateDB->getPerspective().data(), perspectiveID.data(), 32) != 0)
	{
		//first update the perspective of the inner stateDB
		if (!mLocalStateDB->setPerspective(perspectiveID))
			return false;
	}
	//Note:do not update the perspective of the FlowDB it is *NOT* of the Domain's concern.
	//if (mFlowStateDB != NULL)
	//	mFlowStateDB->setPerspective(perspectiveID);
	validatePerspectives();

	if (refreshInnerData)
		refresh();
	mStorageDB->setPerspective(mStoragePerspective);
	std::vector<uint8_t> newAdr = getAddress();
	assertGN(std::memcmp(prevAdr.data(), newAdr.data(), 32) == 0);
	return true;
}

/// <summary>
/// Refreshes this particular state domaina's internal variables to reflect what is available
/// in the version visible from another (updated) perspective.
/// </summary>
void CStateDomain::refresh()
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	assertGN(mLocalStateDB != NULL);
	assertGN(mLocalStateDB->isEmpty(false));

	//mStateDB->changePerspective(mPerspective);
	AfterRunGuard guard = AfterRunGuard(&mLocalStateDB);
	CStateDomain* currentVersion = NULL;
	std::shared_ptr<CStateDomainManager>  sdm = std::make_shared<CStateDomainManager>(mLocalStateDB, mBlockchainMode);
	currentVersion = sdm->findByID(mAddress);
	//Note: mLocalStateDB is empty /pruned almost allways
	//the above assumes that a Flow begins allways from a state available from within the Cold Storage
	//
	if (currentVersion != NULL)
	{
		//create a copy as the former pointed to by currentVersion will be wiped on stateDb destruction
		//currentVersion = new CStateDomain(*currentVersion);

		//copy data from the retrieved state domain; instantiate the state and restore databases

		std::vector<uint8_t> previousAdr = getAddress();
		copyFields(*currentVersion, false);
		std::vector<uint8_t> newAdr = getAddress();
		assertGN(std::memcmp(previousAdr.data(), newAdr.data(), 32) == 0);

		//delete currentVersion; - the node will be deleted during AfterRunGuard anyway
		//common part
		invalidateNode();
		if (mInFlow)
			mWasAlreadyModifiedInFlow = true;


	}
	else
	{
		mNonce = 1;
		mKeyChain = NULL;
		mType = 3;
		mSubType = 1;

		if (mStorageDB == NULL)
		{
			mStorageDB = new CDataTrieDB(mBlockchainMode);
			mStorageDB->setParentDomain(this);
		}
		mStorageDB->setPerspective(mStoragePerspective);
		mStoragePerspective = mStorageDB->getPerspective();
		invalidateNode();
		if (mInFlow)
			mWasAlreadyModifiedInFlow = true;
	}
	CKeyChain chain(false);
	if (mKeyChain != NULL)
		delete mKeyChain;
	if (CSettings::getInstance(mBlockchainMode)->getCurrentKeyChain(chain, false, false, std::string(mAddress.begin(), mAddress.end())))
	{
		mKeyChain = new CKeyChain(chain);
	}
	else
		mKeyChain = NULL;
}
std::vector<uint8_t> CStateDomain::getPerspective()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	assertGN(mLocalStateDB != NULL);
	return mLocalStateDB->getPerspective();
}
void CStateDomain::setKeyChain(CKeyChain* chain)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mKeyChain = chain;
}
CStateDomain& CStateDomain::operator=(const CStateDomain& t)
{
	if (this == &t)
		return *this;

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	CTrieLeafNode::operator =(t);
	mPendingBalanceChange = t.mPendingBalanceChange;
	mLastModified = t.mLastModified;
	mPreviousBodyIDs = t.mPreviousBodyIDs;
	mPreviousPerspectives = t.mPreviousPerspectives;
	mStateDomainManager = t.mStateDomainManager;

	if (mLocalStateDB != NULL && t.mLocalStateDB != NULL)
	{
		mLocalStateDB->pruneTrie();
		*mLocalStateDB = *t.mLocalStateDB;
	}
	else if (t.mLocalStateDB != NULL && mLocalStateDB == NULL)
	{
		mLocalStateDB = new CTrieDB(*t.mLocalStateDB);
	}
	else
	{
		if (mLocalStateDB != NULL)
		{
			delete mLocalStateDB;
		}

		mLocalStateDB = NULL;
	}

	mFlowStateDB = t.mFlowStateDB;
	mInFlow = t.mInFlow;
	if (t.mKeyChain != NULL)
	{
		if (mKeyChain != NULL)
			*mKeyChain = *t.mKeyChain;
		else
			mKeyChain = new CKeyChain(*t.mKeyChain);

	}
	else
	{
		if (mKeyChain != NULL)
			delete mKeyChain;
		mKeyChain = NULL;
	}
	mVersion = t.mVersion;
	mSynced = t.mSynced;
	mNonce = t.mNonce;
	mAddress = t.mAddress;
	mCodeHash = t.mCodeHash;
	mBlockchainMode = t.mBlockchainMode;
	mStoragePerspective = t.mStoragePerspective;
	//StorageDB
	if (mStorageDB == NULL)
	{
		mStorageDB = new CDataTrieDB(*t.mStorageDB);
		mStorageDB->setParentDomain(this);
	}
	else
	{
		if (!t.mStorageDB->isEmpty())
			t.mStorageDB->copyTo(mStorageDB, true);
		mStorageDB->setPerspective(t.mStoragePerspective);
	}
	return *this;
	// TODO: insert return statement here
}

bool CStateDomain::validateStoragePerspective()
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mStorageDB == NULL)
		return false;

	if (mStoragePerspective.size() != 32)
		return false;
	if (std::memcmp(mStorageDB->getRoot()->getHash().data(), mStoragePerspective.data(), 32) != 0)
		return false;
	return true;
}

bool CStateDomain::validatePerspectives()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mFlowStateDB == NULL)
		return true;//the state domain is not within a Flow; there is nothing to compare against.

	if (mLocalStateDB == NULL)
		return false;
	std::vector<uint8_t> mStateP = mLocalStateDB->getPerspective();
	std::vector<uint8_t> mFlowP = mFlowStateDB->getPerspective();
	if (std::memcmp(mStateP.data(), mFlowP.data(), 32) != 0)
		return false;

	return true;
}

void CStateDomain::setInFlowStateDB(CTrieDB* db)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mFlowStateDB = db;
}

bool CStateDomain::exitFlow()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!mInFlow)
		return false;
	mFlowStateDB = NULL;
	mInFlow = false;
	//mStateDomainManager = nullptr; do not delete it; it might come to use later on; no bigger point in doing so;
	return true;
}

void CStateDomain::setStateDomainManager(std::shared_ptr<CStateDomainManager> sdm)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mStateDomainManager = sdm;
}
//retrieves FlowDB from the BlockchainManager and stores its pointer.
//The perspective of mStateDB and mFlowStateDB need to match at all times.
//WARNING: this takes use of  the Global Flow DB provided by the BlockchainManager;
//DO NOT use this method on a StateDomain created by or used by an external/detached Transaction Manager
//i.e. the one created by the Test unit for instance. The test unit has an antirely separate database.
//ToDo: make each StateDomain contain a SmartPointer  to a StateDomain manager responsible for maintaining it.

//WARNING: Do *NOT* enter the Flow on a stack-based object; the Destructor removes itself from the StateDomain manager.
bool CStateDomain::enterFlow(std::shared_ptr<CStateDomainManager> sdm)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	assertGN(sdm != nullptr);
	if (mInFlow)
		return false;
	if (sdm == nullptr)
		return false;
	if (!sdm->isInFlow())
		return false;
	mStateDomainManager = sdm;
	//assert(mFlowStateDB == NULL);
	if (!mStateDomainManager->getDB()->isInFlowStateDB())
		return false;
	mFlowStateDB = mStateDomainManager->getDB();//CBlockchainManager::getInstance(mUseTestDB)->getLiveTransactionsManager()->getFlowDB();

	mInFlow = true;
	mStateDomainManager->registerDomain(this);
	return true;
}

CTrieDB* CStateDomain::getInFlowStateDB()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mFlowStateDB;
}

/// <summary>
/// Adds Domain's body ID to back-log.
/// IF the back-log's length is exceeded. Data-pruning mechanism kicks-in, IF doColdStorageCleanUp was set.
/// </summary>
/// <param name="id"></param>
/// <param name="doColdStorageCleanUp"></param>
/// <returns></returns>
uint64_t CStateDomain::addPreviousBodyID(std::vector<uint8_t> id, std::vector<uint8_t> perspective, bool doColdStorageCleanUp)
{
	uint64_t deletedBodiesCount = 0;
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (id.size() != 32)
		return deletedBodiesCount;

	for (int i = 0; i < mPreviousBodyIDs.size(); i++)
	{
		if (mPreviousBodyIDs.size() == 32)
		{
			if (std::memcmp(mPreviousBodyIDs[i].data(), id.data(), 32))
			{
				return deletedBodiesCount;
			}
		}
	}
	mPreviousBodyIDs.push_back(id);
	mPreviousPerspectives.push_back(perspective);
	invalidateNode();

	if (doColdStorageCleanUp)
	{
		//keep data-pruning until the back-log's length is satisfactiry
		for (auto i = mPreviousBodyIDs.begin(); mPreviousBodyIDs.size() > CGlobalSecSettings::getDomainMaxBacklogCount();)
		{
			i = mPreviousBodyIDs.erase(i);
			//mPreviousPerspectives.erase(i);
			assertGN(mStateDomainManager->deleteDomainBody(getHash()));
			deletedBodiesCount++;
		}
		if (deletedBodiesCount > 0)
			mPreviousPerspectives = std::vector<std::vector<uint8_t>>(mPreviousPerspectives.begin() + deletedBodiesCount, mPreviousPerspectives.end());

	}

	if (mInFlow)
		mWasAlreadyModifiedInFlow = true;
	//todo:remove previous bodies if limit exceeded.
	return deletedBodiesCount;
}

std::vector<std::vector<uint8_t>> CStateDomain::getPreviousBodies()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mPreviousBodyIDs;
}

std::vector<std::vector<uint8_t>> CStateDomain::getPreviousPerspectives()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mPreviousPerspectives;
}

bool CStateDomain::isInFlow()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mInFlow;
}



std::vector<nibblePair> CStateDomain::getAddressNibbles()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mTools->bytesToNibbles(mAddress);

}
void CStateDomain::setAddress(std::vector<uint8_t> adr)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	assertGN(mAddress.size() == 0);
	mAddress = adr;
	invalidateNode();
	if (mInFlow)
		mWasAlreadyModifiedInFlow = true;
}

CStateDomain::CStateDomain(eBlockchainMode::eBlockchainMode blockchainMode) :CTrieLeafNode(NULL)
{
	assertGN(blockchainMode > -1);
	mTools = CTools::getTools();
	mBlockchainMode = blockchainMode;
	mWasAlreadyModifiedInFlow = false;
	mStateDomainManager = nullptr;
	mFlowStateDB = NULL;
	mLastModified = std::time(0);
	mVersion = 1;
	mNonce = 1;
	mLocalStateDB = NULL;
	mKeyChain = NULL;
	mType = eNodeType::leafNode;
	mSubType = eNodeSubType::stateDomain;
	mStorageDB = NULL;
	mSynced = false;
	mInFlow = false;
	mLastModified = std::time(0);
	mPendingBalanceChange = 0;

	mStorageDB = new CDataTrieDB(mBlockchainMode);
	mStorageDB->setParentDomain(this);
}
bool CStateDomain::isDirectoryEx(std::string path)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	if (!path.size() || (path.size() == 1 && path[0] == '/'))
		return true;
	CDataTrieDB* currentDB = mStorageDB;
	std::string currentDir;

	if (path.size() > 0)
	{
		std::vector<std::string> dirs;
		std::string dirPart;
		CTrieNode* n = nullptr;

		if (path.size() > 0)
		{
			if (!mTools->isAbsolutePath(path))
				path = currentDir + path;
		}

		if (!goToPath(path, &n, nullptr, true) || n == nullptr)
		{
			return false;
		}

		if (n->isDirectory())
			return true;

	}
	return false;
}

/// <summary>
/// Returns the amount of locked assets from the perspective of a given Key-Block-Height(fromKBHPerspective).
/// </summary>
/// <returns></returns>
BigInt CStateDomain::getLockedAssets(uint64_t fromKBHPerspective, bool& justReleased, const uint64_t& lockedTillKBH)
{

	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <---

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//LOCAL VARIABLES - BEGIN
	eDataType::eDataType vt;
	std::vector<uint8_t> byteVector;
	BigInt lockedAssets = 0;
	uint64_t lockedTillBlockHeight = 0;
	//LOCAL VARIABLES - END
	bool isVestingEnabledOnAccount = false;

	//check current lock-time
	byteVector = mStorageDB->loadValue(CGlobalSecSettings::getLockedGNCTillBlockHeightVarID(), vt);
	assertGN(byteVector.size() <= 8);

	if (byteVector.size() > 0)
	{
		std::memcpy(&lockedTillBlockHeight, byteVector.data(), byteVector.size());
		const_cast<uint64_t&>(lockedTillKBH) = lockedTillBlockHeight;
	}

	if (fromKBHPerspective > lockedTillBlockHeight)
	{
		justReleased = true;
		return 0;
	}
	else
		justReleased = false;

	//check the current amount of locked assets
	byteVector = mStorageDB->loadValue(CGlobalSecSettings::getLockedGNCValueVarID(), vt);

	if (vt != eDataType::noData && vt != eDataType::BigInt)
		throw "Invalid data type or currency variable.";

	if (byteVector.size() > 0)
		lockedAssets = mTools->BytesToBigInt(byteVector);

	// Load the spentInCurrentInterval value from the storage
	byteVector = mStorageDB->loadValue(CGlobalSecSettings::getSpentInCurrentIntervalID(), vt);
	BigInt spentInCurrentInterval = (byteVector.size() > 0) ? mTools->BytesToBigInt(byteVector) : 0;
	std::string temp = spentInCurrentInterval.str();
	// get the maxSpendPerInterval value
	BigInt maxSpendPerInterval = CGlobalSecSettings::getMaxSpendPerInterval();
	temp = maxSpendPerInterval.str();
	// Compute the additional locked assets due to vesting
	if (fromKBHPerspective < lockedTillBlockHeight) {
		//Full-Lock Period - BEGIN
		// We're still within the full-lock period
		//if (isVestingEnabledOnAccount)
		//{
	   //	 lockedAssets += maxSpendPerInterval - spentInCurrentInterval;
		//}
		//Full-Lock Period  - END
		temp = lockedAssets.str();
	}
	/*
	else if (fromKBHPerspective - lockedTillBlockHeight < CGlobalSecSettings::getVestingInterval()) {
		// We're in a new vesting interval, but the account owner has not yet spent anything
		lockedAssets += maxSpendPerInterval;
		temp = lockedAssets.str();
	}
	else {
		// We're in a new vesting interval, and the account owner has spent some assets
		lockedAssets += maxSpendPerInterval - spentInCurrentInterval;
		temp = lockedAssets.str();
	}
	temp = lockedAssets.str();*/
	return lockedAssets;
}


/// <summary>
/// The function increases the locked amount of resources.
/// The resources will be unlocked AT key-block-height equal to tillKeyBlockHeight.
/// The locked value cumulates i.e. if miner digs a couple of consecutive blocks
/// he/she will need to take a break to make the rewards spendable (which is good, lowers centralization).
/// Also, it's more storage-friendly than keeping a log a received rewards and release-counters for each.
/// IMPORTANT: It's CRUCIAL to provide the current key-block-height (fromKBHPerspective).
/// USAGE: external AND used directly by the GridScript-VM (ie.e it's a 'native' atomic function, there's no counterpart implemented in GridScript).
/// Important: If the function returns FALSE, the Flow should be *ABORTed*.
/// </summary>
/// <param name="value"></param>
/// <param name="tillBlockHeight"></param>
bool CStateDomain::incLockedAssets(std::vector<uint8_t>& perspectiveID, uint64_t fromKBHPerspective, BigInt value, bool sandBoxMode, uint64_t tillBlockHeight)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	invalidateNode();
	//LOCAL VARIABLES - BEGIN
	eDataType::eDataType vt;
	std::vector<uint8_t> byteVector;
	BigInt lockedAssets = 0;
	uint64_t lockedTillKeyBlockHeight = 0;
	uint64_t cost = 0;
	bool lockTimeExpired = false;
	bool wasLockTimeModified = false;
	//LOCAL VARIABLES - END

	//Note: a loadValue() function CAN return an empty vector. it is FINE - we assume 0.

	//check current lock-time
	byteVector = mStorageDB->loadValue(CGlobalSecSettings::getLockedGNCTillBlockHeightVarID(), vt);
	assertGN(byteVector.size() <= 8);

	if (byteVector.size() > 0)
		std::memcpy(&lockedTillKeyBlockHeight, byteVector.data(), byteVector.size());

	if (lockedTillKeyBlockHeight < fromKBHPerspective)
	{	 //NOTE: the lock-time expired
		//we need to reset the cumulative locked-amount of resources
		//otherwise, the  new lock would renew increased by the value of a previous one.
		if (!saveValueDB(CAccessToken::genSysToken(), mTools->stringToBytes(CGlobalSecSettings::getLockedGNCValueVarID()), 0, eDataType::eDataType::unsignedInteger, perspectiveID, cost, sandBoxMode, true, "", "", nullptr, false, false, true, CSecDescriptor::genSysOnlyDescriptor()))
			return false;
	}

	//check the current amount of locked assets
	byteVector = mStorageDB->loadValue(CGlobalSecSettings::getLockedGNCValueVarID(), vt);
	assertGN(byteVector.size() <= 256);

	if (byteVector.size() > 0)
		lockedAssets = mTools->BytesToBigInt(byteVector);


	//update the amount of locked assets
	lockedAssets += value;
	//update the lock-time
	//IMPORTANT: update the lock-time ONLY if the requested lock-time is HIGHER than the one already set.
	//we might NOT know the reason why was it set by *DECENTRALIZED CONSENSUS* to a higher value ALREADY.

	if (tillBlockHeight > lockedTillKeyBlockHeight)
	{
		lockedTillKeyBlockHeight = tillBlockHeight;
		wasLockTimeModified = true;
	}

	//COMMIT the changes. Note: the function would return FALSE if ANY of the below fail.
	//REASONING: We need to ensure the entire transaction was committed (increased balance AND increased block delay)
	//in such a case, the Flow should ABORT.
	if (mInFlow)
		mWasAlreadyModifiedInFlow = true;


	if (!saveValueDB(CAccessToken::genSysToken(), CGlobalSecSettings::getLockedGNCValueVarID(), lockedAssets, eDataType::eDataType::BigInt, perspectiveID, cost, sandBoxMode, true, "", "", nullptr, false, false, true, CSecDescriptor::genSysOnlyDescriptor()))
	{
		return false;
	}

	// if (!saveValueDB(mTools->stringToBytes(CGlobalSecSettings::getLockedGNCValueVarID()), lockedAssets, eDataType::eDataType::BigInt, perspectiveID, cost, sandBoxMode,true,"","",nullptr,false,false,true, CRightsToken::genSysOnlyDescriptor()))
		// return false;

	if (wasLockTimeModified)
	{
		if (!saveValueDB(CAccessToken::genSysToken(), mTools->stringToBytes(CGlobalSecSettings::getLockedGNCTillBlockHeightVarID()), lockedTillKeyBlockHeight, eDataType::eDataType::unsignedInteger, perspectiveID, cost, sandBoxMode, true, "", "", nullptr, false, false, true, CSecDescriptor::genSysOnlyDescriptor()))
			return false;
	}

	return true;

}

bool CStateDomain::setLockedAssets(std::vector<uint8_t>& perspectiveID, uint64_t value, bool sandBoxMode, uint64_t tillBlockHeight)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	invalidateNode();
	//LOCAL VARIABLES - BEGIN
	eDataType::eDataType vt;
	std::vector<uint8_t> byteVector;
	uint64_t lockedAssets = 0;
	uint64_t lockedTillKeyBlockHeight = 0;
	uint64_t cost = 0;
	bool lockTimeExpired = false;
	bool wasLockTimeModified = false;
	//LOCAL VARIABLES - END

	//Note: a loadValue() function CAN return an empty vector. it is FINE - we assume 0.

	//check current lock-time
	byteVector = mStorageDB->loadValue(CGlobalSecSettings::getLockedGNCTillBlockHeightVarID(), vt);
	assertGN(byteVector.size() <= 8);

	if (byteVector.size() > 0)
		std::memcpy(&lockedTillKeyBlockHeight, byteVector.data(), byteVector.size());

	//check the current amount of locked assets
	byteVector = mStorageDB->loadValue(CGlobalSecSettings::getLockedGNCValueVarID(), vt);
	assertGN(byteVector.size() <= 8);

	if (byteVector.size() > 0)
		std::memcpy(&lockedAssets, byteVector.data(), byteVector.size());

	//update the amount of locked assets
	lockedAssets = value;
	//update the lock-time
	//IMPORTANT: update the lock-time ONLY if the requested lock-time is HIGHER than the one already set.
	//we might NOT know the reason why was it set by *DECENTRALIZED CONSENSUS* to a higher value ALREADY.

	if (tillBlockHeight > lockedTillKeyBlockHeight)
	{
		lockedTillKeyBlockHeight = tillBlockHeight;
		wasLockTimeModified = true;
	}

	//COMMIT the changes. Note: the function would return FALSE if ANY of the below fail.
	//REASONING: We need to ensure the entire transaction was commited (increased balance AND increased block delay)
	//in such a case, the Flow should ABORT.
	if (mInFlow)
		mWasAlreadyModifiedInFlow = true;

	if (!saveValueDB(CAccessToken::genSysToken(), mTools->stringToBytes(CGlobalSecSettings::getLockedGNCValueVarID()), lockedAssets, eDataType::eDataType::unsignedInteger, perspectiveID, cost, sandBoxMode, true, "", "", nullptr, false, false, true, CSecDescriptor::genSysOnlyDescriptor()))
		return false;

	if (wasLockTimeModified)
	{
		if (!saveValueDB(CAccessToken::genSysToken(), mTools->stringToBytes(CGlobalSecSettings::getLockedGNCTillBlockHeightVarID()), lockedTillKeyBlockHeight, eDataType::eDataType::unsignedInteger, perspectiveID, cost, sandBoxMode, true, "", "", nullptr, false, false, true, CSecDescriptor::genSysOnlyDescriptor()))
			return false;
	}

	return true;

}

BigSInt CStateDomain::getPendingPreTaxBalanceChange()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mPendingBalanceChange;
}

bool CStateDomain::setPendingPreTaxBalanceChange(BigSInt value)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mPendingBalanceChange = value;
	return true;
}

bool CStateDomain::goToPath(std::string path, CTrieNode** found, CDataTrieDB* currentFolder, bool targetMayNotBeADir)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	if (path.size() == 0)
		return false;

	std::string dirpart;
	std::string filePart, fileName, dirpath, stateDomainPart;
	std::vector<std::string> dirs;
	std::vector<uint8_t> currentHash;
	CTrieNode* currentNode = (currentFolder ? currentFolder : mStorageDB);//fall back to root directory if not provided

	if (currentNode == nullptr)
		currentNode = mStorageDB;

	bool reachedFinalEnd = false;
	bool isAbsolutePath = true;

	if (!mTools->parsePath(isAbsolutePath, path, filePart, dirs, dirpart))
	{
		return false;
	}

	if (isAbsolutePath)
	{
		//state-domain cannot process an absolute path (state-domain ID in root)
		//thus we'll treat the path as relative
		path = std::string(path.begin() + 1, path.end());
		if (!mTools->parsePath(isAbsolutePath, path, fileName, dirs, dirpath, stateDomainPart))
			return false;
		// currentNode = mStorageDB;

	}

	if (dirs.size() == 0)
		dirs.push_back(filePart);
	if (path.size() == 0 || (path.size() == 1 && path[1] == '/'))
		reachedFinalEnd = true;
	else
	{
		for (int i = 0; i < dirs.size(); i++)
		{
			if (dirs[i] != "..")
			{
				currentHash = CCryptoFactory::getInstance()->getSHA2_256Vec(mTools->stringToBytes(dirs[i]));
				if (currentNode->isDirectory())
				{
					currentNode = static_cast<CDataTrieDB*> (currentNode)->findNodeByFullID(currentHash);
					if (currentNode == nullptr)
						return false;
				}
				else return false;//this should be a directory
			}
			else
			{
				//go up the Tree
				currentNode = currentNode->getParentDir();
				if (currentNode == nullptr)
					return false;
			}
			if (i == dirs.size() - 1)
				reachedFinalEnd = true;
		}
	}

	if (reachedFinalEnd)
	{
		if (filePart.size() == 0)
		{
			*found = currentNode;
			return true;
		}
		else
		{
			currentHash = CCryptoFactory::getInstance()->getSHA2_256Vec(mTools->stringToBytes(filePart));

			if (targetMayNotBeADir && filePart.size() && currentNode->isDirectory())
			{
				//object is not REQUIRED to be a directory thus try to find the file
				CTrieNode* node = reinterpret_cast<CDataTrieDB*>(currentNode)->findNodeByFullID(currentHash);

				if (node)
				{
					*found = node;
					return true;
				}
			}


			if (currentNode->isDirectory() || targetMayNotBeADir)
			{//if the object is a directory or object is not REQUIRED to be a directory
				*found = currentNode;
				return true;
			}
			else
			{
				//cant CD into a file. still, the path is valid so we return the containing directory
				*found = currentNode->getParentDir();
				if ((*found) != nullptr && (*found)->isDirectory())
					return true;
				else
					return false;
			}

		}
	}
	else return false;

}

bool CStateDomain::changeBalanceBy(std::vector<uint8_t>& perspectiveID, int64_t incBy, bool doInSanBox)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::shared_ptr<CSecDescriptor> secDescriptor = std::make_shared<CSecDescriptor>(true); //create a system-only security descriptor
	BigSInt incByB = incBy;
	return changeValueBy(perspectiveID, mTools->stringToBytes(CGlobalSecSettings::getGlobalAssetID()), incByB, doInSanBox, secDescriptor);
}

bool CStateDomain::changeBalanceBy(std::vector<uint8_t>& perspectiveID, BigSInt incBy, bool doInSanBox)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::shared_ptr<CSecDescriptor> secDescriptor = std::make_shared<CSecDescriptor>(true); //create a system-only security descriptor

	return changeValueBy(perspectiveID, mTools->stringToBytes(CGlobalSecSettings::getGlobalAssetID()), incBy, doInSanBox, secDescriptor);
}

/// <summary>
/// Forces a change to Big-Int stored within the State Domain.
/// A security-first minimalistic Kernel method.
/// The function *DOES NOT* support other data-types.
/// Use GridScript for that, which DOES support implicit type conversion.
/// 
/// IF a value is not of the aforementioned type the function returns FALSE.
/// If, value does not exist, it is created, as a BigInt.
/// Note the modBy param is SIGNED.
/// IF a state domain of a given stateDomainID does not exist under a given perspectiveID the domain will be created.
/// IF a state domain does not contain any value under the specific key, the value is set to be 0+modBy.
/// </summary>
/// <param name="stateDomainID"></param>
/// <param name="key">The might or might not be 32bytes long. IF it is NOT then it will be hashed to be of that size.</param>
/// <param name="modBy"></param>
/// <param name="doInSanBox"></param>
/// <returns></returns>
bool CStateDomain::changeValueBy(std::vector<uint8_t>& perspectiveID, std::vector<uint8_t> key, BigSInt modBy, bool doInSandBox, std::shared_ptr<CSecDescriptor> secDescriptor)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	//Local Variables - BEGIN
	bool success = false;
	uint64_t cost = 0;
	eDataType::eDataType type;
	BigInt currentVal = 0;

	//Local Variables - END

	//Operational Logic - BEGIN

	//To minimize the possibility of any problems. Here, we do NOT support any kind of data conversions.
	//Target values are stored ALWAYS as Big Unsigned 32 byte vectors (only significant bytes get stored though).
	std::vector<uint8_t> dataVec = loadValueDB(CAccessToken::genSysToken(), key, type);
	switch (type)
	{

	case eDataType::eDataType::BigInt:

		currentVal = mTools->BytesToBigInt(dataVec);

		try {

			if (modBy < 0 && ((static_cast<BigSInt>(currentVal) * -1) > modBy))
				return false;//that would result in an overflow

			currentVal = static_cast<BigInt>(static_cast<BigSInt>(currentVal) + modBy);

		}
		catch (...) { return false; }//(over/under) flow

		success = saveValueDB(CAccessToken::genSysToken(), key, mTools->BigIntToBytes(currentVal), eDataType::eDataType::BigInt, perspectiveID, cost, doInSandBox, true, "", "", nullptr, false, false, true, secDescriptor);

		break;


	case eDataType::eDataType::noData:

		if (currentVal < 0)
			throw "Balance of a new account cannot be < 0.";

		currentVal = static_cast<BigInt>(static_cast<BigSInt>(currentVal) + modBy);
		//convert to an unsigned 
		success = saveValueDB(CAccessToken::genSysToken(), key, mTools->BigIntToBytes(static_cast<BigInt>(currentVal)), eDataType::eDataType::BigInt, perspectiveID, cost, doInSandBox, true, "", "", nullptr, false, false, true, secDescriptor);
		break;
	default:
		return false;
		break;

	}

	//Operational Logic - END
	return success;

}
CStateDomain* CStateDomain::genNode(CTrieNode** baseDataNode, std::vector<uint8_t> perspective, eBlockchainMode::eBlockchainMode blockchainMode)
{
	if (*baseDataNode == NULL)
		return NULL;
	assertGN((*(baseDataNode))->isRegisteredWithinTrie() == false);
	CStateDomain* acc = new CStateDomain(blockchainMode);

	size_t subT = 0;
	try {

		assertGN((*baseDataNode)->getType() == 3);
		acc->mName = (*baseDataNode)->mName;
		Botan::BER_Decoder  dec = Botan::BER_Decoder((*baseDataNode)->getRawData()).start_cons(Botan::ASN1_Tag::SEQUENCE).
			decode(subT).
			decode(acc->mVersion);

		assertGN(subT == 1);//ensure it's an account;

		Botan::BER_Object obj;

		if (acc->mVersion == 1 || acc->mVersion == 2 || acc->mVersion == 3)
		{
			std::vector<uint8_t> adr;
			obj = dec.get_next_object();
			assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);

			Botan::BER_Decoder  dec2 = Botan::BER_Decoder(obj.value);
			dec2.decode(acc->mNonce);
			dec2.decode(acc->mAddress, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(acc->mCodeHash, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(acc->mStoragePerspective, Botan::ASN1_Tag::OCTET_STRING);

			if (acc->mVersion == 2)
			{
				dec2.decode(acc->mLastModified, Botan::ASN1_Tag::INTEGER);

				if (dec2.more_items())
				{
					obj = dec2.get_next_object();
					assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);
					Botan::BER_Decoder  dec3 = Botan::BER_Decoder(obj.value);
					Botan::BER_Object obj;
					while (dec3.more_items())
					{
						obj = dec3.get_next_object();
						if (obj.type_tag == Botan::ASN1_Tag::OCTET_STRING)
						{
							assertGN(Botan::unlock(obj.value).size() == 32);
							acc->mPreviousBodyIDs.push_back(Botan::unlock(obj.value));
						}
						else if (obj.type_tag == Botan::NULL_TAG)
						{
							acc->mPreviousBodyIDs.resize(0);
							break;

						}
					}
				}
			}
			else
				if (acc->mVersion == 3)
				{
					dec2.decode(acc->mLastModified, Botan::ASN1_Tag::INTEGER);
					if (dec2.more_items())
					{
						obj = dec2.get_next_object();
						assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);
						Botan::BER_Decoder  dec3 = Botan::BER_Decoder(obj.value);
						Botan::BER_Object obj;
						//previous bodies
						while (dec3.more_items())
						{
							obj = dec3.get_next_object();
							if (obj.type_tag == Botan::ASN1_Tag::OCTET_STRING)
							{
								assertGN(Botan::unlock(obj.value).size() == 32);
								acc->mPreviousBodyIDs.push_back(Botan::unlock(obj.value));
							}
							else if (obj.type_tag == Botan::NULL_TAG)
							{
								acc->mPreviousBodyIDs.resize(0);
								break;

							}
						}
						//previous perspectives

						obj = dec2.get_next_object();
						assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);
						Botan::BER_Decoder  dec4 = Botan::BER_Decoder(obj.value);
						// Botan::BER_Object obj;
						 //previous bodies
						while (dec4.more_items())
						{
							obj = dec4.get_next_object();
							if (obj.type_tag == Botan::ASN1_Tag::OCTET_STRING)
							{
								assertGN(Botan::unlock(obj.value).size() == 32);
								acc->mPreviousPerspectives.push_back(Botan::unlock(obj.value));
							}
							else if (obj.type_tag == Botan::NULL_TAG)
							{
								acc->mPreviousPerspectives.resize(0);
								break;

							}
						}

					}
				}
			//assert(!dec.more_items());

			//instantiate StateDomain's storage sub-system
			//retrieve State Domain's data storageRoot from local or network
			if (acc->mStoragePerspective.size() > 0 && acc->mAddress.size() > 0)
			{
				acc->mStorageDB = new CDataTrieDB(blockchainMode, "", acc->mStoragePerspective);
				acc->mStorageDB->setParentDomain(acc);
			}
			else
			{

			}
		}

		//commmon part between all specialized node types:
		acc->mInterData.resize((*baseDataNode)->mInterData.size());
		acc->mMainRAWData.resize((*baseDataNode)->mMainRAWData.size());

		std::memcpy(acc->mInterData.data(), (*baseDataNode)->mInterData.data(), (*baseDataNode)->mInterData.size());
		std::memcpy(acc->mMainRAWData.data(), (*baseDataNode)->mMainRAWData.data(), (*baseDataNode)->mMainRAWData.size());
		acc->mHasLeastSignificantNibble = (*baseDataNode)->mHasLeastSignificantNibble;

		if (baseDataNode != NULL)
		{
			if ((*baseDataNode)->mPointerToPointerFromParent != NULL)
				(*(*baseDataNode)->mPointerToPointerFromParent) = NULL;
			delete* baseDataNode;
		}
		(*baseDataNode) = NULL;
		if (perspective.size() == 32)
		{
			acc->setPerspective(perspective, false);

		}

		return acc;
	}
	catch (const std::exception& e)
	{
		if ((*baseDataNode) != NULL)
		{
			delete (*baseDataNode);
			(*baseDataNode) = NULL;
		}
		if (acc != nullptr)
			delete acc;

	}

	return NULL;
}
/// <summary>
/// Retrieves instance of an Identity Token.
/// </summary>
/// <returns></returns>
std::shared_ptr<CIdentityToken> CStateDomain::getIDToken()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	if (mIDToken != nullptr)
		return mIDToken;//pre-cached. It cannot change so..

	eDataType::eDataType vt;
	std::shared_ptr<CIdentityToken> token = nullptr;
	std::vector<uint8_t> serializedIDToken;


	{ // --- BEGIN FIX --- Scope for the storage DB lock
		std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
		// This call is now protected
		serializedIDToken = mStorageDB->loadValue(CGlobalSecSettings::getIDTokenKey(), vt);
	} // --- END FIX --- storageLock is released here

	if (serializedIDToken.size() > 0)
	{
		token = CIdentityToken::instantiate(serializedIDToken);
		mIDToken = token;
	}
	return token;
}

/// <summary>
/// The identity token gets saved as a serialized entity within the StateDomain's internal storage.
/// The Identity Token is a BER-encoded object.
/// </summary>
/// <param name="token"></param>
/// <param name="sandBoxMode"></param>
/// <returns></returns>
bool CStateDomain::setIDToken(CIdentityToken token, uint64_t& cost, bool sandBoxMode)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	if (mInFlow)
		mWasAlreadyModifiedInFlow = true;
	std::vector<uint8_t> newPerspective;
	std::shared_ptr<CSecDescriptor> secDescriptor = std::make_shared<CSecDescriptor>(true); //create a system-only security descriptor

	bool result = saveValueDB(CAccessToken::genSysToken(), CGlobalSecSettings::getIDTokenKey(), token.getPackedData(), eDataType::bytes, newPerspective, cost, sandBoxMode, false, "", "", nullptr, false, false, true, secDescriptor);

	if (result)
	{
		mStoragePerspective = mStorageDB->getPerspective();
	}


	return result;
}

bool CStateDomain::setTempIDToken(std::shared_ptr<CIdentityToken> id)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mTempID = id;
	return true;
}

std::shared_ptr<CIdentityToken> CStateDomain::getTempIDToken()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mTempID;
}

bool CStateDomain::addTokenPool(std::shared_ptr<CTokenPool> pool, uint64_t& cost, const std::vector<uint8_t>& key, bool sandBoxMode)
{
	if (pool == nullptr)
		return false;

	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> newPerspective;
	std::string friendlyID = pool->getFriendlyID();


	if (friendlyID.size() > 0)
	{
		const_cast<std::vector<uint8_t>&>(key) = mTools->stringToBytes(pool->getFriendlyID());
	}
	else
	{
		const_cast<std::vector<uint8_t>&>(key) = pool->getID();
		const_cast<std::vector<uint8_t>&>(key).erase(key.begin() + 4, key.end());//5 bytes of entropy good enough for colisions
		const_cast<std::vector<uint8_t>&>(key) = mTools->stringToBytes(mTools->base58Encode(key));


		///key = mTools->stringToBytes(mTools->getRandomStr(8));
	   //^ WARNING: we cannot use a random value since THAT would result in inconistencies during block processing both at local and other nodes
		//which is why the key needs to be based on a globally verifiable AND AVAILABLE value (token pool's ID is one of these).

	}


	//Note: a sys-only descriptor  is assigned below
	return saveValueDB(CAccessToken::genSysToken(), key, pool->getPackedData(), eDataType::bytes, newPerspective, cost, sandBoxMode, false, CGlobalSecSettings::getTokenPoolsDirName() + "/", "", nullptr, false, false, true, CSecDescriptor::genSysOnlyDescriptor());

}

bool CStateDomain::updateTokenPool(std::shared_ptr<CTokenPool> pool, uint64_t& cost, bool sandBoxMode)
{

	std::vector<uint8_t> key;
	return addTokenPool(pool, cost, key, sandBoxMode);//simple overwrite
}


std::shared_ptr<CTokenPool> CStateDomain::getTokenPoolP1(std::vector<uint8_t> id)
{

	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <---

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	// Ensure loadValueDB template specialization is called correctly

	// Assuming loadValueDB is templated and takes the key bytes directly
	eDataType::eDataType dType;
	std::vector<uint8_t> data = loadValueDB(CAccessToken::genSysToken(), id, dType, CGlobalSecSettings::getTokenPoolsDirName() + "/");//mStorageDB->loadValue(id, dType);


	std::shared_ptr<CTokenPool> pool = CTokenPool::instantiate(data, CCryptoFactory::getInstance());

	return pool;
}



CStateDomain* CStateDomain::instantiateStateDomain(std::vector<uint8_t> packedDataBER, std::vector<uint8_t> perspective, eBlockchainMode::eBlockchainMode blockchainMode)
{
	//CTools mTools;
	assertGN(packedDataBER.size() != 0);

	CStateDomain* domain = static_cast<CStateDomain*>(CBlockchainManager::getInstance(blockchainMode)->getTools()->nodeFromBytes(packedDataBER, blockchainMode, perspective));
	assertGN(domain != NULL);

	return domain;

}

bool operator == (const CStateDomain& c1, const CStateDomain& c2)
{

	return (
		c1.mVersion == c2.mVersion &&
		c1.mNonce == c2.mNonce &&
		std::memcmp(c1.mAddress.data(), c2.mAddress.data(), c1.mAddress.size()) == 0
		&& std::memcmp(c1.mCodeHash.data(), c2.mCodeHash.data(), c1.mCodeHash.size()) == 0
		&& std::memcmp(c1.mStoragePerspective.data(), c2.mStoragePerspective.data(), c1.mStoragePerspective.size()) == 0);
}

bool operator != (const CStateDomain& c1, const CStateDomain& c2)
{

	return !(c1 == c2);
}


CStateDomain::~CStateDomain()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	// if(mStorageTrieRoot!=NULL)
	//delete mStorageTrieRoot; //<= deleted by mStorageDB destructor below
	if (mStorageDB != NULL)
		delete mStorageDB;
	if (mKeyChain != NULL)
		delete mKeyChain;
	if (mLocalStateDB != NULL)
	{
		delete mLocalStateDB;
		//	CAsyncMemCleaner::cleanItVec(mLocalStateDB);
	}
	if (mStateDomainManager != nullptr && mInFlow)
	{
		//mStateDomainManager->unregisterDomain(this->getAddress());
		// => we cannot unregister the domain from here since
		//the object might be a temporary stack object, and this would casue uregistration of the proper one.
		//Update:: stack-based instance should NOT have the mInFlow flag set.
		mStateDomainManager = nullptr;
		mInFlow = false;
	}

}
CStateDomain::CStateDomain(std::vector<uint8_t> address, std::vector<uint8_t> perspective, std::vector<uint8_t> code, eBlockchainMode::eBlockchainMode mode, bool doRefresh) :CTrieLeafNode(NULL)
{
	if (address.size() < 34)
	{
		//expand
		address.resize(34, 0);
	}

	assertGN(address.size() == 34 || address.size() == 33);
	//assert(perspective.size() == 32);
	assertGN(mode > -1);
	mPendingBalanceChange = 0;
	mFlowStateDB = NULL;
	mAddress = address;
	mWasAlreadyModifiedInFlow = false;

	mBlockchainMode = mode;

	mTools = CTools::getTools();
	mLocalStateDB = NULL;
	mKeyChain = NULL;
	mInFlow = false;
	mType = 3;
	mSubType = 1;
	mNonce = 1; //0 represents empty parameter in GridScript in case of this usage
	mLastModified = std::time(0);
	mSynced = false;
	mVersion = 3;
	setPartialID(mTools->bytesToNibbles(address));
	mStorageDB = new CDataTrieDB(mBlockchainMode);
	mStorageDB->setParentDomain(this);

	if (doRefresh && (mode != eBlockchainMode::Unknown))
	{//todo: check this out. if eBlockchainMode is unknown, the domain is just a stub and should never be used for any concrete purpose. 
		mLocalStateDB = new CTrieDB(CBlockchainManager::getInstance(mBlockchainMode)->getStateTrie());
		if (perspective.size() == 32)
		{
			mLocalStateDB->setPerspective(perspective);
		}
		mLocalStateDB->pruneTrie();
		mLocalStateDB->setFlowStateDB(false);
		//AfterRunGuard guard = AfterRunGuard(&mLocalStateDB);
		refresh();
	}
	if (code.size() > 0)
	{
		this->setCode(code);
	}

}
std::vector<uint8_t> CStateDomain::getPackedData()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	//serialize account data into packedAccountData
	prepare(true);

	std::vector<uint8_t> result = ((CTrieLeafNode*)this)->getPackedData();

	return result;
}
CStateDomain::CStateDomain(CStateDomain& sibling) : CTrieLeafNode(sibling)
{
	copyFields(sibling);

}
void CStateDomain::copyFields(CStateDomain& sibling, bool nullPointers)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);


	mPendingBalanceChange = sibling.mPendingBalanceChange;
	mTools = sibling.mTools;
	mStoragePerspective = sibling.mStoragePerspective;
	mLastModified = sibling.mLastModified;
	mPreviousBodyIDs = sibling.mPreviousBodyIDs;
	mPreviousPerspectives = sibling.mPreviousPerspectives;
	mBlockchainMode = sibling.mBlockchainMode;
	mStateDomainManager = sibling.mStateDomainManager;

	if (nullPointers)
	{
		mKeyChain = nullptr;
		mLocalStateDB = nullptr;
		mStorageDB = nullptr;
		mFlowStateDB = nullptr;
	}

	mInFlow = sibling.mInFlow;

	if (mFlowStateDB == nullptr)//keep the current flowDB if it already is assigned.
		mFlowStateDB = sibling.mFlowStateDB;

	if (mLocalStateDB == nullptr)
	{
		mLocalStateDB = new CDataTrieDB(mBlockchainMode);

		//if (!sibling.mLocalStateDB->isEmpty(false))
		sibling.mLocalStateDB->copyTo(mLocalStateDB, true);//we need to copy the eventual data available in HotStorage as they might not be available
		assertGN(std::memcmp(mLocalStateDB->getPerspective().data(), sibling.mLocalStateDB->getPerspective().data(), 32) == 0);
		//mLocalStateDB = new CTrieDB(*sibling.mLocalStateDB);//create a copy on heap
	}
	else
		mLocalStateDB->setPerspective(sibling.getPerspective());

	if (mStorageDB == nullptr)
	{
		mStorageDB = new CDataTrieDB(mBlockchainMode, sibling.getStorageTrie()->getName(), mStoragePerspective);//todo: make use of a nice copy-constructor
		mStorageDB->setParentDomain(this);
		if (!sibling.mStorageDB->isEmpty(false))
			sibling.mStorageDB->copyTo(mStorageDB, true);//we need to copy the eventual data available in HotStorage as they might not be available
		if (std::memcmp(mStorageDB->getPerspective().data(), sibling.mStorageDB->getPerspective().data(), 32) != 0) {
			return;//should never happen
		}
		//in ColdStoraga; in other words; node might be participating in a Flow right now.
	}
	else
	{
		mStorageDB->setPerspective(mStoragePerspective);
		if (!sibling.mStorageDB->isEmpty(false), false)
			sibling.mStorageDB->copyTo(mStorageDB, true);//reason same as above
		if (std::memcmp(mStorageDB->getPerspective().data(), sibling.mStorageDB->getPerspective().data(), 32) != 0)
		{
			return;//should never happen
		}
	}


	if (sibling.mKeyChain != nullptr)
		mKeyChain = new CKeyChain(*sibling.mKeyChain);
	else
	{
		if (mKeyChain != nullptr)
			delete mKeyChain;
		mKeyChain = nullptr;
	}

	mVersion = sibling.mVersion;
	mSynced = sibling.mSynced;
	mNonce = sibling.mNonce;
	mAddress = sibling.mAddress;
	mCodeHash = sibling.mCodeHash;

	//common part
	//invalidateNode();
}
CDataTrieDB* CStateDomain::getStorageTrie()
{
	return mStorageDB;
}
bool CStateDomain::prepare(bool store, bool preparehashOnly)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	assertGN(getType() == 3 && getSubType() == 1, "oh but, ..I'm not that kind of node!");
	if (mIsPrepared)
		preparehashOnly = true;

scnround:
	std::vector<uint8_t> packedAccountData;
	if (store)
	{

		std::vector<uint8_t> dat;
		Botan::DER_Encoder enc1 = Botan::DER_Encoder().start_cons(Botan::ASN1_Tag::SEQUENCE)
			.encode(mSubType)//subtype
			.encode(mVersion).
			start_cons(Botan::ASN1_Tag::SEQUENCE).
			encode(mNonce).
			encode(mAddress, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mCodeHash, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mStoragePerspective, Botan::ASN1_Tag::OCTET_STRING);

		if (!preparehashOnly)
		{
			enc1 = enc1.encode(mLastModified, Botan::ASN1_Tag::INTEGER);
			//start encoding previous bodyIDs
			enc1 = enc1.start_cons(Botan::ASN1_Tag::SEQUENCE);
			for (int i = 0; i < mPreviousBodyIDs.size(); i++)
			{
				if (mPreviousBodyIDs.size() > 0)
					enc1 = enc1.encode(mPreviousBodyIDs[i], Botan::ASN1_Tag::OCTET_STRING);
				else
					enc1 = enc1.encode_null();
			}
			//end encoding previous bodyIDs
			enc1 = enc1.end_cons();

			//start encoding previous perspectives
			enc1 = enc1.start_cons(Botan::ASN1_Tag::SEQUENCE);
			for (int i = 0; i < mPreviousPerspectives.size(); i++)
			{
				if (mPreviousPerspectives.size() > 0)
					enc1 = enc1.encode(mPreviousPerspectives[i], Botan::ASN1_Tag::OCTET_STRING);
				else
					enc1 = enc1.encode_null();
			}
			//end encoding previous perspectives
			enc1 = enc1.end_cons();


		}
		enc1 = enc1.end_cons().end_cons();
		packedAccountData = enc1.get_contents_unlocked();
		if (!preparehashOnly)
		{
			((CTrieLeafNode*)this)->setRawData(packedAccountData);

		}
		else
		{
			mPreComputedHash = CCryptoFactory::getInstance()->getSHA2_256Vec(packedAccountData);
		}
		if (!preparehashOnly)
		{
			preparehashOnly = true;
			mIsPrepared = true;
			goto scnround;
		}


	}

	return true;
}

void CStateDomain::updateTimeStamp()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mLastModified = mTools->getTime();
	invalidateNode();
	if (mInFlow)
		mWasAlreadyModifiedInFlow = true;
}

uint64_t CStateDomain::getTimeStamp()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mLastModified;
}

size_t CStateDomain::getNonce()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mNonce;
}



bool  CStateDomain::incNonce(bool inSandBox)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return setNonce(++mNonce, inSandBox);
}

bool CStateDomain::setNonce(uint64_t nonce, bool inSandBox)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	validatePerspectives();
	mNonce = nonce;
	invalidateNode();
	if (mInFlow)
	{
		assertGN(mStateDomainManager != nullptr);
		mWasAlreadyModifiedInFlow = true;
		std::vector<uint8_t> newPerspective;
		assertGN(mStateDomainManager->update(*this, newPerspective, inSandBox));
	}

	validatePerspectives();
	return true;
}

bool CStateDomain::setSecDescriptor(std::shared_ptr<CSecDescriptor> desc, bool inSandBox)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	validatePerspectives();
	mSecDescriptor = desc;
	invalidateNode();
	if (mInFlow)
	{
		assertGN(mStateDomainManager != nullptr);
		mWasAlreadyModifiedInFlow = true;
		std::vector<uint8_t> newPerspective;
		assertGN(mStateDomainManager->update(*this, newPerspective, inSandBox));
	}

	validatePerspectives();
	return true;
}

std::shared_ptr<CSecDescriptor> CStateDomain::getSecDescriptor()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mSecDescriptor;
}


std::vector<uint8_t> CStateDomain::getCode(bool sandBoxMode)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> toRet;
	assertGN((size_t)mStorageDB < 0x1000000000000000);
	eDataType::eDataType vt;
	std::vector<uint8_t> code = mStorageDB->loadValue(CGlobalSecSettings::getStateDomainCodeBucketID(), vt);
	if (vt == eDataType::eDataType::bytes)
		toRet = code;
	return toRet;
}

bool CStateDomain::isSynced()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return this->mSynced;
}
/// <summary>
/// Sets balance of the main GRIDNET Coin exchange asset.
/// Returns result and the perspective in which the latest domain's state exists.
/// </summary>
/// <param name="key"></param>
/// <param name="value"></param>
/// <param name="sig"></param>
/// <returns></returns>
bool CStateDomain::setBalance(std::vector<uint8_t>& perspectiveID, BigInt value, bool sandBoxMode)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	uint64_t cost = 0;
	invalidateNode();
	if (mInFlow)
		mWasAlreadyModifiedInFlow = true;
	return  saveValueDB(CAccessToken::genSysToken(), CGlobalSecSettings::getGlobalAssetID(), value, eDataType::eDataType::BigInt, perspectiveID, cost, sandBoxMode, true, "", "", nullptr, false, false, true, CSecDescriptor::genSysOnlyDescriptor());


}
/// <summary>
/// Gets balance of the main GRIDNET Coin exchange asset.
/// </summary>
/// <param name="key"></param>
/// <param name="value"></param>
/// <returns></returns>
BigInt CStateDomain::getBalance(bool sandBoxMode)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!mStorageDB)
		return 0;
	eDataType::eDataType vt;
	std::vector<uint8_t> balanceV = mStorageDB->loadValue(CGlobalSecSettings::getGlobalAssetID(), vt);
	//assert(balanceV.size() <= 8);
	BigInt balance = 0;
	if (balanceV.size() > 0)
		balance = mTools->BytesToBigInt(balanceV);

	return balance;

}


bool CStateDomain::setCode(std::vector<uint8_t> code, bool sandBoxMode)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	//return false;
	invalidateNode();
	if (mInFlow)
		mWasAlreadyModifiedInFlow = true;
	bool result = mStorageDB->saveValue(CGlobalSecSettings::getStateDomainCodeBucketID(), code, eDataType::eDataType::bytes);
	if (result)
	{
		std::shared_ptr<CCryptoFactory>  cf = CCryptoFactory::getInstance();
		mCodeHash = cf->getSHA2_256Vec(code);
		mStoragePerspective = mStorageDB->getPerspective();

	}
	return result;
}

AfterRunGuard::AfterRunGuard(CTrieDB** db)
{
	mDBPointer = db;
}

AfterRunGuard::~AfterRunGuard()
{

	(*mDBPointer)->pruneTrie(true, true);
}
