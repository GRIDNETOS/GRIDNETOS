#pragma once
#include "stdafx.h"

class CTransmissionToken
{
private:

	std::recursive_mutex mGuardian;
	//Full-Node needs to verify :
	/*
	1) The existence of the Token Pool -> then reported owner and signature
	2) Check for Double-spend attempts through
	   + does the reported hash corespond to the reported depth?
	      - to verify that, the full node does hashing all the way to the final hash registered within the Token Pool
	3) during reward assignment, the full-node needs to:
		+ calculate GNC reward 
		+ update accounts' balance
		+ update token-pool's usage (to the latest mTokenPoolUsageDepth)

	*/
	//REQUIRED FIELDS - BEGIN
	std::vector<uint8_t> mRevealedHash;//the just revealed hash NEEDS TO be verified by full-node
	std::vector<uint8_t> mTokenPoolID;
	std::vector<uint8_t> mDataHash; // so that it cannot be reused for other data/value-item
	std::vector<uint8_t> mSig; // we need accountability of double-spends
	uint64_t mBankIndex;
	uint64_t mVersion;
	//REQUIRED FIELDS - END

	//Some of the data-fields are OPTIONAL mainly for data-transmission efficiency.

	//OPTIONAL FIELDS - BEGIN
	BigInt mRevealedHashesCount;//how many hashes are being uncovered? (Note: many hashes can be uncovered through a single mRevealedHash)
	//(OPTIONAL IF)  revealed hash at most 1K positions away from the current one (as reported by current state of Token Pool within VM)
	//full-node will need to go through these anyway
	BigInt mBankUsageDepth;//NEEDS TO be verified by full-node
	//(OPTIONAL IF) revealed hash  at most 1K positions away

	
	BigInt mValue;//GNC value other node might want to trust it
	//(OPTIONAL) -> full node needs to verify the value anyway. Still, might be useful for certain kinds of low priority off-the-chain transactions from 
	//the rewardee's perspective.

	std::vector<uint8_t> mRecipient;
	//OPTIONAL FIELDS - END


public:
	CTransmissionToken(const CTransmissionToken& sibling);
	void initFields();
	CTransmissionToken(uint64_t bankID = 0, BigInt value = 0, std::vector<uint8_t> revealedHash = std::vector<uint8_t>(), BigInt RevealedHashesCount = 0, BigInt tokenPoolUsageDept = 0,
		std::vector<uint8_t> dataHash = std::vector<uint8_t>(), std::vector<uint8_t> tokenPoolID = std::vector<uint8_t>(),
		std::vector<uint8_t> transmissionTokenID = std::vector<uint8_t>(),std::vector<uint8_t> sig = std::vector<uint8_t>());
	void setRecipient(std::vector<uint8_t> recipient);
	std::vector<uint8_t> getRecipient();
	uint64_t getVersion();
	bool sign(Botan::secure_vector<uint8_t> privKey);
	bool verifySignature(std::vector<uint8_t> mPubKey);
	uint64_t getBankID();
	void setBankID(uint64_t id);
	std::vector<uint8_t> getRevealedHash();
	BigInt getRevealedHashesCount();
	void setRevealedHashesCount(BigInt count);
	BigInt getCurrentDepth();
	BigInt getValue();
	void setValue(BigInt value);
	std::vector<uint8_t> getDataHash();
	std::vector<uint8_t> getID();
	std::vector<uint8_t> getSig();

	bool validate();

	std::vector<uint8_t> getPackedData(bool includeSig=true);
	static std::shared_ptr<CTransmissionToken> instantiate(std::vector<uint8_t> data);

	CTransmissionToken& operator=(const CTransmissionToken& sibling);

	friend bool operator== (const CTransmissionToken& c1, const CTransmissionToken& c2);
	friend bool operator!= (const CTransmissionToken& c1, const CTransmissionToken& c2);

};