#ifndef TRIE_NODE_H
#define TRIE_NODE_H

#include <vector>
#include <array>
#include "ExclusiveWorkerMutex.hpp"
#include "CryptoFactory.h"
class CDataTrieDB;
class CSecDescriptor;
class CFileMetaData;
class CStateDomain;
class HotStorageProperties{//i.e. RAM storage properties
private:
	uint8_t mTriggerType;
	bool mForDeletion : 1;//to be deleted from RAM
	bool mInvalid : 1; //invalid transaction OR marked as invalid by CValidator
	bool mProcessed : 1;
	bool mDoNotIncludeInChain : 1;
	bool mRequiresTrigger : 1;
	
public:
	uint8_t getTriggerType();
	void setTriggerType(uint8_t val);
	HotStorageProperties& operator = (const HotStorageProperties &t);
	void clear();
	HotStorageProperties(const HotStorageProperties& sibling);
	 HotStorageProperties();
	 void markForDeletion(bool state=true);
	 void markAsInvalid(bool state = true);
	 void markAsProcessed(bool state = true);
	 bool isMarkedForDeletion();
	 bool isMarkedAsInvalid();
	 bool isTriggerRequired();
	 void markRequiresTrigger(bool state = true);
	 bool isMarkedAsProcessed();
	 bool getDoNotIncludeInChain();
	 void markDoNotIncludeInChain(bool state = true);
	 

};

class ColdStorageProperties {//i.e. blockchain-decentralized storage flags. these field is NOT signed
	//these fields are malleable ie.e protected by PoW only. and can be modified by miners. Block originator has no control over these.
	//ie.e these fields are NOT authenticated
	//BUT.. they are protected by the consensus; thus their values need to match on all full-nodes within the network
	//i.e. the value of a coldStorage property *AFFECTS* the node's hash this it affects the Perspective of a Trie
	//IF present within one.
private:
	//EXTREME WARNING: *WE CANNOT* exceed 1 byte here so to ensure inter-portability between versions.
	//additional code for this would be required
	bool mInvalid : 1; //invalid transaction OR marked as invalid by CValidator
	bool mSacrifice : 1;
	bool mDebugThis : 1;
	bool mReserved : 5;

public:
	ColdStorageProperties& operator = (const ColdStorageProperties &t);

	
	void clear();
	ColdStorageProperties(const ColdStorageProperties& sibling);
	ColdStorageProperties();
	void markAsInvalid(bool state = true);

	bool isMarkedAsInvalid();
	bool isASacrifice();
	bool idDebugThis();
	void markAsDebugThis(bool state = true);

	void markAsSacrifice(bool state);
};

class CTrieNode
{
public:
	mutable ExclusiveWorkerMutex mGuardian;

protected:
	uint8_t mNodeVersion=0;
    bool mRegisteredWithinTrie;
    size_t mType=0; // 1 - extension, 2-branch, 3-leaf
    size_t mSubType=254;//1-Account // value deserialized from within the mainRAWData
    std::vector<nibblePair> mTempFullPath;
	int getInterData(std::vector<uint8_t> & interData);

	uint64_t mSerializedSize = 0;// NOT serialized

	void setType(size_t type);
    bool mIsPrepared = false;


public:

	uint64_t getSerialziedSize();
	void setSerializedSize(uint64_t size);

	/// <summary>
	/// Clear all relationships. FOr use with copies of nodes for external use.
	/// IMPORTANT: call this node ONLY on external copies. NEVER on in-Trie objects.
	/// </summary>
	void sanitize();
	uint8_t getVersion();
	CTrieNode(eNodeType::eNodeType type = eNodeType::leafNode, eNodeSubType::eNodeSubType subType= eNodeSubType::RAW);
	bool prepareCachedPermToken();
	std::shared_ptr<CSecDescriptor> getSecDescriptor(bool onlyExplicit=false);//a CACHED token IF available
	bool setSecDescriptor(std::shared_ptr<CSecDescriptor> token);
	std::string getPath(bool includeFileName=false);
	bool isDirectory();
	void initFields();
	CTrieNode& operator = (const CTrieNode &t);
	void unregister();
	virtual ~CTrieNode()= default;//IMPORTANT! removing an object by base pointer without virtual destructor is
	//an undefined behaviour.
	HotStorageProperties HotProperties;
//the basic TrieNode does NOT contain 	ColdStorageProperties as this would produce too much storage overhead to the
	//decentralised database. we use 	ColdStorageProperties only for transactions and verifiables.
	CTrieNode(const CTrieNode& sibling);
	bool isRegisteredWithinTrie();
	bool markAsRegistered();
	void setSubType(size_t subtypeID);
	size_t getSubType();
	void markAsPrepared(bool prepared = true);
	size_t getTransmissionType();
    bool mHasLeastSignificantNibble = true;// Notice: there's no need for mHasMostSignificantNibble since niblles (half-bytes) are always shifted to the left
										   // when converting nibbles to bytes.
	bool isNodePrepared();
	void invalidateNode();
	void invalidatePath();
	CDataTrieDB *getParentDir();
	bool savePathToSS(eBlockchainMode::eBlockchainMode blockchainMode,std::string prefix,bool invalidateNodes);
    std::vector<uint8_t> mInterData; //contains partial-ID of the inserted leaf-node
    std::vector<uint8_t> mMainRAWData; //contains main data relevant to derived node type
	bool unpackData(const std::vector<uint8_t> &data);

	CTrieNode(CTrieNode * parent);
	CTrieNode(const std::vector<uint8_t> &packedData);

	std::vector<uint8_t>  getPackedData(bool forThePurposeOfHashCalculation=false);
	bool validateIntegrity(std::vector<uint8_t> hash);
	void clearInterData();
	CTrieNode * copy(bool markAsPrepared=false);
	std::vector<uint8_t> getHash();

	//std::vector<uint8_t> getFullID(CTrieNode * node, bool &hasRightMostNib);
	//it's not possible to get full id based on a node since there might be multiple roots containing the same node

	CTrieNode(std::vector<uint8_t> BER_data, CTrieNode * parent);

	std::string getAccessRightsString();
	std::vector<nibblePair> getFullID(std::vector<nibblePair> path= std::vector<nibblePair>(), CTrieNode * pointer=nullptr);
	void setPointerToPointerFromParent(CTrieNode ** pointer);
	bool isStateDomain();

	CStateDomain* getDomain();

	std::vector<uint8_t> getNearestOwner();

	size_t getType();
	std::vector<uint8_t> getRawData();
	uint64_t getRawDataSize();
	bool setPartialID(std::vector<nibblePair> partialID);
	CTrieNode* replaceNode(size_t type);
	bool setRawData(std::vector<uint8_t> data);
	bool  setTempFullPath(std::vector<nibblePair> data);
	std::vector<nibblePair>  getTempFullPath();
	bool clearTempData();

	std::vector<nibblePair> getPartialID();
	bool updatePartialID(std::vector<uint8_t> fullID, const std::vector<nibblePair> partialPath = std::vector<nibblePair>());
	bool updatePartialIDK(std::vector<uint8_t> fullID,const std::vector<nibblePair> partialPath= std::vector<nibblePair>());

	bool setParent(CTrieNode * parent);
	CTrieNode * getParent();
	CTrieNode* getParentDomain();
	bool setParentDomain(CTrieNode* parentDomain);
	CTrieNode **mPointerToPointerFromParent = 0;
	size_t getPackedSize();
	std::string getName();
	std::string getExtension();
	void setNamePrefix(std::string name);
	uint64_t getMainRAWDataSize();
	std::string mName;//an optional member. serialized (or not) at the end.
	std::shared_ptr<CSecDescriptor> mSecDescriptor;
	
	std::shared_ptr<CFileMetaData> mMetaData;//immutable; can be set only once by owner of the object; no future owner can alter this property.


    std::shared_ptr<CFileMetaData> getMetaData();
	void setMetaData(std::shared_ptr<CFileMetaData> data);
	bool getHasMetaData();


private:

//the grantEx #GridScript method for more
	//std::recursive_mutex mGuardian;
	

	
   // std::vector<uint8_t> mRandomDat;
    CTrieNode * mParent;//only root directory is to contain pointer to parent state-domain.
	CTrieNode* mParentDomain ;//for backwards compatibility with all the existing code.
							 //todo: make all the algorithms account for the possibility of mParent be a StateDomain.
							 //currently these algorithms expect a nullptr parent  to be right at the home directory.


   // std::shared_ptr<CCryptoFactory> mCrypto;
protected:
	
	std::vector<uint8_t> mPreComputedHash;



	
};

#endif
