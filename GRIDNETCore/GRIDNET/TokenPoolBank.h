#pragma once
#include <stdafx.h>
#include "enums.h"

class CTokenPool;
class CTransmissionToken;

class CTokenPoolBank
{
private:
	uint64_t mID;//index in array.//not serialized
	void initFields();
	
	BigInt mCurrentDepth;//a token pool would be marked as invalid after just a single double-spend attempt (serves as a stake)
	std::recursive_mutex mGuardian;
	eTokenPoolBankStatus::eTokenPoolBankStatus mStatus;
	std::weak_ptr<CTokenPool> mPool;
	std::vector<uint8_t> mFinalHash; //can't get further than that; used for boundaries checking.
	Botan::secure_vector<uint8_t> mSeedHash; //initial seed-hash (warning - SECRET value);
	BigInt mCurrentIndex;//NOT SERIALIZED. optimization
	std::vector<uint8_t> mCurrentFinalHash;//at current DEPTH (optimization). Serialized public value.

	//pre-cached seed hash (provided by mobile app) - BEGIN
	std::vector<uint8_t> mPreCachedSeedHash;
	BigInt mPreCachedSeedHashDepth;
	//pre-cached seed hash (provided by mobile app) - END

	//optional fields - BEGIN

	//optional fields - END
public:
	void reset();
	bool setSeedHash(Botan::secure_vector<uint8_t> seed);
	friend bool operator== (const CTokenPoolBank& c1, const CTokenPoolBank& c2);
	friend bool operator!= (const CTokenPoolBank& c1, const CTokenPoolBank& c2);

	bool setPreCachedSeedHash(std::vector<uint8_t> seed, BigInt depth);
	BigInt getPreCachedSeedHashDepth();

	eTokenPoolBankStatus::eTokenPoolBankStatus getStatus();
	CTokenPoolBank(uint64_t bankID, std::shared_ptr<CTokenPool> pool = nullptr, std::vector<uint8_t> finalHash = std::vector<uint8_t>(),
		BigInt currentDepth=0, Botan::secure_vector<uint8_t> seedHash = Botan::secure_vector<uint8_t>());



	std::string getInfo(std::string newLine);
	void setPool(std::weak_ptr<CTokenPool> pool);
	void setStatus(eTokenPoolBankStatus::eTokenPoolBankStatus status);
	std::vector<uint8_t> getNextHash(bool markUsed = true);
	std::vector<uint8_t> genTokenWorthValue(BigInt value, BigInt& hashesConsumedCount, bool markUsed = true);
	std::shared_ptr<CTransmissionToken> genTTWorthValue(BigInt value, bool markUsed);
	std::vector<uint8_t> getFinalHash(bool currentOne = true);
	std::vector<uint8_t> getHashAtDepth(BigInt depth, bool updateState = true);
	void setCurrentHash(std::vector<uint8_t> currentHash);
	std::vector<uint8_t> getCurrentHash();
	BigInt getCurrentDepth();
	void setCurrentDepth(BigInt index);
	BigInt getValueLeft();

	BigInt getTotalValue();

};
