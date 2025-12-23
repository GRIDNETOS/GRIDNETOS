#pragma once
#include "stdafx.h"
class CCryptoFactory;

struct kcFlags
{
	bool flat : 1;//either AEAD OR pure ChaCha20 stream ciphertext
	bool reserved1 : 1;
	bool reserved2 : 1;
	bool reserved3 : 1;
	bool reserved4 : 1;
	bool reserved5 : 1;
	bool reserved6 : 1;
	bool reserved7 : 1;

	kcFlags(const kcFlags& sibling) {
		std::memcpy(this, &sibling, sizeof(kcFlags));
	}

	kcFlags()
	{
		std::memset(this, 0, sizeof(kcFlags));
	}

	bool operator==(const kcFlags& rhs) const
	{
		return !std::memcmp(this, &rhs, sizeof(kcFlags));
	}
	
};

class CKeyChain
{
private:
	Botan::SecureVector<uint8_t> mMainPrivKey;
	uint32_t mCurrentIndex;
	uint32_t mFurthestIndex;
	std::string mName;
	std::shared_ptr<CCryptoFactory> mCf;
	std::mutex mGuardian;
	kcFlags mFlags;
public:

	kcFlags getFlags();
	static std::shared_ptr<CKeyChain> instantiate(std::vector<uint8_t> bytes);
	
	std::string getDescription(bool includePrivateKey = false, std::string newLine="", eColor::eColor headerColor = eColor::none, bool exportName=false);
	CKeyChain(bool genKeys=true, bool isFlat = false);
	std::string getID();
	void setID(std::string ID);
	CKeyChain(std::shared_ptr<CCryptoFactory> cf, Botan::SecureVector<uint8_t> privateKey, uint32_t index=0,std::string name ="", bool isFlat=false);
	CKeyChain(std::shared_ptr<CCryptoFactory> cf, bool isFlat = false);
	Botan::secure_vector<uint8_t> getMainPrivKey();
	bool unpack(Botan::secure_vector<uint8_t> unpack);
	void setIndex(uint32_t index);
	CKeyChain(const CKeyChain& sibling);
	bool operator==(CKeyChain &rhs);
	bool operator!=(CKeyChain& rhs);
	CKeyChain& operator = (const CKeyChain &t);
	uint32_t getCurrentIndex();
	void setUsedUpTillIndex(uint32_t depth);
	void incIndex();
	std::vector<uint8_t> getPackedData(bool exportName = true);
	uint32_t getUsedUptillIndex();
	bool setMainPrivKey(Botan::SecureVector<uint8_t> mainPriv);
	Botan::SecureVector<uint8_t> getPrivKey();
	std::vector<uint8_t> getPubKey();
	bool searchForPubKey(std::vector<uint8_t> pubKey, uint64_t &slot);
	bool searchForAddress(std::vector<uint8_t> address, uint64_t & slot);
};
