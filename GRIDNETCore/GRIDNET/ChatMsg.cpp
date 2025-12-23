#include "stdafx.h"
#include "ChatMsg.h"
#include "CryptoFactory.h"
#include "DataConcatenator.h"

CChatMsg::CChatMsg(eChatMsgType::eChatMsgType type, std::vector<uint8_t> from, std::vector<uint8_t> to, std::vector<uint8_t> data, uint64_t timestamp, bool external)
{
    initFields();
    mFromID = from;
    mToID =to;

    mData = data;
    mTimestamp = timestamp;
    mExternal = external; //NOT serialized. optimization only
    mType = type;

}

eChatMsgType::eChatMsgType CChatMsg::getType()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return eChatMsgType::eChatMsgType();
}

std::vector<uint8_t> CChatMsg::getSourceID()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mFromID;
}

std::vector<uint8_t> CChatMsg::getDestinationID()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mToID;
}

uint64_t CChatMsg::getTimestamp()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mTimestamp;
}

std::string CChatMsg::getRendering(bool web)
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return std::string();
}

void CChatMsg::initialize()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    mInitialized = true;
}

uint64_t CChatMsg::getVersion()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mVersion;
}

std::vector<uint8_t> CChatMsg::getSig()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mSig;
}

void CChatMsg::sign(Botan::secure_vector<uint8_t> privKey)
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    CDataConcatenator concat;
    concat.add(mFromID);
    concat.add(mToID);
    concat.add(mVersion);
    concat.add(mData);
    concat.add(mTimestamp);
    concat.add(mType);
    mSig = CCryptoFactory::getInstance()->signData(concat.getData(), privKey);
}

bool CChatMsg::verifySignature(std::vector<uint8_t> pubKey)
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    CDataConcatenator concat;
    concat.add(mFromID);
    concat.add(mToID);
    concat.add(mVersion);
    concat.add(mData);
    concat.add(mTimestamp);
    concat.add(mType);

    return CCryptoFactory::getInstance()->verifySignature(mSig,concat.getData(),pubKey);
}




std::vector<uint8_t> CChatMsg::getPackedData(bool includeSig)
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);

    return Botan::DER_Encoder().start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(static_cast<size_t>(mVersion))
        .start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(static_cast<size_t>(mType))
        .encode(mFromID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mToID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mData, Botan::ASN1_Tag::OCTET_STRING)
        .encode(static_cast<size_t>(mTimestamp))
        .encode(includeSig?mSig: std::vector<uint8_t>(), Botan::ASN1_Tag::OCTET_STRING)//todo: do not include the field at all if not instructed to do so
        .end_cons().end_cons().get_contents_unlocked();
}

std::vector<uint8_t> CChatMsg::getData()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mData;
}

std::shared_ptr<CChatMsg> CChatMsg::instantiate(std::vector<uint8_t> packedData)
{
    std::shared_ptr<CChatMsg> msg = std::make_shared<CChatMsg>();
    try {
        if (packedData.size() == 0)
            return nullptr;
        size_t temp = 0;

        Botan::BER_Decoder dec1 = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE).
            decode(reinterpret_cast<size_t&>(msg->mVersion));

        Botan::BER_Object obj;

        if (msg->getVersion() ==1)
        {
            Botan::BER_Object ob = dec1.get_next_object();
            
            if (ob.type_tag != Botan::ASN1_Tag::SEQUENCE)
                return nullptr;

            Botan::BER_Decoder dec2 = Botan::BER_Decoder(ob.value);
            dec2.decode(temp);
            msg->mType = static_cast<eChatMsgType::eChatMsgType>(temp);
            dec2.decode(msg->mFromID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(msg->mToID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(msg->mData, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(temp);
            msg->mTimestamp = temp;
            dec2.decode(msg->mSig, Botan::ASN1_Tag::OCTET_STRING);
            dec2.verify_end();
        }
        else return nullptr;
        return msg;
    }
    catch (...)
    {
        return nullptr;
    }
    return msg;
}

void CChatMsg::initFields()
{
    mVersion = 1;

    mTimestamp = CTools::getInstance()->getTime();;
    mExternal = false; //NOT serialized. optimization only
    mInitialized = false;
    mType = eChatMsgType::text;

    if (mTimestamp == 0)
        mTimestamp = CTools::getInstance()->getTime();
}

bool operator==(const CChatMsg& c1, const CChatMsg& c2)
{
    std::shared_ptr<CTools> tools = CTools::getInstance();
    return tools->compareByteVectors(c1.mFromID, c2.mFromID)
        && tools->compareByteVectors(c1.mToID, c2.mToID)
        && tools->compareByteVectors(c1.mData, c2.mData)
        && tools->compareByteVectors(c1.mSig, c2.mSig)
        && c1.mVersion == c2.mVersion
        && c1.mTimestamp == c2.mTimestamp
        && c1.mExternal == c2.mExternal
        && c1.mType == c2.mType;
}

bool operator!=(const CChatMsg& c1, const CChatMsg& c2)
{
    return (c1 == c2)==false;
}
