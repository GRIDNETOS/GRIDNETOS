// GRIDNET.cpp: Okreœla punkt wejœcia dla aplikacji konsoli.
//
#include "CryptoFactory.h"
#include "resource.h"
#include "stdafx.h"
#include <fstream>
#include "BlockchainManager.h"
#include "GRIDNET.h"
#include "CGlobalSecSettings.h"
#include "mainTests.h"
#include "transactionmanager.h"
#include "NetworkManager.h"
#include "TokenPool.h"
#include "CWorkUltimium.h"
#include "../Security/Core/SecurityCore.h"

//#include <math>
#define MSG_LEN 200
#define  KADEMLIA_ENABLE_DEBUG 1
#define MY_STRING_LITERAL  "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
#include "TrieDB.h"
#include "GRIDNET.h"
// PADDINT TEMPLATE		      "****************************************************************************************************"v------- the following needs to be padded with * exactly up to this place
#define ËXPECTED_BINARY_HASH  "gTLboJd1WERqvdAevTBp9aAMUvSigpsZ2fV3heuzmsq334Nc6***************************************************"

#ifdef _WIN32


void enableColors()
{
	std::cout << "Enabling colors in a Windows-based terminal..\r\n";
	DWORD consoleMode;
	HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleMode(outputHandle, &consoleMode))
	{
		SetConsoleMode(outputHandle, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}

}

#endif

// ============================================================================
// Base64/Base64Check Obligatory Validation Tests
// These MUST pass before GRIDNET Core can start - validates encoding integrity
// Focus: Round-trip integrity (encode->decode must return original data)
// ============================================================================

bool runBase64ObligatoryValidation(std::shared_ptr<CTools> tools) {
	tools->writeLine("");
	tools->writeLine(tools->getColoredString(u8"════════════════════════════════════════", eColor::blue));
	tools->writeLine(tools->getColoredString(u8"  Base64/Base64Check Encoding Validation", eColor::lightCyan));
	tools->writeLine(tools->getColoredString(u8"════════════════════════════════════════", eColor::blue));
	tools->writeLine("");

	int passed = 0;
	int failed = 0;

	// ═══════════════════════════════════════════════════════════════════════
	// Base64 Round-Trip Tests (encode -> decode must return original data)
	// This is the CRITICAL test for bytecode serialization integrity
	// ═══════════════════════════════════════════════════════════════════════
	tools->writeLine(tools->getColoredString("--- Base64 Round-Trip Tests ---", eColor::lightCyan));

	std::vector<std::pair<const char*, std::vector<uint8_t>>> roundTripTests = {
		// Padding edge cases (1, 2, 3 bytes -> different padding)
		{ "Padding test (1 byte)", {0x42} },
		{ "Padding test (2 bytes)", {0x42, 0x43} },
		{ "Padding test (3 bytes)", {0x42, 0x43, 0x44} },

		// Standard test strings
		{ "Hello World", {'H','e','l','l','o',' ','W','o','r','l','d'} },
		{ "foobar (RFC 4648)", {'f','o','o','b','a','r'} },

		// Binary data patterns (critical for bytecode)
		{ "All zeros (16 bytes)", std::vector<uint8_t>(16, 0x00) },
		{ "All 0xFF (16 bytes)", std::vector<uint8_t>(16, 0xFF) },
		{ "Alternating 0x55/0xAA", {0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA} },
		{ "Binary 0x00-0x0F", {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F} },
		{ "Binary 0xF0-0xFF", {0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF} },

		// Larger data (tests AVX2/AVX512 code paths)
		{ "64 bytes (AVX test)", std::vector<uint8_t>(64, 0xAB) },
		{ "128 bytes (AVX test)", std::vector<uint8_t>(128, 0xCD) },
		{ "200 bytes (typical bytecode)", std::vector<uint8_t>(200, 0xEF) },
	};

	// Add sequential 0-255 test (all byte values)
	std::vector<uint8_t> sequential(256);
	for (int i = 0; i < 256; i++) sequential[i] = (uint8_t)i;
	roundTripTests.push_back({ "Sequential 0-255 (all byte values)", sequential });

	for (const auto& test : roundTripTests) {
		std::string encoded = tools->base64Encode(test.second);
		std::vector<uint8_t> encodedBytes(encoded.begin(), encoded.end());
		std::vector<uint8_t> decoded;
		bool decodeSuccess = tools->base64Decode(encodedBytes, decoded);
		bool testPassed = decodeSuccess && (decoded == test.second);

		if (testPassed) {
			tools->writeLine(tools->getColoredString(u8"  ✓ ", eColor::lightGreen) +
				tools->getColoredString(test.first, eColor::lightWhite) +
				tools->getColoredString(" (" + std::to_string(test.second.size()) + " bytes -> " + std::to_string(encoded.size()) + " chars)", eColor::ghostWhite));
			passed++;
		} else {
			tools->writeLine(tools->getColoredString(u8"  ✗ ", eColor::lightPink) +
				tools->getColoredString(test.first, eColor::lightPink));
			tools->writeLine(tools->getColoredString("         Input size: " + std::to_string(test.second.size()) + " bytes", eColor::orange));
			std::string encodedPreview = encoded.substr(0, 50) + (encoded.size() > 50 ? "..." : "");
			tools->writeLine(tools->getColoredString("         Encoded: \"" + encodedPreview + "\" (" + std::to_string(encoded.size()) + " chars)", eColor::orange));
			if (!decodeSuccess) {
				tools->writeLine(tools->getColoredString("         Decode FAILED", eColor::cyberWine));
			} else {
				tools->writeLine(tools->getColoredString("         Decoded size: " + std::to_string(decoded.size()) + " bytes (expected " + std::to_string(test.second.size()) + ")", eColor::orange));
				// Find first mismatch
				size_t minSize = decoded.size() < test.second.size() ? decoded.size() : test.second.size();
				for (size_t idx = 0; idx < minSize; idx++) {
					if (decoded[idx] != test.second[idx]) {
						std::stringstream ss;
						ss << "         First mismatch at byte " << idx << ": got 0x" << std::hex << (int)decoded[idx]
						   << " expected 0x" << (int)test.second[idx] << std::dec;
						tools->writeLine(tools->getColoredString(ss.str(), eColor::cyberWine));
						break;
					}
				}
			}
			failed++;
		}
	}

	// ═══════════════════════════════════════════════════════════════════════
	// Base64Check Round-Trip Tests (with checksum verification)
	// ═══════════════════════════════════════════════════════════════════════
	tools->writeLine("");
	tools->writeLine(tools->getColoredString("--- Base64Check Round-Trip Tests ---", eColor::lightCyan));

	std::vector<std::pair<const char*, std::vector<uint8_t>>> base64CheckTests = {
		{ "Simple string 'Test'", {'T','e','s','t'} },
		{ "Binary data with edge values", {0x00, 0x01, 0x02, 0xFD, 0xFE, 0xFF} },
		{ "GridScript-like data (32 bytes)", std::vector<uint8_t>(32, 0x42) },
		{ "Larger payload (100 bytes)", std::vector<uint8_t>(100, 0x7F) },
	};

	for (const auto& test : base64CheckTests) {
		std::string encoded = tools->base64CheckEncode(test.second);
		std::vector<uint8_t> encodedBytes(encoded.begin(), encoded.end());
		std::vector<uint8_t> decoded;
		bool decodeSuccess = tools->base64CheckDecode(encodedBytes, decoded);
		bool testPassed = decodeSuccess && (decoded == test.second);

		if (testPassed) {
			tools->writeLine(tools->getColoredString(u8"  ✓ ", eColor::lightGreen) +
				tools->getColoredString(test.first, eColor::lightWhite));
			passed++;
		} else {
			tools->writeLine(tools->getColoredString(u8"  ✗ ", eColor::lightPink) +
				tools->getColoredString(test.first, eColor::lightPink));
			std::string encodedPreview = encoded.substr(0, 50) + (encoded.size() > 50 ? "..." : "");
			tools->writeLine(tools->getColoredString("         Encoded: \"" + encodedPreview + "\"", eColor::orange));
			if (!decodeSuccess) {
				tools->writeLine(tools->getColoredString("         Checksum verification or decode FAILED", eColor::cyberWine));
			} else {
				tools->writeLine(tools->getColoredString("         Data mismatch: got " + std::to_string(decoded.size()) + " bytes, expected " + std::to_string(test.second.size()), eColor::orange));
			}
			failed++;
		}
	}

	tools->writeLine("");
	tools->writeLine(tools->getColoredString(u8"════════════════════════════════════════", eColor::blue));

	if (failed > 0) {
		tools->writeLine(tools->getColoredString("Base64 Validation: ", eColor::lightWhite) +
			tools->getColoredString(std::to_string(passed) + " passed", eColor::lightGreen) +
			tools->getColoredString(", ", eColor::lightWhite) +
			tools->getColoredString(std::to_string(failed) + " failed", eColor::lightPink));
		tools->writeLine(tools->getColoredString(u8"════════════════════════════════════════", eColor::blue));
		tools->writeLine("");
		tools->writeLine(tools->getColoredString(u8"╔══════════════════════════════════════════════════════════════════╗", eColor::cyberWine));
		tools->writeLine(tools->getColoredString(u8"║  FATAL: Base64/Base64Check encoding validation FAILED!          ║", eColor::cyberWine));
		tools->writeLine(tools->getColoredString(u8"╠══════════════════════════════════════════════════════════════════╣", eColor::cyberWine));
		tools->writeLine(tools->getColoredString(u8"║  The encoding implementation is not working correctly.          ║", eColor::orange));
		tools->writeLine(tools->getColoredString(u8"║  GRIDNET Core CANNOT proceed - bytecode serialization would     ║", eColor::orange));
		tools->writeLine(tools->getColoredString(u8"║  produce corrupted data.                                        ║", eColor::orange));
		tools->writeLine(tools->getColoredString(u8"║                                                                  ║", eColor::orange));
		tools->writeLine(tools->getColoredString(u8"║  Please verify:                                                  ║", eColor::lightWhite));
		tools->writeLine(tools->getColoredString(u8"║  1. AVX2/AVX512 hardware acceleration is working correctly      ║", eColor::lightWhite));
		tools->writeLine(tools->getColoredString(u8"║  2. Encode->Decode round-trip preserves all byte values         ║", eColor::lightWhite));
		tools->writeLine(tools->getColoredString(u8"║  3. Padding is handled correctly for all input lengths          ║", eColor::lightWhite));
		tools->writeLine(tools->getColoredString(u8"║  4. Checksum verification works for base64Check                 ║", eColor::lightWhite));
		tools->writeLine(tools->getColoredString(u8"╚══════════════════════════════════════════════════════════════════╝", eColor::cyberWine));
		return false;
	}

	tools->writeLine(tools->getColoredString("Base64 Validation: ", eColor::lightWhite) +
		tools->getColoredString(std::to_string(passed) + " passed", eColor::lightGreen) +
		tools->getColoredString(", ", eColor::lightWhite) +
		tools->getColoredString(std::to_string(failed) + " failed", eColor::lightGreen));
	tools->writeLine(tools->getColoredString(u8"════════════════════════════════════════", eColor::blue));
	tools->writeLine("");
	tools->writeLine(tools->getColoredString(u8"✓ ", eColor::lightGreen) +
		tools->getColoredString("Base64/Base64Check validation PASSED - encoding integrity verified", eColor::lightGreen));
	tools->writeLine("");
	return true;
}
std::shared_ptr<CTools> CGRIDNET::sTools = nullptr;
std::mutex  CGRIDNET::sToolsGuardian;

 std::mutex CGRIDNET::sWorkManagerGuardian;
 std::shared_ptr<COCLEngine> CGRIDNET::sOCLEngine = nullptr;
 std::shared_ptr<CWorkManager> CGRIDNET::mWorkManager = nullptr;
 std::shared_ptr<CGRIDNET> CGRIDNET::sInstance = nullptr;


int main()
{
	// Debug checkpoints for startup diagnosis
	std::cerr << "[DEBUG] GRIDNET Core starting..." << std::endl;
	std::cerr.flush();

	// NOTE: CTests creation moved to AFTER CTools initialization
	// CScriptEngine::reset() needs CTools::getInstance() to be valid
	//bool result = CTests::testExclusiveWorkerMutex();

	// BigInt tests disabled - they use CTools::getInstance() before CTools is initialized
	// These can be re-enabled once CTools is moved earlier or tests are moved later
	#if 0
	BigInt a1 = BigInt("54794907337687233901990748671359622986648868320775326425468628102258375815580");
	BigInt a2 = BigInt("955984424979149430541687982934761755698680495270168383438464768588610639557");
	BigInt a3 = a2 - a1;
	BigSInt a4 = static_cast<BigSInt>(a2) - static_cast<BigSInt>(a1);
	std::vector<uint8_t> res = CTools::getInstance()->BigIntToBytes(a3);
	std::vector<uint8_t> res2 = CTools::getInstance()->BigSIntToBytes(a4);

	const std::vector<uint8_t> CURVE_ORDER_BYTES = Botan::hex_decode("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");

	std::stringstream ss;
	ss << "a1: " << a1.str() << " a2: " << a2.str();
	ss << " a3: " << a3.str() << " a4: " << a4.str();


	std::string resStr1 = ss.str();
	ss.str("");

	//test 2 - begin
	a2 = 0;
	a3 = a2 - a1;

	res = CTools::getInstance()->BigIntToBytes(a3);

	ss << "a1: " << a1.str() << " a2: " << a2.str();
	ss << " a3: " << a3.str();

	std::string resStr2 = ss.str();

	ss.str("");

	//test 2 - end

	//test 3 - begin
	a2 = CTools::getInstance()->BytesToBigInt(CURVE_ORDER_BYTES)+1;
	a3 = a2 - a1;

	res = CTools::getInstance()->BigIntToBytes(a3);

	ss << "a1: " << a1.str() << " a2: " << a2.str();
	ss << " a3: " << a3.str();

	std::string resStr3= ss.str();

	ss.str("");

	//test 3 - end

	 //a2 = (CTools::getInstance()->BytesToBigInt(CTools::getInstance()->genRandomVector(32))) / 100;
	a1++;
	a3 = a2 - a1;
	res = CTools::getInstance()->BigIntToBytes(a3);

	ss << "a1: " << a1.str() << " a2: " << a2.str();
	ss << " a3: " << a3.str();

	resStr2 = ss.str();
	#endif

	HANDLE m_singleInstanceMutex = CreateMutex(NULL, TRUE, L"GRIDNET Core");
	if (m_singleInstanceMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
		HWND existingApp = FindWindow(0, L"GRIDNET Core");
		if (existingApp)
		{
			SetForegroundWindow(existingApp);
		}
		std::cout << "GRIDNET Core is already running..\n\r";
		CTools::stopAllOperations();
		return 0;
	}
	std::cout << "Verified to be the only instance running..\n\r";
	CTools::hideCursorANSI();
	CTools::prepareLogFile();
#ifdef _WIN32
	enableColors();
#endif
	std::shared_ptr<CTools> tools = std::make_shared<CTools>("GRIDNET", eBlockchainMode::LocalData);
	tools->enableConsoleFeatures();
	tools->writeLine(tools->getColoredString("Dynamic Libraries loaded..", eColor::lightCyan));

	// Create CTests AFTER CTools is initialized (CScriptEngine::reset() needs CTools::getInstance())
	// Pass the tools shared_ptr so CTests uses the properly initialized CTools instance
	std::cerr << "[DEBUG] Creating CTests (after CTools init)..." << std::endl;
	std::cerr.flush();
	CTests tests = CTests(nullptr, tools);
	std::cerr << "[DEBUG] CTests created successfully" << std::endl;
	std::cerr.flush();

	// ══════════════════════════════════════════════════════════════════════════
	// OBLIGATORY: Validate Base64/Base64Check encoding BEFORE any other tests
	// If this fails, bytecode serialization cannot be trusted - ABORT immediately
	// ══════════════════════════════════════════════════════════════════════════
	if (!runBase64ObligatoryValidation(tools)) {
		tools->writeLine("");
		tools->writeLine(tools->getColoredString("ABORTING: Base64 encoding validation failed!", eColor::cyberWine));
		tools->writeLine(tools->getColoredString("Fix the encoding implementation before running GRIDNET Core.", eColor::orange));
		CTools::stopAllOperations();
		return 2; // Special exit code for validation failure
	}

	

	
	//tools->writeLine("Booting...", true, true, eViewState::unspecified, "Solid Storage");
	//todo: remove the following
	//std::shared_ptr<CTokenPool> bootStrapNode = std::make_shared<CTokenPool>();//shared_from_this() needs a first-time initialization

 // bool result = tests.validateVarIntImplementation();
   //CTests::validateTargetWindowComparison();
   //CTests::validateNonceInsertion();
  // CTests::validateTargetDifficultyChain();

   //int flags = eColor::statusIdle | eColor::BLINK_FLAG; // Should be 66 | 256 = 322
   //CTests::debugPrintFlags(flags);  // This will help verify the flags are set correctly
   //std::cout << tools->getColoredString(u8"IDLE", flags) << "\n";

  //CTests::testColorSystem();
   //assertGN(tests.testFileCertificates());
   //tests.testGridcraftPDU();
   //std::vector<uint8_t> arr = { 154,153,153,153,153,153,241,63 };
   //double t = tools->bytesToDouble(arr);
	//if (!tests.testBigInts())
	//	return false;
	//if (!tests.testHDKeys())
 	//return false;
    if (!tests.testGridScriptCodeSerialization())
	{
		return false;
	}

	// JavaScript Bytecode Cross-Validation: Verify JS-compiled bytecode decompiles correctly in C++
	if (!tests.testJavaScriptBytecodeCrossValidation())
	{
		tools->writeLine(tools->getColoredString("JavaScript cross-validation FAILED!", eColor::cyberWine));
		return false;
	}
	else
	{
		tools->writeLine(tools->getColoredString("JavaScript cross-validation PASSED!", eColor::lightGreen));
	}

	// Export C++ bytecodes for JavaScript reverse testing (C++ compile -> JS decompile)
	if (!tests.exportCppBytecodesForJavaScriptTesting())
	{
		tools->writeLine(tools->getColoredString("C++ bytecode export FAILED!", eColor::cyberWine));
		return false;
	}
	else
	{
		tools->writeLine(tools->getColoredString("C++ bytecode export PASSED! Check temp directory for JS test data.", eColor::lightGreen));
	}

	/*if (!tests.testTransmissionTokenSerialization())
	return false;
	if (!tests.testTransmissionTokenTransactions())
	return false;
	if (!tests.testTransmissionTokenUtilization(true))
	return false;
	if (!tests.testTransmissionTokenUtilization(false))
		return false;
		*/
	//tests.testBigInts();
	//tests.testFileMetaDescriptors();
	//bool result = false;
	//result = tests.testTokenPoolGenerationAndSerialization();
	//result = tests.testTransmissionTokenUtilization();


	//result = tests.testTransmissionTokenUtilization();
	//tests.testHDKeys();
	//tests.testNetworkPacketSecurity();
	//tests.testX25519ChaCha();
	//tests.testNetworkPacketSecurity();

	

	//tests.testWebRTCSwarms();
	//tests.testSDPEntitiesSerializationAndAuth();
	//tests.testAuthenticatedTransmisionTokens();
	//tests.testVMMetaGeneratorAndParser();
	//tests.testTokenPoolGenerationAndSerialization();
	//tests.testTransmissionTokenSerialization();
	//tests.testTransmissionTokenUtilization();
	//tests.testTransmissionTokenTransactions();
	tools->writeLine(tools->getColoredString("GRIDNET Core Version: ", eColor::blue) +
		tools->getColoredString(CGlobalSecSettings::getVersionStr(), eColor::lightCyan));
	tools->writeLine(tools->getColoredString("GRIDNET Core Release Revision: ", eColor::blue) +
		tools->getColoredString(std::to_string(CGlobalSecSettings::getVersionNumber()), eColor::lightCyan));
	std::vector<uint8_t> imageSelf;

		
		// Self-Integrity Verification - BEGIN
#if DO_SELF_INTEGRITY_CHECKS == 1
		std::string errStr;
		const std::string fullHash = ËXPECTED_BINARY_HASH;
		const std::string expectedHash = fullHash.substr(0, fullHash.find('*'));

		// Version Information Display
		
		tools->writeLine(tools->getColoredString("Validating GRIDNET Core integrity.. ", eColor::headerCyan));

		// Hash Validation - BEGIN
		imageSelf = tools->getImageSelf(256000, errStr);
		std::string currentHash = tools->base58CheckEncode(imageSelf);

		std::stringstream fingerprintMsg;
		fingerprintMsg << tools->getColoredString("Expected Core Fingerprint: ", eColor::dataPrimary)
			<< tools->getColoredString(expectedHash, eColor::dataHighlight) << "\n"
			<< tools->getColoredString("Current Core Fingerprint:  ", eColor::dataPrimary)
			<< tools->getColoredString(currentHash, eColor::dataHighlight);

		tools->writeLine(fingerprintMsg.str());

		if (expectedHash != currentHash) {
			tools->writeLine(tools->getColoredString("Core integrity check failed!", eColor::alertError));
			Sleep(2000);
			return 0;
		}
		else
		{
			tools->writeLine(tools->getColoredString("Core integrity verified.", eColor::lightGreen));
		}
		// Hash Validation - END

		Sleep(2000);

#else
	tools->writeLine(tools->getColoredString("WARNING:  ", eColor::cyborgBlood) +
		tools->getColoredString("integrity checks have been disabled for this build.", eColor::lightPink));
#endif	

	
// Self-Integrity Verification - END



// Onboard Self-Protection Mechanics - BEGIN (COMPACT VERSION)
	std::shared_ptr<Security::SecurityCore> security = Security::SecurityCore::GetInstance();

	// Configuration using CGlobalSecSettings static getters
	Security::SecurityCore::Configuration config;
	config.enableHookDetection = CGlobalSecSettings::getSecEnableHookDetection();
	config.enableInjectionDetection = CGlobalSecSettings::getSecEnableInjectionDetection();
	config.enableMemoryProtection = CGlobalSecSettings::getSecEnableMemoryProtection();
	config.enableAntiDebug = CGlobalSecSettings::getSecEnableAntiDebug();
	config.enableETW = CGlobalSecSettings::getSecEnableETW();
	config.enableKernelBridge = CGlobalSecSettings::getSecEnableKernelBridge();
	config.enableForensics = CGlobalSecSettings::getSecEnableForensics();
	config.enableAutoResponse = CGlobalSecSettings::getSecEnableAutoResponse();
	config.scanInterval = CGlobalSecSettings::getSecScanIntervalMs();
	config.quickScanInterval = CGlobalSecSettings::getSecQuickScanIntervalMs();
	config.autoQuarantine = CGlobalSecSettings::getSecAutoQuarantine();
	config.terminateOnCriticalThreat = CGlobalSecSettings::getSecTerminateOnCritical();
	config.notifyUser = CGlobalSecSettings::getSecNotifyUser();
	config.protectionLevel = static_cast<Security::ProtectionLevel>(CGlobalSecSettings::getSecProtectionLevel());
	config.quarantinePath = CGlobalSecSettings::getSecQuarantinePath();
	config.forensicsPath = CGlobalSecSettings::getSecForensicsPath();
	//config.logger = CTools::getInstance();

	// Compact initialization display
	tools->writeLine(tools->getColoredString(u8"╔══════════════════════════════════════════════╗", eColor::headerCyan));
	tools->writeLine(tools->getColoredString(u8"║  GRIDNET CORE SECURITY INITIALIZATION        ║", eColor::headerCyan));
	tools->writeLine(tools->getColoredString(u8"╚══════════════════════════════════════════════╝", eColor::headerCyan));

	if (!security->Initialize(config)) {
		tools->writeLine(tools->getColoredString(u8"✗ FAILED", eColor::alertError) +
			tools->getColoredString(" - Security features unavailable", eColor::alertWarning));
	}
	else {
		// Determine protection level color and name
		std::string levelName;
		eColor::eColor levelColor;
		switch (config.protectionLevel) {
		case Security::ProtectionLevel::DISABLED:  levelName = "DISABLED";  levelColor = eColor::statusOffline; break;
		case Security::ProtectionLevel::MINIMAL:   levelName = "MINIMAL";   levelColor = eColor::cyberYellow; break;
		case Security::ProtectionLevel::STANDARD:  levelName = "STANDARD";  levelColor = eColor::neonGreen; break;
		case Security::ProtectionLevel::ENHANCED:  levelName = "ENHANCED";  levelColor = eColor::plasmaTeal; break;
		case Security::ProtectionLevel::MAXIMUM:   levelName = "MAXIMUM";   levelColor = eColor::neonPurple; break;
		case Security::ProtectionLevel::PARANOID:  levelName = "PARANOID";  levelColor = eColor::synthPink; break;
		default: levelName = "UNKNOWN"; levelColor = eColor::alertWarning;
		}

		tools->writeLine(tools->getColoredString(u8"✓ ACTIVE", eColor::alertSuccess) +
			tools->getColoredString(" - Protection Level: ", eColor::ghostWhite) +
			tools->getColoredString(levelName, levelColor));

		// Feature summary (compact single line)
		std::string features = "Features: ";
		std::vector<std::string> enabled;
		if (config.enableHookDetection) enabled.push_back("Hooks");
		if (config.enableInjectionDetection) enabled.push_back("Injection");
		if (config.enableMemoryProtection) enabled.push_back("Memory");
		if (config.enableAntiDebug) enabled.push_back("AntiDebug");
		if (config.enableETW) enabled.push_back("ETW");
		if (config.enableKernelBridge) enabled.push_back("Kernel");
		if (config.enableForensics) enabled.push_back("Forensics");

		if (enabled.empty()) {
			tools->writeLine(tools->getColoredString("  " + features, eColor::ghostWhite) +
				tools->getColoredString("None", eColor::statusOffline));
		}
		else {
			std::string featureList;
			for (size_t i = 0; i < enabled.size(); ++i) {
				featureList += enabled[i];
				if (i < enabled.size() - 1) featureList += ", ";
			}
			tools->writeLine(tools->getColoredString("  " + features, eColor::ghostWhite) +
				tools->getColoredString(featureList, eColor::statusOnline));
		}

		tools->writeLine(tools->getColoredString("  Scan Interval: ", eColor::ghostWhite) +
			tools->getColoredString(std::to_string(config.scanInterval) + "ms", eColor::dataHighlight));
	}

	tools->writeLine(tools->getColoredString("", eColor::none));

	// Onboard Self-Protection Mechanics - END

	//Validate Integrity of system files - BEGIN

#if DO_OPENCL_INTEGRITY_CHECKS ==1
	std::vector<uint8_t> currentOpenCLHash;
	if (!CWorkUltimium::checkComponents(true, currentOpenCLHash))
	{
		tools->writeLine(tools->getColoredString("Unable to verify integrity of OpenCL components.", eColor::cyborgBlood)+"\n Current OpenCL Hash : "+ tools->getColoredString( tools->base58CheckEncode(currentOpenCLHash), eColor::blue)+" \n Exiting..");
		Sleep(3000);

		return 0;
	}
	else
	{
		tools->writeLine(tools->getColoredString("OpenCL data integrity verified.", eColor::lightGreen));
		Sleep(1000);
	}
#else
	tools->writeLine(tools->getColoredString("WARNING:  ", eColor::cyborgBlood) +
		tools->getColoredString("OpenCL integrity checks have been disabled for this build.", eColor::lightPink));
#endif
	//Validate Integrity of system files - END


	// Validate Structured Exception Handling Availability - BEGIN
	// Check if SEH chain is intact
	if (!tools->validateExceptionHandlingIntegrity())
	{
		return 0; // I'm a Lame Haxor, please excuse
	}
	// Validate Structured Exception Handling Availability - END


	bool abortOpenCLtest = tools->askYesNo("Abort the GPU / CPU test?", false, "Compute Engine",
		true, true, false, 5, true);
	if (tools->askYesNo("Do you want to initiate the self-destruction procedure?", false,"SECURITY",true,true,false,5,true))
	{
		if (tools->askYesNo(tools->getColoredString("CAUTION: You are about to initiate the Secure Self-Destruction procedure. \n ALL THE LOCAL DATA INCLUDING *PRIVATE-KEYS*, DATABASES WILL BE WIPED AWAY. \n Do you CONFIRM?", eColor::cyborgBlood), false,"",true,true,true, 180,true))
		{
			tools->writeLine("Self-destruction procedure initiated, proceeding..");
			//CSolidStorage storage(eBlockchainMode::eBlockchainMode::LocalData, false);
			std::vector< CSolidStorage*> storages;
			storages.push_back(CSolidStorage::getInstance(eBlockchainMode::eBlockchainMode::LocalData));

			for (uint64_t i = 0; i < storages.size(); i++)
			{
				if (storages[i])
				{
					if (storages[i]->destroyData())
					{
						CTools::clearScreen();
						tools->writeLine("Self-destruction procedure for data-store "+ tools->getColoredString("COMPLETED",eColor::lightGreen)+".");
					}
					else
					{
						tools->writeLine("WARNING: Self-destruction procedure for data-store could not complete.");
					}
					
				}
			}
			try {
				tools->writeLine(tools->getColoredString("I'll now shut-down. Bye, bye.", eColor::orange));
				CTools::stopAllOperations();

				std::exit(0);
			}
			catch (...) {};
		
		}

	}

	std::shared_ptr<CGRIDNET> GRIDNET = CGRIDNET::getInstance();

	GRIDNET->setSelfy(imageSelf);

	if (GRIDNET == nullptr)
	{
		tools->writeLine("Unable to initialize The Main Engine");
		std::exit(0);
	}
	else if(GRIDNET->getStatus() == eManagerStatus::stopped)
	{
		tools->writeLine("The Main Engine is in invalid state. Exiting..");
		GRIDNET->shutdown();
		exit(0);
	}

	//SELF-DESTRUCTION Procedure - END



	try {
		std::shared_ptr<CSettings> set = CBlockchainManager::getInstance(eBlockchainMode::eBlockchainMode::LocalData)->getSettings();



		bool want = false;

		want = set->getLoadPreviousGlobalConfiguration();



		//the following is mandatory for the Local Terminal and access to the Decentralized Virtual Console.

		if (!GRIDNET->initialize(eBlockchainMode::eBlockchainMode::LocalData) || GRIDNET->getIsShuttingDown())
		{
			tools->writeLine("Error: I was unable to initialize the TestNet Sandbox Blockchain Sub-System.");
			GRIDNET->shutdown();
			return 0;
		}

		if (GRIDNET->getWorkManager()->wasInitialised() && set->getRunShortMiningTest())
		{

			if (!abortOpenCLtest)
			{
				if (GRIDNET->runOneTimeMiningTest())
				{
					tools->writeLine("The test " + tools->getColoredString("succeeded", eColor::lightGreen) + ".");
				}
				else
				{
					tools->writeLine("The test " + tools->getColoredString("FAILED", eColor::cyborgBlood) + ".");
				}
			}
		}

		//additional tests (for development use mostly/only).
		if (set->getRunTheTests())
		{
			GRIDNET->initialize(eBlockchainMode::eBlockchainMode::LocalData);
			GRIDNET->runLocalTests();
		}


		/* Deprecated. Test-Net is now LIVE-net 
		if (false && set->getInitializeBlockchainMode(eBlockchainMode::eBlockchainMode::LIVE))
		{
			GRIDNET->markModeForInitialization(eBlockchainMode::eBlockchainMode::LIVE);
			if (!GRIDNET->initialize(eBlockchainMode::eBlockchainMode::LIVE) || GRIDNET->getIsShuttingDown()|| !GRIDNET->initialize(eBlockchainMode::eBlockchainMode::LIVESandBox) || GRIDNET->getIsShuttingDown())
			{
				tools->writeLine("Error: I was unable to initialize the LIVE Blockchain Sub-System.");
				GRIDNET->shutdown();

				if (GRIDNET->getIsShuttingDown())
				{
					std::cout << CTools::getInstance()->getColoredString("         \n  [- The app is exiting after a safe-shutdown procedure -] \n", eColor::lightGreen);
				}
				else
				{
					std::cout << CTools::getInstance()->getColoredString("         \n  [- The app is exiting ABNORMALY -]\n", eColor::cyborgBlood);
				}

				return 0;
			}
		}
		*/

		if (set->getInitializeBlockchainMode(eBlockchainMode::eBlockchainMode::TestNet))//todo: rename
		{
			GRIDNET->markModeForInitialization(eBlockchainMode::eBlockchainMode::TestNet);
			if (!GRIDNET->initialize(eBlockchainMode::eBlockchainMode::TestNet) || GRIDNET->getIsShuttingDown() || GRIDNET->getIsShuttingDown())
			{
				tools->logEvent("Fatal exception during system initialisation.", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
				tools->writeLine("Error: I was unable to initialize the TestNet Sandbox Blockchain Sub-System.");
				GRIDNET->shutdown();

				if (GRIDNET->getIsShuttingDown())
				{
					std::cout << CTools::getInstance()->getColoredString("         \n  [- The app is exiting after a safe-shutdown procedure -] \n",eColor::lightGreen);
				}
				else
				{
					std::cout << CTools::getInstance()->getColoredString("         \n  [- The app is exiting ABNORMALY -]\n", eColor::cyborgBlood);
				}

				return 0;
			}
		}

		//todo [meeting 28.02.2021]: consider marking all of the sub-systems are ready only after all of them have properly initialized.
		set->setPreviousGlobalSettingsAvailable(true);
	

		//Keep-Alive Mechanics - BEGIN
		//Rationale: the main thread needs to be kept alive for as long as the Core remains operational.
		
		if (!tools->askYesNo("Do you want to dis-activate (quit) GRIDNET Core now?", false, "", true, true, false,5, true))
		{
			//tools->activateView(eViewState::eViewState::GridScriptConsole); let the user switch view. Besides, currently on local terminal, the input thread is responsible for swithching views
			//as well. Thus being stuck at waiting for input and can't react to a view change-request in between. [todo: discuss on 28.12.2021 - LOW priority].
			CSettings::setIsGlobalAutoConfigInProgress(false);
			while (GRIDNET->getStatus() != eManagerStatus::eManagerStatus::stopped)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
			}
		}
		else {
			tools->askYesNo("I'm about to exit; say bye", true,"",false,false,true);
			GRIDNET->shutdown();
		}
		CSettings::setIsGlobalAutoConfigInProgress(false);
		//Keep-Alive Mechanics - END

	}
	catch (std::bad_alloc ex)
	{
		CTools::writeLineS("An exception probably due to low RAM memory has occurred.");
		CTools::writeLineS("Attempting the Safe-Shutdown procedure..");
			GRIDNET->shutdown();
		std::exit(1);
	}
}


std::shared_ptr<CTools> CGRIDNET::getTools()
{
	return sTools;
}

std::shared_ptr<CWorkManager> CGRIDNET::getWorkManager()
{
	std::lock_guard<std::mutex> lock(sWorkManagerGuardian);
	return mWorkManager;
}

std::shared_ptr<CBlockchainManager> CGRIDNET::getBlockchainManager(eBlockchainMode::eBlockchainMode blockchainMode, bool initializeIfNeeded)
{
	if (blockchainMode == eBlockchainMode::Unknown)
		return nullptr;

	switch (blockchainMode)
	{
	case eBlockchainMode::LIVE:
		if (initializeIfNeeded &&mLiveBlockchainManager==nullptr)
			initialize(blockchainMode);
		return mLiveBlockchainManager;
		break;
	case eBlockchainMode::TestNet:
		if (initializeIfNeeded &&mTestNetBlockchainManager == nullptr)
			initialize(blockchainMode);
		return mTestNetBlockchainManager;
		break;
	case eBlockchainMode::LIVESandBox:
		if (initializeIfNeeded &&mLiveSandBoxBlockchainManager == nullptr)
			initialize(blockchainMode);
		return mLiveSandBoxBlockchainManager;
		break;
	case eBlockchainMode::TestNetSandBox:
		if (initializeIfNeeded &&mTestNetSandBoxBlockchainManager == nullptr)
			initialize(blockchainMode);
		return mTestNetSandBoxBlockchainManager;
		break;
	case eBlockchainMode::LocalData:
		if (initializeIfNeeded &&mLocalTestBlockchainManager == nullptr)
			initialize(blockchainMode);
		return mLocalTestBlockchainManager;
		break;
	default:
		break;
	}
	return nullptr;
}

/// <summary>
/// Shutsdown the ENTIRE system, including all of the sub-systems i.e. main-net, test-net,
/// corresponding network managers, work managers, transaction managers, computation managers etc. etc.
/// </summary>
/// <returns></returns>
bool CGRIDNET::shutdown(bool doExit)
{
	if (getIsShuttingDown())
	{
		return false;
	}
	CTools::setIsThrottlingEnabled(false);
	setIsShuttingDown(true);
	try {
		if (mLocalTestBlockchainManager != nullptr)
		{
			mLocalTestBlockchainManager->exit();
		}
		if (mLiveBlockchainManager != nullptr)
		{
			mLiveBlockchainManager->exit();
		}

		if (mLiveSandBoxBlockchainManager != nullptr)
		{
			mLiveSandBoxBlockchainManager->exit();
		}

		if (mTestNetBlockchainManager != nullptr)
		{
			mTestNetBlockchainManager->exit();
		}

		if (mTestNetSandBoxBlockchainManager != nullptr)
		{
			mTestNetSandBoxBlockchainManager->exit();
		}
		CWorkManager::getInstance(sOCLEngine)->stop();
		CWorkManager::mInstance = nullptr;
		CTools::stopAllOperations();
	    CSolidStorage::closeAllDBs();
		Sleep(2000);
		if (doExit)
		{
			std::exit(0);
		}
		else
		{
			CTools::clearScreen();
			std::cout << "                       Operator! (..) You may now close the app (..)";
			while (true)
			{
				std::getchar();
			}
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool CGRIDNET::initialize(eBlockchainMode::eBlockchainMode blockchainMode )
{
	CSettings::setGlobalAutoConfigStepDescription("Initializing " + getTools()->blockchainmodeToString(blockchainMode));
	std::shared_ptr<CNetworkManager>nm;
	if (blockchainMode == eBlockchainMode::Unknown)
		return false;

	try {
		switch (blockchainMode)
		{
		case eBlockchainMode::LIVE:
			if (mLiveBlockchainManager = nullptr)
				while ((mLiveBlockchainManager = CBlockchainManager::getInstance(blockchainMode)) == nullptr)
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
			if (mLiveBlockchainManager != nullptr)
				return true;
			break;
		case eBlockchainMode::TestNet:
			if(mTestNetBlockchainManager==nullptr)
				while ((mTestNetBlockchainManager = CBlockchainManager::getInstance(blockchainMode)) == nullptr)
				{
					if (CBlockchainManager::getIsMissionAbort())
					{

						CTools::writeLineS("Quitting as part of Mission Abort Sig", "Bootstrap", eViewState::eventView, blockchainMode);
						exit(0);
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				}

			if (CBlockchainManager::getIsMissionAbort())
			{

				CTools::writeLineS("Quitting as part of Mission Abort Sig", "Bootstrap", eViewState::eventView, blockchainMode);
				exit(0);
			}
			//Wait for networking sub-system - BEGIN
			nm = mTestNetBlockchainManager->getNetworkManager();
			if (nm)
			{
				CTools::writeLineS("Waiting for the Network Manager to initialize..", "Bootstrap", eViewState::eventView, blockchainMode);
			
				while (nm->getStatus() == eManagerStatus::initial)
				{
					Sleep(100);
				}
				if (nm->getStatus() != eManagerStatus::running)
				{
					if (!mTestNetBlockchainManager->getTools()->askYesNo("Networking sub-system won't be available do you want to continue?", false, "", false, false,true,90, false))
					{
						mTestNetBlockchainManager->exit(true);
						CTools::clearScreen();
						exit(0);
					}
				}
			}
			//Wait for networking sub-system - END
			if (mTestNetBlockchainManager != nullptr)
				return true;
			break;
		case eBlockchainMode::LIVESandBox:
			if(mLiveSandBoxBlockchainManager=nullptr)
				while ((mLiveSandBoxBlockchainManager = CBlockchainManager::getInstance(blockchainMode)) == nullptr)
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
			if (mLiveSandBoxBlockchainManager != nullptr)
				return true;
			break;
		case eBlockchainMode::TestNetSandBox:
			if(mTestNetSandBoxBlockchainManager==nullptr)
				while ((mTestNetSandBoxBlockchainManager = CBlockchainManager::getInstance(blockchainMode)) == nullptr)
					std::this_thread::sleep_for(std::chrono::milliseconds(50));;
			if (mTestNetSandBoxBlockchainManager != nullptr)
				return true;
			break;
		case eBlockchainMode::LocalData:
			if (mLocalTestBlockchainManager == nullptr)
				while(( mLocalTestBlockchainManager = CBlockchainManager::getInstance(blockchainMode))==nullptr)
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
			if (mLocalTestBlockchainManager != nullptr)
				return true;
			break;
		default:
			return false;
			break;
		}
		return false;
	}
	catch (...)
	{
		return false;
	}
}

bool CGRIDNET::runOneTimeMiningTest()
{
	if (mWorkManager->wasInitialised() == false)
	{
		
		return false;
	}
	std::shared_ptr<CBlockchainManager> testBM = getBlockchainManager(eBlockchainMode::TestNet);
	std::shared_ptr<CBlockchainManager> liveBM = getBlockchainManager(eBlockchainMode::LIVE);
	bool wasLiveNetActive = liveBM != nullptr ? (liveBM->getStatus() == eManagerStatus::eManagerStatus::running) : false;
	bool wasTestNetActive = testBM != nullptr ? (testBM->getStatus() == eManagerStatus::eManagerStatus::running) : false;

	//pause test-net and live-net if active - BEGIN
	if (wasLiveNetActive)
		getBlockchainManager(eBlockchainMode::LIVE)->pause();
	if (wasTestNetActive)
		getBlockchainManager(eBlockchainMode::TestNet)->pause();
	//pause test-net and live-net if active - END

	//std::shared_ptr<CBlockchainManager> bm = getBlockchainManager(eBlockchainMode::eBlockchainMode::LocalData);
	//if (bm == nullptr)
	//	return false;

	///CTests* tests = bm->getTestsEngine();
	CTests tests(nullptr);

	
	if (!tests.testOpenCLPlatform(mWorkManager))
	{
		return false;
	}


	if (wasLiveNetActive)
		liveBM->resume();
	if (wasTestNetActive)
		testBM->resume();

	return true;
}
bool CGRIDNET::runLocalTests()
{
	bool wasLiveNetActive = getBlockchainManager(eBlockchainMode::LIVE) != nullptr ? (getBlockchainManager(eBlockchainMode::LIVE)->getStatus() == eManagerStatus::eManagerStatus::running):false;
	bool wasTestNetActive = getBlockchainManager(eBlockchainMode::TestNet) != nullptr ? (getBlockchainManager(eBlockchainMode::TestNet)->getStatus() == eManagerStatus::eManagerStatus::running):false;

	//pause test-net and live-net if active - BEGIN
	if(wasLiveNetActive)
	getBlockchainManager(eBlockchainMode::LIVE)->pause();
	if(wasTestNetActive)
	getBlockchainManager(eBlockchainMode::TestNet)->pause();
	//pause test-net and live-net if active - END

	if (mLocalTestBlockchainManager == nullptr)
		if (!initialize(eBlockchainMode::eBlockchainMode::LocalData))
			return false;
	getBlockchainManager(eBlockchainMode::LocalData)->resume();


	std::shared_ptr<CBlockchainManager>bm = getBlockchainManager(eBlockchainMode::eBlockchainMode::LocalData);
	if (bm == nullptr)
		return false;

	CTests *tests = bm->getTestsEngine();
 assertGN(tests->testGridScriptCodeSerialization());
 assertGN(tests->testVerifiablesDataStructure());
 assertGN(tests->testGenesisRewardsFactFileGenerator());
 assertGN(tests->testBlockchainBlocks(2, 2, 2));
  

 assertGN(tests->simulateTransactionsLocal());
 assertGN(tests->testDeepForksLocal());
	//assert(tests->initializeStateDB(10));
 assertGN(tests->testDB());
 assertGN(bm->setPerspective(tests->getPerspective()));
	size_t count = 0;
	bm->getLiveTransactionsManager()->getLiveDB()->testTrie(count);

	getBlockchainManager(eBlockchainMode::LocalData)->stop();

	if (wasLiveNetActive)
		getBlockchainManager(eBlockchainMode::LIVE)->resume();
	if (wasTestNetActive)
		getBlockchainManager(eBlockchainMode::TestNet)->resume();

	return true;
}

CGRIDNET::CGRIDNET()
{
	//assertGN(1 == 3);
	std::cout << "\033[2J\r\n";
	sTools = std::make_shared<CTools>("GRIDNET", eBlockchainMode::LocalData);
	std::cout << "                          Welcome " + sTools->getColoredString("Operator", eColor::lightCyan) + " \r\n";

	Sleep(2000);
	
	std::cout << "\033[2J\r\n";

	mShuttingDown = false;
	SetConsoleOutputCP(65001);
	sTools = std::make_shared<CTools>("GRIDNET", eBlockchainMode::LocalData);
	std::vector<uint8_t> bytes;
	sTools->base58CheckDecode("1nTvzGqHksnpXRsJ3QSzgo47y3kf8DW7URowJTYCr7qr5YDfSxXTZgpYPkAPc", bytes);

	//sTools->addKernelFirewallRule("", false , true);
	std::shared_ptr<CKeyChain> k = CKeyChain::instantiate(bytes);
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	Botan::SecureVector<uint8_t> sk = k->getPrivKey();
	std::vector<uint8_t> pub = k->getPubKey();
	std::vector<uint8_t> mainPub = cf->getPubFromPriv(k->getMainPrivKey());



	std::vector<uint8_t>  signature = cf->signData(bytes, sk);

	bool verified = cf->verifySignature(signature, bytes, pub);

	if (verified)
	{
		sTools->writeLine("success");
	}
	else
	{
		sTools->writeLine("failed");
	}

	if (windows)
	{
		if (!sTools->tweakWindowsTDR(30))
		{
			sTools->writeLine(sTools->getColoredString(" Critical Error:",eColor::cyborgBlood)+" TDR tweaking failed.If GPU mining enabled the software might be unstable.");
		}
		//sTools->enableWindowsUnicodeConsoleOutput();
	}
	bool runningAsAdmin = sTools->isRunningAsAdmin();
	bool canAlterFirewall = sTools->isElevatedAndInNetworkConfigurationOperatorsGroup();
	if  (runningAsAdmin || canAlterFirewall)
	{
		if (runningAsAdmin)
		{
			sTools->writeLine(sTools->getColoredString("\n\n[ Running As Administrator ]", eColor::orange) + "", true, false);
		}

		//Update Native Firewall - BEGIN
		bool firewallTweaksResult = sTools->addFirewallRule(443, eFirewallProtocol::UDP, eFirewallDirection::INBOUND, "GRIDNET Sync");
		if (firewallTweaksResult)
		{
			firewallTweaksResult = sTools->addFirewallRule(444, eFirewallProtocol::UDP, eFirewallDirection::INBOUND, "GRIDNET Sync QUIC");
		}
		if (firewallTweaksResult)
		{
			firewallTweaksResult = sTools->addFirewallRule(443, eFirewallProtocol::TCP, eFirewallDirection::INBOUND, "GRIDNET UI");
		}
		if (firewallTweaksResult)
		{
			firewallTweaksResult = sTools->addFirewallRule(22, eFirewallProtocol::TCP, eFirewallDirection::INBOUND, "GRIDNET SSH");
		}

		if (!firewallTweaksResult)
		{
			sTools->writeLine(sTools->getColoredString(" Error:", eColor::cyborgBlood) + " couldn't alter Native Firewall Rules. Make sure you do thin on your own.");
		}
		else
		{
			sTools->writeLine(sTools->getColoredString(" Firewall Updated:", eColor::lightGreen) + " native firewall rules have been updated.");
		}
		//Update Native Firewall - END

	}

	if(!runningAsAdmin)
	{
		sTools->clearScreen();
		sTools->writeLine(sTools->getColoredString( "\n\n[ Limited Permissions ]:", eColor::lightPink) + " it is advised you run GRIDNET Core with Administrative permissions.\n"+
			sTools->getColoredString("[ GRIDNET Core won't be able to ]: \n", eColor::lightPink) +
			"1) increase priority of threads (cross-node, web-sockets etc.)\n"+
			"2) tweak GPU settings (TDR value)\n"+
			"3) auto-configure Windows Firewall With Advanced Security.\n"+
			"4) detect and terminate applications or services occupying needed ports.",true,false);

		
			sTools->writeLine("You " + (canAlterFirewall? sTools->getColoredString("HAVE", eColor::lightGreen): sTools->getColoredString("DO NOT HAVE", eColor::cyborgBlood)) +" permissions to access the Kernel Firewall Module.", true, false);

		Sleep(5000);
	}
	sTools->clearScreen();
	sTools->showStatusBar(true);
	mLocalTestBlockchainManager = nullptr;
	mLiveBlockchainManager = nullptr;
	mLiveSandBoxBlockchainManager = nullptr;
	mTestNetBlockchainManager = nullptr;

	mTestNetSandBoxBlockchainManager = nullptr;
	this->sOCLEngine = std::make_shared<COCLEngine>();
	setStatus(eManagerStatus::eManagerStatus::initial);

	std::shared_ptr<CSettings>  set = CSettings::getInstance(eBlockchainMode::LocalData);

	if (set->getLoadPreviousGlobalConfiguration() || set->getDoGlobalAutoConfig(true))
	{
		CSettings::mWasLoadingPreviousGlobalConfiguration = true;
		CSettings::setIsGlobalAutoConfigInProgress(true);
	}
	else
	{
		std::cout << sTools->getColoredString("\033[2J\r\n\r\n	[Answer 'e' ]", eColor::lightCyan)+ " to any question to resume "+ sTools->getColoredString("auto - configuration",eColor::lightGreen)+".\r\n\r\n\r\n\r\n\r\n\r\n";
		Sleep(3000);
		std::cout << "\033[2J\r\n";
	}



	if (!sOCLEngine->Initialize())
	{
		std::cout << CTools::getInstance()->getColoredString("[WARNING]: I was unable to initialize the OpenCL sub-system. Computational operations (ex. 'mining' won't be available), \r\n[Hint]: Install the OpenCL Runtime environment for your computational platform(s).", eColor::cyborgBlood);
		//setStatus(eManagerStatus::stoppped);
		//return;
	}
	mWorkManager = CWorkManager::getInstance(sOCLEngine);//yeah; set an instance even though it was not initialized.

/*	CTests tests(nullptr);
	 tests.testThreadPoolInitialization();
	 tests.testThreadPoolExpansion();
	 tests.testMaximumTheadPoolLimit();
	 tests.testThreadPoolShrinkBackToMin();
	 */
	if (sOCLEngine->getWasInitialised())
	{
		
		mWorkManager->Initialise();
		mWorkManager->setIsReady();
	}
	setStatus(eManagerStatus::running);

}
void CGRIDNET::showStatusBar(bool showIt)
{
}
std::vector<uint8_t> CGRIDNET::getSelfy()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mSelfImage;
}
void CGRIDNET::setSelfy(const std::vector<uint8_t>& img)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mSelfImage= img;
}
bool CGRIDNET::getIsShuttingDown()
{
	std::lock_guard<std::mutex> lock(mShuttingDownGuardian);
	return mShuttingDown;
}
void CGRIDNET::setIsShuttingDown(bool isIt)
{
	std::lock_guard<std::mutex> lock(mShuttingDownGuardian);
	 mShuttingDown = isIt;
}
std::shared_ptr<CGRIDNET> CGRIDNET::getInstance()
{
	std::lock_guard<std::mutex> lock(sInstanceGuardian);
	if (sInstance == nullptr)
		sInstance = std::make_shared<CGRIDNET>();

	return sInstance;

}

CGRIDNET::~CGRIDNET()
{
	if(mWorkManager!=nullptr)
	mWorkManager->stop();
	mWorkManager = nullptr;
	if (mLocalTestBlockchainManager != nullptr)
	{
		mLocalTestBlockchainManager = nullptr;
	}
	if (mLiveBlockchainManager != nullptr)
	{
		 mLiveBlockchainManager = nullptr;
	}

	if (mLiveSandBoxBlockchainManager != nullptr)
	{
		  mLiveSandBoxBlockchainManager = nullptr;
	}

	if (mTestNetBlockchainManager != nullptr)
	{
		  mTestNetBlockchainManager = nullptr;
	}

	if (mTestNetSandBoxBlockchainManager != nullptr)
	{
		  mTestNetSandBoxBlockchainManager = nullptr;
	}
	CTools::stopAllOperations();
	CWorkManager::getInstance(sOCLEngine)->stop();
	std::exit(0);
	
}

void CGRIDNET::markModeForInitialization(eBlockchainMode::eBlockchainMode mode)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mModesPendingInitialization.push_back(mode);
}

bool CGRIDNET::getIsModeToBeOperational(eBlockchainMode::eBlockchainMode mode)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	for (uint64_t i = 0; i < mModesPendingInitialization.size(); i++)
	{
		if (mModesPendingInitialization[i] == mode)
		{
			return true;
		}
	
	}
	return false;
}

void CGRIDNET::stop()
{
	shutdown();
	mStatus = eManagerStatus::eManagerStatus::stopped;
}

void CGRIDNET::pause()
{
}

void CGRIDNET::resume()
{
}

eManagerStatus::eManagerStatus CGRIDNET::getStatus()
{
	std::lock_guard<std::mutex> lock(mStatusGuardian);
	return mStatus;
}

void CGRIDNET::setStatus(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusGuardian);
	mStatus = status;
}

void CGRIDNET::requestStatusChange(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	mStatusChange = status;
}

eManagerStatus::eManagerStatus CGRIDNET::getRequestedStatusChange()
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	return mStatusChange;
}
