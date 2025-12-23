#include "TransmissionToken.h"
#include "CryptoFactory.h"



CTransmissionToken::CTransmissionToken(const CTransmissionToken& sibling)
{
	mRevealedHash = sibling.mRevealedHash;
	mTokenPoolID = sibling.mTokenPoolID;
	mDataHash = sibling.mDataHash;
	mSig = sibling.mSig;
	mBankIndex = sibling.mBankIndex;
	mVersion = sibling.mVersion;
	mRevealedHashesCount = sibling.mRevealedHashesCount;
	mBankUsageDepth = sibling.mBankUsageDepth;
	mValue = sibling.mValue;
	mRecipient = sibling.mRecipient;
}

void CTransmissionToken::initFields()
{
	mBankUsageDepth = 0;
	mRevealedHashesCount = 0;
	mValue = 0;
	mVersion = 1;
	mBankIndex = 0;
}

CTransmissionToken::CTransmissionToken(uint64_t bankID, BigInt value, std::vector<uint8_t> revealedHash, BigInt revealedHashesCount, BigInt bankUsageDepth, std::vector<uint8_t> dataHash, std::vector<uint8_t> tokenPoolID, std::vector<uint8_t> transmissionTokenID, std::vector<uint8_t> sig)
{
	initFields();
	mBankIndex = bankID;
	mRevealedHash = revealedHash;
	mRevealedHashesCount = revealedHashesCount;
	mBankUsageDepth = bankUsageDepth;
	mDataHash = dataHash;
	mTokenPoolID = tokenPoolID;
	mSig = sig;
	mValue = value;
}

void CTransmissionToken::setRecipient(std::vector<uint8_t> recipient)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mRecipient = recipient;
}

std::vector<uint8_t> CTransmissionToken::getRecipient()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mRecipient;
}

uint64_t CTransmissionToken::getVersion()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mVersion;
}

bool CTransmissionToken::sign(Botan::secure_vector<uint8_t> privKey)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if(privKey.size()!=32)
	return false;
	mSig = CCryptoFactory::getInstance()->signData(getPackedData(false), privKey);
	return true;
}

bool CTransmissionToken::verifySignature(std::vector<uint8_t> pubKey)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> data = getPackedData(false);
	return CCryptoFactory::getInstance()->verifySignature(mSig, data, pubKey);
}

uint64_t CTransmissionToken::getBankID()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mBankIndex;
}

void CTransmissionToken::setBankID(uint64_t id)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mBankIndex = id;
}

std::vector<uint8_t> CTransmissionToken::getRevealedHash()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mRevealedHash;
}

BigInt CTransmissionToken::getRevealedHashesCount()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mRevealedHashesCount;
}

void CTransmissionToken::setRevealedHashesCount(BigInt count)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	 mRevealedHashesCount = count;
}

BigInt CTransmissionToken::getCurrentDepth()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mBankUsageDepth;
}

BigInt CTransmissionToken::getValue()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return  mValue;
}
void  CTransmissionToken::setValue(BigInt value)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	 mValue= value;
}


std::vector<uint8_t> CTransmissionToken::getDataHash()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mDataHash;
}

std::vector<uint8_t> CTransmissionToken::getID()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mTokenPoolID;
}


std::vector<uint8_t> CTransmissionToken::getSig()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mSig;
}

bool CTransmissionToken::validate()
{
	return mBankUsageDepth>0 && mRevealedHashesCount>0 && mVersion>0 && mTokenPoolID.size()==32;
}


/// <summary>
/// Gets the serialized, BER-encoded data-structure
/// </summary>
/// <param name="includeSig"></param>
/// <returns></returns>
std::vector<uint8_t> CTransmissionToken::getPackedData(bool includeSig)
{
	std::lock_guard <std::recursive_mutex> lock(mGuardian);
	
	Botan::DER_Encoder enc;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	enc.start_cons(Botan::ASN1_Tag::SEQUENCE).
		encode(mVersion)
		.start_cons(Botan::ASN1_Tag::SEQUENCE);

	//encode required-fields - BEGIN
	enc.encode(mRevealedHash, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mBankIndex)
		.encode(tools->BigIntToBytes(mBankUsageDepth),  Botan::ASN1_Tag::OCTET_STRING)
		.encode(tools->BigIntToBytes(mRevealedHashesCount),  Botan::ASN1_Tag::OCTET_STRING)
		.encode(mTokenPoolID, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mDataHash, Botan::ASN1_Tag::OCTET_STRING)
		.encode(tools->BigIntToBytes(mValue), Botan::ASN1_Tag::OCTET_STRING);//optional
	

	if (includeSig)
	{
		enc = enc.encode(mSig, Botan::ASN1_Tag::OCTET_STRING);//optional

		if(mRecipient.size()>0) 
			enc=enc.encode(mRecipient, Botan::ASN1_Tag::OCTET_STRING);
	}

	//encode required-fields - END

	//encode optional-fields - BEGIN
			//for encoding optional fields we do not use  NULL-fields we simply ommit the value instead (for storage/transmission efficiency)
			//each optional field has its constant position within the BER-encoded herein declared data-structure
			//the downside is that if value at index N is to be provided, all the values with index <= N needs to be provided as well.

	//encode optional-fields - END

	//finalize 
	enc.end_cons().end_cons();

	return enc.get_contents_unlocked();
}

/// <summary>
/// Instantiates a TransmissionToken from its BER-encoded representation.
/// </summary>
/// <param name="packedData"></param>
/// <returns></returns>
std::shared_ptr<CTransmissionToken> CTransmissionToken::instantiate(std::vector<uint8_t> packedData)
{
	try {
		std::shared_ptr<CTransmissionToken> token = std::make_shared<CTransmissionToken>();
		std::shared_ptr<CTools> tools = CTools::getInstance();
		Botan::BER_Decoder dec1 = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE);
		Botan::BER_Decoder dec2 = dec1.decode(token->mVersion).start_cons(Botan::ASN1_Tag::SEQUENCE);
		std::vector<uint8_t> temp;
		switch (token->mVersion)
		{
		case 1:

			//decode required-fields - BEGIN
			
			dec2.decode(token->mRevealedHash, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(reinterpret_cast<size_t&>(token->mBankIndex));
			dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			token->mBankUsageDepth = tools->BytesToBigInt(temp);
			dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			token->mRevealedHashesCount = tools->BytesToBigInt(temp);
			dec2.decode(token->mTokenPoolID, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(token->mDataHash, Botan::ASN1_Tag::OCTET_STRING);
			//decode required-fields - END

			//decode optional-fields - BEGIN
			
			dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			token->mValue = CTools::getInstance()->BytesToBigInt(temp);

			if (dec2.more_items())
			dec2.decode(token->mSig, Botan::ASN1_Tag::OCTET_STRING);
			if (dec2.more_items())
				dec2.decode(token->mRecipient, Botan::ASN1_Tag::OCTET_STRING);
				
			//decode optional-fields - END

			//finalize
			dec2.end_cons().end_cons();
			dec2.verify_end();
			return token;

		default:
			return nullptr;
			break;
		}
	}
	catch (...)
	{
		return nullptr;
	}
}
CTransmissionToken& CTransmissionToken::operator=(const CTransmissionToken& sibling)
{
	mRevealedHash = sibling.mRevealedHash;
	mTokenPoolID = sibling.mTokenPoolID;
	mDataHash = sibling.mDataHash;
	mSig = sibling.mSig;
	mBankIndex = sibling.mBankIndex;
	mVersion = sibling.mVersion;
	mRevealedHashesCount = sibling.mRevealedHashesCount;
	mBankUsageDepth = sibling.mBankUsageDepth;
	mValue = sibling.mValue;
	mRecipient = sibling.mRecipient;
	return *this;
}

bool operator==(const CTransmissionToken& c1, const CTransmissionToken& c2)
{
	std::shared_ptr<CTools>tools = CTools::getInstance();

	return (
		tools->compareByteVectors(c1.mRecipient, c2.mRecipient) &&
		tools->compareByteVectors(c1.mDataHash, c2.mDataHash) &&
		tools->compareByteVectors(c1.mRevealedHash, c2.mRevealedHash) &&
		tools->compareByteVectors(c1.mSig, c2.mSig) &&
		tools->compareByteVectors(c1.mTokenPoolID, c2.mTokenPoolID) &&
		c1.mBankIndex == c2.mBankIndex &&
		c1.mRevealedHashesCount == c2.mRevealedHashesCount &&
		c1.mBankUsageDepth == c2.mBankUsageDepth &&
		c1.mValue == c2.mValue &&
		c1.mVersion == c2.mVersion);
}

bool operator!=(const CTransmissionToken& c1, const CTransmissionToken& c2)
{
	return !(c1 == c2);
}
