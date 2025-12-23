#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include "enums.h"
#include <stdafx.h>

class CTransmissionToken;
struct nmFlags
{
	bool encrypted : 1;//either AEAD OR pure ChaCha20 stream ciphertext
	bool authenticated : 1;//mSig !=null
	bool boxedEncryption : 1;//true - AEAD container present in mData, false - encrypted with previously negotiated symetric key
	//AND nonce is 12 bytes of sha256(mSourceSeq)
	bool session : 1;//whether session is to be established and consecutive msgs encrypted with plain Chacha20 secret /nonce (based on sourceSeq)
	//and no AEAD, use mExtraData to provide source public key
	bool useAEADpubKeyForSession : 1;//whether to use secret within AEAD-box for session's secret (for this, the msg needs to be encrypted
	//in the firsy place and boxedEncryption set). If not set, DH is performed with pubkey in mExtraData
	bool sessionDescription : 1;//sessionDescription within data-field
	bool useAEADForAuth : 1;
	bool fromMobile : 1;
	nmFlags(const nmFlags& sibling) {
		std::memcpy(this, &sibling, sizeof(nmFlags));
	}

	nmFlags()
	{
		std::memset(this, 0, sizeof(nmFlags));
	}
};
class CNetMsg
{
private:

	static inline constexpr size_t NM_MAX_DESTINATION_SIZE = 50;
	static inline constexpr size_t NM_MAX_SOURCE_SIZE = 50;
	static inline constexpr size_t NM_MAX_NEXTHOP_SIZE = 1024;
	static inline constexpr uint8_t NM_MAX_HOPS = 255;
	static inline constexpr size_t NM_MAX_EXTRADATA_SIZE = 4096;
	static inline constexpr size_t NM_MAX_DATA_SIZE = 100'000'000;
	static inline constexpr size_t NM_MAX_SIG_SIZE = 120;

	std::mutex mGuardian;
	static std::mutex sRecentSourceSeqGuardian;
	static uint64_t sRecentSourceSeq;
	uint64_t getNewSourceSeq();
	void initFields();
	uint8_t mVersion;
	eNetEntType::eNetEntType mEntityType;
	eNetReqType::eNetReqType mRequestType;

	/*
	Hello msg request:
	mExtraData - challange to be signed
	mData - CSessionDescription with ConversationID on local node and pubKey to be used for encryption
	*/

	std::vector<uint8_t> mData;//contains (possibly) encrypted data. Might contain another encapsulated NetMsg
		//when in used in tunneling mode.
	//Sec-data Begin
	std::vector<uint8_t> mSig;//authenticates the below
	//Sec-data End

	// Routing Additions - Begin
	std::shared_ptr<CTransmissionToken> mTT;
	std::vector<uint8_t> mDestination;
	std::vector<uint8_t> mSource;
	std::vector<uint8_t> mNextHop;
	uint64_t mHops;
	uint64_t mSourceSeq;
	nmFlags mFlags;//encrypted etc.
	uint64_t mDestSeq;
	eEndpointType::eEndpointType mDestinationType;
	eEndpointType::eEndpointType mSourceType;
	std::vector<uint8_t> mExtraData;
	// Routing Additions - End

public:

	uint64_t getDataSize();
	nmFlags getFlags();
	void setFlags(nmFlags flags);
	CNetMsg(const CNetMsg& sibling);
	CNetMsg& operator=(const CNetMsg& sibling);
	eEndpointType::eEndpointType  getDestinationType();
	//Security - Begin
	std::vector<uint8_t> getSig();
	void setSig(std::vector<uint8_t> sig);

	bool encrypt(std::vector<uint8_t> key, bool doECIES=true, const Botan::secure_vector<uint8_t>& sessionKey = Botan::secure_vector<uint8_t>());

	bool decrypt(std::vector<uint8_t> decryptionKey, std::vector<uint8_t> pubKey = std::vector<uint8_t>(),const Botan::secure_vector<uint8_t> &sessionKey= Botan::secure_vector<uint8_t>(),
		const std::vector<uint8_t> boxedPubKey =  std::vector<uint8_t>(), const bool& isAuthenticated = false);
	bool signCrypt(Botan::secure_vector<uint8_t> privKey, std::vector<uint8_t> pubKey);

	std::vector<uint8_t> getImage(bool includeDataField = true);

	bool sign(Botan::secure_vector<uint8_t> privKey);
	bool verifySig(std::vector<uint8_t> pubKey);
	//Security - End

	// Routing Additions - Begin
	std::vector<uint8_t> getDestination();
	std::string getFormatedDestination();
	std::string getFormatedSource();
	eEndpointType::eEndpointType getSourceType();
	void setDestinationType(eEndpointType::eEndpointType eType);
	std::string getDescription(bool includeAddresses=true, bool includeEndpointTypes=true, bool includeContainerType=true, bool describeFlags = true);
	void setDestination(std::vector<uint8_t> id);

	std::vector<uint8_t> getSource();
	void setSourceType(eEndpointType::eEndpointType eType);
	void setSource(std::vector<uint8_t> id);

	uint64_t getHops();
	void incHops();
	void setHopCount(uint64_t val);
	void setSourceSeq(uint64_t seq);
	void setDestSeq(uint64_t seq);
	bool hasData();
	uint64_t getSourceSeq();
	uint64_t getDestSeq();

	std::shared_ptr<CTransmissionToken> getTT();
	void setTT(std::shared_ptr<CTransmissionToken> tt);
	
	// Routing Additions - End

	void  setExtraBytes( std::vector<uint8_t> bytes);
	std::vector<uint8_t> getExtraBytes();
	void setIntegerAsRawDataBytes(uint64_t value);
	void setRequestType(eNetReqType::eNetReqType reqType);
	void setEntityType(eNetEntType::eNetEntType entType);
	void setVersion(uint8_t version);
	uint8_t getVersion();
	CNetMsg(eNetEntType::eNetEntType entityType, eNetReqType::eNetReqType reqType, std::vector<uint8_t> data = std::vector<uint8_t>());
	CNetMsg();
	eNetEntType::eNetEntType getEntityType() ;
	eNetReqType::eNetReqType  getRequestType();
	std::vector<uint8_t> getData() ;
	bool setData(const std::vector<uint8_t>& data);

	std::vector<uint8_t> getPackedData();
	static std::shared_ptr<CNetMsg> instantiate(const std::vector<uint8_t>& packedData);
	inline friend  bool operator==(const CNetMsg& lhs, const CNetMsg& rhs) {
		std::shared_ptr<CTools> tools = CTools::getInstance();
		return (lhs.mVersion == rhs.mVersion &&
			lhs.mEntityType == rhs.mEntityType &&
			lhs.mSourceSeq == rhs.mSourceSeq
			&& lhs.mDestSeq == rhs.mDestSeq
			&& lhs.mDestinationType == rhs.mDestinationType
			&& lhs.mHops == rhs.mHops
			&& lhs.mRequestType == rhs.mRequestType
			&& tools->compareByteVectors(lhs.mData,rhs.mData)
			&& tools->compareByteVectors(lhs.mSig, rhs.mSig)
			&& tools->compareByteVectors(lhs.mExtraData, rhs.mExtraData)
			&& tools->compareByteVectors(lhs.mSource, rhs.mSource)
			&& tools->compareByteVectors(lhs.mDestination, rhs.mDestination)
			//&& (*lhs.mTT) == (*rhs.mTT)
			);

	}

};
