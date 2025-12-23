#pragma once
#include "transaction.h"
#include "stdafx.h"
#include<ctime>
#include "Settings.h"
#include "BlockchainManager.h"
#include "DataConcatenator.h"
/// <summary>
/// Prepares the internals of a Transaction container.
/// Notice: this might be either because
/// 1) the data truly needs to be stored.
/// 2) we 'just' need to compute its image.
/// 
/// In the latter case	we need to make sure the version (and other serialized data) remain the exactly same as in  the original one.
/// Meaning in the same representation as originally stored meaning also as dictated by the original version of the container.
/// Otherwise, data integration checks, facilitated through a Merkle-Patricia Trie (comprising the corresponding CTrieDB) storing the Trie
/// of Transaction - these checks would have failed.
/// 
/// In the former case however we always need to pack data in accordance to he latest version of the container. That is because, when
/// actually storing data - we are always acting upon the very Events' Horizon.
/// </summary>
/// <param name="ommitSignature"></param>
/// <returns></returns>
std::vector<uint8_t> CTransaction::internalsPrepare(bool ommitSignature)
{
	std::vector<uint8_t> coldStoragePropertiesBytes;
	std::shared_ptr<CTools> tools = CTools::getInstance();

 assertGN(sizeof(ColdProperties) == 1);
	invalidateNode();
	coldStoragePropertiesBytes.resize(sizeof(ColdProperties));
	if(!ommitSignature)//IMPORTANT: ColdProperties *ARE NOT* signed by the author of the transaction.
		//these Flags are set and validated by liquidators. i.e miners.
		//we do not include these fields when signing, these are filled with 0s instead.
	std::memcpy(coldStoragePropertiesBytes.data(), &ColdProperties, sizeof(ColdProperties));
	else std::memset(coldStoragePropertiesBytes.data(), 0, sizeof(ColdProperties));
	size_t bc_mERGPrice = 0;
	Botan::DER_Encoder enc;
	//IMPORTANT: data is packed ALWAYS in accordance to the original version of this container.
//It is same with receipts.

	//make sure the version remains the original one. WARNING: but only if original version was PROVIDED!
	//otherwise we honor the default value of mVersion which is supposed to represent the current latest version of the container.

	size_t targetVersion = mOriginalVersion ? mOriginalVersion : mVersion;
	enc.start_cons(Botan::ASN1_Tag::SEQUENCE).
		encode(mSubType).encode(targetVersion);//<= IMPORTANT - always pack to the original version.



	switch (targetVersion)
	{
	case 1://[DEPRECATED but still used by the mobile application].
		bc_mERGPrice = static_cast<size_t>(mErgPrice);//notice - this would never overflow since higher number were not supported back then
		//and we always serialized to the latest revision of container when actually storing data.
		
		enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
			.encode(coldStoragePropertiesBytes, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mIssuer, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mPubKey, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mNonce)
			.encode(mCode, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mTimestamp)
			.encode(bc_mERGPrice)//in Revision #1 - this used to be an unsigned integer - serialized from size_t.
			//in #2 we've made it a BigInt.
			.encode(tools->BigIntToBytes(mErgLimit), Botan::ASN1_Tag::OCTET_STRING)
			.encode(mExtData, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mLockTime);
		break;
	case 2://[BUGGY signature - CVE-2025-GRIDNET-001]
	case 3://[LATEST REVISION - CVE-2025-GRIDNET-001 FIXED]
		// Version 2 & 3: Identical BER structure (difference is only in signature computation)
		enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
			.encode(coldStoragePropertiesBytes, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mIssuer, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mPubKey, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mNonce)
			.encode(mCode, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mTimestamp)
			.encode(tools->BigIntToBytes(mErgPrice), Botan::ASN1_Tag::OCTET_STRING)//BigInt, since revision #2.
			.encode(tools->BigIntToBytes(mErgLimit), Botan::ASN1_Tag::OCTET_STRING)
			.encode(mExtData, Botan::ASN1_Tag::OCTET_STRING)
			.encode(mLockTime);
		break;
	default:
		break;
	}

		

		if (ommitSignature)
			enc = enc.encode_null().end_cons().end_cons();
		else enc = enc
            .encode(mSignature, Botan::ASN1_Tag::OCTET_STRING)
			.end_cons().end_cons();
		return enc.get_contents_unlocked();

}
CTransaction & CTransaction::operator=(const CTransaction & t)
{
	CTrieLeafNode::operator =(t);
	this->mOriginalVersion = t.mOriginalVersion;
	this->setSubType(2);
	this->mNonce = t.mNonce;
	this->mEndpoint = t.mEndpoint;
	this->mConversation = t.mConversation;
	this->mReceivedAt = t.mReceivedAt;
	//this->mTransactionSubtype = t.mTransactionSubtype;
	this->mIsPrepared = t.mIsPrepared;
	this->mCode = t.mCode;
	//this->mGUID = t.mGUID;
	this->mTimestamp = t.mTimestamp;
	this->mErgPrice = t.mErgPrice;
	this->mErgLimit = t.mErgLimit;
	this->mExtData = t.mExtData;
	this->mValue = t.mValue;
	this->mVersion = t.mVersion;
	this->mLockTime = t.mLockTime;//TODO add support
	this->mSignature = t.mSignature;
	this->mCf = t.mCf;
	this->mIssuer = t.mIssuer;
	this->mPubKey = t.mPubKey;
	this->ColdProperties = t.ColdProperties;
	this->mMetaData = t.mMetaData;
	invalidateNode();
	return *this;
}
CTransaction::CTransaction() :CTrieLeafNode(NULL)
{
	//mTransactionSubtype = subtype;
	mOriginalVersion = 0;
	mNonce = 0;
    mTimestamp= std::time(0);
	mReceivedAt = mTimestamp;
    mVersion = 3;//Version 3 fixes CVE-2025-GRIDNET-001 (ERG Price/Limit signature authentication)
    mLockTime = 0;
    mValue = 0;
    mErgPrice = 1;
    mIsPrepared = false;
	ColdProperties.markAsInvalid(true);
	//HotProperties.markAsInvalid(true);
    mType = 3;
    mSubType = 2;
	//genID();
	mCf = CCryptoFactory::getInstance();


}

CTransaction::CTransaction(const CTransaction &sibling) :CTrieLeafNode(sibling)
{
	this->mOriginalVersion = sibling.mOriginalVersion;
	this->mConversation = sibling.mConversation;
	this->mEndpoint = sibling.mEndpoint;
	this->mNonce = sibling.mNonce;
	this->ColdProperties = sibling.ColdProperties;
	this->mCf = sibling.mCf;
	this->mReceivedAt = sibling.mReceivedAt;
	this->mCode = sibling.mCode;
	this->mIssuer = sibling.mIssuer;
	this->mPubKey = sibling.mPubKey;
	this->mTimestamp = sibling.mTimestamp;
	this->mErgPrice = sibling.mErgPrice;
	this->mErgLimit = sibling.mErgLimit;
	this->mExtData = sibling.mExtData;
	this->mValue = sibling.mValue;
	this->mVersion = sibling.mVersion;
	this->mLockTime = sibling.mLockTime;//TODO add support
	this->mSignature = sibling.mSignature;
	this->ColdProperties = sibling.ColdProperties;
	this->mMetaData = sibling.mMetaData;
}



size_t  CTransaction::getTime() {
	//std::lock_guard<std::recursive_mutex> lock(mGuardian); 
	return mTimestamp;
}

std::string CTransaction::getSourceCode()
{
	return mSourceCode;
}

void CTransaction::setSourceCode(const std::string& sourceCode)
{
	mSourceCode = sourceCode;
}

BigInt CTransaction::getErgPrice()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mErgPrice;
}

uint64_t CTransaction::getNonce()
{
	return mNonce;
}

void CTransaction::setNonce(uint64_t nonce)
{
	 mNonce= nonce;
}


BigInt CTransaction::getErgLimit()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mErgLimit;
}

size_t CTransaction::getVersion()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mVersion;
}

void CTransaction::setIssuer(std::vector<uint8_t> issuerID)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mIssuer = issuerID;
	invalidateNode();
}

std::vector<uint8_t> CTransaction::getIssuer()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mIssuer;
}


std::vector<uint8_t> CTransaction::getExtData()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mExtData;
}

std::vector<uint8_t> CTransaction::getSignature()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mSignature;
}

/// <summary>
/// Signs the Transaction.
/// </summary>
/// <param name="privateKey"></param>
/// <returns></returns>
bool CTransaction::sign(Botan::secure_vector<uint8_t> privateKey)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (privateKey.size() == 32)
		privateKey = CTools::getTools()->BERVector(privateKey);


	//if (mPubKey.size() != 32)   <= Notice, public key is NOT required if Identity Token is registered on-the-chain
	// mIssuer is then using which public key is retrieved from the state-machine.
		//return false;
	//get data with empty sig field
	//assert(prepare(true, false));

	//UPDATE: we've made the signature independant of the underlying encoding scheme
	//thus data is now low-level little endian data is now concatenated using CDataConcatenator

	/* The local node might not be aware of public key/private key and rely on GRIDNEToken app to provide
	the required information.

	The signature function takes the following input:
	SIG(HASH[HASH[data_without_pubKey] | pubKey])
	
	Thus, data consituting the Transaction MIGHT remain unknown to the GRIDNEToken mobile app, while pubKey MIGHT
	remain unknown to full-node during transaction generation. PrivKey would ALWAYS remain unknown to a remote full-node.
	*/
	
	std::vector<uint8_t> toSign = getDataToSign();

	std::vector<uint8_t> sigBytes =mCf->signData(toSign, privateKey);

	//fill in the sig-field
    mSignature = sigBytes;
	invalidateNode();
 assertGN(prepare(true, true));
	return true;

}
void CTransaction::setSignatureBytes(std::vector<uint8_t> sig)
{
	mSignature = sig;
}

std::vector<uint8_t> CTransaction::getDataToSign()
{
	std::vector<uint8_t> toSign;
	if (!getConcatData(toSign, false, false))
		return std::vector<uint8_t>();

	CDataConcatenator concat;
	concat.add(toSign);
	//concat.add(mPubKey); <- this is not needed , besides the public key might not be known at this time

	std::vector<uint8_t> dataToHash = concat.getData();
	toSign = mCf->getSHA2_256Vec(dataToHash);

	return toSign;
}

/// <summary>
/// Gets RAW concatenated data vector.
/// </summary>
/// <param name="data"></param>
/// <param name="includePubKey"></param>
/// <param name="includeSig"></param>
/// <returns></returns>
bool CTransaction::getConcatData(std::vector<uint8_t>& data, bool includePubKey, bool includeSig)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	CDataConcatenator concat;
	//std::shared_ptr<CTools> tools = CTools::getInstance();
	std::vector<uint8_t> result;
	if (includePubKey && mPubKey.size() == 0)
		return false;
	if (includeSig && mSignature.size() == 0)
		return false;
	uint64_t tempUInt = 0;
	size_t targetVersion = mOriginalVersion ? mOriginalVersion : mVersion;
	switch (targetVersion)
	{
	case 1:
		// Version 1: Uses BUGGY BigInt serialization for ErgLimit (CVE-2025-GRIDNET-001)
		if(includePubKey)
		concat.add(mPubKey);
		if (includeSig)
			concat.add(mSignature);

		concat.add(targetVersion);//for backwards compatibility since we always unpack to the latest revision.
		concat.add(mIssuer);
		concat.add(mNonce);
		concat.add(mCode);
		concat.add(mTimestamp);
		tempUInt = static_cast<uint64_t> (mErgPrice);
		concat.add(tempUInt);
		concat.addBigIntBugged(mErgLimit);  // ❌ BUGGY: ERG Limit not authenticated
		concat.add(mExtData);
		result = concat.getData();
		break;

	case 2:
		// Version 2: Uses BUGGY BigInt serialization (CVE-2025-GRIDNET-001)
		if (includePubKey)
			concat.add(mPubKey);
		if (includeSig)
			concat.add(mSignature);

		concat.add(mVersion);
		concat.add(mIssuer);
		concat.add(mNonce);
		concat.add(mCode);
		concat.add(mTimestamp);
		concat.addBigIntBugged(mErgPrice);  // ❌ BUGGY: ERG Price not authenticated
		concat.addBigIntBugged(mErgLimit);  // ❌ BUGGY: ERG Limit not authenticated
		concat.add(mExtData);
		result = concat.getData();
		break;

	case 3:
		// Version 3: FIXED BigInt serialization (CVE-2025-GRIDNET-001 mitigated)
		// Properly authenticates ERG Price and ERG Limit in transaction signature
		if (includePubKey)
			concat.add(mPubKey);
		if (includeSig)
			concat.add(mSignature);

		concat.add(mVersion);
		concat.add(mIssuer);
		concat.add(mNonce);
		concat.add(mCode);
		concat.add(mTimestamp);
		concat.addBigInt(mErgPrice);  // ✓ FIXED: ERG Price properly authenticated
		concat.addBigInt(mErgLimit);  // ✓ FIXED: ERG Limit properly authenticated
		concat.add(mExtData);
		result = concat.getData();
		break;

	default:
		return false;
		break;
	}
	data = result;
	return true;
}



std::vector<uint8_t> CTransaction::getCode() const
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mCode;

}
void CTransaction::setERGPrice(BigInt price)
{
	mErgPrice = price;
	invalidateNode();
}
void CTransaction::setERGLimit(BigInt limit)
{
	mErgLimit = limit;
	invalidateNode();
}
/// <summary>
/// used for tests.
/// </summary>
/// <returns></returns>
bool CTransaction::destroyEnvelopeSig()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mSignature.size() > 0)
	{
		CTools::getTools()->introduceRandomChange(mSignature);
		invalidateNode();
		return true;
	}
	return false;
}

/// <summary>
/// used for tests.
/// </summary>
/// <returns></returns>
bool CTransaction::destroyPubKey()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mPubKey.size() > 0)
	{
		CTools::getTools()->introduceRandomChange(mPubKey);
		invalidateNode();
		return true;
	}
	return false;
}

/// <summary>
/// used for tests.
/// </summary>
/// <returns></returns>
bool CTransaction::clearPubKey()
{
		mPubKey.clear();
		invalidateNode();
		return true;
}
CTransaction *  CTransaction::genNode(CTrieNode **baseDataNode)
{
	try {
		uint64_t tempUInt = 0;
		std::shared_ptr<CTools> tools = CTools::getInstance();
		std::vector<uint8_t> temp;
		//size_t versionReceived = 0;
		std::vector<uint8_t> ColdPropertiedBytes;
		if (*baseDataNode == NULL)
			return NULL;
	 assertGN((*(baseDataNode))->isRegisteredWithinTrie() == false);

		CTransaction* tr = new CTransaction;
		tr->mName = (*baseDataNode)->mName;
		Botan::BER_Decoder dec1 = Botan::BER_Decoder((*baseDataNode)->getRawData()).start_cons(Botan::ASN1_Tag::SEQUENCE);
		Botan::BER_Decoder dec2 = dec1.decode(tr->mSubType).decode(tr->mOriginalVersion).start_cons(Botan::ASN1_Tag::SEQUENCE);

		tr->mVersion = tr->mOriginalVersion;//preserve original version for signature compatibility
		//tr->mOriginalVersion = versionReceived;//yet still, we need to keep track of the original version, since - as far as consensus is 
		//concerned - the resulting image (sha256 hash) - it needs to remain the very same.
		//otherwise, data integrity checks (through the corresponding Merkle-Patricia Trie, comprising the corresponding CTrieDB),
		//these would have failed - causing the entire block being rejected.
		//Notice: we introduce a similar container translation layer in the case of Receipts.


		//Version Specific Decoding - BEGIN
		switch (tr->mOriginalVersion)
		{

		case 1:
			decodeV1(dec2, ColdPropertiedBytes, tr, tempUInt, temp, tools);
			break;

		case 2:
		case 3:
			// Version 2 & 3: Identical BER structure (difference is only in signature computation)
			extractV2(dec2, ColdPropertiedBytes, tr, temp, tools);
			break;

		}
		//Version Specific Decoding - END

		dec1.end_cons();

		//common part between all specialized node types:
		tr->mInterData.resize((*baseDataNode)->mInterData.size());
		tr->mMainRAWData.resize((*baseDataNode)->mMainRAWData.size());

		std::memcpy(tr->mInterData.data(), (*baseDataNode)->mInterData.data(), (*baseDataNode)->mInterData.size());
		std::memcpy(tr->mMainRAWData.data(), (*baseDataNode)->mMainRAWData.data(), (*baseDataNode)->mMainRAWData.size());
		tr->mHasLeastSignificantNibble = (*baseDataNode)->mHasLeastSignificantNibble;

		if (baseDataNode != NULL)
		{
			//tr->mSerializedSize = (*baseDataNode)->getSerialziedSize();
			if ((*baseDataNode)->mPointerToPointerFromParent != NULL)
				(*(*baseDataNode)->mPointerToPointerFromParent) = NULL;
			delete* baseDataNode;
		}
		(*baseDataNode) = NULL;
		return tr;
	}
	catch (...)
	{
		return nullptr;
	}

}
void CTransaction::extractV2(Botan::BER_Decoder& dec2, std::vector<uint8_t>& ColdPropertiedBytes, CTransaction* tr, std::vector<uint8_t>& temp, std::shared_ptr<CTools>& tools)
{
	dec2.decode(ColdPropertiedBytes, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mIssuer, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mPubKey, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mNonce);
	dec2.decode(tr->mCode, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mTimestamp);
	dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
	tr->mErgPrice = tools->BytesToBigInt(temp);
	dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
	tr->mErgLimit = tools->BytesToBigInt(temp);
	//dec2.decode(tr->mErgLimit);
	dec2.decode(tr->mExtData, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mLockTime);
	dec2.decode(tr->mSignature, Botan::ASN1_Tag::OCTET_STRING);
	dec2.end_cons();
	std::memcpy(&tr->ColdProperties, ColdPropertiedBytes.data(), ColdPropertiedBytes.size());
}
void CTransaction::decodeV1(Botan::BER_Decoder& dec2, std::vector<uint8_t>& ColdPropertiedBytes, CTransaction* tr, uint64_t& tempUInt, std::vector<uint8_t>& temp, std::shared_ptr<CTools>& tools)
{
	dec2.decode(ColdPropertiedBytes, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mIssuer, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mPubKey, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mNonce);
	dec2.decode(tr->mCode, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mTimestamp);
	dec2.decode(tempUInt);
	tr->mErgPrice = tempUInt;
	dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
	tr->mErgLimit = tools->BytesToBigInt(temp);
	//dec2.decode(tr->mErgLimit);
	dec2.decode(tr->mExtData, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(tr->mLockTime);
	dec2.decode(tr->mSignature, Botan::ASN1_Tag::OCTET_STRING);
	dec2.end_cons();
	std::memcpy(&tr->ColdProperties, ColdPropertiedBytes.data(), ColdPropertiedBytes.size());
}
size_t CTransaction::getReceivedAt()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mReceivedAt;
}
std::vector<uint8_t> CTransaction::getPubKey()
{
	return mPubKey;
}
bool CTransaction::setPubKey(std::vector<uint8_t> pubKey)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	invalidateNode();
	mPubKey = pubKey;
	return true;
}
std::shared_ptr<CEndPoint> CTransaction::getIssuingEndpoint()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mEndpoint;
}
void CTransaction::setIssuingEndpoint(std::shared_ptr<CEndPoint> endpoint)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mEndpoint = endpoint;
}
std::shared_ptr<CConversation> CTransaction::getIssuingConversation()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mConversation;
}
void CTransaction::setIssuingConversation(std::shared_ptr<CConversation> conversation)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	 mConversation = conversation;
}
CTransaction * CTransaction::instantiate(std::vector<uint8_t>& serializedData, eBlockchainMode::eBlockchainMode blockchainMode)
{
	CTrieNode * leaf = CTools::getTools()->nodeFromBytes(serializedData, blockchainMode);
	

	if (leaf == NULL || leaf->mMainRAWData.size() == 0)
	{
		return nullptr;
	}

	//CTransaction *tr = genNode(&leaf);

	//CTransaction * tr = new CTransaction();
	return static_cast<CTransaction*> (leaf);
	
}

bool CTransaction::isValid()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mIsPrepared)
		return true;
	else
		return false;
	//TODO: add additional validation checks
}

bool CTransaction::setCode(std::vector<uint8_t> code)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
    this->mCode = code;
	invalidateNode();
	return true;
}

/// <summary>
/// sets extra data. the data is signed
/// </summary>
/// <param name="data"></param>
/// <returns></returns>
bool CTransaction::setExtData(std::vector<uint8_t> data)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
    this->mExtData = data;
	invalidateNode();
	return true;
}

//Verifies the Transaction's signature.
bool CTransaction::verifySignature(std::vector<uint8_t> publicKey)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mSignature.size() == 0)
		return false;

	std::vector<uint8_t> toVerify = getDataToSign();
	std::vector<uint8_t> signature = mSignature;
	
	//prepare(false, false);//do not include signature into serialized data

	return CCryptoFactory::getInstance()->verifySignature(signature, toVerify, publicKey);
}

void CTransaction::markAsProcessed()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	HotProperties.markAsProcessed(true);
}

bool CTransaction::wasProcessed()
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return HotProperties.isMarkedAsProcessed();
 
}

/// <summary>
/// 
/// </summary>
/// <param name="store"></param>
/// <param name="includeSig">each transaction contains a sig. First we need to calculate hash
	///  of the entire transaction(with NULL sig) and then insert signature inside and getPackedData again.</param>
/// <returns></returns>
bool CTransaction::prepare(bool store,bool includeSig)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	//subtype 1 for StateDomains , subtype 2 for transactions , subtype 3 for receipts
 assertGN(getType() == 3 && getSubType() == 2, "oh but, ..I'm not that kind of node!");

		std::vector<uint8_t> internals;
		//IMPORTANT: make sure the version remains the original one.
		internals = internalsPrepare(!includeSig);
	
		((CTrieLeafNode*)this)->setRawData(internals);
		if (includeSig)
			mIsPrepared = true; 
		else
			mIsPrepared = false;//transaction without signature is only fine for signature verification.
	

	return true;
}

std::vector<uint8_t> CTransaction::getPackedData(bool ommitSig)
{
	if(!ommitSig)
	{
    assertGN(mSignature.size() >0 , "transaction was not signed yet!");

	//get data with sig-field filled in

 assertGN(prepare(true, true));
	}
	else
	{
	 assertGN(prepare(false, false));

	}
	return ((CTrieLeafNode*)this)->getPackedData();
}

bool CTransaction::verifyInitCode(std::vector<uint8_t> code)
{
	return false;
}
