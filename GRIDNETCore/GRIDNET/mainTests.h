#pragma once

#include "CryptoFactory.h"
#include "SolidStorage.h"

#include "oclengine.h"
#include <chrono>
#include <thread>
#include <winsock2.h>
#include <windows.h>
#include "arith_uint256.h"
#include "WorkManager.h"
#include <iomanip> 
#include <sstream>
#include "ScriptEngine.h"
#include "base58.h"
#include <string>
#include "TrieLeafNode.h"
#include "TrieBranchNode.h"
#include "TrieDB.h"
#include "SolidStorage.h"
#include "StateDomain.h"
#include "BlockchainManager.h"
#include "ScriptEngine.h"
class CSettings;
class CKeyChain;
class CTests
{
public:
	void testThreadPoolInitialization();
	void testThreadPoolExpansion();
	void testMaximumTheadPoolLimit();
	static void validateNonceInsertion();
	void testThreadPoolShrinkBackToMin();
	bool validateVarIntImplementation();
	bool testFileCertificates();
	bool testBase64();
	bool testBase58();
	void runBase58Test(std::string& encoded, std::vector<uint8_t>& vec, std::string& encodedSatoshi, bool& result, std::vector<uint8_t>& decoded, std::vector<uint8_t>& decodedSatoshi, bool& theSameData);
	CTests(std::shared_ptr<CBlockchainManager> manager, std::shared_ptr<CTools> tools = nullptr);
	bool testGridcraftPDU();
	bool testBigInts();
	bool testFileMetaDescriptors();
	void stop();
	~CTests();
	bool testGenesisRewardsFactFileGenerator();
	bool testVerifiablesDataStructure();
	bool testDomainIDs();
	bool testHDKeys();
	bool testAccounts();
	bool testTrieDBSerialization();
	bool testDirectories();
	bool testPathParsing();
	bool testVMMetaGeneratorAndParser();
	bool testConsole();
	bool testAccountsStorage();
	bool testFlows();
	bool testGridScriptParser();
	bool testDeepForksLocal();
	static bool testExclusiveWorkerMutex();
	static void debugPrintFlags(int colorWithFlags);
	static void testColorSystem();
	static void debugCompare(const std::vector<uint8_t>& vec1, const std::vector<uint8_t>& vec2);
	static void validateTargetDifficultyChain();
	static void validateTargetWindowComparison();
	bool simulateTransactionsLocal();
	bool testNetworkPacketSerialization();
	bool testNetworkPacketSecurity();
	bool simulateTransactionsNetwork();
	bool testTransactionsDataStructure();
	bool testHistoricalData();
	void interactiveGridScriptTest();
	bool testPolls();
	bool testGridScriptCodeSerialization();
	bool testJavaScriptBytecodeCrossValidation();
	bool exportCppBytecodesForJavaScriptTesting();
	bool testInSourceInvocationLookup();
	bool automatedGridScriptTest();
	bool settingsStorageTest();
	bool testBlockchainBlocks(uint32_t nrOfKeyBlocks, uint32_t nrOfRegularBlocks, uint32_t nrOfTransactionsPerRegularBlock);
	bool testChatMsgs();
	bool initializeStateDB(uint32_t nrOfRandomLeafNodes);
	bool testX25519ChaCha();
	bool testColdStorage();
	void clearRAMCache();
	bool mainTests();
	bool testDB();
	bool testMerkleProofs();
	bool testCTrieDBCopy();
	bool testTrieStructure();
	bool testTrieWithDBBackend();
	bool genTestStateDB(uint32_t stateDomainsNr, CTrieDB * stateDB, std::vector<std::vector<uint8_t>> &IDs);
	bool testOpenCLPlatform(std::shared_ptr<CWorkManager> workManager=nullptr);
	bool testIWorkInterface();
	bool testIWorkCLInterface();
	bool testIComputeTaskInterface();
	bool testIWorkHybridInterface();
	bool testComputeTaskLayer();
	
	//Multi-Dimensional Token-Pools Functionality  and Unit Tests - BEGIN
	bool testTokenPoolGenerationAndSerialization();
	bool testWebRTCSwarms();
	bool testSDPEntitiesSerializationAndAuth();
	bool testAuthenticatedTransmisionTokens();
	bool testTransmissionTokenSerialization();
	bool testTransmissionTokenUtilization(bool doCompletenessTest=false);
	bool testTransmissionTokenTransactions();
	//Multi-Dimensional Token-Pools Functionality  and Unit Tests - END
	std::vector<uint8_t>  getPerspective();

private:
	
	std::shared_ptr<CWorkManager> mWorkManager;
	std::shared_ptr<COCLEngine> mOCLEngine;
	std::shared_ptr<CSettings>  mSettings;
	CSolidStorage *mSS;
	std::shared_ptr<CTools> mTools;
	std::shared_ptr<CCryptoFactory>  mCryptoFactory;

	std::vector<std::vector<uint8_t>> testSDIDs;
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	std::shared_ptr<CStateDomainManager> mStateDomainManager;
	std::shared_ptr<CTransactionManager> mTransactionsManager;
	CTrieDB *mStateDB;
	std::shared_ptr<SE::CScriptEngine> mScriptEngine;  // shared_ptr required for shared_from_this() to work in CScriptEngine
	void assertEqual(size_t actual, size_t expected, const std::string& testName);
	void assertTrue(bool condition, const std::string& testName);

};

