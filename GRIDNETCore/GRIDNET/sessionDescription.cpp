#include "SessionDescription.h"

CSessionDescription::CSessionDescription()
{
    initFields();
}
CSessionDescription::CSessionDescription(const CSessionDescription& sibling)
{
    mVersion = sibling.mVersion;
    mAddress = sibling.mAddress;
    mPubKey = sibling.mPubKey;
    mComPubKey = sibling.mPubKey;
    mConversationID = sibling.mConversationID;
    mFlags = sibling.mFlags;
    mChallange = sibling.mChallange;
}
void CSessionDescription::initFields()
{
    reinterpret_cast<uint8_t&>(mFlags) = 0;
    mVersion = 1;
}

uint64_t CSessionDescription::getVersion()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mVersion;
}

std::string CSessionDescription::getDescription()
{
    std::string toRet;
    std::shared_ptr<CTools> tools = CTools::getInstance();
    toRet += "[Version]:" + std::to_string(mVersion);
    toRet += " [Flags]:" + tools->sessionDescriptionFlagsToString(mFlags);
    toRet +=  std::string(" [Address]:" + (mAddress.size() == 32 ? tools->bytesToString(mAddress) : "none")); 
    toRet += std::string(" [PubKey]:" + (mPubKey.size()==32?tools->base58CheckEncode(mPubKey):"none"));
    toRet += std::string(" [ComPubKey]:" + (mComPubKey.size() == 32 ? tools->base58CheckEncode(mComPubKey) : "none"));
    toRet += std::string(" [ConversationID]:" + (mConversationID.size() > 0 ? tools->base58CheckEncode(mConversationID) : "none"));
    toRet += std::string(" [Challange]:" + (mChallange.size() == 32 ? tools->base58CheckEncode(mChallange) : "none")); 
    return toRet;
}

void CSessionDescription::setChallangeData(std::vector<uint8_t> challangeData)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mChallange = challangeData;
}
std::vector<uint8_t> CSessionDescription::getChallangeData()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mChallange;
}
void CSessionDescription::setConversationID(std::vector<uint8_t> id)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mConversationID = id;
}
std::vector<uint8_t> CSessionDescription::getConversationID()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mConversationID;
}
sdFlags CSessionDescription::getFlags()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mFlags;
}

void CSessionDescription::setFlags(sdFlags flags)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mFlags = flags;
}
std::vector<uint8_t> CSessionDescription::getAddress()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mAddress;
}

void CSessionDescription::setAddress(std::vector<uint8_t> address)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mAddress = address;
}


std::vector<uint8_t> CSessionDescription::getPubKey()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mPubKey;
}

void CSessionDescription::setPubKey(std::vector<uint8_t> pubKey)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mPubKey = pubKey;
}

std::vector<uint8_t> CSessionDescription::getComPubKey()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mComPubKey;
}
void CSessionDescription::setComPubKey(std::vector<uint8_t> pubKey)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mComPubKey = pubKey;
}
std::vector<uint8_t> CSessionDescription::getPackedData()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    Botan::DER_Encoder enc;
    std::vector<uint8_t> ttBytes;
    std::vector<uint8_t> flagsBytes(1);
    flagsBytes[0] = reinterpret_cast<uint8_t&>(mFlags);
    /*
        uint64_t mVersion;
        std::vector<uint8_t> mNonce;//nonce to be used by ChaCha20 during consecutive communication
        std::vector<uint8_t> mAddress;
        std::vector<uint8_t> mPubKey;
        void initFields();
        sdFlags mFlags;
    */
    enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(static_cast<size_t>(mVersion))
        .encode(flagsBytes, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mAddress, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mPubKey, Botan::ASN1_Tag::OCTET_STRING)//main user's pub-key *NOT* to be used for encryption.
        .encode(mComPubKey, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mChallange, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mConversationID, Botan::ASN1_Tag::OCTET_STRING)
        .end_cons();
    return enc.get_contents_unlocked();
}
std::shared_ptr<CSessionDescription> CSessionDescription::instantiate(std::vector<uint8_t> packedData)
{

    try {
        if (packedData.size() == 0)
            return nullptr;
        std::shared_ptr<CSessionDescription> sd = std::make_shared<CSessionDescription>();
        size_t temp = 0;
        std::vector<uint8_t> ttBytes, flagBytes;
        Botan::BER_Decoder dec = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE).
            decode(reinterpret_cast<size_t&>(sd->mVersion));

        Botan::BER_Object obj;
        if (sd->getVersion() == 1)
        {
            dec.decode(flagBytes, Botan::ASN1_Tag::OCTET_STRING);
            if (flagBytes.size() != 1)
                return nullptr;
            reinterpret_cast<uint8_t&>(sd->mFlags) = flagBytes[0];

            dec.decode(sd->mAddress, Botan::ASN1_Tag::OCTET_STRING);
            dec.decode(sd->mPubKey, Botan::ASN1_Tag::OCTET_STRING);
            dec.decode(sd->mComPubKey, Botan::ASN1_Tag::OCTET_STRING);
            dec.decode(sd->mChallange, Botan::ASN1_Tag::OCTET_STRING);
            dec.decode(sd->mConversationID, Botan::ASN1_Tag::OCTET_STRING);
            dec.verify_end();

        }
        else 	return nullptr;
        return sd;
    }
    catch (...)
    {
        return nullptr;
    }
}
