#include "NetMsg.h"


#include "TransmissionToken.h"
#include "DataConcatenator.h"
#include "CryptoFactory.h"

std::mutex CNetMsg::sRecentSourceSeqGuardian;
uint64_t CNetMsg::sRecentSourceSeq = 0;

void CNetMsg::setExtraBytes(std::vector<uint8_t> bytes)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mExtraData = bytes;
}

std::vector<uint8_t> CNetMsg::getExtraBytes()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mExtraData;
}

void CNetMsg::setIntegerAsRawDataBytes(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	uint8_t sigB = CTools::getInstance()->getSignificantBytes(value);
	mData.resize(sigB);
	std::memcpy(mData.data(), &value, sigB);
}


void CNetMsg::setRequestType(eNetReqType::eNetReqType reqType)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mRequestType = reqType;
}
void CNetMsg::setEntityType(eNetEntType::eNetEntType entType)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mEntityType = entType;
}
void  CNetMsg::setVersion(uint8_t  version)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mVersion - version;
}
uint8_t  CNetMsg::getVersion()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVersion ;
}

CNetMsg::CNetMsg(eNetEntType::eNetEntType entityType, eNetReqType::eNetReqType reqType, std::vector<uint8_t> data)
{
	initFields();
	mEntityType = entityType;
	mRequestType = reqType;
	mData = data;
}

CNetMsg::CNetMsg()
{
	initFields();
}
eNetEntType::eNetEntType CNetMsg::getEntityType() 
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mEntityType;
}

eNetReqType::eNetReqType CNetMsg::getRequestType() 
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mRequestType;
}

std::vector<uint8_t> CNetMsg::getData() 
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mData;
}

bool CNetMsg::setData(const std::vector<uint8_t> & data)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mData = data;
	return false;
}

std::vector<uint8_t> CNetMsg::getPackedData()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	Botan::DER_Encoder enc;
	std::vector<uint8_t> ttBytes;
	std::vector<uint8_t> flagsBytes(1);
	flagsBytes[0] = reinterpret_cast<uint8_t&>(mFlags);

	if (mTT != nullptr)
		ttBytes = mTT->getPackedData();

	enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
		.encode(static_cast<size_t>(mVersion))
		.encode(flagsBytes, Botan::ASN1_Tag::OCTET_STRING)
		//routing additions - BEGIN
		.encode(static_cast<size_t>(mDestinationType))
		.encode(mDestination, Botan::ASN1_Tag::OCTET_STRING)
		.encode(static_cast<size_t>(mSourceType))
		.encode(mSource, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mNextHop, Botan::ASN1_Tag::OCTET_STRING)
		.encode(static_cast<size_t>(mHops))
		.encode(static_cast<size_t>(mSourceSeq))
		.encode(static_cast<size_t>(mDestSeq))
		.encode(ttBytes, Botan::ASN1_Tag::OCTET_STRING)//incentivization
		.encode(mExtraData, Botan::ASN1_Tag::OCTET_STRING)
		//routing additions - END
		.encode(static_cast<size_t>(mEntityType))//subtype
		.encode(static_cast<size_t>(mRequestType))
		.encode(mData, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mSig, Botan::ASN1_Tag::OCTET_STRING)
		.end_cons();
	return enc.get_contents_unlocked();

}

std::shared_ptr<CNetMsg> CNetMsg::instantiate(const std::vector<uint8_t>& packedData)
{
	try {
		// Original check - maintain for compatibility
		if (packedData.size() == 0)
			return nullptr;

		// Pre-validation: Check minimum size for valid BER SEQUENCE
		// Tag(1) + Length(1) + minimal content
		if (packedData.size() < 5)
			return nullptr;

		// Pre-validation: Verify BER SEQUENCE tag (0x30)
		if (packedData[0] != 0x30)
			return nullptr;

		// Pre-validation: Verify length encoding sanity
		size_t lengthOctets = 1;
		size_t contentLength = 0;

		if (packedData[1] & 0x80) {
			// Long form length
			lengthOctets = (packedData[1] & 0x7F) + 1;

			// Prevent excessive length octets
			if (lengthOctets > 5 || (1 + lengthOctets) > packedData.size())
				return nullptr;

			// Calculate content length with overflow protection
			for (size_t i = 2; i < 1 + lengthOctets; ++i) {
				// Check for potential overflow before shifting
				if (contentLength > (SIZE_MAX >> 8))
					return nullptr;
				contentLength = (contentLength << 8) | packedData[i];
			}
		}
		else {
			// Short form length
			contentLength = packedData[1];
		}


		// Pre-check against maximum reasonable size
		constexpr size_t MAX_DATAGRAM_SIZE = 1ULL * 1024 * 1024 * 1024; // 1GB
		if (contentLength > MAX_DATAGRAM_SIZE)
			return nullptr;

		// Verify total size matches
		if ((1 + lengthOctets + contentLength) != packedData.size())
			return nullptr;

		// Pre-check against maximum reasonable size
		if (packedData.size() > (NM_MAX_DATA_SIZE + 10000)) // Data + overhead
			return nullptr;

		// Now proceed with original logic
		std::shared_ptr<CNetMsg> msg = std::make_shared<CNetMsg>();
		size_t temp = 0;
		std::vector<uint8_t> ttBytes, flagBytes;

		// CRITICAL: Match the original implementation's decoder chain
		// start_cons returns a NEW decoder for the sequence contents
		size_t version = 0;
		Botan::BER_Decoder dec = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE).
			decode(version);

		// Validate version before assignment
		if (version > std::numeric_limits<decltype(msg->mVersion)>::max())
			return nullptr;
		msg->mVersion = static_cast<decltype(msg->mVersion)>(version);

		Botan::BER_Object obj; // Keep for compatibility even if unused

		if (msg->getVersion() == 2)
		{
			// Now 'dec' is the decoder for the SEQUENCE contents, use it for all subsequent decodes
			dec.decode(flagBytes, Botan::ASN1_Tag::OCTET_STRING);
			if (flagBytes.size() != 1)
				return nullptr;
			// Safe assignment instead of reinterpret_cast
			uint8_t flagValue = flagBytes[0];
			std::memcpy(&msg->mFlags, &flagValue, sizeof(uint8_t));

			dec.decode(temp);
			if (temp > static_cast<size_t>(eEndpointType::MaxValue))
				return nullptr;
			msg->setDestinationType(static_cast<eEndpointType::eEndpointType>(temp));

			dec.decode(msg->mDestination, Botan::ASN1_Tag::OCTET_STRING);
			if (msg->mDestination.size() > NM_MAX_DESTINATION_SIZE)
				return nullptr;

			dec.decode(temp);
			if (temp > static_cast<size_t>(eEndpointType::MaxValue))
				return nullptr;
			msg->setSourceType(static_cast<eEndpointType::eEndpointType>(temp));

			dec.decode(msg->mSource, Botan::ASN1_Tag::OCTET_STRING);
			if (msg->mSource.size() > NM_MAX_SOURCE_SIZE)
				return nullptr;

			dec.decode(msg->mNextHop, Botan::ASN1_Tag::OCTET_STRING);
			if (msg->mNextHop.size() > NM_MAX_NEXTHOP_SIZE)
				return nullptr;

			// For numeric fields, decode to temp first to avoid overflow
			dec.decode(temp);
			if (temp > NM_MAX_HOPS || temp > std::numeric_limits<decltype(msg->mHops)>::max())
				return nullptr;
			msg->mHops = static_cast<decltype(msg->mHops)>(temp);

			dec.decode(temp);
			if (temp > std::numeric_limits<decltype(msg->mSourceSeq)>::max())
				return nullptr;
			msg->mSourceSeq = static_cast<decltype(msg->mSourceSeq)>(temp);

			dec.decode(temp);
			if (temp > std::numeric_limits<decltype(msg->mDestSeq)>::max())
				return nullptr;
			msg->mDestSeq = static_cast<decltype(msg->mDestSeq)>(temp);

			dec.decode(ttBytes, Botan::ASN1_Tag::OCTET_STRING);
			if (ttBytes.size() != 0)
			{
				// Add size sanity check for TT
				if (ttBytes.size() > 10000) // Reasonable maximum for TT
					return nullptr;

				// Wrap TT instantiation in try-catch
				try {
					msg->mTT = CTransmissionToken::instantiate(ttBytes);
					if (msg->mTT == nullptr)
						return nullptr;
				}
				catch (...) {
					return nullptr;
				}
			}

			dec.decode(msg->mExtraData, Botan::ASN1_Tag::OCTET_STRING);
			if (msg->mExtraData.size() > NM_MAX_EXTRADATA_SIZE)
				return nullptr;

			dec.decode(temp);
			if (temp > static_cast<size_t>(eNetEntType::MaxValue))
				return nullptr;
			msg->setEntityType(static_cast<eNetEntType::eNetEntType>(temp));

			dec.decode(temp);
			if (temp > static_cast<size_t>(eNetReqType::MaxValue))
				return nullptr;
			msg->setRequestType(static_cast<eNetReqType::eNetReqType>(temp));

			dec.decode(msg->mData, Botan::ASN1_Tag::OCTET_STRING);
			if (msg->mData.size() > NM_MAX_DATA_SIZE)
				return nullptr;

			dec.decode(msg->mSig, Botan::ASN1_Tag::OCTET_STRING);
			if (msg->mSig.size() > NM_MAX_SIG_SIZE)
				return nullptr;

			dec.verify_end();
		}
		else
		{
			return nullptr; // Maintain original behavior for non-v2
		}

		return msg;
	}
	catch (const Botan::BER_Decoding_Error&) {
		// Specific BER decoding errors (malformed structure)
		return nullptr;
	}
	catch (const Botan::Decoding_Error&) {
		// General decoding errors
		return nullptr;
	}
	catch (const Botan::Invalid_Argument&) {
		// Invalid arguments to Botan functions
		return nullptr;
	}
	catch (const Botan::Encoding_Error&) {
		// Encoding related errors
		return nullptr;
	}
	catch (const Botan::Stream_IO_Error&) {
		// I/O errors during parsing
		return nullptr;
	}
	catch (const Botan::Exception&) {
		// Any other Botan exception
		return nullptr;
	}
	catch (const std::bad_alloc&) {
		// Memory allocation failure
		return nullptr;
	}
	catch (const std::length_error&) {
		// Container size errors
		return nullptr;
	}
	catch (const std::out_of_range&) {
		// Range check failures
		return nullptr;
	}
	catch (const std::overflow_error&) {
		// Arithmetic overflow
		return nullptr;
	}
	catch (const std::runtime_error&) {
		// Other runtime errors
		return nullptr;
	}
	catch (const std::exception&) {
		// Any other standard exception
		return nullptr;
	}
	catch (...) {
		// Original catch-all maintained for compatibility
		return nullptr;
	}
}
void CNetMsg::initFields()
{
	reinterpret_cast<uint8_t&>(mFlags) = 0;
	mVersion = 2;
	mRequestType = eNetReqType::notify;
	mEntityType = eNetEntType::msg;
	mHops = 0;
	mDestinationType = eEndpointType::IPv4;
	mSourceSeq = getNewSourceSeq();// the value would be overriden if data-structure is deserialized anyway
	mDestSeq = 0;
	mSourceType = eEndpointType::IPv4;
	mTT = nullptr;
}

uint64_t CNetMsg::getDataSize()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mData.size();
}

nmFlags CNetMsg::getFlags()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mFlags;
}

void CNetMsg::setFlags(nmFlags flags)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mFlags = flags;
}

CNetMsg::CNetMsg(const CNetMsg& sibling)
{
	mFlags = sibling.mFlags;
	mVersion = sibling.mVersion;
	mEntityType = sibling.mEntityType;
	mRequestType = sibling.mRequestType;
	mData = sibling.mData;
	mSig = sibling.mSig;

	if (sibling.mTT)
	{
		(*mTT) = *(sibling.mTT);
	}
	else
	{
		mTT = nullptr;
	}

	mDestination = sibling.mDestination;
	mNextHop = sibling.mNextHop;
	mHops = sibling.mHops;
	mSourceSeq = sibling.mSourceSeq;
	mDestSeq = sibling.mDestSeq;
	mDestinationType = sibling.mDestinationType;
	mSourceType = sibling.mSourceType;
	mExtraData = sibling.mExtraData;
}

CNetMsg& CNetMsg::operator=(const CNetMsg& sibling)
{
	mFlags = sibling.mFlags;
	mVersion = sibling.mVersion;
	mEntityType = sibling.mEntityType;
	mRequestType = sibling.mRequestType;
	mData = sibling.mData;
	mSig = sibling.mSig;

	if (sibling.mTT)
	{
		(*mTT) = *(sibling.mTT);
	}
	else
	{
		mTT = nullptr;
	}

	mDestination = sibling.mDestination;
	mNextHop = sibling.mNextHop;
	mHops = sibling.mHops;
	mSourceSeq = sibling.mSourceSeq;
	mDestSeq = sibling.mDestSeq;
	mDestinationType = sibling.mDestinationType;
	mSourceType = sibling.mSourceType;
	mExtraData = sibling.mExtraData;
	return *this;
}

eEndpointType::eEndpointType CNetMsg::getDestinationType()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDestinationType;
}

std::vector<uint8_t> CNetMsg::getSig()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mSig;
}

void CNetMsg::setSig(std::vector<uint8_t> sig)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mSig = sig;
}


/// <summary>
/// Imposes either ECIES or stream cipher onto the body of the message.
/// Key is either symmetric or public key based on doECIES parameter.
/// The function provided hash on netmsg's data to encryption data for authentication.
/// Thus, the resulting Box authenticates both the cipher-text AND entire content of NetMsg.
/// </summary>
/// <param name="key"></param>
/// <param name="doECIES"></param>
/// <returns></returns>
bool CNetMsg::encrypt(std::vector<uint8_t> key,  bool doECIES, const Botan::secure_vector<uint8_t>& sessionKey)
{
	std::vector<uint8_t> hash;//needs to be computed AFTER flags are updated
	//ANY flags need to be set/updated RIGHT NOW

	try {
		if (doECIES)
		{
			if (key.size() != 32)
				return false;
			mFlags.encrypted = true;
			mFlags.boxedEncryption = true;
			mFlags.authenticated = true;//authenticated through Poly1305
			hash = getImage(false);//needed for Auth to succeed
			//Here we DO want to employ poly1305 for securing integrity since the signature is not to be provided.
			mData = CCryptoFactory::getInstance()->encChaCha20c25519(key, mData, true, sessionKey, hash);
		}
		else
		{
			mFlags.encrypted = true;
			mFlags.boxedEncryption = false;
			hash = getImage(false);//needed for Auth to succeed
			mData = CCryptoFactory::getInstance()->encChaCha20(Botan::secure_vector<uint8_t>(key.begin(), key.end()), mData, true, hash);
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
	return false;
}

/// <summary>
/// Decrypts the body of CNetMsg container.
/// decryptionKey might be either a private asymmetric key or ChaCha20 symmetric key.
/// Will be treated based on the Flags.
/// PubKey will be used to verify signature IF present.
/// The function returns false if ANYTHING goes wrong.
/// boxedPubKey is a public key which was actually present during decryption of an AEAD container.
/// </summary>
/// <param name="key"></param>
/// <returns></returns>
bool CNetMsg::decrypt(std::vector<uint8_t> decryptionKey, std::vector<uint8_t> pubKey, const Botan::secure_vector<uint8_t> &sessionKey, const std::vector<uint8_t> boxedPubKey, const bool& isAuthenticated)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	if (mData.size() == 0)
		return true;
	try {
		
		std::shared_ptr<CCryptoFactory> crypto = CCryptoFactory::getInstance();
		Botan::secure_vector<uint8_t> nonce;
		std::vector<uint8_t> hash = getImage(false);//needed for auth to succeed
		//Signature Verification - BEGIN

		if (mFlags.authenticated && pubKey.size() == 32)
		{
			if (!verifySig(pubKey))
				return false;
		}
		//Signature Verification - END


		//Decryption - BEGIN
		if (mFlags.encrypted && mData.size() > 0)
		{
			if (mFlags.boxedEncryption)
			{
				//ECIES/AEAD box inside
				mData = crypto->decChaCha20c25519(Botan::secure_vector<uint8_t>(decryptionKey.begin(), decryptionKey.end()), mData, sessionKey, hash, boxedPubKey,nonce, isAuthenticated);
			}
			else
			{
				//ChaCha20 symmetric encryption in place
				mData = crypto->decChaCha20(decryptionKey, mData, hash);
			}

			if (mData.size() == 0)//there was an encrypted cipher text thus decryption must have failed
			{
				return false;
			}
			else
			{
				//update flags
				mFlags.encrypted = false;//the message apparently is not encrypted anymore. Updating these flags is important as the datagram MAY traverse further
				//down the network stack i.e. when doing a multi-stage local routing/processing.
				mFlags.boxedEncryption = false;
				//Note: after the message has been decrypted/and or authenticated, its CONSECUTIVE authentication attempts MAY fail (depending on the authentication mode used).
			}
		}
		//Decryption - END
		return true;//by default even if not encrypted nor signed
	}
	catch (...)//the decryption function might throw
	{
		return false;
	}
}

bool CNetMsg::signCrypt(Botan::secure_vector<uint8_t> privKey, std::vector<uint8_t> pubKey)
{
	if (privKey.size() != 32 || pubKey.size() != 32)
		return false;
	//*first encrypt then sign*

	//when doing 'sign-cryption' we do not need authentication provided by underlying ECIES's poly1305 thus we simply Cha-Cha over
	//and the upper-layer's signature assures integrity and authentication.
	mData =CCryptoFactory::getInstance()->encChaCha20c25519(pubKey, mData, false);//leave poly1305 alone, not needed over here

	//note: data within mData field contains now an AEAD BER-encoded wrapper (not RAW chacha20-encoded bytes)   

	//sign
	return sign(privKey);
}

/// <summary>
/// Generates an image of a datagram.
/// Malleable fields are omitted.
/// Data field MAY be omitted (option).
/// </summary>
/// <param name="includeDataField"></param>
/// <returns></returns>
std::vector<uint8_t> CNetMsg::getImage(bool includeDataField)
{
	std::shared_ptr<CCryptoFactory> crypto = CCryptoFactory::getInstance();

	std::vector<uint8_t> flagsBytes(1);
	flagsBytes[0] = reinterpret_cast<uint8_t&>(mFlags);

	CDataConcatenator concat;
	concat.add(mVersion);//PUBLIC
	concat.add(flagsBytes);//PUBLIC
	concat.add(mDestinationType);//PUBLIC
	concat.add(mDestination);//PUBLIC
	concat.add(mSourceType);//PUBLIC
	concat.add(mSource);//PUBLIC
	concat.add(mNextHop);//PUBLIC
	//concat.add(mHops);//PUBLIC melleable
	concat.add(mSourceSeq);//PUBLIC
	concat.add(mDestSeq);//PUBLIC
	concat.add(mExtraData);//PUBLIC
	concat.add(mEntityType);//PUBLIC
	concat.add(mRequestType);//PUBLIC
	if(includeDataField)//do not include when used for Poly1305; we would need to update again after encryption and ciphertext gets authenticated anyway
	concat.add(mData);// - - - ENCRYPTED - - - 

	std::vector<uint8_t> hash = crypto->getSHA2_256Vec(concat.getData());

	return hash;
}

bool CNetMsg::sign(Botan::secure_vector<uint8_t> privKey)
{
	if (privKey.size() == 0)
		return false;//additional verificiation later on
	//mFlags.authenticated = true; WARNING: DO NOT modify the flags. ANY modifications to flags should have been performed by user already EXPLICITLY.
    //BEFORE the signature is performed.

	//usually we we would have signed the ber-encoding, here so no to include reliance on particular encoder
	//(we can't since browser might be running a very different implementation) we sign the RAW data (the inertia).


	std::vector<uint8_t> hash = getImage();//data/ciphertext gets also signed

	mSig = CCryptoFactory::getInstance()->signData(hash, privKey);

	if (mSig.size() > 0)
		return true;
	
	return false;
}

bool CNetMsg::verifySig(std::vector<uint8_t> pubKey)
{

	if (pubKey.size() == 0)
		return false;//additional verificiation later on

	//usually we we would have signed the ber-encoding, here so no to include reliance on particular encoder
	//(we can't since browser might be running a very different implementation) we sign the RAW data (the inertia).
	
	std::vector<uint8_t> hash = getImage();

	return  CCryptoFactory::getInstance()->verifySignature(mSig, hash, pubKey);


	return false;
}

std::vector<uint8_t> CNetMsg::getDestination()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDestination;
}

std::string CNetMsg::getFormatedDestination()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	std::string toRet;

	std::shared_ptr<CTools> tools = CTools::getInstance();

	switch (mDestinationType)
	{
	case eEndpointType::IPv4:
		toRet = tools->bytesToString(mDestination);
		
		break;
	default:
		toRet = tools->base58CheckEncode(mDestination);
	
		break;
	}
	toRet = tools->endpointTypeToString(mDestinationType) + "(" + toRet+ ")";
	return toRet;
}

std::string CNetMsg::getFormatedSource()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	std::string toRet;
	std::shared_ptr<CTools> tools = CTools::getInstance();

	switch (mSourceType)
	{
	case eEndpointType::IPv4:
		toRet = CTools::getInstance()->bytesToString(mSource);

		break;
	default:
		toRet = CTools::getInstance()->base58CheckEncode(mSource);

		break;
	}
	toRet = tools->endpointTypeToString(mSourceType) + "(" + toRet + ")";
	return toRet;
}





void CNetMsg::setDestinationType(eEndpointType::eEndpointType eType)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDestinationType = eType;

}

/// <summary>
/// Returns a brief description of the CNetMsg container.
/// </summary>
/// <param name="includeAddresses"></param>
/// <param name="includeEndpointTypes"></param>
/// <returns></returns>
std::string CNetMsg::getDescription(bool includeAddresses, bool includeEndpointTypes, bool includeContainerType, bool describeFlags)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	std::string toRet;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	bool base58sourceID = true;
	bool base58destinationID = true;
	
	if (includeContainerType)
	{
		toRet += "[Entity]:" + tools->msgEntTypeToString(mEntityType) + " [ReqType]:" + tools->msgReqTypeToString(mRequestType);
		toRet += "[Payload]:" + std::to_string(mData.size()) + " bytes";
	}

	if (includeAddresses || includeEndpointTypes)
	{
		toRet += " ";

		if (mSourceType == eEndpointType::IPv4 || mSourceType == eEndpointType::MAC || mSourceType == eEndpointType::PeerID)
			base58sourceID = false;

		if (mSource.size() == 32)
			base58sourceID = true;//most probably it's a public key

		if (mDestinationType == eEndpointType::IPv4 || mDestinationType == eEndpointType::MAC || mDestinationType == eEndpointType::PeerID)
			base58destinationID = false;

		if (mDestination.size() == 32)
			base58destinationID = true;//most probably it's a public key


		toRet += "[Source]:" + (includeEndpointTypes?tools->endpointTypeToString(mSourceType):"") 
			+ "(" + std::string(base58sourceID ? tools->base58CheckEncode(mSource) : tools->bytesToString(mSource)) + ")";

		if (toRet.size() > 0)
			toRet += " ";

		toRet += "[Destination]:" + (includeEndpointTypes ? tools->endpointTypeToString(mDestinationType) : "")
			+ "(" + std::string(base58destinationID ? tools->base58CheckEncode(mDestination) : tools->bytesToString(mDestination)) + ")";

	}

	if (describeFlags)
	{
		toRet += " -[FLAGS]-: " + tools->netMsgFlagsToString(mFlags);
	}

	
	return toRet;
}
uint64_t CNetMsg::getNewSourceSeq()
{
	std::lock_guard<std::mutex> lock(sRecentSourceSeqGuardian);
	return ++sRecentSourceSeq;
}
void CNetMsg::setDestination(std::vector<uint8_t> id)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDestination = id;
}

std::vector<uint8_t> CNetMsg::getSource()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mSource;
}

void CNetMsg::setSourceType(eEndpointType::eEndpointType eType)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mSourceType = eType;
}

eEndpointType::eEndpointType  CNetMsg::getSourceType()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mSourceType;
}

void CNetMsg::setSource(std::vector<uint8_t> id)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mSource =id;
}

uint64_t CNetMsg::getHops()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mHops;
}

void CNetMsg::incHops()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mHops++;
}

void CNetMsg::setHopCount(uint64_t val)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mHops = val;
}

void CNetMsg::setSourceSeq(uint64_t seq)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	 mSourceSeq = seq;
}

void CNetMsg::setDestSeq(uint64_t seq)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDestSeq = seq;
}

bool CNetMsg::hasData()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mData.size()>0;
}

uint64_t CNetMsg::getSourceSeq()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mSourceSeq;
}

uint64_t CNetMsg::getDestSeq()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDestSeq;
}

std::shared_ptr<CTransmissionToken> CNetMsg::getTT()
{
	return mTT;
}

void CNetMsg::setTT(std::shared_ptr<CTransmissionToken> tt)
{
	mTT = tt;
}
