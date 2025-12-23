#include "CStateDomainManager.h"
#include "keyChain.h"
#include "ThreadPool.h"
#include "BlockchainManager.h"
#include "transactionmanager.h"
#include "TokenPool.h"
#include "SearchResults.hpp"
#include <future>
#include "Receipt.h"
#include "DomainDesc.hpp"
#include "StatusBarHub.h"
#include "SearchFilter.hpp"
CStateDomainManager::CStateDomainManager(CTrieDB * globalStateTrie, eBlockchainMode::eBlockchainMode blockchainMode,CTransactionManager *tManager)
{
	mDomainMetaDataAvailable = false;
	mCurrentDB = globalStateTrie;
	mCurrentDB->forceRootNodeStorage();
	mCurrentDB = globalStateTrie;
	mInFlow = false;
	//mKnownStateDomainsCount = 0;
	mTransactionsManager = tManager;//if we want to be able to enter the Flow, needs not be NULL
	mKnownStateDomainIDs = mCurrentDB->getKnownNodeIDs();
	mBlockchainMode = blockchainMode;
	mTools = std::make_shared<CTools>("StateDomain Manager", blockchainMode);
}

/**
   * @brief Retrieves all cached CDomainDesc instances from the manager.
   * @return A vector of CDomainDesc shared pointers.
   */
std::vector<std::shared_ptr<CDomainDesc>> CStateDomainManager::getAllCachedDomainMetadata() const
{
	std::vector<std::shared_ptr<CDomainDesc>> results;
	std::shared_lock<std::shared_mutex> lock(mMetadataCacheMutex);
	results.reserve(mDomainMetadataCache.size());
	for (auto& kv : mDomainMetadataCache)
	{
		if (kv.second)
			results.push_back(kv.second);
	}
	return results;
}

size_t CStateDomainManager::getAllCachedDomainMetadataCount() const
{
	std::shared_lock<std::shared_mutex> lock(mMetadataCacheMutex);
	return mDomainMetadataCache.size();
}

BigInt CStateDomainManager::analyzeTotalDomainBalances()
{
	std::lock_guard<ExclusiveWorkerMutex>lg(mCurrentDB->mGuardian);
	std::vector <std::vector<uint8_t>> domainIDs = getKnownDomainIDs();
	BigInt balance = 0;
	for (int i = 0; i < domainIDs.size(); i++)
	{
		CStateDomain * domain = findByID(domainIDs[i]);
		if (domain != nullptr)
			balance += domain->getBalance();

	}
	return balance;
}
CStateDomainManager::~CStateDomainManager()
{
}

CStateDomainManager::CStateDomainManager(const CStateDomainManager & sibling)
{
	std::lock_guard<std::recursive_mutex> lock(const_cast<CStateDomainManager & >(sibling).mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(const_cast<CStateDomainManager&>(sibling).mInFlowGuardian);
	
	mDomainMetaDataAvailable = sibling.mDomainMetaDataAvailable;
	mTools = sibling.mTools;
	mBlockchainMode = sibling.mBlockchainMode;
	mInFlow = sibling.mInFlow;
	mCurrentDB = sibling.mCurrentDB;
	mKnownStateDomainIDs = sibling.mKnownStateDomainIDs;
}

void CStateDomainManager::setKnownStateDomainIDs(std::vector<std::vector<uint8_t>> IDs)
{
	std::lock_guard<std::mutex> lock(mKnownDomainsGuardian);
	mKnownStateDomainIDs = IDs;
	//mKnownStateDomainsCount = IDs.size();
}

void CStateDomainManager::appendKnownDomainIDs(std::vector<std::vector<uint8_t>> IDs)
{
	if (IDs.size() == 0)
		return;
		std::lock_guard<std::mutex> lock(mKnownDomainsGuardian);
		//the following only for debugging; it should not happen
		/*bool alreadyknown = false;
		for (auto const y : mKnownStateDomainIDs)
		{
			for (auto const i : IDs)
			{
				if (std::memcmp(i.data(), y.data(), 32) == 0)
				{
					alreadyknown = true;
					return;
				}
			}
		}*/
		mKnownStateDomainIDs.insert(mKnownStateDomainIDs.end(), IDs.begin(), IDs.end());
		//mKnownStateDomainsCount += IDs.size();

}

CTrieDB * CStateDomainManager::getDB()
{
	std::lock_guard<std::recursive_mutex>lg(mCurrentDBGuardian);
	
	return mCurrentDB;
}

// Domain Search - BEGIN
std::shared_ptr<CSearchResults> CStateDomainManager::searchDomains(const std::string& query, uint64_t size, uint64_t page, const CSearchFilter& filter) {
	// Local Variables - BEGIN
	std::vector<CSearchResults::ResultData> allMatchingDomains;
	std::vector<CSearchResults::ResultData> paginatedResults;
	uint64_t totalCount = 0;
	uint64_t startIndex = (page - 1) * size;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::vector<std::vector<uint8_t>> domainIDs = getKnownDomainIDs(true);
	// Local Variables - END

	// Operational Logic - BEGIN
	try {
		// Collect all matching domains
		// Exact Search - BEGIN
		std::vector<uint8_t> exactDomainID = tools->stringToBytes(query);
		if (tools->isDomainIDValid(exactDomainID)) {
			CStateDomain* domain = findByID(exactDomainID);
			if (domain) {
				auto domainDesc = createDomainDescription(domain);
				if (matchesDomainFilter(domainDesc, query, filter)) {
					allMatchingDomains.emplace_back(domainDesc);
				}
			}
		}
		// Exact Search - END

		// Fuzzy Search - BEGIN
		for (const auto& id : domainIDs) {
			// Skip the exact match domain ID if already processed
			if (id == exactDomainID) {
				continue;
			}

			CStateDomain* domain = findByID(id);
			if (!domain) {
				// Log warning about missing domain - this should never happen
				tools->logEvent("Domain not found for ID", "CStateDomainManager::searchDomains", eLogEntryCategory::localSystem, 8, eLogEntryType::failure);
				continue;
			}

			auto domainDesc = createDomainDescription(domain);
			if (matchesDomainFilter(domainDesc, query, filter)) {
				allMatchingDomains.emplace_back(domainDesc);
			}
		}
		// Fuzzy Search - END

		// Total count of matching domains
		totalCount = allMatchingDomains.size();

		// Apply pagination
		if (startIndex < totalCount) {
			uint64_t endIndex = min(startIndex + size, totalCount);
			paginatedResults.assign(allMatchingDomains.begin() + startIndex, allMatchingDomains.begin() + endIndex);
		}
		// Operational Logic - END
	}
	catch (const std::exception& e) {
		// Log the error
		tools->logEvent("Error in searchDomains: " + std::string(e.what()), "CStateDomainManager::searchDomains", eLogEntryCategory::localSystem, 5, eLogEntryType::failure, eColor::cyborgBlood);
	}

	// Handle case where there are fewer results than requested
	return std::make_shared<CSearchResults>(paginatedResults, totalCount, page, size);
}

// todo: @CodesInChaos - consider incorporating additional cache for Domains
std::shared_ptr<CDomainDesc> CStateDomainManager::createDomainDescription(CStateDomain* domain, bool includeSecData) {
	
	if (!domain)
	{
		return nullptr;
	}

	std::shared_ptr<CBlockchainManager> bm = CBlockchainManager::getInstance(mBlockchainMode, false, false);

	// Local Variables - BEGIN
	std::shared_ptr<CTools> tools = CTools::getInstance();
	bool justReleased = false;
	auto [totalReceived, totalSent, txCountIn, txCountOut, totalGenesisRewards,totalMinerRewards ] = getDomainTransferTotals(domain->getAddress());
	uint64_t perspectivesCount = getDomainPerspectivesCount(domain->getAddress());
	// Local Variables - BEGIN

	// Perspectives - BEGIN
	std::vector<std::vector<uint8_t>> priorPerspectives = domain->getPreviousPerspectives();


	// First let's add current perspective
	std::vector<std::vector<uint8_t>> perspectives{ mCurrentDB->getPerspective() };


	// now contiue with adding all the prior ones.
	// the aim here is to preserve both current and prior perspectives
	// notice:  'perspectivesCount' accounts for all the prior and curent perspective (+1)

	perspectives.insert(
		perspectives.end(),
		std::make_move_iterator(priorPerspectives.begin()),
		std::make_move_iterator(priorPerspectives.end())
	);

	std::vector<std::string> perspectivesTxt;

	for (uint64_t i = 0; i < perspectives.size(); i++)
	{
		perspectivesTxt.push_back(mTools->base58CheckEncode(perspectives[i]));
	}
	// Perspectives - END


	// Operational Logic - BEGIN
	std::shared_ptr<CDomainDesc> toRet=  std::make_shared<CDomainDesc>(
		tools->bytesToString(domain->getAddress()),
		txCountIn,
		txCountOut,
		domain->getLockedAssets(bm->getCachedHeight(true), justReleased), // get locked assets based on current key-block height
		domain->getBalance(),
		totalReceived, // txTotalReceived not directly available. We compute the value.
		totalSent, // txTotalSent not directly available. We compute the value.
		totalMinerRewards, // total miner rewards (key-blocks)
		totalGenesisRewards, // total genesis rewards from the Genesis Block
		perspectivesCount, // perspectivesCount not directly available.  We compute the value.
		perspectivesTxt, // base58-Check encoded, each.
		tools->base58CheckEncode(domain->getPerspective()),
		domain->getIDToken(),
		nullptr,
		domain->getNonce()
	);

	
	if (includeSecData)
	{
		auto secInfo = bm->getSecurityReportForOperator(toRet->getDomain());
		toRet->setSecurityInfo(secInfo);
	}
	return toRet;
	// Operational Logic - END
}

uint64_t CStateDomainManager::getDomainPerspectivesCount(const std::vector<uint8_t>& domainID) {
	CStateDomain* domain = findByID(domainID);
	if (!domain) {
		return 0;
	}

	std::vector<std::vector<uint8_t>> previousPerspectives = domain->getPreviousPerspectives();

	// Add 1 to include the current perspective
	return previousPerspectives.size() + 1;
}
std::tuple<BigInt, BigInt, uint64_t, uint64_t, BigInt, BigInt> CStateDomainManager::getDomainTransferTotals(const std::vector<uint8_t>& domainID) {
	// Local Variables - BEGIN
	BigInt totalReceived = 0;
	BigInt totalSent = 0;
	BigInt totalGenesisRewards = 0;    // New: track genesis rewards
	BigInt totalMiningRewards = 0;     // New: track mining rewards
	uint64_t txCountOut = 0;
	uint64_t txCountIn = 0;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::shared_ptr<CBlockchainManager> bm = CBlockchainManager::getInstance(mBlockchainMode, false, false);
	if (!bm) {
		return { totalReceived, totalSent, txCountOut, txCountIn, totalGenesisRewards, totalMiningRewards };
	}
	std::shared_ptr<CAccessToken> sysToken = CAccessToken::genSysToken();
	// Local Variables - END

	// Operational Logic - BEGIN
	CStateDomain* domain = findByID(domainID);
	if (!domain) {
		return { totalReceived, totalSent, txCountOut, txCountIn, totalGenesisRewards, totalMiningRewards };
	}

	std::string TXStatsContainerID = CGlobalSecSettings::getTXStatsContainerID();
	eDataType::eDataType dType;
	std::vector<uint8_t> TXInfoBytes = domain->loadValueDB(sysToken, TXStatsContainerID, dType);
	std::vector<std::vector<uint8_t>> TXNibbles;
	tools->VarLengthDecodeVector(TXInfoBytes, TXNibbles);

	const uint64_t FIELDS_COUNT = 4;
	uint64_t elementsCount = TXNibbles.size() / FIELDS_COUNT;
	uint64_t remainder = TXNibbles.size() % FIELDS_COUNT;

	if (remainder != 0) {
		// Data integrity error
		return { totalReceived, totalSent, txCountOut, txCountIn, totalGenesisRewards, totalMiningRewards };
	}

	for (uint64_t i = TXNibbles.size() - 1; i != static_cast<uint64_t>(-1); i -= FIELDS_COUNT) {
		BigSInt valueAtto = tools->BytesToBigSInt(TXNibbles[i]);

		// Get transaction flags
		SE::txStatFlags txsf(TXNibbles[i - 3]);

		// Check receipt validity first
		std::vector<uint8_t> receiptID = TXNibbles[i - 2];
		std::shared_ptr<CReceipt> receipt = bm->findReceipt(receiptID, false, false);
		bool isValidTx = (!receipt || receipt->getResult() == eTransactionValidationResult::valid);

		if (!isValidTx) {
			continue; // Skip invalid transactions
		}

		// Process based on transaction type
		if (txsf.genesisReward) {
			totalGenesisRewards += static_cast<BigInt>(valueAtto);
		}
		else if (txsf.blockReward) {
			totalMiningRewards += static_cast<BigInt>(valueAtto);
		}
		else { // Regular value transfer
			if (valueAtto > 0) {
				totalReceived += static_cast<BigInt>(valueAtto);
				txCountIn++;
			}
			else if (valueAtto < 0) {
				totalSent += static_cast<BigInt>(-valueAtto);
				txCountOut++;
			}
		}
	}
	// Operational Logic - END

	return { totalReceived, totalSent, txCountOut, txCountIn, totalGenesisRewards, totalMiningRewards };
}



bool CStateDomainManager::matchesDomainFilter(const std::shared_ptr<CDomainDesc>& domainDesc, const std::string& query, const CSearchFilter& filter) {
	// Preliminaries - BEGIN
	if (!domainDesc) {
		return false;
	}

	if (!filter.hasStandardFlag(CSearchFilter::StandardFlags::DOMAINS) &&
		!filter.hasStandardFlag(CSearchFilter::StandardFlags::ADDRESSES)) {
		return false;
	}
	// Preliminaries - END

	// Local Variables - BEGIN
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::string trimmedQuery = tools->trim(query);
	// Local Variables - END

	// Operational Logic - BEGIN
	// Helper lambda
	auto checkProperty = [&](const std::string& propertyName, const std::string& value) {
		std::string key = "domain_" + propertyName;
		bool flagPresent = filter.hasArbitraryFlag(key);

		if (flagPresent) {
			std::string requestedValue = tools->trim(filter.getArbitraryFlagValue(key));

			if (requestedValue.empty()) {
				// If flag is present but value is empty, we're just checking
				// for the existence of the property with non-empty content
				return !tools->trim(value).empty();
			}
			else {
				// Compare the trimmed requested value with the property value
				return tools->findStringIC(value, requestedValue);
			}
		}
		return false;
		};

	// Check CDomainDesc properties
	if (checkProperty("domain", domainDesc->getDomain()))
		return true;
	if (checkProperty("txCount", domainDesc->getTxCountTxt()))
		return true;
	if (checkProperty("lockedBalance", domainDesc->getLockedBalanceTxt()))
		return true;
	if (checkProperty("balance", domainDesc->getBalanceTxt()))
		return true;
	if (checkProperty("txTotalReceived", domainDesc->getTxTotalReceivedTxt()))
		return true;
	if (checkProperty("txTotalSent", domainDesc->getTxTotalSentTxt()))
		return true;
	if (checkProperty("perspective", domainDesc->getPerspective()))
		return true;
	if (checkProperty("perspectivesCount", std::to_string(domainDesc->getPerspectivesCount())))
		return true;

	// Check CIdentityToken properties if available
	auto idToken = domainDesc->getIdentityToken();
	if (idToken) {
		if (checkProperty("publicKey", idToken->getPublicKeyTxt()))
			return true; // todo: search on base58check encoded public key
		if (checkProperty("nickname", idToken->getFriendlyID()))
			return true;
		if (checkProperty("stake", tools->attoToGNCStr(idToken->getConsumedCoins())))
			return true;
	}

	// If no custom filters are set, perform a general search
	if (filter.getArbitraryFlagCount() == 0) {
		// Skip empty query searches
		if (trimmedQuery.empty()) {
			return false;
		}

		return tools->findStringIC(domainDesc->getDomain(), trimmedQuery) ||
			tools->findStringIC(domainDesc->getBalanceTxt(), trimmedQuery) ||
			(idToken && (tools->findStringIC(idToken->getPublicKeyTxt(), trimmedQuery) ||
				tools->findStringIC(idToken->getFriendlyID(), trimmedQuery)));
	}
	// Operational Logic - END

	return false;
}

// Domain Search - END

BigInt CStateDomainManager::getTotalSupply(bool includeLockedAssets, bool allowCached, bool useMetaData) {
	// Cache Support (Part 1) - BEGIN
	if (allowCached) {
		uint64_t now = static_cast<uint64_t>(std::time(0));
		uint64_t lastCheck = getLastTotalSupplyCheck();
		if (now - lastCheck < 300) { // 5 minutes = 300 seconds
			std::shared_lock<std::shared_mutex> readLock(mCachedTotalSupplyMutex);
			return mCachedTotalSupply;
		}
	}
	// Cache Support (Part 1) - END

	// Local Variables - BEGIN
	BigInt totalSupply = 0;
	BigInt totalLockedAssets = 0;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::shared_ptr<CBlockchainManager> bm = CBlockchainManager::getInstance(mBlockchainMode, false, false);
	if (!bm) {
		return 0; // Edge case: Blockchain manager unavailable
	}
	uint64_t keyHeight = bm->getKeyHeight();
	std::vector<std::string> specialDomains = { "ICOFund", "TheFund" };
	std::vector<std::vector<uint8_t>> knownDomains = getKnownDomainIDs(true);
	bool justReleased = false;
	// Local Variables - END

	// Operational Logic - BEGIN
	if (useMetaData) {
		// Using metadata from CBlockDesc
		for (const auto& domainID : knownDomains) {
			std::shared_ptr<CDomainDesc> domainDesc = getCachedDomainMetadata(domainID);
			if (!domainDesc) {
				continue; // Edge case: Null domain metadata, skip to next
			}

			std::shared_ptr<CIdentityToken> id = domainDesc->getIdentityToken();
			std::string friendlyID = id ? id->getFriendlyID() : "";

			// Check if this is a special domain to be excluded
			bool isSpecialDomain = false;
			for (const auto& specialDomain : specialDomains) {
				if (tools->iequals(specialDomain, friendlyID)) {
					isSpecialDomain = true;
					break;
				}
			}

			// Skip special domains
			if (isSpecialDomain) {
				continue;
			}

			// Add domain balance to total supply using metadata
			totalSupply += domainDesc->getBalance();

			// Get the locked assets from metadata
			totalLockedAssets += domainDesc->getLockedBalance();
		}
	}
	else {
		// Using live CStateDomain data
		for (const auto& domainID : knownDomains) {
			CStateDomain* domain = findByID(domainID);
			if (!domain) {
				continue; // Edge case: Null domain, skip to next
			}

			std::shared_ptr<CIdentityToken> id = domain->getIDToken();
			std::string friendlyID = id ? id->getFriendlyID() : "";

			// Check if this is a special domain to be excluded
			bool isSpecialDomain = false;
			for (const auto& specialDomain : specialDomains) {
				if (tools->iequals(specialDomain, friendlyID)) {
					isSpecialDomain = true;
					break;
				}
			}

			// Skip special domains
			if (isSpecialDomain) {
				continue;
			}

			// Add domain balance to total supply
			totalSupply += domain->getBalance();

			// Get the locked assets
			BigInt lockedAssets = domain->getLockedAssets(keyHeight, justReleased);
			totalLockedAssets += lockedAssets;
		}
	}

	// Locked Assets Support - BEGIN
	// If includeLockedAssets is false, subtract locked assets from totalSupply
	if (!includeLockedAssets) {
		// Ensure no underflow occurs: if locked assets are greater than the total supply, set to 0
		if (totalSupply >= totalLockedAssets) {
			totalSupply -= totalLockedAssets;
		}
		else {
			totalSupply = 0; // Edge case: Locked assets exceed total supply
		}
	}
	// Locked Assets Support - END

	// Cache Support (Part 2) - BEGIN
	// Update cache
	{
		std::unique_lock<std::shared_mutex> writeLock(mCachedTotalSupplyMutex);
		mCachedTotalSupply = totalSupply;
	}
	pingLastTotalSupplyCheck();
	// Cache Support (Part 2) - END

	// Operational Logic - END
	return totalSupply;
}
void CStateDomainManager::pingLastTotalSupplyCheck() const {
	std::unique_lock lock(mSharedGuardian);
	mLastTotalSupplyCheck = static_cast<uint64_t>(std::time(0));
}

void CStateDomainManager::clearLastTotalSupplyCheck() const
{
	std::unique_lock lock(mSharedGuardian);
	mLastTotalSupplyCheck = 0;
}

uint64_t CStateDomainManager::getLastTotalSupplyCheck() const {
	std::shared_lock lock(mSharedGuardian);
	return mLastTotalSupplyCheck;
}

bool CStateDomainManager::isInFlow()
{
	std::lock_guard<std::recursive_mutex> lock(mInFlowGuardian);
	return mInFlow;
}
/**
 * @brief Initiates an update of the domain metadata cache.
 *
 * This method processes all known domain IDs, generating metadata descriptions
 * and storing them in the cache. It supports multi-threaded processing using
 * either a provided thread pool or creating its own.
 *
 * @param forceUpdate If true, forces update even if one is already in progress
 * @param numThreads Number of threads to use if no thread pool provided
 * @param threadPool Optional external thread pool to use
 * @return true if update was initiated successfully, false otherwise
 *
 * @note If no thread pool is provided, creates an internal one with numThreads
 * @note Progress can be monitored via getUpdateProgress()
 */
bool CStateDomainManager::doDomainMetadataUpdate(
	bool forceUpdate,
	size_t numThreads,
	std::shared_ptr<ThreadPool> threadPool)
{
	//numThreads = 1;
	setIsDomainMetaDataAvailable(false);

	// Local Variables - BEGIN
	std::shared_ptr<CTools> tools = mTools;
	bool ownThreadPool = false;
	std::mutex exceptionMutex;
	std::vector<std::exception_ptr> exceptions;
	std::atomic<bool> abortFlag(false);
	std::atomic<uint64_t> lastReportedProgress{ 0 }; // it's shared between threads

	// Shared task queue and synchronization primitives
	std::queue<std::pair<std::vector<std::vector<uint8_t>>, size_t>> taskQueue;
	std::mutex queueMutex;
	std::condition_variable queueCV;
	std::atomic<size_t> activeTasks(0);
	// Local Variables - END

	// Pre-flight Checks - BEGIN
	if (!tools) {
		return false;
	}

	try {
		// First check with non-atomic read - optimization for common case
		if (!forceUpdate && mMetadataUpdateInProgress) {
			tools->logEvent("Domain metadata update already in progress",
				"CStateDomainManager",
				eLogEntryCategory::localSystem,
				3,
				eLogEntryType::warning);
			return false;
		}

		// Atomic check and set with proper handling of forceUpdate
		bool expected = false;
		if (!mMetadataUpdateInProgress.compare_exchange_strong(expected, true)) {
			// At this point: expected = true (the actual value found)
			// and the exchange failed (was already true)

			// Honor forceUpdate parameter to maintain original behavior
			if (!forceUpdate) {
				tools->logEvent("Domain metadata update already in progress (atomic check)",
					"CStateDomainManager",
					eLogEntryCategory::localSystem,
					3,
					eLogEntryType::warning);
				return false;
			}
			else {
				// With forceUpdate=true and update already in progress, we log but continue
				tools->logEvent("Forcing additional domain metadata update despite one in progress",
					"CStateDomainManager",
					eLogEntryCategory::localSystem,
					5,
					eLogEntryType::warning);
				// We continue execution with mMetadataUpdateInProgress=true
			}
		}
		// At this point either:
		// 1. We successfully set mMetadataUpdateInProgress = true (was false)
		// 2. OR forceUpdate = true and we're proceeding anyway
	}
	catch (...) {
		tools->logEvent("Exception in pre-flight checks",
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			5,
			eLogEntryType::failure);
		return false;
	}
	// Pre-flight Checks - END

	// Cleanup Guard - ensures flag is always reset, even on exceptions
	auto cleanupGuard = std::unique_ptr<void, std::function<void(void*)>>(
		nullptr, [this](void*) {
			mMetadataUpdateInProgress = false;
		});

	// Initialize Progress Tracking
	mMetadataUpdateProgress = 0;
	mDomainsMetadataProcessed = 0;
	mLastUpdateTimestamp = std::time(nullptr);

	// Domain IDs Collection - BEGIN
	std::vector<std::vector<uint8_t>> domainIDs;
	try {
		domainIDs = getKnownDomainIDs(true);
		mTotalDomainsToUpdate = domainIDs.size();
	}
	catch (const std::exception& e) {
		tools->logEvent("Failed to get domain IDs: " + std::string(e.what()),
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			5,
			eLogEntryType::failure);
		return false;
	}

	if (domainIDs.empty()) {
		tools->logEvent("No domains to process",
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			3,
			eLogEntryType::notification);
		return true;
	}
	// Domain IDs Collection - END

	// Thread Pool Initialization - BEGIN
	try {
		if (!threadPool) {
			threadPool = std::make_shared<ThreadPool>(max(size_t(1), numThreads));
			ownThreadPool = true;
		}
	}
	catch (const std::exception& e) {
		tools->logEvent("Thread pool initialization failed: " + std::string(e.what()),
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			5,
			eLogEntryType::failure);
		return false;
	}
	// Thread Pool Initialization - END

	// Chunk Creation - BEGIN
	std::vector<std::vector<std::vector<uint8_t>>> chunks;
	const uint64_t CHUNK_SIZE = max(
		static_cast<uint64_t>(1),
		static_cast<uint64_t>(domainIDs.size() / (numThreads * 4))
	);

	try {
		for (size_t i = 0; i < domainIDs.size(); i += CHUNK_SIZE) {
			auto end = min(i + CHUNK_SIZE, domainIDs.size());
			chunks.push_back(std::vector<std::vector<uint8_t>>(
				domainIDs.begin() + i,
				domainIDs.begin() + end
			));
		}
	}
	catch (const std::exception& e) {
		tools->logEvent("Chunk creation failed: " + std::string(e.what()),
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			5,
			eLogEntryType::failure);
		return false;
	}
	// Chunk Creation - END

	// Initialize task queue with chunks
	for (size_t i = 0; i < chunks.size(); ++i) {
		std::lock_guard<std::mutex> lock(queueMutex);
		taskQueue.emplace(chunks[i], i);
		activeTasks.fetch_add(1, std::memory_order_relaxed);
	}

	// Worker function definition
	auto worker = [&]() {
		while (true) {
			// Get task from queue
			std::pair<std::vector<std::vector<uint8_t>>, size_t> currentTask;
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				if (abortFlag.load(std::memory_order_relaxed) ||
					(taskQueue.empty() && activeTasks.load(std::memory_order_relaxed) == 0)) {
					break;
				}

				if (taskQueue.empty()) {
					queueCV.wait(lock, [&]() {
						return !taskQueue.empty() ||
							abortFlag.load(std::memory_order_relaxed) ||
							activeTasks.load(std::memory_order_relaxed) == 0;
						});
					if (abortFlag.load(std::memory_order_relaxed) ||
						(taskQueue.empty() && activeTasks.load(std::memory_order_relaxed) == 0)) {
						break;
					}
				}

				if (!taskQueue.empty()) {
					currentTask = std::move(taskQueue.front());
					taskQueue.pop();
				}
			}

			// Process chunk
			try {
				robin_hood::unordered_map<std::vector<uint8_t>,
					std::shared_ptr<CDomainDesc>> chunkCache;

				for (const auto& domainID : currentTask.first) {
					if (abortFlag.load(std::memory_order_relaxed)) {
						break;
					}

					// Domain lookup and metadata creation
					CStateDomain* domain = findByID(domainID, false, false, false);
					if (!domain) {
						continue;
					}

					auto domainDesc = createDomainDescription(domain, true);
					if (!domainDesc) {
						continue;
					}

					chunkCache[domainID] = domainDesc;

					// Update progress atomically
					uint64_t newProcessed = mDomainsMetadataProcessed.fetch_add(1) + 1;
					uint64_t newProgress = static_cast<uint64_t>(((double)newProcessed / (double)mTotalDomainsToUpdate) * 100);

					// Report progress changes only when crossing percentage boundaries
					uint64_t currentLastReportedProgress = lastReportedProgress.load(std::memory_order_relaxed);
					if (newProgress > currentLastReportedProgress) {
						uint64_t expected = currentLastReportedProgress;
						if (lastReportedProgress.compare_exchange_strong(expected, newProgress)) {
							domainMetaDataUpdateProgressChanged(newProgress,
								mTotalDomainsToUpdate);
						}
					}
				}

				// Update the cache with processed domains
				if (!chunkCache.empty()) {
					std::unique_lock<std::shared_mutex> lock(mMetadataCacheMutex);
					for (const auto& [id, desc] : chunkCache) {
						if (desc) {
							mDomainMetadataCache[id] = desc;
						}
					}
				}
			}
			catch (...) {
				std::lock_guard<std::mutex> lock(exceptionMutex);
				exceptions.push_back(std::current_exception());
				abortFlag.store(true, std::memory_order_relaxed);
				queueCV.notify_all();
				break;
			}

			// Mark task as completed
			activeTasks.fetch_sub(1, std::memory_order_relaxed);
			if (activeTasks.load(std::memory_order_relaxed) == 0) {
				queueCV.notify_all();
			}
		}
		};

	// Launch workers
	std::vector<std::unique_ptr<std::thread>> threads;
	try {
		for (size_t i = 0; i < numThreads; ++i) {
			threads.push_back(std::make_unique<std::thread>(worker));
		}

		// Wait for all threads to complete
		for (auto& thread : threads) {
			if (thread && thread->joinable()) {
				thread->join();
			}
		}
	}
	catch (const std::exception& e) {
		abortFlag.store(true, std::memory_order_relaxed);
		queueCV.notify_all();

		// Clean up threads
		for (auto& thread : threads) {
			if (thread && thread->joinable()) {
				thread->join();
			}
		}

		tools->logEvent("Thread management failed: " + std::string(e.what()),
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			5,
			eLogEntryType::failure);
		return false;
	}

	// If we have any exceptions, return false
	if (!exceptions.empty()) {
		tools->logEvent("Domain metadata update failed due to exceptions",
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			10,
			eLogEntryType::failure);
		return false;
	}

	// Check Results - BEGIN
	uint64_t updatedCount = mTotalDomainsToUpdate.load();
	uint64_t toBeUpdatedCount = mTotalDomainsToUpdate.load();

	if (updatedCount == toBeUpdatedCount)
	{
		setIsDomainMetaDataAvailable(true);
		tools->logEvent("Domain metadata update completed successfully",
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			10,
			eLogEntryType::notification);
	}
	else if (updatedCount == 0)
	{
		tools->logEvent("Empty Domain Meta-data Cache",
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			10,
			eLogEntryType::warning);
	}
	else if (updatedCount < toBeUpdatedCount)
	{
		tools->logEvent("Full Domain meta-data refresh failed",
			"CStateDomainManager",
			eLogEntryCategory::localSystem,
			10,
			eLogEntryType::warning);
	}
	// Check Results - END

	return !abortFlag.load(std::memory_order_relaxed);
}
void CStateDomainManager::domainMetaDataUpdateProgressChanged(uint64_t progress, uint64_t total) {
		std::shared_ptr<CStatusBarHub> barHub = CStatusBarHub::getInstance();
		mMetadataUpdateProgress = progress;
		barHub->setCustomStatusBarText(mBlockchainMode,
			991,
			"Domain metadata update progress: " + std::to_string(progress) + "%");
}

bool CStateDomainManager::isMetadataUpdateInProgress() const {

	return mMetadataUpdateInProgress;
}

uint64_t CStateDomainManager::getLastUpdateTimestamp() const {

	return mLastUpdateTimestamp;
}

uint64_t CStateDomainManager::getUpdateProgress() const {
	return mMetadataUpdateProgress;
}

std::shared_ptr<CDomainDesc> CStateDomainManager::getCachedDomainMetadata(
	const std::vector<uint8_t>& domainID) const
{
	std::shared_lock<std::shared_mutex> lock(mMetadataCacheMutex);
	auto it = mDomainMetadataCache.find(domainID);
	return (it != mDomainMetadataCache.end()) ? it->second : nullptr;
}
std::vector<std::vector<uint8_t>> CStateDomainManager::getKnownDomainIDs(bool includeThoseFromCurrentFlow)
{
	std::lock_guard<std::recursive_mutex>lg(mCurrentDBGuardian);
	std::lock_guard<std::mutex> lock(mKnownDomainsGuardian);
	std::vector < std::vector<uint8_t>>toRet;

	if (mKnownStateDomainIDs.size() == 0)
	{
		mKnownStateDomainIDs = mCurrentDB->getKnownNodeIDs();
	}
	toRet = mKnownStateDomainIDs;
	if (includeThoseFromCurrentFlow)
		toRet.insert(toRet.end(), mDomainsCreatedDuringFlow.begin(), mDomainsCreatedDuringFlow.end());
	return  toRet;
}


/// <summary>
/// This function is to be called whenever external objects retrieve sub-objects of this CTrieDB and are to continue using these.
/// Once the external object is done, it MUST call notifyDataReleased().
/// All setPerspective() calls would halt until all external objects have released resources.
/// </summary>
/// <returns></returns>
uint64_t CStateDomainManager::notifyDataInUse()
{
	std::lock_guard<std::recursive_mutex>lg(mCurrentDBGuardian);

	if (!mCurrentDB)
		return 0;

	return mCurrentDB->DataGuardian.acquire();

}

/// <summary>
/// This method MUST be called after an external object called notifyDataInUse() AND after it is done using the retrieved objects.
/// IMPORTANT: after this method has been called, the a-priori retrieved objects may be invalidated at any moment.
/// </summary>
/// <returns></returns>
uint64_t CStateDomainManager::notifyDataReleased()
{
	std::lock_guard<std::recursive_mutex>lg(mCurrentDBGuardian);

	if (!mCurrentDB)
		return 0;

	return mCurrentDB->DataGuardian.release();

}
//Notice: other objects may use std::lock_guard directly on the instance of a CStateDomainManager.
//Thus, there is no need to be using notifyDataInUse() and notifyDataReleased() directly.
//That might be especially convenient when complex logic in the external object is involved (multiple return paths after an explicit lock would have been acquired).
//Notice: calling std::lock_guard upon an instance of a CStateDomain manager does not prevent multiple threads from accessing data within provided by this class.
//That is because unitary operations are protected by a dedicated binary mutex.
//std::lock_guard_wrapper - BEGIN
void CStateDomainManager::lock() {

	notifyDataInUse();
}

void  CStateDomainManager::unlock() {
	notifyDataReleased();
}
//std::lock_guard_wrapper - END
CStateDomain* CStateDomainManager::findByID(std::vector<uint8_t> id, bool verifyDomainID, bool syncDomains, bool supportFlow)
{
	// First mutex lock for database access is still needed regardless of flow support
	std::lock_guard<ExclusiveWorkerMutex> lg(mCurrentDB->mGuardian);

	// Local Variables
	std::vector<uint8_t> friendlyID;

	if (id.size() < 34)
	{
		id.resize(34, 0);
	}

	bool hr = 1;
	if (verifyDomainID && !mTools->isDomainIDValid(id))
		return nullptr;

	CStateDomain* toRet = static_cast<CStateDomain*>(mCurrentDB->findNodeByFullID(
		mTools->bytesToNibbles(id, hr),
		!syncDomains,
		!syncDomains
	));

	if (toRet == nullptr)
		return toRet;

	if (!mTools->isDomainIDValid(toRet->getAddress()))
	{
		return nullptr;
	}

	if (!std::memcmp(id.data(), toRet->getAddress().data(), 32) == 0)
	{
		std::vector<nibblePair> wantedAdr = mTools->bytesToNibbles(id);
		std::vector<nibblePair> presentAdr = mTools->bytesToNibbles(toRet->getAddress());
		return nullptr;
	}

	// Only lock the flow mutex and attempt flow operations if supportFlow is true
	if (supportFlow)
	{
		std::lock_guard<std::recursive_mutex> lock(mInFlowGuardian);
		if (mInFlow && !toRet->isInFlow())
		{
			assertGN(toRet->enterFlow(shared_from_this()));
		}
		else
		{
			toRet->setStateDomainManager(shared_from_this());
		}
	}
	else
	{
		// When supportFlow is false, just set the state domain manager
		// This is still needed for access-right management
		toRet->setStateDomainManager(shared_from_this());
	}

	return toRet;
}

CStateDomain * CStateDomainManager::findByPubKey(std::vector<uint8_t> pubKey)
{
	std::lock_guard<std::recursive_mutex> lg(mGuardian);
	if (pubKey.size() != 32)
		return nullptr;
	return nullptr;
}

CStateDomain * CStateDomainManager::findByIDToken(CIdentityToken id)
{
	std::lock_guard<std::recursive_mutex>lg(mGuardian);
	return nullptr;
}



std::vector<std::vector<uint8_t>> CStateDomainManager::getDomainsCreatedDuringFlow()
{
	return mDomainsCreatedDuringFlow;
}

bool CStateDomainManager::create( std::vector<uint8_t>& perspective, CStateDomain** createdDomain, bool inSandbox,const  std::vector<uint8_t>& id)
{
	if (const_cast<std::vector<uint8_t>&>(id).size() < 34)
	{
		//expand
		const_cast<std::vector<uint8_t>&>(id).resize(34, 0);
	}

	//std::lock_guard<std::recursive_mutex>lg(mGuardian);
	std::lock_guard<std::recursive_mutex>lg(mCurrentDBGuardian);
	std::lock_guard<ExclusiveWorkerMutex>lg2(mCurrentDB->mGuardian);
	std::shared_ptr<CSettings> set;
	CKeyChain chain = CKeyChain();
	bool wasIDEmpty = false;
	if (id.size() == 0)
	{
		wasIDEmpty = true;
		set = CBlockchainManager::getInstance(mBlockchainMode)->getSettings();
		if (!set->saveKeyChain(chain))
			return false;

		if (!CCryptoFactory::getInstance()->genAddress(chain.getPubKey(), const_cast<std::vector<uint8_t>&>(id)))
			return false;
	}
	
	CStateDomain domain = CStateDomain(id, mCurrentDB->getPerspective(),std::vector<uint8_t>(),mBlockchainMode);
	//domain.setPermToken()
	std::lock_guard lock(mInFlowGuardian);//so that we do not exit flow while domain enters

	if (mInFlow) {
		assertGN(domain.enterFlow(shared_from_this()));
	}


	domain.prepare(false);
	std::vector<uint8_t>hashA = mCurrentDB->getPerspective();
	CTrieDB::eResult  result = mCurrentDB->addNodeExt(mTools->bytesToNibbles(domain.getAddress()), &domain, inSandbox);
	hashA = mCurrentDB->getPerspective();
	if (result == CTrieDB::eResult::added)
	{
		perspective = mCurrentDB->getPerspective();
		CStateDomain* dom = static_cast<CStateDomain*>(mCurrentDB->findNodeByFullID(domain.getAddress()));
	 assertGN(std::memcmp(dom->getAddress().data(), id.data(), 32) == 0);
		//assert(dom->setPerspective(perspective, false));
		*createdDomain = dom;
		if(mInFlow)
		mDomainsCreatedDuringFlow.push_back(domain.getAddress());
		return true;
	}
	else return false;
}

bool CStateDomainManager::setPerspective(std::vector<uint8_t> perspective)
{
	std::lock_guard<ExclusiveWorkerMutex>lg(mCurrentDB->mGuardian);
	return mCurrentDB->setPerspective(perspective);
}

void CStateDomainManager::cleanUpFlowData()
{
	mDomainsCreatedDuringFlow.clear();
}


bool CStateDomainManager::update(CStateDomain domain, std::vector<uint8_t> &perspective, bool inSandBox)
{
	std::lock_guard<std::recursive_mutex>lg(mCurrentDBGuardian);
	std::lock_guard<ExclusiveWorkerMutex>lg2(mCurrentDB->mGuardian);
	domain.updateTimeStamp();
	domain.prepare(false);
	CTrieDB::eResult result = CTrieDB::eResult::failure;
	if (!mCurrentDB->findNodeByFullID(domain.getAddress(), true, true))
	 result =mCurrentDB->addNodeExt(mTools->bytesToNibbles(domain.getAddress()),&domain, inSandBox,true);
	else
		result =  mCurrentDB->addNode(mTools->bytesToNibbles(domain.getAddress()), &domain, inSandBox, true);

	if (result == CTrieDB::eResult::added || result == CTrieDB::eResult::updated)
	{
		perspective = mCurrentDB->getPerspective();
		return true;
	}
	return false;
}

bool CStateDomainManager::enterFlow()
{
	std::lock_guard<std::recursive_mutex>lg(mGuardian);
	std::lock_guard<std::recursive_mutex>lg2(mCurrentDBGuardian);
	std::lock_guard<std::recursive_mutex>lg3(mInFlowGuardian);
	if (mInFlow)
		return false;
	if (mTransactionsManager == NULL)
		return false;
	
	mDomainsCreatedDuringFlow.clear(); //the vectors are cleared at the beginning not at the end to allow for the data to be queried aftrer the flow ends
	mCurrentDB = mTransactionsManager->getFlowDB();
	mCurrentDB->clearStateDomainsModifiedDuringTheFlow();
	for (int i = 0; i < mInFlowDomainInstances.size(); i++)
	{
		if (!mInFlowDomainInstances[i]->enterFlow(shared_from_this()))
			return false;
	}
	mInFlow = true;
	return true;
}

CStateDomain*  CStateDomainManager::findByFriendlyID(std::string friendlyID)
{
	std::lock_guard<ExclusiveWorkerMutex>lg(mCurrentDB->mGuardian);
	std::string path = CGlobalSecSettings::getVMSysDirPath(eSysDir::identityTokens);
	std::vector<uint8_t> toRet, retrievedDataVec;
	eDataType::eDataType vt;
	CStateDomain* system = findByID(mTools->stringToBytes(CGlobalSecSettings::getVMSystemDirName()));
	if(system==nullptr)
	return nullptr;

	retrievedDataVec = system->loadValueDB<std::vector<uint8_t>>(CAccessToken::genSysToken(), mTools->stringToBytes(friendlyID), vt, path);

	if (vt == eDataType::noData)
		return nullptr;

	if (mTools->isDomainIDValid(retrievedDataVec))
	{
		return findByID(retrievedDataVec);
	}

	else return nullptr;

}

std::shared_ptr<CTokenPool> CStateDomainManager::findTokenPoolByID(std::vector<uint8_t> id, CStateDomain** owner)
{
	std::lock_guard<ExclusiveWorkerMutex>lg(mCurrentDB->mGuardian);

	//Local Variables - BEGIN
	bool isAbsolutePath = false;
	std::vector<std::string> directories;
	std::string dirPart;
	std::string filePart,SDPart;
	CTrieNode* destination = nullptr;
	std::string tokenPoolsPath = CGlobalSecSettings::getVMSysDirPath(eSysDir::tokenPools);
	std::shared_ptr<CTokenPool> toRet;
	std::vector<uint8_t>  link;
	eDataType::eDataType vt;
	CStateDomain* system = nullptr;

	//Local Variables - END
	
	//Operational Logic - BEGIN

	//look-up system-domain
	system = findByID(mTools->stringToBytes(CGlobalSecSettings::getVMSystemDirName()));
	if (system == nullptr)
		return nullptr;

	//load poolId-> absolute (including state-domain) file path LINK 
	//i.e. check if there's a Token Pool with a given full-id(32 bytes) registered within the System's directory.
	link = system->loadValueDB<std::vector<uint8_t>>(CAccessToken::genSysToken(), id, vt, tokenPoolsPath);

	if (!mTools->parsePath(isAbsolutePath, mTools->bytesToString(link), filePart, directories, dirPart, SDPart) || !isAbsolutePath || SDPart.size()==0)
	{
		return nullptr;
	}

	if (vt == eDataType::noData)
		return toRet;

	std::vector<uint8_t> sdPartB = mTools->stringToBytes(SDPart);
	//load state-domain
	if (mTools->isDomainIDValid(sdPartB))
	{
		*owner = findByID(sdPartB);
	}

	if (*owner == nullptr)
	*owner = findByFriendlyID(SDPart);


	if (*owner == nullptr)
		return toRet;

	//look-up Token-Pool within the Domain
	
	if (*owner!=nullptr)
	{
		return (*owner)->getTokenPoolP1(mTools->stringToBytes(filePart));
	}
	return nullptr;
	//Operational Logic - END
}


bool CStateDomainManager::exitFlow(bool updateKnownStateDomainIDs)
{
	std::lock_guard<std::recursive_mutex>lg(mGuardian);
	std::lock_guard<ExclusiveWorkerMutex>lg2(mCurrentDB->mGuardian);
	std::lock_guard<std::recursive_mutex>lg3(mInFlowGuardian);
	if (!mInFlow)
		return false;
	mTools->writeLine("Exiting the Flow..");

 assertGN(mTransactionsManager != NULL);
	/*for (int i = 0; i < mInFlowDomainInstances.size(); i++)
			{
		if (!mInFlowDomainInstances[i]->exitFlow())
			return false;
			}
			*/
	//Comment: Do NOT exit-flow on the mInFlowDomainInstances; the objects have been pruned from the Flow-State-Trie already.
	mInFlowDomainInstances.clear();
	if (updateKnownStateDomainIDs)
	{
		bool alreadyKnown = false;
		/*bool alreadyKnown = false;
		for (uint64_t i = 0; i < mDomainsCreatedDuringFlow.size(); i++)
		{
			for (uint64_t x = 0; x < mKnownStateDomainIDs.size(); x++)
			{
				if (mTools->compareByteVectors(mDomainsCreatedDuringFlow[i], mKnownStateDomainIDs[x]))
				{
					alreadyKnown = true;
				}
			}
			if(!alreadyKnown)
			mKnownStateDomainIDs.push_back(mDomainsCreatedDuringFlow[x]);
		}*/ //if that's really needed then use std::map and its binary search capabilities

		mKnownDomainsGuardian.lock();
		mKnownStateDomainIDs.insert(mKnownStateDomainIDs.end(), mDomainsCreatedDuringFlow.begin(),
			mDomainsCreatedDuringFlow.end());
		//mKnownStateDomainsCount += mKnownStateDomainIDs.size();
		
		mKnownDomainsGuardian.unlock();
	}
	mCurrentDB = mTransactionsManager->getLiveDB();
	mInFlow = false;
	return true;
}
/// <summary>
/// Deletes a given (deprecated) body from Solid Storage ().
/// </summary>
/// <param name="bodyHash"></param>
/// <returns></returns>
bool CStateDomainManager::deleteDomainBody(std::vector<uint8_t> bodyHash)
{
	//note that at this stage, all the nodes from
/// within Domain's Data-Storage Trie may remain orphaned - if bodies of the same content are not stored within other state domains
/// note that the domain's hash is based solely on its content. We cannot simply remove Trie's member from sold storage as it might happen there's node with same content
/// used somewhere else. i.e. container with the same integer value. that might be quite probable.
/// ways to go: 1) include another factor into the node's hash  ex. parant node's hash - that would make the ID State-domains specific and assure unique copies. 
	std::lock_guard<std::recursive_mutex>lg(mGuardian);

	if (bodyHash.size()!=32)
		return false;
	return CBlockchainManager::getInstance(mBlockchainMode)->getSolidStorage()->deleteNode(bodyHash, mCurrentDB->getSSPrefix());

}

bool CStateDomainManager::unregisterDomain(std::vector<uint8_t> ID)
{
	std::lock_guard<std::recursive_mutex>lg(mGuardian);
	bool didIt = false;
	if (ID.size() < 32)
		return false;
	
	for (int i = 0; i < mInFlowDomainInstances.size(); i++)
	{
		uint32_t size = min(ID.size(), mInFlowDomainInstances[i]->getAddress().size());
			if (size <= 0)
				continue;
			if (std::memcmp(mInFlowDomainInstances[i]->getAddress().data(), ID.data(), size)==0)
			{

				mInFlowDomainInstances.erase(mInFlowDomainInstances.begin() + i);
				if (i > 0)
				{
					i--;
					
				}
				didIt = true;
		}

	}
	return didIt;
}


bool CStateDomainManager::registerDomain(CStateDomain * domain)
{
	std::lock_guard<std::recursive_mutex>lg(mGuardian);
	mInFlowDomainInstances.push_back(domain);
	return true;
}

size_t CStateDomainManager::getKnownDomainsCount(bool includeThoseCreatedDuringTheFlow)
{
	std::lock_guard<std::mutex>lg(mKnownDomainsGuardian);
	size_t toRet = 0;
	toRet = mKnownStateDomainIDs.size();
	if (includeThoseCreatedDuringTheFlow)
		toRet += mDomainsCreatedDuringFlow.size();
	return toRet;
}

bool CStateDomainManager::getIsDataInHotStorage()
{
	std::lock_guard<std::recursive_mutex>lg2(mCurrentDBGuardian);
	if (mCurrentDB == nullptr)
	{
		return false;
	}

	return  mCurrentDB->getIsTrieFullyInRAM();
}

bool CStateDomainManager::getIsDomainMetaDataAvailable()
{
	std::shared_lock lock(mSharedGuardian);
	return mDomainMetaDataAvailable;
}

void CStateDomainManager::setIsDomainMetaDataAvailable(bool isIt)
{
	std::unique_lock lock(mSharedGuardian);
	if (mDomainMetaDataAvailable != isIt)
	{
		if (isIt)
		{
			mTools->logEvent("Domain Meta Data is now available",
				"Cache",
				eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::lightGreen);
		}
		else
		{
			mTools->logEvent("Domain Meta Data is now unavailable",
				"Cache",
				eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::lightPink);
		}
		mDomainMetaDataAvailable = isIt;
	}
}


