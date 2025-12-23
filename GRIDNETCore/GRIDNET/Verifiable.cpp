#include "Verifiable.h"
#include "TrieLeafNode.h"
#include "BlockchainManager.h"
#include "GRIDNET.h"
#include "StateDomain.h"
void CVerifiable::basicInit()
{
	mIsArmed = false;
	mErgLimit = 0;
	mErgPrice = 0;
	mType = 3;
	mSubType = 4;
	genID();
	mReceivedAt = std::time(0);
	mVersion = 1;
	mVerifiableType = eVerifiableType::eVerifiableType::minerReward;
	invalidateNode();
}

std::vector<uint8_t> CVerifiable::getPubKey()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mPubKey;
}

void CVerifiable::setPubKey(std::vector<uint8_t> pubKey)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mPubKey = pubKey;
	invalidateNode();
}

std::vector<uint8_t> CVerifiable::getSig()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mSignature;
}

void CVerifiable::setSignature(std::vector<uint8_t> sig)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mSignature = sig;
	invalidateNode();
}

CVerifiable & CVerifiable::operator=(const CVerifiable & t)
{
	mAffectedDomains = t.mAffectedDomains;
	mVersion = t.mVersion;
	mGUID = t.mGUID;
	mErgPrice = t.mErgPrice;
	mErgLimit = t.mErgLimit;
	mReceivedAt = t.mReceivedAt;
	mVerifiableType = t.mVerifiableType;
	mProof = t.mProof;
	ColdProperties = t.ColdProperties;
	mPubKey = t.mPubKey;
	mSignature = t.mSignature;
	CTrieLeafNode::operator =(t);
	return *this;

}

uint64_t CVerifiable::getErgPrice()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mErgPrice;
}

BigInt CVerifiable::getErgLimit()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mErgLimit;
}

void CVerifiable::setERGPrice(uint64_t price)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mErgPrice = price;
	invalidateNode();
}

void CVerifiable::setERGLimit(BigInt limit)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mErgLimit = limit;
	invalidateNode();
}



size_t CVerifiable::getReceivedAt()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mReceivedAt;
}

void CVerifiable::setReceivedAt(size_t time)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mReceivedAt = time;
	invalidateNode();
}

bool CVerifiable::isVerified()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return !ColdProperties.isMarkedAsInvalid();
}

void CVerifiable::markAsVerified()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	ColdProperties.markAsInvalid(false);
	invalidateNode();
}

/// <summary>
/// Identifier is based on the provided proof.
/// This is so we can detect if there's an attempt of a varifiable-based 'double spend' just by looking at the block and its list of VefribialeIDs.
/// </summary>
/// <returns></returns>
bool CVerifiable::genID()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mProof.size() == 0)
		mGUID = CTools::getTools()->genRandomVector(32);
	else
	mGUID = CCryptoFactory::getInstance()->getSHA2_256Vec(mProof);
	return true;
}

std::vector<uint8_t> CVerifiable::getGUID()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mGUID;
}

bool CVerifiable::prepare(bool store,bool includeSig)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//subtype 1 for StateDomains , subtype 2 for transactions , subtype 3 for receipts, subtype 4 for Verifiables
 assertGN(getType() == 3 && getSubType() == 4, "oh but, ..I'm not that kind of a node!");

	std::vector<uint8_t> packedData;
	std::shared_ptr<CTools> tools = CTools::getInstance();
		std::vector<uint8_t> dat;
		Botan::DER_Encoder enc1 = Botan::DER_Encoder().start_cons(Botan::ASN1_Tag::SEQUENCE)
			.encode(mSubType)//subtype
			.encode(mVersion).
			start_cons(Botan::ASN1_Tag::SEQUENCE)
			.encode(mGUID, Botan::ASN1_Tag::OCTET_STRING)
			.encode(tools->BigIntToBytes(mErgLimit), Botan::ASN1_Tag::OCTET_STRING)
			.encode(static_cast<size_t>(mErgPrice))
			.encode(static_cast<size_t>(mVerifiableType))

			.encode(mProof, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mPubKey, Botan::ASN1_Tag::OCTET_STRING);
			if (includeSig)
			enc1 = enc1.encode(mSignature, Botan::ASN1_Tag::OCTET_STRING);
		else
			enc1 = enc1.encode_null();

			enc1.start_cons((Botan::ASN1_Tag::SEQUENCE));

		

		for (int i = 0; i < mAffectedDomains.size(); i++)
		{
			enc1 = enc1.start_cons((Botan::ASN1_Tag::SEQUENCE));
			enc1 = enc1.encode(mAffectedDomains[i]->getAddress(), Botan::ASN1_Tag::OCTET_STRING);
			BigSInt balChange = mAffectedDomains[i]->getPendingPreTaxBalanceChange();
			//std::vector<uint8_t> nrBytes;
			//size_t s = tools->getSignificantBytes(balChange);
			///nrBytes.resize(s);
			//std::memcpy(nrBytes.data(), &balChange, s);
			//std::memcpy(&balChange,nrBytes.data(), s);
			enc1 = enc1.encode(tools->BigSIntToBytes(balChange), Botan::ASN1_Tag::OCTET_STRING);//todo: Botan has truble encoding negative numbers. improve this.
			enc1 = enc1.end_cons();
		}
		enc1 = enc1.end_cons().end_cons().end_cons();

		packedData = enc1.get_contents_unlocked();
		((CTrieLeafNode*)this)->setRawData(packedData);
		mIsPrepared = true;
	

	return true;
}

/// <summary>
/// Sets the proof-of-fraud BER-encoded proof.
/// Takes two header-bodies of the same Blockchain-height generated by same leader.
/// Data-block headers do NOT contain publicKey of the leader. Without public key it wouldn't be possible to
/// verify whether by which leader the fraud was comited. 
/// Important: in order to aid this AND and eliminate SPAM accross the network we require an on-spotter to be in posession
/// of a coresponding key-block. The key-block proofs PoW and provided the public key allowing for verificaiton of the two data-block headers.
/// Only the header of the key-block is included. Even though another node might be able to retireve the key-block when seeing data-blocks we do not want spam.
/// </summary>
/// <param name="bh1Body"></param>
/// <param name="bh2Body"></param>
/// <returns></returns>
bool CVerifiable::setPoFProof(std::vector<uint8_t> kbBody, std::vector<uint8_t> bh1Body, std::vector<uint8_t> bh2Body)
{
	if (kbBody.size() == 0 || bh1Body.size() == 0 || bh2Body.size() == 0)
		return false;

	try {
		std::vector<uint8_t> proofBytes = Botan::DER_Encoder().start_cons(Botan::ASN1_Tag::SEQUENCE)
			.encode(kbBody, Botan::ASN1_Tag::BIT_STRING)
			.encode(bh1Body, Botan::ASN1_Tag::BIT_STRING)
			.encode(bh2Body, Botan::ASN1_Tag::BIT_STRING).end_cons().get_contents_unlocked();
		mProof = proofBytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool CVerifiable::getPoFProof(std::shared_ptr<CBlockHeader> &kbHeader, std::shared_ptr<CBlockHeader> &bh1, std::shared_ptr<CBlockHeader> &bh2, eBlockchainMode::eBlockchainMode blockchainMode)
{
	std::vector<uint8_t> kbBody, bh1Body, bh2Body;
	if (!getPoFProof(kbBody, bh1Body, bh2Body))
		return false;

	if (kbBody.size() == 0 || bh1Body.size() == 0 || bh2Body.size() == 0)
		return false;

	CBlockHeader::eBlockHeaderInstantiationResult hir;
	std::string err;
	kbHeader = CBlockHeader::instantiate(kbBody, hir, err, false, blockchainMode);
	if (kbHeader == nullptr)
		return false;

	bh1 = CBlockHeader::instantiate(bh1Body, hir, err, false, blockchainMode);
	if (bh1 == nullptr)
		return false;

	bh2 = CBlockHeader::instantiate(bh2Body, hir, err, false, blockchainMode);
	if (bh2 == nullptr)
		return false;
	return true;
}

/// <summary>
/// Retrieved the BER-encoded proof-of-fraud from within the Verifiable.
/// Returns bodies of the the headers.
/// Returns FALSE on failure.
/// </summary>
/// <param name="bh1Body"></param>
/// <param name="bh2Body"></param>
/// <returns></returns>
bool CVerifiable::getPoFProof(std::vector<uint8_t> &kbBody, std::vector<uint8_t> &bh1Body, std::vector<uint8_t> &bh2Body)
{
	try {
		if (mProof.size() == 0)
			return false;

		Botan::BER_Decoder(mProof).start_cons(Botan::ASN1_Tag::SEQUENCE).
			decode(kbBody, Botan::ASN1_Tag::BIT_STRING).
			decode(bh1Body, Botan::ASN1_Tag::BIT_STRING).
			decode(bh2Body, Botan::ASN1_Tag::BIT_STRING);
		if (bh1Body.size() > 0 && bh2Body.size() > 0)//todo: improve validation
			return true;
		else
			return false;
	}
	catch (...)
	{
		return false;
	}
}

CVerifiable * CVerifiable::instantiateVerifiable(std::vector<uint8_t> serializedData, eBlockchainMode::eBlockchainMode mode)
{
	
	CTrieNode * leaf = CTools::getTools()->getTools()->nodeFromBytes(serializedData, mode);
 assertGN(leaf != NULL);

	return static_cast<CVerifiable*> (leaf);
}

CVerifiable * CVerifiable::genNode(CTrieNode ** baseDataNode, bool useTestStorageDB)
{
	std::shared_ptr<CTools> tools = CTools::getInstance();
	if (*baseDataNode == NULL)
		return NULL;
 assertGN((*(baseDataNode))->isRegisteredWithinTrie() == false);
	CVerifiable *ver = new CVerifiable();
	std::vector<uint8_t> temp;
	size_t subT = 0;
	try {
		
	 assertGN((*baseDataNode)->getType() == eNodeType::leafNode);
	ver->mName = (*baseDataNode)->mName;
		Botan::BER_Decoder  dec = Botan::BER_Decoder((*baseDataNode)->getRawData()).start_cons(Botan::ASN1_Tag::SEQUENCE).
			decode(subT).
			decode(ver->mVersion);

	 assertGN(subT == 4);//ensure it's an receipt;

		Botan::BER_Object obj;

		if (ver->mVersion == 1)
		{
			ver->mReceivedAt = 0;
			size_t result = 0;
			std::vector<uint8_t> adr;
			obj = dec.get_next_object();
		 assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);

			Botan::BER_Decoder  dec2 = Botan::BER_Decoder(obj.value);
			dec2.decode(ver->mGUID, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			ver->mErgLimit = tools->BytesToBigInt(temp);
			dec2.decode(ver->mErgPrice);
			dec2.decode(result);
			ver->mVerifiableType = static_cast<eVerifiableType::eVerifiableType>(result);

			dec2.decode(ver->mProof, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(ver->mPubKey, Botan::ASN1_Tag::OCTET_STRING);
			obj = dec2.get_next_object();

			if (obj.type_tag == Botan::ASN1_Tag::OCTET_STRING)
			{
				ver->mSignature = Botan::unlock(obj.value);
			}
			//affected domain
			obj = dec2.get_next_object();
			Botan::BER_Decoder  dec3 = Botan::BER_Decoder(obj.value);
		 assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);//container of affected state-domains
			std::vector<uint8_t> domainID;
			BigSInt  pendingBalanceChangeS=0;
			std::vector<uint8_t> temp;
			while(dec3.more_items())
			{
				obj = dec3.get_next_object();
			 assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);
				Botan::BER_Decoder  dec4 = Botan::BER_Decoder(obj.value);//single affected state-domain
				dec4.decode(domainID, Botan::ASN1_Tag::OCTET_STRING);
				
				dec4.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				pendingBalanceChangeS = tools->BytesToBigSInt(temp);
				//nrBytes.resize(8);
				//pendingBalanceChangeS = *(reinterpret_cast<int64_t*>(nrBytes.data()));//todo: improve this. Botan has trouble encoding negative numbers.
				//pendingBalanceChangeS = bi;
				ver->addBalanceChange(domainID, pendingBalanceChangeS);
			}
		 assertGN(!dec.more_items());
		}
		//common part across all derived node types
		ver->mInterData.resize((*baseDataNode)->mInterData.size());
		ver->mMainRAWData.resize((*baseDataNode)->mMainRAWData.size());

		std::memcpy(ver->mInterData.data(), (*baseDataNode)->mInterData.data(), (*baseDataNode)->mInterData.size());
		std::memcpy(ver->mMainRAWData.data(), (*baseDataNode)->mMainRAWData.data(), (*baseDataNode)->mMainRAWData.size());
		ver->mHasLeastSignificantNibble = (*baseDataNode)->mHasLeastSignificantNibble;


		if (baseDataNode != NULL)
		{
			if ((*baseDataNode)->mPointerToPointerFromParent != NULL)
				(*(*baseDataNode)->mPointerToPointerFromParent) = NULL;
			delete *baseDataNode;
		}
		(*baseDataNode) = NULL;
		return ver;
	}
	catch (const std::exception& e)
	{
		if ((*baseDataNode) != NULL)
		{
			delete (*baseDataNode);
			(*baseDataNode) = NULL;
		}
	 assertGN(false);

	}

	return NULL;
}

CVerifiable::CVerifiable(const CVerifiable & sibling) :CTrieLeafNode(sibling)
{
	mIsArmed = sibling.mIsArmed;
	mAffectedDomains = sibling.mAffectedDomains;
	mVersion = sibling.mVersion;
	mGUID = sibling.mGUID;
	mVerifiableType = sibling.mVerifiableType;
	mProof = sibling.mProof;
	mErgPrice = sibling.mErgPrice;
	mErgLimit = sibling.mErgLimit;
	mReceivedAt = sibling.mReceivedAt;
	ColdProperties = sibling.ColdProperties;
	mPubKey = sibling.mPubKey;
	mSignature = sibling.mSignature;
	
}

eVerifiableType::eVerifiableType CVerifiable::getVerifiableType()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mVerifiableType;
}

/// <summary>
/// Returns a seuqence of bytes constituting a Proof.
/// These bytes can be interpeted differently based on the Verifiable's type.
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CVerifiable::getProof()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mProof;
}



std::vector<uint8_t> CVerifiable::getPackedData(bool includeSig)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if(mGUID.size()!=32)//WARNING: DO *NOT* modify the GUID of a variable at this stage.//most likely the receipt has already been generated!
 assertGN(genID());
 assertGN(prepare(true, includeSig));
	return ((CTrieLeafNode*)this)->getPackedData();
}
bool CVerifiable::verifySignature(std::vector<uint8_t> publicKey)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mSignature.size() == 0)
		return false;
	if (mPubKey.size() != 32)
		mPubKey = publicKey;
	if (mPubKey.size() != 32)
		return false;

	std::vector<uint8_t> signature = mSignature;
	bool valid = false;
	prepare(false, false);//do not include signature into serialized data
	std::shared_ptr<CCryptoFactory>  cf = CCryptoFactory::getInstance();
	return cf->verifySignature(signature, ((CTrieLeafNode*)this)->getRawData(), mPubKey);
}

bool CVerifiable::sign(Botan::secure_vector<uint8_t> privKey)
{
 assertGN(privKey.size() == 32);
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mPubKey = CCryptoFactory::getInstance()->getPubFromPriv(privKey);//the pub-key itself will be signed 
		privKey = CTools::getTools()->BERVector(privKey);
	//get data with empty sig field
 assertGN(prepare(true, false));


	std::vector<uint8_t> toSign = mMainRAWData;
	std::vector<uint8_t> sigBytes =
		CCryptoFactory::getInstance()->signData(toSign, privKey);

	//fill in the sig-field
	mSignature = sigBytes;
	invalidateNode();
 assertGN(prepare(true, true));
	return true;
}

CVerifiable::CVerifiable() :CTrieLeafNode(NULL)
{
	basicInit();
	
}

bool CVerifiable::setGUID(std::vector<uint8_t> GUID)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if(GUID.size()!=33)//+1 for the leading network-id byte
	return false;

	mGUID = GUID;
	return true;
}


uint64_t CVerifiable::getVersion()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mVersion;
}



CVerifiable::CVerifiable(eVerifiableType::eVerifiableType type, std::vector<uint8_t> proofID) : CTrieLeafNode(NULL)
{
	basicInit();
	mVerifiableType = type;
	mProof = proofID;
	genID();
}


/// <summary>
/// Pending balance change for a given account.
/// Serialized.
/// Typically stores values BEFORE Tax.
/// </summary>
/// <param name="sdID"></param>
/// <param name="value"></param>
/// <returns></returns>
bool CVerifiable::addBalanceChange(std::vector<uint8_t> sdID, BigSInt value)
{
	if (!CGRIDNET::getTools()->isDomainIDValid(sdID))
		return false;

	//CStateDomain(std::vector<uint8_t> address, std::vector<uint8_t> perspective, std::vector<uint8_t> code, eBlockchainMode::eBlockchainMode mode)
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::shared_ptr<CStateDomain> domain = std::make_shared<CStateDomain>(sdID, std::vector<uint8_t>(), std::vector<uint8_t>(), eBlockchainMode::TestNet, false);
	domain->setPendingPreTaxBalanceChange(value);
	mAffectedDomains.push_back(domain);
	invalidateNode();
	return true;
}
void CVerifiable::arm()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mIsArmed = true;
}
std::vector<std::shared_ptr<CStateDomain>>
CVerifiable::getAffectedStateDomains(bool useEffectiveIfAvailable /*= true*/)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	// If we're NOT using "armed" overrides, just return the original container
	if (!useEffectiveIfAvailable)
	{
		// Return the raw, serialized domains directly
		return mAffectedDomains;
	}

	// Otherwise, we build a copy of the domains with overrides applied where available
	std::vector<std::shared_ptr<CStateDomain>> result;
	result.reserve(mAffectedDomains.size());

	for (auto& originalDom : mAffectedDomains)
	{
		// Make a copy
		auto domCopy = std::make_shared<CStateDomain>(originalDom->getAddress(), std::vector<uint8_t>(), std::vector<uint8_t>(), eBlockchainMode::TestNet,false); //<- this is expected to be actually faster than a copy constructor (due ot internal CTrieDBs).//;// std::make_shared<CStateDomain>(*originalDom);

		// If an override is found, apply it
		auto it = mArmedBalanceOverrides.find(domCopy->getAddress());
		if (it != mArmedBalanceOverrides.end())
		{
			domCopy->setPendingPreTaxBalanceChange(it->second);
		}
		else
		{
			domCopy->setPendingPreTaxBalanceChange(originalDom->getPendingPreTaxBalanceChange());
		}

		result.push_back(domCopy);
	}

	return result;
}

void CVerifiable::armBalanceChange(const std::vector<uint8_t>& domainID, const BigSInt& newValue)
{
	// Do NOT call invalidateNode(), do NOT modify mAffectedDomains.
	// This ensures that the object’s serialized data remains unchanged.
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mArmedBalanceOverrides[domainID] = newValue;
}



void CVerifiable::setProof(std::vector<uint8_t> proofID)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mProof = proofID;
	genID();
	invalidateNode();
}



bool CVerifiable::isArmed()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return (mIsArmed                     // armed explicitly
		|| !mArmedBalanceOverrides.empty() // armed implicitly
		);
}


BigSInt CVerifiable::getArmedValue(const std::vector<uint8_t>& domainID)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	// First check if there's an armed override
	auto it = mArmedBalanceOverrides.find(domainID);
	if (it != mArmedBalanceOverrides.end())
	{
		return it->second;
	}

	// If no armed value exists, look for the original value in affected domains
	for (const auto& domain : mAffectedDomains)
	{
		if (domain->getAddress() == domainID)
		{
			return domain->getPendingPreTaxBalanceChange();
		}
	}

	// If domain not found at all, return 0
	return BigSInt(0);
}




