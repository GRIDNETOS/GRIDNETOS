#include "BlockDesc.hpp"
#include "BlockDesc.hpp"
#include "BlockDesc.hpp"
#include "TransactionDesc.hpp"
#include "CGlobalSecSettings.h"
CBlockDesc::CBlockDesc(uint64_t coreVersion, const std::string& blockID, eBlockType::eBlockType type, uint64_t keyHeight, uint64_t height,
    uint64_t solvedAt, const std::string& minerID, double difficulty, uint64_t totalDifficulty,const std::string& parentID,
    const BigInt& ergUsed, const BigInt& ergLimit, uint64_t nonce, const BigInt& blockReward,
    const BigInt& totalReward,  const BigInt& totalRewardEffective, const BigInt& totalPaid, const BigInt& totalPaidEffective, uint64_t receiptsCount, uint64_t transactionsCount, uint64_t verifiables,
    const std::vector<std::shared_ptr<CTransactionDesc>>& transactions, uint64_t size)
    : mCoreVersion(coreVersion), mBlockID(blockID), mType(type), mKeyHeight(keyHeight), mHeight(height), mSolvedAt(solvedAt),
    mMinerID(minerID), mDifficulty(difficulty), mTotalDiff(totalDifficulty), mParentID(parentID), mErgUsed(ergUsed), mErgLimit(ergLimit),
    mNonce(nonce), mBlockReward(blockReward), mTotalReward(totalReward),
    mTotalRewardEffective(totalRewardEffective), mTotalPaid(totalPaid), mTotalPaidEffective(totalPaidEffective),
    mReceiptsCount(receiptsCount),
    mTransactionsCount(transactionsCount), mVerifiablesCount(verifiables), mTransactions(transactions), mSize(size) {
    validateInputs();



   // uint64_t realRewardL = CTools::getInstance()->attoToGNC(mTotalRewardEffective);
   //     realRewardL++;
    //    std::cout << realRewardL;
   // std::string t1 = totalRewardEffective.str();
    //std::string t2 = totalReward.str();
}
// Default constructor
CBlockDesc::CBlockDesc()
    :mCoreVersion(0), mBlockID(""), mType(eBlockType::dataBlock), mKeyHeight(0), mHeight(0), mSolvedAt(0),
    mMinerID(""), mDifficulty(0),mTotalDiff(0), mParentID(""), mErgUsed(0), mErgLimit(0),
    mNonce(0), mBlockReward(0), mTotalReward(0), mReceiptsCount(0),
    mTransactionsCount(0), mVerifiablesCount(0), mTransactions(), mTotalRewardEffective(0), mTotalPaidEffective(0), mSize(0){
}

uint64_t CBlockDesc::getCoreVersion()
{
    std::shared_lock  lock(mGuardian);
    return mCoreVersion;
}

void CBlockDesc::validateInputs()
{
    // Block identifier validation
    if (mBlockID.empty()) {
        addAdditionalAnalysisHint("Block ID not specified");
    }
    if (mParentID.empty() && mHeight!=0) {  // Parent ID can be empty for genesis block
        addAdditionalAnalysisHint("Parent block ID not specified for non-genesis block");
    }

    // ERG value validation
    if (mErgUsed < BigInt(0)) {
        addAdditionalAnalysisHint("Negative ERG usage specified");
    }
    if (mErgLimit < BigInt(0)) {
        addAdditionalAnalysisHint("Negative ERG limit specified");
    }
    if (mBlockReward < BigInt(0)) {
        addAdditionalAnalysisHint("Negative block reward specified");
    }
    if (mTotalReward < BigInt(0)) {
        addAdditionalAnalysisHint("Negative total reward specified");
    }

    // ERG consistency checks
    if (mErgUsed > mErgLimit) {
        addAdditionalAnalysisHint("ERG usage exceeds specified limit");
    }
    if (mTotalReward < mBlockReward) {
        addAdditionalAnalysisHint("Total reward less than block reward");
    }

    // Difficulty validation
    if (mDifficulty < 0) {
        addAdditionalAnalysisHint("Negative difficulty specified");
    }

    // Transaction count validation
    if (!mTransactions.empty()) {
        if (mTransactionsCount != mTransactions.size()) {
            addAdditionalAnalysisHint("Transaction count (" + std::to_string(mTransactionsCount) +
                ") does not match actual transactions size (" +
                std::to_string(mTransactions.size()) + ")");
        }

        // Additional transaction-related validations
        if (mType == eBlockType::keyBlock && !mTransactions.empty()) {
            addAdditionalAnalysisHint("Key block contains transactions");
        }

        uint64_t totalTxSize = 0;
        for (const auto& tx : mTransactions) {
            totalTxSize += tx->getSize();
        }
        if (totalTxSize > CGlobalSecSettings::getMaxDataBlockSize()) {
            addAdditionalAnalysisHint("Total transaction size exceeds maximum block size");
        }
    }

    // Time validation
    uint64_t currentTime = std::time(nullptr);
    if (mSolvedAt > currentTime) {
        addAdditionalAnalysisHint("Block solved timestamp is in the future");
    }

    // Height validation
    if (mHeight == 0 && mHeight) {
        addAdditionalAnalysisHint("Zero height specified for non-genesis block");
    }
    if (mType == eBlockType::keyBlock && mKeyHeight == 0 && mHeight) {
        addAdditionalAnalysisHint("Zero key height specified for non-genesis key block");
    }

    // Type-specific validations
    if (mType == eBlockType::keyBlock) {
        if (mMinerID.empty()) {
            addAdditionalAnalysisHint("Miner ID not specified for key block");
        }
        if (mNonce == 0) {
            addAdditionalAnalysisHint("Zero nonce in key block");
        }
    }

    // Count consistency checks
    if (mReceiptsCount > CGlobalSecSettings::getMaxReceiptsPerBlock()) {
        addAdditionalAnalysisHint("Receipts count exceeds maximum allowed per block");
    }
    if (mVerifiablesCount > CGlobalSecSettings::getMaxVerifiablesPerBlock()) {
        addAdditionalAnalysisHint("Verifiables count exceeds maximum allowed per block");
    }
}
std::vector<std::string> CBlockDesc::getAdditonalAnalysisHints()
{
    std::shared_lock lock(mGuardian);
    return mAdditionalAnalysisHints;
}

void CBlockDesc::addAdditionalAnalysisHint(const std::string& hint)
{
    std::unique_lock lock(mGuardian);
    mAdditionalAnalysisHints.push_back(hint);
}

CBlockDesc::CBlockDesc(const CBlockDesc& other)
    : mType(other.mType)
    , mCoreVersion(other.mCoreVersion)
    , mSize(other.mSize)
    , mKeyHeight(other.mKeyHeight)
    , mHeight(other.mHeight)
    , mSolvedAt(other.mSolvedAt)
    , mMinerID(other.mMinerID)
    , mDifficulty(other.mDifficulty)
    , mParentID(other.mParentID)
    , mErgUsed(other.mErgUsed)
    , mErgLimit(other.mErgLimit)
    , mNonce(other.mNonce)
    , mBlockReward(other.mBlockReward)
    , mTotalReward(other.mTotalReward)
    , mReceiptsCount(other.mReceiptsCount)
    , mTransactionsCount(other.mTransactionsCount)
    , mVerifiablesCount(other.mVerifiablesCount)
    , mBlockID(other.mBlockID)
    , mTotalRewardEffective(other.mTotalRewardEffective)
    , mTotalPaidEffective(other.mTotalPaidEffective)
{
    // Deep copy the transactions vector
    mTransactions.reserve(other.mTransactions.size());
    for (const auto& tx : other.mTransactions) {
        mTransactions.push_back(tx ? std::make_shared<CTransactionDesc>(*tx) : nullptr);
    }

    // Copy analysis hints
    mAdditionalAnalysisHints = other.mAdditionalAnalysisHints;

    // Note: mGuardian (shared_mutex) gets default constructed
}

// Add copy assignment operator
CBlockDesc& CBlockDesc::operator=(const CBlockDesc& other) {
    if (this != &other) {
        CBlockDesc temp(other);  // Copy-and-swap idiom
        std::swap(*this, temp);
    }
    return *this;
}

bool CBlockDesc::getPackedData(std::vector<uint8_t> & bytes) const {
    std::shared_lock  lock(mGuardian);
    try {
        std::shared_ptr<CTools> tools = CTools::getInstance();
        Botan::DER_Encoder enc;
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
            .encode(static_cast<size_t>(mCoreVersion))
            .encode(tools->stringToBytes(mBlockID), Botan::ASN1_Tag::OCTET_STRING)
            .encode(static_cast<size_t>(mType))
            .encode(static_cast<size_t>(mKeyHeight))
            .encode(static_cast<size_t>(mHeight))
            .encode(static_cast<size_t>(mSolvedAt))
            .encode(tools->stringToBytes(mMinerID), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->doubleToByteVector(mDifficulty), Botan::ASN1_Tag::OCTET_STRING)
            .encode(static_cast<size_t>(mTotalDiff))
            .encode(tools->stringToBytes(mParentID), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mErgUsed), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mErgLimit), Botan::ASN1_Tag::OCTET_STRING)
            .encode(static_cast<size_t>(mNonce))
            .encode(CTools::getInstance()->BigIntToBytes(mBlockReward), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mTotalReward), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mTotalRewardEffective), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mTotalPaid), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mTotalPaidEffective), Botan::ASN1_Tag::OCTET_STRING)
            .encode(static_cast<size_t>(mReceiptsCount))
            .encode(static_cast<size_t>(mTransactionsCount))
            .encode(static_cast<size_t>((mVerifiablesCount)))
            .encode(static_cast<size_t>((mSize)));


        enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
        for (const auto& tx : mTransactions) {
            std::vector<uint8_t> txBytes;
            if (!tx->getPackedData(txBytes))
                return false;

            enc.encode(txBytes, Botan::ASN1_Tag::OCTET_STRING);
        }
        enc.end_cons().end_cons();
        bytes =  enc.get_contents_unlocked();
        return true;
    }
    catch (const std::exception& e) {
      //  throw std::runtime_error(std::string("Serialization failed: ") + e.what());
        return false;
    }
}

std::shared_ptr<CBlockDesc> CBlockDesc::instantiate(const std::vector<uint8_t>& packedData) {
    try {
        Botan::BER_Decoder dec(packedData);
        std::string blockID, minerID, parentID;
        eBlockType::eBlockType type;
        std::shared_ptr<CTools> tools = CTools::getInstance();
        uint64_t coreVersion,keyHeight, height, solvedAt, nonce, receiptsCount, transactionsCount, verifiables, totalDifficulty, size;
        BigInt ergUsed, ergLimit, blockReward, totalReward, totalRewardEffective, totalPaid, totalPaidEffective;
        double difficulty;
        std::vector<std::shared_ptr<CTransactionDesc>> transactions;

        std::vector<uint8_t> temp;

        dec.start_cons(Botan::ASN1_Tag::SEQUENCE);
        dec.decode(coreVersion);
        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        blockID = tools->bytesToString(temp);

        size_t typeValue;
        dec.decode(typeValue);
        type = static_cast<eBlockType::eBlockType>(typeValue);

        dec.decode(keyHeight)
            .decode(height)
            .decode(solvedAt);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        minerID = tools->bytesToString(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        difficulty = CTools::getInstance()->bytesToDouble(temp);

        dec.decode(totalDifficulty);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        parentID = tools->bytesToString(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        ergUsed = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        ergLimit = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(nonce);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        blockReward = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        totalReward = CTools::getInstance()->BytesToBigInt(temp);
        //// new fields below
        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        totalRewardEffective = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        totalPaid = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        totalPaidEffective = CTools::getInstance()->BytesToBigInt(temp);
     

        dec.decode(receiptsCount)
            .decode(transactionsCount)
            .decode(verifiables)
            .decode(size);

        Botan::BER_Decoder txDec = dec.start_cons(Botan::ASN1_Tag::SEQUENCE);
        while (txDec.more_items()) {
            std::vector<uint8_t> txData;
            txDec.decode(txData, Botan::ASN1_Tag::OCTET_STRING);
            transactions.push_back(CTransactionDesc::instantiate(txData));
        }
        txDec.end_cons();
        dec.end_cons();

        return std::make_shared<CBlockDesc>(coreVersion, blockID, type, keyHeight, height, solvedAt, minerID, difficulty, totalDifficulty,
            parentID, ergUsed, ergLimit, nonce, blockReward, totalReward,totalRewardEffective,totalPaid,totalPaidEffective,
            receiptsCount, transactionsCount, verifiables, transactions, size);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Deserialization failed: ") + e.what());
    }
}

uint64_t CBlockDesc::getTotalDifficulty()
{
    return mTotalDiff;
}


CBlockDesc::~CBlockDesc()
{

   // delete mGuardian;

}

// Getter methods
std::string CBlockDesc::getBlockID() const { return mBlockID; }
eBlockType::eBlockType CBlockDesc::getType() const { return mType; }
uint64_t CBlockDesc::getKeyHeight() const { return mKeyHeight; }
uint64_t CBlockDesc::getHeight() const { return mHeight; }
uint64_t CBlockDesc::getSolvedAt() const { return mSolvedAt; }
std::string CBlockDesc::getMinerID() const { return mMinerID; }
double CBlockDesc::getDifficulty() const { return mDifficulty; }
std::string CBlockDesc::getParentID() const { return mParentID; }
BigInt CBlockDesc::getErgUsed() const { return mErgUsed; }
BigInt CBlockDesc::getErgLimit() const { return mErgLimit; }
uint64_t CBlockDesc::getNonce() const { return mNonce; }
BigInt CBlockDesc::getBlockReward() const { return mBlockReward; }
BigInt CBlockDesc::getTotalReward(bool getEffectiveValue) const
{
    if (getEffectiveValue && mTotalRewardEffective)
    {
 
        return mTotalRewardEffective;
    }
    return mTotalReward;
}


BigInt CBlockDesc::getTotalPaid(bool getEffectiveValue) const
{
    if (getEffectiveValue && mTotalPaidEffective)
    {
        return mTotalPaidEffective;
    }
    return mTotalPaid;
}

// Formatted string getters with effective flag
std::string CBlockDesc::getTotalRewardTxt(bool getEffectiveValue) const
{
    return CTools::formatGNCValue(getTotalReward(getEffectiveValue));
}

std::string CBlockDesc::getTotalPaidTxt(bool getEffectiveValue) const
{
    return CTools::formatGNCValue(getTotalPaid(getEffectiveValue));
}

uint64_t CBlockDesc::getSize() const { return mSize; }

uint64_t CBlockDesc::getReceiptsCount() const { return mReceiptsCount; }
uint64_t CBlockDesc::getTransactionsCount() const { return mTransactionsCount; }
uint64_t CBlockDesc::getVerifiablesCount() const { return mVerifiablesCount; }
std::vector<std::shared_ptr<CTransactionDesc>> CBlockDesc::getTransactions() const { return mTransactions; }

void CBlockDesc::clearTransactions()
{
    mTransactions.clear();
}

// Additional getters for formatted strings
std::string CBlockDesc::getSolvedAtTxt() const {
    return CTools::getInstance()->timeToString(mSolvedAt);
}

std::string CBlockDesc::getDifficultyTxt() const {
    return CTools::shortFormatNumber(mDifficulty);
}

std::string CBlockDesc::getErgUsedTxt() const {
    return  CTools::shortFormatNumber(mErgUsed);
}

std::string CBlockDesc::getErgLimitTxt() const {
    return  CTools::shortFormatNumber(mErgLimit);
}

std::string CBlockDesc::getBlockRewardTxt() const {
    return CTools::formatGNCValue(mBlockReward);
}
