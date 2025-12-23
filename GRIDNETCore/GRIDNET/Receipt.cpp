#include "Receipt.h"
#include "ScriptEngine.h"
#include "transaction.h"
#include "Block.h"
#include "BlockHeader.h"
#include "BlockchainManager.h"

std::shared_ptr<CReceipt> CReceipt::instantiate(std::vector<uint8_t> serializedData, eBlockchainMode::eBlockchainMode mode)
{
	std::shared_ptr<CReceipt> toRet = std::make_shared<CReceipt>();
	CReceipt* rec = CReceipt::instantiateReceipt(serializedData, mode);
	if (rec == nullptr)
		return nullptr;

	*toRet = *rec;
	delete rec;

	return toRet;
}
CReceipt * CReceipt::instantiateReceipt(std::vector<uint8_t> serializedData, eBlockchainMode::eBlockchainMode mode)
{
	CTrieNode * leaf =CTools::getTools()->nodeFromBytes(serializedData, mode);
	if (leaf == nullptr)
		return nullptr;

	CReceipt *rec = genNode(&leaf);
	return rec;
}

CReceipt * CReceipt::genNode(CTrieNode ** baseDataNode, bool useTestStorageDB)
{
	
	if (baseDataNode == nullptr || *baseDataNode == NULL)
		return NULL;
 assertGN((*(baseDataNode))->isRegisteredWithinTrie() == false);
	CReceipt *receipt = new CReceipt(eBlockchainMode::LocalData);//will be overwritten anyhow
	size_t versionReceived = 0;
	size_t subT = 0;
	try {

	 assertGN((*baseDataNode)->getType() == 3);
		receipt->mName = (*baseDataNode)->mName;
		//receipt->mSerializedSize = (*baseDataNode)->getSerialziedSize();
		Botan::BER_Decoder  dec = Botan::BER_Decoder((*baseDataNode)->getRawData()).start_cons(Botan::ASN1_Tag::SEQUENCE).
			decode(subT).
			decode(versionReceived);

	 assertGN(subT == 3);//ensure it's an receipt;

		Botan::BER_Object obj;
		receipt->mVersion = 3;//we always unpack into the latest known version of the data-structure.
		//That is to support newer logic.
		//WARNING:the default constructors of newer versions of objects MUST pay attention to the mOriginalVersion field
		//and initialize with default values accordingly.
		//SERIALZATION: data is ALWAYS serialized in accordance to mOriginalVersion (so that the produced images remain the same).
		//WARNING: thus, the versions of data-structures are thus NEVER upgraded. These COULD be upgraded were we acting upon the very Horizon of events.


		receipt->mOriginalVersion = versionReceived;//needed so that we know whether we could upgrade some of the fields when not acting upon the horizon of events.
		//if mOriginalVersion != latest version and not at the very horizon of events = we CANNOT upgrade (that would break the history of events).

		switch (versionReceived)
		{
		case 1:
			
			decodeReceiptV1(obj, dec, receipt);
			break;
		case 2:
			decodeReceiptV2(obj, dec, receipt);
			break;

		case 3:
			decodeReceiptV3(obj, dec, receipt);
			break;
		default:
		 assertGN(false);
			break;
		}

		
		//common part across all derived node types
		receipt->mInterData.resize((*baseDataNode)->mInterData.size());
		receipt->mMainRAWData.resize((*baseDataNode)->mMainRAWData.size());

		std::memcpy(receipt->mInterData.data(), (*baseDataNode)->mInterData.data(), (*baseDataNode)->mInterData.size());
		std::memcpy(receipt->mMainRAWData.data(), (*baseDataNode)->mMainRAWData.data(), (*baseDataNode)->mMainRAWData.size());
		receipt->mHasLeastSignificantNibble = (*baseDataNode)->mHasLeastSignificantNibble;


		if (baseDataNode != NULL)
		{
			if ((*baseDataNode)->mPointerToPointerFromParent != NULL)
				(*(*baseDataNode)->mPointerToPointerFromParent) = NULL;
			delete *baseDataNode;
		}
		(*baseDataNode) = NULL;
		receipt->mIsPrepared = true;
		return receipt;
	}
	catch (const std::exception& e)
	{
		delete receipt;

		if ((*baseDataNode) != NULL)
		{
			delete (*baseDataNode);
			(*baseDataNode) = NULL;
		}
	 assertGN(false);

	}

	return NULL;
}

void CReceipt::decodeReceiptV2(Botan::BER_Object& obj, Botan::BER_Decoder& dec, CReceipt* receipt)
{
	std::vector<uint8_t> adr;
	obj = dec.get_next_object();
	assert(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);

	Botan::BER_Decoder  dec2 = Botan::BER_Decoder(obj.value);
	size_t temp = 0;
	std::vector<uint8_t> tempBytes;
	dec2.decode(temp);
	receipt->mReceiptType = static_cast<eReceiptType::eReceiptType>(temp);
	dec2.decode(receipt->mGUID, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(receipt->mReceiptID, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(receipt->mBlockHeight);
	dec2.decode(temp);
	receipt->mResult = static_cast<eTransactionValidationResult>(temp);

	dec2.decode(tempBytes, Botan::ASN1_Tag::OCTET_STRING);
	receipt->mERGUsed = CTools::getInstance()->BytesToBigInt(tempBytes);

	dec2.decode(temp);
	receipt->mERGPrice = temp;//that gets upgraded to BigInt in v3

	dec2.decode(tempBytes, Botan::ASN1_Tag::OCTET_STRING);
	receipt->mSacrificedValue = CTools::getInstance()->BytesToBigInt(tempBytes);
	//dec2.decode(receipt->mSacrificedValue);
	//logs
	obj = dec2.get_next_object();
	Botan::BER_Decoder  dec3 = Botan::BER_Decoder(obj.value);
	assert(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);

	while (dec3.more_items())
	{
		std::vector<uint8_t> logEntryV;
		dec3.decode(logEntryV, Botan::ASN1_Tag::OCTET_STRING);
		std::string log(logEntryV.begin(), logEntryV.end());
		receipt->mLog.push_back(log);
	}
	assert(!dec.more_items());
}

void CReceipt::decodeReceiptV3(Botan::BER_Object& obj, Botan::BER_Decoder& dec, CReceipt* receipt)
{
	std::vector<uint8_t> adr;
	obj = dec.get_next_object();
	assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);

	Botan::BER_Decoder  dec2 = Botan::BER_Decoder(obj.value);
	size_t temp = 0;
	std::vector<uint8_t> tempBytes;
	dec2.decode(temp);
	receipt->mReceiptType = static_cast<eReceiptType::eReceiptType>(temp);
	dec2.decode(receipt->mGUID, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(receipt->mReceiptID, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(receipt->mBlockHeight);
	dec2.decode(temp);
	receipt->mResult = static_cast<eTransactionValidationResult>(temp);
	dec2.decode(tempBytes, Botan::ASN1_Tag::OCTET_STRING);
	receipt->mERGUsed = CTools::getInstance()->BytesToBigInt(tempBytes);
	dec2.decode(tempBytes, Botan::ASN1_Tag::OCTET_STRING);
	receipt->mERGPrice = CTools::getInstance()->BytesToBigInt(tempBytes);
	dec2.decode(tempBytes, Botan::ASN1_Tag::OCTET_STRING);
	receipt->mSacrificedValue = CTools::getInstance()->BytesToBigInt(tempBytes);
	//dec2.decode(receipt->mSacrificedValue);
	//logs
	obj = dec2.get_next_object();
	Botan::BER_Decoder  dec3 = Botan::BER_Decoder(obj.value);
	assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);

	while (dec3.more_items())
	{
		std::vector<uint8_t> logEntryV;
		dec3.decode(logEntryV, Botan::ASN1_Tag::OCTET_STRING);
		std::string log(logEntryV.begin(), logEntryV.end());
		receipt->mLog.push_back(log);
	}
	assertGN(!dec.more_items());
}

void CReceipt::decodeReceiptV1(Botan::BER_Object& obj, Botan::BER_Decoder& dec, CReceipt* receipt)
{
	uint64_t tempUInt = 0;
	std::vector<uint8_t> adr;
	obj = dec.get_next_object();
 assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);
	std::vector<uint8_t> tempBytes;
	Botan::BER_Decoder  dec2 = Botan::BER_Decoder(obj.value);
	size_t temp = 0;
	dec2.decode(temp);
	receipt->mReceiptType = static_cast<eReceiptType::eReceiptType>(temp);
	dec2.decode(receipt->mGUID, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(receipt->mReceiptID, Botan::ASN1_Tag::OCTET_STRING);
	dec2.decode(receipt->mBlockHeight);
	dec2.decode(temp);
	receipt->mResult = static_cast<eTransactionValidationResult>(temp);
	dec2.decode(tempBytes, Botan::ASN1_Tag::OCTET_STRING);
	receipt->mERGUsed = CTools::getInstance()->BytesToBigInt(tempBytes);
	dec2.decode(tempUInt);
	receipt->mERGPrice = tempUInt;
	//logs
	obj = dec2.get_next_object();
	Botan::BER_Decoder  dec3 = Botan::BER_Decoder(obj.value);
 assertGN(obj.type_tag == Botan::ASN1_Tag::SEQUENCE);

	while (dec3.more_items())
	{
		std::vector<uint8_t> logEntryV;
		dec3.decode(logEntryV, Botan::ASN1_Tag::OCTET_STRING);
		std::string log(logEntryV.begin(), logEntryV.end());
		receipt->mLog.push_back(log);
	}
 assertGN(!dec.more_items());
}

bool CReceipt::prepare(bool store)
{
	//[SERIALIZATION]:
	//the purpose of packaging is irrelevant.
	//we always package in accordance to the original version.
	//that is for the computed images to match data present in the history of events.
	//IF a data structure is to store additional data (supported by newer versions of an object,
	//it effectively needs to become a NEW OBJECT.

	/*[DESERIALIZATION]:
	* We always unpack to the LATEST version of the container.
	* And pack to the prior version later on, if needed, as indicated by mOriginalVersion.
	* That is to support new data-types (ex. BigInt instead of uint64_t).
	* 
	* -- EXTREMELY IMPORTANT:It is always needed to be ensured that two way transformation between the data-types is needed though.
	* -- For example: it would not be possible to assure compatibility between double and BigFloat.
	*/



	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	//subtype 1 for StateDomains , subtype 2 for transactions , subtype 3 for receipts, subtype 4 for Verifiables
 assertGN(getType() == 3 && getSubType() == 3, "oh but, ..I'm not that kind of node!");

	std::vector<uint8_t> packedReceiptData;
	size_t bc_mERGPrice = 0;
		std::vector<uint8_t> dat;
		Botan::DER_Encoder enc1 = Botan::DER_Encoder().start_cons(Botan::ASN1_Tag::SEQUENCE);
		
		//make sure the version remains the original one. WARNING: but only if original version was PROVIDED!
		//otherwise we honor the default value of mVersion which is supposed to represent the current latest version of the container.
		//it is important for newly generated Receipts.

		size_t targetVersion = mOriginalVersion ? mOriginalVersion : mVersion;
		enc1.encode(mSubType)//subtype
			.encode(targetVersion);

		switch (targetVersion)
		{
		case 2:
			enc1.start_cons(Botan::ASN1_Tag::SEQUENCE)
				.encode(static_cast<size_t>(mReceiptType))
				.encode(mGUID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mReceiptID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mBlockHeight).
				encode(static_cast<size_t>(mResult)).
				encode(CTools::getInstance()->BigIntToBytes(mERGUsed), Botan::ASN1_Tag::OCTET_STRING);
			bc_mERGPrice = static_cast<size_t>(mERGPrice);
			enc1.encode(bc_mERGPrice);//in version 2 it's still size_t

			
			break;

		case 3:
			enc1.start_cons(Botan::ASN1_Tag::SEQUENCE)
				.encode(static_cast<size_t>(mReceiptType))
				.encode(mGUID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mReceiptID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mBlockHeight).
				encode(static_cast<size_t>(mResult)).
				encode(CTools::getInstance()->BigIntToBytes(mERGUsed), Botan::ASN1_Tag::OCTET_STRING);
			enc1.encode(CTools::getInstance()->BigIntToBytes(mERGPrice), Botan::ASN1_Tag::OCTET_STRING);//version 3 has BigInt
			break;
		default:
			enc1.start_cons(Botan::ASN1_Tag::SEQUENCE)
				.encode(static_cast<size_t>(mReceiptType))
				.encode(mGUID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mReceiptID, Botan::ASN1_Tag::OCTET_STRING)
				.encode(mBlockHeight).
				encode(static_cast<size_t>(mResult)).
				encode(CTools::getInstance()->BigIntToBytes(mERGUsed), Botan::ASN1_Tag::OCTET_STRING);
			enc1.encode(CTools::getInstance()->BigIntToBytes(mERGPrice), Botan::ASN1_Tag::OCTET_STRING);//version 3 has BigInt
			break;
		}

		enc1.encode(CTools::getInstance()->BigIntToBytes(mSacrificedValue),  Botan::ASN1_Tag::OCTET_STRING).
			start_cons((Botan::ASN1_Tag::SEQUENCE));

		for (int i = 0; i < mLog.size(); i++)
		{
			enc1 = enc1.encode(CTools::getTools()->stringToBytes(mLog[i]), Botan::ASN1_Tag::OCTET_STRING);
		}
		enc1 = enc1.end_cons().end_cons().end_cons();

		packedReceiptData = enc1.get_contents_unlocked();
		((CTrieLeafNode*)this)->setRawData(packedReceiptData);
		mIsPrepared = true;
	

	return true;
}

CReceipt::CReceipt(eBlockchainMode::eBlockchainMode blockchainMode) :CTrieLeafNode(NULL)
{
	
	initializeFields();
	//genID(blockchainMode);
	invalidateNode();
}

CReceipt::CReceipt(CTransaction & t, eBlockchainMode::eBlockchainMode blockchainMode) :CTrieLeafNode(NULL)
{
	initializeFields();
 assertGN(blockchainMode != eBlockchainMode::eBlockchainMode::Unknown);
	std::vector<uint8_t> guid;
	uint8_t netID = CTools::getInstance()->blockchainModeToNetID(blockchainMode);
	guid.push_back(netID);
	std::vector<uint8_t> hasHash = CCryptoFactory::getInstance()->getSHA2_256Vec(t.getHash());
	guid.insert(guid.begin(), hasHash.begin(), hasHash.end());
	mGUID = guid;
}

CReceipt::CReceipt(std::vector<uint8_t> id) :CTrieLeafNode(NULL)
{
	initializeFields();
	if(id.size() > 32);
	 mGUID = id;
}

void CReceipt::initializeFields()
{
	//mTargetNonce = 0;
	mOriginalVersion = 0;
	mERGPrice = 0;
	mBlockHeight = 0;
	mType = 3;
	mSubType = 3;
	mVersion = 3;
	mERGUsed = 0;
	mSacrificedValue = 0;
	mResult = eTransactionValidationResult::invalid;
	mReceiptType = eReceiptType::eReceiptType::transaction;
	invalidateNode();
}

CReceipt::CReceipt(const CReceipt & sibling) : CTrieLeafNode(sibling)
{
	mOriginalVersion = sibling.mOriginalVersion;
	mSacrificedValue = sibling.mSacrificedValue;
	mBERMetaData = sibling.mBERMetaData;
	mERGPrice = sibling.mERGPrice;
	mBlockHeight = sibling.mBlockHeight;
	mReceiptID = sibling.mReceiptID;
	//mBlockID = sibling.mBlockID;
	mVersion = sibling.mVersion;
	mResult = sibling.mResult;
	mERGUsed = sibling.mERGUsed;
	mReceiptType = sibling.mReceiptType;
	mGUID = sibling.mGUID;
	mLog = sibling.mLog;
	mMetaData = sibling.mMetaData;
	//common part
	//invalidateNode();

}

CReceipt & CReceipt::operator=(const CReceipt & t)
{
	CTrieLeafNode::operator =(t);
	mOriginalVersion = t.mOriginalVersion;
	mSacrificedValue = t.mSacrificedValue;
	mBERMetaData = t.mBERMetaData;
	mERGPrice = t.mERGPrice;
	mBlockHeight = t.mBlockHeight;
	mReceiptID = t.mReceiptID;
	//mBlockID = t.mBlockID;
	mVersion = t.mVersion;
	mResult = t.mResult;
	mERGUsed = t.mERGUsed;
	mReceiptType = t.mReceiptType;
	mGUID = t.mGUID;
	mLog = t.mLog;
	mMetaData = t.mMetaData;
	//common part
	invalidateNode();
	return *this;
}

void CReceipt::setERGUsed(BigInt erg)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mERGUsed = erg;
	invalidateNode();
}

BigInt CReceipt::getERGPrice()
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return mERGPrice;
}

void CReceipt::setERGprice(BigInt price)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mERGPrice = price;
}
/*
uint64_t CReceipt::getTargetNonce()
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);

	return mTargetNonce;
}

void CReceipt::setTargetNonce(uint64_t nonce)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);

	 mTargetNonce = nonce;
}*/

BigInt CReceipt::getERGUsed() const
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return mERGUsed;
}

void CReceipt::setResult(eTransactionValidationResult result)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mResult = result;
	invalidateNode();
}

eTransactionValidationResult CReceipt::getResult() const
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return  mResult;
}

std::vector<uint8_t> CReceipt::getGUID() const
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return mGUID;
}

void CReceipt::logEvent(SE::Cell cell,SE::CD contentDescription, std::shared_ptr<SE::CScriptEngine> se)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	std::string value;
	switch (contentDescription.getDataType())
	{ 
	case eDataType::eDataType::signedInteger:
		value = std::to_string(cell);
		break;
	case eDataType::eDataType::unsignedInteger:
		value = std::to_string(cell);
		break;
	case eDataType::eDataType::booll:
		value = cell ? "True" : "False";
		break;
	case eDataType::eDataType::charr:
		value = std::to_string(cell);
		break;
	case eDataType::eDataType::doublee:
		value = std::to_string(reinterpret_cast<double&>(cell));
		break;
	case eDataType::eDataType::bytes:
	
		if (se->getVec(reinterpret_cast<SE::AAddr>(cell)).size() > 15)
		{
			value = std::string(se->getVec(reinterpret_cast<SE::AAddr>(cell)).begin(), se->getVec(reinterpret_cast<SE::AAddr>(cell)).begin() + 14);
		}
		else
		{
			value = std::string(se->getVec(reinterpret_cast<SE::AAddr>(cell)).begin(), se->getVec(reinterpret_cast<SE::AAddr>(cell)).end());
		}
		break;
	case eDataType::eDataType::pointer:
		//value = std::to_string(reinterpret_cast<uint64_t&>(cell));
		value = "";//no need to store it. potential security/anonymity risk.
		break;
	case eDataType::eDataType::noData:
		value = "";
		break;
	case eDataType::eDataType::directory:
		break;
			
	default:
		break;
	}
	mLog.push_back(contentDescription.getFunctionName()+" v: "+ value+" ("+contentDescription.getDescription()+")");
	invalidateNode();
}

void CReceipt::logEvent(std::string str)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mLog.push_back(str);
	invalidateNode();
}

std::vector<std::string> CReceipt::getLog() const
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return mLog;
}

size_t CReceipt::getLogSize() const
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return mLog.size();
}


/// <summary>
/// Returns top-Count elements rendered as a newLine-delimited string.
/// </summary>
/// <param name="count"></param>
/// <param name="newLine"></param>
/// <returns></returns>
std::string CReceipt::getRenderedLog(uint64_t count, std::string newLine, uint64_t minEntryLength)
{
	std::string toRet;
	uint64_t included = 0;
	if (mLog.size() == 0)
		return toRet;
	std::string previousLine = "";
	for (uint64_t i = mLog.size() - 1; i > 0 && included < count; i--)
	{
		if (mLog[i].size()> minEntryLength && !CTools::getInstance()->doStringsMatch(previousLine, mLog[i]))
		{
			toRet += mLog[i] + newLine;
			included++;
			previousLine = mLog[i];
		}
	}
	return toRet;

}

void CReceipt::setReceiptType(eReceiptType::eReceiptType type)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mReceiptType = type;
	invalidateNode();
}

eReceiptType::eReceiptType CReceipt::getReceiptType() const
{
	return static_cast<eReceiptType::eReceiptType>(mReceiptType);
}

void CReceipt::setBlockInfo(std::shared_ptr<CBlock> block)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
 assertGN(block->getHeader() != NULL);
	//mTransactionID = CCryptoFactory::getInstance()->getSHA3_256Vec(trans->getPackedData());
	mBlockHeight = block->getHeader()->getHeight();
	//mBlockID = block->getHeader()->getHash();
	invalidateNode();
}

void CReceipt::clearBlockInfo()
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mBlockHeight = 0;
	//mBlockID = block->getHeader()->getHash();
	invalidateNode();
}


uint64_t CReceipt::getBlockHeight()
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return mBlockHeight;
}

//std::vector<uint8_t> CReceipt::getBlockHash()
//{
//	return mBlockID;
//}

void CReceipt::setVerifiableID(std::vector<uint8_t> ID)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mReceiptID = ID;
	invalidateNode();
}

std::vector<uint8_t> CReceipt::getVerifiableID() const
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return mReceiptID;
}

std::string CReceipt::toString()
{
	return "";
}

std::string CReceipt::translateStatus() const
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	switch (mResult)
	{
	case eTransactionValidationResult::incosistentData:
		return "inconsistent data";
		break;
	case eTransactionValidationResult::ERGBidTooLow:
		return "ERG-Bid too low";
		break;
	case eTransactionValidationResult::insufficientERG:
		return "insufficient ERG"; 
		break;
	case eTransactionValidationResult::invalid:
		return "GridScript error";
		break;
	case eTransactionValidationResult::invalidBalance:
		return "invalid Balance";
		break;
	case eTransactionValidationResult::invalidNonce:
		return "invalid Nonce";
		break;
	case eTransactionValidationResult::invalidSig:
		return "invalid Signature (envelope)";//the envelope of the transaction was Invalid.
		//if any signature errors occurred during GridScript code execution, these would be indicated within the Receipt's Log.
		break;
	case eTransactionValidationResult::noIDToken:
		return "no ID-Token";
		break;
	case eTransactionValidationResult::noPublicKey:
		return "no public-key";
		break;
	case eTransactionValidationResult::pubNotMatch:
		return "public-key does not match";
		break;
	case eTransactionValidationResult::unknownIssuer:
		return "unknown transaction issuer";
		break;
	case eTransactionValidationResult::valid:
		return "Valid Transaction";
		break;
	case eTransactionValidationResult::validNoTrieEffect:
		return "Valid - no Trie-effect";
		break;
	case 99:
		return "Pending";
		break;
	default: 
		return "unknown";
		break;

	}
	return "unknown";
}

void CReceipt::setID(std::vector<uint8_t> ID)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mGUID = ID;
	invalidateNode();
}

void CReceipt::setBERMetaData(std::vector<uint8_t> data)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mBERMetaData = data;
}

std::vector<uint8_t> CReceipt::getBERMetaData()
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return mBERMetaData;
}

void CReceipt::setSacrificedValue(BigInt value)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mSacrificedValue = value;
}

BigInt CReceipt::getSacrificedValue()
{	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	return mSacrificedValue;
}



bool CReceipt::genID(eBlockchainMode::eBlockchainMode blockchainMode)
{
	std::lock_guard<ExclusiveWorkerMutex> lock(mGuardian);
	mGUID = CTools::getTools()->getReceiptIDForTransaction(blockchainMode);
	invalidateNode();
	return true;
}
