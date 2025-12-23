#include "stdafx.h"
#include "minecraftPDU.h"
#include "TransmissionToken.h"

void CMinecraftPDU::setPoint(point3D& point)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mPoint = point;
}

bool CMinecraftPDU::getPoint(point3D& point)
{    
    std::lock_guard<std::mutex> lock(mGuardian);

    if (!mPoint.isKnown())
        return false;

    point = this->mPoint;
    
    return true;
}

CMinecraftPDU::CMinecraftPDU()
{
    initFields();
}
CMinecraftPDU::CMinecraftPDU(eGridcraftMsgType::eGridcraftMsgType type,  point3D point, std::shared_ptr<CTransmissionToken> token, std::vector<uint8_t> data )
{
    initFields();
    mType = type;
    mPoint = point;
    mTT = token;
}
std::vector<uint8_t> CMinecraftPDU::getData()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mData;
}
std::shared_ptr<CTransmissionToken> CMinecraftPDU::getTransmissionToken()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mTT;
}
void CMinecraftPDU::initFields()
{
    mVersion = 1;
    mType = eGridcraftMsgType::ping;
}


size_t CMinecraftPDU::getVersion()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mVersion;
}


std::string CMinecraftPDU::getPlayerID()
{ 
    std::lock_guard<std::mutex> lock(mGuardian);
    return CTools::getInstance()->bytesToString(mPlayerID);
}

eGridcraftMsgType::eGridcraftMsgType CMinecraftPDU::getType()
{    
    std::lock_guard<std::mutex> lock(mGuardian);
    return mType;
}

/// <summary>
/// Instantiates a Minecraft-specific PDU.
/// </summary>
/// <param name="packedData"></param>
/// <returns></returns>
std::shared_ptr<CMinecraftPDU> CMinecraftPDU::instantiate(std::vector<uint8_t> packedData)
{  
    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::shared_ptr<CMinecraftPDU> pdu = std::make_shared<CMinecraftPDU>();
    try {
        if (packedData.size() == 0)
            return nullptr;
        size_t temp = 0;
        std::vector<uint8_t> tempB;
        double tempF;

        Botan::BER_Decoder dec1 = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE).
            decode(reinterpret_cast<size_t&>(pdu->mVersion));


        double tempX = 0;
        double tempY = 0;
        double tempZ = 0;

        if (pdu->getVersion() ==1)
        {
            Botan::BER_Object obj = dec1.get_next_object();

            if (obj.type_tag != Botan::ASN1_Tag::SEQUENCE)
                return nullptr;

            Botan::BER_Decoder dec2 = Botan::BER_Decoder(obj.value);
            dec2.decode(temp);
            
            pdu->mType = static_cast<eGridcraftMsgType::eGridcraftMsgType>(temp);
            dec2.decode(pdu->mPlayerID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(tempB, Botan::ASN1_Tag::OCTET_STRING);
            tempX= tools->bytesToDouble(tempB);
            dec2.decode(tempB, Botan::ASN1_Tag::OCTET_STRING);
            tempY = tools->bytesToDouble(tempB);
            dec2.decode(tempB, Botan::ASN1_Tag::OCTET_STRING);
            tempZ = tools->bytesToDouble(tempB);
            point3D point = point3D(tempX, tempY, tempZ);

            pdu->setPoint(point);

            dec2.decode(tempB, Botan::ASN1_Tag::OCTET_STRING);
            pdu->mTT = CTransmissionToken::instantiate(tempB);
            dec2.decode(pdu->mData, Botan::ASN1_Tag::OCTET_STRING);

            dec2.verify_end();
        }
        else return nullptr;
        return pdu;
    }
    catch (...)
    {
        return nullptr;
    }
    return pdu;
}

/// <summary>
/// Retrieves a BER-encoded representation of a Minecraft PDU.
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CMinecraftPDU::getPackedData()
{    
    std::lock_guard<std::mutex> lock(mGuardian);
    std::shared_ptr<CTools> tools = CTools::getInstance();

    std::vector<uint8_t> posStub;
    bool cordsKnown = mPoint.isKnown();
    std::vector<uint8_t> bytes = Botan::DER_Encoder().start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(static_cast<size_t>(mVersion))
        .start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(static_cast<size_t>(mType))
        .encode(mPlayerID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(!cordsKnown?posStub: tools->doubleToByteVector(mPoint.getX()),Botan::ASN1_Tag::OCTET_STRING)
        .encode(!cordsKnown?posStub:tools->doubleToByteVector(mPoint.getY()),Botan::ASN1_Tag::OCTET_STRING)
        .encode(!cordsKnown?posStub:tools->doubleToByteVector(mPoint.getZ()),Botan::ASN1_Tag::OCTET_STRING)
        .encode(mTT!=nullptr ? mTT->getPackedData():std::vector<uint8_t>(), Botan::ASN1_Tag::OCTET_STRING)
        .encode(mData, Botan::ASN1_Tag::OCTET_STRING)
        .end_cons().end_cons().get_contents_unlocked();

    return bytes;
}

