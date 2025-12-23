#pragma once

#include "stdafx.h"
#include "identitytoken.h"
#include "CryptoFactory.h"
#include "BlockchainManager.h"
#include "enums.h"
#include "DataConcatenator.h"

std::string CIdentityToken::getPublicKeyTxt()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	if (mPublicKey.empty())
		return "";

	std::shared_ptr<CTools> tools = CTools::getInstance();
	return tools->base58CheckEncode(mPublicKey);
}

CIdentityToken::CIdentityToken(const CIdentityToken & sibling)
{
	mTokenPool = sibling.mTokenPool;
	mSignature = sibling.mSignature;
	mVersion = sibling.mVersion;
	mType = sibling.mType;
		mPublicKey = sibling.mPublicKey;
		mFriendlyID = sibling.mFriendlyID;
		mAdditionalData = sibling.mAdditionalData;

			mPow = sibling.mPow;

			mConsumedCoins = sibling.mConsumedCoins;
			mConsumedInTXReceiptID = sibling.mConsumedInTXReceiptID;

			mSenderAddress = sibling.mSenderAddress;
}

std::shared_ptr<CTokenPool> CIdentityToken::getTokenPoolP1()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mTokenPool;
}

bool CIdentityToken::getKeyChain(CKeyChain & chain,std::shared_ptr<CBlockchainManager> bm)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	if (bm == nullptr)
		return false;
	//check if key-chain is available locally
	std::vector<uint8_t> adr;
	bm->getCryptoFactory()->genAddress(mPublicKey, adr);

	return bm->getSettings()->getCurrentKeyChain(chain,false, true, std::string(adr.begin(), adr.end()));

}

eIdentityTokenType::eIdentityTokenType CIdentityToken::getType()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mType;
}
void CIdentityToken::setType(eIdentityTokenType::eIdentityTokenType type)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mType = type;
}
void CIdentityToken::initFields()
{
	mVersion = 1;
	mConsumedCoins = 0;

}

CIdentityToken::CIdentityToken()
{
	initFields();
}
/*
bool CIdentityToken::sign(Botan::secure_vector<uint8_t> privKey)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	CDataConcatenator concat;
	switch (mVersion)
	{
	case 1:
		concat.add(mVersion);
		concat.add(mPublicKey);
		concat.add(mFriendlyID);
		concat.add(mAdditionalData);
		concat.addGSC(mPow->getNonce());
		concat.addGSC(mPow->getNrOfConcats());
		concat.add(mConsumedCoins);
		concat.add(mConsumedInTXReceiptID);
		concat.add(mSenderAddress);
		break;
	default:
		return false;
	}

	mSignature = CCryptoFactory::getInstance()->signData(concat.getData(), privKey);
	return true;
}*/
bool  CIdentityToken::verifySig()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	CDataConcatenator concat;
	switch (mVersion)
	{
	case 1:
		concat.add(mVersion);
		concat.add(mPublicKey);
		concat.add(mFriendlyID);
		concat.add(mAdditionalData);
		concat.addGSC(mPow->getNonce());
		concat.addGSC(mPow->getNrOfConcats());
		concat.addBigInt(mConsumedCoins);
		concat.add(mConsumedInTXReceiptID);
		concat.add(mSenderAddress);
		break;
	default:
		return false;
	}

	return CCryptoFactory::getInstance()->verifySignature(mSignature, concat.getData(), mPublicKey);
}



std::shared_ptr<CIdentityToken> CIdentityToken::instantiate(std::vector<uint8_t> serializedToken)
{
	std::shared_ptr<CIdentityToken> token = std::make_shared<CIdentityToken>();
	try {
		Botan::BER_Decoder  dec = Botan::BER_Decoder(serializedToken).start_cons(Botan::ASN1_Tag::SEQUENCE).
			decode(token->mVersion);


		Botan::BER_Object obj;

		if (token->mVersion == 1)
		{
			std::vector<uint8_t> adr;
			obj = dec.get_next_object();
			if (obj.type_tag != Botan::ASN1_Tag::SEQUENCE)
				throw "error";

			//Commons - BEGIN
			Botan::BER_Decoder  dec2 = Botan::BER_Decoder(obj.value);
			size_t result = 0;
			eIdentityTokenType::eIdentityTokenType type;

			// Diagnostic logging before decode attempt
			CTools::getTools()->logEvent(
				std::string("CIdentityToken::instantiate() - Attempting to decode Type field, inner sequence size: ") +
				std::to_string(obj.value.size()) + " bytes",
				eLogEntryCategory::debug,
				3,
				eLogEntryType::notification
			);

			// C++ is reference implementation - decode using CONTEXT_SPECIFIC tags
			dec2.decode(result, Botan::ASN1_Tag::INTEGER);
			token->mType = static_cast<eIdentityTokenType::eIdentityTokenType> (result);
			dec2.decode(token->mPublicKey, Botan::ASN1_Tag::OCTET_STRING);
			std::vector<uint8_t> friendlyID;
			dec2.decode(friendlyID, Botan::ASN1_Tag::OCTET_STRING);
			token->mFriendlyID = std::string(friendlyID.begin(), friendlyID.end());
			dec2.decode(token->mSignature, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(token->mAdditionalData, Botan::ASN1_Tag::OCTET_STRING);
			//Commons - END

			if (token->getType() == eIdentityTokenType::eIdentityTokenType::Hybrid ||
				token->getType() == eIdentityTokenType::eIdentityTokenType::PoW)
			{
				//PoW - BEGIN
				std::vector<uint8_t> hash;
				dec2.decode(hash, Botan::ASN1_Tag::OCTET_STRING);
				size_t nonce = 0;
				dec2.decode(nonce, Botan::ASN1_Tag::INTEGER);
				size_t concats = 0;
				dec2.decode(concats, Botan::ASN1_Tag::INTEGER);
				//instantiate and associate a CPoW instance
				std::shared_ptr<CPoW> pow = std::make_shared<CPoW>(static_cast<uint32_t>(nonce), hash, false);
				pow->setNrOfConcats(concats);
				token->mPow = pow;
				//PoW - END

			}

			if (token->getType() == eIdentityTokenType::eIdentityTokenType::Hybrid ||
				token->getType() == eIdentityTokenType::eIdentityTokenType::Stake)
			{
				//Proof-of-Stake - BEGIN
				size_t consumedCoins = 0;
				std::vector<uint8_t> temp;
				dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				token->mConsumedCoins = CTools::getInstance()->BytesToBigInt(temp);
				dec2.decode(token->mConsumedInTXReceiptID, Botan::ASN1_Tag::OCTET_STRING);
				dec2.decode(token->mSenderAddress, Botan::ASN1_Tag::OCTET_STRING);
				//Proof-of-Stake - END
			}
			
			if(dec2.more_items() || dec.more_items())
				throw "error";
			return token;
		}
		else
		{
			return nullptr;
		}

	}
	catch (const Botan::Decoding_Error& e)
	{
		// Specific Botan BER/DER decoding errors
		CTools::getTools()->logEvent(
			std::string("CIdentityToken::instantiate() - Botan decoding error: ") + e.what(),
			eLogEntryCategory::debug,
			1,
			eLogEntryType::failure
		);
		return nullptr;
	}
	catch (const Botan::Exception& e)
	{
		// General Botan exceptions (cryptographic operations, etc.)
		CTools::getTools()->logEvent(
			std::string("CIdentityToken::instantiate() - Botan exception: ") + e.what(),
			eLogEntryCategory::debug,
			1,
			eLogEntryType::failure
		);
		return nullptr;
	}
	catch (const std::exception& e)
	{
		// Standard C++ exceptions
		CTools::getTools()->logEvent(
			std::string("CIdentityToken::instantiate() - Standard exception: ") + e.what(),
			eLogEntryCategory::debug,
			1,
			eLogEntryType::failure
		);
		return nullptr;
	}
	catch (const char* e)
	{
		// String literal exceptions (like "throw \"error\"" in the code)
		CTools::getTools()->logEvent(
			std::string("CIdentityToken::instantiate() - String exception: ") + e,
			eLogEntryCategory::debug,
			1,
			eLogEntryType::failure
		);
		return nullptr;
	}
	catch (...)
	{
		// Catch any remaining unknown exceptions
		CTools::getTools()->logEvent(
			"CIdentityToken::instantiate() - Unknown exception during token deserialization",
			eLogEntryCategory::debug,
			1,
			eLogEntryType::failure
		);
		return nullptr;
	}
}


//instantiate a token with whatever you desire. it will be verified with verifyToken() later on anyway.
CIdentityToken::CIdentityToken(eIdentityTokenType::eIdentityTokenType type , std::vector<uint8_t> pubKey, std::shared_ptr<CPoW> pow, BigInt consumedCoins, std::vector<uint8_t> IOTAddress, std::vector<uint8_t> receiptID, std::vector<uint8_t> additionalData,std::string friendlyID)
{
	initFields();
	mType = type;
	mPublicKey = pubKey;
	mPow = pow;
	mConsumedCoins = consumedCoins;
	mSenderAddress = IOTAddress;
	mAdditionalData = additionalData;
	mConsumedInTXReceiptID = receiptID;
	setFriendlyID(friendlyID);

}

std::vector<uint8_t> CIdentityToken::getPackedData(bool ommitSig)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	size_t nonce = mPow!=nullptr? mPow->getNonce():0;
	std::vector<uint8_t> sig;
	if (!ommitSig)
		sig = mSignature;
	Botan::DER_Encoder enc;
	 enc = enc.start_cons(Botan::ASN1_Tag::SEQUENCE).encode(mVersion)
		//Commons - BEGIN
		.start_cons(Botan::ASN1_Tag::SEQUENCE)
		.encode(static_cast<uint64_t>(mType), Botan::ASN1_Tag::INTEGER)
		.encode(mPublicKey, Botan::ASN1_Tag::OCTET_STRING)
		.encode(CTools::getTools()->stringToBytes(mFriendlyID), Botan::ASN1_Tag::OCTET_STRING)
		.encode(sig, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mAdditionalData, Botan::ASN1_Tag::OCTET_STRING);
		//Commons - END
	 if (mPow!=nullptr &&(mType == eIdentityTokenType::eIdentityTokenType::Hybrid ||
		 mType == eIdentityTokenType::eIdentityTokenType::PoW))
	 {
		 //PoW - BEGIN
		enc =  enc.encode(mPow->getHash(), Botan::ASN1_Tag::OCTET_STRING)
			 .encode(static_cast<size_t>(mPow->getNonce()), Botan::ASN1_Tag::INTEGER)
			 .encode(static_cast<size_t>(mPow->getNrOfConcats()), Botan::ASN1_Tag::INTEGER);
	 }
		//PoW - END
	 if (mType == eIdentityTokenType::eIdentityTokenType::Hybrid ||
		 mType == eIdentityTokenType::eIdentityTokenType::Stake)
	 {
		 //Proof-of-Stake - BEGIN
		enc = enc.encode(CTools::getInstance()->BigIntToBytes(mConsumedCoins), Botan::ASN1_Tag::OCTET_STRING)
			 .encode(mConsumedInTXReceiptID, Botan::ASN1_Tag::OCTET_STRING)
			 .encode(mSenderAddress, Botan::ASN1_Tag::OCTET_STRING);
			 //Proof-of-Stake - END
	 }

	 enc = enc.end_cons().end_cons();
	 return enc.get_contents_unlocked();
}

void CIdentityToken::setVersion(size_t version)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mVersion = version;
}

size_t CIdentityToken::getVersion()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVersion;
}

void CIdentityToken::setPubKey(std::vector<uint8_t> pubKey)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mPublicKey = pubKey;
}

std::vector<uint8_t> CIdentityToken::getPubKey()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mPublicKey;
}



std::vector<uint8_t> CIdentityToken::getSenderAddress()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mSenderAddress;
}

void CIdentityToken::setSenderAddress(std::vector<uint8_t> addr)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mSenderAddress = addr;
}

uint64_t CIdentityToken::getRank()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return 1;
}

std::string CIdentityToken::getFriendlyID() {
	std::lock_guard<std::mutex> lock(mGuardian);
	return this->mFriendlyID;
	}

	void CIdentityToken::setFriendlyID(std::string id) {
		std::lock_guard<std::mutex> lock(mGuardian);
		this->mFriendlyID = id;
	}

	void CIdentityToken::setSignature(std::vector<uint8_t> sig)
	{
		std::lock_guard<std::mutex> lock(mGuardian);
		mSignature = sig;
	}

	std::vector<uint8_t> CIdentityToken::getSignature()
	{
		std::lock_guard<std::mutex> lock(mGuardian);
		return mSignature;
	}

	std::shared_ptr<CPoW> CIdentityToken::getPow() {
		std::lock_guard<std::mutex> lock(mGuardian);  return this->mPow;
	}

	void CIdentityToken::setPow(std::shared_ptr<CPoW> pow) {
		std::lock_guard<std::mutex> lock(mGuardian); this->mPow = pow;
	}

	
	void CIdentityToken::setConsumedCoins(BigInt consumedCoins)
	{
		std::lock_guard<std::mutex> lock(mGuardian); mConsumedCoins = consumedCoins;
	}

	BigInt CIdentityToken::getConsumedCoins()
	{
		std::lock_guard<std::mutex> lock(mGuardian); return mConsumedCoins;
	}

	void CIdentityToken::setTXReceiptID(std::vector<uint8_t> id)
	{
		std::lock_guard<std::mutex> lock(mGuardian); mConsumedInTXReceiptID = id;
		
	}

	std::vector<uint8_t> CIdentityToken::getTXReceiptID()
	{
		std::lock_guard<std::mutex> lock(mGuardian); return mConsumedInTXReceiptID;
	}
