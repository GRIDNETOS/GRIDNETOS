#ifndef HD_SEED_H
#define HD_SEED_H
////////////////////////////////////////////////////////////////////////////////
//
// hdkeys.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//Copyright (c) 2017-2018 Rafal Skowronski (GRIDNET Technologies LLC)
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//
#include "CryptoFactory.h"
#include "stdafx.h"
#include <string>

     //std::vector<uint8_t> GRIDNET_SEED = // key = "GRIDNET seed"
    extern std::vector<uint8_t> GRIDNET_SEED;
	class HDSeed
	{
	public:
		HDSeed(const  std::vector<uint8_t> & seed, const  std::vector<uint8_t> & coin_seed = Botan::hex_decode("260387636f696e2073656564"))
		{
			std::vector<uint8_t>  hmac = CCryptoFactory::getInstance()->genHMAC_sha256(coin_seed, seed);
            mMasterKey.assign(hmac.begin(), hmac.begin() + 32);
            mMasterChainCode.assign(hmac.begin() + 32, hmac.end());
		}

        const  std::vector<uint8_t> & getSeed() const { return mSeed; }
        const  std::vector<uint8_t> & getMasterKey() const { return mMasterKey; }
        const  std::vector<uint8_t> & getMasterChainCode() const { return mMasterChainCode; }
		 std::vector<uint8_t>  getExtendedKey(bool bPrivate = false) const;

	private:
         std::vector<uint8_t>  mSeed;
         std::vector<uint8_t>  mMasterKey;
         std::vector<uint8_t>  mMasterChainCode;
	
	};

	class InvalidHDKeychainException : public std::runtime_error
	{
	public:
		InvalidHDKeychainException()
			: std::runtime_error("Keychain is invalid.") { }
	};

	class InvalidHDKeychainPathException : public std::runtime_error
	{
	public:
		InvalidHDKeychainPathException()
			: std::runtime_error("Keychain path is invalid.") { }
	};

	class HDKeychain
	{
	public:
		HDKeychain() {  }
		HDKeychain(const  std::vector<uint8_t> & key, const  std::vector<uint8_t> & chain_code, uint32_t child_num = 0, uint32_t parent_fp = 0, uint32_t depth = 0,  bool main_net_ = true);
		HDKeychain(const  std::vector<uint8_t> & extkey);
		HDKeychain(const HDKeychain& source);

		HDKeychain& operator=(const HDKeychain& rhs);

        explicit operator bool() const { return mValid; }


		bool operator==(const HDKeychain& rhs) const;
		bool operator!=(const HDKeychain& rhs) const;

		// Serialization
		 std::vector<uint8_t>  extkey() const;
		 //warning this serialziation includes a private key!
		 std::vector<uint8_t>  getPackedData() const;
		// Accessor Methods
        uint32_t version() const { return mVersion; }
        int depth() const { return mDepth; }
        uint32_t parent_fp() const { return mParentFp; }
        uint32_t child_num() const { return mChildNum; }
        const  std::vector<uint8_t> & chain_code() const { return mChainCode; }
        const  std::vector<uint8_t> & key() const { return mKey; }

		Botan::secure_vector<uint8_t>  privkey() const;
        const  std::vector<uint8_t> & pubkey() const { return mPubKey; }
		 std::vector<uint8_t>  uncompressed_pubkey() const;

        bool isPrivate() const { return mIsPrivate; }
		 std::vector<uint8_t>  hash() const; // hash is ripemd160(sha256(pubkey))
		uint32_t fp() const; // fingerprint is first 32 bits of hash
		 std::vector<uint8_t>  full_hash() const; // full_hash is ripemd160(sha256(pubkey + chain_code))

		HDKeychain getPublic() const;
		HDKeychain getChild(uint32_t i) const;
		HDKeychain getChild(const std::string& path) const;
		HDKeychain getChildNode(uint32_t i, bool private_derivation = false) const
		{
			uint32_t mask = private_derivation ? 0x80000000ull : 0x00000000ull;
			return getChild(mask).getChild(i);
		}

	
		 Botan::secure_vector<uint8_t>  getPrivateSigningKey(uint32_t i) const
		{
			
			return getChild(i).privkey();
		}

	
		 std::vector<uint8_t>  getPublicKeyBytes(uint32_t i) const
		{
			return getChild(i).pubkey() ;
		}

        static void setVersions(uint32_t priv_version, uint32_t pub_version) { mPrivVersion = priv_version; mPubVersion = pub_version; }

		std::string toString() const;

	private:
        bool mIsPrivate;
        static uint32_t mPrivVersion;
        static uint32_t mPubVersion;

        static uint32_t mPrivVersionTest;
        static uint32_t mPubVersionTest;
        uint32_t mVersion;
        uint8_t mDepth;
        bool mMainNet;
        bool mPubSign;
        uint32_t mParentFp;
        uint32_t mChildNum;
         std::vector<uint8_t>  mChainCode; // 32 bytes
         std::vector<uint8_t>  mKey;        // 33 bytes, first byte is 0x00 for private key

         std::vector<uint8_t>  mPubKey;

        bool mValid;

		void updatePubkey();
	};

#endif

