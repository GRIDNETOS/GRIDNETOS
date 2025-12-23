#pragma once
#include <stdafx.h>
#include "enums.h"

class CTransmissionToken;
class CTokenPoolBank;
class CCryptoFactory;
class CTokenPoolBank;
class CTokenPool;
class CDTI;

template <int decimals = 0, typename T>
T floor(T const& v)
{
	static const T scale = pow(T(10), decimals);

	if (v.is_zero())
		return v;

	// ceil/floor is found via ADL and uses expression templates for optimization
	if (v<0)
		return ceil(v*scale)/scale;
	else
		// floor is found via ADL and uses expression templates for optimization
		return floor(v*scale)/scale;
}

//Store of frozen assets. Used within state-less blockchain channels.
class CTokenPool : public std::enable_shared_from_this<CTokenPool>
{
private:
	
	std::vector<std::shared_ptr<CTokenPoolBank>> mBanks;
	std::shared_ptr<CCryptoFactory> mCryptoFactory;
	//synchronization primitives - BEGIN
	std::recursive_mutex mGuardian;
	//synchronization primitives - END

	//local member-fields -  BEGIN

	//hashes would be verified up-to it. (no need to go up till the mFinalHash thanks to it)
	//it is assumed that hashes/tokens in between mCurrentHash and mFinalHash have been already USED UP.

	uint64_t mDimensionsCount;// the number of banks/dimensions available within the pool. each pool has same number of tokens
	BigInt mDimensionDepth;// the total number of tokens(hashes) in all dimensions
	//the total number of tokens in each dimension = floor(mTotalValue/mDimensionsCount)
	BigInt mTotalValue;// total - sum of value stored within each bank/dimension. needs to match the Sacrificial Transaction; redundancy for convinience and offline validation purposes
	
	std::vector<uint8_t> mReceiptID;//ID of a transaction within which a sacrifice was made
	std::vector<uint8_t> mTokenPoolID; //random token-pool identifier
	Botan::secure_vector<uint8_t> mMasterSeedHash;
	std::string mFriendlyID; //used for user's convinience

	//local member-fields -  END
	std::vector<uint8_t> mOwnerID;
	uint64_t mVersion;
	eTokenPoolStatus::eTokenPoolStatus mStatus;
	std::shared_ptr<CTokenPoolBank> getBankById(uint64_t id);
	std::vector<uint8_t> mPubKey;

public:
	bool setPubKey(std::vector<uint8_t> pubKey);
	std::vector<uint8_t> getPubKey();
	CTokenPool(std::shared_ptr<CCryptoFactory> cf = nullptr, uint64_t dimensionsCount = 1, std::vector<uint8_t> ownerID = std::vector<uint8_t>(), std::vector<uint8_t> receiptID = std::vector<uint8_t>(),
		BigInt valuePerToken = 1, BigInt totalValue = 0, BigInt currentIndex = 0, std::string friendlyID = "",
		Botan::secure_vector<uint8_t> seedHash = Botan::secure_vector<uint8_t>(), std::vector<uint8_t> finalHash = std::vector<uint8_t>(), std::vector<uint8_t> currentHash = std::vector<uint8_t>());
	//wee need a factory-moded for this object in order for the other arch. rationale to be feasible under C++11 (shared_from_this)
	static std::shared_ptr<CTokenPool>getNew(std::shared_ptr<CCryptoFactory> cf = nullptr, uint64_t dimensionsCount = 1, std::vector<uint8_t> ownerID = std::vector<uint8_t>(), std::vector<uint8_t> receiptID = std::vector<uint8_t>(),
		BigInt valuePerToken = 1, BigInt totalValue = 0, BigInt currentIndex = 0, std::string friendlyID = "",
		Botan::secure_vector<uint8_t> seedHash = Botan::secure_vector<uint8_t>(), std::vector<uint8_t> finalHash = std::vector<uint8_t>(), std::vector<uint8_t> currentHash = std::vector<uint8_t>());
	
	eTokenPoolBankStatus::eTokenPoolBankStatus getBankStatus(uint64_t bankID);
	BigInt getValueLeftInBank(uint64_t bankID);
	void notifyBankStatusChanged(eTokenPoolBankStatus::eTokenPoolBankStatus status);
	std::vector<uint8_t> genTokenWorthValue(uint64_t bankID,BigInt value, BigInt& hashesConsumedCount, bool markUsed = true);
	std::shared_ptr<CTransmissionToken> getTTWorthValue(uint64_t bankID, BigInt value,bool markUsed=true);
	friend bool operator== (const CTokenPool& c1, const CTokenPool& c2);
	friend bool operator!= (const CTokenPool& c1, const CTokenPool& c2);
	BigInt getValueLeft();

	bool validate(bool requireSeed=true);
	void initFields();
	void setTotalValue(BigInt value);
	eTokenPoolStatus::eTokenPoolStatus getStatus();
	uint64_t getDimensionsCount();
	void setDimensionsCount(uint64_t count);
	void setStatus(eTokenPoolStatus::eTokenPoolStatus status);
	void resetBanks();
	bool getIsMasterSeedHashAvailable();

	//Token retrieval
	
	BigInt validateTT(std::shared_ptr<CTransmissionToken> token, bool updateState = true,const bool& isValid = false);
	bool generateDimensions(bool reportStatus=true,bool verify =true,std::shared_ptr<CDTI> dti = nullptr, std::shared_ptr<CTools> tools = CTools::getInstance());
	Botan::secure_vector<uint8_t> getSeedingHashForDimension(uint64_t dimensionID);
	bool setMasterSeedHash(Botan::secure_vector<uint8_t>  seed);
	Botan::secure_vector<uint8_t> getSeedHash();
	std::string getInfo(std::string newLine, bool describeDimensions=false,bool includeAnsiFormatting=true);

	//Token-Pool meta-data retrieval
	std::vector<uint8_t> getOwnerID();
	std::string getFriendlyID();
	void setFriendlyID(std::string id);
	BigInt getDimensionDepth();

	std::vector<uint8_t> getReceiptID();
	std::vector<uint8_t> getID();

	//serialization
	std::vector<uint8_t> getPackedData(bool includeSeed=true); //retrieve BER-encoded packed data
	//deserialization
	static std::shared_ptr<CTokenPool> instantiate(const std::vector<uint8_t> &packedData,std::shared_ptr<CCryptoFactory> cf, Botan::secure_vector<uint8_t> masterSeed= Botan::secure_vector<uint8_t>());

	BigInt getSingleTokenValue();
	BigInt getTotalValue();

	BigInt getTotalValueVerified(const CReceipt & receipt);

	uint64_t getVersion();
};


