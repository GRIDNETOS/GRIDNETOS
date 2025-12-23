#include "SDPEntity.h"

#include <memory>
#include "CryptoFactory.h"
#include "TransmissionToken.h"

std::mutex CSDPEntity::sRecentSourceSeqGuardian;
uint64_t CSDPEntity::sRecentSourceSeq = 0;

void CSDPEntity::initFields()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mTimeCreated = CTools::getInstance()->getTime();
    mHopCount = 0;
    mType = eSDPEntityType::joining;
    mPending = true;
    mVersion = 1;
    mSeqNr = getNewSourceSeq();
    mMarkedForDeletion = false;
    mStatus = eSDPControlStatus::ok;
    mCapabilities = eConnCapabilities::audioVideo;
}
uint64_t CSDPEntity::getNewSourceSeq()
{
    std::lock_guard<std::mutex> lock(sRecentSourceSeqGuardian);
    return ++sRecentSourceSeq;
}
void CSDPEntity::setHopCount(uint64_t value)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mHopCount = value;
}
uint64_t CSDPEntity::getHopCount()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mHopCount;
    
}
eConnCapabilities::eConnCapabilities CSDPEntity::getCapabilities()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mCapabilities;
}

void CSDPEntity::markForDeletion(bool doIt = true)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mMarkedForDeletion = doIt;

}
bool  CSDPEntity::getIsMarkedForDeletion()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mMarkedForDeletion;

}


void CSDPEntity::setCapabilities(eConnCapabilities::eConnCapabilities capabilities)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    this->mCapabilities = capabilities;
}

std::shared_ptr<CTransmissionToken> CSDPEntity::getTT()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mTT;
}

void CSDPEntity::setTT(std::shared_ptr<CTransmissionToken> tt)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mTT = tt;
}

void CSDPEntity::setStatus(eSDPControlStatus::eSDPControlStatus status)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mStatus = status;
}

eSDPControlStatus::eSDPControlStatus CSDPEntity::getStatus()
{
    return mStatus;
}

bool CSDPEntity::validateSignature(std::vector<uint8_t> pubKey)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (pubKey.size() == 0 || mSig.size() == 0)
        return  false;
    std::shared_ptr<CCryptoFactory> crypto = CCryptoFactory::getInstance();
    std::vector<uint8_t> packedData = getPackedData(false);
   return  crypto->verifySignature(mSig, packedData, pubKey);
}

bool CSDPEntity::sign(Botan::secure_vector<uint8_t> privKey)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    std::shared_ptr<CCryptoFactory> crypto = CCryptoFactory::getInstance();
    std::vector<uint8_t> packedData = getPackedData(false);
    std::vector<uint8_t> sig = crypto->signData(packedData, privKey);

    if (sig.size() == 64)
    {
        mSig = sig;
        return true;
    }

    return false;
}
std::vector<uint8_t> CSDPEntity::getSourceID()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mSourceID;
}

std::vector<uint8_t> CSDPEntity::getDestinationID()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mDestinationID;
}

void CSDPEntity::setSeqNr(uint64_t seqNr)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mSeqNr = seqNr;
}

uint64_t CSDPEntity::getSeqNr()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mSeqNr;
}


CSDPEntity::CSDPEntity(std::shared_ptr<CConversation> conversation, eSDPEntityType::eSDPEntityType type, std::vector<uint8_t> swarmID,std::vector<uint8_t> source, std::vector<uint8_t> destination, std::vector<uint8_t> SIPData)
{
    initFields();
    mConversation = conversation;
    mType = type;
    mSDPData = SIPData;
    mSourceID = source;
    mDestinationID = destination;
    mSwarmID = swarmID;

}
std::shared_ptr<CConversation> CSDPEntity::getConversation()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mConversation;
}

 void CSDPEntity::setConversation(std::shared_ptr<CConversation> conversation)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
     mConversation = conversation;
}


eSDPEntityType::eSDPEntityType CSDPEntity::getType()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mType;
}

std::vector<uint8_t> CSDPEntity::getPackedData(bool includeSig)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    std::vector<uint8_t> sig = includeSig ? mSig : std::vector<uint8_t>();
    std::vector<uint8_t> encodedTT;
    if (mTT != nullptr)
        encodedTT = mTT->getPackedData();

    Botan::DER_Encoder enc;
    enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(static_cast<size_t>(mVersion))
        .encode(static_cast<size_t>(mType))
        .start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(static_cast<size_t>(mCapabilities))
        .encode(static_cast<size_t>(mStatus))
        .encode(static_cast<size_t>(mSeqNr))
        .encode(static_cast<size_t>(mTimeCreated))
        .encode(mSwarmID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mSourceID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mDestinationID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mSDPData, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mSDPSessionID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mExtraData, Botan::ASN1_Tag::OCTET_STRING)
        .encode(sig, Botan::ASN1_Tag::OCTET_STRING)
        .encode(encodedTT, Botan::ASN1_Tag::OCTET_STRING)
        .end_cons().end_cons();
    return enc.get_contents_unlocked();
}

std::shared_ptr<CSDPEntity> CSDPEntity::instantiate(std::vector<uint8_t> packedData)
{
    try {
        std::shared_ptr<CSDPEntity> msg = std::make_shared<CSDPEntity>();
        size_t version, typE = 0;
        Botan::BER_Decoder dec = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE).
            decode(version).decode(typE);
        std::vector<uint8_t> encodedTT;
        msg->mVersion = version;
        msg->mType = static_cast<eSDPEntityType::eSDPEntityType>(typE);

        Botan::BER_Object obj;
        if (msg->mVersion == 1)
        {
            size_t result = 0;
            std::vector<uint8_t> adr;
            obj = dec.get_next_object();
            if (obj.type_tag != Botan::ASN1_Tag::SEQUENCE)
                return false;

            Botan::BER_Decoder  dec2 = Botan::BER_Decoder(obj.value);
            dec2.decode(reinterpret_cast<size_t&>(msg->mCapabilities));
            dec2.decode(reinterpret_cast<size_t&>( msg->mStatus));
            dec2.decode(msg->mSeqNr);
            dec2.decode(msg->mTimeCreated);
            dec2.decode(msg->mSwarmID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(msg->mSourceID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(msg->mDestinationID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(msg->mSDPData, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(msg->mSDPSessionID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(msg->mExtraData, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(msg->mSig, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(encodedTT, Botan::ASN1_Tag::OCTET_STRING);
            dec2.verify_end();

            if (encodedTT.size() > 0)
            {
                msg->mTT = CTransmissionToken::instantiate(encodedTT);
                if (msg->mTT == nullptr)
                    return nullptr;
            }
        }
        else 	return nullptr;
        return msg;
    }
    catch (...)
    {
        return nullptr;
    }
}

bool CSDPEntity::getIsPending()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mPending;
}
void CSDPEntity::setSDPSessionID(std::vector<uint8_t> id)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
     mSDPSessionID = id;
}
uint64_t CSDPEntity::getSDPSessionIDAsUInt()//timestamp?
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (mSDPSessionID.size() <= 8)
    {
        return CTools::getInstance()->bytesToUint64(mSDPSessionID);
    }
    return 0;
}
std::vector<uint8_t> CSDPEntity::getSDPSessionID()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mSDPSessionID;
}
size_t CSDPEntity::getTimeCreated()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mTimeCreated;
}


void CSDPEntity::setIsPending(bool isIt)
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    mPending = isIt;
}

std::vector<uint8_t> CSDPEntity::getSwarmID()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mSwarmID;
}

bool operator==(const CSDPEntity& sdp1, const CSDPEntity& sdp2)
{
    std::shared_ptr<CTools> tools = CTools::getInstance();

    return (sdp1.mType == sdp2.mType && sdp1.mTimeCreated == sdp2.mTimeCreated && sdp1.mPending == sdp2.mPending &&
        tools->compareByteVectors(sdp1.mSwarmID, sdp2.mSwarmID) && tools->compareByteVectors(sdp1.mSourceID , sdp2.mSourceID)
        && tools->compareByteVectors(sdp1.mDestinationID, sdp2.mDestinationID) && tools->compareByteVectors(sdp1.mSDPData, sdp2.mSDPData)
        && tools->compareByteVectors(sdp1.mSig, sdp2.mSig), tools->compareByteVectors(sdp1.mExtraData, sdp2.mExtraData) &&
        sdp1.mSeqNr == sdp2.mSeqNr && sdp1.mVersion == sdp2.mVersion);
}

bool operator!=(const CSDPEntity& sdp1, const CSDPEntity& sdp2)
{
    return !(sdp1 == sdp2);
}
