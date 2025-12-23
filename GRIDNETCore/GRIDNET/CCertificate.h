#pragma once
#include <botan_all.h>
#include <vector>
#include <memory>
#include <string>

class CCryptoFactory;

class CCertificate {
private:
    eDataAssetType::eDataAssetType mAssetType;
    uint64_t mFileVersion;
    uint64_t mCertVersion;
    std::string mVersionStr;
    std::vector<uint8_t> mFileFingerprint;
    std::vector<uint8_t> mIssuerPubKey;
    std::string mDescription;
    std::recursive_mutex mFieldsGuardian;
    std::vector<uint8_t> mSignature;
    std::vector<uint8_t> mAssetID; // 32 bytes vector to store Asset ID
    std::vector<std::tuple<uint64_t, uint64_t, std::vector<uint8_t>>> mChunks;
    // New member to hold actual data chunks. Each entry corresponds to a chunk in mChunks.
    std::vector<std::vector<uint8_t>> mChunkData;

public:
    eDataAssetType::eDataAssetType getAssetType();
    void setAssetType(eDataAssetType::eDataAssetType aType);
    bool store(eBlockchainMode::eBlockchainMode mode = eBlockchainMode::TestNet);
    std::shared_ptr<CCertificate> restore(const std::vector<uint8_t>& id, eBlockchainMode::eBlockchainMode mode = eBlockchainMode::TestNet);
    CCertificate(uint64_t fileVersion,const std::vector<uint8_t>& fileFingerprint,
        const std::vector<uint8_t>& issuerPubKey,eDataAssetType::eDataAssetType dType, std::string description = "");
    CCertificate();
    
    std::vector<std::tuple<uint64_t, uint64_t, std::vector<uint8_t>>> getChunks();
    size_t getChunksCount();
    /**
 * @brief Verifies the integrity of chunks for a given file.
 *
 * This method reads each chunk of data from the file and checks the HMAC
 * of that chunk against the stored HMAC for that chunk.
 *
 * @param filePath The path to the file to be verified.
 * @return true If all the chunks of the file match their respective HMACs.
 * @return false If any chunk does not match its HMAC or if the file can't be read.
 */


    bool verifyChunks(const std::string& filePath);

    bool verifySingleChunk(const std::vector<uint8_t>& data, size_t index);
    /**
     * @brief Verifies the integrity of chunks for a given data vector.
     *
     * This method checks the HMAC of each chunk in the provided data against the stored HMAC.
     *
     * @param data The byte vector containing the data to be verified.
     * @return true If all the chunks of the data match their respective HMACs.
     * @return false If any chunk does not match its HMAC.
     */
    bool verifyChunks(const std::vector<uint8_t>& data);

    /**
 * @brief Generates HMAC chunks for a given file.
 *
 * This function divides a file into HMAC chunks and saves them internally.
 * The number of chunks is defined by the maximum number of chunks provided, but the actual chunk size might
 * be adjusted if it's smaller than the minimum chunk size.
 *
 * @param filePath         Path to the file that will be chunked.
 * @param maxNumberOfChunks Maximum number of desired chunks. If set to 0, no chunks will be generated.
 * @param minChunkSize     Minimum desired chunk size in bytes. Default is 10 MB.
 * @return True if chunking was successful, false otherwise (e.g., file doesn't exist or chunk size issues).
 */
    bool generateChunks(std::string filePath, uint64_t maxNumberOfChunks=10, uint64_t minChunkSize = 10 * 1024 * 1024);

    /**
     * @brief Generates HMAC chunks for a data vector.
     *
     * This function divides a vector into HMAC chunks and saves them internally.
     * The number of chunks is defined by the maximum number of chunks provided, but the actual chunk size might
     * be adjusted if it's smaller than the minimum chunk size.
     *
     * @param data              Vector containing the data to be chunked.
     * @param maxNumberOfChunks Maximum number of desired chunks. If set to 0, no chunks will be generated.
     * @param minChunkSize     Minimum desired chunk size in bytes. Default is 10 MB.
     * @return True if chunking was successful, false otherwise (e.g., chunk size issues).
     */
    bool generateChunks(const std::vector<uint8_t>& data, uint64_t maxNumberOfChunks=10, uint64_t minChunkSize = 10 * 1024 * 1024);

    /**
     * @brief Setter method for the Asset ID.
     *
     * This method allows you to set the Asset ID of the certificate.
     *
     * @param assetID A byte vector representing the Asset ID.
     */
    void setAssetID(const std::vector<uint8_t>& assetID);
    /**
 * @brief Stores received chunk data based on its beginning and ending offsets.
 *
 * This method checks if a chunk with the specified offsets is declared in the certificate.
 * If a matching chunk is found, the received data is stored accordingly.
 *
 * @param beginOffset The beginning offset of the received data.
 * @param endOffset The ending offset of the received data.
 * @param receivedData The actual chunk data that has been received.
 * @return True if the data was successfully stored; false otherwise.
 */
    bool storeChunk(uint64_t beginOffset, uint64_t endOffset, const std::vector<uint8_t>& receivedData);

    /**
 * @brief Verifies the integrity of a chunk stored in the hot storage.
 *
 * The function checks the integrity of a specific chunk by comparing the expected HMAC value
 * (stored in the certificate) against the HMAC value of the actual received data.
 *
 * @param index The index of the chunk to verify.
 * @return True if the chunk's HMAC matches and integrity is verified; false otherwise.
 */
    bool verifyHotStorageChunk(size_t index);

    /**
     * @brief Getter method for the Asset ID.
     *
     * This method retrieves the Asset ID of the certificate.
     *
     * @return std::vector<uint8_t> The Asset ID of the certificate.
     */
    std::vector<uint8_t> getAssetID();

    /**
     * @brief Factory method to instantiate a certificate object from its serialized form.
     *
     * This method tries to deserialize a byte vector into a `CCertificate` object.
     *
     * @param bytes A byte vector containing the serialized certificate.
     * @return std::shared_ptr<CCertificate> A smart pointer to the deserialized certificate object, or nullptr if deserialization fails.
     */
    static std::shared_ptr<CCertificate> instantiate(const std::vector<uint8_t>& bytes);


    //Getters - BEGIN
    uint64_t getVersionNumber();
    std::string getVersionStr();
    std::string getDescription();
    std::vector<uint8_t> getIssuerPubKey();
    uint64_t getCertVersion();
    std::vector<uint8_t> getFileFingerprint();
    //Getters - END

    //setters - BEGIN
    void setVersionStr(const std::string& version);
    //setters - END
    
    // Actors - BEGIN

/**
 * @brief Retrieves the fingerprint of the certificate.
 *
 * This method computes and returns the fingerprint of the certificate based on its contents.
 * The fingerprint is typically used for verification and unique identification purposes.
 *
 * @param includeSignature If set to true, the fingerprint will include the certificate's signature. Default is false.
 * @return std::vector<uint8_t> The fingerprint of the certificate as a byte vector.
 */
    std::vector<uint8_t> getCertFingerprint(bool includeSignature = false);

    /**
     * @brief Serializes the certificate into a byte vector.
     *
     * This method prepares the certificate data for transmission or storage by converting it into a serialized form.
     *
     * @param includeSig If set to true, the serialized data will include the certificate's signature. Default is true.
     * @return std::vector<uint8_t> The serialized form of the certificate as a byte vector.
     */
    std::vector<uint8_t> getPackedData(bool includeSig = true);

    /**
     * @brief Signs the certificate using a private key.
     *
     * This method computes a signature for the certificate using the provided private key.
     *
     * @param privKey A secure vector containing the private key used for signing.
     * @return true If the certificate was successfully signed.
     * @return false If there was an error in signing.
     */
    bool sign(const Botan::secure_vector<uint8_t>& privKey);

    /**
     * @brief Validates the certificate.
     *
     * This method verifies the integrity and authenticity of the certificate.
     * It optionally checks the certificate's fingerprint against a provided one and verifies the signature using a provided public key.
     *
     * @param fileFingerprint An optional parameter to specify a fingerprint to check against. Default is an empty vector.
     * @param pubKey An optional parameter to specify a public key for signature verification. Default is an empty vector.
     * @return true If the certificate is valid.
     * @return false If the certificate is invalid or if validation fails for any reason.
     */
    bool validate(const std::vector<uint8_t>& fileFingerprint = std::vector<uint8_t>(), const std::vector<uint8_t>& pubKey = std::vector<uint8_t>());

    // Actors - END

};

// Implementation

