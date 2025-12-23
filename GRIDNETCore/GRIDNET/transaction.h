#ifndef TRANSACTION_H
#define TRANSACTION_H
#pragma once
#include "stdafx.h"
#include "transitpool.h"
#include "transonion.h"

#include <cassert>
#include "CryptoFactory.h"

#include "TrieLeafNode.h"
class CEndPoint;
class CConversation;
//#include "EEndPoint.h"

/// <summary>
/// CTransaction constitutes a wrapper around GridScript code.
/// CTransaction is signed (for accountability AND integrity verification purposes) ADDITIONALY
/// GridScript code within it might contain AND/OR require one or multiple additioanal signatures.
/// Thus, CTransaction is a signed capsule containing any GridScript/ tasks for the decentralized Gridnet vm.
/// 
/// Explanation: a simple transaction cotains its timestamp and ERG price/limit. We need to authenticate these information (the latter especially).
/// if verification of the signature guarding the CTransaction structure fails; execution of the GridScript code within it won't commence.
/// The CTransaction signature is verified at the host level while signatures within the embedded GridScript are verified
/// at the GridScript VM level.
/// 
/// ERG price/erg limits are passed to the GridScript VM.
/// 
/// 
/// </summary>
class CTransaction  : public CTrieLeafNode {
	
public:
	ColdStorageProperties ColdProperties;//IMPORTANT: ColdProperties *ARE NOT* signed by the author of the transaction.
		//these Flags are set and validated by validators. i.e miners.
	//these fields are protected by the consensus mechanism. i.e. consecutive blocks on top of each other.
	CTransaction & operator=(const CTransaction & t);
	CTransaction();
	CTransaction( const CTransaction  &sibling);
	static CTransaction * instantiate(std::vector<uint8_t> &serializedData,eBlockchainMode::eBlockchainMode blockchainMode);
	bool isValid();
	bool setCode(std::vector<uint8_t> code);
	bool setExtData(std::vector<uint8_t> data);
	
	bool verifySignature(std::vector<uint8_t> publicKey);

	size_t getTime();
	std::string getSourceCode();// only for caching purposes
	void setSourceCode(const std::string& sourceCode);// only for caching purposes
	BigInt getErgPrice();
	uint64_t getNonce();
	void setNonce(uint64_t nonce);
	BigInt getErgLimit();
	void setERGPrice(BigInt price);
	void setERGLimit(BigInt limit);
	bool destroyEnvelopeSig();
	bool sign(Botan::secure_vector<uint8_t> privateKey);
	void setSignatureBytes(std::vector<uint8_t> sig);
	std::vector<uint8_t> getDataToSign();
	bool getConcatData(std::vector<uint8_t>&data, bool includePubKey=true,bool includeSig=true);
	//uint64_t getTransactionSubtype();
	std::vector<uint8_t> getExtData();
	std::vector<uint8_t> getSignature();
	size_t getVersion();
	void setIssuer(std::vector<uint8_t> issuerID);
	std::vector<uint8_t> getIssuer();
	//std::vector<uint8_t> getGUID() const;
	std::vector<uint8_t> getCode() const;

	void markAsProcessed();
	bool wasProcessed();
	bool prepare(bool store =true,bool includeSig=true);

	std::vector<uint8_t> getPackedData(bool omitSig=false);

	bool destroyPubKey();

	bool clearPubKey();

	//hash of the transaction header
	static CTransaction * genNode(CTrieNode **node);
	static void extractV2(Botan::BER_Decoder& dec2, std::vector<uint8_t>& ColdPropertiedBytes, CTransaction* tr, std::vector<uint8_t>& temp, std::shared_ptr<CTools>& tools);
	static void decodeV1(Botan::BER_Decoder& dec2, std::vector<uint8_t>& ColdPropertiedBytes, CTransaction* tr, uint64_t& tempUInt, std::vector<uint8_t>& temp, std::shared_ptr<CTools>& tools);
	size_t getReceivedAt();
	std::vector<uint8_t> getPubKey();
	bool setPubKey(std::vector<uint8_t> pubKey);
	std::shared_ptr<CEndPoint> getIssuingEndpoint();
	void setIssuingEndpoint(std::shared_ptr<CEndPoint> endpoint);
	std::shared_ptr<CConversation> getIssuingConversation();
	void setIssuingConversation(std::shared_ptr<CConversation> conversation);

private:
	std::string mSourceCode;//// only for caching purposes
	std::shared_ptr <CEndPoint> mEndpoint;//NOT serialized
	std::shared_ptr <CConversation> mConversation;//NOT serialized
	size_t mOriginalVersion;
	mutable std::recursive_mutex mGuardian;
	//bool genID();
	std::vector<uint8_t> internalsPrepare(bool ommitSignature = false);

    std::shared_ptr<CCryptoFactory> mCf;
	size_t mReceivedAt;
	bool verifyInitCode(std::vector<uint8_t> code);
    std::vector<uint8_t> mCode;
	std::vector<uint8_t> mIssuer;
	std::vector<uint8_t> mPubKey;
   // std::vector<uint8_t> mGUID;
    size_t mTimestamp=0;
	BigInt mErgPrice=0;
	BigInt mErgLimit=0;
    std::vector<uint8_t> mExtData;//any kind of additional information.
	//Note: the above equals to:
	//VerifiableID - in case of 
    size_t mValue=0;//used for Block Rewards AND 'Sacrifices'
	//Note: a 'Sacrificial Transaction' has an appropriate Bit set in ColdStorage flags.
    size_t mVersion=0;
    size_t mLockTime=0;
	uint64_t mNonce = 0;
	//size_t mTransactionSubtype;
    std::vector<uint8_t> mSignature;

};

#endif
