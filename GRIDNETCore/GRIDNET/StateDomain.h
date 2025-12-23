#pragma once
#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "stdafx.h"
#include "TrieNode.h"
#include "TrieLeafNode.h"
#include "TrieDB.h"
#include "DataTrie.h"
#include "CGlobalSecSettings.h"
#include "ScriptEngine.h"
#include "RightsToken.h"
#include "AccessToken.h"
#include "CStateDomainManager.h"
#include "Poll.h"
static std::string H_ACCOUNT_ID = "acc_";
class CIdentityToken;
class CTokenPool;
class CTrieDB;
/// <summary>
/// Ensures local instance of StateDB is destroyed after modification ends.
/// (after the function execution ends and AfterRunGuard gets out of scope)
/// </summary>
class AfterRunGuard {
private:
	
	CTrieDB **mDBPointer;
public:
	AfterRunGuard(CTrieDB ** db);
	~AfterRunGuard();
};

/// <summary>
/// Represents state of a given State Domain.
/// All of the state domains together account for a given state of the global, decentralized State Database.
/// State Domains constitute Leaves of the latter Trie.
/// 
/// Change to any variables (code etc.) encapsulated within any State Database results in a new state of the global
/// State Database.(when we change any inter State-Domain variable the change propagates to  change within the
/// Global State Domain's root hash.
/// 
/// We can access prior states of a given State Domain by accessing the domain
/// 
/// </summary>
class CStateDomain : public CTrieLeafNode
{
private:
	BigSInt mPendingBalanceChange;//not serialized HotStorage ONLY
	bool mWasAlreadyModifiedInFlow;
	std::shared_ptr<CIdentityToken> mIDToken;
	
public :
	/*
	* Notice: a good example on how to modify the fields of CStateDomain is to look at setNonce().
	* It makes changes to private field propagate to the root of the Decentralized State Machine.
	*/
	bool isDirectoryEx(std::string path);
	BigInt getLockedAssets(uint64_t fromKBHPerspective,bool &justReleased, const uint64_t &lockedTillKBH=0);
	bool incLockedAssets(std::vector<uint8_t> &perspectiveID, uint64_t fromKBHPerspective,BigInt value, bool sandBoxMode, uint64_t tillBlockHeight);

	bool setLockedAssets(std::vector<uint8_t> &perspectiveID, uint64_t value, bool sandBoxMode, uint64_t tillBlockHeight=0);
	BigSInt getPendingPreTaxBalanceChange();
	bool setPendingPreTaxBalanceChange(BigSInt value);
	bool goToPath(std::string path, CTrieNode ** found,CDataTrieDB *currentFolder = nullptr,bool targetMayNotBeADir=false);
	bool changeBalanceBy(std::vector<uint8_t> & perspectiveID,int64_t incBy, bool doInSanBox = false);
	bool changeBalanceBy(std::vector<uint8_t>& perspectiveID, BigSInt incBy, bool doInSanBox);
	bool changeValueBy(std::vector<uint8_t>& perspectiveID, std::vector<uint8_t>key, BigSInt incBy, bool doInSanBox = false, std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);
	static CStateDomain * genNode(CTrieNode **node,std::vector<uint8_t> perspective,eBlockchainMode::eBlockchainMode blockchainMode);
	std::shared_ptr<CIdentityToken> getIDToken();
	bool setIDToken(CIdentityToken token,uint64_t &cost,bool sandBoxMode = true);
	bool setTempIDToken(std::shared_ptr<CIdentityToken> id);
	bool addTokenPool(std::shared_ptr<CTokenPool> pool, uint64_t& cost, const std::vector<uint8_t> &key = std::vector<uint8_t>(), bool sandBoxMode = false);
	bool updateTokenPool(std::shared_ptr<CTokenPool> pool,uint64_t & cost, bool sandBoxMode);
	std::shared_ptr<CTokenPool> getTokenPoolP1(std::vector<uint8_t> id);
	//constructors and destructor
	CStateDomain(std::vector<uint8_t> address, std::vector<uint8_t> perspective, std::vector<uint8_t> code,eBlockchainMode::eBlockchainMode mode, bool doRefresh=true);
	CStateDomain(eBlockchainMode::eBlockchainMode blockchainMode);//do not use directly
	~CStateDomain();
	CStateDomain(CStateDomain & sibling);

	void copyFields(CStateDomain & sibling,bool nullPointers=true);

	std::shared_ptr<CIdentityToken> getTempIDToken();

	friend bool operator == (const CStateDomain &c1, const CStateDomain &c2);
	friend bool operator != (const CStateDomain &c1, const CStateDomain &c2);

	std::vector<uint8_t> getAddress(bool prettyPrint=false);
	void setAddress(std::vector<uint8_t> adr);
	bool prepare(bool strore,bool preparehashOnly=false);
	void updateTimeStamp();
	uint64_t getTimeStamp();
	//data and status verification
	bool verifyStatus();
	bool createDirectory(std::string name, std::vector<uint8_t>& newPerspective, uint64_t& cost, std::string path = "",std::string currentDir="", bool inSandbox=false,bool hidden=false, std::shared_ptr<CAccessToken> accessToken = CAccessToken::genSysToken(), std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);
	bool removeElement(std::string name, std::vector<uint8_t>& newPerspective, uint64_t& cost, std::string path = "",std::string currentDir="", bool inSandbox=false,bool hidden=false, std::shared_ptr<CAccessToken> accessToken = CAccessToken::genSysToken(), std::shared_ptr<CSecDescriptor> secDescriptor = nullptr);
	bool updateDirectory(CDataTrieDB dir, std::vector<uint8_t>& newPerspective, uint64_t& cost, std::string path, std::string currentDir, bool inSandbox, bool hidden, std::shared_ptr<CAccessToken> accessToken = CAccessToken::genSysToken());
	//bool updateDirectory(CDataTrieDB dir, std::vector<uint8_t>& newPerspective, uint64_t& cost, std::string path, std::string currentDir, bool inSandbox, bool hidden);
	bool preparePath(std::shared_ptr<CAccessToken> accessToken,std::vector<CDataTrieDB*> &directoriesCreated,std::string &path, CDataTrieDB *currentDir, CDataTrieDB **pathEnd, uint64_t& cost, bool invalidateNodes = false , bool inSandbox=false,std::shared_ptr<CSecDescriptor> secToken=nullptr, bool inheritPermissions=true);
	//nonce
	size_t getNonce();
	bool incNonce(bool inSandBox = true);
	bool setNonce(uint64_t nonce ,bool inSandBox = true);
	bool setSecDescriptor(std::shared_ptr <CSecDescriptor> desc, bool inSandBox);
	std::shared_ptr <CSecDescriptor> getSecDescriptor();
	//balance
	bool setBalance(std::vector<uint8_t> &perspectiveID,BigInt balance, bool sandBoxMode=true);
	BigInt getBalance(bool sandBoxMode=false);

	// contract code
	bool setCode(std::vector<uint8_t> code, bool sandBoxMode=true);
	std::vector<uint8_t> getCode(bool sandBoxMode=false);


	//data
	template <typename TK,typename TV>
	bool saveValueDB(std::shared_ptr<CAccessToken> accessToken ,TK key, TV value,  eDataType::eDataType dType,std::vector<uint8_t>& newPerspective, uint64_t &cost, bool sandBoxMode=false,bool hidden=true,std::string path="",std::string currentDir="",CTrieDB* extStateTrie=nullptr,bool keepLocalDBInSyncDuringUpdate=false,bool justUpdateTheSecToken=false,  bool inheritPermissions=true, std::shared_ptr<CSecDescriptor> secDescriptor = nullptr, CTrieNode* RAWPointer=nullptr);
	//template<typename TK, typename TV>
	void propagateUpTheTrie(CTrieDB* extStateTrie, bool keepLocalDBInSyncDuringUpdate =false, bool sandBoxMode = false);
	//bool saveValueDB(valueType vType, std::string key, std::vector<uint8_t> value, CTrieDB * stateDB, bool sandBoxMode = false);
	template <typename T>
	std::vector<uint8_t> loadValueDB(std::shared_ptr<CAccessToken> accessToken,T key, eDataType::eDataType &vt, std::string path="",std::string currentDir="", const std::string & confirmedPath ="");
	bool isDirectory(std::string path);
	//std::vector<uint8_t> loadValueDB(std::vector<uint8_t> key, bool sandBoxMode = false);

	//serialization & de-serialization
	std::vector<uint8_t> getPackedData();
	static CStateDomain * instantiateStateDomain(std::vector<uint8_t> packedDataBER, std::vector<uint8_t> perspective,eBlockchainMode::eBlockchainMode blockchainMode);
	bool isSynced();
	std::vector<nibblePair> getAddressNibbles();
	std::vector<uint8_t> getStoragePerspective();
	CDataTrieDB* getStorageTrie();
	void syncPerspectiveFieldFromDB();
	bool setStoragePerspective(std::vector<uint8_t> perspective);
	CKeyChain * getKeyChain();
	bool setPerspective(std::vector<uint8_t> perspectiveID,bool refreshInnerData=true);

	std::vector<uint8_t> getPerspective();
	void setKeyChain(CKeyChain * chain);
	CStateDomain & operator = (const CStateDomain &t);
	bool validateStoragePerspective();
	bool validatePerspectives();
	void setInFlowStateDB(CTrieDB * db);
	bool exitFlow();

	void setStateDomainManager(std::shared_ptr<CStateDomainManager> sdm);

	bool enterFlow(std::shared_ptr<CStateDomainManager> mgr);
	CTrieDB * getInFlowStateDB();
	uint64_t addPreviousBodyID(std::vector<uint8_t> id, std::vector<uint8_t> perspective, bool doColdStorageCleanUp = true);
	std::vector<std::vector<uint8_t>> getPreviousBodies();
	std::vector<std::vector<uint8_t>> getPreviousPerspectives();
	bool isInFlow();
private:
	std::shared_ptr<CStateDomainManager> mStateDomainManager;
	std::vector <std::vector<uint8_t>> mPreviousBodyIDs;//  these IDs represent previous states of the StateDomain
	//these can be used to retrieve previous versions.
	//these are used also to prune data from the Cold Storage(HDD) when it's decided to be stale.
	//the state of a domain is decided to be stale once it's at a block-depth deemed to be feasibly irreversible.
	//note: these values are NOT protected by the consensus similarly to the ColdStorage Flags. 
	std::vector <std::vector<uint8_t>> mPreviousPerspectives;//same as above. 
	bool mInFlow;
	void refresh();
	mutable std::recursive_mutex mGuardian;

	//Global State Trie - BEGIN
	CTrieDB * mLocalStateDB;//representation of a global decentralized StateDB from a given Perspective
	CTrieDB * mFlowStateDB;//pointer to EXTERNAL stateDB used within The Flow. it is NOT pruned with AfterRunGuard and its state is maintained across state domains
	/*
	We've got the following state database instances:
	mStateDB within a State Domain - populated only on demand. 
	Allows for nice autonomous state domains each being in a different perspective at the same time.
	We can set perspective using  setPerspective().
	If a domain has a mFlowStateDB set it needs to be in the same Perspective.

	mFlowStateDB - a pointer to mFlowStateDB within Blockchain Manager. This Trie contains flow steps
	which might have not been commuted to cold storage yet. We use double-bufforing.mFlowStateDB within
	Transaction  Manager is used for Flow execution and might not maintain integrity at all times.
	This Trie is pruned whenever a Flow ends. And is populated on demand and only as required when a Flow starts.

	Whereas,
	mStateDB - within the Transaction  Manager and Blockchain Manager contains only the committed flows and is used in production.
	Ie.e it is used for providing data to other nodes etc. it is populated at all times.
	*/

	//Global State Trie - END

	//Local Storage Trie - BEGIN
	CDataTrieDB *mStorageDB;//individual StorageDB assigned to a StateDomain
	std::vector<uint8_t> mStoragePerspective;
	//Local Storage Trie - END

	std::shared_ptr<CIdentityToken> mTempID;
	//the local mStateDB as well mStorageDB are destroyed *AS SOON AS* they are not in use;
	//they are is recovered with current mCurrentPerspectiveID as needed
	uint64_t mLastModified;
	CKeyChain * mKeyChain; //won't be NULL only if key chain for this domain is stored locally. HD-keys won't be traversed so only root public key is checked.not serialized.
    size_t mVersion = 1;
    bool mSynced = false;
    size_t mNonce;////The nonce helps to prevent replay attacks. It needs to be checked each time state domain's data is to be modified.
    std::vector<uint8_t> mAddress;
    std::vector<uint8_t> mCodeHash; //code stored inside of the AccountStorageTrie
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	std::shared_ptr<CTools> mTools;
};

/// <summary>
/// Stores value within a StateDomain.
/// 
/// Security Notice: the function can either be set to inherit permissions from parent directory OR be instructed 
/// to use an explicit security descriptor. When an explicit security descriptor is provided, it is to be used for any directories
/// created during the process i.e. during path's preparation and for the final leaf-node as well. When permissions' inheritance enabled,
/// it would be similar, but with the security descriptor copied and inherited down the inheritance tree.
/// 
/// 
/// Warning::outside StateDB needs to reflect the new returned  perspective if it wants to stay in synch.
/// 
/// Important: stateDB points to the global decentralized State Database.
/// There are many possible states of the decentralized State Database.
/// 
/// We can change through the available states of the decentralized state database can by switching the
/// State DB's root node (changeRoot() public method).
/// 
/// Besides the global State Abase each State Domain has a StorageDB of its own.
/// When a change to a State Domain occurs(to its storage database for instance, then
///  the state domain's mStorageRootHash hash gets changed. This results in a change to 
/// the results returned by stateDomain's getPackedData() function.Packed State Domains constitute Leaves of the 
/// the global State Domain's Trie. If any State Domain gets changed it results in a change to the global StateDabase's root hash.
/// 
/// Thus, previous states of a State Domain can be restored by changing the root node of the Global State Domain,
/// and looking up a state domain of a particular ID. (the ID of the state domain is the same across all its versions).
/// 
/// The keepLocalDBInSyncDuringUpdate is used to indicate whether updates to the mLocalStateDB are to be done.
/// During normal operation, this shouldn't be needed. If chosen, the state of LocalStatDB will be maintained across the update.
/// Then the localDB will be wiped, before its perspective is assured to be in sync with the local-db.
/// </summary>
/// 
/// extStateTrie points to an external StateDB which also needs to be refreshed.
template<typename TK, typename TV>
inline bool CStateDomain::saveValueDB(std::shared_ptr<CAccessToken> accessToken, TK key, TV value, eDataType::eDataType dType, std::vector<uint8_t>& newPerspective, uint64_t &cost, bool sandBoxMode, bool hidden, std::string path, std::string currentDir, CTrieDB * extStateTrie, bool keepLocalDBInSyncDuringUpdate, bool justUpdateTheSecToken, bool inheritPermissions, std::shared_ptr<CSecDescriptor> secDescriptor, CTrieNode* RAWPointer)
{
	// ---> BEGIN FIX <---
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// ---> END FIX <-

	if (accessToken)
		accessToken->clearState();
	//Note: if any problems with this arise keep in mind that we've added the keepLocalDBInSyncDuringUpdate conditional parameter.
	//ENABLE it if any problems with state occur.
	assertGN(mLocalStateDB != NULL);
	if (keepLocalDBInSyncDuringUpdate)
	 assertGN(mFlowStateDB != nullptr);//well then we need at least to have access to the mFlowStateDB for proper state update and resulting perspective.
	AfterRunGuard guardStateDB = AfterRunGuard(&(this->mLocalStateDB));
	std::vector<CTrieNode*> leaves;
	std::vector<uint8_t> prevAdr = getAddress();
	if (!sandBoxMode && mStateDomainManager != nullptr)
	{
		mStateDomainManager->getDB()->registerStateDomainsInitialHash(getAddress(), getHash());//will succeed only once per flow
	}

	
	if (!validatePerspectives() || mWasAlreadyModifiedInFlow || (keepLocalDBInSyncDuringUpdate&&mLocalStateDB->isRootEmptyHS()))// look at the '2nd important note below' for explanation of isEmptyHs() usage here 
	{
		//during the Flow; mStateDB after the first transaction is cleared by the AfterRunGuard.(trie wiped)
		//state changes occurring within the Flow are contained only within the HotStorage(RAM),
		//thus its state cannot be recovered with a call to setPerspective (which uses ColdStorage to retrieve data) to match the state of FlowDB

		/*
		during the second transaction; the mStateDB cannot be restored fully by adding nodes from flowDB
		as the flowDB's state does NOT exist in cold storage and thus the trie's root and its hashes are not sufficient to restore
		the full inter-data field within the leaf-node.
		ways to go:
		1) do not use inter-data for calculation of the node's hash
		=> 2)*DONE* implement a full trie-db copy function (takes time and uses a lot of RAM), time grows linearly with the flowDB's size
		3) during the flow only flowDB will be in use while mStateDB will serve for solid state data retrieval.
		4) remove flow-db as a whole and stick only with one DB
		*/

		/*
		SECOND important Note: mLocalDB is pruned each time. all that is left within it after the last flow-transaction
		processing is the EXACT root. (including all the bytes so the getPerspective() produces a valid perspective)
		and now an extreme 'BUT'.. if it happens that the same StateDB is part of a transaction within the SAME flow
		which was not committed YET; .. then the validatePerspectives() would return true BUT.. we wouldn't be able
		to sync the two databases WITHOUT performing a PARTIAL mem-copy ( partial in the sense of only the nodes located in RAM already )
		 from mFlowStateDB to mLocalStateDB;

		We can use the following methods to detect the situation:
		1) check if we can revert perspective from cold storage of the root node
		2) check with a call to the TrieDB's isRootEmptyHS()  to check if its nodes are locally available
		3) check is the StateDomain's flag mWasAlreadyModifiedInFlow is set

		3rd would be best for performance; for now we stick to the 1st option as a compromise to ensure good maintainability of the code-base.
		*/

		if (keepLocalDBInSyncDuringUpdate &&mFlowStateDB != NULL)
		{	static_cast<CTrieBranchNode*>(mLocalStateDB->getRoot())->clear();
			*mLocalStateDB = *mFlowStateDB;

			//mFlowStateDB->copyTo(mLocalStateDB);
			mLocalStateDB->setFlowStateDB(false);
		}
		else if(mFlowStateDB==nullptr)
		{
		 assertGN(false);//we cannot modify such a domain. it contains a root node which does not point to valid
			//nodes within the ColdStorage... AND.. the domain is not within a Flow so we do not have from where to restore the nodes from.
			//we could just return false here.
		}
	}
	if(keepLocalDBInSyncDuringUpdate)
		if (!validatePerspectives())
		{
			return false;
		}
 assertGN(mAddress.size() > 0);
	bool result = false;
	if (mInFlow)
		mWasAlreadyModifiedInFlow = true;

	mStoragePerspective = mStorageDB->getPerspective();
	
	std::shared_ptr<CSecDescriptor> secDesc;
	std::vector<CDataTrieDB*> modifiedDirectories;
	CTrieNode* endingObject = nullptr;// file OR directory
	//whereas
	CDataTrieDB* endingDir = mStorageDB;// is always a directory

	if (path.size() > 0)
	{
		//First, try to traverse the entire path and see into the final object's security descriptor.
		//If the traversal succeeds, there's no need to prepare a new path.
		//Authorize against the final object (file or directory).

		if (!mTools->isAbsolutePath(path))
			path = currentDir + path;

		if (goToPath(path, &endingObject, mStorageDB , true))// the ending object MIGHT NOT represent the desired element, 
		{//since it MIGHT NOT exist as of now.

			secDesc = endingObject->getSecDescriptor();

			if (secDesc && accessToken)// check for explicit access token, authorize against it.
			{
				bool updateSecDesc = false;
				assertGN(mStateDomainManager != nullptr);
				if (!secDesc->getIsAllowed(accessToken, mStateDomainManager, updateSecDesc))
				{
					accessToken->setRecentOperationNotAuthorized();
					return false;
				}
			}
		}

		if (path.size() != 1)
		{
			

			//this would compare against security descriptors in each nested directory.
			//PathEnd - it's *always* a directory.
			result = preparePath(accessToken, modifiedDirectories, path, mStorageDB, &endingDir, cost, true, sandBoxMode, secDescriptor, inheritPermissions);
			if (!result)
				return false;
		}
		else
			endingDir = mStorageDB;
	}

	//Security - BEGIN
	
	//Access Check - BEGIN
	secDesc = endingDir->getSecDescriptor();
	bool updateSecDesc = false;
	if (endingDir && !endingObject && secDesc && accessToken && !endingDir->getSecDescriptor()->getIsAllowed(accessToken, mStateDomainManager, updateSecDesc))
	{//authorize against the furthest directory
		accessToken->setRecentOperationNotAuthorized();
		return false;
	}

	//if (updateSecDesc)
	//{//verify if this would work from here
	//	updateObjSecDesc(endingDir, endingDir->getPermToken(), newPerspective, cost, path, currentDir, accessToken);//do we need additional checks? outside of saveValueDB()? for calls leading to it? Ye, here acces to prior dirs is checked.
	//}

	
	//Access Check - END

	//Apply Security Descriptor - BEGIN
	if (secDescriptor)
	{
		//simply use it for the child leaf-node
	}
	else if (inheritPermissions)
	{
		//attempt to inherit permissions from the parent object
		std::shared_ptr<CSecDescriptor> parentToken= endingDir->getSecDescriptor();
		if (parentToken)
		{
			secDescriptor = std::make_shared<CSecDescriptor>(parentToken);


			//Extensions - BEGIN
			std::shared_ptr<CPollFileElem> pollExt = secDescriptor->getPollExt();
			PollElemFlags flags;
			if(pollExt)
			{
				std::shared_ptr<CPollFileElem> pollExtNew = std::make_shared< CPollFileElem>(*(secDescriptor->getPollExt()));//create a copy
				//clear state - we do not want votes etc. to be inherited
				pollExtNew->resetPollsState();
				parentToken->setPollExt(pollExtNew);
				std::vector<std::shared_ptr<CPollElem>>  polls = pollExtNew->getPolls();
				for (uint64_t i = 0; i < polls.size(); i++)
				{
					flags  = polls[i]->getFlags();
					flags.setWasInherited(true);
					polls[i]->setFlags(flags);
				}
			}
			//Extensions - END

		}
	}
	//Apply Security Descriptor - END
	//Security - END


	if (!justUpdateTheSecToken)
		result = endingDir->saveValue(key, value, dType, sandBoxMode, hidden, cost, secDescriptor);
	else
		result = endingDir->updateSecToken(key, RAWPointer? RAWPointer:reinterpret_cast<CTrieNode *>(&value), sandBoxMode);//RAW pointer is needed because Virtual Function Pointer table gets stripped for anything specialized and derived from CTrieNoder when passed to this templated method
	
	if (sandBoxMode)
		endingDir->invalidatePath();
	else
		endingDir->savePathToSS(mBlockchainMode,mStorageDB->getSSPrefix(),true);

	
	if (result)
	{
		propagateUpTheTrie(extStateTrie, keepLocalDBInSyncDuringUpdate , sandBoxMode);
	}

	if (keepLocalDBInSyncDuringUpdate && !validatePerspectives())
	{
	    assertGN(false);
		return false;
	}
	std::vector<uint8_t> newAdr = getAddress();
    assertGN(std::memcmp(newAdr.data(), prevAdr.data(), 32) == 0);

	if(keepLocalDBInSyncDuringUpdate)//both of these would be the same anyway.
	newPerspective = mLocalStateDB->getPerspective();
	else
	{
		newPerspective = mFlowStateDB->getPerspective();
	}
	
	invalidatePath();
	return result;
}


inline void CStateDomain::propagateUpTheTrie(CTrieDB* extStateTrie, bool keepLocalDBInSyncDuringUpdate , bool sandBoxMode)
{
	//assertGN(extStateTrie);
	mStoragePerspective = mStorageDB->getPerspective();
	assertGN(getAddress().size() > 0);

	//propagate the change in StorageDB higher to the StateDB (state-machine-global DB) itself.
	//most important calls within: postModificationSync()
	//proceeded by invalidatePath or savePathToSS();

	//IMPORTANT: if there is no full ID-coverage in local StateDommain's mStateDB
	//then we need to create a new instance of the state domain. WE CANNOT put there
	//a pointer to THIS; as this would cause the following deadlock:
	//AfterRunGuard=>PruneTrieBelow=>destructor of THIS containing StateDB with THIS as leaf.

	//assert that the StateDomain exists if not => add it
	std::vector<CTrieDB*> stateTriesToMod;
	if (keepLocalDBInSyncDuringUpdate)
		stateTriesToMod.push_back(mLocalStateDB);
	if (mFlowStateDB != nullptr)
		stateTriesToMod.push_back(mFlowStateDB);
	if (extStateTrie != nullptr)
		stateTriesToMod.push_back(extStateTrie);

	CTrieNode* node;
	bool disablePerspectiveUpdate = true;

	for (int i = 0; i < stateTriesToMod.size(); i++)
	{
		node = nullptr;
		disablePerspectiveUpdate = true;

		//DISABLE integrity checks AND perspective update; these are to be changed anyway.(more info below and within the function)
		if (!(node = stateTriesToMod[i]->findNodeByFullID(mAddress, true, disablePerspectiveUpdate)))//disable perspective update, which would case the localStateDB to be reverted to previous state. it is going to be altered/updated in the below call anyway
		{
			assertGN(stateTriesToMod[i]->addNodeExt(mTools->bytesToNibbles(mAddress), this, sandBoxMode) == CTrieDB::eResult::added);
		}
		else
		{
			//if (node != this)//do NOT update the trie if it is the one which contains THIS
			//WROOOOONG/// we need to modify the Trie as its hash has just changed.
			//WARNING; the copy constructor will be executed over the object owning the CURRENT FUNCTION
			assertGN(stateTriesToMod[i]->postModificationSync(mTools->bytesToNibbles(mAddress), this, sandBoxMode));
			//Notice: call to the above function does the actual heavy-lifting (storing data to HDD for each trie-node
			//down to the HDD level,first re-packing all the nodes, etc).
		}
	}
}






template <typename T>
std::vector<uint8_t>  CStateDomain::loadValueDB(std::shared_ptr<CAccessToken> accessToken, T key,  eDataType::eDataType &vt, std::string path, std::string currentDir, const std::string& confirmedPath)
{

	// --- BEGIN FIX ---
   // Lock the internal storage database for this domain
	std::lock_guard<ExclusiveWorkerMutex> storageLock(mStorageDB->mGuardian);
	// --- END FIX ---

	if (accessToken == nullptr)
	{
		return std::vector<uint8_t>();
	}

	if (currentDir.size() == 0 || currentDir[0]=='0')
	{
		currentDir = "/";
	}
 assertGN(mStorageDB != NULL);
	//local decentralized StateDB copy initialization and its AfterRunGuard destructor END

	CDataTrieDB *currentDB = mStorageDB;
	
	if (path.size() > 0)
	{
		std::vector<std::string> dirs;
		std::string dirPart;
		CTrieNode *n = nullptr;

		if (path.size() > 0)
		{
			if (!mTools->isAbsolutePath(path))
				path = currentDir + path;
		}

		if (!goToPath(path, &n) || n == nullptr)
		{
			vt = eDataType::noData;
			return std::vector<uint8_t>();
		}

	

		if (n->isDirectory())
			currentDB = static_cast<CDataTrieDB*>(n);
		
	}

	std::vector<uint8_t> toRet = currentDB->loadValue(key, vt, confirmedPath);
	return toRet;
}

#endif
