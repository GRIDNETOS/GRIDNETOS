#pragma once
#include "stdafx.h"
#include <shared_mutex>
#include "robin_hood.h"
class CKeyChain;
class CSolidStorage;
class CTools;
class CSettings 
{
private:
	robin_hood::unordered_map<std::vector<uint8_t>, std::vector<uint8_t>> mSettingsCache;
	std::mutex mCacheGuardian;
	void putIntoCache(const std::string & key, const std::vector<uint8_t>& value, bool isGlobal=false);
	std::vector<uint8_t> getFromCache(const std::string& key, bool isGlobal = false);
	std::mutex mSolidStorageGuardian;
	std::recursive_mutex mWarden;
	CSolidStorage * mSolidStorage;
	std::shared_ptr<CTools> mTools;
	bool setUInt32Value( const std::string & key, uint32_t val, bool isGlobal = false);
	bool setUInt64Value(const std::string& key, uint64_t val, bool isGlobal = false);
	bool setBigSIntValue(std::string key, const BigSInt& val, bool isGlobal = false);
	bool getUInt32Value(std::string key,uint32_t &val, bool isGlobal = false, bool canUseCache = true);

	bool getBigSIntValue(std::string key, BigSInt& val, bool isGlobal = false);

	std::string mGlobalSettingsName = "Global Settings";

	bool getUInt64Value(std::string key, uint64_t& val, bool isGlobal = false, bool canUseCache = true);

	bool setBooleanValue(const std::string &key,bool val, bool isGlobal=false);
	bool getBooleanValue(std::string key, bool& val, bool isGlobal = false, bool canUseCache=true);

	bool setString(const std::string &key, std::string val, bool isGlobal = false);
	bool getString(std::string varName, std::string& str, bool isGlobal = false);

	bool setBytes(const std::string &key, std::vector<uint8_t> val, bool isGlobal = false);
	bool getBytes(std::string varName, std::vector<uint8_t>& bytes, bool isGlobal = false);
	

	eBlockchainMode::eBlockchainMode mBlockchainMode;
	
	bool mDoGlobalAutoConfigAsked;

	static bool mLoadPreviousGlobalConfigurationAsked;
	CSolidStorage * getStorage(bool getGlobal=false);
	std::vector < std::vector<uint8_t>> mAlreadyAsked;
	
	std::mutex mFieldsGuardian;
public:
	bool wasQuestionAlreadyAsked(std::string paramName, bool markIt = true);
	static bool mWasLoadingPreviousGlobalConfiguration;
	bool mLoadPreviousConfigurationAsked;
	bool mFirstRun;
	 CSettings(eBlockchainMode::eBlockchainMode blockchainMode,std::shared_ptr<CTools> tools=nullptr);
std::string getExportFolderName();
std::string getImportsFolderName();
	 //configuration preferences - BEGIN
	 bool setInitializeBlockchainMode(eBlockchainMode::eBlockchainMode mode);
	 bool getInitializeBlockchainMode(eBlockchainMode::eBlockchainMode mode);
	 bool getNrOfClonesForPlatform(std::string platformID, uint32_t& nr);

	 //ERG - begin
	 bool getClientTotalERGLimit(uint64_t &limit);
	 bool getPreviousCheckpointsCount(uint64_t& limit);
	 bool setCheckpointsCount(uint64_t count);
	 bool setClientTotalERGLimit(uint64_t limit);

	 bool getClientDefTransERGLimit(uint64_t &limit);
	 bool setClientDefTransERGLimit(uint64_t limit);

	 bool getClientDefERGPrice(BigSInt &price, const uint64_t & blockHeight);
	 bool setClientDefERGPrice(BigSInt price = 1);

	 bool setSetMinNodeERGPrice(const BigInt &price);
	 BigInt getMinNodeERGPrice(const uint64_t & blockHeight);
	 //ERG  - end 
	 bool setNrOfClonesForPlatform(std::string platformID, uint32_t nr=0);
	 bool getPreviousSettingsAvailable();
	 bool setPreviousSettingsAvailable(bool choice = true);

	 bool getPreviousGlobalSettingsAvailable();

	 bool setPreviousGlobalSettingsAvailable(bool choice);

	 bool setLoadPreviousConfiguration(bool choice = true);
	 bool setLoadPreviousGlobalConfiguration(bool choice);
	 bool getFirstRun();
	 void markIsPastFirstRun();
	 eBlockchainMode::eBlockchainMode getBlockchainMode();
	 bool getLoadPreviousConfiguration();

	 bool getLoadPreviousGlobalConfiguration();

	 void markPreviousConfigurationAsked(bool isIt = true);
	
	 bool setEnterNetworkAnalysisMode(bool choice);
	 bool setInitializeDTIServer(bool choice);
	 bool setInitializeUDTServer(bool choice);
	 bool setEnableKernelModeFirewallIntegration(bool choice);
	 bool setUseQUICForSync(bool doIt);
	 bool setUseUDTForSync(bool doIt);
	 bool setInitializeQUICServer(bool choice);
	 bool setRequireObligatoryCheckpoints(bool choice);
	 bool setInitializeFileSystemServer(bool choice);
	 bool setInitializeWebServer(bool choice);

	 bool setInitializeCORSProxy(bool choice);
	 bool getInitializeCORSProxy();
	 bool setInitializeWebSocketsServer(bool choice);
	 bool setInitializeKademliaServer(bool choice);
	 bool getInitializeDTIServer();
	 bool getInitializeWebServer();
	 bool getInitializeWebSocketsServer();
	 bool getInitializeFileSystemServer();
	 bool getInitializeUDTServer(bool defaultV=true);
	 bool getEnableKernelModeFirewallIntegration(bool defaultV);
	 bool getUseQUICForSync(bool defaultV=true);
	 bool getUseUDTForSync(bool defaultV=true);
	 bool getInitializeQUICServer(bool doIt=true);
	 bool getInitializeKademliaProtocol(bool defaultV=true);
	 bool setDoGlobalAutoConfig(bool choice);
	 bool getDoGlobalAutoConfig(bool defaultV);
	 bool getRequireObligatoryCheckpoints();
	 static bool getIsGlobalAutoConfigInProgress();
	 static bool getIsPostAutoConfig();
	 static void  setIsPostAutoConfig(bool isIt=true);
	 static bool broadcastLocalEventsToRemoteTerminals();
	 static void setIsGlobalAutoConfigInProgress(bool isIt=true);
	 static void setGlobalAutoConfigStepDescription(std::string & description);
	 static std::string getGlobalAutoConfigStepDescription();
	 bool setDoStateSynchronization(bool choice);
	 bool setDisableExternalBlockProcessing(bool choice);
	 bool getGetDisableExternalBlockProcessing();
	 bool getDisableExternalChainProofProcessing();
	 bool setDisableExternalChainProofProcessing(bool choice);

	 bool setDoBlockFormation(bool choice = true);
	 bool getDoBlockFormation(bool defaultV=true);

	 bool getEnterNetworkAnalysisMode(bool defaultV);

	 bool getDoStateSynchronization(bool defaultV = true);

	 bool setDoInitialStateDBTest(bool choice = true);
	 bool getDoInitialStateDBTest();

	 bool setDoInitialBlockAvailabilityVerification(bool choice = true);
	 bool getDoInitialBlockAvailabilityVerification();

	 bool setDoInitialChainProofAnalysis(bool choice = true);
	 bool getDoInitialChainProofAnalysis();
	 
	 bool setEnableNetworkingSubSystem(bool choice = true);
	 bool getEnableNetworkingSubSystem(bool defaultV=true);

	 bool setForceUsageOfOnlyBootstrapNodes(bool choice);

	 bool getForceUsageOfOnlyBootstrapNodes();



	 bool setRunAsNetworkBootstrapNode(bool choice = true);
	 bool getRunAsNetworkBootstrapNode();

	 bool setRunTheTests(bool choice = true);
	 bool getRunTheTests();

	 bool setRunShortMiningTests(bool choice);



	 bool getRunShortMiningTest();

	 bool setLogEvents(eLogEntryCategory::eLogEntryCategory category, eLogEntryType::eLogEntryType nType, bool choice = true);
	 bool getLogEvents(eLogEntryCategory::eLogEntryCategory category,  eLogEntryType::eLogEntryType nType);



	 bool setUseGPUs(bool choice = true);
	 bool getUseGPUs();

	 bool setUseCPUs(bool choice = true);
	 bool getUseCPUs();

	 std::vector<std::vector<uint8_t>> getWhitelistedIPs(bool& valueFromStorage);

	 bool setWhitelistedIPs(const std::vector<std::string>& IPs);
	 bool setWhitelistedIPs(const std::vector<std::vector<uint8_t>>& IPsAsBytes);

	 bool setLocalIPv4(std::string ip);

	

	 std::string getPublicIPv4(bool& valueFromStorage, std::string detectedPublicIPv4="", std::string criticalFallbackValue = "");

	 bool setPublicIPv4(std::string ipv4);

	 bool setUIDebuggingIPv4(std::string ipv4);

	 std::string getUIDebuggingIPv4();

	 std::vector<std::string> getBootstrapIPv4s(bool& valueFromStorage, bool interactiveMode=true, bool forceAskForAdditionalNodes=false, const std::vector<std::string>& defaultBootstrapIPv4s = std::vector<std::string>());

	 bool setBootstrapIPv4s(const std::vector<std::string>& ipv4s);


	

	 std::string getFocusOnIPv4();

	 bool setFocusOnIPv4(std::string ipv4);

	 std::string getLocalIPv4(bool& valueFromStorage,std::string detectedLocalIPv4="" );

	 bool setUsePlatformID(std::string platformID,bool choice=true);
	 bool getUsePlatformID(std::string platformID);

	 bool setEnableOpenCLProgramCaching(bool choice = true);
	 bool getEnableOpenCLProgramCaching();

	 bool setUseRandomERGPriceDuringTests(bool choice = true);
	 bool getUseRandomERGPriceDuringTests();

	 bool setDoTrieDBTestAfterEachRound(bool choice = true);
	 bool getDoTrieDBTestAfterEachRound();

	
	 //configuration preferenced - END
	static bool sIsAutoConfigInProgress;
	static bool sIsPostAutoConfig;
	static std::string sAutoConfigInProgressStepDescription;
	static std::mutex sConfigurationStatus;
	static std::shared_ptr<CSettings>  sLIVEInstance;
	static std::shared_ptr<CSettings> sLIVESandBoxInstance;
	static std::shared_ptr<CSettings> sTestNetInstance;
	static std::shared_ptr<CSettings> sTestNetSandBoxInstance;
	static std::shared_ptr<CSettings>  sLocalDataInstance;
	static  std::shared_ptr<CSettings> getInstance(eBlockchainMode::eBlockchainMode blockchainMode);
	uint64_t getCurrentMiningReward();
	bool  setCurrentMiningReward(uint64_t reward);
	bool setMinDifficultyCoefficientSummedTimeDiffs(double minDiff);
	double getMinDifficultyCoefficientSummedTimeDiffs();
	bool setMinDifficultyCoefficientTimeSlotsCount(uint64_t count);
	uint64_t getMinDifficultyCoefficientTimeSlotsCount();
	bool setEMALastBlockIndex(uint64_t lastBlockIndex);
	uint64_t getEMALastBlockIndex();
	bool setEMATimeBetweenBlocks(double time);
	double getEMATimeBetweenBlocks();
	bool setEMADifficulty(double time);
	double getEMADifficulty();
	bool setEMADifficultyCoefficient(double summedDiff);
	double getEMADifficultyCoefficient();
	std::string getMinersKeyChainID();
	double getMinSynchronizationPerc();
	bool setMinSynchronizationPerc(double perc);
	static uint32_t getMaxCmdLineBufferLength();
	static uint32_t getCmdLineHeight();
	static char getAbortExecHotKey();
	static char getExitTerminalHotKey();
	static char getEventViewHotKey();
	static char getWallViewHotKey();
	static char getGridScriptViewHotKey();
	static uint32_t getCmdLineHistoryLength();
	static uint32_t getWallHistoryLength();
	bool setMinersKeyChainID(std::string sdID);
	bool setMinersKeyChainID(std::vector<uint8_t> sdID);

	bool setNewIdentityEveryNumberOfKeyBlocks(uint64_t number);

	uint64_t getNewIdentityEveryNumberOfKeyBlocks(uint32_t defaultL=2);

	bool setMinEventPriorityForConsole(uint64_t priority);

	bool setNumberOfMiningIdentitiesToUse(uint64_t number);

	uint64_t getNumberOfMiningIdentitiesToUse(uint32_t defaultL=1);

	bool setEnableAutomaticIDSwitching(bool choice);

	bool getEnableAutomaticIDSwitching(bool defaultV);


	uint64_t getMinEventPriorityForConsole(uint32_t defaultL=1);
	std::shared_ptr<CTools> getTools();
	bool saveKeyChain(CKeyChain chain);
	bool saveKeyChain(std::vector<uint8_t> serializedKeyChain,std::string name);
	Botan::secure_vector<uint8_t>  getSerializedKeyChain(std::string name);
	bool  getCurrentKeyChain(CKeyChain &chain, bool markAsUsed = true,bool notifyAboutResult=true, std::string name = "");

};

