#include "Settings.h"
#pragma once
#include "Settings.h"
#include "SolidStorage.h" 
#include "CGlobalSecSettings.h"
#include "BlockchainManager.h"
#include "keyChain.h"


#include <shared_mutex>
bool  CSettings::sIsAutoConfigInProgress = false;
bool CSettings::sIsPostAutoConfig = false;
std::string  CSettings::sAutoConfigInProgressStepDescription;
std::mutex CSettings::sConfigurationStatus;

std::shared_ptr<CSettings> CSettings::sLIVEInstance = nullptr;
std::shared_ptr<CSettings> CSettings::sLIVESandBoxInstance = nullptr;
std::shared_ptr<CSettings>  CSettings::sTestNetInstance = nullptr;
std::shared_ptr<CSettings>  CSettings::sTestNetSandBoxInstance = nullptr;
std::shared_ptr<CSettings> CSettings::sLocalDataInstance = nullptr;

/**
 * @brief Stores an unsigned 32-bit integer setting.
 *
 * This function stores the value both in the persistent storage and the in-house Robin-Hood based cache.
 * It ensures that once a value is written to storage, the cache has the fresh value for optimized read
 * operations later on.
 *
 * @param key The unique key corresponding to the desired setting.
 * @param val The unsigned 32-bit integer value to be stored.
 * @param isGlobal A flag indicating whether the setting is of global nature or not.
 *                 Depending on its value, the function may interact with a different storage
 *                 or cache section.
 *
 * @return true if the value is successfully stored.
 *         false if there are any issues during the storage process.
 *
 * @exception The function might encounter exceptions due to storage issues,
 *            access violations, and the likes. It's designed to catch them and return false.
 *
 * @note Cache management is crucial. Be wary of scenarios where the cache might contain stale
 *       data. Ensure the cache is appropriately invalidated or updated when related changes occur.
 */
bool CSettings::setUInt32Value(const std::string& key, uint32_t val, bool isGlobal)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	std::shared_ptr<CTools> tools = getTools();

	try {
		std::string valString = std::to_string(val);

		// Save to the Cold Storage.
		bool result = getStorage(isGlobal)->saveValue(key, valString);
		if (result) {
			// Only on successful save to Cold Storage, update the in-house cache as well.
			putIntoCache(key, tools->stringToBytes(valString), isGlobal);
		}
		return result;
	}
	catch (int e) {
		return false;
	}
}


/**
 * @brief Stores an unsigned 64-bit integer setting.
 *
 * This function stores the value both in the persistent storage and the in-house Robin-Hood based cache.
 * It ensures that once a value is written to storage, the cache has the fresh value for optimized read
 * operations later on.
 *
 * @param key The unique key corresponding to the desired setting.
 * @param val The unsigned 64-bit integer value to be stored.
 * @param isGlobal A flag indicating whether the setting is of global nature or not.
 *                 Depending on its value, the function may interact with a different storage
 *                 or cache section.
 *
 * @return true if the value is successfully stored.
 *         false if there are any issues during the storage process.
 *
 * @exception The function might encounter exceptions due to storage issues,
 *            access violations, and the likes. It's designed to catch them and return false.
 *
 * @note Cache management is crucial. Be wary of scenarios where the cache might contain stale
 *       data. Ensure the cache is appropriately invalidated or updated when related changes occur.
 */
bool CSettings::setUInt64Value(const std::string& key, uint64_t val, bool isGlobal)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	std::shared_ptr<CTools> tools = getTools();

	try {
		std::string valString = std::to_string(val);

		// Save to the Cold Storage.
		bool result = getStorage(isGlobal)->saveValue(key, valString);
		if (result) {
			// Only on successful save to Cold Storage, update the in-house cache as well.
			putIntoCache(key, tools->stringToBytes(valString), isGlobal);
		}
		return result;
	}
	catch (int e) {
		return false;
	}
}


bool CSettings::setBigSIntValue(std::string key, const BigSInt& val, bool isGlobal)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	try {
		return getStorage(isGlobal)->saveValue(key, mTools->BigSIntToBytes(val));
	}
	catch (int e)
	{
		return false;
	}
	return true;
}



std::string CSettings::getExportFolderName()
{
	return "exports";
}

std::string CSettings::getImportsFolderName()
{
	return "imports";
}


/**
 * @brief Retrieves an unsigned 32-bit integer setting.
 *
 * This function aims to efficiently fetch settings. It primarily checks
 * a Robin-Hood based in-house cache for the desired value. If the value
 * is not present or caching is disabled, it then resorts to fetching
 * from a more persistent storage, presumably RocksDB in this context.
 *
 * @param key The key corresponding to the desired setting.
 * @param[out] val The reference where the fetched value will be stored.
 * @param isGlobal Flag to denote whether the setting is global or not.
 *                 Depending on this, the function might opt for a different
 *                 storage or cache section.
 * @param canUseCache Flag that enables or disables the usage of the Robin-Hood cache.
 *                    True by default. If set to false, the function will directly
 *                    access the persistent storage.
 *
 * @return Returns true if the value is successfully fetched and is valid.
 *         Returns false otherwise, including cases where the value isn't
 *         present or there are exceptions during the fetch.
 *
 * @exception This function might throw exceptions due to data conversions,
 *            access violations, etc., although it's primarily designed to
 *            catch them and return false in such cases.
 *
 * @note When using caching, it's essential to ensure that the cached values
 *       are always updated or invalidated correctly whenever related changes
 *       occur. This avoids potential issues stemming from reading stale data.
 */
bool CSettings::getUInt32Value(std::string key, uint32_t& val, bool isGlobal, bool canUseCache)
{
	std::shared_ptr<CTools> tools = getTools();

	try {
		// Try the Local Robin-Hood based cache first.
		std::vector<uint8_t> retS = canUseCache ? getFromCache(key, isGlobal) : std::vector<uint8_t>();
		if (retS.empty())
		{
			// If no luck with the cache, then use RocksDB.
			retS = getStorage(isGlobal)->getValue(key);

			// Only use the in-house cache when the value is not empty.
			if (!retS.empty())
			{
				putIntoCache(key, retS, isGlobal); // So that it's available from cache later on.
			}
		}

		if (retS.empty())
			return false;

		// Convert the bytes to a string, then to an unsigned 32-bit integer.
		val = std::stoul(tools->bytesToString(retS));
		return true;
	}
	catch (...)
	{
		return false;
	}
}


bool CSettings::getBigSIntValue(std::string key, BigSInt& val, bool isGlobal)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	try {
		std::string sdId;
		std::vector<uint8_t> retS = getStorage(isGlobal)->getValue(key);
		if (retS.size() == 0)
			return false;
		BigSInt retrieved = mTools->BytesToBigSInt(retS);
		val = retrieved;
		return true;
	}
	catch (...)
	{
		return false;
	}
}
/**
 * @brief Retrieves an unsigned 64-bit integer setting.
 *
 * This function is designed to fetch settings in an optimized manner. Initially, it checks
 * the Robin-Hood based in-house cache for the value. If the cache doesn't have the value or
 * caching is turned off, it then fetches from the persistent storage, which is presumed
 * to be RocksDB in this context.
 *
 * @param key The unique key corresponding to the desired setting.
 * @param[out] val Reference where the fetched value will be stored upon successful retrieval.
 * @param isGlobal A flag indicating whether the setting is of global nature or not.
 *                 Depending on its value, the function may interact with a different storage
 *                 or cache section.
 * @param canUseCache Optional flag that enables or disables the use of the Robin-Hood cache.
 *                    Enabled by default. When disabled, the function fetches data directly
 *                    from the persistent storage.
 *
 * @return true if the value is successfully retrieved and is valid.
 *         false if there are any issues like the value not being present or exceptions
 *         during the retrieval process.
 *
 * @exception The function might encounter exceptions due to conversion errors,
 *            access violations, and the likes. It's designed to catch them and return false.
 *
 * @note Cache management is crucial. Always ensure cached values are updated or invalidated
 *       appropriately when any related changes happen to prevent potential stale data issues.
 */
bool CSettings::getUInt64Value(std::string key, uint64_t& val, bool isGlobal, bool canUseCache)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	try {
		// Try the Robin-Hood based cache first.
		std::vector<uint8_t> retS = canUseCache ? getFromCache(key, isGlobal) : std::vector<uint8_t>();

		if (retS.empty()) {
			retS = getStorage(isGlobal)->getValue(key);

			// Only use the in-house cache when the value is not empty.
			if (!retS.empty()) {
				putIntoCache(key, retS, isGlobal);
			}
		}

		if (retS.empty()) {
			return false;
		}

		uint64_t retrieved = std::stol(mTools->bytesToString(retS));
		val = retrieved;

		return true;
	}
	catch (...) {
		return false;
	}
}

/// <summary>
/// Stores a boolean setting.
/// </summary>
/// <param name="key"></param>
/// <param name="val"></param>
/// <param name="isGlobal"></param>
/// <returns></returns>
bool CSettings::setBooleanValue(const std::string& key, bool val, bool isGlobal)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	std::shared_ptr<CTools> tools = getTools();
	try {

		std::string valString = val ? "1" : "0";
		bool result = getStorage(isGlobal)->saveValue(std::string(key), valString);
		if (result)
		{
			//only on success in Cold Storage, use the in-house cache as well.
			putIntoCache(std::string(key), tools->stringToBytes(valString), isGlobal);
		}
		return result;
	}
	catch (int e)
	{
		return false;
	}
}

bool CSettings::setString(const std::string& key, std::string val, bool isGlobal)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	try {
		return getStorage(isGlobal)->saveValue(key, val);
	}
	catch (int e)
	{
		return false;
	}
	return true;
}

bool CSettings::setBytes(const std::string& key, std::vector<uint8_t> val, bool isGlobal)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	try {
		return getStorage(isGlobal)->saveValue(key, val);
	}
	catch (int e)
	{
		return false;
	}
	return true;
}
void CSettings::putIntoCache(const std::string& key, const std::vector<uint8_t>& value, bool isGlobal)
{
	// Prepare the key outside of the critical section
	std::string key2 = key + (isGlobal ? "_G" : "_L");
	std::vector<uint8_t> keyData(key2.begin(), key2.end());

	// Now we enter the critical section and perform the update
	std::vector<uint8_t> k = CCryptoFactory::getInstance()->getSHA2_256Vec(keyData);
	std::lock_guard<std::mutex> lock(mCacheGuardian);
	mSettingsCache[k] = value;
}

/// <summary>
/// Retrieves data from cache.
/// </summary>
/// <param name="getGlobal"></param>
/// <returns></returns>
std::vector<uint8_t> CSettings::getFromCache(const std::string& key, bool isGlobal)
{
	std::string key2 = key + (isGlobal ? "_G" : "_L");
	std::vector<uint8_t> keyVec(key2.begin(), key2.end());
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::lock_guard<std::mutex> lock(mCacheGuardian);
	auto it = mSettingsCache.find(cf->getSHA2_256Vec(keyVec));
	if (it != mSettingsCache.end())
	{
		return it->second;
	}
	return {};
}

/// <summary>
/// Retrieves a boolean setting.
/// </summary>
/// <param name="getGlobal"></param>
/// <returns></returns>
bool CSettings::getBooleanValue(std::string key, bool& val, bool isGlobal, bool canUseCache)
{
	std::shared_ptr<CTools> tools = getTools();
	//std::lock_guard<std::recursive_mutex> lock(mWarden); <- not needed.
	try {
		//Try the Local Robin-Hood based cache first.
		std::vector<uint8_t> retS = canUseCache ? getFromCache(std::string(key), isGlobal) : std::vector<uint8_t>();
		if (retS.empty())
		{
			retS = getStorage(isGlobal)->getValue(std::string(key)); //if no luck then use RocksDB.

			//Only use the in-house cache when the value is not empty.
			if (!retS.empty())
			{
				putIntoCache(std::string(key), retS, isGlobal);//so that it is available from cache later on.
			}
		}

		if (retS.empty())
			return false;

		val = (std::stoi(tools->bytesToString(retS)) == 1);
		return true;
	}
	catch (...)
	{
		return false;
	}
}



CSolidStorage* CSettings::getStorage(bool getGlobal)
{
	std::lock_guard<std::mutex> lock(mSolidStorageGuardian);
	if (!getGlobal)
		return mSolidStorage;
	else
		return CSolidStorage::getInstance(eBlockchainMode::LocalData);
}

bool CSettings::wasQuestionAlreadyAsked(std::string paramName, bool markIt)
{
	if (!paramName.size())
	{
		return false;
	}
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	std::vector<uint8_t> image = CCryptoFactory::getInstance()->getSHA2_256Vec(tools->stringToBytes(paramName));

	for (uint64_t i = 0; i < mAlreadyAsked.size(); i++)
	{
		if (tools->compareByteVectors(mAlreadyAsked[i], image))
		{
			return true;
		}
	}
	if (markIt)
	{
		mAlreadyAsked.push_back(image);
	}
	return false;
}

/// <summary>
/// CSettings constructor does not take boolean testNet indicator as it instantiates both
/// and particular function parameters take the indicator and use the appropriate DB pointer.
/// </summary>
CSettings::CSettings(eBlockchainMode::eBlockchainMode blockchainMode, std::shared_ptr<CTools> tools)
{
	//mLoadPreviousGlobalConfigurationAsked = false;

	mLoadPreviousConfigurationAsked = false;
	mFirstRun = false;
	mDoGlobalAutoConfigAsked = false;
	mBlockchainMode = blockchainMode;
	if (tools == nullptr)
		mTools = std::make_shared<CTools>("Settings", blockchainMode);
	else mTools = tools;
	mSolidStorage = CSolidStorage::getInstance(blockchainMode);
}

bool CSettings::setInitializeBlockchainMode(eBlockchainMode::eBlockchainMode mode)
{
	return setBooleanValue("initializeBlockchain" + std::to_string(mode), mode, true);
}

bool CSettings::getInitializeBlockchainMode(eBlockchainMode::eBlockchainMode mode)
{
	bool defaultVal = true;
	bool retrievedVal = false;

	switch (mode)
	{
	case eBlockchainMode::LIVE:
		defaultVal = false;
		break;
	case eBlockchainMode::TestNet:
		defaultVal = true;
		break;
	case eBlockchainMode::LIVESandBox:
		defaultVal = true;
		break;
	case eBlockchainMode::TestNetSandBox:
		defaultVal = true;
		break;
	case eBlockchainMode::LocalData:
		defaultVal = true;
		break;
	default:
		break;
	}

	if ((getPreviousGlobalSettingsAvailable() && getLoadPreviousGlobalConfiguration() || wasQuestionAlreadyAsked("initializeBlockchain")) && getBooleanValue("initializeBlockchain" + std::to_string(mode), retrievedVal, true))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Do you want to initialize the " + mTools->blockchainmodeToString(mode) + " Blockchain?", defaultVal, mGlobalSettingsName, true, true, false, 5, true);
		setBooleanValue("initializeBlockchain" + std::to_string(mode), retrievedVal, true);
		//assert(setDoBlockFormation(retrievedVal));
	}
	return retrievedVal;
}

bool CSettings::getNrOfClonesForPlatform(std::string platformID, uint32_t& nr)
{

	uint32_t defaultVal = 0;
	uint32_t retrievedVal = 0;
	std::string setID = ("clones_" + platformID);
	if ((getLoadPreviousGlobalConfiguration() || wasQuestionAlreadyAsked(setID)) && getUInt32Value(setID, retrievedVal, true))
	{
		nr = retrievedVal;
		return true;

	}
	else
	{
		retrievedVal = static_cast<uint32_t>(mTools->askInt("How many clones to create (1-20) on platform " + platformID + " ?", defaultVal, mGlobalSettingsName, true));
		setNrOfClonesForPlatform(platformID, retrievedVal);
	}
	nr = retrievedVal;
	return true;

}
bool CSettings::getClientTotalERGLimit(uint64_t& limit)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	uint64_t defaultVal = 1;
	uint64_t retrievedVal = 0;

	if (getLoadPreviousConfiguration() && getUInt64Value(H_CLIENT_TOTAL_ERG_LIMIT, retrievedVal))
	{
		limit = retrievedVal;
		return true;
	}
	else
	{
		retrievedVal = static_cast<uint64_t>(mTools->askInt("Define the total ERG limit for all outbound transactions:", defaultVal));
		setClientTotalERGLimit(retrievedVal);
		limit = retrievedVal;
	}
	return true;
}

bool CSettings::setClientTotalERGLimit(uint64_t limit)
{
	return setUInt32Value(H_CLIENT_TOTAL_ERG_LIMIT, limit);
}

bool CSettings::getPreviousCheckpointsCount(uint64_t& count)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	uint64_t defaultVal = CBlockchainManager::getForcedResyncSequenceNumber(getBlockchainMode());
	uint64_t retrievedVal = 0;

	if (getUInt64Value(H_CHECKPOINTS_COUNT, retrievedVal))
	{
		count = retrievedVal;
		return true;
	}
	else
	{
		retrievedVal = defaultVal;
		setCheckpointsCount(retrievedVal);
		count = retrievedVal;
	}
	return true;
}

bool CSettings::setCheckpointsCount(uint64_t count)
{
	return setUInt32Value(H_CHECKPOINTS_COUNT, count);
}





bool CSettings::getClientDefTransERGLimit(uint64_t& limit)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	uint64_t defaultVal = 1;
	uint64_t retrievedVal = 0;
	if (getLoadPreviousConfiguration() && getUInt64Value(H_CLIENT_DEF_TRANS_ERG_LIMIT, retrievedVal))
	{
		limit = retrievedVal;
		return true;
	}
	else
	{
		retrievedVal = static_cast<uint64_t>(mTools->askInt("Define default Transaction ERG limit:", defaultVal));
		setClientDefTransERGLimit(retrievedVal);
		limit = retrievedVal;
	}
	return true;
}

bool CSettings::setClientDefTransERGLimit(uint64_t limit)
{
	return setUInt32Value(H_CLIENT_DEF_TRANS_ERG_LIMIT, limit);
}

bool CSettings::getClientDefERGPrice(BigSInt& price, const uint64_t& blockHeight)
{

	std::lock_guard<std::recursive_mutex> lock(mWarden);

	BigSInt defaultVal = CGlobalSecSettings::getDefaultERGpriceInGBU(blockHeight);
	BigSInt retrievedVal = 0;
	std::string setID = H_CLIENT_DEF_ERG_BID;
	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked(setID)) && getBigSIntValue(setID, retrievedVal, false)) // it's not global but a per sub-net setting.
	{
		retrievedVal = retrievedVal;
		return true;

	}
	else
	{
		retrievedVal = static_cast<uint32_t>(mTools->askInt("Provide minimum acceptable ERG price: ", defaultVal, mGlobalSettingsName, true));
		setClientDefERGPrice(static_cast<BigInt>(retrievedVal));
	}
	price = static_cast<BigInt>(retrievedVal);
	return true;

}

bool CSettings::setClientDefERGPrice(BigSInt price)
{
	return setBigSIntValue(H_CLIENT_DEF_ERG_BID, price, false);
}

bool CSettings::setNrOfClonesForPlatform(std::string platformID, uint32_t nr)
{
	return setUInt32Value(platformID + "_clones", nr, true);
}

bool CSettings::getPreviousSettingsAvailable()
{

	bool defaultVal = false;
	bool retrievedVal = false;
	if (mTools->isDTI())
		return false;
	if (getBooleanValue("PreviousSettingsAvailable", retrievedVal))
	{
		return retrievedVal;
	}
	else return defaultVal;
}

bool CSettings::setPreviousSettingsAvailable(bool choice)
{
	return setBooleanValue("PreviousSettingsAvailable", choice);
}

bool CSettings::getPreviousGlobalSettingsAvailable()
{

	bool defaultVal = false;
	bool retrievedVal = false;
	if (getBooleanValue("PreviousGlobalSettingsAvailable", retrievedVal, true))
	{
		return retrievedVal;
	}
	else return defaultVal;
}
bool CSettings::mWasLoadingPreviousGlobalConfiguration = false;
bool CSettings::setPreviousGlobalSettingsAvailable(bool choice)
{
	return setBooleanValue("PreviousGlobalSettingsAvailable", choice, true);
}

bool CSettings::setLoadPreviousConfiguration(bool choice)
{
	return setBooleanValue("LoadPreviousConfiguration", choice);
}

bool CSettings::setLoadPreviousGlobalConfiguration(bool choice)
{
	return setBooleanValue("LoadPreviousGlobalConfiguration", choice);
}



bool CSettings::getFirstRun()
{
	bool val = false;
	return !getBooleanValue("isPastFirstCoreRun", val);
}

void CSettings::markIsPastFirstRun()
{
	setBooleanValue("isPastFirstCoreRun", true);
}

eBlockchainMode::eBlockchainMode CSettings::getBlockchainMode()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mBlockchainMode;
}


bool CSettings::getLoadPreviousConfiguration()
{
	std::shared_ptr<CTools> tools = getTools();
	if (!getPreviousSettingsAvailable())//|| getFirstRun())
	{
		return false;
	}
	bool defaultVal = true;
	bool retrievedVal = false;
	eBlockchainMode::eBlockchainMode bm = getBlockchainMode();
	if (bm == eBlockchainMode::LIVESandBox || bm == eBlockchainMode::TestNetSandBox)
		return true;
	if ((getIsPostAutoConfig() || mWasLoadingPreviousGlobalConfiguration || mLoadPreviousConfigurationAsked || wasQuestionAlreadyAsked("LoadPreviousConfiguration")) && getBooleanValue("LoadPreviousConfiguration", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = tools->askYesNo("Do you want me to load the previous configuration?", defaultVal);
		setLoadPreviousConfiguration(retrievedVal);
		mLoadPreviousConfigurationAsked = true;
	}
	return retrievedVal;

}

bool CSettings::getLoadPreviousGlobalConfiguration()
{
	if (!getPreviousGlobalSettingsAvailable())
	{
		mLoadPreviousGlobalConfigurationAsked = true;
		return false;
	}
	bool defaultVal = true;
	bool retrievedVal = false;
	if (mLoadPreviousGlobalConfigurationAsked && getBooleanValue("LoadPreviousGlobalConfiguration", retrievedVal, true))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Do you want me to load the previous GLOBAL configuration?", defaultVal, mGlobalSettingsName, true, false, false, 5, true);
		setLoadPreviousGlobalConfiguration(retrievedVal);
		mLoadPreviousGlobalConfigurationAsked = true;
	}
	return retrievedVal;

}

void CSettings::markPreviousConfigurationAsked(bool isIt)
{
	mLoadPreviousGlobalConfigurationAsked = isIt;
}
bool CSettings::mLoadPreviousGlobalConfigurationAsked = false;


bool CSettings::setEnterNetworkAnalysisMode(bool choice)
{
	return setBooleanValue("EnterNetworkAnalysis", choice);
}

bool CSettings::setDoStateSynchronization(bool choice)
{
	return setBooleanValue("DoStateSynchronization", choice);
}

bool CSettings::setDoBlockFormation(bool choice)
{
	return setBooleanValue("DoBlockFormation", choice);
}

bool CSettings::getGetDisableExternalBlockProcessing()
{
	bool defaultVal = false;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("DisableExternalBlockProcessing", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal =  defaultVal;
		setDisableExternalBlockProcessing(retrievedVal);
	}
	return retrievedVal;
}


bool CSettings::getDisableExternalChainProofProcessing()
{
	bool defaultVal = false;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("DisableExternalChainProofProcessing", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = defaultVal;
		setDisableExternalChainProofProcessing(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setDisableExternalChainProofProcessing(bool choice)
{
	return setBooleanValue("DisableExternalChainProofProcessing", choice);
}


bool CSettings::setDisableExternalBlockProcessing(bool choice)
{
	return setBooleanValue("DisableExternalBlockProcessing", choice);
}

bool CSettings::getDoBlockFormation(bool defaultV)
{
	bool defaultVal = defaultV;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("DoBlockFormation", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Do Block Formation?", defaultVal);
		setDoBlockFormation(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::getEnterNetworkAnalysisMode(bool defaultV)
{


	bool defaultVal = defaultV;
	bool retrievedVal = false;

	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked("EnterNetworkAnalysis")) && getBooleanValue("EnterNetworkAnalysis", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Enter Network Analysis Mode?", defaultVal);
		setEnterNetworkAnalysisMode(retrievedVal);
	}
	return retrievedVal;


}

bool CSettings::getDoStateSynchronization(bool defaultV)
{
	bool defaultVal = defaultV;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("DoStateSynchronization", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Do State Synchronization?", defaultVal);
		setDoStateSynchronization(retrievedVal);
	}
	return retrievedVal;
}
//
bool CSettings::setDoGlobalAutoConfig(bool choice)
{

	return setBooleanValue("GlobalAutoConfig", choice, true);
}

bool CSettings::getDoGlobalAutoConfig(bool defaultV)
{
	bool defaultVal = true;
	bool retrievedVal = false;
	//bool previousGlobal = getLoadPreviousGlobalConfiguration();
	if (mDoGlobalAutoConfigAsked && getBooleanValue("GlobalAutoConfig", retrievedVal, true))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Initialize Global Auto-Configuration (Experimental)?", defaultVal, mGlobalSettingsName, true, false, true, 180, false);

		setDoGlobalAutoConfig(retrievedVal);
		mDoGlobalAutoConfigAsked = true;
	}
	return retrievedVal;
}



bool CSettings::getRequireObligatoryCheckpoints()
{
	bool defaultVal = true;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("requireObligatoryCheckpoints", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Require obligatory checkpoints?", defaultVal);
		setRequireObligatoryCheckpoints(retrievedVal);
	}
	return retrievedVal;
}


bool CSettings::getIsGlobalAutoConfigInProgress()
{
	std::lock_guard<std::mutex> lock(sConfigurationStatus);
	return sIsAutoConfigInProgress;
}

bool CSettings::getIsPostAutoConfig()
{
	std::lock_guard<std::mutex> lock(sConfigurationStatus);
	return sIsPostAutoConfig;
}
void CSettings::setIsPostAutoConfig(bool isIt)
{
	std::lock_guard<std::mutex> lock(sConfigurationStatus);
	sIsPostAutoConfig = isIt;
}


bool CSettings::broadcastLocalEventsToRemoteTerminals()
{
	return true;
}

void CSettings::setIsGlobalAutoConfigInProgress(bool isIt)
{
	std::lock_guard<std::mutex> lock(sConfigurationStatus);
	sIsAutoConfigInProgress = isIt;
	if (isIt)
	{
		sIsPostAutoConfig = isIt;
	}
}

void CSettings::setGlobalAutoConfigStepDescription(std::string& description)
{
	std::lock_guard<std::mutex> lock(sConfigurationStatus);
	sAutoConfigInProgressStepDescription = description;
}



std::string CSettings::getGlobalAutoConfigStepDescription()
{
	std::lock_guard<std::mutex> lock(sConfigurationStatus);
	return sAutoConfigInProgressStepDescription;
}

bool CSettings::setInitializeDTIServer(bool choice)
{
	return setBooleanValue("InitializeDTIServer", choice);
}

bool CSettings::setInitializeUDTServer(bool choice)
{
	return setBooleanValue("InitializeUDTServer", choice);
}

bool CSettings::setEnableKernelModeFirewallIntegration(bool choice)
{
	return setBooleanValue("KernelModeIntegration", choice);
}

bool CSettings::setUseQUICForSync(bool doIt)
{
	return setBooleanValue("QUICSyncEnabled", doIt);
}

bool CSettings::setUseUDTForSync(bool doIt)
{
	return setBooleanValue("UDTSyncEnabled", doIt);
}
bool CSettings::setInitializeQUICServer(bool choice)
{
	return setBooleanValue("InitializeQUICServer", choice);
}


bool CSettings::setRequireObligatoryCheckpoints(bool choice)
{
	return setBooleanValue("requireObligatoryCheckpoints", choice);
}

bool CSettings::setInitializeFileSystemServer(bool choice)
{
	return setBooleanValue("InitializeFileSystemServer", choice);
}

bool CSettings::setInitializeWebServer(bool choice)
{
	return setBooleanValue("InitializeWebServer", choice);
}

bool CSettings::setInitializeCORSProxy(bool choice)
{
	return setBooleanValue("InitializeCORSProxy", choice);
}

bool CSettings::getInitializeCORSProxy()
{
	bool defaultVal = true;
	bool retrievedVal = false;

	if (getLoadPreviousConfiguration() && getBooleanValue("InitializeCORSProxy", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Initialize CORS proxy?", defaultVal);
		setInitializeWebSocketsServer(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setInitializeWebSocketsServer(bool choice)
{
	return setBooleanValue("InitializeWebSocketsServer", choice);
}

bool CSettings::setInitializeKademliaServer(bool choice)
{
	return setBooleanValue("InitializeKademliaServer", choice);
}

bool CSettings::getInitializeDTIServer()
{
	bool defaultVal = true;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("InitializeDTIServer", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Initialize DTI Server?", defaultVal);
		setInitializeDTIServer(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::getInitializeWebServer()
{
	bool defaultVal = true;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("InitializeWebServer", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Initialize Web(HTTP/HTTPS) Server?", defaultVal);
		setInitializeWebServer(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::getInitializeWebSocketsServer()
{
	bool defaultVal = true;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("InitializeWebSocketsServer", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Initialize Web-Sockets Server?", defaultVal);
		setInitializeWebSocketsServer(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::getInitializeFileSystemServer()
{
	bool defaultVal = true;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("InitializeFileSystemServer", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Initialize File-System Server?", defaultVal);
		setInitializeFileSystemServer(retrievedVal);
	}
	return retrievedVal;
}



bool CSettings::getInitializeUDTServer(bool defaultV)
{

	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("InitializeUDTServer", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Initialize UDT Server?", defaultV);
		setInitializeUDTServer(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::getEnableKernelModeFirewallIntegration(bool defaultV)
{
	bool defaultVal = defaultV;
	bool retrievedVal = false;

	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked("KernelModeIntegration")) && getBooleanValue("KernelModeIntegration", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Enable Kernel-mode Firewall integration?", defaultVal);
		setEnableKernelModeFirewallIntegration(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::getUseQUICForSync(bool defaultV)
{
	bool defaultVal = defaultV;
	bool retrievedVal = false;

	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked("QUICSyncEnabled")) && getBooleanValue("QUICSyncEnabled", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Use QUIC protocol for synchronization?", defaultVal);
		setUseQUICForSync(retrievedVal);
	}
	return retrievedVal;
}


bool CSettings::getUseUDTForSync(bool defaultV)
{
	bool defaultVal = defaultV;
	bool retrievedVal = false;

	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked("UDTSyncEnabled")) && getBooleanValue("UDTSyncEnabled", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Use UDT protocol for synchronization?", defaultVal);
		setUseUDTForSync(retrievedVal);
	}
	return retrievedVal;
}
bool CSettings::getInitializeQUICServer(bool doIt)
{
	bool defaultVal = doIt;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("InitializeQUICServer", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Initialize QUIC Server?", defaultVal);
		setInitializeQUICServer(retrievedVal);
	}
	return retrievedVal;
}


bool CSettings::getInitializeKademliaProtocol(bool defaultV)
{
	bool defaultVal = defaultV;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("InitializeKademliaServer", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Initialize the P2P Discovery?", defaultVal);
		setInitializeKademliaServer(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setDoInitialStateDBTest(bool choice)
{
	return setBooleanValue("DoInitialStateDBTest", choice);
}

bool CSettings::getDoInitialStateDBTest()
{
	bool defaultVal = false;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("DoInitialStateDBTest", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Do you want me to test the entire local StateDB? (this will take some time)", defaultVal);
		setDoInitialStateDBTest(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setDoInitialBlockAvailabilityVerification(bool choice)
{
	return setBooleanValue("DoInitialBlockAvailabilityVerification", choice);
}

bool CSettings::getDoInitialBlockAvailabilityVerification()
{
	bool defaultVal = true;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("DoInitialBlockAvailabilityVerification", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Do you want to verify the Local Block-Data Availability?", defaultVal);
		setDoInitialBlockAvailabilityVerification(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setDoInitialChainProofAnalysis(bool choice)
{
	return setBooleanValue("DoInitialChainProofAnalysis", choice);
}

bool CSettings::getDoInitialChainProofAnalysis()
{
	bool defaultVal = true;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("DoInitialChainProofAnalysis", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Do you want to do a one-time meticulous Encoded Chain-Proof analysis ?", defaultVal);
		setDoInitialChainProofAnalysis(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setEnableNetworkingSubSystem(bool choice)
{
	return setBooleanValue("EnableNetworkingSubSystem", choice);
}

bool CSettings::getEnableNetworkingSubSystem(bool defaultV)
{
	bool defaultVal = defaultV;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("EnableNetworkingSubSystem", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Enable the Networking Sub-System?", defaultVal);
		setEnableNetworkingSubSystem(retrievedVal);
	}
	return retrievedVal;
}


bool CSettings::setForceUsageOfOnlyBootstrapNodes(bool choice)
{
	return setBooleanValue("ForceUseOnlyBootstrapNode", choice);
}

bool CSettings::getForceUsageOfOnlyBootstrapNodes()
{
	bool defaultVal = false;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("ForceUseOnlyBootstrapNode", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Connect only with Bootstrap nodes?", defaultVal);
		setForceUsageOfOnlyBootstrapNodes(retrievedVal);
	}
	return retrievedVal;
}


bool CSettings::setRunAsNetworkBootstrapNode(bool choice)
{
	return setBooleanValue("RunAsNetworkBootstrapNode", choice);
}

bool CSettings::getRunAsNetworkBootstrapNode()
{
	bool defaultVal = false;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("RunAsNetworkBootstrapNode", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Run as Network Bootstrap Node?", defaultVal);
		setRunAsNetworkBootstrapNode(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setRunTheTests(bool choice)
{
	return setBooleanValue("RunTheTests", choice, true);
}

bool CSettings::getRunTheTests()
{
	bool defaultVal = false;
	bool retrievedVal = false;
	if (getLoadPreviousGlobalConfiguration() && getBooleanValue("RunTheTests", retrievedVal, true))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Run the Security/Integrity/Validation Tests?", defaultVal);
		setRunTheTests(retrievedVal);
	}
	return retrievedVal;
}
bool CSettings::setRunShortMiningTests(bool choice)
{
	return setBooleanValue("RunMiningTest", choice, true);
}

bool CSettings::getRunShortMiningTest()
{
	bool defaultVal = true;
	bool retrievedVal = false;
	if (getLoadPreviousGlobalConfiguration() && getBooleanValue("RunMiningTest", retrievedVal, false))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Run a quick OpenCL (mining) test?", defaultVal);
		setRunShortMiningTests(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setLogEvents(eLogEntryCategory::eLogEntryCategory category, eLogEntryType::eLogEntryType nType, bool choice)
{
	std::string logID = "Log_" + std::to_string(category) + "_" + std::to_string(nType);
	return  setBooleanValue(logID, choice);;
}



bool CSettings::getLogEvents(eLogEntryCategory::eLogEntryCategory category, eLogEntryType::eLogEntryType nType)
{
	std::string logID = "Log_" + std::to_string(category) + "_" + std::to_string(nType);
	bool defaultVal = true;
	bool retrievedVal = false;

	//Default Values - BEGIN
	switch (category)
	{
	case eLogEntryCategory::network:
		defaultVal = false;
		break;
	case eLogEntryCategory::localSystem:
		break;
	case eLogEntryCategory::VM:
		break;
	case eLogEntryCategory::debug:
		break;
	case eLogEntryCategory::unknown:
		defaultVal = false;
		break;
	default:

		break;
	}
	switch (nType)
	{
	case eLogEntryType::notification:
		break;
	case eLogEntryType::warning:
		break;
	case eLogEntryType::failure:
		break;
	case eLogEntryType::unknown:
		defaultVal = false;
		break;
	default:
		break;
	}

	//Default Values - END

	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked(logID)) && getBooleanValue(logID, retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Log " + mTools->logEntryCategoryToStr(category) + " events of type " + mTools->logEntryTypeToStr(nType) + "?", defaultVal);
		setLogEvents(category, nType);
	}
	return retrievedVal;
}


bool CSettings::setUseGPUs(bool choice)
{
	return setBooleanValue("UseGPUs", choice, true);
}

bool CSettings::getUseGPUs()
{
	bool defaultVal = true;
	bool retrievedVal = false;

	if (getLoadPreviousGlobalConfiguration() && getBooleanValue("UseGPUs", retrievedVal, true))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Use the available GPUs?", defaultVal, mGlobalSettingsName, true, false, false, 10, true);
		setUseGPUs(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setUseCPUs(bool choice)
{
	return setBooleanValue("UseCPUs", choice, true);
}

bool CSettings::getUseCPUs()
{
	bool defaultVal = false;
	bool retrievedVal = false;
	if (getLoadPreviousGlobalConfiguration() && getBooleanValue("UseCPUs", retrievedVal, true))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Use the available CPUs?", defaultVal, mGlobalSettingsName, true, false, false, 10, true);
		setUseCPUs(retrievedVal);
	}
	return retrievedVal;
}


bool CSettings::getBytes(std::string varName, std::vector<uint8_t>& bytes, bool isGlobal)
{
	bytes = getStorage()->getValue(varName);
	if (bytes.size() > 0)
		return true;
	else return false;
}

bool CSettings::getString(std::string varName, std::string& str, bool isGlobal)
{
	str = mTools->bytesToString(getStorage(isGlobal)->getValue(varName));
	if (str.size() > 0)
		return true;
	else return false;
}



/// <summary>
/// Must be a valid concrete IPv4 address. The address is assigned to the Eth0 Virtual Interface.
///  This typically would be the address of a NAT, if present.
/// </summary>
/// <param name="valueFromStorage"></param>
/// <param name="detectedPublicIPv4"></param>
/// <param name="criticalFallbackValue"></param>
/// <returns></returns>
std::string CSettings::getPublicIPv4(bool& valueFromStorage, std::string detectedPublicIPv4, std::string criticalFallbackValue)//fall-back is localIPAddress
{
	//Local Variables - BEGIN
	bool defaultVal = true;
	std::string ipStr = detectedPublicIPv4;
	uint64_t tries = 0;
	//Local Variables - END
	std::shared_ptr<CTools> tools = CTools::getTools();
	if (getLoadPreviousGlobalConfiguration() && getString("publicIpv4", ipStr, true) && mTools->validateIPv4(ipStr) &&
		!tools->doStringsMatch(ipStr, "0.0.0.0"))//public address needs to be explicit.
	{
		valueFromStorage = true;
		return ipStr;
	}
	else
	{
		valueFromStorage = false;

		if (!sIsAutoConfigInProgress)
		{
			ipStr.clear();

			while ((!mTools->validateIPv4(ipStr) || tools->doStringsMatch("0.0.0.0", ipStr) || tools->doStringsMatch("0.0.0.0/0", ipStr)) && tries < 3)
			{
				ipStr = mTools->askString("Which IPv4 am I to use as 'public'? (ex.for QR-codes)", detectedPublicIPv4, mGlobalSettingsName, true);

				tries++;
			}
		}
		else
		{
			ipStr = detectedPublicIPv4;

		}

		if (!mTools->validateIPv4(ipStr))
		{
			mTools->writeLine("Falling back to a critical fall-back value for an IPv4 address:" + criticalFallbackValue);
			ipStr = criticalFallbackValue;
		}


		if (!setPublicIPv4(ipStr))
		{
			mTools->writeLine("There was an error writing IPv4 setting. Falling back to a critical fall-back value for an IPv4 address:" + criticalFallbackValue);
			return criticalFallbackValue;
		}
	}
	return ipStr;

}



bool CSettings::setPublicIPv4(std::string ipv4)
{
	return setString("publicIpv4", ipv4, true);
}


bool CSettings::setUIDebuggingIPv4(std::string ipv4)
{
	return setString("debuggingIpv4", ipv4, true);
}


/// <summary>
/// Must be a valid concrete IPv4 address. The address is the one which is allowed to access the DUI once in Debug UI mode.
///  This typically would be the address of a NAT, if present.
/// </summary>
/// <param name="valueFromStorage"></param>
/// <param name="detectedPublicIPv4"></param>
/// <param name="criticalFallbackValue"></param>
/// <returns></returns>
std::string CSettings::getUIDebuggingIPv4()//fall-back is localIPAddress
{
	std::string ipStr;

	getString("debuggingIpv4", ipStr, true);

	if (mTools->validateIPv4(ipStr) == false)
	{
		ipStr = "127.0.0.1"; // default
	}

	return ipStr;
}/// <summary>
/// Retrieves a list of IPv4 addresses assigned to bootstrap nodes. This can include wildcard addresses. 
/// These addresses are assigned to Eth1 Virtual Interfaces, typically indicating devices
/// directly available at this node. The IP addresses are not published externally.
/// 
/// Order of address retrieval:
/// 1. Attempt to load from storage if allowed by configuration.
/// 2. If interactive mode is enabled (and auto-config is not in progress), prompt the user.
/// 3. If still no addresses, use provided default addresses.
/// 4. If all else fails, use the critical fallback value.
///
/// This method ensures that at least one valid IPv4 address is returned.
/// </summary>
/// <param name="valueFromStorage">[out] Set to true if the returned values were obtained from storage</param>
/// <param name="interactiveMode">If true, user may be prompted for inputs unless auto-config is active</param>
/// <param name="forceAskForAdditionalNodes">If true, user interaction will continue asking for nodes even if some are already known</param>
/// <param name="defaultBootstrapIPv4s">A default set of bootstrap IPv4s if none are in storage and interactive mode is not used</param>
/// <param name="criticalFallbackValue">A guaranteed fallback IPv4 address if no other source provides a valid address</param>
/// <returns>A vector of valid bootstrap IPv4 addresses</returns>
std::vector<std::string> CSettings::getBootstrapIPv4s(
	bool& valueFromStorage,
	bool interactiveMode,
	bool forceAskForAdditionalNodes,
	const std::vector<std::string>& defaultBootstrapIPv4s)
{
	bool userProvided = false;
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	valueFromStorage = false;
	std::vector<std::string> addresses;

	// 1. Attempt to load from storage if configured or forced.
	if (!sIsAutoConfigInProgress || getLoadPreviousGlobalConfiguration())
	{
		std::vector<uint8_t> encodedData;
		if (getBytes("bootstrapIpv4s", encodedData, true))
		{
			std::vector<std::vector<uint8_t>> decodedIPBytes;
			if (mTools->VarLengthDecodeVector(encodedData, decodedIPBytes))
			{
				// Convert and validate stored IPs
				for (const auto& ipBytes : decodedIPBytes)
				{
					std::string ip = mTools->bytesToString(ipBytes);
					if (mTools->validateIPv4(ip))
					{
						addresses.push_back(ip);
					}
				}

				// If we successfully retrieved at least one valid IP from storage
				if (!addresses.empty())
				{
					valueFromStorage = true;
					// Directly return these addresses; they are trusted previous config.
					return addresses;
				}
				else
				{
					mTools->writeLine("No valid IPv4 addresses found in stored configuration. Will attempt other methods.");
				}
			}
			else
			{
				mTools->writeLine("Failed to decode stored bootstrap IPv4 addresses. Will attempt other methods.");
			}
		}
		else
		{
			mTools->writeLine("No stored IPv4 addresses found or read error. Will attempt other methods.");
		}
	}

	// 2. If interactive mode is enabled and auto-config is not in progress, prompt the user.
	if (interactiveMode && !sIsAutoConfigInProgress)
	{
		// Prompt the user for additional nodes until 'done' or no longer forced.
		// A maximum attempt count could be introduced to prevent infinite loops:
		const int maxAttempts = 20; // Example safeguard
		int attempts = 0;

		while (true)
		{
			attempts++;
			// If we already have addresses and not forced to ask for more, break out.
			if (!addresses.empty() && !forceAskForAdditionalNodes)
			{
				break;
			}

			// Suggest the first default IPv4 if available, else no suggestion.
			std::string suggestion = defaultBootstrapIPv4s.empty() ? "" : defaultBootstrapIPv4s.front();

			std::string ip = mTools->askString(
				"Enter IPv4 address of a Bootstrap node (or 'done' to finish):",
				suggestion,
				mGlobalSettingsName,
				true
			);

			if (ip == "done")
			{
				break;
			}

			if (mTools->validateIPv4(ip))
			{
				userProvided = true;
				// Only add if it's not already present
				if (std::find(addresses.begin(), addresses.end(), ip) == addresses.end())
				{
					addresses.push_back(ip);
				}
				else
				{
					mTools->writeLine("This IP is already included. Please enter another or 'done'.");
				}
			}
			else
			{
				mTools->writeLine("Invalid IPv4 address. Please try again.");
			}

			// Prevent infinite loops due to user error or refusal
			if (attempts >= maxAttempts)
			{
				mTools->writeLine("Maximum attempts reached. Proceeding with collected addresses or fallback.");
				break;
			}
		}
	}
	else
	{
		// 3. If not interactive or auto-config in progress, just use the default addresses.
		addresses = defaultBootstrapIPv4s;
	}


	// Return the final list of addresses. At least one valid address is guaranteed here.
	return addresses;
}


/// <summary>
/// Sets the list of bootstrap IPv4 addresses after validating them and removing duplicates
/// </summary>
/// <param name="ipv4s">Vector of IPv4 addresses to store</param>
/// <returns>True if successfully stored, false otherwise</returns>
bool CSettings::setBootstrapIPv4s(const std::vector<std::string>& ipv4s)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	// Validate and deduplicate addresses
	std::set<std::string> uniqueAddresses;
	std::vector<std::vector<uint8_t>> ipBytes;

	for (const auto& ip : ipv4s)
	{
		if (mTools->validateIPv4(ip) && uniqueAddresses.insert(ip).second)
		{
			ipBytes.push_back(mTools->stringToBytes(ip));
		}
	}

	if (ipBytes.empty()) {
		return false;
	}

	// Encode and store the addresses
	std::vector<uint8_t> encoded = mTools->VarLengthEncodeVector(ipBytes);
	return setBytes("bootstrapIpv4s", encoded);
}
//
std::string CSettings::getFocusOnIPv4()//fallback is localIPAddress
{
	std::string ipStr;

	if (getString("focusOnIPv4", ipStr, true) && mTools->validateIPv4(ipStr))
	{
		return ipStr;
	}

	return "";
}

bool CSettings::setFocusOnIPv4(std::string ipv4)
{
	return setString("focusOnIPv4", ipv4, true);
}



std::string CSettings::getLocalIPv4(bool& valueFromStorage, std::string detectedIPv4)
{
	//Local Variables - BEGIN
	bool defaultVal = true;
	std::string ipStr = detectedIPv4;
	std::string criticalFallbackValue = "0.0.0.0";
	uint64_t tries = 0;
	//Local Variables - END

	if (getLoadPreviousGlobalConfiguration() && getString("localIpv4", ipStr, true) && mTools->validateIPv4(ipStr))
	{
		valueFromStorage = true;
		return ipStr;
	}
	else
	{

		ipStr = criticalFallbackValue;

	}
	return ipStr;

}


bool CSettings::setUsePlatformID(std::string platformID, bool choice)
{
	return setBooleanValue("UsePlatformID_" + platformID, choice, true);
}


///


std::vector<std::vector<uint8_t>>  CSettings::getWhitelistedIPs(bool& valueFromStorage)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	// Initialize return value
	std::vector<std::vector<uint8_t>>   whitelistedIPs;
	valueFromStorage = false;

	// Early return if loading previous configuration is disabled
	if (!getLoadPreviousGlobalConfiguration()) {
		return whitelistedIPs;
	}

	// Load encoded data
	std::vector<uint8_t> encodedData;
	if (!getBytes("whitelistedIPs", encodedData, true)) {
		return whitelistedIPs;
	}

	// Decode the data
	std::vector<std::vector<uint8_t>> decodedIPBytes;
	if (!mTools->VarLengthDecodeVector(encodedData, decodedIPBytes)) {
		return whitelistedIPs;
	}

	// Convert and validate IPs
	for (const auto& ipBytes : decodedIPBytes) {
		std::string ip = mTools->bytesToString(ipBytes);
		if (mTools->validateIPv4(ip)) {
			whitelistedIPs.push_back(mTools->stringToBytes(ip));
		}
	}

	// Set success flag if we found valid IPs
	if (!whitelistedIPs.empty()) {
		valueFromStorage = true;
	}

	return whitelistedIPs;
}
bool CSettings::setWhitelistedIPs(const std::vector<std::vector<uint8_t>>& IPsAsBytes)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	// Input validation
	if (IPsAsBytes.empty()) {
		return false;
	}

	// Validate each IP
	for (const auto& ipBytes : IPsAsBytes) {
		std::string ip = mTools->bytesToString(ipBytes);
		if (!mTools->validateIPv4(ip)) {
			return false;
		}
	}

	// Encode the vector of IP bytes
	std::vector<uint8_t> encoded = mTools->VarLengthEncodeVector(IPsAsBytes);
	if (encoded.empty()) {
		return false;
	}

	// Save to storage
	return setBytes("whitelistedIPs", encoded);
}

bool CSettings::setWhitelistedIPs(const std::vector<std::string>& IPs)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	// Input validation
	if (IPs.empty()) {
		return false;
	}

	// Convert IPs to vector format and validate
	std::vector<std::vector<uint8_t>> IPsAsBytes;
	for (const auto& ip : IPs) {
		if (!mTools->validateIPv4(ip)) {
			return false;
		}
		IPsAsBytes.push_back(mTools->stringToBytes(ip));
	}

	// Use the byte vector version to avoid code duplication
	return setWhitelistedIPs(IPsAsBytes);
}

bool CSettings::setLocalIPv4(std::string ipv4)
{
	return setString("localIpv4", ipv4, true);
}
///

bool CSettings::getUsePlatformID(std::string platformID)
{
	bool defaultVal = true;
	bool retrievedVal = false;
	bool likelyEmbedded = false;

	if (platformID.find("Intel") != std::string::npos)
	{
		likelyEmbedded = true;
		defaultVal = false;
	}

	std::string settID = ("UsePlatformID_" + platformID);

	if ((getLoadPreviousGlobalConfiguration() || wasQuestionAlreadyAsked(settID)) && getBooleanValue(settID, retrievedVal, true))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Use " + platformID + " platform" + std::string(likelyEmbedded ? " (integrated)" : " (dedicated)") + " ?", defaultVal, mGlobalSettingsName, true, false, false, 180, true);
		setUsePlatformID(platformID, retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setEnableOpenCLProgramCaching(bool choice)
{
	return setBooleanValue("EnableOpenCLProgramCaching", choice, true);
}

bool CSettings::getEnableOpenCLProgramCaching()
{
	bool defaultVal = true;
	bool retrievedVal = false;
	if (getLoadPreviousGlobalConfiguration() && getBooleanValue("EnableOpenCLProgramCaching", retrievedVal, true))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Enable OpenCL program caching? ", defaultVal, mGlobalSettingsName, true, false, false, 180, true);
		setEnableOpenCLProgramCaching(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setUseRandomERGPriceDuringTests(bool choice)
{
	return setBooleanValue("UseRandomERGPriceDuringTests", choice);
}

bool CSettings::getUseRandomERGPriceDuringTests()
{
	bool defaultVal = true;
	bool retrievedVal = false;

	if (getLoadPreviousConfiguration() && getBooleanValue("UseRandomERGPriceDuringTests", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Use Random ERG price during tests?", defaultVal);
		setUseRandomERGPriceDuringTests(retrievedVal);
	}
	return retrievedVal;
}

bool CSettings::setDoTrieDBTestAfterEachRound(bool choice)
{
	return setBooleanValue("DoTrieDBTestAfterEachRound", choice);
}
bool CSettings::getDoTrieDBTestAfterEachRound()
{
	bool defaultVal = false;
	bool retrievedVal = false;
	if (getLoadPreviousConfiguration() && getBooleanValue("DoTrieDBTestAfterEachRound", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Test TrieDB after each test transaction-round?", defaultVal);
		setDoTrieDBTestAfterEachRound(retrievedVal);
	}
	return retrievedVal;
}

std::shared_ptr<CSettings> CSettings::getInstance(eBlockchainMode::eBlockchainMode blockchainMode)
{
	switch (blockchainMode)
	{
	case eBlockchainMode::LIVE:
		if (sLIVEInstance == nullptr)
		{
			sLIVEInstance = std::make_shared<CSettings>(blockchainMode);
		}
		return sLIVEInstance;
		break;
	case eBlockchainMode::TestNet:
		if (sTestNetInstance == nullptr)
		{
			sTestNetInstance = std::make_shared<CSettings>(blockchainMode);
		}
		return sTestNetInstance;
		break;
	case eBlockchainMode::LIVESandBox:
		if (sLIVESandBoxInstance == nullptr)
		{
			sLIVESandBoxInstance = std::make_shared<CSettings>(blockchainMode);
		}
		return sLIVESandBoxInstance;
		break;
	case eBlockchainMode::TestNetSandBox:
		if (sTestNetSandBoxInstance == nullptr)
		{
			sTestNetSandBoxInstance = std::make_shared<CSettings>(blockchainMode);
		}
		return sTestNetSandBoxInstance;
		break;

	case eBlockchainMode::LocalData:
		if (sLocalDataInstance == nullptr)
		{
			sLocalDataInstance = std::make_shared<CSettings>(blockchainMode);
		}
		return sLocalDataInstance;
		break;
	default:
		assertGN(false);//should not happen.
		break;
	}
	return nullptr;

}


///

uint64_t CSettings::getCurrentMiningReward()
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::vector<uint8_t>  bv = getStorage()->getValue(H_REWARD);
	if (bv.size() != 0)
	{
		base_blob<256> asd(bv);
		uint256 diffInt = uint256(asd);
		return   UintToArith256(diffInt).GetLow64();
	}
	else
	{
		return 4128500000;
	}
}

bool CSettings::setCurrentMiningReward(uint64_t reward)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	std::vector<unsigned char> rewardV;
	rewardV.resize(32);
	arith_uint256  re256 = reward;
	re256.GetBytes((char*)rewardV.data());
	try {
		return getStorage()->saveValue(H_REWARD, rewardV);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}

bool CSettings::setMinDifficultyCoefficientSummedTimeDiffs(double minDiff)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	std::vector<unsigned char> minDiffV;
	minDiffV.resize(8);
	std::memcpy(minDiffV.data(), &minDiff, 8);
	try {
		return getStorage()->saveValue(H_MIN_DIFF + "diffCoEffSummedTime", minDiffV);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}

double CSettings::getMinDifficultyCoefficientSummedTimeDiffs()
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::vector<uint8_t>  bv = getStorage()->getValue(H_MIN_DIFF + "diffCoEffSummedTime");
	if (bv.size() == 8)
	{
		double  pt = *reinterpret_cast<double*>(bv.data());
		if (pt <= 0)
			return 0;

		return pt;

	}
	else
	{
		//mTools->writeLine("*WARNING: no current minimal difficulty known! Falling back to difficulty 1!*");
		return 0;
	}
}

bool CSettings::setMinDifficultyCoefficientTimeSlotsCount(uint64_t minDiff)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	std::vector<unsigned char> minDiffV;
	minDiffV.resize(8);
	std::memcpy(minDiffV.data(), &minDiff, 8);
	try {
		return getStorage()->saveValue(H_MIN_DIFF + "diffCoEffSlotsCount", minDiffV);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}

uint64_t CSettings::getMinDifficultyCoefficientTimeSlotsCount()
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::vector<uint8_t>  bv = getStorage()->getValue(H_MIN_DIFF + "diffCoEffSlotsCount");
	if (bv.size() == 8)
	{
		uint64_t  pt = *reinterpret_cast<uint64_t*>(bv.data());
		if (pt <= 0)
			return 0;

		return pt;

	}
	else
	{
		//mTools->writeLine("*WARNING: no current minimal difficulty known! Falling back to difficulty 1!*");
		return 0;
	}
}
//
bool CSettings::setEMALastBlockIndex(uint64_t lastBlockIndex)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	std::vector<unsigned char> valueBytes;
	valueBytes.resize(8);
	std::memcpy(valueBytes.data(), &lastBlockIndex, 8);
	try {
		return getStorage()->saveValue(H_MIN_DIFF + "EMALastBlockIndex", valueBytes);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}

uint64_t CSettings::getEMALastBlockIndex()
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::vector<uint8_t>  bv = getStorage()->getValue(H_MIN_DIFF + "EMALastBlockIndex");
	if (bv.size() == 8)
	{
		uint64_t  pt = *reinterpret_cast<uint64_t*>(bv.data());
		if (pt <= 0)
			return 0;

		return pt;

	}
	else
	{
		//mTools->writeLine("*WARNING: no current minimal difficulty known! Falling back to difficulty 1!*");
		return 0;
	}
}
//
bool CSettings::setEMATimeBetweenBlocks(double time)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	std::vector<unsigned char> minDiffV;
	minDiffV.resize(8);
	std::memcpy(minDiffV.data(), &time, 8);
	try {
		return getStorage()->saveValue(H_MIN_DIFF + "EMATime", minDiffV);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}

double CSettings::getEMATimeBetweenBlocks()
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::vector<uint8_t>  bv = getStorage()->getValue(H_MIN_DIFF + "EMATime");
	if (bv.size() == 8)
	{
		double  pt = *reinterpret_cast<double*>(bv.data());
		if (pt <= 0)
			return 0;

		return pt;

	}
	else
	{
		//mTools->writeLine("*WARNING: no current minimal difficulty known! Falling back to difficulty 1!*");
		return 0;
	}
}

bool CSettings::setEMADifficulty(double time)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	std::vector<unsigned char> minDiffV;
	minDiffV.resize(8);
	std::memcpy(minDiffV.data(), &time, 8);
	try {
		return getStorage()->saveValue(H_MIN_DIFF + "EMADiff", minDiffV);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}

double CSettings::getEMADifficulty()
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::vector<uint8_t>  bv = getStorage()->getValue(H_MIN_DIFF + "EMADiff");
	if (bv.size() == 8)
	{
		double  pt = *reinterpret_cast<double*>(bv.data());
		if (pt <= 0)
			return 0;

		return pt;

	}
	else
	{
		//mTools->writeLine("*WARNING: no current minimal difficulty known! Falling back to difficulty 1!*");
		return 0;
	}
}

bool CSettings::setEMADifficultyCoefficient(double summedDiff)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	std::vector<unsigned char> minDiffV;
	minDiffV.resize(8);
	std::memcpy(minDiffV.data(), &summedDiff, 8);
	try {
		return getStorage()->saveValue(H_MIN_DIFF + "diffCoEffSummedDiff", minDiffV);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}

double CSettings::getEMADifficultyCoefficient()
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::vector<uint8_t>  bv = getStorage()->getValue(H_MIN_DIFF + "diffCoEffSummedDiff");
	if (bv.size() == 8)
	{
		double  pt = *reinterpret_cast<double*>(bv.data());
		if (pt <= 0)
			return 0;

		return pt;

	}
	else
	{
		//mTools->writeLine("*WARNING: no current minimal difficulty known! Falling back to difficulty 1!*");
		return 0;
	}
}

std::string CSettings::getMinersKeyChainID()
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::string sdId;
	std::vector<uint8_t> mid = getStorage()->getValue("minersChainID");
	if (mid.size() == 0)
		return sdId;

	return mTools->bytesToString(mid);
}

double CSettings::getMinSynchronizationPerc()
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::vector<uint8_t>  bv = getStorage()->getValue(H_MIN_SYNC + "main");
	if (bv.size() == 8)
	{
		double  pt = *reinterpret_cast<double*>(bv.data());
		return pt;

	}
	else
	{
		mTools->writeLine("*WARNING: no current min. synchronization perc. set ! Falling back to 90%!");
		return 90;
	}
}

bool CSettings::setMinSynchronizationPerc(double perc)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	std::vector<unsigned char> minPercV;
	minPercV.resize(8);
	std::memcpy(minPercV.data(), &perc, 8);
	try {
		return getStorage()->saveValue(H_MIN_SYNC + "main", minPercV);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}



uint32_t CSettings::getMaxCmdLineBufferLength()
{
	return 1000000;
}

uint32_t CSettings::getCmdLineHeight()
{
	return 30;
}

char CSettings::getAbortExecHotKey()
{
	return '\x3';
}

char CSettings::getExitTerminalHotKey()
{
	return '\x4';
}

char CSettings::getEventViewHotKey()
{
	return 23;
}

char CSettings::getWallViewHotKey()
{
	return 17;
}

char CSettings::getGridScriptViewHotKey()
{
	return 5;
}

uint32_t CSettings::getCmdLineHistoryLength()
{
	return 30 * 3;
}

uint32_t CSettings::getWallHistoryLength()
{
	return 30 * 3;
}

bool CSettings::setMinersKeyChainID(std::string sdID)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	return getStorage()->saveValue("minersChainID", sdID);
}
bool CSettings::setMinersKeyChainID(std::vector<uint8_t> sdID)
{
	return setMinersKeyChainID(mTools->bytesToString(sdID));
}


bool CSettings::setNewIdentityEveryNumberOfKeyBlocks(uint64_t  number)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	try {
		return setUInt32Value("newIdentityEveryBlocks", number);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}


uint64_t CSettings::getNewIdentityEveryNumberOfKeyBlocks(uint32_t defaultL)
{
	std::shared_ptr<CTools> tools = getTools();
	uint32_t defaultVal = defaultL;
	uint32_t retrievedVal = false;

	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked("newIdentityEveryBlocks")) && getUInt32Value("newIdentityEveryBlocks", retrievedVal))
	{
		return static_cast<uint64_t>(retrievedVal);
	}
	else
	{
		retrievedVal = static_cast<uint32_t>(tools->askInt("Change Identity after how many Key Blocks?", defaultVal));
		setNewIdentityEveryNumberOfKeyBlocks(retrievedVal);
	}
	return static_cast<uint64_t>(retrievedVal);
}


bool CSettings::setNumberOfMiningIdentitiesToUse(uint64_t  number)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	try {
		return setUInt32Value("numberOfMiningIdentitiesToUse", number);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}


uint64_t CSettings::getNumberOfMiningIdentitiesToUse(uint32_t defaultL)
{
	std::shared_ptr<CTools> tools = getTools();
	uint32_t defaultVal = defaultL;
	uint32_t retrievedVal = false;

	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked("numberOfMiningIdentitiesToUse")) && getUInt32Value("numberOfMiningIdentitiesToUse", retrievedVal))
	{
		return static_cast<uint64_t>(retrievedVal);
	}
	else
	{
		retrievedVal = 0;

		while (!retrievedVal)
		{
			retrievedVal = static_cast<uint32_t>(tools->askInt("Number of Identities for Mining Operations?", defaultVal));
		}

		setNumberOfMiningIdentitiesToUse(retrievedVal);
	}
	return static_cast<uint64_t>(retrievedVal);
}


bool CSettings::setEnableAutomaticIDSwitching(bool choice)
{
	return setBooleanValue("DoIDSwitching", choice);
}
bool CSettings::getEnableAutomaticIDSwitching(bool defaultV)
{
	bool defaultVal = defaultV;
	bool retrievedVal = false;
	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked("DoIDSwitching")) && getBooleanValue("DoIDSwitching", retrievedVal))
	{
		return retrievedVal;
	}
	else
	{
		retrievedVal = mTools->askYesNo("Enable automatic ID switching?", defaultVal);
		setEnableAutomaticIDSwitching(retrievedVal);
	}
	return retrievedVal;
}


bool CSettings::setMinEventPriorityForConsole(uint64_t  priority)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);


	try {
		return setUInt32Value("MinEventsPriorityForConsole", priority);
	}
	catch (int e)
	{
		return false;

	}
	return false;
}
uint64_t CSettings::getMinEventPriorityForConsole(uint32_t defaultL)
{
	//std::lock_guard<std::recursive_mutex> lock(mWarden); <= not needed.
	//
	std::shared_ptr<CTools> tools = getTools();
	uint32_t defaultVal = defaultL;
	uint32_t retrievedVal = false;
	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked("MinEventsPriorityForConsole")) && getUInt32Value("MinEventsPriorityForConsole", retrievedVal))
	{
		return static_cast<uint64_t>(retrievedVal);
	}
	else
	{
		retrievedVal = static_cast<uint32_t>(tools->askInt("Min. level of notifications for Terminal? (0 for debug-info):", defaultVal));
		setMinEventPriorityForConsole(retrievedVal);
	}
	return static_cast<uint64_t>(retrievedVal);
	//
}
std::shared_ptr<CTools> CSettings::getTools()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mTools;
}
//
bool CSettings::setSetMinNodeERGPrice(const BigInt& price)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	return setBigSIntValue(H_MIN_ERG_BID, static_cast<BigSInt>(price), false);// it's not global but a per sub-net setting.
}

BigInt CSettings::getMinNodeERGPrice(const uint64_t& blockHeight)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	BigSInt defaultVal = CGlobalSecSettings::getDefaultERGpriceInGBU(blockHeight);
	BigSInt retrievedVal = 0;
	std::string setID = H_MIN_ERG_BID;
	if ((getLoadPreviousConfiguration() || wasQuestionAlreadyAsked(setID)) && getBigSIntValue(setID, retrievedVal, false)) // it's not global but a per sub-net setting.
	{
		retrievedVal = retrievedVal;
		return true;

	}
	else
	{
		retrievedVal = static_cast<uint32_t>(mTools->askInt("Provide minimum acceptable ERG price: ", defaultVal, mGlobalSettingsName, true));
		setSetMinNodeERGPrice(static_cast<BigInt>(retrievedVal));
	}

	return static_cast<BigInt>(retrievedVal);


}
/// <summary>
/// By defaut keychain is saved under the SDID generated by its first public key.
/// </summary>
/// <param name="serializedKeyChain"></param>
/// <param name="name"></param>
/// <param name="testNet"></param>
/// <returns></returns>
bool CSettings::saveKeyChain(CKeyChain chain)
{
	return saveKeyChain(chain.getPackedData(), chain.getID());
}



bool CSettings::saveKeyChain(std::vector<uint8_t> serializedKeyChain, std::string name)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	//todo: add support for additional real users
	try {
		return  getStorage()->saveValue(H_USER_CHAIN + name, serializedKeyChain);
		return true;
	}
	catch (int e)
	{
		return false;

	}
	return false;
}

Botan::secure_vector<uint8_t>  CSettings::getSerializedKeyChain(std::string name)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);

	std::vector < uint8_t>  bv = getStorage()->getValue(H_USER_CHAIN + name);
	return  Botan::secure_vector<uint8_t>(bv.begin(), bv.end());


}

/// <summary>
/// Gets current key chain. Incresements chain depth and updates chain state if requested.
/// </summary>
/// <param name="name"></param>
/// <param name="chain"></param>
/// <param name="markAsUsed"></param>
/// <param name="testNet"></param>
/// <returns></returns>
bool  CSettings::getCurrentKeyChain(CKeyChain& chain, bool markAsUsed, bool notifyAboutResult, std::string name)
{
	std::lock_guard<std::recursive_mutex> lock(mWarden);
	if (name.size() == 0)
		name = getMinersKeyChainID();
	Botan::secure_vector<uint8_t> ser = CSettings::getSerializedKeyChain(name);

	if (ser.size() != 0)
	{
		if (notifyAboutResult)
			mTools->writeLine("key-chain '" + name + "' was FOUND!");

	}
	else
	{
		if (notifyAboutResult)
			mTools->writeLine("key-chain '" + name + "' was NOT FOUND!");

		return false;
	}

	CKeyChain chainS = CKeyChain(CCryptoFactory::getInstance());
	if (!chainS.unpack(ser))
		return false;

	if (markAsUsed)
	{
		CKeyChain chainN = CKeyChain(chainS);
		chainN.incIndex();
		saveKeyChain(chainN.getPackedData(), name);
	}
	chain = chainS;

	//fix name - BEGIN
	if (chain.getID().empty())
	{
		chain.setID(name);
	}
	//fix name - END

	return true;
}







