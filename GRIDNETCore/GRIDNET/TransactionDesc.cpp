/**
 * @file TransactionDesc.cpp
 * @brief Implementation of the CTransactionDesc class for representing blockchain transaction descriptions.
 * @version 2.0
 * @date 2024-08-27
 *
 * This implementation adheres to military-grade software standards, including:
 * - Comprehensive error checking and handling
 * - Thread-safety measures
 * - Detailed in-code documentation
 * - Consistent naming conventions
 * - Robust input validation
 */

#include "TransactionDesc.hpp"
#include "Tools.h"
#include <stdexcept>
#include <algorithm>
#include "Receipt.h"
#include "BlockDesc.hpp"


CTransactionDesc::~CTransactionDesc()
{
    //delete mGuardian;
}

/**
  * @brief Constructs a CTransactionDesc object.
  *
  * @param result The result status of the transaction.
  * @param value The value transferred in the transaction.
  * @param sender The address of the sender.
  * @param receiver The address of the receiver.
  * @param ERGUsed The amount of ERG used in the transaction.
  * @param verifiableID The unique identifier for the transaction.
  * @param sacrificedValue The value sacrificed in the transaction.
  * @param sourceCode The source code associated with the transaction.
  * @param ERGLimit The maximum ERG allowed for the transaction.
  * @param nonce The nonce value for the transaction.
  * @param confirmedTimestamp The timestamp when the transaction was confirmed.
  * @param unconfirmedTimestamp The timestamp when the transaction was first seen unconfirmed.
  * @param type The transaction type (on-the-chain, off-the-chain etc.).
  * @param tax The tax covered by Sender as part of this transaction.
  * @param log An array of log messages associated with the transaction.
  * @param height The height of the block containing this transaction.
  * @param keyHeight The key height of the block containing this transaction.
  * @throws std::invalid_argument If input validation fails.
  */
CTransactionDesc::CTransactionDesc(uint64_t result, const BigInt& value, const std::string& sender, const std::string& receiver,
    const BigInt& ERGUsed, const BigInt& ERGPrice, const std::string& verifiableID, const BigInt& sacrificedValue,
    const std::string& sourceCode, const BigInt& ERGLimit, uint64_t nonce,
    uint64_t confirmedTimestamp, uint64_t unconfirmedTimestamp,
    eTxType::eTxType type, const BigInt& tax, const std::vector<std::string>& log,
    uint64_t height, uint64_t keyHeight, bool parsingResultValid, const std::vector<uint8_t>& blockID, uint64_t sizeInBytes)
    : mResult(result), mValue(value), mSender(sender), mReceiver(receiver), mERGUsed(ERGUsed), mERGPrice(ERGPrice),
    mVerifiableID(verifiableID), mSacrificedValue(sacrificedValue), mSourceCode(sourceCode),
    mERGLimit(ERGLimit), mNonce(nonce), mConfirmedTimestamp(confirmedTimestamp), mLastPreValidationResult(eTransactionValidationResult::received),
    mUnconfirmedTimestamp(unconfirmedTimestamp), mType(type), mTax(tax), mLog(log), mLastPreValidation(0),
    mHeight(height), mKeyHeight(keyHeight), mParsingResultValid(parsingResultValid), mBlockID(blockID), mSizeInBytes(sizeInBytes), mBytecodeVersion(0) {
    mLastPreValidation = 0;
    validateInputs();
}

uint64_t CTransactionDesc::getSize()
{
    return mSizeInBytes;
}

/**
 * @brief Validates the input parameters.
 * @throws std::invalid_argument If any parameter is invalid.
 */
void CTransactionDesc::validateInputs()
{
    if (mParsingResultValid)
    {
        // Check sender, receiver, and verifiableID
        if (mSender.empty()) {
            addAdditionalAnalysisHint("Sender address not specified");
        }
        if (mReceiver.empty()) {
            addAdditionalAnalysisHint("Receiver address not specified");
        }
        if (mVerifiableID.empty()) {
            addAdditionalAnalysisHint("Verifiable ID not specified");
        }

        // Check value and sacrificed value
        if (mValue <= BigInt(0) && mSacrificedValue <= BigInt(0)) {
            addAdditionalAnalysisHint("Neither transaction value nor sacrificed value is greater than zero");
        }

        // ERG related checks
        if (mERGUsed > mERGLimit) {
            addAdditionalAnalysisHint("ERG usage exceeds specified limit");
        }
        if (mERGLimit == BigInt(0)) {
            addAdditionalAnalysisHint("ERG limit not specified");
        }
    }
    else
    {
        addAdditionalAnalysisHint("Transaction parsing result not valid");
    }

    // Timestamp validation
    if (mConfirmedTimestamp)
    {
        // Three days in seconds
        constexpr uint64_t THREE_DHOURS_IN_SECONDS = 259200;  // 3 * 60 * 60

        // Check timestamp difference
        if (mUnconfirmedTimestamp > 0) {  // Only if unconfirmed timestamp exists
            uint64_t timeDiff = (mConfirmedTimestamp > mUnconfirmedTimestamp)
                ? mConfirmedTimestamp - mUnconfirmedTimestamp
                : mUnconfirmedTimestamp - mConfirmedTimestamp;

            if (timeDiff > THREE_DHOURS_IN_SECONDS) {
                addAdditionalAnalysisHint("Large time gap between confirmed and unconfirmed timestamps (> 3 days)");
            }
        }

        // Check for future timestamps
        uint64_t currentTime = std::time(nullptr);
        if (mConfirmedTimestamp > currentTime) {
            addAdditionalAnalysisHint("Confirmed timestamp is in the future");
        }
        if (mUnconfirmedTimestamp > currentTime) {
            addAdditionalAnalysisHint("Unconfirmed timestamp is in the future");
        }
    }

    // Additional checks
    if (mSourceCode.empty()) {
        addAdditionalAnalysisHint("Transaction contains code but source code is not provided");
    }

    if (mNonce == 0) {
        addAdditionalAnalysisHint("Transaction nonce not set");
    }

    // Tax validation
    if (mTax <= BigInt(0)) {
        addAdditionalAnalysisHint("Transaction tax not specified");
    }

    // Check for reasonable amounts
    if (mValue > BigInt("1000000000000000000000000")) {  // 1M GNC
        addAdditionalAnalysisHint("Unusually large transaction value");
    }

    // Log consistency check
    if (!mLog.empty() && mResult == eTransactionValidationResult::unknownIssuer) {
        addAdditionalAnalysisHint("Transaction has log entries but unknown issuer");
    }
}

/**
 * @brief Converts a string to a byte vector.
 * @param str The input string.
 * @return std::vector<uint8_t> The resulting byte vector.
 */
std::vector<uint8_t> CTransactionDesc::stringToBytes(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}


uint64_t CTransactionDesc::getHeight() const { return mHeight; }
uint64_t CTransactionDesc::getKeyHeight() const { return mKeyHeight; }

std::vector<std::string> CTransactionDesc::getAdditonalAnalysisHints()
{
    std::shared_lock lock(mGuardian);
    return mAdditionalAnalysisHints;
}
CTransactionDesc::CTransactionDesc(const CTransactionDesc& other)
    : mResult(other.mResult)
    , mValue(other.mValue)
    , mSender(other.mSender)
    , mReceiver(other.mReceiver)
    , mERGUsed(other.mERGUsed)
    , mERGPrice(other.mERGPrice)
    , mVerifiableID(other.mVerifiableID)
    , mSacrificedValue(other.mSacrificedValue)
    , mSourceCode(other.mSourceCode)
    , mERGLimit(other.mERGLimit)
    , mNonce(other.mNonce)
    , mConfirmedTimestamp(other.mConfirmedTimestamp)
    , mUnconfirmedTimestamp(other.mUnconfirmedTimestamp)
    , mSizeInBytes(other.mSizeInBytes)
    , mTax(other.mTax)
    , mLog(other.mLog)
    , mType(other.mType)
    , mBlockID(other.mBlockID)
    , mParsingResultValid(other.mParsingResultValid)
    , mHeight(other.mHeight)
    , mKeyHeight(other.mKeyHeight)
    , mLastPreValidation(other.mLastPreValidation)
    , mLastPreValidationResult(other.mLastPreValidationResult)
    , mAdditionalAnalysisHints(other.mAdditionalAnalysisHints)
    , mBytecodeVersion(other.mBytecodeVersion)
{
    // Note: mGuardian (shared_mutex) gets default constructed
}



CTransactionDesc& CTransactionDesc::operator=(const CTransactionDesc& other)
{
    if (this != &other) {
        CTransactionDesc temp(other);
        std::swap(*this, temp);
    }

    return *this;
}

// Add move constructor
CTransactionDesc::CTransactionDesc(CTransactionDesc&& other) noexcept
    : mResult(std::exchange(other.mResult, 0))
    , mValue(std::move(other.mValue))
    , mSender(std::move(other.mSender))
    , mReceiver(std::move(other.mReceiver))
    , mERGUsed(std::move(other.mERGUsed))
    , mERGPrice(std::move(other.mERGPrice))
    , mVerifiableID(std::move(other.mVerifiableID))
    , mSacrificedValue(std::move(other.mSacrificedValue))
    , mSourceCode(std::move(other.mSourceCode))
    , mERGLimit(std::move(other.mERGLimit))
    , mNonce(std::exchange(other.mNonce, 0))
    , mConfirmedTimestamp(std::exchange(other.mConfirmedTimestamp, 0))
    , mUnconfirmedTimestamp(std::exchange(other.mUnconfirmedTimestamp, 0))
    , mSizeInBytes(std::exchange(other.mSizeInBytes, 0))
    , mTax(std::move(other.mTax))
    , mLog(std::move(other.mLog))
    , mType(other.mType)
    , mBlockID(std::move(other.mBlockID))
    , mParsingResultValid(std::exchange(other.mParsingResultValid, false))
    , mHeight(std::exchange(other.mHeight, 0))
    , mKeyHeight(std::exchange(other.mKeyHeight, 0))
    , mLastPreValidation(other.mLastPreValidation)
    , mLastPreValidationResult(other.mLastPreValidationResult)
    , mAdditionalAnalysisHints(std::move(other.mAdditionalAnalysisHints))
    , mBytecodeVersion(std::exchange(other.mBytecodeVersion, 0))
{
    // Note: mGuardian gets default constructed
    // mLastPreValidationResult is already moved from 'other' in the initializer list
}
CTransactionDesc& CTransactionDesc::operator=(CTransactionDesc&& other) noexcept
{
    if (this != &other) {
        mResult = std::exchange(other.mResult, 0);
        mValue = std::move(other.mValue);
        mSender = std::move(other.mSender);
        mReceiver = std::move(other.mReceiver);
        mERGUsed = std::move(other.mERGUsed);
        mERGPrice= std::move(other.mERGPrice);
        mVerifiableID = std::move(other.mVerifiableID);
        mSacrificedValue = std::move(other.mSacrificedValue);
        mSourceCode = std::move(other.mSourceCode);
        mERGLimit = std::move(other.mERGLimit);
        mNonce = std::exchange(other.mNonce, 0);
        mConfirmedTimestamp = std::exchange(other.mConfirmedTimestamp, 0);
        mUnconfirmedTimestamp = std::exchange(other.mUnconfirmedTimestamp, 0);
        mSizeInBytes = std::exchange(other.mSizeInBytes, 0);
        mTax = std::move(other.mTax);
        mLog = std::move(other.mLog);
        mType = other.mType;
        mBlockID = std::move(other.mBlockID);
        mParsingResultValid = std::exchange(other.mParsingResultValid, false);
        mHeight = std::exchange(other.mHeight, 0);
        mKeyHeight = std::exchange(other.mKeyHeight, 0);
        mAdditionalAnalysisHints = std::move(other.mAdditionalAnalysisHints);
        mBytecodeVersion = std::exchange(other.mBytecodeVersion, 0);
        // Note: mGuardian is left as is since it's not movable
    }
    return *this;
}


void CTransactionDesc::addAdditionalAnalysisHint(const std::string& hint)
{
    std::unique_lock lock(mGuardian);
    mAdditionalAnalysisHints.push_back(hint);
}

uint64_t CTransactionDesc::getLastPreValidation()
{
    std::shared_lock lock(mGuardian);

    return mLastPreValidation;
}

void CTransactionDesc::pingLastPreValidation()
{
    std::unique_lock lock(mGuardian);
    mLastPreValidation = std::time(0);
}

eTransactionValidationResult CTransactionDesc::getLastPreValidationResult()
{
    std::shared_lock lock(mGuardian);

    return mLastPreValidationResult;
}

void CTransactionDesc::setLastPreValidationResult(eTransactionValidationResult result)
{
    std::shared_lock lock(mGuardian);
    mLastPreValidationResult = result;
    mLastPreValidation = std::time(0);
}

/**
 * @brief Converts a byte vector to a string.
 * @param bytes The input byte vector.
 * @return std::string The resulting string.
 */
std::string CTransactionDesc::bytesToString(const std::vector<uint8_t>& bytes) {
    return std::string(bytes.begin(), bytes.end());
}

/**
 * @brief Serializes the transaction description to a byte vector.
 * @param bytes The output byte vector.
 * @return bool True if serialization was successful, false otherwise.
 */
bool CTransactionDesc::getPackedData(std::vector<uint8_t>& bytes) const {
    std::shared_lock lock(mGuardian);
    try {
        Botan::DER_Encoder enc;
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
            .encode(static_cast<size_t>(mResult))
            .encode(CTools::getInstance()->BigIntToBytes(mValue), Botan::ASN1_Tag::OCTET_STRING)
            .encode(stringToBytes(mSender), Botan::ASN1_Tag::OCTET_STRING)
            .encode(stringToBytes(mReceiver), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mERGUsed), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mERGPrice), Botan::ASN1_Tag::OCTET_STRING)
            .encode(stringToBytes(mVerifiableID), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mSacrificedValue), Botan::ASN1_Tag::OCTET_STRING)
            .encode(stringToBytes(mSourceCode), Botan::ASN1_Tag::OCTET_STRING)
            .encode(CTools::getInstance()->BigIntToBytes(mERGLimit), Botan::ASN1_Tag::OCTET_STRING)
            .encode(static_cast<size_t>(mNonce))
            .encode(static_cast<size_t>(mConfirmedTimestamp))
            .encode(static_cast<size_t>(mUnconfirmedTimestamp))
            .encode(static_cast<size_t>(mType))
            .encode(CTools::getInstance()->BigIntToBytes(mTax), Botan::ASN1_Tag::OCTET_STRING);
          
            //.encode(mParsingResultValid);
        // Encode mLog
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
        for (const auto& logEntry : mLog) {
            enc.encode(stringToBytes(logEntry), Botan::ASN1_Tag::OCTET_STRING);
        }
        enc.end_cons();

        // Encode height and keyHeight after mLog
        enc.encode(mHeight)
            .encode(mKeyHeight)
            .encode(mBlockID, Botan::ASN1_Tag::OCTET_STRING)
            .encode(static_cast<uint64_t>(mParsingResultValid ? 1 : 0))
            .encode(mSizeInBytes)
            .encode(mBytecodeVersion);  // LAST FIELD: Bytecode version

        enc.end_cons();
        bytes = enc.get_contents_unlocked();
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

/**
 * @brief Deserializes a byte vector into a CTransactionDesc object.
 * @param packedData The byte vector containing the serialized transaction description.
 * @return std::shared_ptr<CTransactionDesc> A shared pointer to a new CTransactionDesc object.
 * @throws std::runtime_error If deserialization fails or data is invalid.
 */
std::shared_ptr<CTransactionDesc> CTransactionDesc::instantiate(const std::vector<uint8_t>& packedData) {
    try {
        Botan::BER_Decoder dec(packedData);
        uint64_t result, nonce, confirmedTimestamp, unconfirmedTimestamp, height, keyHeight, parsingResultValidValue, sizeInBytes;
        BigInt value, ERGUsed, ERGPrice, sacrificedValue, ERGLimit, tax;
        std::string sender, receiver, verifiableID, sourceCode;
        std::vector<std::string> log;
        std::vector<uint8_t> blockID;
        eTxType::eTxType type;

        std::vector<uint8_t> temp;

        dec.start_cons(Botan::ASN1_Tag::SEQUENCE)
            .decode(result)
            .decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        value = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        sender = bytesToString(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        receiver = bytesToString(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        ERGUsed = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        ERGPrice = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        verifiableID = bytesToString(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        sacrificedValue = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        sourceCode = bytesToString(temp);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        ERGLimit = CTools::getInstance()->BytesToBigInt(temp);

        dec.decode(nonce)
            .decode(confirmedTimestamp)
            .decode(unconfirmedTimestamp);

        size_t typeValue;
        dec.decode(typeValue);
        type = static_cast<eTxType::eTxType>(typeValue);

        dec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
        tax = CTools::getInstance()->BytesToBigInt(temp);

        // Decode mLog
        Botan::BER_Decoder logDec = dec.start_cons(Botan::ASN1_Tag::SEQUENCE);
        while (logDec.more_items()) {
            logDec.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
            log.push_back(bytesToString(temp));
        }
        logDec.end_cons();

        // Decode height and keyHeight after mLog
        dec.decode(height)
            .decode(keyHeight);
        dec.decode(blockID, Botan::ASN1_Tag::OCTET_STRING);
        dec.decode(parsingResultValidValue);
        dec.decode(sizeInBytes);

        // Bytecode version (LAST FIELD) - backwards compatible (default to 0 if not present)
        uint64_t bytecodeVersion = 0;
        if (dec.more_items()) {
            dec.decode(bytecodeVersion);
        }

        bool parsingResultValid = (parsingResultValidValue == 1);
        dec.end_cons();

        auto txDesc = std::make_shared<CTransactionDesc>(result, value, sender, receiver, ERGUsed, ERGPrice, verifiableID,
            sacrificedValue, sourceCode, ERGLimit, nonce, confirmedTimestamp, unconfirmedTimestamp,
            type, tax, log, height, keyHeight, parsingResultValid, blockID, sizeInBytes);
        txDesc->setBytecodeVersion(bytecodeVersion);
        return txDesc;
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("CTransactionDesc::instantiate: Deserialization failed: ") + e.what());
    }
}

bool CTransactionDesc::getParsingResultValid() const
{
    return mParsingResultValid;
}

void CTransactionDesc::setParsingResultValid(bool valid)
{
    mParsingResultValid = valid;
}

// Getter methods
std::string CTransactionDesc::getResultTxt(bool useANSI) const {
    std::shared_ptr<CTools> tools = CTools::getInstance();
    
    if (!useANSI)
    {
        return tools->transactionStatusText(mResult);
    }

    return tools->getColoredString(tools->transactionStatusText(mResult), tools->transactionStatusColor(mResult));

}

uint64_t CTransactionDesc::getResult() const { return mResult; }
const BigInt& CTransactionDesc::getValue() const { return mValue; }
const std::string& CTransactionDesc::getSender() const { return mSender; }
const std::string& CTransactionDesc::getReceiver() const { return mReceiver; }
const BigInt& CTransactionDesc::getERGUsed() const { return mERGUsed; }
const BigInt& CTransactionDesc::getERGPrice() const
{
    return mERGPrice;
}
const std::string& CTransactionDesc::getVerifiableID() const { return mVerifiableID; }
const BigInt& CTransactionDesc::getSacrificedValue() const { return mSacrificedValue; }
const std::string& CTransactionDesc::getSourceCode() const { return mSourceCode; }
const BigInt& CTransactionDesc::getERGLimit() const { return mERGLimit; }
uint64_t CTransactionDesc::getNonce() const { return mNonce; }
uint64_t CTransactionDesc::getConfirmedTimestamp() const { return mConfirmedTimestamp; }
uint64_t CTransactionDesc::getUnconfirmedTimestamp() const { return mUnconfirmedTimestamp; }
const BigInt& CTransactionDesc::getTax() const { return mTax; }
const std::vector<std::string>& CTransactionDesc::getLog() const { return mLog; }
eTxType::eTxType CTransactionDesc::getType() const { return mType; }



/**
 * @brief Get the color associated with the transaction result.
 * @return std::string The color code for the transaction status.
 */
eColor::eColor CTransactionDesc::getResultColor() const {
    return  CTools::getInstance()->transactionStatusColor(mResult);
}

/**
 * @brief Get the formatted string of the confirmed timestamp.
 * @return std::string The formatted confirmed timestamp.
 */
std::string CTransactionDesc::getConfirmedTimestampTxt() const {
    return CTools::getInstance()->timeToString(mConfirmedTimestamp);
}

/**
 * @brief Get the formatted string of the unconfirmed timestamp.
 * @return std::string The formatted unconfirmed timestamp.
 */
std::string CTransactionDesc::getUnconfirmedTimestampTxt() const {
    return CTools::getInstance()->timeToString(mUnconfirmedTimestamp);
}

/**
 * @brief Get the formatted string representation of the tax.
 * @return std::string The tax value in GNC format.
 */
std::string CTransactionDesc::getTaxTxt() const {
    return CTools::formatGNCValue(mTax);
}

std::vector<uint8_t> CTransactionDesc::getBlockID() const
{
    return mBlockID;
}

void CTransactionDesc::setBlockID(const std::vector<uint8_t>& id)
{
    mBlockID = id;
}

std::string CTransactionDesc::getValueTxt(uint64_t precision) const {
    return CTools::formatGNCValue(mValue);
}

uint64_t CTransactionDesc::getBytecodeVersion() const {
    std::shared_lock lock(mGuardian);
    return mBytecodeVersion;
}

void CTransactionDesc::setBytecodeVersion(uint64_t version) {
    std::unique_lock lock(mGuardian);
    mBytecodeVersion = version;
}
