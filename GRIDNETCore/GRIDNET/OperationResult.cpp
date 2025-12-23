#include "OperationResult.h"


std::string COperationStatus::getNotes()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mNotes;
}

void COperationStatus::setNotes(std::string notes)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mNotes = notes;
}

uint64_t COperationStatus::getReqID()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mReqID;
}

std::string COperationStatus::getDescription()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    //Local Variables - BEGIN
    std::string  toRet;
    std::shared_ptr<CTools> tools = CTools::getInstance();
    //Local Variables - END

    toRet += "[Scope]:" + tools->operationScopeToString(mScope);
    toRet += " [Status]:" + tools->operationStatusToString(mStatus);
    toRet += " [ReqID]:" + std::to_string(mReqID);
    toRet += " [Version]:" + std::to_string(mVersion);
    toRet += " [Receipt]:" +std::string( mReceipt != nullptr ? "Present" : "None");
    return toRet;
}

void  COperationStatus::setReqID(uint64_t id)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mReqID = id;
}


std::vector<uint8_t> COperationStatus::getID()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return  mID;
}

void COperationStatus::setID(std::vector<uint8_t> id)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mID = id;
}

eOperationStatus::eOperationStatus COperationStatus::getStatus()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return  mStatus;
}

eOperationScope::eOperationScope COperationStatus::getScope()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return  mScope;
}


COperationStatus::COperationStatus()
{
    initFields();

}
void COperationStatus::setReceipt(std::shared_ptr<CReceipt> receipt)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mReceipt = receipt;
}
std::shared_ptr<CReceipt> COperationStatus::getTransaction()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mReceipt;
}
uint64_t COperationStatus::getVersion()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return  mVersion;
}

COperationStatus::COperationStatus(eOperationStatus::eOperationStatus status, eOperationScope::eOperationScope scope)
{
    initFields();
    mScope = scope;
    mStatus = status;
}

void COperationStatus::initFields()
{
    mVersion = 1;
    mReqID = 0;
    mScope = eOperationScope::other;
    mStatus = eOperationStatus::inProgress;
}

std::vector<uint8_t> COperationStatus::getPackedData()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    Botan::DER_Encoder enc;
    std::vector<uint8_t> packedReceipt;

    if (mReceipt != nullptr)
        packedReceipt = mReceipt->getPackedData();

    enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
        .encode(static_cast<size_t>(mVersion))
        .encode(static_cast<size_t>(mReqID))
        .encode(static_cast<size_t>(mStatus))
        .encode(static_cast<size_t>(mScope))
        .encode(mID, Botan::ASN1_Tag::OCTET_STRING)
        .encode(packedReceipt, Botan::ASN1_Tag::OCTET_STRING)
        .encode(CTools::getInstance()->stringToBytes(mNotes), Botan::ASN1_Tag::OCTET_STRING)
        .end_cons();
    return enc.get_contents_unlocked();
}

std::shared_ptr<COperationStatus> COperationStatus::instantiate(std::vector<uint8_t> packedData)
{
    try {
        if (packedData.size() == 0)
            return nullptr;
        std::shared_ptr<COperationStatus> os = std::make_shared<COperationStatus>();
        size_t temp = 0;
        std::vector<uint8_t> receiptBytes, tempBytes;
        Botan::BER_Decoder dec = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE).
            decode(reinterpret_cast<size_t&>(os->mVersion));

        Botan::BER_Object obj;
        if (os->getVersion() == 1)
        {
            dec.decode(reinterpret_cast<size_t&>(os->mReqID));
            dec.decode(reinterpret_cast<size_t&>(os->mStatus));
            dec.decode(reinterpret_cast<size_t&>(os->mScope));
            dec.decode(os->mID, Botan::ASN1_Tag::OCTET_STRING);
            dec.decode(receiptBytes, Botan::ASN1_Tag::OCTET_STRING);
            dec.decode(tempBytes, Botan::ASN1_Tag::OCTET_STRING);
            dec.verify_end();

            if (receiptBytes.size() > 0)
            {
                os->mReceipt = CReceipt::instantiate(packedData, eBlockchainMode::Unknown);
            }

            if (tempBytes.size() > 0)
            {
                os->mNotes =CTools::getInstance()->bytesToString(tempBytes);
            }

        }
        else return nullptr;
        return os;
    }
    catch (...)
    {
        return nullptr;
    }
}
