#pragma once
#include "mainTests.h"
#include "transaction.h"
#include "Block.h"

#include "TrieDB.h"
#include "limits"
#include "CWorkC.h"
#include "CWorkHybrid.h"
#include "CWorkUltimium.h"
#include "cmemparam.h"
#include "DataConcatenator.h"
#include "cvalueparam.h"
#include <thread>
#include "Settings.h"
#include "keyChain.h"
#include "Verifiable.h"
#include "ScriptEngine.h"
#include "NetMsg.h"
#include "keyChain.h"
#include "CStateDomainManager.h"
#include "Receipt.h"
#include <algorithm>
#include "GridScriptParser.h"
#include <cmath>
#include "GenesisRewards.h"
#include "VMMetaGenerator.h"
#include "CVMMetaParser.h"
#include <limits>
#include "VMMetaEntry.h"
#include "StatusBarHub.h"
#include "VMMetaSection.h"
#include <sstream>
#include <fstream>
#include <filesystem>
#include "TokenPool.h"
#include "TransmissionToken.h"
#include "SDPEntity.h"
#include "webRTCSwarm.h"
#include "webRTCSwarmMember.h"
#include "FileMetaData.h"
#include "minecraftPDU.h"
#include "ChatMsg.h"
#include "Poll.h"
#include "CCertificate.h"
#include "ThreadPool.h"
#include "ExclusiveWorkerMutex.hpp"

/**
 * @brief Comprehensive validation test for VarInt encoding/decoding functionality with detailed logging
 */
 /**
  * @brief Comprehensive validation test for VarInt encoding/decoding functionality with detailed logging
  */
bool CTests::validateVarIntImplementation() {
	try {
		std::shared_ptr<CTools> tools = CTools::getInstance();
		bool allTestsPassed = true;

		size_t totalTests = 0;
		size_t passedTests = 0;

		// Enhanced test recording with logging
		auto recordTest = [&](bool result, const char* testName) {
			totalTests++;
			if (result) {
				passedTests++;
				printf("[PASS] %s\n", testName);
			}
			else {
				printf("[FAIL] %s\n", testName);
			}
			allTestsPassed &= result;
			return result;
			};

		// Stage 1: Basic VarInt encoding/decoding tests
		{
			std::vector<uint8_t> encoded;
			uint64_t testValues[] = {
				0, 1, 127, 128, 255, 256,
				16383, 16384,
				2097151, 2097152,
				268435455, 268435456,
				UINT64_MAX
			};

			for (uint64_t value : testValues) {
				encoded.clear();
				tools->EncodeVarInt(value, encoded);

				size_t index = 0;
				uint64_t decoded;
				bool decodeResult = tools->DecodeVarInt(encoded, index, decoded);

				char testName[100];
				snprintf(testName, sizeof(testName),
					"Basic VarInt encoding/decoding test - value: %llu",
					static_cast<unsigned long long>(value));

				if (!decodeResult) {
					printf("    Failed to decode value: %llu\n",
						static_cast<unsigned long long>(value));
				}
				else if (value != decoded) {
					printf("    Value mismatch: expected %llu, got %llu\n",
						static_cast<unsigned long long>(value),
						static_cast<unsigned long long>(decoded));
				}
				else if (index != encoded.size()) {
					printf("    Index mismatch: expected %zu, got %zu\n",
						encoded.size(), index);
				}

				recordTest(decodeResult && value == decoded &&
					index == encoded.size(), testName);
			}
		}

		// Stage 2: Vector encoding/decoding tests
		{
			printf("\nStarting vector encoding/decoding tests\n");
			std::vector<std::vector<uint8_t>> testVectors;

			testVectors.push_back(std::vector<uint8_t>());
			testVectors.push_back(std::vector<uint8_t>{1, 2, 3});
			testVectors.push_back(std::vector<uint8_t>(127, 0xFF));
			testVectors.push_back(std::vector<uint8_t>(128, 0xAA));
			testVectors.push_back(std::vector<uint8_t>(256, 0x55));
			testVectors.push_back(std::vector<uint8_t>(16384, 0x77));

			// Single-threaded test
			std::vector<uint8_t> encoded =
				tools->VarLengthEncodeVector(testVectors, 1, false);
			std::vector<std::vector<uint8_t>> decoded;
			bool decodeResult = tools->VarLengthDecodeVector(encoded, decoded);

			if (!decodeResult) {
				printf("    Single-threaded decode failed\n");
			}
			else if (testVectors != decoded) {
				printf("    Single-threaded vectors mismatch\n");
				printf("    Original size: %zu, Decoded size: %zu\n",
					testVectors.size(), decoded.size());
			}

			recordTest(decodeResult && testVectors == decoded,
				"Single-threaded vector encoding/decoding");

			// Multi-threaded tests
			std::vector<std::vector<uint8_t>> largeTestSet;
			for (int i = 0; i < 10000; i++) {
				largeTestSet.push_back(std::vector<uint8_t>(
					rand() % 1000 + 1, static_cast<uint8_t>(i & 0xFF)));
			}

			for (unsigned int threads : {2, 4, 8, 16}) {
				char testName[100];
				snprintf(testName, sizeof(testName),
					"Multi-threaded test with %u threads", threads);

				std::vector<uint8_t> threadEncoded =
					tools->VarLengthEncodeVector(largeTestSet, threads, false);
				std::vector<std::vector<uint8_t>> threadDecoded;
				bool threadDecodeResult =
					tools->VarLengthDecodeVector(threadEncoded, threadDecoded);

				if (!threadDecodeResult) {
					printf("    Failed to decode with %u threads\n", threads);
				}
				else if (largeTestSet != threadDecoded) {
					printf("    Vector mismatch with %u threads\n", threads);
					printf("    Original size: %zu, Decoded size: %zu\n",
						largeTestSet.size(), threadDecoded.size());
				}

				recordTest(threadDecodeResult && largeTestSet == threadDecoded,
					testName);
			}
		}

		// Stage 3: Edge case tests
		{
			printf("\nStarting edge case tests\n");

			// Empty input test
			std::vector<std::vector<uint8_t>> emptyInput;
			std::vector<uint8_t> emptyEncoded =
				tools->VarLengthEncodeVector(emptyInput, 1, false);
			recordTest(emptyEncoded.empty(), "Empty input vector test");

			// Invalid VarInt test
			std::vector<uint8_t> invalidVarInt{ 0x80, 0x80, 0x80, 0x80,
				0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
			size_t index = 0;
			uint64_t value;
			recordTest(!tools->DecodeVarInt(invalidVarInt, index, value),
				"Invalid VarInt detection");

			// Truncated input test
			std::vector<uint8_t> truncated{ 0x80 };
			index = 0;
			recordTest(!tools->DecodeVarInt(truncated, index, value),
				"Truncated input detection");

			// Overflow test
			std::vector<uint8_t> overflowTest{ 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F };
			index = 0;
			recordTest(!tools->DecodeVarInt(overflowTest, index, value),
				"Overflow condition detection");

			// Invalid length test
			std::vector<uint8_t> invalidLength;
			tools->EncodeVarInt(UINT64_MAX, invalidLength);
			std::vector<std::vector<uint8_t>> decodedInvalid;
			recordTest(!tools->VarLengthDecodeVector(invalidLength, decodedInvalid),
				"Invalid length detection");
		}

		// Stage 4: Stress test
		{
			printf("\nStarting stress test\n");
			std::vector<std::vector<uint8_t>> stressTest;
			const size_t numVectors = 100000;
			stressTest.reserve(numVectors);

			for (size_t i = 0; i < numVectors; i++) {
				size_t size = (i % 1000) + 1;
				stressTest.push_back(std::vector<uint8_t>(
					size, static_cast<uint8_t>(i & 0xFF)));
			}

			std::vector<uint8_t> stressEncoded =
				tools->VarLengthEncodeVector(stressTest, 8, false);
			std::vector<std::vector<uint8_t>> stressDecoded;
			bool stressResult =
				tools->VarLengthDecodeVector(stressEncoded, stressDecoded);

			if (!stressResult) {
				printf("    Stress test decode failed\n");
			}
			else if (stressTest != stressDecoded) {
				printf("    Stress test vector mismatch\n");
				printf("    Original size: %zu, Decoded size: %zu\n",
					stressTest.size(), stressDecoded.size());
			}

			recordTest(stressResult && stressTest == stressDecoded,
				"Large dataset stress test");
		}

		printf("\nVarInt Implementation Test Results: %zu/%zu tests passed\n",
			passedTests, totalTests);
		return allTestsPassed;

	}
	catch (const std::exception& e) {
		printf("Exception during VarInt tests: %s\n", e.what());
		return false;
	}
	catch (...) {
		printf("Unknown exception during VarInt tests\n");
		return false;
	}
}
bool CTests::testFileCertificates()
{
	mTools->writeLine("Commencing with file certificate tests..");
	bool success = true;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::vector<uint8_t> mTestContents = tools->genRandomVector(100000000);
	std::string pathSelf = tools->getExecutablePath();
	Botan::secure_vector<uint8_t> privKey;
	std::vector<uint8_t> pubKey;
	std::vector<uint8_t> contentID = tools->genRandomVector(32);
	cf->genKeyPair(privKey, pubKey);

	std::string versionString = "1.1.1";
	std::string certDescription = "Test Certificate";
	// 1. Create a certificate
	std::vector<uint8_t> fileFingerprint = CTools::getInstance()->genRandomVector(32);
	std::vector<uint8_t> issuerPubKey = pubKey;
	CCertificate cert(1, fileFingerprint, issuerPubKey, eDataAssetType::file, certDescription);
	cert.setVersionStr(versionString);
	cert.setAssetID(contentID);
	cert.sign(privKey);


	//Chunks' support

	if (!cert.generateChunks(pathSelf))
	{
		return false;
	}

	if (!cert.verifyChunks(pathSelf))
	{
		return false;
	}


	// 1. Sign the certificate and then validate the signature
	if (cert.sign(privKey) && !cert.validate()) {
		success = false;
		std::cerr << "Error: Certificate signature validation failed!" << std::endl;
	}

	// 2. now try with direct arbitrary data buffer
	cert.generateChunks(mTestContents);

	if (!cert.verifyChunks(mTestContents))
	{
		return false;
	}

	// 3. Sign the certificate and then validate the signature
	if (cert.sign(privKey) && !cert.validate()) {
		success = false;
		std::cerr << "Error: Certificate signature validation failed!" << std::endl;
	}


	// 4. Serialize (pack) the certificate
	std::vector<uint8_t> packedData = cert.getPackedData();




	// 5. Deserialize (instantiate) the certificate
	auto deserializedCert = CCertificate::instantiate(packedData);
	if (!deserializedCert) {
		success = false;
		// Log or print an error message
		std::cerr << "Error: Certificate de-serialization failed!" << std::endl;
	}

	if (!success)
	{
		return false;
	}

	// 6. Validate the certificate
	if (!cert.validate()) {
		success = false;
		std::cerr << "Error: Certificate validation failed!" << std::endl;
	}
	if (!success)
	{
		return false;
	}
	// 7. Check the various getters
	if (cert.getVersionNumber() != 1 ||
		!tools->compareByteVectors(cert.getVersionStr(), versionString) ||
		!tools->compareByteVectors(cert.getDescription(), certDescription) ||
		!tools->compareByteVectors(cert.getIssuerPubKey(), issuerPubKey) ||
		cert.getCertVersion() != 1 ||
		!tools->compareByteVectors(cert.getFileFingerprint(), fileFingerprint))
	{
		success = false;
		std::cerr << "Error: Getters returned unexpected values!" << std::endl;
	}
	if (!success)
	{
		return false;
	}

	//storage

	if (!cert.store())
	{
		return false;
	}
	std::shared_ptr< CCertificate> restored = cert.restore(contentID);

	if (!restored)
	{
		return false;
	}

	// Negative tests
	// 1. Try to instantiate with an empty buffer
	auto emptyCert = CCertificate::instantiate({});
	if (emptyCert) {
		success = false;
		std::cerr << "Error: Certificate instantiation should fail with empty buffer!" << std::endl;
	}
	if (!success)
	{
		return false;
	}
	// 2. Try to validate a certificate with an incorrect fileFingerprint
	std::vector<uint8_t> wrongFingerprint = { 0xAA, 0xBB, 0xCC };
	if (cert.validate(wrongFingerprint)) {
		success = false;
		std::cerr << "Error: Certificate validation should fail with wrong fingerprint!" << std::endl;
	}
	if (!success)
	{
		return false;
	}
	// 3. Try to validate a certificate with an incorrect publicKey
	std::vector<uint8_t> wrongPubKey = { 0xDD, 0xEE, 0xFF };
	if (cert.validate(fileFingerprint, wrongPubKey)) {
		success = false;
		std::cerr << "Error: Certificate validation should fail with wrong public key!" << std::endl;
	}
	if (!success)
	{
		return false;
	}

	// 4. Destroy authenticated data
	uint64_t destroyAtIndex = tools->genRandomNumber(0, mTestContents.size() - 1);
	mTestContents[destroyAtIndex] = ~mTestContents[destroyAtIndex];
	if (cert.verifyChunks(mTestContents))
	{
		return false;
	}

	// 4. Test a malformed serialized certificate
	//packedData.push_back(0x01);  // Append some random byte
	//auto malformedCert = CCertificate::instantiate(packedData);
	//if (malformedCert) {
	//	success = false;
	//	std::cerr << "Error: Certificate de-serialization should fail with malformed data!" << std::endl;
	//}
	//if (!success)
	//{
	//	return false;
	//}
	// 5. Try to sign with an empty privateKey
	Botan::secure_vector<uint8_t> emptyPrivKey;
	if (cert.sign(emptyPrivKey)) {
		success = false;
		std::cerr << "Error: Certificate signing should fail with an empty private key!" << std::endl;
	}

	if (!success)
	{
		return false;
	}
	mTools->writeLine("File certificate tests succeeded..");
	return success;
}

void CTests::testThreadPoolInitialization() {
	const size_t minThreads = 10;
	ThreadPool pool(minThreads, 50, 5);
	assertEqual(pool.getActiveThreadCount(), minThreads, "Initialization");
}

void CTests::testThreadPoolExpansion() {
	const size_t minThreads = 10;
	const size_t maxThreads = 20;
	const size_t tasksCount = 15; // More than minThreads to force expansion
	ThreadPool pool(minThreads, maxThreads, 5);

	for (size_t i = 0; i < tasksCount; ++i) {
		pool.enqueue([]() {
			std::this_thread::sleep_for(std::chrono::seconds(10));
			});
	}

	std::this_thread::sleep_for(std::chrono::seconds(2)); // Brief pause to allow for expansion
	size_t activeCount = pool.getActiveThreadCount();
	bool condition = activeCount > minThreads && pool.getActiveThreadCount() <= maxThreads;
	assertTrue(condition, "ThreadExpansion");
}

void CTests::testMaximumTheadPoolLimit() {
	const size_t minThreads = 10;
	const size_t maxThreads = 20;
	const size_t tasksCount = 30; // More than maxThreads to test limit
	ThreadPool pool(minThreads, maxThreads, 5);

	for (size_t i = 0; i < tasksCount; ++i) {
		pool.enqueue([]() {
			std::this_thread::sleep_for(std::chrono::seconds(10));
			});
	}

	std::this_thread::sleep_for(std::chrono::seconds(2)); // Brief pause to allow for potential exceedance

	assertEqual(pool.getActiveThreadCount(), maxThreads, "MaximumLimit");
}

void CTests::validateNonceInsertion() {
	// Test nonce - choosing 0x12345678 as it has distinct bytes for clear visualization
	uint32_t testNonce = 0x12345678;
	printf("Testing with nonce: 0x%08x (in memory: 78 56 34 12)\n", testNonce);

	// Create test data buffer - sequential bytes for easy tracking
	std::vector<uint8_t> data(80, 0);
	for (int i = 0; i < 80; i++) {
		data[i] = i;
	}

	// CPU PATH - Original reference implementation
	// Loads 64 bits from offset 24 in little-endian (x86 native) format
	sph_u64 cpuM9 = *((sph_u64*)&((data)[24]));
	// Clear lower 32 bits to make room for nonce
	cpuM9 &= 0xFFFFFFFF00000000;
	// XOR with nonce value (nonce is in little-endian)
	cpuM9 ^= testNonce;

	printf("\nGPU operations simulation (matching OpenCL kernel):\n");

	// Step 1: Simulate GPU's vload8 operation
	// This loads 64 bits from offset 24 in same format as CPU
	sph_u64 gpuM3 = *((sph_u64*)&((data)[24]));
	printf("1. After vload8:           %016llx (same as CPU load)\n", gpuM3);

	// Step 2: Clear lower 32 bits for nonce insertion
	gpuM3 &= 0xFFFFFFFF00000000;
	printf("2. After clear:            %016llx (upper 32 bits preserved)\n", gpuM3);

	// Step 3: Insert nonce value using XOR
	// Nonce is in little-endian format, same as CPU
	gpuM3 ^= testNonce;
	printf("3. After nonce insertion:  %016llx (nonce in lower 32 bits)\n", gpuM3);

	// Step 4: Convert to big-endian for hash chain
	// Hash functions expect data in big-endian format
	sph_u64 gpuM3_swapped = ((gpuM3 & 0xFF00000000000000ULL) >> 56) |
		((gpuM3 & 0x00FF000000000000ULL) >> 40) |
		((gpuM3 & 0x0000FF0000000000ULL) >> 24) |
		((gpuM3 & 0x000000FF00000000ULL) >> 8) |
		((gpuM3 & 0x00000000FF000000ULL) << 8) |
		((gpuM3 & 0x0000000000FF0000ULL) << 24) |
		((gpuM3 & 0x000000000000FF00ULL) << 40) |
		((gpuM3 & 0x00000000000000FFULL) << 56);
	printf("4. After SWAP8:            %016llx (ready for hash chain)\n", gpuM3_swapped);

	printf("\nDetailed comparison:\n");
	printf("Original data at offset 24: ");
	for (int i = 0; i < 8; i++) {
		printf("%02x ", data[24 + i]);
	}
	printf("(sequential test data)\n");

	printf("Nonce in hex:               %08x (to be inserted)\n", testNonce);
	printf("CPU final (before chain):   %016llx (little-endian)\n", cpuM9);
	printf("GPU final (before chain):   %016llx (little-endian)\n", gpuM3);
	printf("GPU final (after SWAP8):    %016llx (big-endian for hash)\n", gpuM3_swapped);

	// Detailed byte-by-byte comparison to verify ordering
	printf("\nByte-by-byte analysis:\n");
	printf("CPU bytes:              ");
	for (int i = 0; i < 8; i++) {
		printf("%02x ", (uint8_t)(cpuM9 >> (i * 8)));
	}
	printf("(little-endian, before hash)\n");

	printf("GPU bytes before SWAP8: ");
	for (int i = 0; i < 8; i++) {
		printf("%02x ", (uint8_t)(gpuM3 >> (i * 8)));
	}
	printf("(little-endian, should match CPU)\n");

	printf("GPU bytes after SWAP8:  ");
	for (int i = 0; i < 8; i++) {
		printf("%02x ", (uint8_t)(gpuM3_swapped >> (i * 8)));
	}
	printf("(big-endian, ready for hash chain)\n");

	// Validate results
	if (cpuM9 != gpuM3) {
		printf("\nERROR: CPU and GPU values don't match before SWAP8!\n");
	}
	else {
		printf("\nValidation: CPU and GPU produce identical values before SWAP8\n");
		printf("Hash chain will receive correct big-endian data after SWAP8\n");
	}
}
void CTests::testThreadPoolShrinkBackToMin() {
	const size_t minThreads = 10;
	const size_t maxThreads = 20;
	ThreadPool pool(minThreads, maxThreads, 5);

	// Enqueue enough tasks to potentially expand the pool
	for (size_t i = 0; i < maxThreads * 2; ++i) {
		pool.enqueue([]() {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			});
	}

	// Wait for all tasks to complete
	pool.waitAll();

	// Additional wait time might be necessary to allow the thread pool to shrink
	std::this_thread::sleep_for(std::chrono::seconds(10)); // Assuming pool adjusts threads periodically

	assertEqual(pool.getActiveThreadCount(), minThreads, "ShrinkBackToMin");
}

// Helper methods for assertions
void CTests::assertEqual(size_t actual, size_t expected, const std::string& testName) {
	if (actual == expected) {
		std::cout << testName << " passed." << std::endl;
	}
	else {
		std::cout << testName << " failed: Expected " << expected << ", but got " << actual << "." << std::endl;
	}
}

void CTests::assertTrue(bool condition, const std::string& testName) {
	if (condition) {
		std::cout << testName << " passed." << std::endl;
	}
	else {
		std::cout << testName << " failed." << std::endl;
	}
}

bool CTests::testBase64()
{
	std::vector<uint8_t> vec2;
	std::vector<uint8_t> vec = mTools->genRandomVector(500000);
	std::string s = mTools->base64CheckEncode(vec);

	bool result = mTools->base64CheckDecode(mTools->stringToBytes(s), vec2);
	if (!result)
		return false;

	result = mTools->base64CheckDecode(mTools->stringToBytes(s), vec2);
	if (!mTools->compareByteVectors(vec, vec2))
		return false;

	//negative tests
	std::vector<uint8_t> rB = mTools->stringToBytes(s);
	if (!mTools->introduceRandomChange(rB))
		return false;

	s = mTools->bytesToString(rB);
	result = mTools->base64CheckDecode(mTools->stringToBytes(s), vec2);
	if (result)
		return false;

	return true;
}

bool CTests::testBase58()
{
	mTools->writeLine("Testing 10 bytes of data..");
	std::vector<uint8_t> vec = mTools->genRandomVector(10);
	std::string encoded = mTools->base58CheckEncode(vec);
	std::string encodedSatoshi = mTools->base58CheckEncode(vec, false, nullptr, true);
	std::vector<uint8_t> decoded, decodedSatoshi;
	bool result = false;
	result = mTools->base58CheckDecode(encoded, decoded);
	result = mTools->base58CheckDecode(encoded, decoded, true);

	bool theSameData = false;
	if (mTools->compareByteVectors(decoded, decodedSatoshi))
	{
		theSameData = true;
	}

	//5KBytes
	mTools->writeLine("Testing 5kB of data..");
	vec = mTools->genRandomVector(5000);
	runBase58Test(encoded, vec, encodedSatoshi, result, decoded, decodedSatoshi, theSameData);
	//50KBytes
	mTools->writeLine("Testing 50kB of data..");
	vec = mTools->genRandomVector(50000);
	runBase58Test(encoded, vec, encodedSatoshi, result, decoded, decodedSatoshi, theSameData);

	if (!theSameData)
		return false;
	return true;
}
void CTests::runBase58Test(std::string& encoded, std::vector<uint8_t>& vec, std::string& encodedSatoshi, bool& result, std::vector<uint8_t>& decoded, std::vector<uint8_t>& decodedSatoshi, bool& theSameData)
{
	uint64_t timeStart = mTools->getTime();
	encoded = mTools->base58CheckEncode(vec);
	uint64_t timeEnd = mTools->getTime();

	mTools->writeLine("GRIDNET's base58 encoding took " + std::to_string(timeEnd - timeStart) + " seconds");
	timeStart = mTools->getTime();
	encodedSatoshi = mTools->base58CheckEncode(vec, true, nullptr, true);
	timeEnd = mTools->getTime();
	mTools->writeLine("Bitcoin's base58 encoding took " + std::to_string(timeEnd - timeStart) + " seconds");

	timeStart = mTools->getTime();
	result = mTools->base58CheckDecode(encoded, decoded);
	timeEnd = mTools->getTime();
	mTools->writeLine("GRIDNET's base58 decoding took " + std::to_string(timeEnd - timeStart) + " seconds");
	timeStart = mTools->getTime();
	result = mTools->base58CheckDecode(encodedSatoshi, decodedSatoshi, true);
	timeEnd = mTools->getTime();
	mTools->writeLine("Bitcoin's base58 decoding took " + std::to_string(timeEnd - timeStart) + " seconds");
	theSameData = false;
	if (mTools->compareByteVectors(decoded, decodedSatoshi))
	{
		theSameData = true;
	}
}
CTests::CTests(std::shared_ptr<CBlockchainManager> manager, std::shared_ptr<CTools> tools)
{
	std::cerr << "[DEBUG CTests] Constructor starting..." << std::endl;
	std::cerr.flush();

	eBlockchainMode::eBlockchainMode blockchainMode = eBlockchainMode::eBlockchainMode::LocalData;
	CGlobalSecSettings::setTesMode(true);

	// Use passed CTools or create a new one if not provided
	if (tools) {
		mTools = tools;
	} else {
		mTools = std::make_shared<CTools>("Test Unit", eBlockchainMode::eBlockchainMode::LocalData);
	}

	mTransactionsManager = nullptr;
	mBlockchainManager = nullptr;
	mWorkManager = nullptr;
	mStateDomainManager = nullptr;
	mStateDB = nullptr;
	mCryptoFactory = nullptr;
	this->mBlockchainManager = manager;

	std::cerr << "[DEBUG CTests] BlockchainManager set (null=" << (manager == nullptr ? "yes" : "no") << ")" << std::endl;
	std::cerr.flush();

	if (mBlockchainManager != nullptr)
		mSettings = manager->getSettings();
	if (mBlockchainManager != nullptr)
		this->mWorkManager = mBlockchainManager->getWorkManager();
	if (mBlockchainManager != nullptr)
	{
		this->mTransactionsManager = std::make_shared<CTransactionManager>(eTransactionsManagerMode::Terminal, mBlockchainManager, mWorkManager, mSettings->getMinersKeyChainID(), false, blockchainMode, false, true, true); // doBlockFormation=false, createDetachedDB=true, doNOTlockChainGuardian=true
		this->mTransactionsManager->setNamePrefix("Tests");
	}
	if (mBlockchainManager != nullptr)
		this->mSS = mBlockchainManager->getSolidStorage();
	if (mTransactionsManager != nullptr)
		this->mStateDB = mTransactionsManager->getFlowDB();
	if (mBlockchainManager != nullptr)
		this->mCryptoFactory = mBlockchainManager->getCryptoFactory();
	if (mTransactionsManager != nullptr)
		this->mStateDomainManager = mTransactionsManager->getStateDomainManager();
	//this->mStateDomainManager->enterFlow();

	std::cerr << "[DEBUG CTests] Creating CScriptEngine (as shared_ptr)..." << std::endl;
	std::cerr.flush();
	// Use make_shared so shared_from_this() works inside CScriptEngine
	this->mScriptEngine = std::make_shared<SE::CScriptEngine>(mTransactionsManager, mBlockchainManager, true, eViewState::eViewState::eventView);
	std::cerr << "[DEBUG CTests] CScriptEngine created" << std::endl;
	std::cerr.flush();

	// Initialize GridScript definitions - required for decompilation to work
	// reset(true) calls initializeDefinitions() internally to load all codeword definitions
	std::cerr << "[DEBUG CTests] Calling reset(true)..." << std::endl;
	std::cerr.flush();
	try {
		this->mScriptEngine->reset(true);
		std::cerr << "[DEBUG CTests] reset(true) completed" << std::endl;
		std::cerr.flush();
	} catch (const std::exception& e) {
		std::cerr << "[DEBUG CTests] reset(true) threw exception: " << e.what() << std::endl;
		std::cerr.flush();
		throw;
	} catch (...) {
		std::cerr << "[DEBUG CTests] reset(true) threw unknown exception" << std::endl;
		std::cerr.flush();
		throw;
	}

	if (mCryptoFactory == nullptr)
		mCryptoFactory = CCryptoFactory::getInstance();
	//assert(this->mScriptEngine != NULL && mBlockchainManager!=NULL && mSS!=NULL, mStateDB!=NULL, mCryptoFactory=NULL,
	//	mStateDomainManager=NULL, mScriptEngine=NULL, mWorkManager!=NULL);
}
bool CTests::testGridcraftPDU()
{
	std::shared_ptr<CMinecraftPDU> pdu = std::make_shared<CMinecraftPDU>(eGridcraftMsgType::ready);
	std::vector<uint8_t> bytes = pdu->getPackedData();
	std::shared_ptr<CMinecraftPDU> retrieved = CMinecraftPDU::instantiate(bytes);

	if (!retrieved)
		return false;
	return true;

}
bool CTests::testBigInts()
{

	BigInt bi = 324324;
	std::vector<uint8_t> b = mTools->BigIntToBytes(bi);
	//mTools->swap
	uint64_t uib = 0;
	std::memcpy(&uib, b.data(), b.size());

	if (bi != uib)
		return false;


	//Unsigned - BEGIN
	BigInt bi1 = 2;
	BigInt bi2 = 0;
	std::string s1;
	std::string s2;


	bi1 = 100000000000;
	bi2 = bi1;
	std::vector<uint8_t> bytes = mTools->BigIntToBytes(bi1);
	bi2 = mTools->BytesToBigInt(bytes);
	if (bi2 != bi1)
		return false;

	bi1 = 0;
	bi2 = bi1;
	bytes = mTools->BigIntToBytes(bi1);
	bi2 = mTools->BytesToBigInt(bytes);
	if (bi2 != bi1)
		return false;

	bi1 = 1;
	bi2 = bi1;
	bytes = mTools->BigIntToBytes(bi1);
	bi2 = mTools->BytesToBigInt(bytes);
	if (bi2 != bi1)
		return false;


	bi1 = UINT64_MAX * 2;
	bi2 = bi1;
	bytes = mTools->BigIntToBytes(bi1);
	bi2 = mTools->BytesToBigInt(bytes);
	if (bi2 != bi1)
		return false;
	//Unsigned - END

	//Signed - Begin
	BigSInt bsi1 = 2;
	BigSInt bsi2 = 0;

	bsi1 = 0;
	bsi2 = bsi1;
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	if (bsi2 != bsi1)
		return false;

	bsi1 = UINT64_MAX * 2;
	bsi2 = bsi1;
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	if (bsi2 != bsi1)
		return false;

	bsi1 = 100000000000;
	bsi2 = bsi1;
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	if (bsi2 != bsi1)
		return false;

	bsi1 = -234324;
	s1 = bsi1.str();
	bsi1 = bsi1;
	bsi2 = bsi1;
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	s2 = bsi2.str();
	if (bsi2 != bsi1)
		return false;

	if (s1.compare(s2) != 0)
		return false;

	//bsi1 = INT64_MAX * 2; WARNING: AVOID  as INT64_MAX * 2 would be computer in-place and would resulting in an overflow/rounding
	//INSTEAD; assign to BigSInt and multiple THEN.
	bsi1 = INT64_MAX;
	bsi1 = bsi1 * 2;
	s1 = bsi1.str();
	bsi2 = bsi1;
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	if (bsi2 != bsi1)
		return false;
	s2 = bsi2.str();

	if (s1.compare(s2) != 0)
		return false;

	bsi1 = 0;
	bsi2 = bsi1;
	s1 = bsi1.str();
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	if (bsi2 != bsi1)
		return false;
	s2 = bsi2.str();

	if (s1.compare(s2) != 0)
		return false;

	bi1 = 0;
	bi2 = bi1;
	s1 = bsi1.str();
	bytes = mTools->BigIntToBytes(bi1);
	bi2 = mTools->BytesToBigInt(bytes);
	if (bi1 != bi2)
		return false;
	s2 = bsi2.str();

	if (s1.compare(s2) != 0)
		return false;

	int64_t si = -12132321;
	s1 = std::to_string(si);
	bsi1 = si;
	s1 = bsi1.str();
	bsi2 = bsi1;
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	if (bsi2 != bsi1)
		return false;
	s2 = bsi2.str();

	if (s1.compare(s2) != 0)
		return false;

	si = -121323212;
	s1 = std::to_string(si * 2);
	bsi1 = si * 2;
	s1 = bsi1.str();
	s2 = std::to_string(si * 2);
	if (s1.compare(s2) != 0)
		return false;
	s1 = bsi1.str();
	bsi2 = bsi1;
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	if (bsi2 != bsi1)
		return false;
	s2 = bsi2.str();

	if (s1.compare(s2) != 0)
		return false;


	bsi1 = INT64_MAX * 2;
	bsi2 = bsi1;
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	if (bsi2 != bsi2)
		return false;


	//Atto Units - BEGIN
	bi1 = mTools->GNCToAtto(300000);
	s1 = bi1.str();
	bsi1 = ((BigSInt)-1 * (BigSInt)bi1);
	s1 = bsi1.str();
	bytes = mTools->BigSIntToBytes(bsi1);
	bsi2 = mTools->BytesToBigSInt(bytes);
	s2 = bsi2.str(0);

	if (bsi1 != bsi2)
		return false;

	//Atto Units - END


	//Signed - END

	//Floating Point Precision - BEGIN
	BigFloat f1 = BigFloat("332432432432432432111.5");
	bytes = mTools->BigFloatToBytes(f1);

	BigFloat f2 = mTools->BytesToBigFloat(bytes);

	if (f1 != f2)
		return false;
	//Floating Point Precision - END

	return true;
}
bool CTests::testFileMetaDescriptors()
{
	std::shared_ptr<CFileMetaData> desc = std::make_shared<CFileMetaData>();

	//test properties
	desc->setName(mTools->getRandomStr(5));
	desc->setDescription(mTools->getRandomStr(5));
	desc->setIcon(mTools->genRandomVector(5));
	desc->setPreviewURI(mTools->getRandomStr(5));

	//test additional custom attributes
	desc->addAttribute(mTools->getRandomStr(5), mTools->getRandomStr(5));
	desc->addAttribute(mTools->getRandomStr(5), mTools->getRandomStr(5));

	std::vector<uint8_t> serialized = desc->getPackedData();

	std::shared_ptr<CFileMetaData> retrieved = CFileMetaData::instantiate(serialized);

	if (!retrieved)
		return false;

	std::vector<uint8_t> serialized2 = retrieved->getPackedData();

	if (!mTools->compareByteVectors(serialized, serialized2))
		return false;

	return true;

}

void CTests::stop()
{

	if (mTransactionsManager != nullptr)
		mTransactionsManager->stop();

}

CTests::~CTests()
{
	// mScriptEngine is now a shared_ptr - no manual delete needed
	// (shared_ptr manages its own memory via reference counting)
	//if (mTransactionsManager != NULL)
	//	delete mTransactionsManager;
}
/// <summary>
/// Tests the Genesis facts-file JSOn serialization/deserialization; file storage and retrieval also integirty tests.
/// </summary>
/// <returns></returns>
bool CTests::testGenesisRewardsFactFileGenerator()
{
	CGenesisRewards rewards(mCryptoFactory, mBlockchainManager->getTools(), eBlockchainMode::LocalData);
	std::vector<uint8_t> pubKey;
	Botan::secure_vector<uint8_t> privKey;
	std::shared_ptr<CTools> tools = mBlockchainManager->getTools();
	uint64_t awardeesCount = 5;
	std::string fileName = CGlobalSecSettings::getGenesisFactFileName();
	std::string hash58 = CGlobalSecSettings::getBase58GenesisFactFileHash();
	std::string factFileContent;


	std::vector<uint8_t> supposedHash;

	if (hash58.size() > 0)
		if (!mTools->base58CheckDecode(hash58, supposedHash))
			return false;

	if (mBlockchainManager->getSolidStorage()->readStringFromFile(factFileContent, fileName))
	{
		mTools->writeLine("Genesis Facts-File found (" + fileName + ") ! Loading..");
		awardeesCount = 0;//do not verify numerosity - use current file
	}
	else
	{

		mTools->writeLine("Attempting to generate the Genesis JSON facts-file..");
		CKeyChain chain;
		//Create the Genesis Domain
		assertGN(rewards.addAwardee(CGlobalSecSettings::getTheGenesisStateDomainValue(), pubKey, privKey));
		mSettings->saveKeyChain(chain);
		mSettings->setMinersKeyChainID(chain.getID());
		tools->writeLine("Genesis State Domain: " + chain.getID());
		//issue rewards
		mTools->writeLine("Rewarding investors..");

		for (int i = 0; i < awardeesCount; i++)
		{
			chain = CKeyChain();
			if (!rewards.addAwardee(mTools->genRandomNumber(10, 100000000), pubKey, privKey))
				return false;
		}

		if (!rewards.genGenesisRewardsJSON(factFileContent, supposedHash));

		mTools->writeLine("Successfully generated content of the Gensesis JSON facts-file. Hash: " + mTools->base58CheckEncode(supposedHash));
		mTools->writeLine("Saving file " + fileName);

		if (!mBlockchainManager->getSolidStorage()->saveStringToFile(factFileContent, fileName))
			return false;

	}

	mTools->writeLine("Attempting to retrieve the Gensesis JSON facts-file..");
	rewards.reset();

	if (!mBlockchainManager->getSolidStorage()->readStringFromFile(factFileContent, fileName))
		return false;

	mTools->writeLine("Attempting to validate and parse the Gensesis JSON facts-file..");
	//verify hash
	if (supposedHash.size() > 0)
	{
		std::vector<uint8_t> actualHash = mCryptoFactory->getSHA2_256Vec(mTools->stringToBytes(factFileContent));
		if (!mTools->compareByteVectors(supposedHash, actualHash))
		{
			mTools->writeLine("Invalid hash of the Genesis facts-file!");
			return false;
		}
		else
		{
			mTools->writeLine("Integrity of the Genesis facts-file VERIFIED!");
		}

	}

	if (!rewards.parse(factFileContent))
		return false;

	std::vector<std::shared_ptr<CStateDomain>> domains;

	if (!rewards.getGenesisStateDomains(domains))
		return false;

	if (awardeesCount && domains.size() != awardeesCount + 1)
		return false;
	mTools->writeLine("The retrieval was successfull. (" + std::to_string(domains.size()) + " awardees found )");
	return true;
}

/// <summary>
/// Testing the Verifiables data-structure.
/// Signature mechanism, state-domain modification, copy constructors etc. are validated.
/// </summary>
/// <returns></returns>
bool CTests::testVerifiablesDataStructure()
{
	//test serialization
	CKeyChain chain;
	CVerifiable ver = CVerifiable(eVerifiableType::minerReward, mTools->genRandomVector(32));
	std::vector<uint8_t> dID1 = mTools->genRandomVector(32);
	std::vector<uint8_t> dID2 = mTools->genRandomVector(32);
	mCryptoFactory->genAddress(dID1, dID1);
	mCryptoFactory->genAddress(dID2, dID2);
	assertGN(ver.addBalanceChange(dID1, 1));
	assertGN(ver.addBalanceChange(dID2, -1));
	assertGN(ver.sign(chain.getPrivKey()));
	bool valid = ver.verifySignature();
	std::vector<uint8_t> serialized = ver.getPackedData();
	CTrieNode* reconstructed = mTools->nodeFromBytes(serialized, eBlockchainMode::eBlockchainMode::LocalData);

	if (reconstructed == nullptr)
		return false;

	if (!reinterpret_cast<CVerifiable*>(reconstructed)->verifySignature(chain.getPubKey()))
		return false;
	std::vector<std::shared_ptr<CStateDomain>> domains = reinterpret_cast<CVerifiable*>(reconstructed)->getAffectedStateDomains();
	if (domains.size() != 2)
		return false;
	if (!mTools->compareByteVectors(domains[0]->getAddress(), dID1))
		return false;
	if (!mTools->compareByteVectors(domains[1]->getAddress(), dID2))
		return false;

	BigSInt bChange1 = domains[0]->getPendingPreTaxBalanceChange();
	BigSInt bChange2 = domains[1]->getPendingPreTaxBalanceChange();
	if (bChange1 != 1)
		return false;
	if (bChange2 != -1)
		return false;

	//test copy constructor
	CVerifiable r = *(reinterpret_cast<CVerifiable*> (reconstructed));

	std::vector<uint8_t> serialized2 = r.getPackedData();
	if (!(serialized.size() == serialized2.size() && mTools->compareByteVectors(serialized, serialized2)))
		return false;
	//test assignment operator
	CVerifiable r2 = CVerifiable(eVerifiableType::minerReward, mTools->genRandomVector(32));
	r2 = r;

	std::vector<uint8_t> serialized3 = r2.getPackedData();
	if (!(serialized3.size() == serialized2.size() && mTools->compareByteVectors(serialized3, serialized2)))
		return false;

	return true;
	//TODO: test copy constructors
}

bool CTests::testDomainIDs()
{
	uint32_t progress = 0;
	uint32_t count = 100;
	int previousFlashed = 0;

	Sleep(1000);
	mTools->writeLine("Domain IDs test , commencing..");
	CKeyChain chain = CKeyChain();
	std::vector<uint8_t> domainID;
	mSettings->saveKeyChain(chain);



	for (int i = 0; i < count; i++)
	{
		progress = (int)(((double)i / (double)count) * 100);

		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("completed: " + std::to_string((int)progress) + "%");
		}

		if (!mCryptoFactory->genAddress(chain.getPubKey(), domainID))
			return false;
		if (domainID.size() != 34 && domainID.size() != 33)
			return false;
	}

	mTools->writeLine("Tests successful.");
	return true;
}

bool CTests::testHistoricalData()
{
	mTools->writeLine("\nBlockchain historical-data test commencing now.\n");
	Sleep(2000);
	mTools->writeLine("testing cached root-blocks and their subtries");
	std::vector<std::shared_ptr<CBlock>> blocks = mSS->getCachedBlocks();
	mTools->writeLine("Number of cached TrieDB histories: " + std::to_string(blocks.size()));
	std::shared_ptr<CBlock> block;
	std::shared_ptr<CBlockHeader> header;
	CTrieNode* root;
	CTrieNode* testNode;
	std::string msg;
	for (int i = 0; i < blocks.size(); i++)
	{
		header = blocks[i]->getHeader();
		std::vector<uint8_t> rh = header->getPerspective(state);

		if (mStateDB->setPerspective(rh))
			mTools->writeLine("Block's State-Root retrived!");
		else {
			mTools->writeLine("There was an error!");
			return false;
		}
		size_t s = 0;
		mStateDB->testTrie(s);
		testNode = mStateDB->findNodeByFullID(testSDIDs[i]);
		if (testNode != NULL)
			mTools->writeLine("SUCCESS ! - Test-Node found in historical state.");
		else
		{
			mTools->writeLine("ERROR: Test-Node could not be reconstructed!");
			return false;
		}


	}

	mTools->writeLine("\n\n == Historical data test: Passed! ==");
	return true;
}

void CTests::interactiveGridScriptTest()
{
	assertGN(mScriptEngine != NULL);
	mTools->writeLine("Entering *Interactive* (user input required, or just simply type 'quit'");
	mScriptEngine->setTerminalMode(true);
	mScriptEngine->main();

}

bool CTests::testPolls()
{
	std::shared_ptr<CPollElem> poll;
	PollElemFlags flags;
	BigInt reqScoring = 0;
	uint64_t pollsCount = 14;
	uint64_t level1VotersCount = 30;
	uint64_t level2VotersCount = 30;//per level-1 voter
	std::shared_ptr<CVoter> voterL1;
	std::vector < std::tuple<std::vector<uint8_t>, uint64_t>> voterL1IDs;
	std::vector<uint8_t> bytes;
	std::shared_ptr<CPollElem> retrieved;
	std::shared_ptr<CVoter> v;
	std::shared_ptr<CPollFileElem> fileElem = std::make_shared<CPollFileElem>();
	uint64_t localPollID = 0;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	//Test Data Structures - BEGIN
	VoterFlags vf;
	for (uint64_t z = 0; z < pollsCount; z++)
	{
		voterL1IDs.clear();

		poll = std::make_shared<CPollElem>();

		reqScoring = mTools->genRandomNumber(1000023, 21332434234234);
		// CPollElem(std::shared_ptr<CPollFileElem> fileElem, const uint64_t localPollID, const BigInt & requiredScoring, const PollElemFlags flags, const std::string & description);
		poll = std::make_shared<CPollElem>(fileElem, z, reqScoring, flags);
		poll->setDescription(mTools->getRandomStr(25));

		for (uint64_t y = 0; y < level1VotersCount; y++)
		{
			vf.mDoNotCountPower = tools->genRandomNumber(0, 1) > 0 ? true : false;
			voterL1IDs.push_back(std::make_tuple(mTools->genRandomVector(34), vf.getNr()));
			voterL1 = std::make_shared<CVoter>(std::get<0>(voterL1IDs[voterL1IDs.size() - 1]), VoterFlags(vf.getNr()));

			for (uint64_t i = 0; i < level2VotersCount; i++)
			{
				voterL1->addL2Voter(std::make_shared<CVoter>(mTools->genRandomVector(34)));
			}

			poll->addVoter(voterL1);
		}
		bytes = poll->getPackedData();
		retrieved = CPollElem::instantiate(bytes);
		if (retrieved)
		{
			if (reqScoring != poll->getRequiredScoring() || level1VotersCount != poll->getVotersCount()
				|| poll->getFlags() != flags)
			{
				return false;
			}


			for (uint64_t i = 0; i < voterL1IDs.size(); i++)
			{
				v = poll->getVoterByID(std::get<0>(voterL1IDs[i]));
				if ((v == nullptr))
				{
					return false;//missing L1 voter
				}
				if (v->getVotersCount() != level2VotersCount)
				{
					return false;
				}
				if (mTools->compareByteVectors(v->getID(), std::get<0>(voterL1IDs[i])) == false)
				{
					return false;
				}
				if (v->getFlags().getNr() != std::get<1>(voterL1IDs[i]))
				{
					return false;
				}


			}


		}
		fileElem->addPoll(poll);
		localPollID++;
	}

	//File Elem - BEGIN

	bytes = fileElem->getPackedData();
	std::shared_ptr<CPollFileElem> retrievedFElem = CPollFileElem::instantiate(bytes);

	//test copy/assignment-constructors - BEGIN
	std::shared_ptr<CPollFileElem> newCopy = std::make_shared<CPollFileElem>(*retrievedFElem);
	std::vector<uint8_t> bytes2 = newCopy->getPackedData();
	if (!mTools->compareByteVectors(bytes2, bytes))
	{
		return false;
	}

	PollElemFlags pf = newCopy->getPollAtIndex(1)->getFlags();
	pf.setFired(true);
	pf.setIsOneTime();
	pf.setIsActive(true);
	newCopy->getPollAtIndex(1)->setFlags(pf);
	newCopy->resetPollsState();
	pf = newCopy->getPollAtIndex(1)->getFlags();
	if (!(pf.getIsActive() && !pf.getFired()))
	{
		return false;
	}
	if (newCopy->getPollAtIndex(1)->getVotersCount() != 0)
		return false;

	newCopy->resetPollsState();
	//test copy/assignment-constructors - BEGIN



	for (uint64_t i = 0; i < pollsCount; i++)
	{
		if (fileElem->getPollAtIndex(i)->getLocalPollID() != i)
		{
			return false;
		}
	}

	if (retrievedFElem->getPollsCount() != pollsCount)
	{
		return false;
	}

	poll = retrievedFElem->getPollWithLocalID(eSystemPollID::removalAccessRightsPoll);

	if (!poll)
		return false;
	flags.setIsActive(true);
	poll->setFlags(flags);

	//File Elem - END


	//Test Data Structures - END
	return true;

}

bool CTests::testGridScriptCodeSerialization()
{

	std::string s1 = "1 2 data John -1234.333 123.32 -123.1222 123.342 + 1 1 0";
	std::string s2 = "10000000000000 data GNC data 15ZtqfUW5wjv23SK7J9F3kbhC2nxLdsnKV data 0 0 data 0";
	std::string s3 = "4128500000 data GNC data 1JFb99si5GT2yGwpxcz7NQy3E3B5MAsDTN data 0 0 data 0 xvalue";

	std::string s4 = "data58 QXaaDTQHxgUaLghBCvSDUXSS6s3kAWFjGzc4HnqfDheYvepEX data 16RcPSCdd8has7pTindEXt3XsoQeppBM5q data 1PeTP9d9hNNux3w2yUxBiPviNBahNdxYxT data GNC 0 data58 G8usXcHDoxhxq45AR4b5E3CsBVMFsF8W9Bxd8yAgAEW7rKykZyWVXhzhPHoj1A522qSUjEqYZ4j4MxZ9dH18kR3uLQ7Nr xvalue";
	std::string s5 = "send 10 QXaaDTQHxgUaLghBCvSDUXSS6s3kAWFjGzc4HnqfDheYvepEX"; //multiple inline arguments to a function call
	std::string s6 = "send 10"; //multiple inline arguments to a function call
	std::string s7 = "sdasdsa sadsadsa sdfdsf sdfdsf";
	std::string s8 = "";//nothing to compile
	std::string s9 = "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / data58 48xZ5TG6MJHqC3c7YiTfHvA6N9Y7HJupHUZzdkrgsi2LVXo8Tnx7sAuP9FUrUg2TLtRbDmUUVSWkQz2aRZCFLhkiFbzRiSfCmcjFGShR2urWaYMaAsw3PUi8NihGU5fqKPXfCrdDXvCUd67JpSWjH882bPxT9dnBcfR6TQjaQkseqCprVx1xu7RwZZ1LybGdS7WmccrKbmBaDJo3c8EBG68aofc3KNsTz3dhGpivL34a4Vw6SsPWkzGQWbJu6HAyhZheg7qwGaAZeoRZkFo5jRyKYyn regPoolEx";//regPoolEx: 0 inline params, data from stack

	// regPool test scenarios - V2+ bytecode allows omitting inline arguments (data comes from stack)
	// regPool has 1 inline param configured but V2+ allows calling without it when data is on stack
	std::string s15 = "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / data58 48xZ5TG6MJHqC3c7YiTfHvA6N9Y7HJupHUZzdkrgsi2LVXo8Tnx7sAuP9FUrUg2TLtRbDmUUVSWkQz2aRZCFLhkiFbzRiSfCmcjFGShR2urWaYMaAsw3PUi8NihGU5fqKPXfCrdDXvCUd67JpSWjH882bPxT9dnBcfR6TQjaQkseqCprVx1xu7RwZZ1LybGdS7WmccrKbmBaDJo3c8EBG68aofc3KNsTz3dhGpivL34a4Vw6SsPWkzGQWbJu6HAyhZheg7qwGaAZeoRZkFo5jRyKYyn regPool";//regPool: 1 inline param but omitted (V2+ allows empty inline args)

	// regPool WITH base64-encoded inline argument (the -t flag triggers inline parameter parsing)
	// Generate proper base64Check encoded data for s16 (same data size as s15's base58 data for consistency)
	std::vector<uint8_t> s16Data = mTools->genRandomVector(200);  // ~200 bytes of test data
	std::string s16 = "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / regPool -t " + mTools->base64CheckEncode(s16Data);//regPool: with -flag inline argument

	// s17: regPool WITH +flag (plus flag) variant - tests both - and + flag prefixes
	std::vector<uint8_t> s17Data = mTools->genRandomVector(150);  // slightly different size
	std::string s17 = "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd / regPool +verbose " + mTools->base64CheckEncode(s17Data);//regPool: with +flag inline argument

	std::vector<uint8_t> rData = mTools->genRandomVector(50000);

	std::string s10 = "data64 " + mTools->base64CheckEncode(rData);
	std::string s11 = "3242343243246545645645765765765765756456456664566457657 -3242343243246545645645765765765765756456456664566457657";//big numbers
	std::string s12 = "cd '/1CH6REvyxr4jWnqBSEqv7PnYQafH89CfXu/' 0 0 data \"New File 15\" setVarEx adata64 PHA + Z2ZkZ2ZkZ2ZnIGRmZ2RmZ2ZkZzwvcD65XzBN 0 data \"/18kjSJCKT8DfJUeSJsgD4r6EpXznKQ746E/New File 15\" setVarEx  cd // ";
	std::string s13 = "cd '/1CH6REvyxr4jWnqBSEqv7PnYQafH89CfXu/' adata64 SUNPIHByaWNlIHBlciBHUklETkVUIENPSU4gPSAwLjEgVVNEDQoNCjIwMCAwMDAgR0JVcyAob2Jzb2xldGUpID0gNTAwIFVTRDsNCg0Kd2hpY2ggZ2l2ZXMgNTAwMCBHUklETkVUIENvaW5zIGZvciAyMDAgMDAwIEdCVXMgKGFwcCBkb3dubG9hZCkNCg0KDQpmb3IgZWFjaCAyMDAgMDAwIEdCVXMgd2Ugbm93IG5lZWQgdG8gaGFuZGxlIDUwMDAgR1JJRE5FVCBDb2lucw0KDQoNCmVhY2ggYWNjb3VudCBuZWVkcyB0aGlzIHRvIGVuZC11cCB3aXRoIChiYWxhbmNlLzIwMCAwMDApICogNTAwMCBHTkMNCg0KDQoNCndlZSBuZWVkIHRvIHJhaXNlIDIwMCBtaWxsaW9uIHVzZCAoSUNPIGNhcCkNCjAuNyp4ID0gMjAwDQp4PT0gMjAwLzAuNw0KeCA9IDI4NS43MTQyODU3MTQyODU3MTQyODU3MTQyODU3MTQyOSAodG90YWwgY2FwIGluIFVTRCkNCg0KODUgbWlsbGlvbiBVU0QgIGdvZXMgdG8gdGhlIHRlYW0NCg0KVG90YWwgU3VwcHkgaW4gR05DOg0KDQoyODUuNzE0Mjg1NzE0Mjg1NzE0Mjg1NzE0Mjg1NzE0MjkgLyAwLjMNCg0KOTUyLjM4MDk1MjM4MDk1MjM4MDk1MjM4MDk1MjM4MDk1IG1pbGxpb24gR05DIFRvdGFsIHNwcGx5B4QA5A== 0 data /18kjSJCKT8DfJUeSJsgD4r6EpXznKQ746E/rewards.txt setVarEx cd //";

	std::string s14 = "cd '/1692oZ2xanmUozb83quAmP3BvefvCjG9Yv/' data \"thread.begin(); thread.preserveCall = true; console.log('I was called by: ' + system.callersID); console.log('I received ' + system.assets.receivedGNCFloat + ' GNC'); var randomNr = crypto.getRandomNumber(0, 10, false, true, true, true); if (randomNr > 6) { var price = system.assets.receivedGNCFloat * 2; console.log('You have won ' + price + ' GNC!'); if (price > 0) { console.log('Making a transfer to: ' + system.callersID); system.assets.sendGNCFloat(system.callersID, price); } } else { console.log('You have lost.'); }\" 0 data dice.gpp setVarEx";
	std::vector<uint8_t> byteCode;

	std::string testedScript = s6;
	std::string  reconstructed;



	//FALSE-positive tests - BEGIN

	testedScript = s10;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;


	if (!mScriptEngine->getByteCode(testedScript, byteCode))
		return false;// version 2 byte-code compilation now accepts missing inline arguments


	testedScript = s12;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		return false;//this shouldn't compile as there's a space within adata64

	testedScript = s13;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;

	testedScript = s11;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;






	testedScript = s9;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;

	// ════════════════════════════════════════════════════════════════════
	//                    regPool TEST SCENARIOS
	// ════════════════════════════════════════════════════════════════════
	// Test s15: regPool WITHOUT inline argument (V2+ bytecode feature)
	// regPool expects 1 inline param but V2+ allows empty inline args when data comes from stack
	testedScript = s15;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;

	// Test s16: regPool WITH -flag inline argument provided
	// Tests the minus flag (-t) with inline parameter
	testedScript = s16;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;

	// Test s17: regPool WITH +flag inline argument provided
	// Tests the plus flag (+verbose) with inline parameter
	testedScript = s17;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;

	//testedScript = s7;
	//if (mScriptEngine->getByteCode(testedScript, byteCode))
		//return false;//should not succeed;unknown op-codes

	//testedScript = s8;
	//if (mScriptEngine->getByteCode(testedScript, byteCode))
	//	return false;//should not succeed;unknown op-codes
	//FALSE-positive tests - END


	//GridScript++ - BEGIN
	testedScript = s14;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;

	//GridScript++ - END

	testedScript = s5;
	assertGN(mScriptEngine->getByteCode(testedScript, byteCode));
	reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (std::memcmp(testedScript.c_str(), reconstructed.c_str(), testedScript.size()) != 0)
		return false;

	testedScript = s4;
	assertGN(mScriptEngine->getByteCode(testedScript, byteCode));
	reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (std::memcmp(testedScript.c_str(), reconstructed.c_str(), testedScript.size()) != 0)
		return false;

	assertGN(mScriptEngine->getByteCode(s1, byteCode));
	reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (std::memcmp(s1.c_str(), reconstructed.c_str(), s1.size()) != 0)
		return false;

	assertGN(mScriptEngine->getByteCode(s2, byteCode));
	reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (std::memcmp(s2.c_str(), reconstructed.c_str(), s2.size()) != 0)
		return false;// WARNING the result won't be the same if we were to use high precision double numbers.
	//ex: std::string s1 = "data rafal -123242423424.33322233 123.323344442 -123.122342342 123.342323444421 + 1.000000001 1 0";
	assertGN(mScriptEngine->getByteCode(s3, byteCode));
	reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (std::memcmp(s3.c_str(), reconstructed.c_str(), s3.size()) != 0)
		return false;

	// ════════════════════════════════════════════════════════════════════
	//              PRODUCTION TEST CASES (Token Pool & Sacrificial)
	// ════════════════════════════════════════════════════════════════════
	// These are real-world GridScript patterns used in production
	// Token Pool Registration with large data58 payload (extended length encoding)
	std::string tokenPoolReg = "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp data58 H4KoDXsTTyEkk1juHx13ePgUjudq6NoL53Q1wKuNPVAKuCctSnfEq5t2sjPhKyBdegB7Fypksii8JV7P6SUA4vyfj5vpouoycAfQNciK7uXSXZ1RtPjjybjCPi5JDJ97ZKMu5dex8CHTadrchHMKGS5rZ1HPsUoJMBrUG6McgxLZ37yLGCUuS8rsFq5T5dvey9NAijP4TXyWkT2RYDsMq9PyJbMVzz19xAH121zA8RPbfczhDG5D4Dsay1gsbfDQTWcxpdbyhyfhXwCdPqQrMmSmQcQSMinNswtnSdWQ9dujrgs71WD44ibFBvJwyDntKSBvrtFqkWPnq2PKcvVMih4LXxzUBUuYRKy6vZTvrzNsjqowf5 regPool";
	testedScript = tokenPoolReg;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;

	// Sacrificial Transaction - real production pattern
	std::string sacrificialTx = "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp sacrifice 14400000";
	testedScript = sacrificialTx;
	if (mScriptEngine->getByteCode(testedScript, byteCode))
		reconstructed = mScriptEngine->getSourceCode(byteCode);
	if (!CTools::getTools()->doStringsMatch(testedScript, reconstructed))
		return false;

	return true;
}

bool CTests::testJavaScriptBytecodeCrossValidation()
{
	// ════════════════════════════════════════════════════════════════════════════
	//  JAVASCRIPT BYTECODE CROSS-VALIDATION TEST
	// ════════════════════════════════════════════════════════════════════════════
	// This test verifies that bytecode compiled in JavaScript (production WebUI)
	// can be correctly decompiled by the C++ CScriptEngine (production GRIDNET Core).
	// This ensures bidirectional compatibility between JavaScript and C++ compilers.
	//
	// CROSS-VALIDATION FLOW:
	// 1. JavaScript compiles test cases → writes bytecodes to gridnet_js_bytecodes_for_cpp.json
	// 2. C++ reads gridnet_js_bytecodes_for_cpp.json → decompiles and validates (THIS FUNCTION)
	// 3. C++ compiles test cases → writes bytecodes to gridnet_cpp_bytecodes_for_js.json
	// 4. JavaScript reads gridnet_cpp_bytecodes_for_js.json → decompiles and validates
	// ════════════════════════════════════════════════════════════════════════════

	struct CrossValidationTest {
		std::string name;
		std::string base64Bytecode;
		std::string expectedSource;
	};

	std::vector<CrossValidationTest> tests;
	int passed = 0, failed = 0;
	std::vector<uint8_t> bytecode;

	// Open log file in temp directory for detailed debugging
	std::filesystem::path tempDir = std::filesystem::temp_directory_path();
	std::filesystem::path logPath = tempDir / "gridnet_crossval_debug.txt";
	std::filesystem::path jsInputPath = tempDir / "gridnet_js_bytecodes_for_cpp.json";

	std::ofstream logFile(logPath.string());
	logFile << "C++ JavaScript Bytecode Cross-Validation Results\n";
	logFile << "================================================\n";
	logFile << "Log path: " << logPath.string() << "\n";
	logFile << "JS bytecodes file: " << jsInputPath.string() << "\n\n";

	// Try to read JavaScript bytecodes from file
	if (std::filesystem::exists(jsInputPath))
	{
		mTools->writeLine("Reading JavaScript bytecodes from: " + jsInputPath.string());
		logFile << "Reading JavaScript bytecodes from file...\n";

		std::ifstream jsFile(jsInputPath.string());
		if (jsFile.is_open())
		{
			std::stringstream buffer;
			buffer << jsFile.rdbuf();
			std::string jsonContent = buffer.str();
			jsFile.close();

			// Simple JSON parsing for array of {name, bytecode, expected}
			// Format: [{"name":"...","bytecode":"...","expected":"..."},...]
			size_t pos = 0;
			while ((pos = jsonContent.find("\"name\"", pos)) != std::string::npos)
			{
				CrossValidationTest test;

				// Extract name
				size_t nameStart = jsonContent.find("\"", pos + 6) + 1;
				size_t nameEnd = jsonContent.find("\"", nameStart);
				test.name = jsonContent.substr(nameStart, nameEnd - nameStart);

				// Extract bytecode
				size_t bcPos = jsonContent.find("\"bytecode\"", nameEnd);
				size_t bcStart = jsonContent.find("\"", bcPos + 10) + 1;
				size_t bcEnd = jsonContent.find("\"", bcStart);
				test.base64Bytecode = jsonContent.substr(bcStart, bcEnd - bcStart);

				// Extract expected
				size_t expPos = jsonContent.find("\"expected\"", bcEnd);
				size_t expStart = jsonContent.find("\"", expPos + 10) + 1;
				size_t expEnd = jsonContent.find("\"", expStart);
				test.expectedSource = jsonContent.substr(expStart, expEnd - expStart);

				tests.push_back(test);
				pos = expEnd;
			}

			logFile << "Loaded " << tests.size() << " test cases from JavaScript bytecodes file\n";
			mTools->writeLine("Loaded " + std::to_string(tests.size()) + " JavaScript bytecode test cases");
		}
		else
		{
			logFile << "ERROR: Failed to open JavaScript bytecodes file\n";
			mTools->writeLine("ERROR: Failed to open JavaScript bytecodes file");
		}
	}
	else
	{
		mTools->writeLine("WARNING: JavaScript bytecodes file not found: " + jsInputPath.string());
		mTools->writeLine("Run JavaScript tests first: node WebUI/test/test_bytecode_serialization.mjs");
		logFile << "WARNING: JavaScript bytecodes file not found\n";
		logFile << "Run JavaScript tests first to generate bytecodes.\n";
		logFile.close();
		return false;
	}

	if (tests.empty())
	{
		mTools->writeLine("ERROR: No JavaScript bytecode tests loaded");
		logFile << "ERROR: No tests loaded\n";
		logFile.close();
		return false;
	}

	// Debug: Log definitions with IDs 9-20
	auto definitions = mScriptEngine->getDefinitions();
	logFile << "DEBUG: Total definitions: " << definitions.size() << "\n";
	logFile << "DEBUG: Sample definitions (IDs 9-20):\n";
	for (const auto& def : definitions) {
		if (def.id >= 9 && def.id <= 20) {
			logFile << "  ID " << def.id << ": " << def.name << " (findable=" << (def.isFindable() ? "yes" : "no") << ")\n";
		}
	}
	logFile << "\n";

	for (const auto& test : tests)
	{
		logFile << "\n--- Test: " << test.name << " ---\n";
		logFile << "Base64 input length: " << test.base64Bytecode.length() << "\n";

		// Convert base64 string to bytes and decode
		std::vector<uint8_t> base64Bytes(test.base64Bytecode.begin(), test.base64Bytecode.end());

		logFile << "Base64 bytes vector size: " << base64Bytes.size() << "\n";

		if (!mTools->base64Decode(base64Bytes, bytecode))
		{
			mTools->writeLine("FAILED: " + test.name + " - base64 decode failed");
			logFile << "RESULT: FAILED - base64 decode returned false\n";
			logFile << "First 20 chars: " << test.base64Bytecode.substr(0, 20) << "\n";
			failed++;
			continue;
		}

		logFile << "Decoded bytecode size: " << bytecode.size() << " bytes\n";
		logFile << "First 5 bytes (hex): ";
		size_t bytesToShow = bytecode.size() < 5 ? bytecode.size() : 5;
		for (size_t i = 0; i < bytesToShow; i++) {
			logFile << std::hex << (int)bytecode[i] << " ";
		}
		logFile << std::dec << "\n";

		// Decompile using production CScriptEngine
		std::string decompiled = mScriptEngine->getSourceCode(bytecode);

		logFile << "Decompiled length: " << decompiled.length() << "\n";
		logFile << "Decompiled result: \"" << decompiled << "\"\n";
		logFile << "Expected result: \"" << test.expectedSource << "\"\n";

		// If decompilation failed, log opcode info for debugging
		if (decompiled.empty() && bytecode.size() > 33) {
			uint8_t firstOpcode = bytecode[33];
			logFile << "DEBUG: First opcode byte at pos 33: " << (int)firstOpcode << " (0x" << std::hex << (int)firstOpcode << std::dec << ")\n";
			if (bytecode.size() > 34) {
				logFile << "DEBUG: Second byte at pos 34: " << (int)bytecode[34] << " (0x" << std::hex << (int)bytecode[34] << std::dec << ")\n";
			}
			// Log bytes 33-40 for full opcode context
			logFile << "DEBUG: Bytes 33-40: ";
			for (size_t i = 33; i < bytecode.size() && i < 41; i++) {
				logFile << (int)bytecode[i] << " ";
			}
			logFile << "\n";
		}

		// Compare with expected (use == operator since doStringsMatch requires non-const refs)
		if (decompiled == test.expectedSource)
		{
			passed++;
			logFile << "RESULT: PASSED\n";
		}
		else
		{
			mTools->writeLine("FAILED: " + test.name);
			mTools->writeLine("  Expected: " + test.expectedSource);
			mTools->writeLine("  Got:      " + decompiled);
			logFile << "RESULT: FAILED - mismatch\n";
			failed++;
		}
	}

	mTools->writeLine("JavaScript Cross-Validation: " + std::to_string(passed) + " passed, " + std::to_string(failed) + " failed");

	// Write summary
	logFile << "\n═══════════════════════════════════════════════════════════════\n";
	logFile << "Summary:\n";
	logFile << "Total tests: " << tests.size() << "\n";
	logFile << "Passed: " << passed << "\n";
	logFile << "Failed: " << failed << "\n";
	logFile << "Result: " << (failed == 0 ? "ALL PASSED" : "SOME FAILED") << "\n";
	logFile.close();

	return failed == 0;
}

bool CTests::exportCppBytecodesForJavaScriptTesting()
{
	// ════════════════════════════════════════════════════════════════════════════
	//  C++ BYTECODE EXPORT FOR JAVASCRIPT REVERSE TESTING
	// ════════════════════════════════════════════════════════════════════════════
	// This function compiles GridScript using the PRODUCTION C++ CScriptEngine
	// and exports base64-encoded bytecodes that can be used for JavaScript
	// reverse cross-validation (testing that JavaScript can decompile C++ bytecodes).
	//
	// This is the SECOND DIRECTION of cross-validation:
	//   Direction 1: JavaScript compiles -> C++ decompiles (testJavaScriptBytecodeCrossValidation)
	//   Direction 2: C++ compiles -> JavaScript decompiles (this function exports bytecodes)
	// ════════════════════════════════════════════════════════════════════════════

	struct CppExportTest {
		const char* name;
		const char* source;
	};

	// Test cases - same sources as JavaScript tests for cross-validation
	std::vector<CppExportTest> tests = {
		{ "simple cd", "cd /test/path" },
		{ "cd with address", "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo" },
		{ "cd with / path", "cd /" },
		{ "multiple cd commands", "cd 1AB6WF3y3e5y9hmYAYvPeHqXmcboYkcujo cd /" },
		{ "simple data", "data GNC" },
		{ "multiple data", "data GNC data 15ZtqfUW5wjv23SK7J9F3kbhC2nxLdsnKV data 0" },
		{ "xvalue command", "data GNC xvalue" },
		{ "simple data58", "data58 2bNcNLCKuxzGWJ5VP" },
		{ "data64 simple", "data64 SGVsbG8gV29ybGSlkabU" },
		{ "regPool without inline param", "regPool" },
		{ "regPool with inline param", "regPool VGVzdCByZWdQb29sIGRhdGHOWbY6" },
		{ "regPool with -t flag", "regPool -t SGVsbG8gV29ybGSlkabU" },
		{ "regPool with +verbose flag", "regPool +verbose VmVyYm9zZSzVcQk=" },
		{ "setVarEx canonical case", "setVarEx" },
		{ "getVarEx canonical case", "getVarEx" },
		{ "echo canonical case", "data GNC echo" },
		{ "Token Pool Registration", "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp data58 H4KoDXsTTyEkk1juHx13ePgUjudq6NoL53Q1wKuNPVAKuCctSnfEq5t2sjPhKyBdegB7Fypksii8JV7P6SUA4vyfj5vpouoycAfQNciK7uXSXZ1RtPjjybjCPi5JDJ97ZKMu5dex8CHTadrchHMKGS5rZ1HPsUoJMBrUG6McgxLZ37yLGCUuS8rsFq5T5dvey9NAijP4TXyWkT2RYDsMq9PyJbMVzz19xAH121zA8RPbfczhDG5D4Dsay1gsbfDQTWcxpdbyhyfhXwCdPqQrMmSmQcQSMinNswtnSdWQ9dujrgs71WD44ibFBvJwyDntKSBvrtFqkWPnq2PKcvVMih4LXxzUBUuYRKy6vZTvrzNsjqowf5 regPool" },
		{ "Sacrificial Transaction", "cd /144c2Aof5t3GDUTv8x9mPorpzMLhkFQLWp sacrifice 14400000" },
	};

	// Open output file - use JSON format for JavaScript to parse
	std::filesystem::path tempDir = std::filesystem::temp_directory_path();
	std::filesystem::path outputPath = tempDir / "gridnet_cpp_bytecodes_for_js.json";
	std::ofstream outFile(outputPath.string());

	mTools->writeLine("Writing C++ bytecodes to: " + outputPath.string());

	outFile << "[\n";

	int compiled = 0, failed = 0;
	std::vector<uint8_t> bytecode;
	bool firstEntry = true;

	for (const auto& test : tests)
	{
		std::string source(test.source);

		if (mScriptEngine->getByteCode(source, bytecode))
		{
			// Encode to base64 for JavaScript testing
			std::string base64 = mTools->base64Encode(bytecode);

			// Write JSON entry
			if (!firstEntry) outFile << ",\n";
			firstEntry = false;
			outFile << "  {\n";
			outFile << "    \"name\": \"CPP: " << test.name << "\",\n";
			outFile << "    \"bytecode\": \"" << base64 << "\",\n";
			outFile << "    \"expected\": \"" << test.source << "\"\n";
			outFile << "  }";

			// Also verify C++ can decompile its own bytecode
			std::string decompiled = mScriptEngine->getSourceCode(bytecode);
			if (CTools::getTools()->doStringsMatch(source, decompiled))
			{
				compiled++;
			}
			else
			{
				mTools->writeLine("CPP EXPORT WARNING: C++ round-trip mismatch for: " + std::string(test.name));
				mTools->writeLine("  Source:     " + source);
				mTools->writeLine("  Decompiled: " + decompiled);
				failed++;
			}
		}
		else
		{
			mTools->writeLine("CPP EXPORT ERROR: Failed to compile: " + std::string(test.name));
			failed++;
		}
	}

	outFile << "\n]\n";
	outFile.close();

	mTools->writeLine("C++ Bytecode Export: " + std::to_string(compiled) + " compiled, " + std::to_string(failed) + " failed");
	mTools->writeLine("Output file: " + outputPath.string());

	return failed == 0;
}

bool CTests::testInSourceInvocationLookup()
{
	//TEST - BEGIN
	std::vector<std::string> source;

	std::vector<std::string> functionsToTest;
	functionsToTest = { "send","sendEx" };

	for (int i = 0; i < functionsToTest.size(); i++)
	{
		source.clear();
		std::string funcName = functionsToTest[i];

		SE::Xt xt = mScriptEngine->findDefinition(funcName);

		//stack parameters
		for (int i = 0; i < xt->reqStackWordsProceeding; i++)
			source.push_back(std::to_string(0));
		//security token parameters
		if (xt->requiresSecToken)
		{
			for (int i = 0; i < SEC_TOKEN_LENGTH; i++)
			{
				source.push_back("data58");
				source.push_back(std::to_string(0));

			}

		}
		//push the function name to source
		source.push_back(xt->name);

		//inline parameters
		for (int i = 0; i < xt->inlineParamCount; i++)
			source.push_back(std::to_string(0));

		std::vector<std::string> stacParams, secTokenParams, inlineParams;
		uint64_t beginsAt, endsAt;

		if (!mScriptEngine->findInvocation(source, funcName, stacParams, secTokenParams, inlineParams, beginsAt, endsAt))
			return false;

		if (stacParams.size() != xt->reqStackWordsProceeding)
			return false;

		if (xt->requiresSecToken && secTokenParams.size() != SEC_TOKEN_LENGTH)
			return false;
		if (inlineParams.size() != xt->inlineParamCount)
			return false;

	}

	return true; //test succeeded
	//TEST - END

}

bool CTests::automatedGridScriptTest()
{//
	mTools->writeLine("Entering an automated GridScript Test");

	std::string inScript("data64m [sig] data64m [pubKey]");
	std::string outScript("ultimium swap . data64m [hash] comp  . . versig");
	std::string conactScript = inScript + outScript;
	intptr_t result;
	size_t ERGused = 0;
	//assert(mScriptEngine->processScript(conactScript.c_str(), result, ERGused)==SE::CScriptEngine::eProsessingResult::pointless);
	mTools->writeLine("Automated GridScript Test completed.");
	return true;
}

bool CTests::settingsStorageTest()
{
	uint64_t rewardA = mSettings->getCurrentMiningReward();
	assertGN(mSettings->setCurrentMiningReward(rewardA + 1));
	uint64_t rewardB = mSettings->getCurrentMiningReward();
	if (rewardB == rewardA + 1)
		return true;
	else return false;
}

bool CTests::testHDKeys()
{

	//Local Variables - BEGIN
	uint32_t progress = 0;
	uint32_t count = 1000;
	int previousFlashed = 0;
	std::vector<uint8_t> packed;
	Botan::secure_vector<uint8_t> privKey, childPrivKey;
	std::vector<uint8_t> pubKey, childPubKey;
	//Local Variables - END
	Sleep(1000);
	mTools->writeLine("HD Wallet Key Tests , commencing..");


	//Serialization Tests - BEGIN

	//Test Flat keychains - BEGIN
	CKeyChain chain = CKeyChain(true, true);
	if (!chain.getFlags().flat)
		return false;
	chain.setID("FlatChain");
	packed = chain.getPackedData();
	chain.setIndex(50);//should have no effect
	chain.setUsedUpTillIndex(60);//should have no effect

	if (chain.getCurrentIndex() != 0)
		return false;
	if (chain.getUsedUptillIndex() != 0)
		return false;

	std::shared_ptr<CKeyChain> retrieved = CKeyChain::instantiate(packed);

	if ((*retrieved) != chain)
		return false;
	if (!retrieved->getFlags().flat)
		return false;

	//Test Flat keychains - END

	//Test Extended keychains - BEGIN
	chain = CKeyChain(true, false);
	if (chain.getFlags().flat)
		return false;

	chain.setIndex(50);
	chain.setUsedUpTillIndex(60);

	if (chain.getCurrentIndex() != 50)
		return false;
	if (chain.getUsedUptillIndex() != 60)
		return false;

	packed = chain.getPackedData();
	retrieved = CKeyChain::instantiate(packed);

	if ((*retrieved) != chain)
		return false;

	if (retrieved->getFlags().flat)
		return false;

	//Test Extended keychains - END

	//Serialization Tests - END

	//Usage, storage and retrieval from Settings - BEGIN

	//Test Flat-KeyChain utilization - BEGIN
	chain = CKeyChain(false, true);
	mCryptoFactory->genKeyPair(privKey, pubKey);
	chain.setMainPrivKey(privKey);
	Botan::secure_vector<uint8_t> retrievedKey = chain.getPrivKey();

	if (!mTools->compareByteVectors(retrievedKey, privKey))
		return false;

	chain.setIndex(10);//should have no effect at all

	if (!mTools->compareByteVectors(retrievedKey, privKey))
		return false;


	//Test Flat-KeyChain utilization - END	


	//test storage - BEGIN
	chain = CKeyChain(true, false);
	chain.setID("testHD");

	std::vector<uint8_t> domainID;

	if (mSettings != nullptr)
		mSettings->saveKeyChain(chain);
	mCryptoFactory->genAddress(chain.getPubKey(), domainID);
	std::vector<uint8_t> pubkey = chain.getPubKey();

	if (mSettings != nullptr)
	{
		Botan::secure_vector<uint8_t> chains = mSettings->getSerializedKeyChain(mTools->bytesToString(domainID));
		if (chains.size() == 0)
			return false;
		retrieved = CKeyChain::instantiate(Botan::unlock(chains));
		if ((*retrieved) != chain)
			return false;
	}
	//test storage - END

	//Test RAW child-key pair generation - BEGIN

	bool ok = mCryptoFactory->genKeyPair(privKey, pubKey);

	if (!ok)
		return false;

	for (int i = 0; i < count; i++)
	{
		progress = (int)(((double)i / (double)count) * 100);

		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("completed: " + std::to_string((int)progress) + "%");
		}

		if (!mCryptoFactory->getChildKeyPair(privKey, i, childPrivKey, childPubKey))
			return false;
	}

	//Test RAW child-key pair generation - END


	//Test HD-KeyChain utilization - BEGIN
	std::vector<uint8_t> data, sig;

	mTools->writeLine("HD Wallet Key's Signatures-Test , commencing..");
	for (int i = 0; i < count; i++)
	{
		progress = (int)(((double)i / (double)count) * 100);

		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("completed: " + std::to_string((int)progress) + "%");
		}

		data = mTools->genRandomVector(128);
		chain.setIndex(i);
		privKey = chain.getPrivKey();
		pubKey = chain.getPubKey();

		sig = mCryptoFactory->signData(data, privKey);
		if (!mCryptoFactory->verifySignature(sig, data, pubKey))
		{
			return false;
		}

		mTools->introduceRandomChange(data);

		if (mCryptoFactory->verifySignature(sig, data, pubKey))
		{
			return false;
		}
	}



	if (chain.getUsedUptillIndex() != count - 1)
		return false;
	//Test HD-KeyChain utilization - END	


	mTools->writeLine("Tests successful.");
	//usage, storage and retrieval from Settings - END
	return true;
}

/// <summary>
/// The test generates Key-Blocks, Regular-Blocks and Transactions.
/// Serialization of blocks/transactions and their retrieval is tested. 
/// Storage and retrieval from Cold Storage is tested also.
/// </summary>
/// <param name="nrOfKeyBlocks"></param>
/// <param name="nrOfRegularBlocks"></param>
/// <param name="nrOfTransactionsPerRegularBlock"></param>
/// <returns></returns>
bool CTests::testBlockchainBlocks(uint32_t nrOfKeyBlocks, uint32_t regBlocksPerKeyBlock, uint32_t nrOfTransactionsPerRegularBlock)
{
	//LOCAL Variables - START
	uint32_t currentNonce = 0;
	std::shared_ptr<CBlock>currentBlock;
	std::vector<uint8_t> masterPubKey25519RAW;
	std::vector<uint8_t> previousHash;
	std::vector<CTransaction> transactions;
	std::vector <std::vector <uint8_t>> transactionIDs;
	std::vector<uint8_t> parentBlockHash;
	std::vector<uint8_t> parentBlockDataHash;
	CTrieNode* transactionsRoot;
	std::vector<uint8_t> claimedPreviousHash;
	std::vector<uint8_t> retrievedBlockHash;
	std::vector<uint8_t> originalBlockHash;
	std::vector<uint8_t> originalPackedData;
	std::vector<uint8_t> originalHeaderData;
	std::vector<uint8_t> originalHeaderHash;
	std::vector<uint8_t> retrievedHeaderHash;
	std::vector<uint8_t> retrievedHeaderData;
	std::vector<uint8_t> originalTransactionRootId;
	std::vector<uint8_t> newTransactionRootId;
	std::vector<uint8_t> transactionRootID;
	std::vector<uint8_t> currentKeyBlockID;
	std::vector<uint8_t> currentKeyBlockPubKey;
	std::vector<uint8_t> p1, p2;
	std::shared_ptr<CBlock>  retrievedParent = nullptr;
	std::shared_ptr<CBlock>  retrievedBlock = nullptr;
	uint64_t nonce = 0;
	uint64_t diff = 0;
	uint32_t progress = 0;
	uint32_t iterationsMax = 0;
	std::vector<uint8_t> retrievedPackedData;
	//LOCAL Variables - END
	mTools->writeLine("\n\n\nCommencing with the Blockchain-Blocks Self-Test..", true, false);
	mTools->writeLine("Key blocks to generate: " + std::to_string(nrOfKeyBlocks));
	mTools->writeLine("Regular blocks per Key-Block: " + std::to_string(regBlocksPerKeyBlock));
	mTools->writeLine("Transactions per Regular-Block: " + std::to_string(nrOfTransactionsPerRegularBlock));

	//load the current StateDB use it for genesis block
	eBlockInstantiationResult::eBlockInstantiationResult bResult;
	//check for the latest block generate GenesisBlock is non present
	mBlockchainManager->getFormationFlowManager()->setFraudulantDataBlocksToGeneratePerKeyBlock(1);
	if (currentBlock == nullptr)//generate a Genesis block
		currentBlock = CBlock::newBlock(nullptr, bResult, eBlockchainMode::eBlockchainMode::LocalData, true);
	uint64_t lockPeriod = mBlockchainManager->getMinersRewardLockPeriod(currentBlock);

	mBlockchainManager->getFormationFlowManager()->setNewPubKeyEveryNKeyBlocks(2);//testing if resources will be freed after the lock-time
	mBlockchainManager->getFormationFlowManager()->setCycleBackIdentiesAfterNKeysUsed(10);//same as above

	CKeyChain chain;
	currentBlock->getHeader()->setPackedTarget(1);

	assertGN(mCryptoFactory->signBlock(currentBlock, chain.getPrivKey()));
	assertGN(mCryptoFactory->verifyBlockSignature(currentBlock));
	assertGN(mSS->saveBlock(currentBlock));


	eBlockInstantiationResult::eBlockInstantiationResult res;

	std::shared_ptr<CBlock>  rgb = mSS->getBlockByHash(currentBlock->getID(), res);
	assertGN(rgb != nullptr);

	//generate transactions
	std::vector<uint8_t> transactionsRootHash;
	mTools->writeLine("Generating test transactions..");
	std::vector<CKeyChain> chains;
	//these transactions will be put into the regular blocks
	iterationsMax = nrOfTransactionsPerRegularBlock * regBlocksPerKeyBlock * nrOfKeyBlocks;
	currentKeyBlockID = currentBlock->getID();
	for (int y = 0; y < iterationsMax; y++)
	{
		CTransaction trans;
		currentNonce++;
		Botan::secure_vector<uint8_t> sdPrivKey;
		CKeyChain chain;
		chains.push_back(chain);
		//mSettings->getCurrentKeyChain(mTools->bytesToString(ids[y]),chain,false);
		trans.sign(chain.getPrivKey());
		assertGN(trans.verifySignature(mTools->BERVector(chain.getPubKey())));
		transactions.push_back(trans);
		transactionIDs.push_back(trans.getHash());

		progress = (uint32_t)(((double)y / (double)iterationsMax) * 100);
		mTools->flashLine("Transactions generated: " + std::to_string((int)progress) + "%");
	}

	mTools->writeLine("Transactions generated: " + std::to_string((int)progress) + "%");
	mTools->writeLine("Generating test Blockchain Blocks.. (both key and regular)");
	mTools->writeLine("Integrity checks, blocks / transactions retrieval tests will commence now");

	//there'll be many reg. blocks per a single key-block
	for (uint64_t b = 1; b < nrOfKeyBlocks; b++)
	{
		nonce = mTools->genRandomNumber(1, 10000);
		diff = mTools->genRandomNumber(1, 10000);

		CKeyChain chain;//random key chain; simulating random leaders
		//the parent for the first Key-Block will be a nullptr
		currentBlock = CBlock::newBlock(currentBlock, bResult, eBlockchainMode::eBlockchainMode::LocalData, true);
		currentBlock->getHeader()->setKeyHeight(b);
		currentBlock->getHeader()->setParentKeyBlockID(currentKeyBlockID);
		currentBlock->getHeader()->setNonce(nonce);
		currentBlock->getHeader()->setPackedTarget(diff);

		assertGN(mCryptoFactory->signBlock(currentBlock, chain.getPrivKey()));
		assertGN(mCryptoFactory->verifyBlockSignature(currentBlock));
		//test key-block - BEGIN
		//test copy constructors
		std::vector<uint8_t> temp;
		assertGN(currentBlock->getHeader()->getPackedData(temp));
		std::vector<uint8_t> h1 = mCryptoFactory->getSHA2_256Vec(temp);
		CBlockHeader copied = CBlockHeader(const_cast<const CBlockHeader&>(*currentBlock->getHeader()));
		assertGN(copied.getPackedData(temp));
		std::vector<uint8_t> h11 = mCryptoFactory->getSHA2_256Vec(temp);

		//compare hash of both
		assertGN(std::memcmp(h1.data(), h11.data(), h1.size()) == 0);
		//in case hashing mechanism is compromised, test also few variables from within
		assertGN(nonce == copied.getNonce());
		assertGN(diff == copied.getPackedTarget());

		//save the newly created block to Solid Storage
		assertGN(mSS->saveBlock(currentBlock));
		currentKeyBlockID = currentBlock->getID();
		currentBlock->getPackedData(originalPackedData);
		originalHeaderHash = currentBlock->getHeader()->getHash();
		originalBlockHash = mCryptoFactory->getSHA2_256Vec(originalPackedData);
		//retrieve block
		retrievedBlock = mSS->getBlockByHash(currentBlock->getID(), res);
		assertGN(mBlockchainManager->wasBlockRetrievalSuccessful(res));
		assertGN(retrievedBlock != nullptr);

		//verify if the retrived block-header is exactly the same as the original one
		assertGN(retrievedBlock != nullptr);
		retrievedHeaderHash = retrievedBlock->getHeader()->getHash();

		assertGN(mTools->compareByteVectors(retrievedHeaderHash, originalHeaderHash));

		//verify if the retrieved block is exactly the same as the original one
		assertGN(retrievedBlock != nullptr);
		assertGN(retrievedBlock->getPackedData(retrievedPackedData));
		retrievedBlockHash = mCryptoFactory->getSHA2_256Vec(retrievedPackedData);

		assertGN(mTools->compareByteVectors(retrievedBlockHash, originalBlockHash));

		//test key-block - END
		currentKeyBlockID = currentBlock->getID();
		currentKeyBlockPubKey = currentBlock->getHeader()->getPubKey();
		//add some consecutive regular blocks to follow after the Key-Block
		for (int i = 0; i < regBlocksPerKeyBlock; i++)
		{
			progress = (uint32_t)(((double)i / (double)regBlocksPerKeyBlock) * 100);
			mTools->flashLine("Regular Blocks: " + std::to_string((int)progress) + "%");
			originalHeaderHash = currentBlock->getHeader()->getHash();

			//Calculate parent block hash

			//if (currentBlock->getParentPtr() != nullptr)
			parentBlockHash = currentBlock->getID();
			currentBlock->getPackedData(parentBlockDataHash);
			parentBlockDataHash = parentBlockDataHash = mCryptoFactory->getSHA2_256Vec(parentBlockDataHash);
			//create a new block with the previous one as parent and add some random signed transactions
			currentBlock = CBlock::newBlock(currentBlock, bResult, eBlockchainMode::eBlockchainMode::LocalData, false);
			transactionsRoot = currentBlock->getHeader()->getTransactionsDB()->getRoot();
			transactionsRootHash = transactionsRoot->getHash();

			//add transactions to the newly created regular block
			for (int y = 0; y < nrOfTransactionsPerRegularBlock; y++)
			{
				CTransaction t;
				int tI = (b > 0 ? ((b - 1) * regBlocksPerKeyBlock * nrOfTransactionsPerRegularBlock) : 0) + (i > 0 ? ((i - 1) * nrOfTransactionsPerRegularBlock) : 0) + y;
				assertGN(currentBlock->addTransaction(transactions[tI]));
				transactionRootID = currentBlock->getHeader()->getPerspective(eTrieID::transactions);
				//test transaction retrieval
				assertGN(currentBlock->getTransaction(transactions[tI].getHash(), t));

				//Note: do not compare contents of packed data directly(as follows) since
				//intermediary data (inter-data) within CTrieNode, with a partial ID might be present or not.
				//verify signatures instead.
				t.prepare(false, false);
				p1 = t.getRawData();
				transactions[tI].prepare(false, false);
				p2 = transactions[tI].getRawData();
				assertGN(p1.size() == p2.size() && std::memcmp(p1.data(), p2.data(), p1.size()) == 0);
				assertGN(t.verifySignature(chains[tI].getPubKey()));// chain.getPubKey()));
			}

			//sign the block already
			currentBlock->getHeader()->setParentKeyBlockID(currentKeyBlockID);
			assertGN(mCryptoFactory->signBlock(currentBlock, chain.getPrivKey()));
			assertGN(mCryptoFactory->verifyBlockSignature(currentBlock, currentKeyBlockPubKey));

			//test copy constructors
			assertGN(currentBlock->getHeader()->getPackedData(temp));
			h1 = mCryptoFactory->getSHA2_256Vec(temp);
			CBlockHeader copied = *currentBlock->getHeader();
			assertGN(copied.getPackedData(temp));
			h11 = mCryptoFactory->getSHA2_256Vec(temp);

			assertGN(std::memcmp(h1.data(), h11.data(), h1.size()) == 0);
			size_t countt = 0;
			currentBlock->getHeader()->getTransactionsDB()->testTrie(countt, false, false);
			currentBlock->getHeader()->setPerspective(mTools->genRandomVector(32), eTrieID::state);

			//save the newly created block to Solid Storage

			//mSS->addBlockToCache(currentBlock);
			//mBlockchainManager->setCurrentLeader(currentBlock);

			//retrieve block

			assertGN(mSS->saveBlock(currentBlock));
			retrievedBlock = mSS->getBlockByHash(currentBlock->getID(), res);
			assertGN(retrievedBlock != nullptr);
			//check if all the transactions are already there
			size_t reportedNrOfRetrievedTransactions = 0;
			currentBlock->getHeader()->getTransactionsDB()->testTrie(reportedNrOfRetrievedTransactions);
			assertGN(reportedNrOfRetrievedTransactions == nrOfTransactionsPerRegularBlock);

			originalTransactionRootId = currentBlock->getHeader()->getPerspective(eTrieID::transactions);
			newTransactionRootId = retrievedBlock->getHeader()->getPerspective(eTrieID::transactions);
			assertGN(mTools->compareByteVectors(newTransactionRootId, originalTransactionRootId));
			currentBlock->getHeader()->freeTries();
			CTransaction t1, t2, t;
			assertGN(currentBlock->getTransaction(currentBlock->getTransactionsIDs()[0], t1));

			retrievedBlock->getHeader()->freeTries();
			assertGN(retrievedBlock->getTransaction(currentBlock->getTransactionsIDs()[0], t2));

			for (int y = 0; y < nrOfTransactionsPerRegularBlock; y++)
			{
				int tI = (b > 0 ? ((b - 1) * regBlocksPerKeyBlock * nrOfTransactionsPerRegularBlock) : 0) + (i > 0 ? ((i - 1) * nrOfTransactionsPerRegularBlock) : 0) + y;
				assertGN(retrievedBlock->getTransaction(transactionIDs[tI], t));
				t.clearInterData();
				std::vector<uint8_t >t1Data = t.getPackedData();
				std::vector<uint8_t >t2Data = transactions[tI].getPackedData();
				assertGN(std::memcmp(t2Data.data(), t1Data.data(), t1Data.size()) == 0);
			}

			assertGN(retrievedBlock->getPackedData(retrievedPackedData));

			//retrieve previous parent block and try to reconsturct the new one based on unpacked transactions
			if (currentBlock->getHeader()->getHeight() > 0)
			{
				claimedPreviousHash = currentBlock->getHeader()->getParentID();//as claimed by child block

				assertGN(mTools->compareByteVectors(claimedPreviousHash, parentBlockHash));
				retrievedParent = mSS->getBlockByHash(currentBlock->getHeader()->getParentID(), res);
				assertGN(retrievedParent != nullptr);
				assertGN(retrievedParent->getPackedData(retrievedPackedData));

				//verify if the retrived parent block is exactly the same as the original one
				assertGN(retrievedParent != nullptr);
				assertGN(retrievedParent->getPackedData(retrievedPackedData));
				retrievedBlockHash = mCryptoFactory->getSHA2_256Vec(retrievedPackedData);
				assertGN(mTools->compareByteVectors(retrievedBlockHash, parentBlockDataHash));
			}
			retrievedParent = nullptr;

		}


	}
	mTools->writeLine("Key Blocks: " + std::to_string((int)progress) + "%");
	mTools->writeLine("Regular Blocks: " + std::to_string((int)progress) + "%");
	//traverse the chain
	std::shared_ptr<CBlock>current = currentBlock;
	int traversedBlocksCount = 0;
	uint32_t keyBlockCount = 0;
	uint32_t regBlocksCount = 0;
	uint32_t transactionsCount = 0;
	while (current != NULL)
	{
		traversedBlocksCount++;
		if (current->getHeader()->isKeyBlock())
			keyBlockCount++;
		else
		{
			regBlocksCount++;
			transactionsCount += current->getHeader()->getNrOfTransactions();
		}
		eBlockInstantiationResult::eBlockInstantiationResult res;
		current = current->getParent(res);


	}
	assertGN(traversedBlocksCount == regBlocksPerKeyBlock * nrOfKeyBlocks);

	mTools->writeLine("Test-Results:\n" + std::to_string(traversedBlocksCount) + " blocks in total\n" +
		std::to_string(keyBlockCount) + " key-blocks in total\n"
		+ std::to_string(regBlocksCount) + " regular-blocks in total\n"
		+ std::to_string(transactionsCount) + " transactions in total\n", true, false);

	mTools->writeLine("Blockchain Test completed.");

	return true;
}

bool CTests::testChatMsgs()
{
	std::shared_ptr<CChatMsg> cm = std::make_shared<CChatMsg>(eChatMsgType::text, CTools::getInstance()->genRandomVector(100), CTools::getInstance()->genRandomVector(100), CTools::getInstance()->genRandomVector(1000));
	std::vector<uint8_t> bytes = cm->getPackedData();
	std::shared_ptr<CChatMsg> cm2 = CChatMsg::instantiate(bytes);
	if (cm2 == nullptr)
		return false;

	if (*cm != *cm2)
	{
		return false;
	}

	return true;


}
/// <summary>
/// the test will threat already present nodes as accounts
/// </summary>
/// <returns></returns>
bool CTests::testAccountsStorage()
{

	assertGN(!mTransactionsManager->getIsInFlow());
	assertGN(!mStateDomainManager->isInFlow());
	assertGN(mTransactionsManager->setPerspective(mTransactionsManager->getPerspective(true)));
	//mTransactionsManager->st
	size_t iterations = 10;
	uint32_t progress = 0;
	uint32_t count = 0;
	CStateDomain* found = NULL;
	int previousFlashed = 0;
	bool hlsb;
	std::shared_ptr<CCryptoFactory>   cf = CCryptoFactory::getInstance();
	//C//Tools mTools;
	mTools->flashLine("Accounts storage test: " + std::to_string((int)progress) + "%");
	bool saveAsInteger = false;
	std::vector <std::vector<uint8_t>> domainIDS = mStateDomainManager->getKnownDomainIDs();
	if (domainIDS.size() == 0)
	{
		mTools->writeLine("No State Domains unable to start the account storage test.");
		return false;
	}
	bool testColdStorageCommits = true;
	std::vector<uint8_t> currentPerspective;
	std::vector<uint8_t> dataToSave;
	dataToSave.resize(sizeof(uint64_t));//64bit uint64_t

	std::vector < std::vector<uint8_t>> pastPerspectives;
	std::vector<uint8_t> currentDomainID;
	//data storage plus additional integrity  verifications
	assertGN(mTransactionsManager->startFlow());
	std::vector<std::vector<uint8_t>> processedDomains;
	for (uint64_t i = 0; i < iterations; i++)
	{

		currentDomainID = domainIDS[mTools->genRandomNumber(0, domainIDS.size() - 1)];
		processedDomains.push_back(currentDomainID);
		found = static_cast<CStateDomain*>(mStateDomainManager->findByID(currentDomainID));
		assertGN(found != NULL);
		std::memcpy(dataToSave.data(), &i, sizeof(i));

		//PROGRESS BAR  - START
		progress = (int)(((double)i / (double)iterations) * 100);

		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("Accounts storage test: " + std::to_string((int)progress) + "%");
		}
		//PROGRESS BAR - END

		currentPerspective = mStateDB->getPerspective();

		if (i % 2 == 0)
			saveAsInteger = true;
		else
			saveAsInteger = false;

		//validate integrity of a Trie-Node under which the State Domain is stored - START
		size_t r = ((CTrieBranchNode*)found->getParent())->getIndexOfPointer(*((CTrieNode**)(found->mPointerToPointerFromParent)));
		bool verRes = found->validateIntegrity(((CTrieBranchNode*)found->getParent())->getMemberHash(r));
		assertGN(verRes && found != NULL && found->getType() == 3 && found->getSubType() == 1);
		//validate integrity of a Trie-Node under which the State Domain is stored - END

		//fetch State-Domain credential from the local Security-Cache - START
		std::vector<uint8_t> pubKey;
		Botan::secure_vector<uint8_t> privKey;
		CKeyChain keyChain;
		assertGN(mSettings->getCurrentKeyChain(keyChain, false, true, mTools->bytesToString(((CStateDomain*)found)->getAddress())));
		assertGN(mTools->getSDCredentials(keyChain, ((CStateDomain*)found)->getAddress(), privKey, pubKey));
		//fetch State-Domain credential from the local Security-Cache - END

		//Modify the random State-Domain - START
		std::vector<uint8_t> newPerspective;
		currentPerspective = mTransactionsManager->getPerspective(true);
		uint64_t cost = 0;
		if (!saveAsInteger)
		{
			assertGN(found->saveValueDB(CAccessToken::genSysToken(), std::string("test"), dataToSave, eDataType::eDataType::bytes, newPerspective, cost));
		}
		else
		{
			assertGN(found->saveValueDB(CAccessToken::genSysToken(), std::string("test"), *(reinterpret_cast<uintptr_t*>(dataToSave.data())), eDataType::eDataType::unsignedInteger, newPerspective, cost));
		}
		currentPerspective = mTransactionsManager->getPerspective();
		found->incNonce(false);
		currentPerspective = mTransactionsManager->getPerspective();
		std::vector<uint8_t> resultingPerspective;
		found->changeBalanceBy(newPerspective, 10, false);
		currentPerspective = mTransactionsManager->getPerspective();
		assertGN(newPerspective.size() > 0);
		//Modify the random State-Domain - END

		currentPerspective = mTransactionsManager->getPerspective();
		assertGN(std::memcmp(newPerspective.data(), currentPerspective.data(), 32) == 0);
		//make sure the Transaction Manager reflects the same Perspective

		//TEST REMOVAL AND RETRIEVAL of a State-Domain from HotStorage and make sure it is properly restored afterwards - BEGIN
		found = static_cast<CStateDomain*>(mStateDB->findNodeByFullID(mTools->bytesToNibbles(currentDomainID)));
		pastPerspectives.push_back(newPerspective);
		std::vector<nibblePair> v = found->getTempFullPath();
		assertGN(found->validateIntegrity(((CTrieBranchNode*)found->getParent())->getMemberHash(r)));
		std::vector<uint8_t> toDebugOnly = ((CStateDomain*)found)->getStoragePerspective();
		assertGN(mStateDB->removeNode(mTools->bytesToNibbles(currentDomainID)));

		found = static_cast<CStateDomain*>(mStateDB->findNodeByFullID(mTools->bytesToNibbles(currentDomainID)));
		assertGN(found != NULL);
		//TEST REMOVAL AND RETRIEVAL -END

		toDebugOnly = ((CStateDomain*)found)->getStoragePerspective();
		assertGN(found->getType() == 3 && found->getSubType() == 1);
		eDataType::eDataType vl;
		std::vector<uint8_t> data = ((CStateDomain*)found)->loadValueDB(CAccessToken::genSysToken(), "test", vl);

		if (found != NULL && std::memcmp(data.data(), dataToSave.data(), dataToSave.size()) == 0)
		{
			count++;

		}
		else
		{
			mTransactionsManager->abortFlow();
			return false;
		}

	}
	assertGN(mTransactionsManager->endFlow(true, false, true) != CTransactionManager::eDBTransactionFlowResult::failure);

	//data retrieval. we will iterate through all the past perspectives.
	bool readAsInteger = false;
	found = static_cast<CStateDomain*>(mStateDB->findNodeByFullID(mTools->bytesToNibbles(currentDomainID)));
	mTools->writeLine("Trying to retrieve historical data from a random state domain. Testing also data types.");
	assertGN(mTransactionsManager->startFlow());
	mTools->writeLine("Testing State Domain data retrieval..");
	for (uint64_t i = 0; i < processedDomains.size(); i++)
	{
		//PROGRESS BAR  - START
		progress = (int)(((double)i / (double)iterations) * 100);

		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("Accounts storage test: " + std::to_string((int)progress) + "%");
		}
		//PROGRESS BAR - END

		currentDomainID = processedDomains[i];
		found = static_cast<CStateDomain*>(mStateDomainManager->findByID(currentDomainID));

		assertGN(mStateDomainManager->isInFlow());
		if (i % 2 == 0)
			readAsInteger = true;
		else
			readAsInteger = false;
		assertGN(found->setPerspective(pastPerspectives[i]));
		eDataType::eDataType dType;
		std::vector<uint8_t> retrievedData = found->loadValueDB(CAccessToken::genSysToken(), std::string("test"), dType);

		if (readAsInteger && (dType != eDataType::eDataType::unsignedInteger &&
			dType != eDataType::eDataType::signedInteger))
		{
			mStateDomainManager->exitFlow();
			return false;
		}
		if (!readAsInteger && (dType != eDataType::eDataType::bytes))
		{
			mStateDomainManager->exitFlow();
			return false;
		}

		if (!std::memcmp(&i, retrievedData.data(), sizeof(i)) == 0)
		{
			mStateDomainManager->exitFlow();
			return false;
		}


	}
	assertGN(mTransactionsManager->endFlow(true, false, true) != CTransactionManager::eDBTransactionFlowResult::failure);
	progress = 100;
	mTools->flashLine("Accounts storage test: " + std::to_string((int)progress) + "%");

	if (progress == 100)
	{
		mTools->writeLine("GOOD.");
		mStateDomainManager->exitFlow();
		return true;
	}
	else {
		mStateDomainManager->exitFlow();
		return false;
	}
}

bool CTests::testFlows()
{
	bool result = true;
	mTools->writeLine("Data Flows Test Commencing..");
	uint32_t progress = 0;
	uint32_t iterations = 1000;
	uint32_t perFlowInterations = 10;
	uint32_t previousFlashed = 0;
	uint32_t pruneTrieDBseveryRounds = 3;
	uint32_t nrOfNewDomainsPerRound = 10;
	std::vector<uint8_t>currentDomainID;
	std::vector <std::vector<uint8_t>> domainIDS = mStateDomainManager->getKnownDomainIDs();
	std::vector<uint8_t>previousFlowPerspective;
	uint32_t roundsWithoutPruining = 0;
	std::vector<uint8_t> inFlowPerspectiveA, inFlowPerspectiveB;
	size_t liveDomainCountB = mStateDomainManager->getKnownDomainsCount();
	size_t liveDomainCountA = liveDomainCountB;
	previousFlowPerspective = mTransactionsManager->getPerspective();
	for (uint64_t i = 0; i < iterations; i++)
	{
		domainIDS = mStateDomainManager->getKnownDomainIDs();
		std::vector<uint8_t> initialPerspective = previousFlowPerspective;

		assertGN(mTransactionsManager->setPerspective(previousFlowPerspective));
		//size_t count = 0;
		mTransactionsManager->getLiveDB()->testTrie(liveDomainCountB, false, false);
		mTransactionsManager->getLiveDB()->testTrie(liveDomainCountB);
		if (i > 0)
		{
			if (!(liveDomainCountB == (liveDomainCountA + nrOfNewDomainsPerRound)))
				assertGN(false);
			if (!(mStateDomainManager->getKnownDomainsCount() == liveDomainCountB))
				assertGN(false);
		}

		if (false)
		{//jump to this section on detected error to replay the faulty events
			assertGN(mTransactionsManager->setPerspective(initialPerspective));
			assertGN(mTransactionsManager->startFlow(initialPerspective));

		}
		assertGN(mTransactionsManager->startFlow(previousFlowPerspective));
		std::vector < std::vector<uint8_t>> toBeAddedDomains;

		CStateDomain* sd = nullptr;

		toBeAddedDomains.clear();
		for (int o = 0; o < nrOfNewDomainsPerRound; o++)
		{
			std::vector<uint8_t> adr;
			CKeyChain chain;
			mCryptoFactory->genAddress(chain.getPubKey(), adr);
			mSettings->saveKeyChain(chain.getPackedData(), std::string(adr.begin(), adr.end()));
			toBeAddedDomains.push_back(adr);
		}


		for (int u = 0; u < nrOfNewDomainsPerRound; u++)
		{
			size_t currentCount = 0;
			mTransactionsManager->getLiveDB()->pruneTrie();
			//mTransactionsManager->getLiveDB()->testTrie(liveDomainCountB,false,false);
			std::vector<uint8_t> liveRootHash1 = mTransactionsManager->getLiveDB()->getRoot()->getHash();
			assertGN(mStateDomainManager->create(inFlowPerspectiveA, &sd, false, toBeAddedDomains[u]));
			std::vector<uint8_t> liveRootHash2 = mTransactionsManager->getLiveDB()->getRoot()->getHash();
			assertGN(std::memcmp(liveRootHash1.data(), liveRootHash2.data(), 32) == 0);

			mTransactionsManager->getLiveDB()->testTrie(liveDomainCountB);//this should not break the Live perspective should NOT be affected at this time
			mTransactionsManager->getStateDomainManager()->getDB()->testTrie(currentCount);
			if ((mStateDomainManager->getKnownDomainsCount(true) != (liveDomainCountB + (u + 1))))
			{
				assertGN(false);
			}
			if (currentCount != (liveDomainCountB + (u + 1)))
			{
				assertGN(false);
			}
		}

		roundsWithoutPruining++;
		if (roundsWithoutPruining > pruneTrieDBseveryRounds)
		{
			mTransactionsManager->getFlowDB()->pruneTrie();
			mTransactionsManager->getLiveDB()->pruneTrie();
			roundsWithoutPruining = 0;
		}

		//PROGRESS BAR  - START
		progress = (uint32_t)(((double)i / (double)iterations) * 100);

		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("Flows Test progress: " + std::to_string((int)progress) + "%");
		}



		for (uint64_t y = 0; y < perFlowInterations; y++)
		{

			inFlowPerspectiveA = mTransactionsManager->getPerspective();
			currentDomainID = domainIDS[domainIDS.size() > 1 ? (mTools->genRandomNumber(0, domainIDS.size() - 1)) : 0];
			CStateDomain* sd = mStateDomainManager->findByID(currentDomainID);
			assertGN(sd != nullptr);
			sd->incNonce();
			std::vector<uint8_t> fp;
			sd->changeBalanceBy(fp, 100);
			inFlowPerspectiveB = mTransactionsManager->getPerspective();
			assertGN(std::memcmp(inFlowPerspectiveA.data(), inFlowPerspectiveB.data(), 32) != 0);
		}

		assertGN(mTransactionsManager->endFlow(true, false, true) == 0);
		if (i > 0)
			assertGN(std::memcmp(mTransactionsManager->getPerspective(true).data(), previousFlowPerspective.data(), 32) != 0);
		previousFlowPerspective = mTransactionsManager->getPerspective(true);
		assertGN(std::memcmp(inFlowPerspectiveB.data(), previousFlowPerspective.data(), 32) == 0);

		mTransactionsManager->getLiveDB()->testTrie(liveDomainCountA, true);
	}
	mTools->writeLine("Data Flows Test Finished.");
	return result;
}
bool CTests::testGridScriptParser() {
	try {
		// Test Case 1: Basic send command with atto units
		{
			std::string sourceCode = "cd 1Wpnr7qPtSUoehb6VjjHwfvmZhw8JELrT8 send 1Z1Y0uuyyk6n74XKvbN8vMQYQ68NFek15i 342023969671960473600";
			auto result = parseSendCommand(sourceCode);

			if (!result->isSuccess() ||
				result->getRecipient() != "1Z1Y0uuyyk6n74XKvbN8vMQYQ68NFek15i" ||
				result->getAmount() != BigInt("342023969671960473600")) {
				return false;
			}
		}

		// Test Case 2: Send command with -gnc flag
		{
			std::string sourceCode = "send Johny 1 -gnc";
			auto result = parseSendCommand(sourceCode);
			auto tools = CTools::getInstance();
			BigInt expectedAmount = tools->GNCToAtto(1.0);

			if (!result->isSuccess() ||
				result->getRecipient() != "Johny" ||
				result->getAmount() != expectedAmount) {
				return false;
			}
		}

		// Test Case 3: Complex sendEx command with stack operations
		{
			std::string sourceCode = "cd '/12otW4mryynPqEyjeBoTQfPE7bMVQTCfHY/' 1 data 1DTHi1CfHiPYWbr8hAKUQdCQU9RqcPqk6E 2041740000000000000000 sendEx cd //\"\"";

			// Debug the stack contents
			std::cout << "Testing sendEx command:" << std::endl;
			std::cout << "Source code: " << sourceCode << std::endl;

			auto result = parseSendCommand(sourceCode);

			std::cout << "Result success: " << result->isSuccess() << std::endl;
			std::cout << "Result recipient: " << result->getRecipient() << std::endl;
			std::cout << "Result amount: " << result->getAmount().str() << std::endl;

			if (!result->isSuccess() ||
				result->getRecipient() != "1DTHi1CfHiPYWbr8hAKUQdCQU9RqcPqk6E" ||
				result->getAmount() != BigInt("2041740000000000000000")) {
				std::cout << "Test case 3 failed!" << std::endl;
				return false;
			}
		}

		// Test Case 4: Non-send command (should return false success)
		{
			std::string sourceCode = "context -c getTransactionDetails 4kGftx4MCE7QbLdpYp1HATMMYYxza63jPi8odKoHdRULgnWqwS";
			auto result = parseSendCommand(sourceCode);

			if (result->isSuccess()) {
				return false;
			}
		}

		// Test Case 5: Send command with multiple flags
		{
			std::string sourceCode = "send 1DTHi1CfHiPYWbr8hAKUQdCQU9RqcPqk6E 5.5 -gnc -notax";
			auto result = parseSendCommand(sourceCode);
			auto tools = CTools::getInstance();
			BigInt expectedAmount = tools->GNCToAtto(5.5);

			if (!result->isSuccess() ||
				result->getRecipient() != "1DTHi1CfHiPYWbr8hAKUQdCQU9RqcPqk6E" ||
				result->getAmount() != expectedAmount) {
				return false;
			}
		}

		// Test Case 6: Invalid amount format
		{
			std::string sourceCode = "send 1DTHi1CfHiPYWbr8hAKUQdCQU9RqcPqk6E abc";
			auto result = parseSendCommand(sourceCode);

			if (result->isSuccess()) {
				return false;
			}
		}

		// Test Case 7: Missing amount
		{
			std::string sourceCode = "send 1DTHi1CfHiPYWbr8hAKUQdCQU9RqcPqk6E";
			auto result = parseSendCommand(sourceCode);

			if (result->isSuccess()) {
				return false;
			}
		}

		// Test Case 8: Send command with decimal atto (should fail)
		{
			std::string sourceCode = "send 1DTHi1CfHiPYWbr8hAKUQdCQU9RqcPqk6E 100.5";
			auto result = parseSendCommand(sourceCode);

			if (result->isSuccess()) {
				return false;
			}
		}

		// All tests passed
		return true;

	}
	catch (const std::exception& e) {
		// Log the error if needed
		return false;
	}
}

/// <summary>
/// The aim of the test is to test handling of situations where node is way behind the rest of the network.
/// 
/// Initial Requirements: the minial LENGTH of blockchain (mVerifiedChainProof > minLength).
/// As opposed to other tests concerned with forks, this test verifies  proper handling of chain-proofs.
/// 
/// Procedure: 
/// 1)The Blockchain Manager will halt and block production will cease for ceaseProductionTime, also,  minLength-1 blocks will be removed from recent blocks.
/// [Note 1]: Thus, the leader WOULD change (to one of the previous ones). All of the removed blocks would be still present in Cold Storage.
/// 2)The mHeaviestChainProof would be reset to what is available in mVerifiedChainProof
/// 3)Each of the removed blocks will be marked as delayedForProcessing with exponential delays.
/// 4)the Blockchain Manager will be constantly trying to verify blocks described by mHeaviestChainProof once they are made available (the 
/// hold-back timeouts expire) by calling commitHeaviestChainProofBlocks (that's normal)
/// 5) The tests succeeds once the mVerifiedChainProof becomes mHeaviestChainProof and the initial leader is restored.
/// 
/// Conditionale: If the ceaseProductionTime is not high enough the local node might win over the prevoius chain/ history of events and thus the tests would fail
/// (though everything would be going more than fine).
/// 
/// </summary>
/// <returns></returns>
bool CTests::testDeepForksLocal()
{
	//LOCAL VARIABLES - BEGIN (inc. settins)
	bool affectColdStorage = true;
	uint64_t deRouteKeyBlocksNr = 3;
	uint64_t initialCommitedCount = 0;
	uint64_t currentCommitedCount = 0;
	uint64_t deRouteEachBlockFor = 30;//each block (key- or data) would be exponentially delayed for this ammount of time in sec.
	std::vector<uint8_t> initialPerspective, finalPerspective;
	uint64_t ceaseProductionTime = CGlobalSecSettings::getTargetedBlockInterval() * (deRouteKeyBlocksNr + 2);
	//LOCAL VARIABLES - END

	//CHECK INITIAL-REQUIREMENTS - BEGIN
	if (mBlockchainManager->getDepth() < deRouteKeyBlocksNr)
	{
		mTools->writeLine("The test cannot commence - not enough blocks.");
		return false;
	}
	//CHECK INITIAL-REQUIREMENTS - END

	mBlockchainManager->enterMaintenanceMode();
	mBlockchainManager->setDefaultDeRouteDelay(deRouteEachBlockFor);
	initialPerspective = mBlockchainManager->getPerspective();
	initialCommitedCount = mBlockchainManager->getCommitedHeaviestChainProofBlocksCount();
	mBlockchainManager->deRouteBlocksToHeaviestChain(deRouteKeyBlocksNr, true, affectColdStorage);
	mBlockchainManager->getFormationFlowManager()->haltBlockProductionFor(ceaseProductionTime);
	mBlockchainManager->exitMaintenanceMode();//work would resume

	//wait for the Blockchain Manager to do its work

	while (mBlockchainManager->getCommitedHeaviestChainProofBlocksCount() < (initialCommitedCount + deRouteKeyBlocksNr))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	//check the final result. Does the global state of the decentralzed-state machine match in every bit the a-priori state?
	finalPerspective = mBlockchainManager->getPerspective();
	if (mTools->compareByteVectors(initialPerspective, finalPerspective))
	{
		mTools->writeLine("The deep-forks tests SUCCEEDED!");
		return true;
	}
	else
	{
		mTools->writeLine("The deep-forks tests FAILED.");
		return false;
	}
}


bool CTests::testExclusiveWorkerMutex() {
	try {
		bool allTestsPassed = true;
		size_t totalTests = 0;
		size_t passedTests = 0;

		// Enhanced test recording with logging
		auto recordTest = [&](bool result, const char* testName) {
			totalTests++;
			if (result) {
				passedTests++;
				printf("[PASS] %s\n", testName);
			}
			else {
				printf("[FAIL] %s\n", testName);
			}
			allTestsPassed &= result;
			return result;
			};

		ExclusiveWorkerMutex mutex;

		// Atomic variables to track counters
		std::atomic<int> shared_counter{ 0 };
		std::atomic<int> exclusive_counter{ 0 };

		// Stage 1: Multiple authorized threads acquiring shared locks
		{
			printf("\nStage 1: Multiple authorized threads acquiring shared locks\n");

			bool stagePassed = true;

			std::vector<std::thread> threads;
			std::atomic<int> success_count{ 0 };

			auto shared_task = [&](int thread_id) {
				try {
					mutex.authorize_thread(std::this_thread::get_id());
					std::shared_lock<ExclusiveWorkerMutex> lock(mutex);

					// Simulate work
					shared_counter++;
					std::this_thread::sleep_for(std::chrono::milliseconds(100));

					// Check that no exclusive locks are held
					if (exclusive_counter.load() > 0) {
						stagePassed = false;
					}

					shared_counter--;
					success_count++;
				}
				catch (...) {
					stagePassed = false;
				}
				};

			for (int i = 0; i < 3; ++i) {
				threads.emplace_back(shared_task, i);
			}

			// Wait for threads to finish
			for (auto& t : threads) {
				t.join();
				mutex.unauthorize_thread(t.get_id());
			}

			bool result = stagePassed && (success_count.load() == 3) && (shared_counter.load() == 0);
			recordTest(result, "Authorized threads acquiring shared locks");
		}

		// Stage 2: Unauthorized thread attempting to acquire shared lock when conditions allow
		{
			printf("\nStage 2: Unauthorized thread automatically authorized for shared lock\n");

			bool stagePassed = true;
			std::atomic<bool> shared_lock_acquired{ false };
			std::atomic<bool> is_authorized_before{ false };
			std::atomic<bool> is_authorized_during{ false };
			std::atomic<bool> is_authorized_after{ false };

			auto shared_task = [&]() {
				try {
					// Before acquiring the lock, check if the thread is authorized
					is_authorized_before = mutex.is_current_thread_authorized();

					{
						std::shared_lock<ExclusiveWorkerMutex> lock(mutex);

						// Shared lock acquired successfully
						shared_lock_acquired = true;

						// During the lock, check if the thread is authorized
						is_authorized_during = mutex.is_current_thread_authorized();

						// Simulate work
						std::this_thread::sleep_for(std::chrono::milliseconds(50));

						// Lock is released here
					}

					// After releasing the lock, check if the thread is still authorized
					is_authorized_after = mutex.is_current_thread_authorized();

					// Stage passes if:
					// - The thread was not authorized before the lock
					// - The thread was authorized during the lock
					// - The thread is not authorized after unlocking
					if (is_authorized_before.load()) {
						printf("    Error: Thread should not be authorized before lock\n");
						stagePassed = false;
					}
					if (!is_authorized_during.load()) {
						printf("    Error: Thread should be authorized during lock\n");
						stagePassed = false;
					}
					if (is_authorized_after.load()) {
						printf("    Error: Thread should not be authorized after unlock\n");
						stagePassed = false;
					}
				}
				catch (...) {
					// Unexpected exception
					stagePassed = false;
				}
				};

			std::thread t(shared_task);
			t.join();

			bool result = stagePassed && shared_lock_acquired.load();
			recordTest(result, "Unauthorized thread automatically authorized for shared lock");
		}

		// Stage 3: Authorized thread acquiring exclusive lock
		{
			printf("\nStage 3: Authorized thread acquiring exclusive lock\n");

			bool stagePassed = true;
			std::atomic<bool> exclusive_acquired{ false };

			auto exclusive_task = [&]() {
				try {
					mutex.authorize_thread(std::this_thread::get_id());
					std::lock_guard<ExclusiveWorkerMutex> lock(mutex);

					// Simulate work
					exclusive_counter++;
					exclusive_acquired = true;
					std::this_thread::sleep_for(std::chrono::milliseconds(100));

					// Check that no shared locks are held
					if (shared_counter.load() > 0) {
						stagePassed = false;
					}

					exclusive_counter--;
				}
				catch (...) {
					stagePassed = false;
				}
				};

			std::thread t(exclusive_task);
			t.join();

			bool result = stagePassed && exclusive_acquired.load() && (exclusive_counter.load() == 0);
			recordTest(result, "Authorized thread acquiring exclusive lock");

			// Clean up authorization
			mutex.unauthorize_thread(std::this_thread::get_id());
		}

		// Stage 4: Unauthorized thread acquiring exclusive lock
		{
			printf("\nStage 4: Unauthorized thread acquiring exclusive lock\n");

			bool stagePassed = true;
			std::atomic<bool> exclusive_acquired{ false };

			auto exclusive_task = [&]() {
				try {
					std::lock_guard<ExclusiveWorkerMutex> lock(mutex);

					// Simulate work
					exclusive_counter++;
					exclusive_acquired = true;
					std::this_thread::sleep_for(std::chrono::milliseconds(100));

					// Check that no shared locks are held
					if (shared_counter.load() > 0) {
						stagePassed = false;
					}

					exclusive_counter--;
				}
				catch (...) {
					stagePassed = false;
				}
				};

			std::thread t(exclusive_task);
			t.join();

			bool result = stagePassed && exclusive_acquired.load() && (exclusive_counter.load() == 0);
			recordTest(result, "Unauthorized thread acquiring exclusive lock");
		}

		// Stage 5: Recursive locking by authorized thread
		{
			printf("\nStage 5: Recursive locking by authorized thread\n");

			bool stagePassed = true;
			std::atomic<int> recursion_depth{ 0 };

			auto recursive_task = [&]() {
				try {
					mutex.authorize_thread(std::this_thread::get_id());

					// First lock
					mutex.lock();
					recursion_depth++;

					// Second lock (recursive)
					mutex.lock();
					recursion_depth++;

					// Simulate work
					std::this_thread::sleep_for(std::chrono::milliseconds(100));

					// Unlock twice
					mutex.unlock();
					recursion_depth--;
					mutex.unlock();
					recursion_depth--;

					if (recursion_depth.load() != 0) {
						stagePassed = false;
					}
				}
				catch (...) {
					stagePassed = false;
				}
				};

			std::thread t(recursive_task);
			t.join();

			bool result = stagePassed && (recursion_depth.load() == 0);
			recordTest(result, "Recursive locking by authorized thread");

			// Clean up authorization
			mutex.unauthorize_thread(std::this_thread::get_id());
		}

		// Stage 6: Temporary authorization handling
		{
			printf("\nStage 6: Temporary authorization handling\n");

			bool stagePassed = true;
			std::atomic<bool> temp_authorized{ false };
			std::atomic<bool> unauthorized_after_unlock{ false };

			auto shared_task = [&]() {
				try {
					// Before lock
					bool authorized_before = mutex.is_current_thread_authorized();

					{
						// Acquire shared lock without prior authorization
						std::shared_lock<ExclusiveWorkerMutex> lock(mutex);

						// During lock
						temp_authorized = mutex.is_current_thread_authorized();

						// Simulate work
						std::this_thread::sleep_for(std::chrono::milliseconds(50));

						// Lock is released here

						// After lock
					}
						unauthorized_after_unlock = !mutex.is_current_thread_authorized();
					

					// Stage passes if:
					// - The thread was not authorized before the lock
					// - The thread is authorized during the lock
					// - The thread is not authorized after unlocking
					if (authorized_before) {
						printf("    Error: Thread should not be authorized before lock\n");
						stagePassed = false;
					}
					if (!temp_authorized.load()) {
						printf("    Error: Thread should be authorized during lock\n");
						stagePassed = false;
					}
					if (!unauthorized_after_unlock.load()) {
						printf("    Error: Thread should not be authorized after unlock\n");
						stagePassed = false;
					}
				}
				catch (...) {
					stagePassed = false;
				}
				};

			std::thread t1(shared_task);
			t1.join();

			bool result = stagePassed && temp_authorized.load() && unauthorized_after_unlock.load();
			recordTest(result, "Temporary authorization during shared lock");
		}

		// Stage 7: Edge case tests
		{
			printf("\nStage 7: Edge case tests\n");

			bool stagePassed = true;

			// Attempt to unlock without holding a lock
			try {
				mutex.unlock();
				// Should not reach here
				stagePassed = false;
				printf("    Error: Unlocking without holding a lock did not throw an exception\n");
			}
			catch (const std::system_error& e) {
				// Expected exception
			}
			catch (...) {
				stagePassed = false;
			}

			// Attempt to unlock_shared without holding a shared lock
			try {
				mutex.unlock_shared();
				// Should not reach here
				stagePassed = false;
				printf("    Error: Unlocking shared without holding a shared lock did not throw an exception\n");
			}
			catch (const std::system_error& e) {
				// Expected exception
			}
			catch (...) {
				stagePassed = false;
			}

			uint64_t successfullLocks = 0;
			// Attempt to lock recursively without authorization
			try {
				// First lock (unauthorized)
				mutex.lock();
				successfullLocks++;
				// Second lock (recursive)
				mutex.lock();
				successfullLocks++;
				// Should not reach here
				stagePassed = true;
		
			}
			catch (const std::system_error& e) {
				printf("    Error: Unauthorized recursive lock failed.\n");
				mutex.unlock(); // Unlock the first lock
			}
			catch (...) {
				stagePassed = false;
			}
			for (uint64_t i = 0; i < successfullLocks; i++)
			{
				try {
					mutex.unlock();
				}
				catch (const std::system_error& e)
				{
					printf("    Error: Failed to unlock mutex after recursive lock. \n");
				}
			}

			// Verify that the mutex is not locked
			if (mutex.is_locked()) {
				stagePassed = false;
				printf("    Error: Mutex should be unlocked but is locked\n");
			}

			recordTest(stagePassed, "Edge case tests");
		}

		// Stage 8: Unauthorized owner acquiring shared lock while holding exclusive lock
		{
			printf("\nStage 8: Unauthorized owner acquiring shared lock while holding exclusive lock\n");

			bool stagePassed = true;
			std::atomic<bool> shared_lock_acquired{ false };
			std::atomic<bool> exclusive_lock_acquired{ false };
			std::atomic<bool> other_thread_blocked{ false };

			ExclusiveWorkerMutex mutex;

			// Thread that will attempt to acquire locks while unauthorized owner holds locks
			auto competing_thread_task = [&]() {
				try {
					// Attempt to acquire lock_shared(), should block until unauthorized owner releases locks
					if (mutex.try_lock_shared() == true)
					{
						// If we reach here before unauthorized owner releases locks, it's an error
						other_thread_blocked = false;
					}
				}
				catch (...) {
					stagePassed = false;
				}
				};

			auto unauthorized_thread_task = [&]() {
				try {
					// Acquire exclusive lock
					mutex.lock();
					exclusive_lock_acquired = true;

					// Start the competing thread after acquiring exclusive lock
					std::thread competing_thread(competing_thread_task);
					std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Ensure competing thread attempts to acquire lock

					// Attempt to acquire shared lock while holding exclusive lock
					mutex.lock_shared();
					shared_lock_acquired = true;

					// Simulate work
					std::this_thread::sleep_for(std::chrono::milliseconds(500));

					// Release shared lock
					mutex.unlock_shared();

					// Release exclusive lock
					mutex.unlock();

					// Wait for competing thread to finish
					competing_thread.join();
				}
				catch (...) {
					stagePassed = false;
				}
				};

			// Initially, set other_thread_blocked to true
			other_thread_blocked = true;

			std::thread t(unauthorized_thread_task);
			t.join();

			// Verify that the competing thread was indeed blocked during unauthorized owner's locks
			bool result = stagePassed && exclusive_lock_acquired.load() && shared_lock_acquired.load() && other_thread_blocked.load();
			recordTest(result, "Unauthorized owner acquiring shared lock while holding exclusive lock");
		}

		// Stage 9: Explicit Authorization Mode Tests
		{
			printf("\nStage 9: Explicit Authorization Mode Tests\n");

			ExclusiveWorkerMutex explicitMutex(false, true);  // requires explicit authorization
			bool stagePassed = true;
			std::atomic<bool> unauthorized_attempt_blocked{ false };
			std::atomic<bool> authorized_attempt_succeeded{ false };

			// Test unauthorized attempt
			auto unauthorized_task = [&]() {
				try {
					std::shared_lock<ExclusiveWorkerMutex> lock(explicitMutex);
					// Should not reach here
					stagePassed = false;
					printf("    Error: Unauthorized thread acquired shared lock in explicit mode\n");
				}
				catch (const std::system_error&) {
					// Expected exception
					unauthorized_attempt_blocked = true;
				}
				catch (...) {
					stagePassed = false;
					printf("    Error: Unexpected exception type\n");
				}
				};

			// Test authorized attempt
			auto authorized_task = [&]() {
				try {
					ExclusiveWorkerAuthorization auth(explicitMutex);
					std::shared_lock<ExclusiveWorkerMutex> lock(explicitMutex);
					authorized_attempt_succeeded = true;
				}
				catch (...) {
					stagePassed = false;
					printf("    Error: Authorized thread failed to acquire shared lock\n");
				}
				};

			std::thread t1(unauthorized_task);
			t1.join();
			std::thread t2(authorized_task);
			t2.join();

			bool result = stagePassed && unauthorized_attempt_blocked && authorized_attempt_succeeded;
			recordTest(result, "Explicit authorization mode behavior");
		}

		// Stage 10: Creating Thread Special Status
		{
			printf("\nStage 10: Creating Thread Special Status Tests\n");

			bool stagePassed = true;
			std::atomic<bool> shared_lock_acquired{ false };
			std::atomic<bool> authorization_persisted{ false };

			auto creator_task = [&]() {
				try {
					// Create mutex in this thread and authorize it
					ExclusiveWorkerMutex creatorMutex(true, false);

					// Verify initial authorization
					if (!creatorMutex.is_current_thread_authorized()) {
						printf("    Error: Creating thread not initially authorized\n");
						stagePassed = false;
						return;
					}

					// Test shared lock acquisition without explicit authorization
					{
						std::shared_lock<ExclusiveWorkerMutex> lock(creatorMutex);
						shared_lock_acquired = true;
					}

					// Verify authorization persists after lock release
					authorization_persisted = creatorMutex.is_current_thread_authorized();

				}
				catch (...) {
					stagePassed = false;
					printf("    Error: Unexpected exception in creating thread\n");
				}
				};

			std::thread t(creator_task);
			t.join();

			bool result = stagePassed && shared_lock_acquired && authorization_persisted;
			recordTest(result, "Creating thread special status");
		}

		// Stage 11: Mixed Lock Mode Transition Tests
		{
			printf("\nStage 11: Lock Mode Transition Tests\n");

			bool stagePassed = true;
			std::atomic<bool> shared_to_exclusive_transition{ false };
			std::atomic<bool> exclusive_to_shared_transition{ false };

			ExclusiveWorkerMutex mutex;
			mutex.authorize_thread(std::this_thread::get_id());  // Authorize for shared locks

			auto transition_task = [&]() {
				try {
					// Test shared to exclusive transition
					{
						std::shared_lock<ExclusiveWorkerMutex> shared_lock(mutex);
						shared_lock.unlock();

						std::lock_guard<ExclusiveWorkerMutex> exclusive_lock(mutex);
						shared_to_exclusive_transition = true;
					}

					// Test exclusive to shared transition
					{
						std::lock_guard<ExclusiveWorkerMutex> exclusive_lock(mutex);
						{
							std::shared_lock<ExclusiveWorkerMutex> shared_lock(mutex);
							exclusive_to_shared_transition = true;
						}
					}
				}
				catch (...) {
					stagePassed = false;
					printf("    Error: Exception during lock mode transition\n");
				}
				};

			std::thread t(transition_task);
			t.join();

			bool result = stagePassed && shared_to_exclusive_transition && exclusive_to_shared_transition;
			recordTest(result, "Lock mode transitions");
		}

		// Stage 12: Stress Test with Mixed Operations
		{
			printf("\nStage 12: Stress Test with Mixed Operations\n");

			bool stagePassed = true;
			ExclusiveWorkerMutex mutex;
			std::atomic<int> shared_access_count{ 0 };
			std::atomic<int> exclusive_access_count{ 0 };
			std::atomic<bool> stop_flag{ false };
			std::atomic<int> error_count{ 0 };

			auto stress_worker = [&](int id) {
				try {
					for (int i = 0; i < 100 && error_count == 0; ++i) {
						switch (id % 3) {
						case 0: {  // Exclusive lock attempts
							std::lock_guard<ExclusiveWorkerMutex> lock(mutex);
							if (shared_access_count > 0) {
								error_count++;
								printf("    Error: Exclusive access with shared holders\n");
							}
							exclusive_access_count++;
							std::this_thread::sleep_for(std::chrono::milliseconds(1));
							exclusive_access_count--;
							break;
						}
						case 1: {  // Shared lock attempts
							ExclusiveWorkerAuthorization auth(mutex);
							std::shared_lock<ExclusiveWorkerMutex> lock(mutex);
							if (exclusive_access_count > 0) {
								error_count++;
								printf("    Error: Shared access with exclusive holder\n");
							}
							shared_access_count++;
							std::this_thread::sleep_for(std::chrono::milliseconds(1));
							shared_access_count--;
							break;
						}
						case 2: {  // Mixed operations
							if (mutex.try_lock()) {
								mutex.unlock();
							}
							if (mutex.try_lock_shared()) {
								mutex.unlock_shared();
							}
							break;
						}
						}
					}
				}
				catch (...) {
					error_count++;
					printf("    Error: Unexpected exception in stress worker\n");
				}
				};

			std::vector<std::thread> threads;
			for (int i = 0; i < 10; ++i) {
				threads.emplace_back(stress_worker, i);
			}

			for (auto& t : threads) {
				t.join();
			}

			bool result = stagePassed && (error_count == 0) &&
				(shared_access_count == 0) && (exclusive_access_count == 0);
			recordTest(result, "Stress test with mixed operations");
		}

		// Stage 13: Explicit Authorization Mode Behavior
		{
			printf("\nStage 13: Explicit Authorization Mode Behavior Extended Tests\n");

			bool stagePassed = true;
			ExclusiveWorkerMutex explicitMutex(false, true);  // requires explicit authorization
			std::atomic<bool> unauthorized_shared_blocked{ false };
			std::atomic<bool> authorized_shared_succeeded{ false };
			std::atomic<bool> exclusive_succeeded{ false };

			// Test 1: Unauthorized shared lock should fail
			auto unauthorized_shared_test = [&]() {
				try {
					std::shared_lock<ExclusiveWorkerMutex> lock(explicitMutex);
					stagePassed = false;  // Should not reach here
				}
				catch (const std::system_error&) {
					unauthorized_shared_blocked = true;  // Expected behavior
				}
				};

			// Test 2: Authorized shared lock should succeed
			auto authorized_shared_test = [&]() {
				try {
					ExclusiveWorkerAuthorization auth(explicitMutex);
					std::shared_lock<ExclusiveWorkerMutex> lock(explicitMutex);
					authorized_shared_succeeded = true;
				}
				catch (...) {
					stagePassed = false;
				}
				};

			// Test 3: Exclusive lock should work without authorization
			auto exclusive_test = [&]() {
				try {
					std::lock_guard<ExclusiveWorkerMutex> lock(explicitMutex);
					exclusive_succeeded = true;
				}
				catch (...) {
					stagePassed = false;
				}
				};

			std::thread t1(unauthorized_shared_test);
			std::thread t2(authorized_shared_test);
			std::thread t3(exclusive_test);

			t1.join();
			t2.join();
			t3.join();

			bool result = stagePassed && unauthorized_shared_blocked &&
				authorized_shared_succeeded && exclusive_succeeded;
			recordTest(result, "Explicit authorization mode behavior");
		}

		// Stage 14: Deep Recursive Lock Test
		if(false)//do not - performance hit
		{
			printf("\nStage 14: Deep Recursive Lock Test\n");

			bool stagePassed = true;
			ExclusiveWorkerMutex mutex;
			std::atomic<size_t> successful_locks{ 0 };
			std::atomic<bool> deep_recursion_handled{ false };

			auto recursive_test = [&]() {
				try {
					// First lock
					mutex.lock();
					size_t depth = 1;

					// Try deep recursion
					while (depth < 1000000) {  // Large but not SIZE_MAX
						try {
							mutex.lock();
							depth++;
						}
						catch (const std::system_error&) {
							deep_recursion_handled = true;
							break;
						}
					}

					successful_locks = depth;

					// Unlock all acquired locks
					while (depth > 0) {
						try {
							mutex.unlock();
							depth--;
						}
						catch (...) {
							stagePassed = false;
							break;
						}
					}
				}
				catch (...) {
					stagePassed = false;
				}
				};

			std::thread t(recursive_test);
			t.join();

			bool result = stagePassed && (successful_locks > 0) &&
				(!mutex.is_locked());  // Ensure cleanup succeeded
			recordTest(result, "Deep recursive lock handling");
		}

		// Stage 15: Mixed Lock Acquisition Patterns
		{
			printf("\nStage 15: Mixed Lock Acquisition Patterns\n");

			bool stagePassed = true;
			ExclusiveWorkerMutex mutex;
			std::atomic<int> shared_holders{ 0 };
			std::atomic<bool> exclusive_held{ false };
			std::atomic<int> error_count{ 0 };

			auto shared_worker = [&]() {
				ExclusiveWorkerAuthorization auth(mutex);
				for (int i = 0; i < 100 && error_count == 0; ++i) {
					std::shared_lock<ExclusiveWorkerMutex> lock(mutex);
					shared_holders++;

					if (exclusive_held) {
						error_count++;
						printf("    Error: Shared access during exclusive lock\n");
					}

					std::this_thread::sleep_for(std::chrono::microseconds(1));
					shared_holders--;
				}
				};

			auto exclusive_worker = [&]() {
				for (int i = 0; i < 50 && error_count == 0; ++i) {
					std::lock_guard<ExclusiveWorkerMutex> lock(mutex);

					if (shared_holders > 0 || exclusive_held) {
						error_count++;
						printf("    Error: Invalid lock state during exclusive access\n");
					}

					exclusive_held = true;
					std::this_thread::sleep_for(std::chrono::microseconds(1));
					exclusive_held = false;
				}
				};

			std::vector<std::thread> threads;
			for (int i = 0; i < 3; ++i) {
				threads.emplace_back(shared_worker);
				threads.emplace_back(exclusive_worker);
			}

			for (auto& t : threads) {
				t.join();
			}

			bool result = stagePassed && (error_count == 0) &&
				(shared_holders == 0) && !exclusive_held;
			recordTest(result, "Mixed lock acquisition patterns");
		}

		// Stage 17: Authorization State Changes
		{
			printf("\nStage 17: Authorization State Changes Tests\n");
			ExclusiveWorkerMutex mutex;
			bool stagePassed = true;

			auto test_task = [&]() {
				try {
					// Get exclusive lock first
					std::lock_guard<ExclusiveWorkerMutex> lock(mutex);

					// Authorize while holding lock
					mutex.authorize_thread(std::this_thread::get_id());

					// This should succeed
					mutex.lock_shared();
					mutex.unlock_shared();

					// Unauthorize while holding lock
					mutex.unauthorize_thread(std::this_thread::get_id());
				}
				catch (...) {
					stagePassed = false;
				}
				};

			std::thread t(test_task);
			t.join();

			bool result = stagePassed;
			recordTest(result, "Authorization state changes during lock holding");
		}

		// Stage 18: Mode Transition Tests
		{
			printf("\nStage 18: Mode Transition Tests\n");
			ExclusiveWorkerMutex explicitMutex(false, true);
			ExclusiveWorkerMutex nonExplicitMutex(false, false);
			bool stagePassed = true;

			// Test shared lock behavior in both modes
			auto shared_task = [&]() {
				try {
					// Should fail in explicit mode without authorization
					try {
						std::shared_lock<ExclusiveWorkerMutex> lock(explicitMutex);
						stagePassed = false; // Should not reach here
					}
					catch (const std::system_error&) {
						// Expected
					}

					// Should succeed in non-explicit mode
					std::shared_lock<ExclusiveWorkerMutex> lock(nonExplicitMutex);
				}
				catch (...) {
					stagePassed = false;
				}
				};

			std::thread t(shared_task);
			t.join();

			bool result = stagePassed;
			recordTest(result, "Mode transition behavior");
		}

		// Stage 19: Random Mode Changes Stress Test
		{
			printf("\nStage 19: Random Mode Changes Stress Test\n");
			ExclusiveWorkerMutex mutex;
			std::atomic<bool> stop{ false };
			std::atomic<int> error_count{ 0 };

			auto stress_worker = [&](int id) {
				try {
					for (int i = 0; i < 100 && error_count == 0; ++i) {
						bool do_authorize = (rand() % 2) == 0;
						if (do_authorize) {
							mutex.authorize_thread(std::this_thread::get_id());
							std::shared_lock<ExclusiveWorkerMutex> lock(mutex);
							mutex.unauthorize_thread(std::this_thread::get_id());
						}
						else {
							std::lock_guard<ExclusiveWorkerMutex> lock(mutex);
						}
					}
				}
				catch (...) {
					error_count++;
				}
				};

			std::vector<std::thread> threads;
			for (int i = 0; i < 8; ++i) {
				threads.emplace_back(stress_worker, i);
			}

			for (auto& t : threads) {
				t.join();
			}

			bool result = error_count == 0;
			recordTest(result, "Random mode changes stress test");
		}

		// Final check to ensure counters are back to zero
		if (shared_counter.load() != 0 || exclusive_counter.load() != 0) {
			printf("    Error: Counters not zero at end of tests\n");
			allTestsPassed = false;
		}

		printf("\nExclusiveWorkerMutex Test Results: %zu/%zu tests passed\n",
			passedTests, totalTests);
		return allTestsPassed;
	}
	catch (const std::exception& e) {
		printf("Exception during ExclusiveWorkerMutex tests: %s\n", e.what());
		return false;
	}
	catch (...) {
		printf("Unknown exception during ExclusiveWorkerMutex tests\n");
		return false;
	}
}



// Debug method to verify flag values
void CTests::debugPrintFlags(int colorWithFlags) {
	std::cout << "Debug flags for value " << colorWithFlags << ":\n";
	std::cout << "Base color: " << (colorWithFlags & eColor::COLOR_MASK) << "\n";
	std::cout << "Is blinking: " << ((colorWithFlags & eColor::BLINK_FLAG) != 0) << "\n";
	std::cout << "Is background: " << ((colorWithFlags & eColor::BACKGROUND_FLAG) != 0) << "\n";
	std::cout << "Is bold: " << ((colorWithFlags & eColor::BOLD_FLAG) != 0) << "\n";
}
void CTests::testColorSystem()
{

	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::cout << "\n=== Color System Showcase ===\n\n";

	// Basic color examples
	std::cout << tools->getColoredString("Basic Colors:", eColor::headerCyan | eColor::BOLD_FLAG) << "\n";
	std::cout << tools->getColoredString("Neon Purple ", eColor::neonPurple)
		<< tools->getColoredString("Neon Green ", eColor::neonGreen)
		<< tools->getColoredString("Cyber Wine", eColor::cyberWine) << "\n";

	// Blinking effects
	std::cout << "\n" << tools->getColoredString("Blinking Effects:", eColor::headerCyan | eColor::BOLD_FLAG) << "\n";
	std::cout << tools->getColoredString("WARNING: System overload ", eColor::alertWarning | eColor::BLINK_FLAG)
		<< tools->getColoredString("ERROR: Connection lost", eColor::alertError | eColor::BLINK_FLAG) << "\n";

	// Status indicators
	std::cout << "\n" << tools->getColoredString("Status Indicators:", eColor::headerCyan | eColor::BOLD_FLAG) << "\n";
	std::cout << tools->getColoredString(u8"ONLINE ", eColor::statusOnline | eColor::BOLD_FLAG)
		<< tools->getColoredString(u8"OFFLINE ", eColor::statusOffline)
		<< tools->getColoredString(u8"IDLE", eColor::statusIdle | eColor::BLINK_FLAG) << "\n";

	// Data visualization colors
	std::cout << "\n" << tools->getColoredString("Data Visualization:", eColor::headerCyan | eColor::BOLD_FLAG) << "\n";
	std::cout << tools->getColoredString("Primary Data ", eColor::dataPrimary)
		<< tools->getColoredString("Secondary Data ", eColor::dataSecondary)
		<< tools->getColoredString("Highlighted Data", eColor::dataHighlight | eColor::BOLD_FLAG) << "\n";

	// RGB custom colors
	std::cout << "\n" << tools->getColoredString("Custom RGB Colors:", eColor::headerCyan | eColor::BOLD_FLAG) << "\n";
	std::cout << tools->getColoredString("Custom RGB (255,128,0) ", 255, 128, 0)
		<< tools->getColoredString("Custom RGB (128,0,255)", 128, 0, 255) << "\n";

	// Using WriteLine for logging-style output
	std::cout << "\n" << tools->getColoredString("Log-style Output:", eColor::headerCyan | eColor::BOLD_FLAG) << "\n";
	tools->writeLine(tools->getColoredString("[INFO] ", eColor::lightCyan) +
		tools->getColoredString("System initialized", eColor::ghostWhite));
	tools->writeLine(tools->getColoredString("[WARNING] ", eColor::alertWarning | eColor::BOLD_FLAG) +
		tools->getColoredString("Low memory condition", eColor::ghostWhite));
	tools->writeLine(tools->getColoredString("[ERROR] ", eColor::alertError | eColor::BOLD_FLAG | eColor::BLINK_FLAG) +
		tools->getColoredString("Critical system failure", eColor::ghostWhite));
	tools->writeLine(tools->getColoredString("[SUCCESS] ", eColor::alertSuccess | eColor::BOLD_FLAG) +
		tools->getColoredString("Backup completed", eColor::ghostWhite));

	// Cyberpunk-style header
	std::string header = "=== GRIDNET OS ===";
	std::cout << "\n" << tools->getColoredString(header, eColor::synthPink | eColor::BOLD_FLAG) << "\n";

	// Progress bar example

	tools->writeLine("\nColor system test completed. " +
		tools->getColoredString("All systems nominal.", eColor::alertSuccess | eColor::BOLD_FLAG));

		tools->flashLine(tools->getProgressBarTxt(50));


}

void CTests::validateTargetDifficultyChain() {
	std::shared_ptr<CTools> tools = CTools::getInstance();

	printf("\nTarget/Difficulty Chain Validation\n");
	printf("==================================\n");

	struct TestCase {
		double difficulty;
		std::string description;
		bool expectedResult;
	};

	std::vector<TestCase> testCases = {
		// Test window 3 (easiest difficulties)
		{1.0, "Window 3 - Minimum difficulty", true},
		{100000000.0, "Window 3 - Medium difficulty", true},
		{10000000000000.0, "Window 3 - High difficulty", true},

		// Test window 2
		{100000000000000000000.0, "Window 2 - Low difficulty", true},
		{1000000000000000000000000.0, "Window 2 - Medium difficulty", true},
		{1000000000000000000000000000000.0, "Window 2 - High difficulty", true},

		// Test window 1
		{10000000000000000000000000000000000.0, "Window 1 - Low difficulty", true},
		{10000000000000000000000000000000000000000.0, "Window 1 - Medium difficulty", true},
		{10000000000000000000000000000000000000000000000000000.0, "Window 1 - High difficulty", true},

		// Test window 0 (hardest difficulties)
		{100000000000000000000000000000000000000000000000000000000000.0, "Window 0 - Low difficulty", true},
		{10000000000000000000000000000000000000000000000000000000000000000000000.0, "Window 0 - Medium difficulty", true},
		{DBL_MAX, "Window 0 - Maximum difficulty", false}
	};

	for (const auto& test : testCases) {
		printf("\nTesting %s (difficulty: %.2f)\n",
			test.description.c_str(), test.difficulty);
		printf("----------------------------------------\n");

		// 1. Difficulty to target conversion
		std::vector<uint8_t> targetBytes = tools->diff2target(test.difficulty);

		// Show target bytes in little-endian
		printf("Target bytes (little-endian):\n");
		for (int i = 0; i < 32; i++) {
			printf("%02x ", targetBytes[i]);
			if ((i + 1) % 8 == 0) printf("\n");
		}

		// Show target bytes in big-endian
		printf("Target bytes (big-endian):\n");
		for (int i = 31; i >= 0; i--) {
			printf("%02x ", targetBytes[i]);
			if ((31 - i) % 8 == 7) printf("\n");
		}

		// 2. Convert targetBytes to std::array<unsigned char, 32>
		std::array<unsigned char, 32> targetArray;
		std::copy(targetBytes.begin(), targetBytes.end(), targetArray.begin());

		// 3. Determine active window
		unsigned int window = CWorkUltimium::detectTargetWindowOffset(targetArray);

		printf("\nActive window: %d\n", window);

		// 4. Window analysis
		printf("\nWindow analysis:\n");
		for (int i = 0; i < 4; i++) {
			uint64_t windowVal = ReadLE64(targetArray.data() + i * 8);
			printf("Window %d [bytes %2d-%2d]: 0x%016llx %s\n",
				i, i * 8, (i + 1) * 8 - 1, windowVal,
				(i == window) ? "<-- Active" : "");

			// Check that higher windows are zero
			if (i > window && windowVal != 0) {
				printf("ERROR: Window %d should be zero for proper comparison!\n", i);
			}
		}

		// 5. Hash comparison tests
		printf("\nHash comparison tests:\n");

		struct HashTest {
			std::vector<uint8_t> hashBytes;
			bool shouldPass;
			std::string description;
		};

		std::vector<HashTest> hashTests;

		// Create a valid hash (just below the target)
		std::vector<uint8_t> validHashBytes = targetBytes;
		// Subtract 1 from target to get valid hash
		bool borrow = true;
		for (size_t i = 0; i < 32; ++i) {
			if (borrow) {
				if (validHashBytes[i] == 0x00) {
					validHashBytes[i] = 0xFF;
				}
				else {
					validHashBytes[i] -= 1;
					borrow = false;
				}
			}
		}
		hashTests.push_back({ validHashBytes, test.expectedResult, "Just below target" });

		// Create an invalid hash (just above the target in active window)
		std::vector<uint8_t> invalidHashBytes1 = targetBytes;
		// Add 1 to target to get invalid hash
		bool carry = true;
		for (size_t i = 0; i < 32; ++i) {
			if (carry) {
				if (invalidHashBytes1[i] == 0xFF) {
					invalidHashBytes1[i] = 0x00;
				}
				else {
					invalidHashBytes1[i] += 1;
					carry = false;
				}
			}
		}
		hashTests.push_back({ invalidHashBytes1, false, "Just above target in active window" });

		// Create an invalid hash (non-zero in higher window)
		if (window < 3) { // Ensure there is a higher window
			std::vector<uint8_t> invalidHashBytes2 = validHashBytes;
			// Set a byte in the higher window to 1
			invalidHashBytes2[(window + 1) * 8] = 0x01;
			hashTests.push_back({ invalidHashBytes2, false, "Non-zero in higher window" });
		}

		// Create an invalid hash (maximum possible value)
		std::vector<uint8_t> invalidHashBytes3(32, 0xFF);
		hashTests.push_back({ invalidHashBytes3, false, "Maximum hash value" });

		for (const auto& hashTest : hashTests) {
			printf("\nTesting %s:\n", hashTest.description.c_str());

			// Show hash bytes
			printf("Hash bytes (little-endian):\n");
			for (int i = 0; i < 32; i++) {
				printf("%02x ", hashTest.hashBytes[i]);
				if ((i + 1) % 8 == 0) printf("\n");
			}

			// 6. Perform comparisons
			printf("\nDetailed value comparison:\n");

			// Convert hash bytes to arith_uint256
			arith_uint256 hashValue = tools->targetVec2arithUint(hashTest.hashBytes);
			arith_uint256 targetValue = tools->targetVec2arithUint(targetBytes);

			// Display hash and target values
			printf("Hash Value:   %s\n", hashValue.GetHex().c_str());
			printf("Target Value: %s\n", targetValue.GetHex().c_str());
			printf("Hash <= Target: %s\n", (hashValue <= targetValue) ? "True" : "False");

			// Window-based comparison
			bool windowCompare = true;

			// Check higher windows (more significant than active window)
			for (int i = window + 1; i < 4; i++) {
				uint64_t hashWindowVal = ReadLE64(hashTest.hashBytes.data() + i * 8);
				if (hashWindowVal != 0) {
					windowCompare = false;
					break;
				}
			}

			if (windowCompare) {
				// Compare active window
				uint64_t hashWindowVal = ReadLE64(hashTest.hashBytes.data() + window * 8);
				uint64_t targetWindowVal = ReadLE64(targetBytes.data() + window * 8);

				if (hashWindowVal < targetWindowVal) {
					windowCompare = true;
				}
				else if (hashWindowVal > targetWindowVal) {
					windowCompare = false;
				}
				else {
					// Active window values are equal, compare less significant windows
					if (window > 0) {
						for (int i = window - 1; i >= 0; i--) {
							uint64_t hashWindowVal = ReadLE64(hashTest.hashBytes.data() + i * 8);
							uint64_t targetWindowVal = ReadLE64(targetBytes.data() + i * 8);

							if (hashWindowVal < targetWindowVal) {
								windowCompare = true;
								break;
							}
							else if (hashWindowVal > targetWindowVal) {
								windowCompare = false;
								break;
							}
							// Continue if values are equal
						}
					}
					else {
						// All windows are equal
						windowCompare = true;
					}
				}
			}
			else {
				windowCompare = false;
			}


			// Full 256-bit comparison
			bool fullCompare = hashValue <= targetValue;

			printf("\nWindow comparison: %s\n", windowCompare ? "PASS" : "FAIL");
			printf("Full comparison:   %s\n", fullCompare ? "PASS" : "FAIL");
			printf("Expected result:   %s\n", hashTest.shouldPass ? "PASS" : "FAIL");

			if (windowCompare != fullCompare) {
				printf("ERROR: Window and full comparisons disagree!\n");
			}
			if (fullCompare != hashTest.shouldPass) {
				printf("ERROR: Incorrect result!\n");
			}
		}
	}
}

void CTests::debugCompare(const std::vector<uint8_t>& vec1, const std::vector<uint8_t>& vec2) {
	std::shared_ptr<CTools> tools = CTools::getInstance();
	arith_uint256 a = tools->targetVec2arithUint(vec1);
	arith_uint256 b = tools->targetVec2arithUint(vec2);

	printf("Comparing values:\n");
	printf("Value 1: %s\n", a.ToString().c_str());
	printf("Value 2: %s\n", b.ToString().c_str());
	printf("Comparison (1 <= 2): %s\n", (a <= b) ? "true" : "false");

	// ASCII visualization
	printf("ASCII visualization of Value 1:\n");
	for (int i = 0; i < 32; i++) {
		uint8_t byte = vec1[31 - i]; // Big Endian order for display
		printf("%c", isprint(byte) ? byte : '.');
	}
	printf("\nASCII visualization of Value 2:\n");
	for (int i = 0; i < 32; i++) {
		uint8_t byte = vec2[31 - i]; // Big Endian order for display
		printf("%c", isprint(byte) ? byte : '.');
	}
	printf("\n");
}


void CTests:: validateTargetWindowComparison() {
	printf("Target window comparison validation:\n");

	std::array<unsigned char, 32> target = { 0 };

	// Set value in window 0 (least significant in little-endian)
	uint64_t testValue = 0x1234567890ABCDEF;
	memcpy(target.data(), &testValue, 8);  // Window 0 in little-endian

	printf("Target bytes (little-endian):\n");
	for (int i = 0; i < 32; i++) {
		printf("%02x ", target[i]);
		if ((i + 1) % 8 == 0) printf("\n");
	}

	unsigned int offset = CWorkUltimium::detectTargetWindowOffset(target);
	printf("Detected window offset: %d (should be 0 for least significant)\n", offset);

	printf("\nWindow comparison test (little-endian order):\n");
	for (int i = 0; i < 4; i++) {
		uint64_t windowValue = *((uint64_t*)(target.data() + i * 8));
		printf("Window %d (bytes %2d-%2d): 0x%016llx %s\n",
			i, i * 8, (i * 8) + 7, windowValue,
			(i == offset) ? "<-- Active window (least significant non-zero)" : "");
	}

	printf("\nIn memory (left = lower address = least significant in little-endian):\n");
	printf("Window 0 [LSB] -> Window 3 [MSB]\n");
}

/// <summary>
/// Begins a local transactions simulation.
/// The simulation mimics agents willing to make simple GNC transactions locally.
/// No network interactions are involved. Transactions are made, blocks verified and added on top of each other.
/// 
/// Note: The concept of a StateDomain is abstract logical concept. It is represented mostly by an indentifier (based on a public key)
/// and storage which might contain executable code. (executed by a CALL within a client/incomming GridScript code). It was established to describe
/// an autonomous entity capable of mainting access rights; storage messaging and code execution. i.e. to describe progressive states and
/// interactions between discrete systems. Time continous systems might be also supported through discritization.
/// 
/// Intially a State Domain consists of a public key which is registered nowhere, thus it might be contained on a peace of paper.
/// 
/// Description: On the high-level - the test will be generating new transactions between the constatntly expanding set of already known state domains.
/// 
/// Prerequisits:
/// 
/// + initialPoW
/// + SS1 - the set of known State Domains (might be an empty set). Initally indicated by the number of leaf-nodes within the StateDB.
/// + maxTransactionsPerRound - the ceiling ammount of transactions to generate in each round
/// + SS2 - (the set of new transaction targets) / the set of newly created State Domains. 
/// maxExpansionUnits - the maximum number of State Domains to generate each round - each will serve as a transaction target if below maxTransactionsPerRound.
/// 
/// When generating transactions, targets will choson from SS2 on random, to the extent allowed by maxTransactionsPerRound.State domains within SS1 will be served
/// to the extent allowed by the remainder of maxTransactionsPerRound.
/// 
/// Note:
/// A random KNOWN StateDomain will become the miner (will receive the block verication reward).
/// 
/// 
/// During the test, transactionsManager will be responsible for new block formation.
/// At the end of each round the proposals will be validated and accounted for by processBlock() within the BlockchainManager.
/// In a loop, what follows will commence:
/// 1) SS2 is generated. (the set of new transaction targets)
///	2) an attempt to fetch a set of random source state domains from SS1 is made. IF available,
///		for each State Domain, a random outgress transaction limited by the extent of the available resources is generated and serialized.
//		IF NOT, +2b) the initial genesis transaction is generated. 
/// 3) the serialized transactions are delivered to the transactionsManager with a call to addTransactionToMemPool
/// 4) transactionsManager attempts to form a new block (including PoW verification)
/// 5) when transactionsManager Succeeds, it delivers the block to the BlockchainManager and a new block
/// will be added to the blockchain.
/// </summary>
/// <returns></returns>
bool CTests::simulateTransactionsLocal()
{
	CBlockchainManager::getInstance(eBlockchainMode::LocalData)->getFormationFlowManager()->setDoBlockFormation();
	bool randomErgPrice = mSettings->getUseRandomERGPriceDuringTests();
	bool doTrieDBTestsAfterEachRound = mSettings->getDoTrieDBTestAfterEachRound();
	mBlockchainManager->setTestTrieAfterEachRound(doTrieDBTestsAfterEachRound);
	//uint64_t genesisTransactionAmmount = 100000000000;
	size_t maxExpansionUnits = 20;
	size_t nrOfReceiptsToTestForEachRound = 10;
	size_t maxReceiptIDsCacheSize = 1000;
	std::vector <std::vector<uint8_t>> collectedReceiptIDs;
	uint64_t alternativePathAfter = 4;//after how many blocks I'll try to mine on an alternative path
	uint64_t howDeepToRevertTo = 0;//once the above is aimed for

	//arith_uint256 iitalPow = mSettings->getMinPackedTarget();
	//mSettings->setMinDifficulty
	size_t maxTransactionsPerRound = 100; //max since there might be a single or no stateDomains available
	//or no minimal transaction value might be available on any account thus unable to form min required transactions count
	std::vector<CTransaction> newTransactions;
	std::vector <std::vector<uint8_t>> collectedTransactionsDestinations;
	uint64_t lastMinersChainUtilizationDepth = 0;
	uint32_t evilTransactionsFactor = 0;// the percentage of malicious transactions to generate.
	//malicious transactions include GridScript with transaction value exceeding the source balance.
	std::vector <std::vector<uint8_t>> SS2; //new targets - new state domain identifiers
	std::vector<std::vector<uint8_t>> SS1;// = mBlockchainManager->getStateDomainManager()->getKnownDomainIDs(); //exsiting state domains
	std::vector<std::vector<uint8_t>> servedSources;//to prevent double spend attempts (if not desired).
	std::vector<std::vector<uint8_t>> servedTargets;
	int64_t rounds = -1;//<0 == unlimited
	size_t sleepFor = 3000;//for how long to sleep for (sec) between new blocks. (empty blocks will be generated in the meantime)
	//assert(mStateDB->setPerspective(mBlockchainManager->getPerspective()));
	size_t height = 0;
	size_t domainsCountLive = 0;
	size_t domainsCountLocal = 0;
	uint64_t obigatoryNrOfPastLocalIdentitiesToIssueTransactionsFrom = 5;//if such number is available
	uint64_t singleMiningPathLength = 0;
	bool usingForcedPerspective = false;

	//wait for the  Blockchain Manager to become ready
	mTools->writeLine("Waiting for the Blockchain Manager to become ready..");
	while (!mBlockchainManager->getIsReady())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (mBlockchainManager->getDepth() == 0)
	{
		if (!mBlockchainManager->getFormationFlowManager()->getDoBlockFormation())
		{
			if (mTools->askYesNo("Warning: No Genesis Block and Block Formation disabled. This would cause a dead-lock. Enable block formation? (or quit?)", true))
			{
				if (mTools->askYesNo("Do you want to form the Genesis Block *ONLY*?", false))
					mBlockchainManager->setKeyBlockFormationLimit(1);

				mSettings->setDoBlockFormation(true);
				mBlockchainManager->getFormationFlowManager()->setDoBlockFormation(true);
			}
			else mSettings->setDoBlockFormation(false);

		}
		mTools->writeLine("Awaiting the Genesis Blocks..");
		//blockS since the second block will be the data block containing the actual genesis transactions
		while (mBlockchainManager->getDepth() == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	else
	{
		bool doIt = mTools->askYesNo("Do you want to enable block-formation during tests?", true, "Simulation");
		mBlockchainManager->getFormationFlowManager()->setDoBlockFormation(doIt);
	}

	std::vector<uint8_t> miningPerspective = mBlockchainManager->getPerspective();
	std::vector<uint8_t> forcedParentBlockID;
	uint64_t nrOfTimesUnableToReproduceWorldView = 0;
	CKeyChain localKeyChain(false);
	for (uint64_t r = 0; (r < rounds || rounds < 0); r++)
	{
		std::shared_ptr<CBlock> leader = mBlockchainManager->getLeader();
		std::shared_ptr<CBlock> keyLeader = mBlockchainManager->getLeader(true);
		assertGN(leader != nullptr && keyLeader != nullptr);
		assertGN(mSettings->getCurrentKeyChain(localKeyChain, false, false));
		assertGN(mSettings->getMinersKeyChainID().size() > 0);


		height = leader->getHeader()->getHeight();
		usingForcedPerspective = false;

		mTools->writeLine("I'm preparing for the next round of test transactions..Round " + std::to_string(r + 1));
		servedSources.clear();
		servedTargets.clear();
		newTransactions.clear();
		//collectedTransactionsDestinations.clear();
		std::vector<uint8_t> id;
		//

		//Add to SS1 all the state-domains created using the local miner's key-chain
		//so that we can test all the rewarded domains
		//(deferred reward mechanism, penalties etc.)





		uint32_t testTries = 0;
		bool consistent = false;

		bool isLivePerspective = true;


		do
		{

			miningPerspective = mBlockchainManager->getPerspective();
			if (true)//(((singleMiningPathLength < alternativePathAfter) || mBlockchainManager->getDepth() < 2))
			{
				mTools->writeLine("Assuming current Live perspective for mining");
				forcedParentBlockID.clear();
			}
			else
			{
				usingForcedPerspective = true;
				isLivePerspective = false;
				mTools->writeLine("I'll now try to mine on an alternative key-Path");
				uint64_t maxDepth = min(keyLeader->getHeader()->getKeyHeight() - 1, 4);
				howDeepToRevertTo = 1;
				if (maxDepth != 1)
					howDeepToRevertTo = max(1, mTools->genRandomNumber(1, maxDepth));

				mTools->writeLine("Assuming a key-block at depth " + std::to_string(howDeepToRevertTo) + " as the current Leader");

				std::shared_ptr<CBlock> b = mBlockchainManager->getBlockAtDepth(howDeepToRevertTo, true, eChainProof::verified);
				if (b != nullptr)
				{
					miningPerspective = b->getHeader()->getPerspective(eTrieID::state);
					forcedParentBlockID = b->getID();
				}
				singleMiningPathLength = 0;

			}
			mTools->writeLine("Assuming Perspective " + mTools->base58CheckEncode(miningPerspective) + " as the current one");


			//compare the number of domains in local test db vs the number of domains in Live db
			if (doTrieDBTestsAfterEachRound)
			{
				if (!usingForcedPerspective)
					while (!mBlockchainManager->isNrOfStateDomainsFresh())
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			//set the current test perspective to match the one in Live DB so that we have a current view on things
			if (mTransactionsManager->getIsInFlow())
				mTransactionsManager->abortFlow();
			assertGN(mTransactionsManager->startFlow(miningPerspective));


			if (SS1.size() == 0)
			{
				mTools->writeLine("The effective number of known State Domains is Zero (0)..");

				//this should never happen; by now we should be at at least at block height two. with one domain
				//owned by local miner.
				mTools->writeLine("Seems like the Genesis Block is known; I'll just fetch a few domains as starting points..");
				size_t nc;
				mStateDB->testTrie(nc, true, true, true, eTrieSize::smallTrie, 100);
				SS1 = mStateDomainManager->getKnownDomainIDs();
				assertGN(nc > 0);//there's a block but no state domains known? even the genesis block  is not empty

			}

			while ((lastMinersChainUtilizationDepth <= localKeyChain.getUsedUptillIndex()) || SS1.size() == 0)
			{
				localKeyChain.setIndex(lastMinersChainUtilizationDepth);
				CCryptoFactory::getInstance()->genAddress(localKeyChain.getPubKey(), id);
				SS1.push_back(id);
				lastMinersChainUtilizationDepth++;
			}

			if (doTrieDBTestsAfterEachRound)
			{
				if (isLivePerspective)
				{
					domainsCountLive = mBlockchainManager->getKnownStateDomainsCount();
					mTools->writeLine("Testing local test-trie DB" + mTools->getColoredString("This may take some time(> 5 minutes)..", eColor::orange));
					assertGN(mStateDB->testTrie(domainsCountLocal, false, false));
					bool htb = false;
					std::vector<CTrieNode*> leaves = mStateDB->getAllSubLeafNodes();
					for (int i = 0; i < leaves.size(); i++)
					{
						std::vector<uint8_t> address = static_cast<CStateDomain*>(leaves[i])->getAddress();
						std::vector<uint8_t> actualPath = mTools->nibblesToBytes(leaves[i]->getTempFullPath(), htb);
						std::vector<nibblePair> np1 = mTools->bytesToNibbles(address);
						std::vector<nibblePair> np2 = mTools->bytesToNibbles(actualPath);
						assertGN(std::memcmp(address.data(), actualPath.data(), 32) == 0);
					}
					mStateDB->pruneTrie();
					if (domainsCountLocal == domainsCountLive)
						consistent = true;
				}
				else
					consistent = false; //reverted perspective

				if (!consistent && isLivePerspective)
				{//maybe there was a block added in the meanwhile etc. etc.
					mTools->writeLine("I was unable to retrieve entire Live perspective's world-view ( " + std::to_string(testTries) + " tries )");
					mTools->writeLine("I'll wait till the current block processing is over.");
					mBlockchainManager->waitTillBlockProcessingDone();
					//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
				testTries++;
			}
		} while (doTrieDBTestsAfterEachRound && (!consistent && testTries < 3));

		if (doTrieDBTestsAfterEachRound)
		{
			if (isLivePerspective && consistent)
				mTools->writeLine("Test and Live world-views are consistent");
			else if (isLivePerspective && !consistent)
			{
				mTools->writeLine("I was unable to reproduce the full Live world-view after " + std::to_string(testTries) + "tries; I'll give up for this round; This happened already " + std::to_string(nrOfTimesUnableToReproduceWorldView) + " times");

				nrOfTimesUnableToReproduceWorldView++;
				while (mBlockchainManager->getLeader() == NULL || (mBlockchainManager->getLeader()->getHeader()->getHeight() == height))
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				mTransactionsManager->abortFlow();
				continue;
			}
		}
		if (!doTrieDBTestsAfterEachRound)//the trie is not parsed / tested after each round from cold storage
			//thus it might not and will not contain all the State Domains.
			//that is why we are copying over the known identifiers from the Live State Domain manager.
			mStateDomainManager->setKnownStateDomainIDs(mBlockchainManager->getStateDomainManager()->getKnownDomainIDs());


		Sleep(200);


		SS2.clear();

		//clean-up after recent block formation end

		//GENERATE NEW TARGET STATE DOMAIN IDS - BEGIN
		std::string ids;
		mTools->writeLine("Generating identifiers for new State Domains as per the expansion ratio..");
		for (int i = 0; i < maxExpansionUnits && i < maxTransactionsPerRound; i++)
		{//generate KeyChains for each new state domain
			CKeyChain chain;
			assertGN(mSettings->saveKeyChain(chain));
			SS2.push_back(mTools->stringToBytes(chain.getID()));
			ids += chain.getID() + " ";
		}

		mTools->writeLine("Generated IDs:" + ids);
		//GENERATE NEW TARGET STATE DOMAIN IDS - END

		//GENERATE TRANSACTIONS - BEGIN
		bool destroyEnvelopeSig = false;
		bool destroyXValueSig = false;
		bool destroyPubKey = false;
		bool removePubKey = false;
		bool genUnknownSource = false;
		bool exceedBalance = false;
		std::vector<uint8_t> destinationAddress;
		//TRANSACTIONS FROM OLD TO NEW STATE DOMAINS - BEGIN
		mTools->writeLine("Generating test transactions from existing to new state domains");
		if (SS1.size() > 0 && SS2.size() > 0)
			for (int i = newTransactions.size(); i < maxExpansionUnits
				&& i < maxTransactionsPerRound; i++)
			{

				exceedBalance = false;
				destroyEnvelopeSig = false;
				destroyXValueSig = false;
				destroyPubKey = false;
				removePubKey = false;
				genUnknownSource = false;

				if (mTools->genRandomNumber(1, 100) <= 5)
					destroyEnvelopeSig = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					destroyXValueSig = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					destroyPubKey = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					removePubKey = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					genUnknownSource = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					exceedBalance = true;


				size_t nCount;
				//assert(mStateDomainManager->getDB()->testTrie(nCount,false,false));

				CStateDomain* sourceSD = nullptr;

				if (SS1.size() > 1)
					sourceSD = mStateDomainManager->findByID(SS1[mTools->genRandomNumber(0, SS1.size() - 1)]);
				else if (SS1.size() > 0)
					sourceSD = mStateDomainManager->findByID(SS1[0]);


				if (sourceSD == nullptr
					)
				{//Note:: it's perfectly normal fot the node not to be available if it's not the current perspective
					//(the domain list gets copied from the live db)
					if (!(!isLivePerspective && !doTrieDBTestsAfterEachRound))
						mTools->writeLine("Error: source state Domain unknown from current Perspective");
					//NOTE: when bootstraping, the miner's State-Domain is not available yet in Genesis Block BUT
					//it was added to SS1 already; it will be available as soon as miner diggs the first key-block
					continue;
				}


				BigInt availableResourcesA = sourceSD->getBalance();
				if (availableResourcesA == 0)
				{
					continue;
				}
				BigFloat availableResourcesAF = availableResourcesA.convert_to<BigFloat>();
				BigInt ceiligValue = (availableResourcesAF * BigFloat("0.5")).convert_to<BigInt>();

				BigInt transactionValue = ceiligValue > 1 ? mTools->genRandomNumber(1, static_cast<int64_t>(ceiligValue)) : 1;

				if (exceedBalance)
					transactionValue = (availableResourcesAF * BigFloat("1.2")).convert_to<BigInt>();
				CKeyChain chainS(false);

				if (!mSettings->getCurrentKeyChain(chainS, false, false, mTools->bytesToString(((CStateDomain*)sourceSD)->getAddress())))
				{
					//check if owned by local leader (i.e. offspring of its own chain)
					bool privKeyFoundinLocalLeadersChain = false;
					for (uint64_t y = 0; y < lastMinersChainUtilizationDepth; y++)
					{
						localKeyChain.setIndex(y);
						CCryptoFactory::getInstance()->genAddress(localKeyChain.getPubKey(), id);
						if (mTools->compareByteVectors(id, ((CStateDomain*)sourceSD)->getAddress()))
						{
							privKeyFoundinLocalLeadersChain = true;
							chainS = localKeyChain;
							break;
						}

					}

					if (!privKeyFoundinLocalLeadersChain)
						continue;
				}
				uint64_t currentNonce = sourceSD->getNonce();//the transactions are sorted by potential income so if multiple transactions are generated from the same state domain then they migh appear in invalid order on the other end.
				destinationAddress = SS2[i];
				Botan::SecureVector<uint8_t> privKey = chainS.getPrivKey();
				if (destroyXValueSig)
					mTools->introduceRandomChange(privKey);
				CTransaction t = mTools->genAtomicTransaction(destinationAddress, transactionValue,
					chainS.getPubKey(), privKey, currentNonce);

				t.setERGLimit(mTools->genRandomNumber(2000, 3500));
				t.setERGPrice(randomErgPrice ? mTools->genRandomNumber(1, 5) : 1);

				if (!genUnknownSource)
				{
					t.setPubKey(chainS.getPubKey());
					t.sign(privKey);
				}
				else
				{
					CKeyChain c;
					t.setPubKey(c.getPubKey());
					t.sign(c.getPrivKey());
				}

				if (destroyEnvelopeSig)
					t.destroyEnvelopeSig();
				if (removePubKey)
					t.clearPubKey();
				if (destroyPubKey)
					t.destroyPubKey();

				newTransactions.push_back(t);
				collectedTransactionsDestinations.push_back(destinationAddress);
				std::vector<uint8_t> perspectiveID;

				if (!exceedBalance)
				{
					assertGN(sourceSD->changeBalanceBy(perspectiveID, -1 * transactionValue, true));//this happens only within the
					//local testing sub-system and does not affect the Blockchain Manager's LIVE or 'in-flo'w view of things.
					sourceSD->incNonce(true);// the nonce should remain increased (in HotStorage) untill the perspective of the state-db gets updated (next round)
					BigInt availableResourcesB = sourceSD->getBalance();
					assertGN(availableResourcesB == (availableResourcesA - transactionValue));
				}
				servedSources.push_back(sourceSD->getAddress());
				servedTargets.push_back(SS2[i]);
			}

		//TRANSACTIONS FROM OLD TO NEW STATE DOMAINS - END

		//TRANSACTIONS BETWEEN EXISTING STATE DOMAINS - BEGIN

		mTools->writeLine("Generating test transactions between the existing state domains");
		if (SS1.size() > 1)
			for (int i = newTransactions.size(); i < maxTransactionsPerRound; i++)
			{
				exceedBalance = false;
				destroyEnvelopeSig = false;
				destroyXValueSig = false;
				destroyPubKey = false;
				removePubKey = false;
				genUnknownSource = false;

				if (mTools->genRandomNumber(1, 100) <= 5)
					destroyEnvelopeSig = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					destroyXValueSig = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					destroyPubKey = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					removePubKey = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					genUnknownSource = true;
				if (mTools->genRandomNumber(1, 100) <= 5)
					exceedBalance = true;

				CStateDomain* sourceSD = mStateDomainManager->findByID(SS1[mTools->genRandomNumber(0, SS1.size() - 1)]);
				CStateDomain* targetSD = mStateDomainManager->findByID(SS1[mTools->genRandomNumber(0, SS1.size() - 1)]);

				if ((sourceSD == NULL || targetSD == NULL))//might happen after reverting to an old subchain

				{
					mTools->writeLine("warning: state domain unknown from the current Perspective");
					continue;
				}

				destinationAddress = targetSD->getAddress();
				if (mTools->checkVectorContained(sourceSD->getAddress(), servedSources) ||
					mTools->checkVectorContained(targetSD->getAddress(), servedTargets) ||
					mTools->compareByteVectors(sourceSD->getAddress(), targetSD->getAddress())
					)
				{
					continue;
				}
				BigInt availableResourcesA = 0;
				BigInt availableResourcesB = 0;
				CKeyChain chainS(false), chainT(false);
				availableResourcesA = sourceSD->getBalance();
				BigFloat availableResourcesAF = availableResourcesA.convert_to<BigFloat>();
				if (availableResourcesA == 0)
					continue;
				if (!mSettings->getCurrentKeyChain(chainS, false, true, mTools->bytesToString(((CStateDomain*)sourceSD)->getAddress())))
					continue;
				if (!(mSettings->getCurrentKeyChain(chainT, false, true, mTools->bytesToString(((CStateDomain*)targetSD)->getAddress()))))
					continue;
				BigInt ceiligValue = (availableResourcesAF * BigFloat("0.5")).convert_to<BigInt>();
				BigInt transactionValue = ceiligValue > 1 ? mTools->genRandomNumber(1, static_cast<uint64_t>(ceiligValue)) : 1;

				if (exceedBalance)
					transactionValue = (availableResourcesAF * BigFloat("1.2")).convert_to<BigInt>();

				Botan::SecureVector<uint8_t> privKey = chainS.getPrivKey();
				if (destroyXValueSig)
					mTools->introduceRandomChange(privKey);
				CTransaction t = mTools->genAtomicTransaction(destinationAddress, transactionValue,
					chainS.getPubKey(), privKey, sourceSD->getNonce());
				servedSources.push_back(sourceSD->getAddress());
				servedTargets.push_back(targetSD->getAddress());
				t.setERGLimit(mTools->genRandomNumber(2000, 3500));
				t.setERGPrice(randomErgPrice ? mTools->genRandomNumber(1, 5) : 1);

				if (!genUnknownSource)
				{
					t.setPubKey(chainS.getPubKey());
					t.sign(privKey);
				}
				else
				{
					CKeyChain c;
					t.setPubKey(c.getPubKey());
					t.sign(c.getPrivKey());
				}

				if (destroyEnvelopeSig)
					t.destroyEnvelopeSig();
				if (removePubKey)
					t.clearPubKey();
				if (destroyPubKey)
					t.destroyPubKey();

				newTransactions.push_back(t);
				collectedTransactionsDestinations.push_back(destinationAddress);
				std::vector<uint8_t> perspectiveID;
				//
				if (!exceedBalance)
				{
					assertGN(sourceSD->changeBalanceBy(perspectiveID, -1 * transactionValue, true));//this happens only within the
					//local testing sub-system and does not affect the Blockchain Manager's LIVE or 'in-flo'w view of things.
					sourceSD->incNonce(true);// the nonce should remain increased (in HotStorage) untill the perspective of the state-db gets updated (next round)
					BigInt availableResourcesB = sourceSD->getBalance();
					assertGN(availableResourcesB == (availableResourcesA - transactionValue));
				}
			}
		//TRANSACTIONS BETWEEN EXISTING STATE DOMAINS - END

	//GENERATE TRANSACTIONS - END

		assertGN(mTransactionsManager->abortFlow());
		mTools->writeLine("Registering transactions within the Transactions Manager..");
		if ((collectedReceiptIDs.size() + newTransactions.size()) > maxReceiptIDsCacheSize)
		{
			mTools->writeLine("Clearing collected receipt-IDs and transaction-destinations cache..");
			collectedReceiptIDs.clear();
			collectedTransactionsDestinations.clear();
		}

		std::vector<uint8_t> receiptID;
		for (int i = 0; i < newTransactions.size(); i++)
		{
			if (mBlockchainManager->getFormationFlowManager()->registerTransaction(newTransactions[i], receiptID))
			{

				collectedReceiptIDs.push_back(receiptID);
			}
			else
				mTools->writeLine("Couldn't register transaciton. Possible duplicate.");
		}


		mTools->writeLine("I'll now wait for Transaction Manager to form a block..");
		mBlockchainManager->getFormationFlowManager()->resume(); //unfreez the main Flow transaction manager (not the local test one);
		//when no transactions are available in mem pool; the tranaction manager will attempt to create empty blocks
		if (usingForcedPerspective)
		{
			//mBlockchainManager->getFlowTransactionsManager()->forceMiningPerspectiveOfNextBlock(miningPerspective);
			mBlockchainManager->getFormationFlowManager()->forceOneTimeParentMiningBlockID(forcedParentBlockID);
			mBlockchainManager->getFormationFlowManager()->setOneTimeDiffMultiplier(mTools->genRandomNumber((howDeepToRevertTo - 1) >= 1 ? (howDeepToRevertTo - 1) : 1, howDeepToRevertTo + 4 - 1));//too low should produce insifficient PoW
		}



		singleMiningPathLength++;
		//lets now wait a while 
		Sleep(sleepFor);



		uint64_t transactionsInCurrentRoundCount = newTransactions.size();
		//let us wait for one of the latest transactions to become confirmed, and get the block in which it was
		//CReceipt rec(eBlockchainMode::LocalData);

		size_t waitStart = mTools->getTime();
		size_t maxWaitTime = 60;
		std::shared_ptr<CBlock> container = nullptr;
		std::vector<uint8_t> receiptHash;
		std::vector<uint8_t> blockID;
		eBlockInstantiationResult::eBlockInstantiationResult bir;
		uint64_t informedThatWaiting = 0;
		mTools->writeLine("I'll now wait for the block with one of the new transactions.(" + std::to_string(maxWaitTime) + "sec. timeout)");
		while (container == nullptr && (mTools->getTime() - waitStart) < maxWaitTime)
		{
			if ((mTools->getTime() - informedThatWaiting) > 3)
			{
				mTools->writeLine("Waitining for confirmations of same random past transactions.. (" + std::to_string(max(0, maxWaitTime - (mTools->getTime() - waitStart))) + " sec. left)");
				informedThatWaiting = mTools->getTime();
			}
			for (uint64_t o = 0; o < newTransactions.size() && container == nullptr && collectedReceiptIDs.size() >= o + 1; o++)
			{
				if (std::shared_ptr<CReceipt> receipt = mBlockchainManager->findReceipt(collectedReceiptIDs[collectedReceiptIDs.size() - o - 1], false))
				{
					if (mSS->loadLink(receipt->getGUID(), receiptHash, eLinkType::eLinkType::receiptsGUIDtoReceiptsHash))
					{
						mSS->loadLink(receiptHash, blockID, eLinkType::eLinkType::receiptHashToBlockID);
					}
					if (blockID.size() > 0)
					{
						container = mSS->getBlockByHash(blockID, bir, true);
						mTools->writeLine("Transactions found in block " + mTools->base58CheckEncode(blockID));
					}
				}
			}
			Sleep(100);
		}

		if (container != nullptr)
		{

			//Test Retrieval of Receipts - BEGIN
			mTools->writeLine("Trying to retrieve receipts of some random past transactions..");

			uint64_t randomReceiptIndex = 0;
			if (collectedTransactionsDestinations.size() > 0)
				for (int i = 0; i < nrOfReceiptsToTestForEachRound && i < collectedReceiptIDs.size(); i++)
				{
					randomReceiptIndex = mTools->genRandomNumber(0, min(collectedReceiptIDs.size(), collectedTransactionsDestinations.size() - 1));

					if (std::shared_ptr<CReceipt> receipt = mBlockchainManager->findReceipt(collectedReceiptIDs[randomReceiptIndex], false))
					{
						mTools->writeLine("Receipt (GUID: " + mTools->base58CheckEncode(collectedReceiptIDs[randomReceiptIndex]) + " retrieved! Transaction result: "
							+ receipt->translateStatus());

						if (receipt->getResult() == eTransactionValidationResult::valid)
						{
							//the transaction succeeded
							//add target domain to SS1, we control it.
							//from now on it will be used during test as source of transactions
							bool alreadyInSS1 = false;
							for (uint64_t y = 0; y < SS1.size(); y++)
							{
								if (mTools->compareByteVectors(SS1[y], collectedTransactionsDestinations[randomReceiptIndex]))
								{
									alreadyInSS1 = true;
									break;
								}
							}
							if (!alreadyInSS1)
								SS1.push_back(collectedTransactionsDestinations[randomReceiptIndex]);
						}
					}
					else
						mTools->writeLine("I could not retrieve the receipt with GUID: " + mTools->base58CheckEncode(collectedReceiptIDs[i]));
				}
			//Test Retrieval of Receipts - END

			//Test Transaction Merkle-Proofs - BEGIN
			mTools->writeLine("Testing Merkle-Proofs against transactions in the found block..");
			for (int i = 0; i < newTransactions.size(); i++)
			{
				std::vector<uint8_t> proof = container->getHeader()->getMerkleProof(newTransactions[i].getHash(), eTrieID::transactions);
				if (proof.size() != 0)
				{
					bool validProof = container->getHeader()->getTransactionsDB()->verifyPackedMerklePath(proof);
					if (validProof)
					{
						mTools->writeLine("Merkle Proof for transaction " + mTools->base58CheckEncode(newTransactions[i].getHash()) + " is VALID, the transaction is within the"
							+ mTools->base58CheckEncode(container->getID()) + " block (leader)");

					}
					else
					{
						mTools->writeLine("Merkle Proof for transaction " + mTools->base58CheckEncode(newTransactions[i].getHash()) + " is INVALID, the transaction is NOT within the"
							+ mTools->base58CheckEncode(container->getID()) + " block (leader)");
					}
				}
				else
				{
					mTools->writeLine("Merkle Proof for transaction " + mTools->base58CheckEncode(newTransactions[i].getHash()) + " was NOT FOUND, the transaction is NOT within the"
						+ mTools->base58CheckEncode(container->getID()) + " block (leader)");
				}
			}
		}
		//Test Transaction Merkle-Proofs - END
	}

	//mTransactionsManager->freezOperations(true);

	//abort block formation so there is no race condition among  other tests (todo: ensure other tests do not use the testDB);
	mBlockchainManager->getFormationFlowManager()->setDoBlockFormation(false);
	CBlockchainManager::getInstance(eBlockchainMode::LocalData)->getFormationFlowManager()->setDoBlockFormation(false);
	return true;
}


bool CTests::testNetworkPacketSerialization()
{
	mTools->writeLine("Testing network packets serialization/de-serialization");
	std::vector<uint8_t> t1, t2;
	t1 = mTools->genRandomVector(120);
	t2 = mTools->genRandomVector(256);
	CNetMsg msg = CNetMsg(eNetEntType::msg, eNetReqType::notify, t1);
	msg.setDestinationType(eEndpointType::IPv4);
	msg.setDestination(mTools->genRandomVector(8));
	msg.setSourceType(eEndpointType::IPv4);
	msg.setSource(mTools->genRandomVector(8));
	msg.incHops();
	msg.setSourceSeq(65);
	msg.setDestSeq(61);
	msg.setExtraBytes(mTools->genRandomVector(13));

	std::vector<uint8_t> packedData = msg.getPackedData();


	std::shared_ptr<CNetMsg> retrieved = CNetMsg::instantiate(packedData);
	if (!retrieved)
		return false;

	if (msg == (*retrieved))
	{
		mTools->writeLine("ok");
		return true;
	}


	return false;
}

bool CTests::testNetworkPacketSecurity()
{
	mTools->writeLine("Testing network packets security");
	std::vector<uint8_t> t1, t2;
	t1 = mTools->genRandomVector(120);
	t2 = mTools->genRandomVector(256);

	//Local Variables - BEGIN
	std::vector<uint8_t> pubKey;	Botan::secure_vector<uint8_t> privKey;
	CCryptoFactory::getInstance()->genKeyPair(privKey, pubKey);
	CNetMsg msg = CNetMsg(eNetEntType::msg, eNetReqType::notify, t1);
	std::vector<uint8_t> symetricKey = mTools->genRandomVector(32);
	//Local Variables - END


	mCryptoFactory->genKeyPair(privKey, pubKey);
	mTools->writeLine("Testing network packets security - encryption/authentication (ECIES)");
	if (!msg.encrypt(pubKey, true))
		return false;

	if (!msg.decrypt(Botan::unlock(privKey)))
		return false;

	if (!mTools->compareByteVectors(t1, msg.getData()))
		return false;

	//negative tests (authentication)
	if (!msg.encrypt(pubKey, true))
		return false;

	msg.setRequestType(eNetReqType::request);//attack the request-type vector; should be detected through Poly1305 mechanics

	if (msg.decrypt(Botan::unlock(privKey)))
		return false;//this time, this SHOULD fail (ciphertext-extrinsic datagram related data was tempered with);

	mTools->writeLine("Testing network packets security - encryption/authentication (symetric key)");
	msg.setData(t1);
	if (!msg.encrypt(symetricKey, false))
		return false;

	if (!msg.decrypt(symetricKey))
		return false;

	if (!mTools->compareByteVectors(t1, msg.getData()))
		return false;

	//negative tests
	if (!msg.encrypt(symetricKey, false))
		return false;

	msg.setEntityType(eNetEntType::hello);
	if (msg.decrypt(symetricKey))
		return false;//this SHOULD fail


	mTools->writeLine("Testing network packets security - signatures");

	msg.setDestinationType(eEndpointType::PeerID);
	msg.setDestination(mTools->genRandomVector(8));
	msg.setSourceType(eEndpointType::PeerID);
	msg.setSource(mTools->genRandomVector(8));
	msg.incHops();
	msg.setSourceSeq(65);
	msg.setDestSeq(61);
	msg.setExtraBytes(mTools->genRandomVector(13));
	if (!msg.sign(privKey))
		return false;

	bool valid = msg.verifySig(pubKey);
	if (!valid)
		return false;

	//testing serialized datagrams
	std::vector<uint8_t> packed = msg.getPackedData();
	std::shared_ptr<CNetMsg> msgR = CNetMsg::instantiate(packed);

	valid = msgR->verifySig(pubKey);
	if (!valid)
		return false;


	//test against malicious pubkey
	std::vector<uint8_t> evilPub = pubKey;
	mTools->introduceRandomChange(evilPub);
	valid = msg.verifySig(evilPub);
	if (valid)
		return false;
	//test against altered msg's content

	std::vector<uint8_t> evilData = msg.getData();
	mTools->introduceRandomChange(evilData);
	msg.setData(evilData);
	valid = msg.verifySig(pubKey);
	if (valid)
		return false;

	return true;
}
bool CTests::simulateTransactionsNetwork()
{
	return false;
}
bool CTests::testAccounts()
{
	mTools->writeLine("Testing the State Domain data structure..");
	bool toRet = false;
	//return true;
	std::vector<uint8_t> id = mTools->genRandomVector(33);
	std::vector<uint8_t> code = mTools->genRandomVector(128);
	CStateDomain acs(id, mStateDB->getPerspective(), code, eBlockchainMode::eBlockchainMode::LocalData);
	std::vector<uint8_t> packedData = acs.getPackedData();

	CStateDomain* acs2 = CStateDomain::instantiateStateDomain(packedData, mStateDB->getPerspective(), eBlockchainMode::eBlockchainMode::LocalData);

	if (acs == *acs2)
		toRet = true;
	std::vector<uint8_t> hash1 = acs2->getHash();
	acs2->setCode(mTools->genRandomVector(128));
	std::vector<uint8_t> hash2 = acs2->getHash();
	if (std::memcmp(hash1.data(), hash2.data(), 32) == 0)
		return false;
	CStateDomain sd(eBlockchainMode::LocalData);
	hash1 = sd.getHash();
	sd.setAddress((mTools->genRandomVector(32)));
	hash2 = sd.getHash();
	delete acs2;
	if (std::memcmp(hash1.data(), hash2.data(), 32) == 0)
		return false;



	return toRet;
}
bool CTests::testTrieDBSerialization()
{
	//test empty Data-Trie DB - START
	CDataTrieDB testDir(eBlockchainMode::eBlockchainMode::TestNet);
	std::vector<uint8_t> packedData = testDir.getPackedData();
	CDataTrieDB* retrievedDir = static_cast<CDataTrieDB*>(mTools->nodeFromBytes(packedData, eBlockchainMode::eBlockchainMode::TestNet));

	if (retrievedDir != nullptr)
		delete retrievedDir;
	else return false;
	//test empty Data-Trie DB - START

	//test Data-Trie-DB retrieval with additional data saved to cold-storage - START
	CDataTrieDB testDir2(eBlockchainMode::eBlockchainMode::TestNet);
	std::vector<uint8_t> bytesToSave = mTools->stringToBytes("information");
	if (!testDir2.saveValue("info", bytesToSave, eDataType::eDataType::bytes))
		return false;
	packedData = testDir2.getPackedData();
	retrievedDir = static_cast<CDataTrieDB*>(mTools->nodeFromBytes(packedData, eBlockchainMode::eBlockchainMode::TestNet));
	if (retrievedDir == nullptr)
		return false;

	eDataType::eDataType retrievedDataType;

	std::vector<uint8_t> retrievedBytes = retrievedDir->loadValue(mTools->stringToBytes("info"), retrievedDataType);
	if (retrievedDataType != eDataType::eDataType::bytes)
		return false;

	if (retrievedBytes.size() != bytesToSave.size())
		return false;
	if (std::memcmp(retrievedBytes.data(), bytesToSave.data(), retrievedBytes.size()) != 0)
		return false;

	if (retrievedDir != nullptr)
		delete retrievedDir;
	else return false;
	//test Data-Trie-DB retrieval with additional data saved to cold-storage - END

	return true;
}
bool CTests::testDirectories()
{
	mTools->writeLine("Testing directories..");
	mTools->writeLine("Directories test finished.");
	assertGN(!mTransactionsManager->getIsInFlow());
	assertGN(!mStateDomainManager->isInFlow());
	assertGN(mTransactionsManager->setPerspective(mTransactionsManager->getPerspective(true)));

	size_t iterations = 50;
	uint32_t progress = 0;
	uint32_t count = 0;
	CStateDomain* found = NULL;
	int previousFlashed = 0;
	bool hlsb;
	std::shared_ptr<CCryptoFactory>   cf = CCryptoFactory::getInstance();
	//C//Tools mTools;
	mTools->flashLine("Directories test: " + std::to_string((int)progress) + "%");
	bool saveAsInteger = false;
	std::vector <std::vector<uint8_t>> domainIDS = mStateDomainManager->getKnownDomainIDs();
	if (domainIDS.size() == 0)
	{
		mTools->writeLine("No State Domains unable to start the Directories test.");
		return false;
	}
	bool testColdStorageCommits = true;
	std::vector<uint8_t> currentPerspective = mStateDB->getPerspective();
	std::vector<uint8_t> dataToSave;
	dataToSave.resize(sizeof(uint64_t));//64bit uint64_t

	std::vector < std::vector<uint8_t>> pastPerspectives;
	std::vector<uint8_t> currentDomainID;
	//data storage plus additional integrity  verifications
	assertGN(mTransactionsManager->startFlow());

	std::vector<std::vector<uint8_t>> processedDomains;
	CTrieNode* foundDir = nullptr;
	std::string path;
	std::string dirName;
	uint64_t cost = 0;

	for (uint64_t i = 0; i < iterations; i++)
	{
		currentDomainID = domainIDS[mTools->genRandomNumber(0, domainIDS.size() - 1)];
		processedDomains.push_back(currentDomainID);
		found = static_cast<CStateDomain*>(mStateDomainManager->findByID(currentDomainID));
		assertGN(found != NULL);
		std::memcpy(dataToSave.data(), &i, sizeof(i));

		dirName = "dir" + std::to_string(i);
		if (!found->createDirectory(dirName, currentPerspective, cost))
			return false;

		path = "//" + dirName + "//";

		if (!found->goToPath(path, &foundDir))
		{
			return false;
		}
		if (!foundDir->isDirectory())
			return false;
		//remove node and try to re-iterate the directory from cold-storage
		assertGN(mStateDB->removeNode(mTools->bytesToNibbles(currentDomainID)));
		found = static_cast<CStateDomain*>(mStateDomainManager->findByID(currentDomainID));
		if (!found->goToPath(path, &foundDir))
		{
			return false;
		}

		std::vector<uint8_t> newPerspective;
		dirName = dirName + "." + std::to_string(i);
		if (!found->createDirectory(dirName, newPerspective, cost, path))
			return false;
		assertGN(std::memcmp(currentPerspective.data(), newPerspective.data(), 32) != 0);
		path = path + "/" + dirName + "/";

		if (!found->goToPath(path, &foundDir))
		{
			return false;
		}
		if (!foundDir->isDirectory())
			return false;
		//std::vector<uint8_t> aPerspective = mTransactionsManager->getPerspective();
		std::string testData = "test file-content: " + mTools->bytesToString(mTools->genRandomVector(256));
		std::string testFileName = "file" + std::to_string(i) + ".txt";
		assertGN(found->saveValueDB(CAccessToken::genSysToken(), testFileName, mTools->stringToBytes(testData), eDataType::eDataType::bytes, newPerspective, cost, false, false, path));
		assertGN(static_cast<CDataTrieDB*>(foundDir)->getName().size() == dirName.size());

		assertGN(std::memcmp(static_cast<CDataTrieDB*>(foundDir)->getName().data(),
			dirName.data(), dirName.size()) == 0);
		//try to retrieve State-Domain and directory from Cold Storage

		//Note: if we were updating the Trie in sandbox-mode; after removal of the node the Trie wold need to be reverted to
		//the previous perspective (aPerspective)
		assertGN(mStateDB->removeNode(mTools->bytesToNibbles(currentDomainID)));
		//mTransactionsManager->setPerspective(aPerspective);
		found = static_cast<CStateDomain*>(mStateDomainManager->findByID(currentDomainID));
		assertGN(found != nullptr);

		//try to iterate the same path again
		if (!found->goToPath(path, &foundDir))
		{
			return false;
		}
		if (!foundDir->isDirectory())
			return false;
		eDataType::eDataType retrievedType;
		std::vector<uint8_t> retrievedVector = found->loadValueDB(CAccessToken::genSysToken(), testFileName, retrievedType, path);
		std::string retrievedStr = mTools->bytesToString(retrievedVector);
		assertGN(retrievedStr.size() == testData.size());
		assertGN(std::memcmp(retrievedStr.data(), testData.data(), testData.size()) == 0);

		//PROGRESS BAR  - START
		progress = (int)(((double)i / (double)iterations) * 100);

		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("Directories test: " + std::to_string((int)progress) + "%");
		}
	}
	assertGN(mTransactionsManager->endFlow(true, false, true) != CTransactionManager::eDBTransactionFlowResult::failure);
	return true;
}
bool CTests::testPathParsing()
{
	std::vector<std::string> dirs;
	std::string dirPart;
	bool isAbsolutePath = false;
	std::string filename;
	std::string sample = "./var/log/xyz/10032008.log";
	if (!mTools->parsePath(isAbsolutePath, sample, filename, dirs, dirPart))
		return false;
	return true;

}


bool  CTests::testVMMetaGeneratorAndParser()
{
	CVMMetaGenerator generator;
	generator.beginSection(eVMMetaSectionType::requests);
	generator.addDataRequest(eDataRequestType::boolean, "Question", "yes or no?");
	generator.endSection();
	std::vector<uint8_t> bytes = generator.getData();

	CVMMetaParser parser;
	std::vector<std::shared_ptr<CVMMetaSection>> sections = parser.decode(bytes);


	if (sections.size() == 0)
		return false;

	if (sections[0]->getType() != eVMMetaSectionType::requests)
		return false;

	if (sections[0]->getEntries().size() == 0)
		return false;
	if (sections[0]->getEntries()[0]->getType() != eVMMetaEntryType::dataRequest)
		return false;
	if (sections[0]->getEntries()[0]->getFields().size() == 0)
		return false;

	generator.reset();
	generator.beginSection(eVMMetaSectionType::notifications);
	generator.addNotification(eNotificationType::notification, "Question", "yes or no?");
	generator.endSection();


	return true;

}

bool CTests::testConsole()
{
	uint32_t counter = 0;
	uint32_t g = 0;
	std::string answerR;
	while (counter < 300)
	{

		mTools->writeLine("New message to display (WriteLine)" + std::to_string(counter));
		mTools->flashLine("New message to flash (FlashLine)" + std::to_string(counter));
		std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		bool answer = mTools->askYesNo("yes or no?", true);
		answerR = answer ? "True" : "False";
		mTools->writeLine("The answer was: " + answerR);
		std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		g = static_cast<uint64_t>(mTools->askInt("Tell me the numer?", 1));
		mTools->writeLine("The number was:" + std::to_string(g));

		std::string str = mTools->askString("your name?", "unknown", "System");
		mTools->writeLine("Your name is:" + str);
		std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		counter++;

	}
	return true;
}
bool CTests::testTransactionsDataStructure()
{
	CTransaction trans;
	Botan::secure_vector<uint8_t> privKey;
	std::vector<uint8_t> pubKey;

	mCryptoFactory->genKeyPair(privKey, pubKey);
	trans.sign(privKey);
	std::vector<uint8_t> data = trans.getPackedData();
	bool lfg = false;
	CTransaction* t = CTransaction::instantiate(data, eBlockchainMode::eBlockchainMode::LocalData);
	bool result = false;
	if (
		trans.getErgLimit() == t->getErgLimit() &&
		trans.getErgPrice() == t->getErgPrice() &&
		trans.getExtData() == t->getExtData() &&
		trans.getTime() == t->getTime() &&
		trans.getCode() == t->getCode() &&

		std::memcmp(trans.getPackedData().data(),
			t->getPackedData().data(), t->getPackedData().size()) == 0)
	{
		result = true;
	}
	result = trans.verifySignature(pubKey);

	return result;
}

bool CTests::initializeStateDB(uint32_t nrOfRandomStateDomains)
{
	assertGN(!mTransactionsManager->getIsInFlow());
	assertGN(!mStateDomainManager->isInFlow());
	mTools->writeLine("Initalizing DB.");

	//mTransactionsManager->startFlow();
	if (nrOfRandomStateDomains > 0)
		mTools->writeLine("Nr. of random State Domains to generate: " + std::to_string(nrOfRandomStateDomains));
	size_t nodeCount = 0;

	if (mStateDB->getRoot() != NULL && !mStateDB->isEmpty())
	{
		testSDIDs.clear();

		mTools->writeLine("State TrieRoot already found in cold storage. Testing.\n This might take some time.");
		mStateDB->testTrie(nodeCount);
		testSDIDs = mStateDB->getKnownNodeIDs();
	}
	else
	{
		mTools->writeLine("State TrieRoot is missing.\n Need to generate a new test-trie.");

	}
	if ((nodeCount < nrOfRandomStateDomains) || testSDIDs.size() < nrOfRandomStateDomains)
	{
		if (testSDIDs.size() < nrOfRandomStateDomains)
			mTools->writeLine("The number of detected StateDomains is lower than expected.");

		genTestStateDB(nrOfRandomStateDomains, mStateDB, testSDIDs);

		mTools->writeLine("Database generated. Initial validation commencing now.");

		assertGN(mStateDomainManager->getKnownDomainsCount() == nrOfRandomStateDomains);
		mTools->writeLine("Initial validation was Successfull");
	}
	else
	{
		mTools->writeLine("[GOOD] Nr. of nodes in Cold Storage: " + std::to_string(nrOfRandomStateDomains));
		//mStateDB->setCurrentTrieRoot(mStateDB->getRoot());
	}
	//mTransactionsManager->endFlow(true);
	return true;
}

bool CTests::testX25519ChaCha()
{
	Botan::secure_vector<uint8_t> privKey;
	std::vector<uint8_t> pubKey;
	std::vector<uint8_t> recovered;
	std::vector<uint8_t> plaintext = mTools->genRandomVector(1024);
	mCryptoFactory->genKeyPair(privKey, pubKey);

	//ephemeral key-pair - BEGIN
	std::vector<uint8_t> ciphertext = mCryptoFactory->encChaCha20c25519(pubKey, plaintext, true);

	recovered = mCryptoFactory->decChaCha20c25519(privKey, ciphertext);



	if (!mTools->compareByteVectors(plaintext, recovered))
	{
		return false;
	}

	//ephemeral key-pair - END

	//static key-pair - BEGIN

	Botan::secure_vector<uint8_t> staticPrivKey, sessionKey, retrievedSessionKey;
	std::vector<uint8_t> staticPubKey;
	mCryptoFactory->genKeyPair(staticPrivKey, staticPubKey);

	ciphertext = mCryptoFactory->encChaCha20c25519(pubKey, plaintext, true, sessionKey, std::vector<uint8_t>(), staticPrivKey);//nonce to be generated autonomously

	recovered = mCryptoFactory->decChaCha20c25519(privKey, ciphertext, retrievedSessionKey);


	if (!mTools->compareByteVectors(plaintext, recovered))
	{
		return false;
	}
	if (!mTools->compareByteVectors(retrievedSessionKey, sessionKey))
	{
		return false;
	}
	//static key-pair - END

	return true;

}
bool CTests::testColdStorage()
{

	mTools->writeLine("\nCold-Storage specific tests commencing..");
	CTrieNode* root = mStateDB->getRoot();
	mTools->writeLine("Clearing DB RAM cache..");
	mStateDB->pruneTrie();
	mTools->writeLine("ok");

	uint32_t count = 0;
	mTools->writeLine("testing..");

	CTrieNode* found;
	//db.testTrie(count);
	int progress = 0;
	int previousFlashed = 0;
	mTools->flashLine("Cold-Storage Tests: " + std::to_string((int)progress) + "%");
	for (int i = 0; i < testSDIDs.size(); i++)
	{
		progress = (int)(((double)i / (double)testSDIDs.size()) * 100);
		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("Cold-Storage Tests: " + std::to_string((int)progress) + "%");
		}


		found = mStateDB->findNodeByFullID(testSDIDs[i]);
		//check all the current leaf-nodes (their addresses)

	  //check state-domains under these addresses to check if something got misplaced or for error in addressing within Trie


		assertGN(found != nullptr);
		if (!(std::memcmp(static_cast<CStateDomain*>(found)->getAddress().data(), testSDIDs[i].data(), 32) == 0))
		{
			assertGN(false);//this should detect when a state-domain got misplaced within the trie
		}

		//end test retrieval
		if (found != NULL)
		{
			count++;
			//delete found;
		}
		else
			return false;
	}
	progress = 100;
	mTools->flashLine("Cold-Storage Tests: " + std::to_string((int)progress) + "%");


	if (count == testSDIDs.size())
	{
		if (progress == 100)
			mTools->writeLine("GOOD.");
		return true;
	}
	else {
		return false;
	}
}

void CTests::clearRAMCache()
{
	mStateDB->clearRAMCache();
}

bool CTests::mainTests()
{

	CTrieNode* node = NULL;
	mTools->writeLine();

	std::vector<uint8_t> ID;
	CTrieNode* currentNode;
	bool rmn;
	bool ln;

	int progress = 0;
	int previousFlashed = 0;

	mTools->flashLine("Main DB Tests: " + std::to_string((int)progress) + "%");
	for (int i = 0; i < testSDIDs.size(); i++)
	{
		progress = (int)(((double)i / (double)testSDIDs.size()) * 100);

		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("Main DB Tests: " + std::to_string((int)progress) + "%");
		}
		rmn, ln = false;

		currentNode = mStateDomainManager->findByID(testSDIDs[i]);
		assertGN(std::memcmp(static_cast<CStateDomain*>(currentNode)->getAddress().data(), testSDIDs[i].data(), 32) == 0);
		if (currentNode == NULL)
			assertGN(currentNode != NULL);
	}

	progress = 100;
	mTools->flashLine("Main DB Tests: " + std::to_string((int)progress) + "%");

	if (progress == 100)
		mTools->writeLine("GOOD.");
	return true;
}
bool CTests::testDB()
{
	size_t count = 0;
	if (mStateDB->testTrie(count) && count > 0)
		return true;
	return false;
}
bool CTests::testCTrieDBCopy()
{
	CTrieDB* dbA = mTransactionsManager->getFlowDB();
	CTrieDB dbB = *dbA;
	dbB.pruneTrie();
	static_cast<CTrieBranchNode*>(dbB.getRoot())->clear();
	dbA->copyTo(&dbB, true);
	size_t aS = 0;
	dbA->testTrie(aS);
	size_t bS = 0;
	dbB.testTrie(bS);
	if (aS != bS)
		return false;
	static_cast<CTrieBranchNode*>(dbB.getRoot())->prepare(false, true);
	static_cast<CTrieBranchNode*>(dbA->getRoot())->prepare(false, true);
	if (std::memcmp(dbA->getPerspective().data(), dbB.getPerspective().data(), 32) != 0)
		return false;
	return true;
}

bool CTests::testMerkleProofs()
{
	//stateDB->pruneTrieBelow(stateDB->getRoot(), stateDB->getRoot(), false);
	//size_t t = IDs.size();
	//stateDB->testTrie(t);
	mTools->writeLine();
	uint32_t count = 0;
	bool result = false;
	CTrieNode* node;

	int progress = 0;
	int previousFlashed = 0;
	bool introduceError = false;
	mTools->flashLine("Merkle-Patricia Trie Proofs Test: " + std::to_string((int)progress) + "%");
again:
	for (int i = 0; i < testSDIDs.size(); i++)
	{
		progress = (int)(((double)i / (double)testSDIDs.size()) * 100);
		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			if (!introduceError)
				mTools->flashLine("Merkle-Patricia Trie Proofs Test: " + std::to_string((int)progress) + "%");
			else
				mTools->flashLine("Merkle-Patricia Trie FALSE-Proofs Test: " + std::to_string((int)progress) + "%");
		}
		node = mStateDB->findNodeByFullID(testSDIDs[i]);
		//TODO:create markleproof while traversing Trie from top to bottom
		//TODO: support mutliple roots,slect which root to modify
		std::vector<uint8_t> packedPath = mStateDB->getPackedMerklePathTopBottom(mTools->bytesToNibbles(testSDIDs[i]));


		result = mStateDB->verifyPackedMerklePath(packedPath, mStateDB->getRoot()->getHash(), introduceError);
		if (!result && introduceError == false)
			return false;
		else if (result && introduceError)
			return false;


	}
	if (introduceError == false)
	{
		progress = 100;
		mTools->flashLine("Merkle-Patricia Trie Proofs Test: " + std::to_string((int)progress) + "%");

		introduceError = true;
		mTools->writeLine("\nCommencing FALSE-Proofs Testing, Now.\n");
		progress = 0; previousFlashed = 0;
		goto again;
	}

	progress = 100;
	mTools->writeLine("Merkle-Patricia Trie Proofs Test: " + std::to_string((int)progress) + "%");
	if (progress == 100)
		mTools->writeLine("GOOD.");
	return true;

}

bool CTests::testTrieStructure()
{
	size_t nrOfNodesFound; //the total number of nodes stepped upon
	if (mStateDB->testTrie(nrOfNodesFound) == false)
		return false;

	return true;
}

bool CTests::testTrieWithDBBackend()
{
	return false;
}

bool CTests::genTestStateDB(uint32_t stateDomainsNr, CTrieDB* stateDB, std::vector<std::vector<uint8_t>>& testSDIDs)
{
	assertGN(!mTransactionsManager->getIsInFlow());
	assertGN(!mStateDomainManager->isInFlow());

	mTools->writeLine("Generating a Sandbox StateDB");

	std::vector<uint8_t> idd;
	std::vector<uint8_t> reportedid;
	std::vector<std::vector<uint8_t>> byteIDs;
	std::vector<uint8_t> randomData;// = mTools->genRandomVector(64);
	std::vector<CKeyChain> chains;
	//generate key-chains for each State Domain
	if (testSDIDs.size() >= stateDomainsNr)
	{
		mTools->writeLine("Nothing to generate");
		return false;
	}
	mTransactionsManager->startFlow();
	uint64_t nrOfDomainsToGenerate = stateDomainsNr - testSDIDs.size();
	mTools->writeLine("Generating pub/priv key pairs for State Domains..(" + std::to_string(nrOfDomainsToGenerate) + ")");
	int progress = 0;
	int previousFlashed = 0;
	for (int i = 0; i < nrOfDomainsToGenerate; i++)
	{
		progress = (int)(((double)i / (double)nrOfDomainsToGenerate) * 100);
		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("Progress: " + std::to_string((int)progress) + "%");
		}
		CKeyChain chain; std::vector<uint8_t> address;
		mCryptoFactory->genAddress(chain.getPubKey(), address);
		mSettings->saveKeyChain(chain.getPackedData(), std::string(address.begin(), address.end()));
		testSDIDs.push_back(address);
		chains.push_back(chain);
		assertGN(address.size() == 34 || address.size() == 33);
	}

	for (int i = 0; i < testSDIDs.size(); i++)
	{
		byteIDs.push_back(testSDIDs[i]);
	}
	mSS->saveByteVectors("testIDs", byteIDs);

	progress = 0;
	previousFlashed = 0;
	mTools->writeLine();
	bool rmn = false;
	std::vector<uint8_t> code = mTools->genRandomVector(32);

	CStateDomain* acc;

	mTools->flashLine("Database generated: " + std::to_string((int)progress) + "%");
	for (int i = 0; i < testSDIDs.size(); i++)
	{
		//randomData = mTools->genRandomVector(51);

		progress = (int)(((double)i / (double)testSDIDs.size()) * 100);
		if (progress - previousFlashed > 0)
		{
			previousFlashed = progress;
			mTools->flashLine("Database generated: " + std::to_string((int)progress) + "%");
		}
		std::vector<uint8_t> perspective;
		assertGN(mStateDomainManager->create(perspective, &acc, false, byteIDs[i]));





		//acc = new CStateDomain(byteIDs[i],mStateDB->getPerspective(), randomData,true);
		CKeyChain chain;

		//acc->prepare(true);

		//CTrieDB::eResult  res = stateDB->addNode(mTools->bytesToNibbles(testSDIDs[i]), acc);
		//size_t ct = 0;
		//CTrieNode * retrieved = mStateDomainManager->findByID(testSDIDs[i]);
		//assert(retrieved != NULL);
		//std::vector<uint8_t> rootHash = stateDB->getPerspective();

	}
	std::vector<uint8_t> finalPerspective = mTransactionsManager->getPerspective(true);


	mTransactionsManager->endFlow(true, false, true);

	finalPerspective = mTransactionsManager->getPerspective(true);
	assertGN(mTransactionsManager->setPerspective(finalPerspective));
	//stateDB->forceRootNodeStorage();
	progress = 100;
	mTools->flashLine("Database generated: " + std::to_string((int)progress) + "%");
	return true;
}

bool CTests::testComputeTaskLayer()
{

	bool keepRunning = true;
	srand(time(NULL));

	//TODO

	return true;
}

/// <summary>
/// It will test proper generation and deserialization of Token Pools, including nested TokenPoolBanks (dimensions).
/// </summary>
/// <returns></returns>
bool CTests::testTokenPoolGenerationAndSerialization()
{
	//non-athenticated token-pool - begin
	std::shared_ptr<CTokenPool> pool = CTokenPool::getNew(mCryptoFactory, 100, mTools->genRandomVector(32),
		mTools->genRandomVector(32), 1, 100000, 0, "My Pool");

	if (!pool->generateDimensions())
		return false;

	std::vector<uint8_t> packedData = pool->getPackedData();
	std::shared_ptr<CTokenPool> retrievedPool = CTokenPool::instantiate(packedData, mCryptoFactory);

	if (retrievedPool == nullptr || *retrievedPool != *pool)
		return false;

	//non-athenticated token-pool - end

	//athenticated token-pool - begin
	pool = CTokenPool::getNew(mCryptoFactory, 100, mTools->genRandomVector(32),
		mTools->genRandomVector(32), 1, 100000, 0, "My Authenticated Pool");

	pool->setPubKey(mTools->genRandomVector(32));

	if (!pool->generateDimensions())
		return false;

	packedData = pool->getPackedData();
	retrievedPool = CTokenPool::instantiate(packedData, mCryptoFactory);

	if (retrievedPool == nullptr || *retrievedPool != *pool)
		return false;
	//athenticated token-pool - end
	return true;
}

bool CTests::testWebRTCSwarms()
{
	uint64_t maxSwarmMembers = 10;
	CWebRTCSwarm swarm(nullptr, mTools->genRandomVector(32), nullptr, maxSwarmMembers, 15);

	CWebRTCSwarmMember member;

	for (uint64_t i = 0; i < maxSwarmMembers + 1; i++)
	{

		std::shared_ptr< CWebRTCSwarmMember> member = std::make_shared< CWebRTCSwarmMember>();
		swarm.addMember(member);
		member->ping();
	}

	if (swarm.getMembers().size() > maxSwarmMembers)
		return false;
	Sleep(20000);//test auto-removal
	for (uint64_t i = 0; i < maxSwarmMembers / 2; i++)
	{
		swarm.getMembers()[i]->ping();
	}


	swarm.doMaintenance();
	if (swarm.getMembers().size() != maxSwarmMembers / 2)
		return false;

	Sleep(20000);//test auto-removal
	swarm.doMaintenance();
	if (swarm.getMembers().size() != 0)
		return false;

	return true;


}
bool CTests::testSDPEntitiesSerializationAndAuth()
{

	Botan::secure_vector<uint8_t> privKey, privKey2;
	std::vector<uint8_t> pubKey, pubKey2, swarmID;;
	mCryptoFactory->genKeyPair(privKey, pubKey);

	CSDPEntity sdp1(nullptr, eSDPEntityType::getOffer, swarmID, pubKey, mTools->genRandomVector(32), mTools->genRandomVector(62));

	sdp1.sign(privKey);
	std::vector<uint8_t> packedData = sdp1.getPackedData();
	std::shared_ptr< CSDPEntity> retrievedSDP = CSDPEntity::instantiate(packedData);
	if (retrievedSDP == nullptr)
		return false;

	if (*retrievedSDP != sdp1)
		return false;

	if (!sdp1.validateSignature(pubKey))
		return false;

	return true;

}

/// <summary>
/// Tests authenticated Transmission-Tokens.
/// The signature embedded within the token is validated against a Token-Pool-specific public-key.
/// In case of dicsrepancies, authentication and pay-out fails.
/// </summary>
/// <returns></returns>
bool  CTests::testAuthenticatedTransmisionTokens()
{
	Botan::secure_vector<uint8_t> privKey, privKey2;
	std::vector<uint8_t> pubKey, pubKey2;

	std::shared_ptr<CTokenPool> pool = CTokenPool::getNew(mCryptoFactory, 1, mTools->genRandomVector(32),
		mTools->genRandomVector(32), 1, 100, 0, "My Pool");

	mCryptoFactory->genKeyPair(privKey, pubKey);
	if (!pool->setPubKey(pubKey))
		return false;

	if (!pool->generateDimensions())
		return false;

	std::shared_ptr<CTransmissionToken>  token = pool->getTTWorthValue(0, 10, false);

	if (token == nullptr)
		return false;

	if (!token->sign(privKey))
		return false;

	if (!pool->validateTT(token, false))//the signature would be verified against the public-key
		//available within the Token-Pool
		return false;

	//negative test - begin
	mCryptoFactory->genKeyPair(privKey2, pubKey2);//change signing key
	if (!token->sign(privKey2))
		return false;

	if (pool->validateTT(token))
		return false;
	//negative test - end
	return true;
}
bool CTests::testTransmissionTokenSerialization()
{//authenticated - begin
	CTransmissionToken token = CTransmissionToken(mTools->genRandomNumber(0, 100), mTools->genRandomNumber(0, 100), mTools->genRandomVector(32),
		mTools->genRandomNumber(0, 100), mTools->genRandomNumber(0, 100), mTools->genRandomVector(32), mTools->genRandomVector(32),
		mTools->genRandomVector(32));

	token.setRecipient(mTools->genRandomVector(32));
	std::vector<uint8_t> serializedData = token.getPackedData();

	std::shared_ptr<CTransmissionToken> retrievedToken = CTransmissionToken::instantiate(serializedData);


	if (retrievedToken == nullptr)
		return false;

	if (*retrievedToken != token)
		return false;


	//authenticated - end

	//non-authenticated - begin
	CTransmissionToken token2 = CTransmissionToken(mTools->genRandomNumber(0, 100), mTools->genRandomNumber(0, 100), mTools->genRandomVector(32),
		mTools->genRandomNumber(0, 100), mTools->genRandomNumber(0, 100), mTools->genRandomVector(32), mTools->genRandomVector(32),
		mTools->genRandomVector(32));

	serializedData = token2.getPackedData();

	retrievedToken = CTransmissionToken::instantiate(serializedData);


	if (retrievedToken == nullptr)
		return false;

	if (*retrievedToken != token2)
		return false;
	//non-authenticated - end

	return true;
}

/// <summary>
/// Tests Multi-Dimensional Token-Pools and Transmission Tokens.
/// Does NOT test integration of these with the decentralized VM.
/// </summary>
/// <returns></returns>
bool CTests::testTransmissionTokenUtilization(bool doCompletenessTest)
{
	mTools->writeLine("Transmission-Token Utilization Test Commencing..");
	//Local Variables - BEGIN
	uint64_t agentsCount = 10;
	BigInt tossedValue = 0;
	uint64_t tossedBank = 0;
	std::vector<uint8_t> sP;
	uint64_t iterationsCount = 0;
	BigInt totalValueUsedUp = 0;
	bool passOnIt = false;
	bool ifNotEnoughGiveWhatAvailable = true;
	BigInt expectedValue = 0;
	Botan::secure_vector<uint8_t> secret;
	BigInt retrievedValue = 0;
	uint64_t dimensionsCount = 100;
	bool doInRange = true;
	BigInt totalValue = 10000;
	std::shared_ptr <CTransmissionToken> token;
	std::vector<uint8_t> previousHash;
	//bool doCompletenessTest = false;
	std::vector<uint64_t> availableBanks;
	std::shared_ptr<CTokenPool> pool;
	std::shared_ptr<CTransmissionToken> tt;
	BigInt v = 0;
	BigInt totalInitialValueLeft = 0;
	//Local Variables - END


	/*
	Enable
	*/

	if (doCompletenessTest)
	{

		pool = CTokenPool::getNew(mCryptoFactory, dimensionsCount, mTools->genRandomVector(32),
			mTools->genRandomVector(32), 1, totalValue, 0, "My Pool");

		mTools->writeLine("Generating Token Pool Dimensions / Banks..");
		pool->generateDimensions();

		//Test Retrieval From ColdStorage - BEGIN
		tt = pool->getTTWorthValue(0, 1, false);
		if (tt == nullptr)
			return false;
		if (!pool->validateTT(tt, false))
		{
			return false;
		}
		secret = pool->getSeedHash();
		sP = pool->getPackedData(false);
		pool = CTokenPool::instantiate(sP, mCryptoFactory);
		pool->setMasterSeedHash(secret);//all dimensions should get updated
		tt = pool->getTTWorthValue(0, 1, false);
		v = pool->validateTT(tt, false);
		if (v != 1)
			return false;
		v = pool->validateTT(tt, true);
		if (v != 1)
			return false;
		v = pool->validateTT(tt, true);//double spend
		if (v != 0)//negative test
			return false;
		//Test Retrieval From ColdStorage - END

		//basic 1-time token Test 1 - BEGIN
		mTools->writeLine("Using Token Pool\n" + pool->getInfo("\n"));

		expectedValue = mTools->genRandomNumber(1, 100);

		//do not update state this time
		token = pool->getTTWorthValue(0, expectedValue, false);

		if (token == nullptr)
			return false;

		retrievedValue = pool->validateTT(token, false);

		if (retrievedValue != expectedValue)
			return false;

		//now let us try again this time updating the pool's state 

		previousHash = token->getRevealedHash();
		token = pool->getTTWorthValue(0, expectedValue, false);

		//the hash-values should be the same (same token)
		if (!mTools->compareByteVectors(previousHash, token->getRevealedHash()))
			return false;

		retrievedValue = pool->validateTT(token, true);

		if (retrievedValue != expectedValue)
			return false;
		//test against a double-spend

	//now let us try to perform a double spend (should be marked as used-up by now)
		retrievedValue = pool->validateTT(token, true);

		if (retrievedValue > 0)
			return false;

		//basic 1-time Token Test 1 - END

		//multiple concurent agents Test 2 - BEGIN

		//multiple concurent agents are to be rewarded with tokens
		//from the same token-pool, values tossed on random.
		//agents NOT tied to particular token pool-dimensions
		//the test will commence untill token-pool and all of its internal banks are depleted.


		for (uint64_t i = 0; i < pool->getDimensionsCount(); i++)
			availableBanks.push_back(i);

		passOnIt = false;
		ifNotEnoughGiveWhatAvailable = true;
		totalInitialValueLeft = pool->getValueLeft();

		while (pool->getStatus() != eTokenPoolStatus::depleted)
		{
			if (availableBanks.size() == 0)
				break;
			passOnIt = false;
			tossedValue = mTools->genRandomNumber(1, 10);
			if (availableBanks.size() == 1)
				tossedBank = 0;
			else
				tossedBank = availableBanks[mTools->genRandomNumber(1, availableBanks.size() - 1)];
		tryAgain:
			token = pool->getTTWorthValue(tossedBank, tossedValue, false);

			if (token == nullptr)
			{
				if (pool->getBankStatus(tossedBank) == eTokenPoolBankStatus::depleted)
				{
					if (pool->getValueLeftInBank(tossedBank) == 0)//verify
					{
						mTools->writeLine("Bank " + std::to_string(tossedBank) + " depleted!");

						std::vector<uint64_t>::iterator position = std::find(availableBanks.begin(), availableBanks.end(), tossedBank);
						if (position != availableBanks.end()) // == myVector.end() means the element was not found
							availableBanks.erase(position);

						passOnIt = true;
					}
					else return false;//this should not happen
				}
				else if (pool->getBankStatus(tossedBank) == eTokenPoolBankStatus::active && pool->getValueLeftInBank(tossedBank) < tossedValue)
				{
					mTools->writeLine("Not enough assets (" + std::to_string(tossedBank) + " required, "
						+ pool->getValueLeftInBank(tossedBank).str() + " GBUs left) within bank " + std::to_string(tossedBank));
					if (!ifNotEnoughGiveWhatAvailable)
						passOnIt = true;
					else
					{
						mTools->writeLine("giving what available..");
						tossedValue = pool->getValueLeftInBank(tossedBank);
						goto tryAgain;
					}
				}
				else
				{
					return false;//this should not happen
				}
			}
			else
				mTools->writeLine("Rewarded " + token->getValue().str() + " GBUs from bank " + std::to_string(token->getBankID()) + "!");

			if (passOnIt)
				continue;

			if (pool->validateTT(token) != tossedValue)//did we get what we wanted?
				return false;

			totalValueUsedUp += tossedValue;//total value used-up during this session

			iterationsCount++;

			if (iterationsCount > (pool->getDimensionsCount() * pool->getDimensionDepth()))//dead-lock + logic check
				return false;//this should have never happened
		}

		if (totalValueUsedUp != totalInitialValueLeft) //total token-pool value should be used-up by now
			return false;//and the Token-Pool's state should be set as 'Depleted' also of all its internal banks.
		if (pool->getStatus() != eTokenPoolStatus::depleted)
			return false;

		//multiple concurent agents Test 2 - END
	}
	else
	{

		//Big Numbers Support - BEGIN
		mTools->writeLine(mTools->getColoredString("Big Values Token Pool test commencing...", eColor::lightCyan));
		Sleep(2000);
		agentsCount = 10;
		tossedValue = 0;
		tossedBank = 0;
		iterationsCount = 0;
		totalValueUsedUp = 0;
		expectedValue = 0;
		secret.clear();
		retrievedValue = 0;
		availableBanks.clear();
		dimensionsCount = 100;
		totalValue = MAXUINT64;
		totalValue = totalValue * 10;
		token = nullptr;
		previousHash.clear();
		BigInt singleTokenValue = totalValue / 100;
		pool = CTokenPool::getNew(mCryptoFactory, dimensionsCount, mTools->genRandomVector(32),
			mTools->genRandomVector(32), totalValue / 100, totalValue, 0, "My Pool");

		mTools->writeLine("Generating Token Pool Dimensions / Banks..");
		pool->generateDimensions();

		//Test Retrieval From ColdStorage - BEGIN
		tt = pool->getTTWorthValue(0, 1, false);
		if (tt == nullptr)
			return false;
		if (!pool->validateTT(tt, false))
		{
			return false;
		}
		secret = pool->getSeedHash();
		sP = pool->getPackedData(false);
		pool = CTokenPool::instantiate(sP, mCryptoFactory);
		pool->setMasterSeedHash(secret);//all dimensions should get updated
		tt = pool->getTTWorthValue(0, 1, false);//not: this would generate a Token which is worth AT LEAST was required
		v = pool->validateTT(tt, false);
		std::string temp = v.str();
		if (v == 0)
			return false;

		if (v < singleTokenValue)
			return false;
		v = pool->validateTT(tt, true);
		if (v == 0)
			return false;

		if (v < singleTokenValue)
			return false;
		v = pool->validateTT(tt, true);//double spend
		if (v != 0)//negative test
			return false;
		//Test Retrieval From ColdStorage - END

		//basic 1-time token Test 1 - BEGIN
		mTools->writeLine("Using Token Pool\n" + pool->getInfo("\n"));

		expectedValue = mTools->genRandomNumber(1, 100);

		//do not update state this time
		token = pool->getTTWorthValue(1, expectedValue, false);

		if (token == nullptr)
			return false;

		retrievedValue = pool->validateTT(token, false);

		if (retrievedValue < expectedValue || retrievedValue> singleTokenValue)
			return false;

		//now let us try again this time updating the pool's state 

		previousHash = token->getRevealedHash();
		token = pool->getTTWorthValue(1, expectedValue, false);

		//the hash-values should be the same (same token)
		if (!mTools->compareByteVectors(previousHash, token->getRevealedHash()))
			return false;

		retrievedValue = pool->validateTT(token, true);

		if (retrievedValue <  expectedValue || retrievedValue> singleTokenValue)
			return false;
		//test against a double-spend

		//now let us try to perform a double spend (should be marked as used-up by now)
		retrievedValue = pool->validateTT(token, true);

		if (retrievedValue > 0)
			return false;

		//basic 1-time Token Test 1 - END

		//multiple concurent agents Test 2 - BEGIN

		//multiple concurent agents are to be rewarded with tokens
		//from the same token-pool, values tossed on random.
		//agents NOT tied to particular token pool-dimensions
		//the test will commence untill token-pool and all of its internal banks are depleted.


		for (uint64_t i = 0; i < pool->getDimensionsCount(); i++)
			availableBanks.push_back(i);

		passOnIt = false;
		ifNotEnoughGiveWhatAvailable = true;
		totalInitialValueLeft = pool->getValueLeft();
		bool notEnoughInBank = false;
		while (pool->getStatus() != eTokenPoolStatus::depleted)
		{
			notEnoughInBank = false;
			if (availableBanks.size() == 0)
				break;
			passOnIt = false;
			doInRange = mTools->genRandomNumber(0, 2);
			tossedValue = (doInRange == false) ? mTools->genRandomNumber(static_cast<uint64_t>(singleTokenValue + 1), static_cast<uint64_t>(singleTokenValue + (singleTokenValue / 10)))
				: mTools->genRandomNumber(static_cast<uint64_t>(singleTokenValue - (singleTokenValue / 10)), static_cast<uint64_t>(singleTokenValue));

			//tossedValue = tossedValue * 10;
			if (availableBanks.size() == 1)
				tossedBank = 0;
			else
				tossedBank = availableBanks[mTools->genRandomNumber(1, availableBanks.size() - 1)];
		tryAgain2:
			mTools->writeLine(mTools->getColoredString("Requesting ", eColor::blue) + mTools->getColoredString(tossedValue.str(), eColor::lightCyberWine) + mTools->getColoredString(" Atto Tokens from bank ", eColor::blue) + std::to_string(tossedBank) + ".");
			token = pool->getTTWorthValue(tossedBank, tossedValue, false);

			if (token == nullptr)
			{
				if (pool->getBankStatus(tossedBank) == eTokenPoolBankStatus::depleted)
				{
					if (pool->getValueLeftInBank(tossedBank) == 0)//verify
					{
						mTools->writeLine(mTools->getColoredString("Bank " + std::to_string(tossedBank) + " depleted!", eColor::lightPink));

						std::vector<uint64_t>::iterator position = std::find(availableBanks.begin(), availableBanks.end(), tossedBank);
						if (position != availableBanks.end()) // == myVector.end() means the element was not found
							availableBanks.erase(position);

						passOnIt = true;
					}
					else return false;//this should not happen
				}
				else if (pool->getBankStatus(tossedBank) == eTokenPoolBankStatus::active && pool->getValueLeftInBank(tossedBank) < tossedValue)
				{
					notEnoughInBank = true;

					mTools->writeLine("Not enough assets (" + tossedValue.str() + " required, "
						+ pool->getValueLeftInBank(tossedBank).str() + " GBUs left) within bank " + std::to_string(tossedBank));
					if (!ifNotEnoughGiveWhatAvailable)
						passOnIt = true;
					else
					{
						mTools->writeLine(mTools->getColoredString("Giving what available..", eColor::orange));
						tossedValue = pool->getValueLeftInBank(tossedBank);
						goto tryAgain2;
					}
				}
				else
				{
					return false;//this should not happen
				}
			}
			else
				mTools->writeLine(mTools->getColoredString("Properly Rewarded. " + token->getValue().str() + " GBUs from bank " + std::to_string(token->getBankID()) + "!", eColor::lightGreen));

			if (passOnIt)
				continue;

			retrievedValue = pool->validateTT(token);
			if (retrievedValue <= tossedValue || retrievedValue > singleTokenValue)//did we get what we wanted?
			{
				if (!notEnoughInBank)
					return false;
			}
			totalValueUsedUp += tossedValue;//total value used-up during this session

			iterationsCount++;

			if (iterationsCount > (pool->getDimensionsCount() * pool->getDimensionDepth()))//dead-lock + logic check
				return false;//this should have never happened
		}

		//if (totalValueUsedUp != totalInitialValueLeft) //total token-pool value should be used-up by now
		///	return false;//and the Token-Pool's state should be set as 'Depleted' also of all its internal banks.
		if (pool->getStatus() != eTokenPoolStatus::depleted)
			return false;
		mTools->writeLine(mTools->getColoredString("Big Values Token Pool test SUCCEEDED...", eColor::lightCyan));
		Sleep(2000);
		//Big Numbers Support - END
	}

	mTools->writeLine("Transmission-Token Utilization Test Succeeded!");
	return true;
}
bool CTests::testTransmissionTokenTransactions()
{
	Botan::secure_vector<uint8_t> privKey;
	std::vector<uint8_t> pubKey;
	std::string transactionSourceCode;
	BigInt tokenValue = 10;

	std::shared_ptr<CTokenPool> pool = CTokenPool::getNew(mCryptoFactory, 1, mTools->genRandomVector(32),
		mTools->genRandomVector(32), 1, 100, 0, "My Pool");

	mCryptoFactory->genKeyPair(privKey, pubKey);
	if (!pool->setPubKey(pubKey))
		return false;

	if (!pool->generateDimensions())
		return false;

	std::shared_ptr<CTransmissionToken>  token = pool->getTTWorthValue(0, tokenValue, false);

	if (token == nullptr)
		return false;

	if (!token->sign(privKey))
		return false;

	BigInt retrievedTokenValue = pool->validateTT(token, false);//the signature would be verified against the public-key
	//available within the Token-Pool
	if (tokenValue != retrievedTokenValue)
		return false;

	//prepare a #GridScript transaction 
	transactionSourceCode = "xtt -t " + mTools->base58CheckEncode(pool->getPackedData(true));


	///Big Values Support - BEGIN

	BigInt bigTotalValue = MAXUINT64;
	bigTotalValue = bigTotalValue * 4;

	pool = CTokenPool::getNew(mCryptoFactory, 1, mTools->genRandomVector(32),
		mTools->genRandomVector(32), MAXUINT64, bigTotalValue, 0, "My Pool of Big Values");

	tokenValue = MAXUINT64;

	mCryptoFactory->genKeyPair(privKey, pubKey);
	if (!pool->setPubKey(pubKey))
		return false;

	if (!pool->generateDimensions())
		return false;

	token = pool->getTTWorthValue(0, tokenValue, false);

	if (token == nullptr)
		return false;

	if (!token->sign(privKey))
		return false;

	retrievedTokenValue = pool->validateTT(token, false);//the signature would be verified against the public-key
	//available within the Token-Pool
	if (tokenValue != retrievedTokenValue)
		return false;

	//prepare a #GridScript transaction 
	transactionSourceCode = "xtt -t " + mTools->base58CheckEncode(pool->getPackedData(true));

	//Big Values - Negative Test - BEGIN

	tokenValue = tokenValue * 10;
	token = pool->getTTWorthValue(0, tokenValue, false);

	retrievedTokenValue = pool->validateTT(token, false);

	if (tokenValue == retrievedTokenValue)
		return false;
	//Big Values - Negative Test - END

	///Big Values Support - END

	return true;
}
std::vector<uint8_t> CTests::getPerspective()
{
	return mStateDB->getPerspective();
}
bool CTests::testOpenCLPlatform(std::shared_ptr<CWorkManager> wmP)
{
	CTools::setIsOpenCLEvaluationActive(true);

	struct OpenCLEvaluationGuard {
		~OpenCLEvaluationGuard() {
			// Reset OpenCL evaluation state when going out of scope
			CTools::setIsOpenCLEvaluationActive(false);
		}
	};
	OpenCLEvaluationGuard guard;
	std::vector<std::shared_ptr<IWorkResult>> results;
	std::shared_ptr<CWorkManager> workManager = wmP;

	//Pre-Flight - BEGIN
	if (!workManager)
	{
		workManager = mWorkManager;
	}
	if (!workManager)
	{
		return false;
	}
	//Pre-Flight - END

	//Local Variables - BEGIN
	uint64_t maxPartialTaskMultiplier = 10;
	bool keepRunning = true;
	uint64_t tasksPerUnit = static_cast<uint64_t>(mTools->askInt("Amount of test mining tasks?", 3, "GPU Diagnostics", true, false, 10));
	uint64_t difficultyMultiplier = static_cast<uint64_t>(mTools->askInt("Difficulty Multiplier?", 1, "GPU Diagnostics", true, false, 10));
	uint64_t tasksCount = tasksPerUnit;
	mTools->writeLine("Work Manager (OpenCL platform) Test Commencing now..");

	if (workManager)
	{
		tasksCount = workManager->getWorkers().size() * tasksPerUnit;
	}
	mTools->writeLine("Proceeding with " + std::to_string(tasksCount) + " OpenCL tasks (" + std::to_string(tasksPerUnit) + " tasks per device).");

	srand(time(NULL));
	mTools->writeLine("Work Manager (OpenCL platform) Test Commencing now..");

	mTools->writeLine("Max testing time: 3 minutes or until mined.");

	Sleep(1000);
	if (!workManager->wasInitialised())
		workManager->Initialise();
	size_t startTime = mTools->getTime();
	size_t startTimeAllReady = mTools->getTime();//gets reset if kernels not compiled.
	arith_uint256 diff = 0;
	

	if (difficultyMultiplier > maxPartialTaskMultiplier)
	{
		mTools->writeLine("Multiplier for partial tasks exceeded. I'll assume " + std::to_string(maxPartialTaskMultiplier));
	}

	double mainTargetDiff = 10 * 10 * difficultyMultiplier;//100*100*5; // IMPORTANT: on 4090 there are issues if we make difficulty any lower. why?
	double minTargetDiff = 10 * min(difficultyMultiplier, maxPartialTaskMultiplier);//IMPORTANT: on 4090 there are issues if we make difficulty any lower. why?

	mainTargetDiff += 0.10101;
	minTargetDiff += 0.10101;
	uint32_t x = 0x1f00ffff;
	uint64_t dispatchedTasks = 0;
	std::vector<uint8_t> targetMainBV = mTools->diff2target(mainTargetDiff);
	std::vector<uint8_t> targetMinBV = mTools->diff2target(minTargetDiff);

	mTools->writeLine("Setting min. Test difficulty to: " + mTools->cleanDoubleStr(std::to_string(minTargetDiff), 3));
	mTools->writeLine("Setting Test difficulty to: " + mTools->cleanDoubleStr(std::to_string(mainTargetDiff), 3));

	std::vector<uint8_t> data_to_work_on;
	std::shared_ptr<CWorkUltimium> work;
	std::vector<std::shared_ptr<CWorker>> workers;

	uint32_t previouslyFlashed = 0;
	std::string statusStr = "";
	uint64_t resultsCount = 0;
	bool gotActiveWorkers = false;
	uint64_t reportedDeadlocks = 0;
	uint64_t mLastDevReportMode = 0;
	uint64_t now = std::time(0);
	uint64_t mLastDevReportModeSwitch = 0;
	eWorkState workState = eWorkState::unknown;
	eWorkState previousState = workState;
	double mhps = 0;
	CTools* tools = mTools.get();
	std::stringstream report;
	uint64_t mTimeStarted = 0;
	uint64_t lastTimeReported = 0;
	uint64_t warnings = 0;
	uint64_t lastReportedMhps = 0;
	std::vector<uint8_t> mCurrentMiningTaskID;
	bool workPrepping = false;
	bool subTaskPrepping = false;
	uint64_t compilingDevicesCount = 0;
	uint64_t compiledDevicesCount = 0;
	uint64_t runningTime = 0;
	uint64_t workersAvailable = 0;
	uint64_t lastNagAboutWontAbort = 0;
	uint64_t waitingForDevicesSince = std::time(0);
	uint64_t maxWaitForDevices = 60 * 60 * 15;// Wait up to 15 hours for kernels to be compiled.
	uint64_t lastWarningReport = 0;
	uint64_t waitingForWorkersFor = 0;
	std::shared_ptr<CWorker> assignedWorker, previousAssignedWorker;
	std::shared_ptr<CStatusBarHub> barHub = CStatusBarHub::getInstance();
	//Local Variables - END


	//Operational Logic - BEGIN

	//we need to wait for at least one computational device to become available before attempting to assess the overall performance.
	//OpenCL kernels might take many hours to compile.

	//Wait for devices - BEGIN
	mTools->writeLine("Waiting for at least one computational device to become available (this might take many hours)..", eColor::orange);

	do
	{
		now = std::time(0);

		workersAvailable = workManager->getHealthyAvailableWorkers().size();
		waitingForWorkersFor = now - waitingForDevicesSince;
		tools->flashLine(tools->getColoredString("Waiting for workers to become available..", eColor::orange) + "(" + tools->secondsToFormattedString(waitingForWorkersFor) + ")");
		Sleep(300);


	} while (!workersAvailable && (waitingForWorkersFor < maxWaitForDevices));

	//Wait for devices - END

	if (!workersAvailable)
	{
		mTools->writeLine("OpenCL test failed ! Could not prepare OpenCL devices.", eColor::cyborgBlood);
	}
	else
	{
		mTools->writeLine("Ready devices found. Proceeding further..", eColor::orange);
	}

	for (uint64_t i = 0; i < tasksCount; i++)
	{
		//initialize work object
		std::array<unsigned char, 32> workTargetMainArray, workTargetMinArray;
		std::memcpy(&workTargetMainArray[0], &targetMainBV[0], 32);
		std::memcpy(&workTargetMinArray[0], &targetMinBV[0], 32);



		work = std::make_shared<CWorkUltimium>(nullptr, eBlockchainMode::LocalData, workTargetMainArray, workTargetMinArray, 0x00000000, 0xffffffff);

		if (i == 0)
		{
			// Warm Up Task - BEGIN
			// [Rationale]: the task is merely to prepare corresponding computational devices.
			//				Only once all devices are prepared do we proceed with actual computations.
			work->setIsWarmUpTask(true);
			// Warm Up Task - END
		}

		work->markDataAsHeader();
		mCurrentMiningTaskID = work->getGUID();
		// --- Begin: Test Setup for OpenCL Work Task Using Stub Block Header Data ---
//
// Instead of using a random 80-byte vector, we create a stub block header
// exactly as in formBlock() using the public factory method.
// We use TestNet mode from the eBlockchainMode enum.

	
		eBlockInstantiationResult::eBlockInstantiationResult instResult;// for header instantiation result
		CBlockHeader::eBlockHeaderInstantiationResult bhInstResult;
		// Create a dummy block (with no parent) in TestNet mode; isKeyBlock is false.
		std::shared_ptr<CBlock> stubBlock = CBlock::newBlock(nullptr, instResult, eBlockchainMode::TestNet, false);

		// Create a block header via the public factory method.
		// (Signature: newBlockHeader(block, blockchainMode, isKeyBlock, instResult))
		std::shared_ptr<CBlockHeader> stubHeader =
			CBlockHeader::newBlockHeader(stubBlock, eBlockchainMode::TestNet, false, bhInstResult);

		// --- Populate Stub Data in the Header ---
		//
		// Set a random public key (32 bytes) as the miner’s identity.
		std::vector<uint8_t> randomPubKey = mTools->genRandomVector(32);
		stubHeader->setPubSig(randomPubKey);

		// Set a random key height (for example, between 1 and 1000).
		uint64_t randomKeyHeight = 1 + (rand() % 1000);
		stubHeader->setKeyHeight(randomKeyHeight);

		// Set a stub Parent Key-Block ID (simulate a 32-byte hash) so that it mimics real behavior.
		std::vector<uint8_t> randomParentID = mTools->genRandomVector(32);
		stubHeader->setParentKeyBlockID(randomParentID);

		// --- Mimic Proof-of-Work Input Data Creation ---
		//
		// The real formBlock() builds its PoW input data using a CDataConcatenator.
		// The fields below MUST NOT change during PoW.
		// 1. Parent Key-Block ID (if available)
		// 2. Miner’s public key
		// 3. Key height
		CDataConcatenator concat; // data based on WHICH to generate PoW

		// Retrieve the Parent Key-Block ID from the header.
		std::vector<uint8_t> parentKeyBlockID = stubHeader->getParentKeyBlockID();
		if (!parentKeyBlockID.empty())
		{
			// - Parent Key-Block CANNOT change during PoW computation.
			// - Data block CAN change (set AFTER PoW is computed).
			concat.add(parentKeyBlockID); // i.e. the entire parental key-block ID hash.
		}

		// Add the miner's public key.
		concat.add(stubHeader->getPubKey());

		// Add the key height (this value cannot change during PoW computation).
		concat.add(stubHeader->getKeyHeight());

		// The expected input for the OpenCL kernel is an 80-byte buffer.
		// If our concatenated data is less than 80 bytes, pad with zeros.
		std::vector<uint8_t> headerData = concat.getData();
		if (headerData.size() < 80)
		{
			headerData.resize(80, 0); // pad with zeros to reach 80 bytes.
		}

		// Now use the resulting headerData exactly as in formBlock().
		// This data is passed to the work task.
		work->markDataAsHeader();
		mCurrentMiningTaskID = work->getGUID();
		work->setDataToWorkOn(headerData);
		work->markAsTestWork();

		workManager->registerTask(work);

		//reset variables - BEGIN
		workPrepping = false;
		runningTime = 0;
		subTaskPrepping = false;
		startTime = mTools->getTime();
		mTools->writeLine(mTools->getColoredString("About to commence with a " + std::string(
			i == 0 ? tools->getColoredString("Warm-Up", eColor::orange) : "") + " " + " Computational Task ", eColor::lightCyan) + mTools->getColoredString(std::to_string(i + 1) + " / " + std::to_string(tasksCount), eColor::orange));
		Sleep(2000);
		previouslyFlashed = 0;
		keepRunning = true;
		statusStr = "";
		resultsCount = 0;
		std::vector<std::shared_ptr<IWork>>  subTasks;
		gotActiveWorkers = false;
		//reset variables - END
		uint64_t lastSubtaskStatusReport = 0;
		size_t timeDiff = 0;
		bool notifiedAllDevicesReady = false;
		while (keepRunning)//keep running while tests in progress
		{//the tests MAY prolong for many hours due to kernels taking long times to compile

			if (mTimeStarted)
			{
				timeDiff = std::time(0) - mTimeStarted;
			}

			workState = workManager->getWorkState(mCurrentMiningTaskID);
			now = std::time(0);
			mTools->clearLines(previouslyFlashed);
			previouslyFlashed = 0;
			workers = workManager->getWorkers();
			mhps = 0;
			workPrepping = false;
			subTaskPrepping = false;
			compilingDevicesCount = 0;
			compiledDevicesCount = 0;
			statusStr = "";
			gotActiveWorkers = false;
			std::string timeStampPrefix = "[" + tools->timeToString(now) + "]: ";

			// OpenCL Compilation Progress Tracking - BEGIN
			// Track maximum remaining compilation time and count of devices in different states
			uint64_t maximumTimeLeftEstimate = 0;
			uint64_t compilingDevicesCount = 0;
			uint64_t compiledDevicesCount = 0;

			// Check Status of Workers - BEGIN
			// Iterate through all workers to determine compilation state and time estimates
			for (uint64_t w = 0; w < workers.size(); w++) {
				std::shared_ptr<CWorker> worker = workers[w];
				if (!worker) {
					continue; // Skip null workers for safety
				}

				// Track devices that are currently compiling and haven't succeeded yet
				if (worker->getState() == eWorkerState::compiling && !worker->getKernelEverCompiled()) {
					// Update maximum time estimate if this worker has a longer estimated time
					uint64_t workerEstimate = worker->getEstimatedCompilationTime();
					if (workerEstimate > maximumTimeLeftEstimate) {
						maximumTimeLeftEstimate = workerEstimate;
					}
					compilingDevicesCount++; // Count device as compiling if not ever successfully compiled
				}

				// Track successfully compiled devices
				if (worker->getKernelEverCompiled()) {
					compiledDevicesCount++;
				}
			}
			// Check Status of Workers - END

			// Progress Assessment - BEGIN
			if (compilingDevicesCount > 0 && timeDiff > 0) { // Ensure we don't divide by zero
				// Calculate progress percentage, capped at 100%
				double progress = min(
					(static_cast<double>(timeDiff) / static_cast<double>(maximumTimeLeftEstimate)) * 100.0,
					100.0
				);

				// Update status bars with current compilation progress
				std::string elapsedTimeStr = tools->secondsToString(timeDiff);
				std::string progressBarStr = tools->getProgressBarTxt(progress);

				// Update main progress bar
				barHub->setCustomStatusBarText(
					eBlockchainMode::TestNet,
					OPENCL_COMPILING_DEVICES_SINCE_BAR_ID,
					"Preparing Kernels - " + progressBarStr + " (" + elapsedTimeStr + " elapsed )"
				);

				// Construct device count status message
				std::string deviceCountStr = "Preparing " +
					tools->getColoredString(std::to_string(compilingDevicesCount), eColor::orange) +
					(compilingDevicesCount > 1 ? " devices." : " device.");

				// Add completed device count if any exist
				if (compiledDevicesCount > 0) {
					deviceCountStr += " Already prepared: " +
						tools->getColoredString(std::to_string(compiledDevicesCount), eColor::lightCyan) +
						".";
				}

				// Update device count status bar
				barHub->setCustomStatusBarText(
					eBlockchainMode::TestNet,
					OPENCL_COMPILED_DEVICES_COUNT_ID,
					deviceCountStr
				);
			}
			else {
				// Clear status bars when no devices are compiling
				barHub->invalidateCustomStatusBar(eBlockchainMode::TestNet, OPENCL_COMPILING_DEVICES_SINCE_BAR_ID);
				barHub->invalidateCustomStatusBar(eBlockchainMode::TestNet, OPENCL_COMPILED_DEVICES_COUNT_ID);
			}
			// OpenCL Compilation Progress Tracking - END

			if (previousState != workState)
				mTimeStarted = std::time(0);

			if (i == 0)
			{
				// Wait for all devices to compile kernels - BEGIN
				// [Rationale]: we want evaluation of computational power to proceed ONLY once all corresponding 'kernels' are compiled already.
				if (compilingDevicesCount)
				{
					if ((now - lastWarningReport) > 60)
					{
						tools->writeLine(timeStampPrefix + tools->getColoredString("waiting for ", eColor::orange)
							+ std::to_string(compilingDevicesCount) + " devices to compile..", true, false);
						lastWarningReport = now;
					}
					goto loopEnding;
				}
				else if (compiledDevicesCount == (workers.size()))
				{
					if (!notifiedAllDevicesReady)
					{
						tools->writeLine(timeStampPrefix + "All "
							+ std::to_string(workers.size()) + " devices are now " + tools->getColoredString("Ready!", eColor::lightGreen), true, false);
						notifiedAllDevicesReady = true;
					}
				}
			}
			// Wait for all devices to compile kernels - END


			subTasks = work->getChildTasks();

			if (work->getState() == eWorkState::preparing)
			{
				workPrepping = true;
			}

			// Check Tasks' Status  - BEGIN

			for (uint64_t i = 0; i < subTasks.size(); i++)
			{
				if (subTasks[i]->getState() == eWorkState::preparing)
				{
					workPrepping = true;
					subTaskPrepping = true;
					std::shared_ptr<CWorker> sw = std::dynamic_pointer_cast<CWorkUltimium>(subTasks[i])->getWorker();

					if (sw)
					{
						if ((now - lastSubtaskStatusReport) > (60 * 10))
						{
							lastSubtaskStatusReport = now;
							tools->writeLine(timeStampPrefix + "sub-task Kernel for " + sw->getShortName() + " is being compiled..", true, false);
						}
					}
					break;
				}
			}

			// Check Tasks' Status  - END


			if (workPrepping)
			{
				startTimeAllReady = tools->getTime();
			}

			for (int i = 0; i < workers.size(); i++)
			{
				if (workers[i]->getState() == eWorkerState::running)
				{
					gotActiveWorkers = true;
					mhps = workers[i]->getMhps();
					if (mhps == 0)
						continue;

					statusStr += workers[i]->getShortName() + ": " +
						mTools->to_string_with_precision(workers[i]->getMhps()) +
						" ";
				}
			}

			double aver = workManager->getAverageMhps();
			double currenMhps = workManager->getCurrentTotalMhps();



			if ((now - mLastDevReportModeSwitch) > 5)
			{
				mLastDevReportMode = (mLastDevReportMode + 1) % 4;
				mLastDevReportModeSwitch = now;
			}
			if ((now - lastTimeReported) >= 0 || (lastReportedMhps == 0 && currenMhps > 0))
			{
				lastReportedMhps = workManager->getCurrentTotalMhps();
				double aver = workManager->getAverageMhps();

				//detect zombie process - BEGIN
				runningTime = work->getRunningTime();
				if (workState == eWorkState::running && aver == 0 && (runningTime > 15))//15s
				{
					warnings++;
					if (warnings > 10)
					{
						tools->writeLine(tools->getColoredString("Aborting mining operation.. turned out to be a Zombie..", eColor::cyborgBlood));
						workManager->abortWork(mCurrentMiningTaskID);
					}
				}

				//detect zombie process - END
				std::string  d = (timeDiff % 2 ? "/" : "-");
				std::stringstream report;
				bool kerneReady = false;

				//we assume that a kernel is being compiled once enqueued 
				//todo: we should be actually checking for this explicitly
				if (workState == eWorkState::enqueued)
				{
					startTimeAllReady = std::time(0);//this would stop being refreshed once kernel ready and a count-down would begin

					//Wait for devices - BEGIN
					assignedWorker = work->getWorker();
					bool workerChanged = false;
					if ((assignedWorker && !previousAssignedWorker) ||
						(!assignedWorker && previousAssignedWorker) ||
						(assignedWorker && assignedWorker->getDevice() != previousAssignedWorker->getDevice()))
					{
						workerChanged = true;
					}

					previousAssignedWorker = assignedWorker;
					uint64_t waitingForKernelFor = work->getTimeInQueue();


					if (!assignedWorker)
					{
						std::vector<std::shared_ptr<IWork>>  subTasks = work->getChildTasks();
					}

					//Wait for devices - END

				}
				else if (gotActiveWorkers)
				{//kernel already compiled and it's working
					// Status Bar Report - BEGIN
					workers = workManager->getWorkers();

					switch (mLastDevReportMode)
					{
					case 0:
						report << tools->getColoredString("Total Mining Speed (MHps): ", eColor::lightCyan) + tools->to_string_with_precision(currenMhps, 4) + (currenMhps > aver ? tools->getColoredString(" >", eColor::lightGreen) : tools->getColoredString(" <", eColor::cyborgBlood)) + " aver.: " + tools->to_string_with_precision(aver, 4) + " (" + std::to_string((timeDiff) / 60) + "min. " + std::to_string(timeDiff % 60) + "sec. elapsed)";

						break;
					case 1:
						for (uint64_t i = 0; i < workers.size(); i++)
						{
							report << tools->getColoredString(" [" + workers[i]->getShortName() + "]: ", eColor::blue) + tools->getColoredString(tools->cleanDoubleStr(std::to_string(workers[i]->getMhps()), 3), eColor::lightCyan) + tools->getColoredString(" Mhps", eColor::greyWhiteBox);
						}

						break;

					case 2:
						for (uint64_t i = 0; i < workers.size(); i++)
						{
							report << tools->getColoredString(" [" + workers[i]->getShortName() + "]: ", eColor::blue) + tools->getColoredString(tools->cleanDoubleStr(std::to_string(((double)workers[i]->getKernelExecTime() / (double)1000)), 2), eColor::lightCyan) + tools->getColoredString(" sec", eColor::greyWhiteBox);
						}
						break;
					case 3:
						for (uint64_t i = 0; i < workers.size(); i++)
						{
							report << tools->getColoredString(" [" + workers[i]->getShortName() + "]: ", eColor::blue) + workers[i]->getStateName(true);
						}
						break;
					default:
						break;
					}
					barHub->setCustomStatusBarText(eBlockchainMode::TestNet, OPENCL_DIAGNOSTICS_BAR_ID, report.str());
					// Status Bar Report - END
				}
			}
			 results = work->getWorkResult();
			bool gotFinalPoW = false;
			if (!results.empty())
			{

				for (auto& r : results)
				{
					if (!std::dynamic_pointer_cast<CPoW>(r)->isPartialProof())
					{
						gotFinalPoW = true;
						break;
					}
				}

				if (gotFinalPoW && !compilingDevicesCount)
				{
					reportedDeadlocks = 0;
					mTools->writeLine(mTools->getColoredString("Stopping performance calculation - got confirmed final results..", eColor::lightGreen));
					resultsCount++;
					keepRunning = false;
				}

			}

			if (startTimeAllReady && !compilingDevicesCount && !gotActiveWorkers && (mTools->getTime() - startTimeAllReady) > 30)//startTimeAllReady is set to now once kernel compiled (or stops being refreshed)
			{

				keepRunning = false;
				mTools->writeLine("Kernel inactivity detected. Warning issued..");
				if (resultsCount == 0)
				{
					reportedDeadlocks++;
					if (reportedDeadlocks > 3)
					{
						barHub->invalidateCustomStatusBar(eBlockchainMode::TestNet, OPENCL_COMPILING_DEVICES_SINCE_BAR_ID);
						barHub->invalidateCustomStatusBar(eBlockchainMode::TestNet, OPENCL_COMPILED_DEVICES_COUNT_ID);
						mTools->writeLine(mTools->getColoredString("Dead-lock threshold reached. This platform might be very unstable.", eColor::lightPink));
						mTools->writeLine(mTools->getColoredString("Unable to initialize OpenCL workers and no results returned.", eColor::cyborgBlood));
						return false;
					}
					else
					{
						mTools->writeLine(mTools->getColoredString("A dead-lock situation detected while processing task " + std::to_string(i) + ".", eColor::lightPink));
					}
				}
			}
			if ((mTools->getTime() - startTime) > (60 * 60 * 15))//15 hours for kernels to compile
			{
				keepRunning = false;
				mTools->writeLine(mTools->getColoredString("Stopping performance calculation due to a timeout.", eColor::lightPink));
			}

		loopEnding:
			previousState = workState;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		if (compiledDevicesCount == workers.size())
		{
			barHub->invalidateCustomStatusBar(eBlockchainMode::TestNet, OPENCL_COMPILING_DEVICES_SINCE_BAR_ID);
			barHub->invalidateCustomStatusBar(eBlockchainMode::TestNet, OPENCL_COMPILED_DEVICES_COUNT_ID);
		}

		work->setState(eWorkState::aborted);


		mTools->writeLine(work->getReport());
	}

	std::stringstream openCLReport;
	openCLReport << mTools->getColoredString(">\r\nv-----------------------", eColor::blue) <<
		mTools->getColoredString(" OpenCL platform Test Report ", eColor::lightCyan) << mTools->getColoredString("-----------------------v\r\n", eColor::blue);
	//openCLReport << ">\r\n>>>>>>       Accomplished Ultimium PoWs     <<<<<<<<<<<<<\r\n";

	for (int i = 0; i < workers.size(); i++)
	{
		openCLReport << workers[i]->getReport() << "\r\n";
	}
	openCLReport << tools->getColoredString("[Computational Power of Active Devices]:", eColor::blue) << tools->getColoredString(std::to_string(workManager->getTotalComputationalPower(true)), eColor::lightCyan) << " (Mhps)" << "\r\n";
	openCLReport << tools->getColoredString("[Computational Power of All Devices]:", eColor::blue) << tools->getColoredString(std::to_string(workManager->getTotalComputationalPower()), eColor::lightCyan) << " (Mhps)" << "\r\n";
	//*TODO*: ensure all workers are always occupied. Divide work among workers which become available again. in proportion to their MHPS.
	//TODO: print test results
	//TODO: print total measured computational power.
	//TODO: 
	openCLReport << mTools->getColoredString(">\r\n^-----------------------", eColor::blue) <<
		mTools->getColoredString(" OpenCL platform Test Report ", eColor::lightCyan) << mTools->getColoredString("-----------------------^\r\n", eColor::blue);
	mTools->writeLine(openCLReport.str(), true, false);
	return true;

	//Operational Logic - END
}

bool CTests::testIWorkCLInterface()// this test is redundant with testOpenCLPlatform so far.
{
	if (mWorkManager->wasInitialised())
		mWorkManager->Initialise();

	arith_uint256 diff;	trr:
	uint32_t x = 0x1f00ffff;
	arith_uint256  min_target = diff.SetCompact(x);
	arith_uint256  main_target = diff.SetCompact(x);
	main_target = main_target / 10;
	min_target = min_target * 10;


	std::array<unsigned char, 32> device_min_target;
	min_target.GetBytes((char*)device_min_target.data());
	std::string sss = min_target.GetHex();

	std::array<unsigned char, 32> device_main_target;
	main_target.GetBytes((char*)device_main_target.data());
	std::string sss2 = main_target.GetHex();

	for (int i = 0; i < 3; i++)
	{
		std::shared_ptr<CWorkUltimium> work = std::make_shared<CWorkUltimium>(nullptr, eBlockchainMode::eBlockchainMode::LocalData, device_main_target, device_min_target, 0, 0xFFFFFFFF);


		std::vector<uint8_t> data_to_work_on;
		data_to_work_on.resize(80);

		for (int i = 0; i < data_to_work_on.size(); i++)
		{
			data_to_work_on[i] = i;

		}
		std::vector<uint8_t> id = work->getGUID();
		work->setDataToWorkOn(data_to_work_on);
		work->markAsTestWork();
		mWorkManager->registerTask(work);
	}
	bool keepRunning = true;
	while (keepRunning)
	{

	}

	return true;
}

bool CTests::testIComputeTaskInterface()
{
	/*CComputeTask *computeTask = new CComputeTask(".\\kernel\\gridnetTest.cl", "");

	CComputeTask task1("", "");

	//header
	CMemParam p1(eParamType::memoryBuffer, eMemType::clMem, eAccessType::read, 80);
	//output
	CMemParam p2(eParamType::memoryBuffer, eMemType::clMem, eAccessType::write, BUFFERSIZE);
	//test output
	CMemParam p3(eParamType::memoryBuffer, eMemType::clMem, eAccessType::write, TEST_BUFFERSIZE);
	//hash buffer
	CMemParam p4(eParamType::memoryBuffer, eMemType::clMem, eAccessType::write, mHashBufferSize);

	task1.addParam(p1);
	task1.addParam(p2);
	task1.addParam(p3);
	task1.addParam(p4);

	CComputeTask task2("", "blake");
	p1.setCLMem(mHeaderBuffer);
	p4.setCLMem(mHashBuffer);
	task2.addParam(p1);
	task2.addParam(p4);

	CComputeTask task3("", "keccak");
	p4.setCLMem(mHashBuffer);
	task3.addParam(p4);

	CComputeTask task4("", "hamsi");
	p4.setCLMem(mHashBuffer);
	task4.addParam(p4);

	CComputeTask task5("", "shavite");
	p4.setCLMem(mHashBuffer);
	task5.addParam(p4);

	CComputeTask task6("", "jh");
	p4.setCLMem(mHashBuffer);
	task6.addParam(p4);

	CComputeTask task7("", "skein");
	CValueParam p5(eParamType::numerical);
	CValueParam p6(eParamType::numerical);
	p6.setValue(mMinTargetOffset);
	p5.setValue((uint64_t)(getMinDifficulty()->data() + mMinTargetOffset));
	p4.setCLMem(mHashBuffer);
	p2.setCLMem(mOutputBuffer);
	p3.setCLMem(mTestBuffer);
	task7.addParam(p4);
	task7.addParam(p2);
	task7.addParam(p5);
	task7.addParam(p6);
	task7.addParam(p3);

	computeTask->addSubTask(task1);
	computeTask->addSubTask(task2);
	computeTask->addSubTask(task3);
	computeTask->addSubTask(task4);
	computeTask->addSubTask(task5);
	computeTask->addSubTask(task6);
	computeTask->addSubTask(task7);
	*/
	return false;
}

using namespace std::chrono_literals;
bool CTests::testIWorkInterface()
{
	std::vector<uint8_t> id;

	//COCLEngine engine;
	//engine.Initialize();

	//CWorkManager mgr(&engine);
	if (!mWorkManager->wasInitialised())
		mWorkManager->Initialise();


	for (int i = 0; i < 10; i++)
	{
		std::shared_ptr<CWork> work = std::make_shared<CWork>(eBlockchainMode::eBlockchainMode::LocalData);
		work->setIndex(i);
		work->setPriority(i);
		mWorkManager->registerTask(work);
		std::this_thread::sleep_for(1000ms);
	}

	bool keepRunning = true;
	while (keepRunning)
	{
		std::this_thread::sleep_for(10000ms);

		if (mWorkManager->getTaskIDs(eWorkState::running).size() > 0)
			mWorkManager->setWorkPriority(mWorkManager->getTaskIDs(eWorkState::running).at(5), 10);
	}
	return true;
}

bool CTests::testIWorkHybridInterface()
{
	std::vector<uint8_t> id;


	if (!mWorkManager->wasInitialised())
		mWorkManager->Initialise();


	for (int i = 0; i < 10; i++)
	{
		std::shared_ptr<CWorkHybrid>work = std::make_shared<CWorkHybrid>(nullptr, eBlockchainMode::eBlockchainMode::LocalData);
		work->setPriority(i);
		mBlockchainManager->getWorkManager()->registerTask(work);
		std::this_thread::sleep_for(1000ms);
	}

	bool keepRunning = true;
	while (keepRunning)
	{
		std::this_thread::sleep_for(10000ms);

		if (mBlockchainManager->getWorkManager()->getTaskIDs(eWorkState::running).size() > 0)
			mBlockchainManager->getWorkManager()->setWorkPriority(mBlockchainManager->getWorkManager()->getTaskIDs(eWorkState::running).at(5), 10);
	}
	return true;
}