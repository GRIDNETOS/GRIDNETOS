#pragma once
#include "stdafx.h"
#include <memory>
#include "enums.h"
#include <mutex>
class CTools;
class CStateDomain;
class CCryptoFactory;

class CGenesisRewards
{
private:
	std::vector <std::shared_ptr<CStateDomain>> mDomains;
	std::shared_ptr<CCryptoFactory> mCryptoFactory;
	eBlockchainMode::eBlockchainMode mMode;
	std::recursive_mutex mGuardian;
	std::shared_ptr<CTools> mTools;

public:

	bool parse(std::string content);
	CGenesisRewards(std::shared_ptr<CCryptoFactory> cryptoFactory, std::shared_ptr <CTools> tools, eBlockchainMode::eBlockchainMode mode);
	bool genGenesisRewardsJSON(std::string &JSONTxt, std::vector<uint8_t>&hash);

	//Add Awardees
	bool addAwardee( BigInt value, std::vector<uint8_t> & pubKey, Botan::secure_vector<uint8_t> & privKey);
	bool addAwardeeByAddr(std::vector<uint8_t> addr, BigInt value, std::shared_ptr<CIdentityToken> id =nullptr);
	bool addAwardee(std::vector<uint8_t> pubKey,BigInt value);
	bool addAwardeeBase58(std::string base58PubKey, BigInt value);
	bool addAwardee(Botan::secure_vector<uint8_t> privKey, BigInt value);


	bool getGenesisStateDomains(std::vector<std::shared_ptr<CStateDomain>> &domains);
	eBlockchainMode::eBlockchainMode  getBlockchainMode();
	bool reset();
};