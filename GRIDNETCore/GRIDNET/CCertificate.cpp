#include "CCertificate.h"
#include "CryptoFactory.h"
#include "BlockchainManager.h"
#include <fstream>
#include "CGlobalSecSettings.h"

CCertificate::CCertificate(uint64_t fileVersion, const std::vector<uint8_t>& fileFingerprint,
    const std::vector<uint8_t>& issuerPubKey, eDataAssetType::eDataAssetType dType, std::string description)
    : mFileVersion(fileVersion), mFileFingerprint(fileFingerprint),
    mIssuerPubKey(issuerPubKey), mDescription(description), mCertVersion(1), mAssetType(dType){
    // other initializations, e.g., set mCertVersion, mVersionStr
}

CCertificate::CCertificate() : mCertVersion(1), mAssetType(eDataAssetType::file)
{
}

std::vector<std::tuple<uint64_t, uint64_t, std::vector<uint8_t>>> CCertificate::getChunks()
{
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mChunks;
}
size_t  CCertificate::getChunksCount()
{
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mChunks.size();
}

bool CCertificate::generateChunks(const std::vector<uint8_t>& data, uint64_t maxNumberOfChunks, uint64_t minChunkSize) {
    
    //Pre-Flight - BEGIN
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    if (maxNumberOfChunks == 0) {
        return true; // No chunks are to be generated
    }
    mChunks.clear();
    //Pre-Flight - END

    std::shared_ptr<CCryptoFactory> cryptoFactory = CCryptoFactory::getInstance();
    //Local Variables - BEGIN
    uint64_t dataSize = data.size();
    uint64_t beginOffset = 0;
    uint64_t endOffset = 0;
    uint64_t chunkSize = 0;
    std::vector<uint8_t> chunkData;
    uint64_t actualNumberOfChunks = 0;
    std::vector<uint8_t> hmac;
    //Local Variables - END
    
    //Operational Logic - BEGIN
    mSignature.clear();
    // Edge case: Empty data vector
    if (dataSize == 0) {
        return false; // No data to chunk
    }

    // Edge case: minChunkSize is larger than the data
    if (minChunkSize > dataSize) {
        return false;
    }

     chunkSize = dataSize / maxNumberOfChunks;
    if (chunkSize < minChunkSize) {
        chunkSize = minChunkSize;
    }

     actualNumberOfChunks = dataSize / chunkSize;
    if (dataSize % chunkSize != 0) {
        actualNumberOfChunks += 1; // Account for the extra chunk with leftover bytes
    }

    for (uint64_t i = 0; i < actualNumberOfChunks; ++i) {
        beginOffset = i * chunkSize;
         endOffset = (i == actualNumberOfChunks - 1) ? dataSize : beginOffset + chunkSize;

        // Extract the chunk data using iterators
         chunkData = std::vector<uint8_t>(data.begin() + beginOffset, data.begin() + endOffset);

        hmac = cryptoFactory->genHMAC_sha256(mIssuerPubKey, chunkData);
        mChunks.push_back(std::make_tuple(beginOffset, endOffset, hmac));
    }
    //Operational Logic - END
    return true;
}

std::shared_ptr<CCertificate> CCertificate::restore(const std::vector<uint8_t>& id,  eBlockchainMode::eBlockchainMode mode) {
    if (id.size() < 12) {
        return nullptr; // Provided ID is too short
    }

    std::shared_ptr<CTools> tools = CTools::getInstance();
    // Construct filename
    std::vector<uint8_t> first12Bytes(id.begin(), id.begin() + 12);
    std::string fileName = tools->base58Encode(first12Bytes) + ".der";

    std::string mainDir = CBlockchainManager::getInstance(mode)->getSolidStorage()->getMainDataDir();
    std::string fullPath = mainDir + CGlobalSecSettings::getCertsDirPath() + "\\" + fileName;

    std::vector<uint8_t> fileData = tools->readFileEx(fullPath);
    if (fileData.empty()) {
        return nullptr; // Failed to read the file or file is empty
    }

    // Assuming you have a method to deserialize the certificate from data
    return CCertificate::instantiate(fileData);
}

eDataAssetType::eDataAssetType CCertificate::getAssetType()
{
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mAssetType;
}

void CCertificate::setAssetType(eDataAssetType::eDataAssetType aType)
{
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    mAssetType = aType;
}

bool CCertificate::store(eBlockchainMode::eBlockchainMode mode) {
    auto tools = CTools::getInstance();
    std::string mainDir = CBlockchainManager::getInstance(mode)->getSolidStorage()->getMainDataDir();
    std::string certDir = mainDir + CGlobalSecSettings::getCertsDirPath();

    // Ensure the directory exists
    if (!tools->createDirectoryRecursive(certDir)) {
        return false; // Failed to create directory
    }

    // Construct filename
    std::vector<uint8_t> first12Bytes(mAssetID.begin(), mAssetID.begin() + 12);
    std::string fileName =tools->base58Encode(first12Bytes) + ".der";

    std::string fullPath = certDir + "\\" + fileName;

    // Serialize the certificate into a format suitable for writing
    std::vector<uint8_t> serializedData = this->getPackedData(); // Assuming you have a method to serialize the certificate

    return tools->writeToFileEx(fullPath, serializedData, true); // Using 'wipe' to clear previous contents
}

bool CCertificate::generateChunks(std::string filePath, uint64_t maxNumberOfChunks, uint64_t minChunkSize) {


    //Pre-Flight - BEGIN
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    if (maxNumberOfChunks == 0) {
        return true; // No chunks are to be generated
    }
    mChunks.clear();
    //Pre-Flight - END
    
    //Local Variables - BEGIN
    std::shared_ptr<CCryptoFactory> cryptoFactory = CCryptoFactory::getInstance();
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    uint64_t beginOffset = 0;
    uint64_t endOffset = 0;
    uint64_t chunkSize = 0;
    std::vector<uint8_t> chunkData;
    uint64_t actualNumberOfChunks = 0;
    std::vector<uint8_t> hmac;
    //Local Variables - END

    //Operational Logic - BEGIN
    mSignature.clear();
    if (!file.is_open()) {
        return false;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize == 0) {
        return false; // Empty file
    }

    // If minChunkSize is larger than the file, return false or adjust behavior as needed
    if (minChunkSize > fileSize) {
        return false;
    }

    chunkSize = fileSize / maxNumberOfChunks;
    if (chunkSize < minChunkSize) {
        chunkSize = minChunkSize;
    }

    actualNumberOfChunks = fileSize / chunkSize;
    if (fileSize % chunkSize != 0) {
        actualNumberOfChunks += 1;
    }

    file.seekg(0, std::ios::beg);
   

    for (uint64_t i = 0; i < actualNumberOfChunks; ++i) {

         beginOffset = i * chunkSize;
         endOffset = (i == actualNumberOfChunks - 1) ? fileSize : beginOffset + chunkSize;
         chunkData = std::vector<uint8_t>(endOffset - beginOffset);
         file.read(reinterpret_cast<char*>(chunkData.data()), endOffset - beginOffset);

        // Check if file read was successful
        if (!file) {
            file.close();
            return false;
        }

        hmac = cryptoFactory->genHMAC_sha256(mIssuerPubKey, chunkData);
        mChunks.push_back(std::make_tuple(beginOffset, endOffset, hmac));
    }

    file.close();
    return true;
    //Operational Logic - END
}




bool CCertificate::verifySingleChunk(const std::vector<uint8_t>& data, size_t index) {
    if (index >= mChunks.size()) {
        return false;  // Index out of bounds
    }

    const auto& [beginOffset, endOffset, hmac] = mChunks[index];
    uint64_t chunkSize = endOffset - beginOffset;

    if (beginOffset + chunkSize > data.size()) {
        return false;
    }

    std::vector<uint8_t> chunkData(data.begin() + beginOffset, data.begin() + endOffset);

    std::shared_ptr<CCryptoFactory> cryptoFactory = CCryptoFactory::getInstance();
    return cryptoFactory->verifyHMAC_sha256(mIssuerPubKey, chunkData, hmac);
}

bool CCertificate::verifyChunks(const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < mChunks.size(); ++i) {
        if (!verifySingleChunk(data, i)) {
            return false;
        }
    }

    return true;
}

bool CCertificate::verifyChunks(const std::string& filePath) {

    std::shared_ptr<CCryptoFactory> cryptoFactory = CCryptoFactory::getInstance();

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& [beginOffset, endOffset, hmac] : mChunks) {
        uint64_t chunkSize = endOffset - beginOffset;
        file.seekg(beginOffset, std::ios::beg);

        std::vector<uint8_t> chunkData(chunkSize);
        file.read(reinterpret_cast<char*>(chunkData.data()), chunkSize);

        if (!cryptoFactory->verifyHMAC_sha256(mIssuerPubKey, chunkData, hmac)) {
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}



/**
 * @brief Serializes the CCertificate object into a byte vector representation.
 *
 * This method converts the current state of the CCertificate object into
 * a serialized byte vector using the Botan library's BER encoder.
 * The serialized format is suitable for transmission or storage.
 *
 * @return A byte vector containing the serialized representation of the CCertificate object.
 */
std::vector<uint8_t> CCertificate::getPackedData(bool includeSig) {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);  // Assuming you also have a mutex for CCertificate fields

    // Convert strings into byte vectors directly within the chained call
    std::vector<uint8_t> versionStrData(mVersionStr.begin(), mVersionStr.end());
    std::vector<uint8_t> descriptionData(mDescription.begin(), mDescription.end());

    try {
        // Begin the main sequence
        Botan::DER_Encoder encoder = Botan::DER_Encoder().start_cons(Botan::ASN1_Tag::SEQUENCE)
            .encode(static_cast<size_t>(mCertVersion))
            .start_cons(Botan::ASN1_Tag::SEQUENCE)
            .encode(mFileVersion)
            .encode(static_cast<size_t>(mAssetType))
            .encode(mAssetID, Botan::ASN1_Tag::OCTET_STRING)
            .encode(mFileFingerprint, Botan::ASN1_Tag::OCTET_STRING)
            .encode(mIssuerPubKey, Botan::ASN1_Tag::OCTET_STRING)
            .encode(versionStrData, Botan::ASN1_Tag::OCTET_STRING)
            .encode(descriptionData, Botan::ASN1_Tag::OCTET_STRING);

        if (includeSig) {
            encoder = encoder.encode(mSignature, Botan::ASN1_Tag::OCTET_STRING);
        }

        // Encode the chunks
        encoder = encoder.start_cons(Botan::ASN1_Tag::SEQUENCE);
        for (const auto& [beginOffset, endOffset, hmac] : mChunks) {
            encoder = encoder.start_cons(Botan::ASN1_Tag::SEQUENCE)
                .encode(beginOffset)
                .encode(endOffset)
                .encode(hmac, Botan::ASN1_Tag::OCTET_STRING)
                .end_cons(); // End of each chunk's sequence
        }
        encoder = encoder.end_cons();  // End of chunks sequence

        return encoder.end_cons().end_cons() // End of main sequence
            .get_contents_unlocked();
    }
    catch (...) {
        // Handle the error appropriately (e.g., logging). For now, just return an empty vector.
        return {};
    }
}



/**
 * @brief Instantiates a CCertificate object from a serialized byte representation.
 *
 * This static function deserializes the provided byte vector to create
 * a new CCertificate object. If the deserialization fails or the byte
 * representation is invalid, it returns a nullptr.
 *
 * @param bytes A byte vector representing a serialized CCertificate object.
 * @return A shared pointer to the deserialized CCertificate object or nullptr if failed.
 */
 /**
 * @brief Deserializes a CCertificate object from a byte vector representation.
 *
 * This method reconstructs a CCertificate object from its serialized byte vector form.
 * The Botan library's BER decoder is utilized for this purpose.
 *
 * @param bytes The byte vector containing the serialized representation of the CCertificate object.
 * @return A shared pointer to the newly constructed CCertificate object or nullptr if deserialization fails.
 */
std::shared_ptr<CCertificate> CCertificate::instantiate(const std::vector<uint8_t>& bytes) {
    std::shared_ptr<CCertificate> cert = std::make_shared<CCertificate>();

    try {
        if (bytes.size() == 0)
            return nullptr;

        std::vector<uint8_t> tempV;
        uint64_t beginOffset, endOffset;
        std::vector<uint8_t> hmac;
        size_t tempS = 0;
        // Begin decoding the main sequence
        Botan::BER_Decoder mainDecoder = Botan::BER_Decoder(bytes).start_cons(Botan::ASN1_Tag::SEQUENCE)
            .decode(reinterpret_cast<size_t&>(cert->mCertVersion));

        Botan::BER_Object obj;

        if (cert->mCertVersion == 1) {

            obj = mainDecoder.get_next_object();
            if (obj.type_tag != Botan::ASN1_Tag::SEQUENCE)
                return nullptr;
            Botan::BER_Decoder dec2 = Botan::BER_Decoder(obj.value);

            // Unpack the certificate details
            dec2.decode(cert->mFileVersion);
            dec2.decode(tempS);
            cert->mAssetType = static_cast<eDataAssetType::eDataAssetType>(tempS);
            dec2.decode(cert->mAssetID, Botan::ASN1_Tag::OCTET_STRING);
            dec2.decode(cert->mFileFingerprint, Botan::ASN1_Tag::OCTET_STRING)
                .decode(cert->mIssuerPubKey, Botan::ASN1_Tag::OCTET_STRING)
                .decode(tempV, Botan::ASN1_Tag::OCTET_STRING);
            cert->mVersionStr = std::string(tempV.begin(), tempV.end());
            dec2.decode(tempV, Botan::ASN1_Tag::OCTET_STRING);
            cert->mDescription = std::string(tempV.begin(), tempV.end());
            dec2.decode(cert->mSignature, Botan::ASN1_Tag::OCTET_STRING);

    
            // Decode the chunks
            obj = dec2.get_next_object();
            if (obj.type_tag == Botan::ASN1_Tag::SEQUENCE) {
                Botan::BER_Decoder chunkDecoder = Botan::BER_Decoder(obj.value);
                while (chunkDecoder.more_items()) {
                    
                    chunkDecoder.start_cons(Botan::ASN1_Tag::SEQUENCE)
                        .decode(beginOffset)
                        .decode(endOffset)
                        .decode(hmac, Botan::ASN1_Tag::OCTET_STRING)
                        .end_cons();

                    cert->mChunks.push_back({ beginOffset, endOffset, hmac });
                }
            }

            dec2.verify_end();
        }
        else
        {
            return nullptr;
        }

        // Ensure that all items in the sequence were consumed
        mainDecoder.verify_end();

        return cert;
    }
    catch (...) {
        return nullptr;  // Return nullptr on any exception during serialization
    }
}



/**
 * @brief Retrieves the version number of the file authenticated by the certificate.
 *
 * This function provides thread-safe access to the version number of the file
 * authenticated by this certificate.
 *
 * @return Version number of the authenticated file.
 */
uint64_t CCertificate::getVersionNumber() {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mFileVersion;
}

/**
 * @brief Retrieves the version string of the file authenticated by the certificate.
 *
 * This function provides thread-safe access to the version string of the file
 * authenticated by this certificate. Example format: "1.1.1."
 *
 * @return Version string of the authenticated file.
 */
std::string CCertificate::getVersionStr() {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mVersionStr;
}

/**
 * @brief Retrieves the description associated with the certificate.
 *
 * This function provides thread-safe access to the description or additional
 * information associated with this certificate.
 *
 * @return Description string.
 */
std::string CCertificate::getDescription() {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mDescription;
}

/**
 * @brief Retrieves the issuer's public key.
 *
 * This function provides thread-safe access to the public key of the issuer
 * that authenticated this certificate.
 *
 * @return Vector containing the issuer's public key.
 */
std::vector<uint8_t> CCertificate::getIssuerPubKey() {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mIssuerPubKey;
}

/**
 * @brief Retrieves the version of the certificate.
 *
 * This function provides thread-safe access to the version of this certificate.
 *
 * @return Version number of the certificate.
 */
uint64_t CCertificate::getCertVersion() {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mCertVersion;
}
void CCertificate::setAssetID(const std::vector<uint8_t>& assetID) {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    mSignature.clear();
    if (assetID.size() != 32) {
        std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
        const_cast<std::vector<uint8_t>& >(assetID) = cf->getSHA2_256Vec(assetID);
    }
    mAssetID = assetID;
}


bool CCertificate::storeChunk(uint64_t beginOffset, uint64_t endOffset, const std::vector<uint8_t>& receivedData) {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);

    // Sanity checks
    if (beginOffset >= endOffset) {
        return false;  // Invalid offsets provided
    }
    if ((endOffset - beginOffset) != receivedData.size()) {
        return false;  // Mismatch between provided offsets and actual data size
    }

    // Check if the chunk's offset range is declared in mChunks
    for (size_t i = 0; i < mChunks.size(); ++i) {
        auto [existingBegin, existingEnd, _] = mChunks[i];
        if (existingBegin == beginOffset && existingEnd == endOffset) {
            // Matching chunk found, store the received data
            if (mChunkData.size() <= i) {
                mChunkData.resize(i + 1);  // Ensure enough space to store this chunk
            }
            mChunkData[i] = receivedData;
            return true;
        }
    }

    return false;  // No matching chunk declaration found
}


bool CCertificate::verifyHotStorageChunk(size_t index) {
    if (index >= mChunks.size() || index >= mChunkData.size()) {
        return false;  // Index out of bounds or data for this chunk not received yet
    }

    const auto& [beginOffset, endOffset, hmac] = mChunks[index];
    const std::vector<uint8_t>& chunkData = mChunkData[index];

    std::shared_ptr<CCryptoFactory> cryptoFactory = CCryptoFactory::getInstance();
    return cryptoFactory->verifyHMAC_sha256(mIssuerPubKey, chunkData, hmac);
}


std::vector<uint8_t> CCertificate::getAssetID()  {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mAssetID;
}
/**
 * @brief Signs the certificate using the provided private key.
 *
 * The method serializes the CCertificate object, computes its fingerprint (excluding the signature),
 * and then signs this fingerprint using the given private key. The resulting signature is stored
 * within the certificate object.
 *
 * @param privKey The private key in Botan::secure_vector<uint8_t> format to use for signing.
 * @return True if the signing was successful, otherwise false.
 */
bool CCertificate::sign(const Botan::secure_vector<uint8_t>& privKey) {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);

    if (privKey.size() != 32)
        return false;
    // Compute the fingerprint of the certificate excluding the signature
    std::vector<uint8_t> certFingerprint = getCertFingerprint(false);

    // Use the crypto factory to sign the fingerprint
    std::shared_ptr<CCryptoFactory> cryptoFactory = CCryptoFactory::getInstance();
    mSignature = cryptoFactory->signData(certFingerprint, privKey);

    // Check if the signature generation was successful
    return !mSignature.empty();
}


/**
 * @brief Validates the certificate.
 *
 * The method performs the following validation checks:
 * 1. If a public key is provided, it must match the public key present in the certificate.
 * 2. If no public key is provided, the certificate must have a public key; otherwise, it's invalid.
 * 3. If a file fingerprint is provided, it must match the file fingerprint present in the certificate.
 * 4. If no file fingerprint is provided, the certificate must have a file fingerprint; otherwise, it's invalid.
 * 5. The signature present in the certificate must validate against the certificate's contents.
 *
 * @param fileFingerprint Optional file fingerprint to check against the one in the certificate.
 * @param pubKey Optional public key to check against the one in the certificate.
 * @return True if all validation checks pass, otherwise false.
 */

bool CCertificate::validate(const std::vector<uint8_t>& fileFingerprint, const std::vector<uint8_t>& pubKey) {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    std::shared_ptr<CCryptoFactory> cryptoFactory = CCryptoFactory::getInstance();

    if (mSignature.empty())
        return false;

    // Validate public key
    if (!pubKey.empty() && pubKey != mIssuerPubKey) {
        return false;
    }
    else if (pubKey.empty() && mIssuerPubKey.empty()) {
        return false;
    }

    // Validate file fingerprint
    if (!fileFingerprint.empty() && fileFingerprint != mFileFingerprint) {
        return false;
    }
    else if (fileFingerprint.empty() && mFileFingerprint.empty()) {
        return false;
    }

    // Validate signature
    std::vector<uint8_t> certFingerprint = getCertFingerprint(false);//do not take the internal signature into account
   
    bool isSignatureValid = cryptoFactory->verifySignature(mSignature, certFingerprint, mIssuerPubKey);

    return isSignatureValid;
}



/**
 * @brief Computes and returns the SHA-256 fingerprint of the certificate.
 *
 * This method serializes the CCertificate object and then calculates its SHA-256 hash.
 * The signature can optionally be included or excluded based on the argument.
 *
 * @param includeSignature Indicates whether the signature should be included when computing the hash.
 * @return The SHA-256 fingerprint of the certificate in std::vector<uint8_t> format.
 */
std::vector<uint8_t> CCertificate::getCertFingerprint(bool includeSignature) {
    // Serialize the certificate with or without the signature
    std::vector<uint8_t> serializedCert = getPackedData(includeSignature);

    // Use the crypto factory to compute the hash
    std::shared_ptr<CCryptoFactory> cryptoFactory = CCryptoFactory::getInstance();
    return cryptoFactory->getSHA2_256Vec(serializedCert);
}


/**
 * @brief Retrieves the fingerprint of the file authenticated by the certificate.
 *
 * This function provides thread-safe access to the fingerprint of the file
 * authenticated by this certificate. The fingerprint acts as a unique identifier
 * for the file.
 *
 * @return Vector containing the fingerprint of the authenticated file.
 */
std::vector<uint8_t> CCertificate::getFileFingerprint() {
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mFileFingerprint;
}

void CCertificate::setVersionStr(const std::string& version)
{
    std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    mSignature.clear();
    mVersionStr = version;
}


