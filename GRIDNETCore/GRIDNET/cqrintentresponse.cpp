#include "cqrintentresponse.h"


#include "cqrintentresponse.h"
#include "cqrintentresponse.h"
#include "BlockchainManager.h"
CQRIntentResponse::CQRIntentResponse()
{

}

CQRIntentResponse::CQRIntentResponse(std::vector<uint8_t> terminalID, std::vector<uint8_t> sig, std::vector<uint8_t> pubKey, std::vector<uint8_t> data)
{
    mTerminalID = terminalID;
    mSignature = sig;
    mPubKey=pubKey;
    mData = data;

}

std::vector<uint8_t> CQRIntentResponse::getSig()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mSignature;
}

std::vector<uint8_t> CQRIntentResponse::getPubKey()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mPubKey;
}

std::vector<uint8_t> CQRIntentResponse::getDestinationID()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mTerminalID;
}

std::vector<uint8_t> CQRIntentResponse::getData()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mData;
}

void CQRIntentResponse::setData(std::vector<uint8_t> data)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mData = data;
}

/// <summary>
/// Gets BER-encoded, serialized QRIntent data. This data will be directly  rendered as a QR-code.
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CQRIntentResponse::getPackedData()
{
    std::lock_guard<std::mutex> lock(mGuardian);

    std::vector<uint8_t> dat;
    return Botan::DER_Encoder().start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(mTerminalID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mSignature, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mPubKey, Botan::ASN1_Tag::OCTET_STRING)
        .encode(mData, Botan::ASN1_Tag::OCTET_STRING)
        .end_cons().get_contents_unlocked();
}

//Instantiates a CQRIntentResponse from its BER-encoded representation.
std::shared_ptr<CQRIntentResponse> CQRIntentResponse::instantiate(std::vector<uint8_t> bytes)
{
    std::shared_ptr<CQRIntentResponse> response = nullptr;
    response = std::make_shared<CQRIntentResponse>();
    try {

       Botan::BER_Decoder decoder = Botan::BER_Decoder(bytes);

       if(!decoder.more_items())
           return nullptr;//no valid BER-data found within

               decoder.start_cons(Botan::ASN1_Tag::SEQUENCE)
               .decode(response->mTerminalID,Botan::ASN1_Tag::OCTET_STRING)
               .decode(response->mSignature, Botan::ASN1_Tag::OCTET_STRING)
               .decode(response->mPubKey, Botan::ASN1_Tag::OCTET_STRING)
               .decode(response->mData, Botan::ASN1_Tag::OCTET_STRING)
               .end_cons();
                return response;

    } catch (std::exception ex) {

        return nullptr;
    }
}

std::shared_ptr<CQRIntentResponse> CQRIntentResponse::instantiate(std::string &base58Encoded)
{
    std::vector<uint8_t> bytes;
    if(!CTools::getInstance()->base58CheckDecode(base58Encoded,bytes))
        return nullptr;

    return instantiate(bytes);

}
