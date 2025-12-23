#ifndef CRYPTO_FACTORY_H
#define CRYPTO_FACTORY_H

#include "stdafx.h"

#include "identitytoken.h"
#include "chainparams.h"
#include "identitytoken.h"
#include <string>
#include <iostream>

#include "Ultimium.h"
#include <mutex>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include "./CL/cl.h"
#endif
#include "Settings.h"
class CBlockHeader;
class CKeyChain;
class CBlock;

class CCryptoFactory : public std::enable_shared_from_this<CCryptoFactory>
{
private:
	std::recursive_mutex mUltimiumWarden;
	static std::recursive_mutex sStaticInstanceGuardian;
    std::shared_ptr<CTools> mTools;
    CUltimium *mUltimium;
	std::mutex mUltimiumGuardian;
    Botan::AutoSeeded_RNG mRng;
	std::recursive_mutex mWarden;

	std::mutex mRngLock;
	static std::shared_ptr<CCryptoFactory>  sInstance;
	static  std::vector<uint8_t> sEmptyNodeHash;

public:
	static std::vector<uint8_t> getEmptyNodeHash();
	static std::shared_ptr<CCryptoFactory>  getInstance();
	std::string generateSelfSignedCert(std::string &privKeyPEM, std::string commonName = "", std::string ip = "", std::string contactMail = "info@gridnet.org");
	CCryptoFactory();
	~CCryptoFactory();
	CCryptoFactory(const CCryptoFactory& sibling) =delete;

	std::vector<uint8_t> genHMAC_sha256(const std::vector<uint8_t> &key, const std::vector<uint8_t> &data);
	bool verifyHMAC_sha256(const std::vector<uint8_t> &key, const  std::vector<uint8_t> &data, const std::vector<uint8_t> &providedHMAC);
	bool genKeyPair(Botan::secure_vector<uint8_t> &  priv, std::vector<uint8_t> & pub);
	std::vector<uint8_t> getPubFromPriv(Botan::secure_vector<uint8_t> privKeyRAW);
	bool getChildKeyPair(const Botan::secure_vector<uint8_t> masterPriv, uint32_t index,
		Botan::secure_vector<uint8_t> & childPriv, std::vector<uint8_t> &  childPub, bool verifyKeyPair = false);
	
	bool genAddress(std::vector<uint8_t> pubKeyRAW, std::vector<uint8_t>& address, uint32_t type=0);

	bool checkAddr(std::vector<uint8_t>  address);




	Botan::secure_vector<uint8_t> ECDH(std::vector<uint8_t> pubKey, Botan::secure_vector<uint8_t> secret);

	std::vector<uint8_t> encChaCha20(Botan::secure_vector<uint8_t> key, std::vector<uint8_t> data, bool authenticate =true, std::vector<uint8_t> externalDatahHash = std::vector<uint8_t>());
	std::vector<uint8_t> decChaCha20(std::vector<uint8_t> key, std::vector<uint8_t> data, std::vector<uint8_t> externalDatahHash = std::vector<uint8_t>());

	/// <summary>
/// Returns BER-encoded encrypted data. Provided authentication if desired.
/// The optionally returned sessionKey might be used for encryption of external data.
/// </summary>
/// <param name="rng"></param>
/// <param name="pubKeyBER">BER-encoded public key.</param>
/// <param name="data">BER-encoded data.</param>
/// <param name="autheniticate">Whether to authenticate the data or not.</param>
/// <returns></returns>
	std::vector<uint8_t>  encChaCha20c25519(Botan::AutoSeeded_RNG& rng, std::vector<uint8_t>  pubKeyBER,
		std::vector<uint8_t> data, bool autheniticate, bool directSHA3 = true,
		const Botan::secure_vector<uint8_t>& sessionKey = Botan::secure_vector<uint8_t>(), std::vector<uint8_t> externalDataHash = std::vector<uint8_t>(),
		const Botan::secure_vector<uint8_t> privateKey = Botan::secure_vector<uint8_t>(), const Botan::secure_vector<uint8_t>& nonce = Botan::secure_vector<uint8_t>());
	/// <summary>
	/// Returns BER-encoded encrypted data. Provided authentication if desired.
	/// </summary>
	/// <param name="rng"></param>
	/// <param name="pubKeyBER">BER-encoded public key.</param>
	/// <param name="data">'RAW' data.</param>
	/// <param name="autheniticate">Whether to authenticate the data or not.</param>
	/// <returns></returns>
	std::vector<uint8_t>  encChaCha20c25519(std::vector<uint8_t>  pubKeyBER,
	std::vector<uint8_t> data, bool autheniticate, const Botan::secure_vector<uint8_t> &sessionKey = Botan::secure_vector<uint8_t>(), std::vector<uint8_t> externalDataHash = std::vector<uint8_t>()
	, const Botan::secure_vector<uint8_t> privateKey = Botan::secure_vector<uint8_t>(), const Botan::secure_vector<uint8_t>& nonce = Botan::secure_vector<uint8_t>());

	std::vector<uint8_t>  decChaCha20c25519(Botan::AutoSeeded_RNG& rng, Botan::secure_vector<uint8_t>  privKeyBER,
		std::vector<uint8_t> data, bool directSHA3 = true, const Botan::secure_vector<uint8_t> &sessionKey = Botan::secure_vector<uint8_t>(), std::vector<uint8_t> externalDataHash = std::vector<uint8_t>(),
		const std::vector<uint8_t> &sendersPubKey= std::vector<uint8_t>(), const Botan::secure_vector<uint8_t>& nonce = Botan::secure_vector<uint8_t>(), const bool &isAuthenticated=false);


	std::vector<uint8_t>  decChaCha20c25519( Botan::secure_vector<uint8_t>  privKeyBER,
		std::vector<uint8_t> data, const Botan::secure_vector<uint8_t> &sessionKey = Botan::secure_vector<uint8_t>(), std::vector<uint8_t> externalDataHash = std::vector<uint8_t>(),
		const std::vector<uint8_t>& sendersPubKey = std::vector<uint8_t>(), const Botan::secure_vector<uint8_t>& nonce = Botan::secure_vector<uint8_t>(), const bool& isAuthenticated = false);

	bool refreshBlockHeader(std::shared_ptr<CBlockHeader>header, Botan::secure_vector<uint8_t> privKey);
	bool signBlock(std::shared_ptr<CBlock> block, Botan::secure_vector<uint8_t> privKey);

	bool verifyBlockSignature(std::shared_ptr<CBlock> block,std::vector<uint8_t> pubKey= std::vector<uint8_t>());
	bool verifyBlockSignature(std::shared_ptr<CBlockHeader> header, std::vector<uint8_t> pubKey = std::vector<uint8_t>());
	std::vector<uint8_t> encChaCha20c128(std::vector<uint8_t> pubKeyBER, std::vector<uint8_t> data, bool autheniticate);
	std::vector<uint8_t>  encChaCha20c128(Botan::AutoSeeded_RNG &rng, std::vector<uint8_t>  pubKeyBER,
		 std::vector<uint8_t> data,bool autheniticate);

	std::vector<uint8_t>  decChaCha20c128(Botan::AutoSeeded_RNG &rng, Botan::secure_vector<uint8_t>  privKeyBER,
		std::vector<uint8_t> data);
	std::vector<uint8_t>  decChaCha20c128( Botan::secure_vector<uint8_t>  privKeyBER,
		std::vector<uint8_t> data);


	bool verifyKeyPair(std::vector<uint8_t> pubKey, Botan::secure_vector<uint8_t> priKey,bool extendedKeys=false);
	

	bool verifyNonce(CBlockHeader keyBlockHeader, arith_uint256 target=0);
	bool verifyNonce(uint32_t nonce, std::vector<uint8_t> data, arith_uint256 target, bool isHeader=false, const std::vector<uint8_t> &resultinghash= std::vector<uint8_t>());
	double getPoWDiff(std::vector<uint8_t> data, uint32_t nonce);
	bool verifyHeaderNonce(uint32_t nonce, std::vector<uint8_t> * data, cl_ulong localTarget,unsigned int targetWindow,std::vector<uint8_t> target,cl_ulong testValueFromKernel=0);

	bool checkIfPoWMeetsTarget(std::array<unsigned char, 32> pow, int difficulty);

	std::vector<uint8_t> signData(Botan::AutoSeeded_RNG &rng, std::vector<uint8_t> inputData, Botan::secure_vector<uint8_t> privateKey);
	std::vector<uint8_t> signData(std::vector<uint8_t> inputData, Botan::secure_vector<uint8_t> privateKey);
	
	bool verifySignature(Botan::AutoSeeded_RNG &rng, std::vector<uint8_t> signature,std::vector<uint8_t> inputData, std::vector <uint8_t> public_key);
	bool verifySignature( std::vector<uint8_t> signature, std::vector<uint8_t> inputData, std::vector <uint8_t> public_key);
	std::vector<uint8_t> getSHA3_512Vec(std::vector<uint8_t> inputData);
	std::vector<uint8_t> getSHA2_512Vec(std::vector<uint8_t> inputData);
	std::vector<uint8_t> getSHA3_256Vec(std::vector<uint8_t> inputData);
	std::vector<uint8_t> getSHA2_256Vec(std::vector<uint8_t> inputData);
};

#endif
