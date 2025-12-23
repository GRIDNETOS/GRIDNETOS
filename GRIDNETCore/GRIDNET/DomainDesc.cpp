#include "DomainDesc.hpp"
#include "DomainDesc.hpp"
#include "Tools.h"
#include "IdentityToken.h"


CDomainDesc::CDomainDesc(const std::string& domain, uint64_t txInCount, uint64_t txOutCount, const BigInt& lockedBalance, const BigInt& balance,
    const BigInt& txTotalReceived, const BigInt& txTotalSent, const BigInt& GNCTotalMined, const BigInt& GNCTotalGenesis,
    uint64_t perspectivesCount,
    const std::vector<std::string>& perspectives, const std::string& perspective,
    std::shared_ptr<CIdentityToken> identityToken, std::shared_ptr<COperatorSecurityInfo> securityInfo, uint64_t nonce)
    : mDomain(domain), mTxInCount(txInCount), mTxOutCount(txOutCount), mLockedBalance(lockedBalance), mBalance(balance),
    mTxTotalReceived(txTotalReceived), mTxTotalSent(txTotalSent), mPerspectivesCount(perspectivesCount),
    mPerspectives(perspectives), mPerspective(perspective), mIdentityToken(identityToken),
    mSecurityInfo(securityInfo) , mGNCTotalGenesisRewards(GNCTotalGenesis), mGNCTotalMined(GNCTotalMined), mNonce(nonce)
{
    validateInputs();
}

// Add default constructor to match JavaScript implementation
CDomainDesc::CDomainDesc()
    : mDomain(""), mTxInCount(0), mTxOutCount(0), mLockedBalance(0), mBalance(0),
    mTxTotalReceived(0), mTxTotalSent(0), mPerspectivesCount(0),
    mPerspectives(), mPerspective(""), mIdentityToken(nullptr), mGNCTotalGenesisRewards(0), mGNCTotalMined(0) {
}

void CDomainDesc::validateInputs() {
    if (mDomain.empty() && !mIdentityToken) {
        throw std::invalid_argument("Either fully qualified domain address or an Identity Token must be available");
    }
    //if (mLockedBalance < BigInt(0) || mBalance < BigInt(0) || mTxTotalReceived < BigInt(0) ||
    //    mTxTotalSent < BigInt(0) || mIdentityToken->getConsumedCoins() < BigInt(0)) {
     //   throw std::invalid_argument("Balance and transaction values must be non-negative");
    //}
    //if (mPerspectives.empty()== false && (mPerspectivesCount != mPerspectives.size())) {  <- Perspectives count might be set while perspectives themselves might not be provided ( data exchange efficiency ).
    //    throw std::invalid_argument("Perspectives count mismatch");
    //}
}
bool CDomainDesc::getPackedData(std::vector<uint8_t> & bytes) const {
    std::shared_lock lock(mGuardian);
    try {
        std::shared_ptr<CTools> tools = CTools::getInstance();
        Botan::DER_Encoder enc;
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
            .encode(tools->stringToBytes(mDomain), Botan::ASN1_Tag::OCTET_STRING)
            .encode(mNonce)
            .encode(mTxInCount)
            .encode(mTxOutCount)
            .encode(tools->BigIntToBytes(mLockedBalance), Botan::ASN1_Tag::OCTET_STRING)
            .encode(tools->BigIntToBytes(mBalance), Botan::ASN1_Tag::OCTET_STRING)
            .encode(tools->BigIntToBytes(mTxTotalReceived), Botan::ASN1_Tag::OCTET_STRING)
            .encode(tools->BigIntToBytes(mTxTotalSent), Botan::ASN1_Tag::OCTET_STRING)
            .encode(tools->BigIntToBytes(mGNCTotalGenesisRewards), Botan::ASN1_Tag::OCTET_STRING)
            .encode(tools->BigIntToBytes(mGNCTotalMined), Botan::ASN1_Tag::OCTET_STRING)
            .encode(mPerspectivesCount);

        enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
        for (const auto& persp : mPerspectives) {
            enc.encode(tools->stringToBytes(persp), Botan::ASN1_Tag::OCTET_STRING);
        }
        enc.end_cons();

        enc.encode(tools->stringToBytes(mPerspective), Botan::ASN1_Tag::OCTET_STRING);

        if (mIdentityToken) {
            enc.encode(mIdentityToken->getPackedData(), Botan::ASN1_Tag::OCTET_STRING);
        }
        else {
            enc.encode(std::vector<uint8_t>(), Botan::ASN1_Tag::OCTET_STRING);
        }

        // Security Assessment Informatin - BEGIN
        if (mSecurityInfo) {
            std::vector<uint8_t> secInfoData;
            if (mSecurityInfo->getPackedData(secInfoData)) {
                enc.encode(secInfoData, Botan::ASN1_Tag::OCTET_STRING);
            }
            else {
                enc.encode(std::vector<uint8_t>(), Botan::ASN1_Tag::OCTET_STRING);
            }
        }
        else {
            enc.encode(std::vector<uint8_t>(), Botan::ASN1_Tag::OCTET_STRING);
        }
        // Security Assessment Informatin - END
        enc.end_cons();
        bytes =  enc.get_contents_unlocked();
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
    return false;
}

//todo: implement in JavaScript
std::shared_ptr<COperatorSecurityInfo> CDomainDesc::getSecurityInfo() const {
    std::shared_lock lock(mGuardian);
    return mSecurityInfo;
}


void CDomainDesc::setSecurityInfo(std::shared_ptr<COperatorSecurityInfo> secInfo) {
    std::unique_lock lock(mGuardian);
    mSecurityInfo = secInfo;
}

std::shared_ptr<CDomainDesc> CDomainDesc::instantiate(const std::vector<uint8_t>& packedData) {
    try {
        Botan::BER_Decoder dec(packedData);
        std::string domain, perspective;
        uint64_t txInCount, txOutCount, perspectivesCount, nonce;
        BigInt lockedBalance, balance, txTotalReceived, txTotalSent, GNCTotalMined, GNCTotalGenesis;
        std::vector<std::string> perspectives;
        std::vector<uint8_t> identityTokenData;

        std::vector<uint8_t> temp;
        std::shared_ptr<CTools> tools = CTools::getInstance();

        dec.start_cons(Botan::ASN1_Tag::SEQUENCE);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        domain = tools->bytesToString(temp);
        dec.decode(nonce);

        dec.decode(txInCount);
        dec.decode(txOutCount);
        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        lockedBalance = tools->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        balance = tools->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        txTotalReceived = tools->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        txTotalSent = tools->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        GNCTotalGenesis = tools->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        GNCTotalMined = tools->BytesToBigInt(temp);

        dec.decode(perspectivesCount);

        Botan::BER_Decoder perspDec = dec.start_cons(Botan::ASN1_Tag::SEQUENCE);
        while (perspDec.more_items()) {
            perspDec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
            perspectives.push_back(tools->bytesToString(temp));
        }
        perspDec.end_cons();

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        perspective = tools->bytesToString(temp);

        dec.decode(identityTokenData, Botan::ASN1_Tag::OCTET_STRING);

        // Operator Security Assessment - BEGIN
        std::vector<uint8_t> securityInfoData;
        dec.decode(securityInfoData, Botan::ASN1_Tag::OCTET_STRING);
        std::shared_ptr<COperatorSecurityInfo> securityInfo = nullptr;
        if (!securityInfoData.empty()) {
            securityInfo = COperatorSecurityInfo::instantiate(securityInfoData);
        }
        // Operator Security Assessment - END
   
        dec.end_cons();

        std::shared_ptr<CIdentityToken> identityToken = nullptr;
        if (!identityTokenData.empty()) {
            identityToken = CIdentityToken::instantiate(identityTokenData);
        }

        return std::make_shared<CDomainDesc>(domain, txInCount,txOutCount, lockedBalance, balance, txTotalReceived,
            txTotalSent, GNCTotalMined, GNCTotalGenesis, perspectivesCount, perspectives, perspective, identityToken, securityInfo, nonce);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Deserialization failed: ") + e.what());
    }
}


uint64_t CDomainDesc::getNonce() const
{
    return mNonce;
}

// Getter methods
std::string CDomainDesc::getDomain() const { return mDomain; }
uint64_t CDomainDesc::getTxCount() const { return (mTxInCount + mTxOutCount); }
std::string CDomainDesc::getTxCountTxt() const { return std::to_string(getTxCount()); }


uint64_t CDomainDesc::getTxInCount() const { return (mTxInCount); }
std::string CDomainDesc::getTxInCountTxt() const { return std::to_string(mTxInCount); }

uint64_t CDomainDesc::getTxOutCount() const { return (mTxOutCount); }
std::string CDomainDesc::getTxOutCountTxt() const { return std::to_string(mTxOutCount); }
/**
* @brief GNC (Cryptocurrency) Value Management Methods
* These methods handle retrieval and formatting of various GNC balance types:
* - Locked balance: Funds locked in special operations
* - Available balance: Currently spendable funds
* - Total received: Sum of all incoming transactions
* - Total sent: Sum of all outgoing transactions
*
* Each value has two accessor methods:
* - Raw value getter returning BigInt
* - Formatted text getter with precision-based formatting and caching
*/

// Locked Balance Methods
BigInt CDomainDesc::getLockedBalance() const {
    return mLockedBalance;
}

std::string CDomainDesc::getLockedBalanceTxt(uint64_t precision) {
    // Return cached string if available
    if (mLockedBalanceStr.empty() == false) {
        return mLockedBalanceStr;
    }

    // Format and cache the value
    mLockedBalanceStr = CTools::formatGNCValue(mLockedBalance, precision);
    return mLockedBalanceStr;
}

// Available Balance Methods
BigInt CDomainDesc::getBalance() const {
    return mBalance;
}

std::string CDomainDesc::getBalanceTxt(uint64_t precision) {
    // Return cached string if available
    if (mBalanceStr.empty() == false) {
        return mBalanceStr;
    }

    // Format and cache the value
    mBalanceStr = CTools::formatGNCValue(mBalance, precision);
    return mBalanceStr;
}

// Total Received Methods
BigInt CDomainDesc::getTxTotalReceived() const {
    return mTxTotalReceived;
}

BigInt CDomainDesc::getGNCTotalMined() const
{
    return mGNCTotalMined;
}

BigInt CDomainDesc::getGNCTotalGenesisRewardsReceived() const
{
    return mGNCTotalGenesisRewards;
}

std::string CDomainDesc::getTxTotalReceivedTxt(uint64_t precision) {
    // Return cached string if available
    if (mTxTotalReceivedStr.empty() == false) {
        return mTxTotalReceivedStr;
    }

    // Format and cache the value
    mTxTotalReceivedStr = CTools::formatGNCValue(mTxTotalReceived, precision);
    return mTxTotalReceivedStr;
}

std::string CDomainDesc::getGNCTotalMinedTxt(uint64_t precision)
{
    // Return cached string if available
    if (mGNCTotalMinedStr.empty() == false) {
        return mGNCTotalMinedStr;
    }

    // Format and cache the value
    mGNCTotalMinedStr = CTools::formatGNCValue(mGNCTotalMined, precision);
    return mGNCTotalMinedStr;
}

std::string CDomainDesc::getGNCTotalGenesisRewardsReceivedTxt(uint64_t precision)
{
    // Return cached string if available
    if (mGNCTotalGenesisRewardsStr.empty() == false) {
        return mGNCTotalGenesisRewardsStr;
    }

    // Format and cache the value
    mGNCTotalGenesisRewardsStr = CTools::formatGNCValue(mGNCTotalGenesisRewards, precision);
    return mGNCTotalGenesisRewardsStr;
}

// Total Sent Methods
BigInt CDomainDesc::getTxTotalSent() const {
    return mTxTotalSent;
}

std::string CDomainDesc::getTxTotalSentTxt(uint64_t precision) {
    // Return cached string if available
    if (mTxTotalSentStr.empty() == false) {
        return mTxTotalSentStr;
    }

    // Format and cache the value
    mTxTotalSentStr = CTools::formatGNCValue(mTxTotalSent, precision);
    return mTxTotalSentStr;
}

uint64_t CDomainDesc::getPerspectivesCount() const { return mPerspectivesCount; }
std::vector<std::string> CDomainDesc::getPerspectives() const { return mPerspectives; }
std::string CDomainDesc::getPerspective() const { return mPerspective; }

std::shared_ptr<CIdentityToken> CDomainDesc::getIdentityToken() const {
    if (!mIdentityToken) return nullptr;

    return mIdentityToken;
}
