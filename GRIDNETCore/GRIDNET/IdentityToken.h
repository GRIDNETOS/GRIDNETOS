#ifndef IDENTITY_TOKEN_H
#define IDENTITY_TOKEN_H

#include "stdafx.h"
class CTokenPool;
class CBlockchainManager;
class CIdentityToken
{
	
private:

	std::mutex mGuardian;
	std::shared_ptr<CTokenPool> mTokenPool;
	std::vector<uint8_t> mSignature;
	size_t mVersion;
	eIdentityTokenType::eIdentityTokenType mType;
	std::vector<uint8_t> mPublicKey;
    std::string mFriendlyID; //Nickname
	std::vector<uint8_t> mAdditionalData;
	//PoW Section
    std::shared_ptr<CPoW> mPow;
	//Sacrificed Coins Section
	BigInt mConsumedCoins;
	std::vector<uint8_t> mConsumedInTXReceiptID;
	//IoT Section
	std::vector<uint8_t> mSenderAddress;
	//Cyber-Physical / Power section
public:
	std::string getPublicKeyTxt();
	CIdentityToken(const CIdentityToken &sibling);
	std::shared_ptr<CTokenPool> getTokenPoolP1();
	bool getKeyChain(CKeyChain&chain,std::shared_ptr<CBlockchainManager> bm);
	eIdentityTokenType::eIdentityTokenType getType();
	void setType(eIdentityTokenType::eIdentityTokenType type);
	void initFields();
	CIdentityToken();
	//bool sign(Botan::secure_vector<uint8_t> privKey); the id token does NOT need to be signed it suffices for it to be within the state-domain.
	//its inclusion was authenticated already
	bool verifySig();
	static std::shared_ptr<CIdentityToken> instantiate(std::vector<uint8_t> data);

	CIdentityToken(eIdentityTokenType::eIdentityTokenType type, std::vector<uint8_t> pubKey, std::shared_ptr<CPoW> pow, BigInt consumedCoins, std::vector<uint8_t> IOTAddress, std::vector<uint8_t> TXID, std::vector<uint8_t> additionalData,std::string friendlyID);
	//Common Part
	std::vector<uint8_t> getPackedData(bool ommitSig=false);
	void setVersion(size_t version);
	size_t getVersion();

	void setPubKey(std::vector<uint8_t> pubKey);
	std::vector<uint8_t> getPubKey();

	std::string getFriendlyID();
	void setFriendlyID(std::string id);

	void setSignature(std::vector<uint8_t> sig);
	std::vector<uint8_t> getSignature();

	//PoW
	std::shared_ptr<CPoW> getPow();
	void setPow(std::shared_ptr<CPoW> pow);

	//Stake
	void setConsumedCoins(BigInt consumedCoins);
	BigInt getConsumedCoins();

	void setTXReceiptID(std::vector<uint8_t> id);
	std::vector<uint8_t> getTXReceiptID();


	//Iot
	std::vector<uint8_t> getSenderAddress();
	void setSenderAddress(std::vector<uint8_t> addr);

	//CPS
	uint64_t getRank();


};

#endif
