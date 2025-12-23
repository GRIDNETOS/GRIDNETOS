#include "GenesisRewards.h"
#include "CryptoFactory.h"
#include "StateDomain.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "IdentityToken.h"
using namespace rapidjson;

/// <summary>
/// Generated internal data based on factFileContent (JSON).
/// WARNING: the hash of the factFileContent NEEDS to be verified within the CVerifier first!
/// </summary>
/// <param name="content"></param>
/// <returns></returns>
bool CGenesisRewards::parse(std::string content)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	try {
		if (content.size() == 0)
			return false;
		mDomains.clear();
		Document document;
		document.Parse(content.c_str(), content.size());

		if (!document.IsObject())
			return false;
		if (!document.HasMember("rewardees"))
			return false;

		Value rewardees = document["rewardees"].GetArray();
		std::string ID, balanceStr,tmp;
		BigInt balance = 0;
		BigFloat f2 = BigFloat(200000);
		std::shared_ptr<CIdentityToken> id;
		uint64_t stub = 0;
		std::string pubKey;
		std::vector<uint8_t> pubKeyB;
		BigInt stake = 0;
		std::string friendlyID;

		std::vector<std::string> systemDomains = { "TheFund" };
		eIdentityTokenType::eIdentityTokenType iType = eIdentityTokenType::Basic;
		for (uint64_t i = 0; i < rewardees.Size(); i++)
		{
			//Reset Variables - BEGIN
			friendlyID.clear();
			pubKey.clear();
			pubKeyB.clear();
			stake = 0;
			stub = 0;
			id = nullptr;
			iType = eIdentityTokenType::Basic;
			//Reset Variables - END

			if (!rewardees[i].HasMember("domainID") || !rewardees[i].HasMember("balance"))
				return false;

			ID = std::string(rewardees[i]["domainID"].GetString());
			try {

				if (rewardees[i].HasMember("friendlyID") || rewardees[i].HasMember("pubKey"))
				{
					if (rewardees[i].HasMember("friendlyID"))
					{
						friendlyID = rewardees[i]["friendlyID"].GetString();
					}
				
					if (rewardees[i].HasMember("pubKey"))
					{
						pubKey = rewardees[i]["pubKey"].GetString();
						if (!mTools->base58CheckDecode(pubKey, pubKeyB, false))
							throw "Invalid Pub-Key";
					}

					if (rewardees[i].HasMember("stake"))
					{
						stake = BigInt(rewardees[i]["stake"].GetString());
						iType = eIdentityTokenType::Stake;
					}

					id = std::make_shared<CIdentityToken>(iType, mTools->stringToBytes(pubKey), nullptr, stake, std::vector<uint8_t>(), std::vector<uint8_t>(), std::vector<uint8_t>(), friendlyID);

				}
				else
					id = nullptr;


				balanceStr = std::string(rewardees[i]["balance"].IsString() ? rewardees[i]["balance"].GetString() : std::to_string(rewardees[i]["balance"].GetUint64()));
				balance = BigInt(balanceStr);

				/*one Time Things - BEGIN
				ICO price per GRIDNET COIN = 0.1 USD
				200 000 GBUs (obsolete) = 500 USD;
				which gives 5000 GRIDNET Coins for 200 000 GBUs (app download)
				for each 200 000 GBUs we now need to handle 5000 GRIDNET Coins
				each account needs this to end-up with (balance/200 000) * 5000 GNC
				*/
				
				//if (std::find(systemDomains.begin(), systemDomains.end(), friendlyID) == systemDomains.end())
				//{
				//	balance = (static_cast<BigInt>(static_cast<BigFloat> (balance) / f2) * CGlobalSecSettings::getAttoUnits(5000));
				//}
				//one Time Things - END

				std::shared_ptr<CStateDomain> dom = std::make_shared<CStateDomain>(mMode);
				dom->setAddress(mTools->stringToBytes(ID));
				dom->setPendingPreTaxBalanceChange(balance);
				dom->setTempIDToken(id);
				mDomains.push_back(dom);
			}
			catch (...)
			{
				mTools->writeLine(mTools->getColoredString("Invalid GENESIS data for: " + ID, eColor::lightPink));
			}
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

CGenesisRewards::CGenesisRewards(std::shared_ptr<CCryptoFactory> cryptoFactory, std::shared_ptr <CTools> tools,eBlockchainMode::eBlockchainMode mode)
{
	mCryptoFactory = cryptoFactory;
	mMode = mode;
	mTools = tools;
}
/// <summary>
/// Generates the GenesisReward JSON file to be used as a 'fact-file' by the Genesis Verifiable.
/// Returns hash of the generated file. The hash is hard-coded within the source-code.
/// </summary>
/// <param name="JSONTxt"></param>
/// <param name="hash"></param>
/// <returns></returns>
bool CGenesisRewards::genGenesisRewardsJSON(std::string & JSONTxt, std::vector<uint8_t>& hash)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mDomains.size() == 0)
		return false;


	try {
		std::string result;
		Document document;
		document.SetObject();
		Document::AllocatorType& allocator = document.GetAllocator();
		Value domains(kArrayType);
		std::string pubKeyB58;
		std::shared_ptr<CIdentityToken> id;
		std::string fID;
		BigInt stake;
		std::string stakeStr;
		for (uint64_t i = 0; i < mDomains.size(); i++)
		{
			id = mDomains[i]->getTempIDToken();

			Value domain(kObjectType);
			std::string adr = mTools->bytesToString(mDomains[i]->getAddress());
			//for the below we use a mem-copy based contructor.
			//From docs: "(..)And it assumes the input is null-terminated and calls a strlen()-like function to obtain the length."
			domain.AddMember("domainID",Value().SetString(adr.c_str(), allocator), allocator); //address is base58 already.
			domain.AddMember("balance",Value().SetString(mDomains[i]->getPendingPreTaxBalanceChange().str().c_str(), allocator), allocator);

			if (id)
			{
				fID = id->getFriendlyID();

				if (fID.size())
				{
					domain.AddMember("friendlyID",Value().SetString(fID.c_str(), allocator), allocator);
				}

				stake = id->getConsumedCoins();

				if (stake)
				{
					stakeStr = stake.str();
					domain.AddMember("stake",Value().SetString(stakeStr.c_str(), allocator), allocator);
				}


				if (id->getPubKey().size())
				{
					pubKeyB58 = mTools->base58CheckEncode(id->getPubKey());
					domain.AddMember("pubKey",Value().SetString(pubKeyB58.c_str(), allocator), allocator);
				}


			
			}
			domains.PushBack(domain, allocator);
			
		}
		document.AddMember("rewardees", domains, document.GetAllocator());

		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		document.Accept(writer);
		result = buffer.GetString();
		JSONTxt = result;
		hash = mCryptoFactory->getSHA2_256Vec(mTools->stringToBytes(result));//depands largely on formatting/encoding but we can  live with that easily.
		return true;
	}
	catch (...)
	{
		return false;
	}
	return false;
}
bool CGenesisRewards::addAwardeeBase58(std::string base58PubKey, BigInt value)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> pubKeyRAW;
	if (!mTools->base58CheckDecode(base58PubKey, pubKeyRAW))
		return false;
	return addAwardee(pubKeyRAW, value);

}
bool CGenesisRewards::getGenesisStateDomains(std::vector<std::shared_ptr<CStateDomain>>& domains)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mDomains.size() > 0)
	{
		domains = mDomains;
		return true;
	}
	return false;
}
eBlockchainMode::eBlockchainMode CGenesisRewards::getBlockchainMode()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mMode;
}
bool CGenesisRewards::addAwardee(Botan::secure_vector<uint8_t> privKey, BigInt value)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> pubKey = mCryptoFactory->getPubFromPriv(privKey);//verification inside
	if (pubKey.size()!=32)
		return false;

	return addAwardee(pubKey, value);
}
bool CGenesisRewards::reset()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mDomains.clear();
	return true;
}
//
bool CGenesisRewards::addAwardee(BigInt value, std::vector<uint8_t>& pubKey, Botan::secure_vector<uint8_t>& privKey)
{
	
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!mCryptoFactory->genKeyPair(privKey, pubKey))
		return false;

	return addAwardee(pubKey, value);

}

bool CGenesisRewards::addAwardeeByAddr(std::vector<uint8_t> addr, BigInt value, std::shared_ptr<CIdentityToken> id )
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	
	if (!mTools->isDomainIDValid(addr))
		return false;
	uint64_t stub = 0;
	std::shared_ptr<CStateDomain> domain = std::make_shared<CStateDomain>(mMode);
	domain->setTempIDToken(id);
	domain->setAddress(addr);
	if (!domain->setPendingPreTaxBalanceChange(value))
		return false;
	mDomains.push_back(domain);

	return true;
}

bool CGenesisRewards::addAwardee(std::vector<uint8_t> pubKey, BigInt value)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> address;

	if (!mCryptoFactory->genAddress(pubKey, address, mMode))
		return false;

		std::shared_ptr<CStateDomain> domain = std::make_shared<CStateDomain>(mMode);
		domain->setAddress(address);

		if (!domain->setPendingPreTaxBalanceChange(static_cast<BigSInt>(value)))
			return false;
		mDomains.push_back(domain);

	return true;
}
