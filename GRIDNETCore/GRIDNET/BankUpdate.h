#pragma once
#include "enums.h"
#include "botan_all.h"
#include <vector>
#include <mutex>

class CBankUpdate
{
private:

	std::vector<uint8_t> mPreCachedSeedHash;
	uint64_t mPreCachedSeedHashDepth;
	uint64_t mBankID;
	std:::vector<uint8_t> mPoolID;
	std::mutex mGuardian;
	uint64_t mVersion = 1;

public:
	CBankUpdate();
	CBankUpdate(uint64_t bankID,std::vector<uint8_t> preCachedSeedHash, uint64_t preCachedSeedHashDepth);
	uint64_t getPreCachedSeedHashDepth();
	setPreCachedSeedHashDepth(uint64_t depth);
	std::vector<uint8_t> getPoolID();
	setPoolID(std::vector<uint8_t> id);//for transmission efficiency this SHOULD NOT need to be set and rather set within an agreagtor
	uint64_t getBankID();
	setBankID(uint64_t bankID);
	static std::shared_ptr<CBankUpdate> instantiate(std::vector<uint8_t> data);
	std::vector<uint8_t> getPackedData();

	uint64_t getVersion();
	setVersion(uint64_t version);
}

