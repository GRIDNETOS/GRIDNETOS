#include "SolidStorage.h"
#include "miniz.h"
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>


#include <rocksdb/options.h>      // For Options, BlockBasedTableOptions
#include <rocksdb/table.h>        // For table related options
#include <rocksdb/cache.h>        // For block cache operations
#include <rocksdb/utilities/options_util.h>  // For options utilities
#include <rocksdb/convenience.h>  // For convenience functions

#include "TrieNode.h"
#include "TrieBranchNode.h"
#include "Block.h"
#include "CGlobalSecSettings.h"
#include "BlockchainManager.h"
#include <locale>
#include <codecvt>
#include <string>
#include <fstream>
#include "LinkContainer.h"
#include <iostream>
#include <cstdint>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>
#include <Windows.h>
#include "WorkManager.h"
#include "StatusBarHub.h"


bool CSolidStorage::staticVirginityDetector = true;
bool CSolidStorage::mWasSQLDatabaseInitialized = false;
bool CSolidStorage::mWereRocksDBsDatabasesInitialized = false;
std::string CSolidStorage::mMainDataDir;
std::string CSolidStorage::mKernelCacheDir;

//LIVE-NET
 rocksdb::DB* CSolidStorage::sLIVEBlockchainDB = nullptr; //contains Blockchain blocks
 rocksdb::DB* CSolidStorage::sLIVEStateTrieDB = nullptr; //contains a Global State Trie
 rocksdb::DB* CSolidStorage::sLIVEStagedStateTrieDB = nullptr;//pruned, Staged version of the Global State-Trie
//Test-Net
 rocksdb::DB* CSolidStorage::sTestNetBlockchainDB = nullptr;
 rocksdb::DB* CSolidStorage::sTestNetStateTrieDB = nullptr;
 rocksdb::DB* CSolidStorage::sTestNetStagedStateTrieDB = nullptr;

//LOCAL-DATA-TESTS
 rocksdb::DB* CSolidStorage::sLocalTestBlockchainDB = nullptr;
 rocksdb::DB* CSolidStorage::sLocalTestStateTrieDB = nullptr;
 rocksdb::DB* CSolidStorage::sLocalTestStagedStateTrieDB = nullptr;
 namespace fs = std::filesystem;
/// <summary>
/// Retrieves BER-encoded node from cold-storage
/// </summary>
/// <param name="hash"></param>
/// <param name="prefix"></param>
/// <returns></returns>
std::vector<uint8_t> CSolidStorage::loadNode(std::vector<uint8_t> hash,std::string prefix)
{
	std::shared_lock lock(mDataAccessGuardian);

	if (!getIsSystemAvailable())
		return std::vector<uint8_t>();
	std::string key = std::string(hash.begin(), hash.end());
	std::string value;
	key = H_NODE_HASH +prefix+ key;
	
	rocksdb::ReadOptions options = getOptimizedReadOptions();
	//options.prefetch_distance = 32 * 1024; // 32KB prefetch
	options.readahead_size = 0;// 64 * 1024;    // 64KB readahead
	mStateTrieDB->Get(options, key, &value);

	std::vector<uint8_t> a = std::vector<uint8_t>(value.begin(), value.end());
	return a;
}
double CSolidStorage::getTotalRAMUsageGB(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType) {
	rocksdb::DB** dbRef = getDatabaseReference(mode, dbType);
	if (dbRef == nullptr || *dbRef == nullptr) {
		return 0.0;
	}

	double totalRAMBytes = 0.0;
	std::string value;

	if ((*dbRef)->GetProperty("rocksdb.estimate-table-readers-mem", &value)) {
		totalRAMBytes += std::stoull(value);
	}
	if ((*dbRef)->GetProperty("rocksdb.cur-size-all-mem-tables", &value)) {
		totalRAMBytes += std::stoull(value);
	}
	if ((*dbRef)->GetProperty("rocksdb.block-cache-usage", &value)) {
		totalRAMBytes += std::stoull(value);
	}
	// Add these critical metrics:
	if ((*dbRef)->GetProperty("rocksdb.size-all-mem-tables", &value)) {
		totalRAMBytes += std::stoull(value);
	}
	if ((*dbRef)->GetProperty("rocksdb.block-cache-pinned-usage", &value)) {
		totalRAMBytes += std::stoull(value);
	}

	// Convert to GB (1 GB = 1024^3 bytes)
	return totalRAMBytes / (1024.0 * 1024.0 * 1024.0);
}

double CSolidStorage::getTotalDiskUsageGB(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType) {
	rocksdb::DB** dbRef = getDatabaseReference(mode, dbType);
	if (dbRef == nullptr || *dbRef == nullptr) {
		return 0.0;
	}

	double totalDiskBytes = 0.0;
	std::string value;

	// Get total SST files size
	if ((*dbRef)->GetProperty("rocksdb.total-sst-files-size", &value)) {
		totalDiskBytes += std::stoull(value);
	}

	// Get live SST files size
	if ((*dbRef)->GetProperty("rocksdb.live-sst-files-size", &value)) {
		totalDiskBytes += std::stoull(value);
	}

	// Get size of all memtables that will be flushed to disk
	if ((*dbRef)->GetProperty("rocksdb.size-all-mem-tables", &value)) {
		totalDiskBytes += std::stoull(value);
	}

	// Convert to GB (1 GB = 1024^3 bytes)
	return totalDiskBytes / (1024.0 * 1024.0 * 1024.0);


}

rocksdb::ReadOptions getOptimizedReadOptions() {
	rocksdb::ReadOptions options;
	options.fill_cache = true; // Don't pollute cache for large scans
	options.verify_checksums = false; // Reduce overhead
	options.readahead_size = 256 * 1024;  // 256KB readahead
	return options;
}

bool CSolidStorage::releaseMemory(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType) {
	std::shared_ptr<CTools> tools = getTools();
	rocksdb::DB** dbRef = getDatabaseReference(mode, dbType);
	if (dbRef == nullptr || *dbRef == nullptr) {
		tools->writeLine("Invalid database reference for mode: " + std::to_string(mode) + ", dbType: " + std::to_string(dbType),
			true, true, eViewState::unspecified, "Solid Storage");
		return false;
	}

	try {
		std::unique_lock lock(mDataAccessGuardian);

		// Store initial memory state for comparison
		std::string initMemValue;
		if (!(*dbRef)->GetProperty("rocksdb.estimate-table-readers-mem", &initMemValue)) {
			tools->writeLine("Failed to retrieve initial memory usage", true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}

		uint64_t initialMemory = std::stoull(initMemValue);

		// Step 1: Memory Table Flush
		tools->writeLine("Initiating memtable flush...", true, true, eViewState::unspecified, "Solid Storage");
		rocksdb::FlushOptions flush_options;
		flush_options.wait = true;
		flush_options.allow_write_stall = true; // Allow write stall to ensure flush completes

		rocksdb::Status status = (*dbRef)->Flush(flush_options);
		if (!status.ok() && !status.IsAborted()) {
			tools->writeLine("Memtable flush failed: " + status.ToString(), true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}

		// Step 2: Cache Statistics
		tools->writeLine("Analyzing cache usage...", true, true, eViewState::unspecified, "Solid Storage");
		std::string beforeCache;
		if ((*dbRef)->GetProperty("rocksdb.block-cache-usage", &beforeCache)) {
			tools->writeLine("Cache usage before compaction: " + std::to_string(std::stoull(beforeCache) / (1024 * 1024)) + " MB",
				true, true, eViewState::unspecified, "Solid Storage");
		}

		// Step 3: Light Compaction
		tools->writeLine("Performing light compaction...", true, true, eViewState::unspecified, "Solid Storage");
		rocksdb::CompactRangeOptions compact_options;
		compact_options.bottommost_level_compaction = rocksdb::BottommostLevelCompaction::kForce;
		compact_options.allow_write_stall = true; // Allow write stall to ensure compaction completes
		compact_options.exclusive_manual_compaction = false;
		compact_options.max_subcompactions = 2;

		status = (*dbRef)->CompactRange(compact_options, nullptr, nullptr);
		if (!status.ok() && !status.IsAborted()) {
			tools->writeLine("Compaction failed: " + status.ToString(), true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}

		// Calculate total memory release
		std::string finalMemValue;
		if (!(*dbRef)->GetProperty("rocksdb.estimate-table-readers-mem", &finalMemValue)) {
			tools->writeLine("Failed to retrieve final memory usage", true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}

		uint64_t finalMemory = std::stoull(finalMemValue);

		if (finalMemory < initialMemory) {
			uint64_t totalReleased = (initialMemory - finalMemory) / (1024 * 1024);
			tools->writeLine(tools->getColoredString("Successfully released approximately " +
				std::to_string(totalReleased) + " MB of memory", eColor::lightGreen),
				true, true, eViewState::unspecified, "Solid Storage");
		}
		else {
			tools->writeLine("No significant memory was released during the operation.",
				true, true, eViewState::unspecified, "Solid Storage");
		}

		// Verify database is still operational
		status = (*dbRef)->Put(rocksdb::WriteOptions(), "test_key", "test_value");
		if (!status.ok()) {
			tools->writeLine("Failed to write test key", true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}

		std::string value;
		status = (*dbRef)->Get(rocksdb::ReadOptions(), "test_key", &value);
		if (!status.ok() || value != "test_value") {
			tools->writeLine(tools->getColoredString("Warning: Database read test failed after memory release",
				eColor::orange), true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}

		// Clean up test key
		status = (*dbRef)->Delete(rocksdb::WriteOptions(), "test_key");
		if (!status.ok()) {
			tools->writeLine("Failed to delete test key", true, true, eViewState::unspecified, "Solid Storage");
		}

		return true;
	}
	catch (const std::exception& e) {
		tools->writeLine("Exception during memory release: " + std::string(e.what()),
			true, true, eViewState::unspecified, "Solid Storage");
		return false;
	}
	catch (...) {
		tools->writeLine("Unknown error during memory release", true, true, eViewState::unspecified, "Solid Storage");
		return false;
	}
}

bool CSolidStorage::generateDatabaseReport(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType, std::string& report) {
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::stringstream reportStream;
	rocksdb::DB** dbRef = getDatabaseReference(mode, dbType);

	if (dbRef == nullptr || *dbRef == nullptr) {
		report = tools->getColoredString("Error: Database reference is invalid", eColor::alertError);
		return false;
	}

	// Database Identity Section
	reportStream << tools->getColoredString(u8"════════════════════════════════════════════", eColor::synthPink) << std::endl;
	reportStream << tools->getColoredString(u8"             DATABASE REPORT", eColor::headerCyan) << std::endl;
	reportStream << tools->getColoredString(u8"════════════════════════════════════════════", eColor::synthPink) << std::endl << std::endl;

	std::string dbTypeStr;
	switch (dbType) {
	case eDatabaseType::BlockchainDB: dbTypeStr = "Blockchain DB"; break;
	case eDatabaseType::StateDB: dbTypeStr = "State DB"; break;
	case eDatabaseType::StagedStateDB: dbTypeStr = "Staged State DB"; break;
	}

	std::string modeStr;
	switch (mode) {
	case eBlockchainMode::LIVE: modeStr = "LIVE"; break;
	case eBlockchainMode::TestNet: modeStr = "TestNet"; break;
	case eBlockchainMode::LocalData: modeStr = "LocalData"; break;
	case eBlockchainMode::LIVESandBox: modeStr = "LIVE Sandbox"; break;
	case eBlockchainMode::TestNetSandBox: modeStr = "TestNet Sandbox"; break;
	}

	reportStream << tools->getColoredString("Database Type: ", eColor::ghostWhite)
		<< tools->getColoredString(dbTypeStr, eColor::dataPrimary) << std::endl;
	reportStream << tools->getColoredString("Mode: ", eColor::ghostWhite)
		<< tools->getColoredString(modeStr, eColor::dataPrimary) << std::endl << std::endl;

	// Memory Usage Section
	reportStream << tools->getColoredString(u8"█ MEMORY UTILIZATION", eColor::neonBlue) << std::endl;
	reportStream << tools->getColoredString(u8"────────────────────", eColor::synthPink) << std::endl;

	std::string value;
	if ((*dbRef)->GetProperty("rocksdb.estimate-table-readers-mem", &value)) {
		reportStream << tools->getColoredString("Table Readers: ", eColor::lightCyan)
			<< tools->getColoredString(std::to_string(std::stoull(value) / (1024 * 1024)) + " MB", eColor::dataSecondary) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.cur-size-all-mem-tables", &value)) {
		reportStream << tools->getColoredString("Memtables Total: ", eColor::lightCyan)
			<< tools->getColoredString(std::to_string(std::stoull(value) / (1024 * 1024)) + " MB", eColor::dataSecondary) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.block-cache-usage", &value)) {
		reportStream << tools->getColoredString("Block Cache Usage: ", eColor::lightCyan)
			<< tools->getColoredString(std::to_string(std::stoull(value) / (1024 * 1024)) + " MB", eColor::dataSecondary) << std::endl;
	}

	// Performance Metrics Section
	reportStream << std::endl << tools->getColoredString(u8"█ PERFORMANCE METRICS", eColor::neonGreen) << std::endl;
	reportStream << tools->getColoredString(u8"────────────────────", eColor::synthPink) << std::endl;

	if ((*dbRef)->GetProperty("rocksdb.num-running-compactions", &value)) {
		reportStream << tools->getColoredString("Active Compactions: ", eColor::lightCyan)
			<< tools->getColoredString(value, eColor::dataTertiary) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.num-running-flushes", &value)) {
		reportStream << tools->getColoredString("Active Flushes: ", eColor::lightCyan)
			<< tools->getColoredString(value, eColor::dataTertiary) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.background-errors", &value) && std::stoi(value) > 0) {
		reportStream << tools->getColoredString("Background Errors: ", eColor::alertError)
			<< tools->getColoredString(value, eColor::cyborgBlood) << std::endl;
	}

	// Database Size Metrics
	reportStream << std::endl << tools->getColoredString(u8"█ DATABASE METRICS", eColor::toxicGreen) << std::endl;
	reportStream << tools->getColoredString(u8"────────────────────", eColor::synthPink) << std::endl;

	if ((*dbRef)->GetProperty("rocksdb.estimate-num-keys", &value)) {
		reportStream << tools->getColoredString("Estimated Keys: ", eColor::plasmaTeal)
			<< tools->getColoredString(value, eColor::dataHighlight) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.estimate-live-data-size", &value)) {
		reportStream << tools->getColoredString("Live Data Size: ", eColor::plasmaTeal)
			<< tools->getColoredString(std::to_string(std::stoull(value) / (1024 * 1024)) + " MB", eColor::dataHighlight) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.total-sst-files-size", &value)) {
		reportStream << tools->getColoredString("SST Files Size: ", eColor::plasmaTeal)
			<< tools->getColoredString(std::to_string(std::stoull(value) / (1024 * 1024)) + " MB", eColor::dataHighlight) << std::endl;
	}

	// Level Statistics
	reportStream << std::endl << tools->getColoredString(u8"█ LEVEL STATISTICS", eColor::neonPurple) << std::endl;
	reportStream << tools->getColoredString(u8"────────────────────", eColor::synthPink) << std::endl;

	std::string levelStats;
	if ((*dbRef)->GetProperty("rocksdb.levelstats", &levelStats)) {
		std::istringstream levelStream(levelStats);
		std::string line;
		while (std::getline(levelStream, line)) {
			if (line.find("Level") != std::string::npos) {
				reportStream << tools->getColoredString(line, eColor::cyberYellow) << std::endl;
			}
			else if (!line.empty()) {
				reportStream << "  " << tools->getColoredString(line, eColor::ghostWhite) << std::endl;
			}
		}
	}

	// Compression Stats
	reportStream << std::endl << tools->getColoredString(u8"█ COMPRESSION METRICS", eColor::synthPink) << std::endl;
	reportStream << tools->getColoredString(u8"────────────────────", eColor::synthPink) << std::endl;

	for (int level = 0; level < 7; level++) {
		std::string ratioKey = "rocksdb.compression-ratio-at-level" + std::to_string(level);
		if ((*dbRef)->GetProperty(ratioKey, &value)) {
			reportStream << tools->getColoredString("Level " + std::to_string(level) + " Ratio: ", eColor::dataPrimary)
				<< tools->getColoredString(value, eColor::dataSecondary) << std::endl;
		}
	}

	// File System Stats
	reportStream << std::endl << tools->getColoredString(u8"█ FILE SYSTEM STATUS", eColor::headerCyan) << std::endl;
	reportStream << tools->getColoredString(u8"────────────────────", eColor::synthPink) << std::endl;

	std::string dbPath;
	switch (dbType) {
	case eDatabaseType::BlockchainDB:
		dbPath = getMainDataDir() + "\\" + CGlobalSecSettings::getBlockchainDBID(mode);
		break;
	case eDatabaseType::StateDB:
		dbPath = getMainDataDir() + "\\" + CGlobalSecSettings::getStateDBID(mode);
		break;
	case eDatabaseType::StagedStateDB:
		dbPath = getMainDataDir() + "\\" + CGlobalSecSettings::getStagedStateDBID(mode);
		break;
	}

	reportStream << tools->getColoredString("Path: ", eColor::ghostWhite)
		<< tools->getColoredString(dbPath, eColor::dataHighlight) << std::endl;

	if (std::filesystem::exists(dbPath)) {
		std::filesystem::space_info spaceInfo = std::filesystem::space(dbPath);
		reportStream << tools->getColoredString("Total Space: ", eColor::ghostWhite)
			<< tools->getColoredString(std::to_string(spaceInfo.capacity / (1024 * 1024)) + " MB", eColor::dataPrimary) << std::endl;
		reportStream << tools->getColoredString("Free Space: ", eColor::ghostWhite)
			<< tools->getColoredString(std::to_string(spaceInfo.free / (1024 * 1024)) + " MB", eColor::statusOnline) << std::endl;
		reportStream << tools->getColoredString("Available Space: ", eColor::ghostWhite)
			<< tools->getColoredString(std::to_string(spaceInfo.available / (1024 * 1024)) + " MB", eColor::statusOnline) << std::endl;
	}

	// Additional Database Properties
	reportStream << std::endl << tools->getColoredString(u8"█ ADDITIONAL PROPERTIES", eColor::cyberYellow) << std::endl;
	reportStream << tools->getColoredString(u8"────────────────────", eColor::synthPink) << std::endl;

	if ((*dbRef)->GetProperty("rocksdb.num-immutable-mem-table", &value)) {
		reportStream << tools->getColoredString("Immutable Memtables: ", eColor::lightCyan)
			<< tools->getColoredString(value, eColor::dataTertiary) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.num-entries-active-mem-table", &value)) {
		reportStream << tools->getColoredString("Active Memtable Entries: ", eColor::lightCyan)
			<< tools->getColoredString(value, eColor::dataTertiary) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.num-entries-imm-mem-tables", &value)) {
		reportStream << tools->getColoredString("Immutable Memtable Entries: ", eColor::lightCyan)
			<< tools->getColoredString(value, eColor::dataTertiary) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.num-deletes-active-mem-table", &value)) {
		reportStream << tools->getColoredString("Active Memtable Deletes: ", eColor::lightCyan)
			<< tools->getColoredString(value, eColor::dataTertiary) << std::endl;
	}
	if ((*dbRef)->GetProperty("rocksdb.num-deletes-imm-mem-tables", &value)) {
		reportStream << tools->getColoredString("Immutable Memtable Deletes: ", eColor::lightCyan)
			<< tools->getColoredString(value, eColor::dataTertiary) << std::endl;
	}

	reportStream << tools->getColoredString(u8"════════════════════════════════════════════", eColor::synthPink) << std::endl;

	report = reportStream.str();
	return true;
}

std::vector<std::string> CSolidStorage::unzip(std::string const& zipFile, std::string const& path, std::string const& password)
{

	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::vector<std::string> files = {};
	uint64_t progress = 0;
	mz_zip_archive zip_archive;
	memset(&zip_archive, 0, sizeof(zip_archive));

	auto status = mz_zip_reader_init_file(&zip_archive, zipFile.c_str(), 0);
	if (!status) return files;
	int fileCount = (int)mz_zip_reader_get_num_files(&zip_archive);
	if (fileCount == 0)
	{
		mz_zip_reader_end(&zip_archive);
		return files;
	}
	mz_zip_archive_file_stat file_stat;
	if (!mz_zip_reader_file_stat(&zip_archive, 0, &file_stat))
	{
		mz_zip_reader_end(&zip_archive);
		return files;
	}
	// Get root folder
	std::string lastDir = "";
	if (!std::filesystem::exists(path))
	{
		if (!tools->createDirectoryRecursive(path)) //std::filesystem::create_directory(path))
		{
			return files;
		}
	}
		
		
	std::string base =(path +"\\"); // path delim on end
	std::string elemPath;
	uint64_t lastProgressReportedAt = 0;
	std::string dirPath;
	uint64_t lastSlashPos = 0;
	std::shared_ptr<CStatusBarHub> barHub = CStatusBarHub::getInstance();
	// Get and print information about each file in the archive.
	tools->logEvent("[Update]: About to extract " + std::to_string(fileCount) + " elements, proceeding now..");
	for (int i = 0; i < fileCount; i++)
	{
		try {


			progress = static_cast<uint64_t> (((double)i / (double)fileCount) * (double)100);

			if ((progress - lastProgressReportedAt) >= 1)
			{
				lastProgressReportedAt = progress;
				barHub->setCustomStatusBarText(getBlockchainMode(), 998, "[UI Update]: " + tools->getColoredString(std::to_string(progress) + "%", eColor::lightCyan));
			}


			if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
				continue;

			elemPath = base + file_stat.m_filename;
			if (mz_zip_reader_is_file_a_directory(&zip_archive, i))
			{
				//create directory
				if (!tools->createDirectoryRecursive(elemPath))
				{
					throw "cold not extract a directory.";
				}
				//std::filesystem::create_directory(elemPath);
			}
			else
			{
				lastSlashPos = elemPath.find_last_of("\\/");

				if (lastSlashPos != std::string::npos)
				{
					dirPath = elemPath.substr(0, lastSlashPos);
					if (!tools->createDirectoryRecursive(dirPath))
					{
						throw "cold not extract a directory.";
					}
				}

				if (mz_zip_reader_extract_to_file(&zip_archive, i, elemPath.c_str(), 0))
				{
					elemPath = base + file_stat.m_filename;
					//extract file
					files.emplace_back(elemPath);
				}
				else
				{
					throw "cold not extract a file.";
				}
			}
		}
		catch (std::exception ex)
		{
			throw ex;
			//return files;
		}

	}
	barHub->setCustomStatusBarText(getBlockchainMode(), 998, tools->getColoredString("[UI Update]: maneuvers completed.", eColor::lightCyan));
	// Close the archive, freeing any resources it was using
	mz_zip_reader_end(&zip_archive);

	return files;
}

bool CSolidStorage::destroyDatabase(const std::string& dbPath, rocksdb::Options& options)
{
	rocksdb::Status status = rocksdb::DestroyDB(dbPath, options);
	return status.ok();
}


/// <summary>
/// Destroys a specific database associated with a given blockchain mode and database type.
/// This method constructs the path for the database based on the specified mode and type,
/// and then attempts to destroy it.
/// </summary>
/// <param name="mode">The blockchain mode, which defines the operational environment of the database.</param>
/// <param name="dbType">The type of database to destroy, which can be BlockchainDB, StateDB, or StagedStateDB.</param>
/// <returns>True if the database was successfully destroyed, false if an error occurred.</returns>
bool CSolidStorage::destroySpecificDatabase(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType)
{
	// Initialize RocksDB options for database destruction.
	rocksdb::Options options;

	// Retrieve the main directory path where databases are stored.
	std::string mainDir = getMainDataDir() + "\\";

	// Determine the full path of the database based on the blockchain mode and database type.
	std::string dbPath;
	switch (dbType)
	{
	case eDatabaseType::BlockchainDB:
		dbPath = mainDir + CGlobalSecSettings::getBlockchainDBID(mode);
		break;
	case eDatabaseType::StateDB:
		dbPath = mainDir + CGlobalSecSettings::getStateDBID(mode);
		break;
	case eDatabaseType::StagedStateDB:
		dbPath = mainDir + CGlobalSecSettings::getStagedStateDBID(mode);
		break;
	default:
		// Handle unexpected database type with a comprehensive error message.
		CTools::getInstance()->writeLine("Invalid database type specified.", true, true, eViewState::unspecified, "Solid Storage");
		return false;
	}

	// Attempt to destroy the specified database. Log the path and result for auditing purposes.
	bool result = destroyDatabase(dbPath, options);
	if (!result)
	{
		// Log failure along with the database path for troubleshooting.
		CTools::getInstance()->writeLine("Failed to destroy database at path: " + dbPath, true, true, eViewState::unspecified, "Solid Storage");
	}
	return result;
}



/// <summary>
/// Destroys the databases associated with a specific blockchain mode.
/// </summary>
/// <param name="mode">The blockchain mode whose databases are to be destroyed.</param>
/// <returns>True if all databases for the mode were successfully destroyed, otherwise false.</returns>
bool CSolidStorage::destroyDatabasesForMode(eBlockchainMode::eBlockchainMode mode)
{
    // Ensure the main directory is non-empty
    std::string mainDir = getMainDataDir() + "\\";
    if (mainDir.empty())
    {
        getTools()->writeLine("There's nothing to be destroyed...", true, true, eViewState::unspecified, "Solid Storage");
        return true;
    }

    // Destroy each database type and aggregate the results
    bool result = true;

	// Operational Logic - BEGIN
    result &= destroySpecificDatabase(mode, eDatabaseType::BlockchainDB);
    result &= destroySpecificDatabase(mode, eDatabaseType::StateDB);
    result &= destroySpecificDatabase(mode, eDatabaseType::StagedStateDB);
	// Operational Logic - END
    return result;
}



/// <summary>
/// Destroys all data in Cold-Storage related to specific blockchain modes.
/// This function iterates over a predefined set of blockchain modes, and for each mode,
/// it attempts to destroy the associated databases. If any operation fails, the function
/// returns false, indicating a failure in data destruction.
/// </summary>
/// <returns>True if all data was successfully destroyed for all specified modes, otherwise false.</returns>
bool CSolidStorage::destroyData()
{
	try {
		// First, ensure all databases are closed properly before destruction.
		if (!closeAllDBs(false))
			return false;

		// Specify the blockchain modes that require data destruction.
		eBlockchainMode::eBlockchainMode modes[] = { eBlockchainMode::LIVE, eBlockchainMode::TestNet, eBlockchainMode::LocalData };
		bool allSuccess = true;

		// Process each blockchain mode using a helper function.
		for (auto mode : modes)
		{
			allSuccess &= destroyDatabasesForMode(mode);
		}

		return allSuccess;
	}
	catch (...)
	{
		// Log an unspecified exception as a failure to destroy data.
		getTools()->writeLine("An error occurred during data destruction.", true, true, eViewState::unspecified, "Solid Storage");
		return false;
	}
}



std::string CSolidStorage::getDebugUIDir()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return std::filesystem::absolute(".\\..\\..\\WebUI").string();
}

std::string CSolidStorage::getUIDir()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mUIPackageDir;
}
bool CSolidStorage::checkUIDataAvailable()
{

	std::shared_ptr<CTools> tools = CTools::getInstance();

	bool toRet = false;
	struct stat info;
	std::string strAppDataString;
	std::wstring pBuf;


	DWORD dwRetTemp = GetEnvironmentVariable
	(
		L"APPDATA",  // environment variable name
		NULL, // buffer for variable value
		(DWORD)0      // size of buffer
	);



	if (dwRetTemp != 0)
	{
		pBuf.resize(dwRetTemp, 0);

		GetEnvironmentVariable
		(
			L"APPDATA",  // environment variable name
			pBuf.data(), // buffer for variable value
			dwRetTemp      // size of buffer
		);
	}
	else
	{
		getTools()->writeLine("[UI Update]: There was an error initializing the data directory...", true, true, eViewState::unspecified, "Solid Storage");
		return false;
	}

	strAppDataString = tools->wstring_to_utf8(pBuf);
	strAppDataString.resize(dwRetTemp - 1);
	std::string dataDir = (strAppDataString + "\\GRIDNET");
	//all the essential UI files go over here below

	std::vector < std::string> essentials = { dataDir,
		dataDir + "\\WebUIPublic\\index.html",
		dataDir + "\\WebUIPublic\\maintenance.html" };


	for (uint64_t i = 0; i < essentials.size(); i++)
	{
		if (stat(essentials[i].c_str(), &info) != 0)
		{
			setUIDataAvailable(false);
			return false;
		}
	}
	std::string uiDir = (dataDir + "\\WebUIPublic");

	mFieldsGuardian.lock();
	mUIPackageDir = uiDir;
	mFieldsGuardian.unlock();
	setUIDataAvailable(true);
	return true;
}
bool CSolidStorage::getIsUIUpdateInProgress()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mUIUpdateInProgress;
}

bool CSolidStorage::updateUIPackage(bool initial, bool justCheckForPackage)
{
	

	std::shared_ptr<CTools> tools = CTools::getInstance();
	mFieldsGuardian.lock();
	if (mUIUpdateInProgress)
	{
		tools->logEvent(tools->getColoredString("UI Update already in progress..", eColor::orange) , "UI Update", eLogEntryCategory::localSystem, 10);
		mFieldsGuardian.unlock();
		return false;
	}
	mFieldsGuardian.unlock();

	mFieldsGuardian.lock();
	mUIUpdateInProgress = true;
	mFieldsGuardian.unlock();



	if (initial)
	{

		tools->logEvent("Attempting to install the Decentralized UI Package...", "Solid Storage", eLogEntryCategory::localSystem);
	}
	bool toRet = false;
	struct stat info;
	bool packagePresent = false;
	std::string appDataDir;
	std::wstring pBuf;


	DWORD dwRetTemp = GetEnvironmentVariable
	(
		L"APPDATA",  // environment variable name
		NULL, // buffer for variable value
		(DWORD)0      // size of buffer
	);



	if (dwRetTemp != 0)
	{
		pBuf.resize(dwRetTemp, 0);

		GetEnvironmentVariable
		(
			L"APPDATA",  // environment variable name
			pBuf.data(), // buffer for variable value
			dwRetTemp      // size of buffer
		);
	}
	else
	{
		tools->logEvent("There was an error initializing the data directory...", "Solid Storage", eLogEntryCategory::localSystem, 10);
		UIUpdateExited();
		return false;
	}

	appDataDir = tools->wstring_to_utf8(pBuf);
	appDataDir.resize(dwRetTemp - 1);

	std::string dataDir = (appDataDir + "\\GRIDNET");
	std::string uiDir = (dataDir + "\\WebUIPublic");
	std::string zipFilePath = (dataDir + "\\WebUIPublic.zip");
	if (stat(zipFilePath.data(), &info) != 0)
		packagePresent = false;
	else if (info.st_mode)
		packagePresent = true;
	else
		packagePresent = false;

	if (justCheckForPackage)
	{
		mFieldsGuardian.lock();
		mUIPackageDir = uiDir;
		mFieldsGuardian.unlock();
		UIUpdateExited();
		return packagePresent;
	}


	if (!packagePresent)
	{
		if (initial)
		{
			tools->logEvent("No UI Package present.", "Solid Storage", eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::orange);
		}
		UIUpdateExited();
		return false;
	}


	mFieldsGuardian.lock();
	if (mUIUpdateThread.joinable())
		mUIUpdateThread.join();
	mUIUpdateThread = std::thread(&CSolidStorage::updateUIPackageThreadF, this, appDataDir);
	mFieldsGuardian.unlock();
	return true;
}

void CSolidStorage::UIUpdateExited()
{
	mFieldsGuardian.lock();
	mUIUpdateInProgress = false;
	mFieldsGuardian.unlock();
}



void CSolidStorage::updateUIPackageThreadF(std::string appDataDir)
{
	std::string tName = "UI Update Thread";
	getTools()->SetThreadName(tName.data());
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::string dataDir = (appDataDir + "\\GRIDNET");
	std::string zipFilePath = (dataDir + "\\WebUIPublic.zip");
	std::string uiDir = (dataDir + "\\WebUIPublic");
	std::string uiDirTemp = (dataDir + "\\WebUIPublicTemp");//double buffering
	std::vector<std::string> filesWithin;
	bool toRet = false;
		try {

			//Operational Logic - BEGIN
			tools->logEvent(tools->getColoredString("UI Update Package found.",eColor::lightGreen)+ " Attempting to process..", "UI Update", eLogEntryCategory::localSystem, 10);
			
			mFieldsGuardian.lock();
			mUIPackageDir = uiDir;
			mFieldsGuardian.unlock();

			if (std::filesystem::exists(uiDirTemp))
			{
				tools->logEvent("Getting rid of previous UI Components' temporary directory....", "UI Update", eLogEntryCategory::localSystem, 10);

				if (!std::filesystem::remove_all(uiDirTemp))
				{
					tools->logEvent("Error removing the UI Components' temporary directory!", "UI Update", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
					UIUpdateExited();
					return;
				}
			}

			tools->logEvent("Extracting components to a temporary directory.. " + std::to_string(filesWithin.size()) + " files..", "UI Update", eLogEntryCategory::localSystem, 10);
		    
			try {
				filesWithin = unzip(zipFilePath, uiDirTemp, "");
			}
			catch (...)
			{
				tools->logEvent("There was an error while performing DUI update - the process has FAILED.", "UI Update", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
				return;
			}

			if (filesWithin.size() == 0)
			{
				tools->logEvent("Error extracting files..", "UI Update", eLogEntryCategory::localSystem, 10,eLogEntryType::failure);
				UIUpdateExited();
				return ;
			}

			tools->logEvent("Extracted " + std::to_string(filesWithin.size()) + " files..", "UI Update", eLogEntryCategory::localSystem, 10);
		
			
		
			setUIDataAvailable(false);//disable access to UI..
			Sleep(5000);
			if (std::filesystem::exists(uiDir))
			{
				tools->logEvent("Getting rid of previous Decentralized UI Components..", "UI Update", eLogEntryCategory::localSystem, 10);

				if (!std::filesystem::remove_all(uiDir))
				{
					tools->logEvent("Error removing the UI Package File after install!", "UI Update", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
					UIUpdateExited();
					return;
				}
			}

			tools->logEvent("Activating new Decentralized UI Components..", "UI Update", eLogEntryCategory::localSystem, 10);

			std::filesystem::rename(uiDirTemp, uiDir);

			if(std::filesystem::exists(uiDir)==false)

			{
				tools->logEvent("Error activating new Decentralized UI Components!", "UI Update", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
				UIUpdateExited();
				return;
			}

			tools->logEvent("Getting rid of the UI Package..", "UI Update", eLogEntryCategory::localSystem, 10);

			if (!std::filesystem::remove(zipFilePath))
			{
				tools->logEvent("Error removing the UI Package File after install!", "UI Update", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
				UIUpdateExited();
				return;
			}
			toRet = true;
			setUIDataAvailable(true);//restore access to UI..

			//Operational Logic - END

		}
		catch (std::exception ex)
		{
			//disable access to UI..
			setUIDataAvailable(false); 
			tools->logEvent("The UI update process failed!", "UI Update", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
			UIUpdateExited();
			
		}
		
	

	if (toRet)
	{
		mFieldsGuardian.lock();
		mUIPackageDir = uiDir;
		mFieldsGuardian.unlock();
		setUIDataAvailable(true);
		tools->logEvent(" The UI update process succeeded!", "UI Update", eLogEntryCategory::localSystem, 10, eLogEntryType::notification,eColor::lightGreen);
	}
	else
	{
		tools->logEvent(" The UI update process failed!", "UI Update", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
	}
	UIUpdateExited();

}
bool CSolidStorage::setupDataDir(std::string& path)
{
	std::shared_ptr<CTools> tools = CTools::getInstance();
	tools->writeLine("Attempting to initialize data directory...", true, true, eViewState::unspecified, "Solid Storage");

	bool toRet = false;
	struct stat info;
	bool alreadyExists = false;
	std::string strAppDataString;
	std::wstring pBuf;


	DWORD dwRetTemp = GetEnvironmentVariable
	(
		L"APPDATA",  // environment variable name
		NULL, // buffer for variable value
		(DWORD)0      // size of buffer
	);



	if (dwRetTemp != 0)
	{
	
		pBuf.resize(dwRetTemp, 0);

		GetEnvironmentVariable
		(
			L"APPDATA",  // environment variable name
			pBuf.data(), // buffer for variable value
			dwRetTemp      // size of buffer
		);
	}
	else
	{
		
		tools->writeLine("There was an error initializing the data directory...", true, true, eViewState::unspecified, "Solid Storage");
		return false;
	}

	strAppDataString = tools->wstring_to_utf8(pBuf);
	strAppDataString.resize(dwRetTemp - 1);
	strAppDataString += "\\GRIDNET";
	path = strAppDataString;


	if (stat(strAppDataString.data(), &info) != 0)
		alreadyExists = false;
	else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on my windows 
		alreadyExists = true;
	else
		alreadyExists = false;

	if (!alreadyExists)
	{
		tools->writeLine("trying to create local data directory.." + tools->getColoredString(strAppDataString, eColor::blue) + "'", true, true, eViewState::unspecified, "Solid Storage");

		toRet = std::filesystem::create_directory(strAppDataString);
	}
	else
	{
		tools->writeLine("File directory was initialized at: '"+ tools->getColoredString(strAppDataString, eColor::blue)+"'", true, true, eViewState::unspecified, "Solid Storage");
		toRet = true;
	}

	return toRet;
}

std::string CSolidStorage::getMainDataDir()
{
	//std::lock_guard<std::mutex> lock(mFieldsGuardian);
	if (mMainDataDir.size() == 0)
	{
		if (!setupDataDir(mMainDataDir))
		{
			return "";
		}
	}
	return mMainDataDir;

}

void  CSolidStorage::setMainDataDir(const std::string &dir)
{
	//std::lock_guard<std::mutex> lock(sFieldsGuardian);
	mMainDataDir = dir;

}

void CSolidStorage::setKernelCacheDir(const std::string& dir)
{
	//std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mKernelCacheDir = dir;
}

std::string CSolidStorage::getKernelCacheDir()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mKernelCacheDir;
}

/// <summary>
/// Responsible for initialization of both the LOCAL TEST and LIVE-StateDBs
/// </summary>
/// <returns></returns>
bool CSolidStorage::initialiseSolidStorage()
{

	try {

		std::string dataDir;
		std::shared_ptr<CTools> tools = CTools::getInstance();
		if (!setupDataDir(dataDir))
		{
			tools->writeLine("Error initializing RocksDB directories..", true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}

		setMainDataDir(dataDir);

		
		std::string kernelCacheDir = dataDir + ("\\kernel\\");
		
		//setup directory to store cached already compiled OpenCL Kernels
		if (tools->createDirectoryRecursive(kernelCacheDir))
		{
			tools->writeLine(tools->getColoredString("successfully", eColor::lightGreen) + " deployed OpenCL cache at: '" +
				tools->getColoredString(kernelCacheDir, eColor::blue) + "'");
			
			tools->writeLine(tools->getColoredString("Testing OpenCL cache", eColor::orange) + " at: '" +
				tools->getColoredString(kernelCacheDir, eColor::blue) + "'");
	
			std::string filename = kernelCacheDir + ("\\" + tools->getRandomStr(7) + ".bin");

			std::ofstream myfile;
			bool testOk = true;
			myfile.open(filename, std::ios::trunc  | std::ios::out | std::ios_base::binary);
			if (!myfile.is_open())
			{
			
				testOk = false;
			}
			if (testOk)
			{
				myfile << "content";
				myfile.close();

				// Delete the test file after closing it
				if (std::remove(filename.c_str()) != 0) {
					tools->writeLine(tools->getColoredString("Warning: Failed to delete test file", eColor::lightPink));
				}
			}

			if (testOk)
			{
				tools->writeLine("OpenCL cache was tested "+ tools->getColoredString("successfully!", eColor::lightGreen) , eLogEntryCategory::localSystem, 10);
				setKernelCacheDir(kernelCacheDir);
			}
			else
			{
				tools->writeLine("OpenCL cache tests " + tools->getColoredString("FAILED!", eColor::cyborgBlood), eLogEntryCategory::localSystem, 10);
			}
		}
		else
		{
			tools->writeLine(tools->getColoredString("ERROR", eColor::cyborgBlood) + " deploying OpenCL cache at: '" +
				tools->getColoredString(kernelCacheDir, eColor::blue));

			setKernelCacheDir("");
		}
		

		if (dataDir.size())
		{
			dataDir += "\\";
		}
		
	
		if (mWereRocksDBsDatabasesInitialized)
		{

			return true;
		}

		tools->writeLine("Now initializing RocksDBs...", true, true, eViewState::unspecified, "Solid Storage");

		if (!initialiseRocksDBs())
		{
			return false;
		}

		mWereRocksDBsDatabasesInitialized = true;
		return true;
	
	}
	catch (std::exception ex)
	{
	CTools::getInstance()->writeLine("Error initializing rocks....", true, true, eViewState::unspecified, "Solid Storage");

		return false;
	}
}
/// <summary>
/// Retrieves the reference to the database pointer based on the blockchain mode and database type.
/// </summary>
/// <param name="mode">Blockchain mode (e.g., LIVE, TestNet, LocalData).</param>
/// <param name="type">Type of database (e.g., BlockchainDB, StateDB, StagedStateDB).</param>
/// <returns>Pointer to the database pointer (rocksdb::DB**).</returns>
rocksdb::DB** CSolidStorage::getDatabaseReference(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType type)
{
	switch (mode)
	{
	case eBlockchainMode::LIVE:
		switch (type)
		{
		case eDatabaseType::BlockchainDB:
			return &sLIVEBlockchainDB;
		case eDatabaseType::StateDB:
			return &sLIVEStateTrieDB;
		case eDatabaseType::StagedStateDB:
			return &sLIVEStagedStateTrieDB;
		default:
			break; // Optionally add error handling or logging here
		}
		break;
	case eBlockchainMode::TestNet:
		switch (type)
		{
		case eDatabaseType::BlockchainDB:
			return &sTestNetBlockchainDB;
		case eDatabaseType::StateDB:
			return &sTestNetStateTrieDB;
		case eDatabaseType::StagedStateDB:
			return &sTestNetStagedStateTrieDB;
		default:
			break; // Optionally add error handling or logging here
		}
		break;
	case eBlockchainMode::LocalData:
		switch (type)
		{
		case eDatabaseType::BlockchainDB:
			return &sLocalTestBlockchainDB;
		case eDatabaseType::StateDB:
			return &sLocalTestStateTrieDB;
		case eDatabaseType::StagedStateDB:
			return &sLocalTestStagedStateTrieDB;
		default:
			break; // Optionally add error handling or logging here
		}
		break;
	default:
		// Handle unknown or unsupported modes
		CTools::getInstance()->writeLine("Unsupported blockchain mode provided.", true, true, eViewState::unspecified, "Solid Storage");
		return nullptr;
	}

	// Handle invalid database type or mode by returning nullptr or asserting
	CTools::getInstance()->writeLine("Invalid database type or mode provided.", true, true, eViewState::unspecified, "Solid Storage");
	return nullptr;
}


void CSolidStorage::configureRocksDBOptions(rocksdb::Options& options, const HardwareSpecs& specs) {
	std::stringstream report;
	report << "RocksDB Options Configuration Report:\n";

	auto logOption = [&report](const std::string& name, const auto& value) {
		report << name << ": " << value << "\n";
		};

	options.allow_concurrent_memtable_write = true;
	options.enable_write_thread_adaptive_yield = true;
	options.enable_blob_files = true;
	options.min_blob_size = 1024 * 1024; // 1MB - values larger than this go to blob files
	options.blob_file_size = 1024 * 1024 * 1024; // 1GB blob files
	options.blob_compression_type = rocksdb::kLZ4Compression;
	options.create_if_missing = true;
	logOption("create_if_missing", options.create_if_missing);

	options.max_background_jobs = min(specs.cpuCores, 8);
	logOption("max_background_jobs", options.max_background_jobs);

	options.max_subcompactions = min(specs.cpuCores / 2, 4);
	logOption("max_subcompactions", options.max_subcompactions);

	options.bytes_per_sync = 1024 * 1024;  // 1MB
	logOption("bytes_per_sync", options.bytes_per_sync);

	uint64_t memTableSize = min(specs.totalMemory / 8, static_cast<uint64_t>(64) * 1024 * 1024);
	options.write_buffer_size = 32 * 1024 * 1024;  // 32MB, smaller than default
	options.arena_block_size = 512 * 1024;         // 512KB, reduces allocation size
	options.memtable_factory.reset(
		new rocksdb::SkipListFactory()  // Use default skiplist configuration
	);

	options.max_write_buffer_number = 3;
	logOption("max_write_buffer_number", options.max_write_buffer_number);

	options.min_write_buffer_number_to_merge = 1;
	logOption("min_write_buffer_number_to_merge", options.min_write_buffer_number_to_merge);

	options.compaction_style = rocksdb::kCompactionStyleLevel;
	logOption("compaction_style", "kCompactionStyleLevel");

	// Set to true if purely random access
	options.advise_random_on_open = true;

	options.level_compaction_dynamic_level_bytes = true;
	logOption("level_compaction_dynamic_level_bytes", options.level_compaction_dynamic_level_bytes);

	options.num_levels = 4;
	logOption("num_levels", options.num_levels);

	options.max_bytes_for_level_base = 10 * memTableSize;
	logOption("max_bytes_for_level_base", options.max_bytes_for_level_base);

	options.max_bytes_for_level_multiplier = 10;
	logOption("max_bytes_for_level_multiplier", options.max_bytes_for_level_multiplier);

	options.target_file_size_base = memTableSize;
	logOption("target_file_size_base", options.target_file_size_base);

	options.target_file_size_multiplier = 1;
	logOption("target_file_size_multiplier", options.target_file_size_multiplier);

	options.level0_file_num_compaction_trigger = 2;
	logOption("level0_file_num_compaction_trigger", options.level0_file_num_compaction_trigger);

	options.level0_slowdown_writes_trigger = 20;
	logOption("level0_slowdown_writes_trigger", options.level0_slowdown_writes_trigger);

	options.level0_stop_writes_trigger = 36;
	logOption("level0_stop_writes_trigger", options.level0_stop_writes_trigger);

	options.compression = rocksdb::kLZ4Compression;
	logOption("compression", "kLZ4Compression");

	options.compression_per_level = {
		rocksdb::kNoCompression,
		rocksdb::kLZ4Compression,
		rocksdb::kLZ4Compression,
		rocksdb::kLZ4Compression
	};
		

	logOption("compression_per_level_size", options.compression_per_level.size());

	if (specs.hasSSDs) {
		options.use_direct_io_for_flush_and_compaction = true;
		options.use_direct_reads = true; // Supported in newer RocksDB
		options.compaction_pri = rocksdb::kMinOverlappingRatio;
		logOption("compaction_pri", "kMinOverlappingRatio");
	}
	else {
		options.compaction_pri = rocksdb::kByCompensatedSize;
		logOption("compaction_pri", "kByCompensatedSize");
	}
	logOption("use_direct_io_for_flush_and_compaction", options.use_direct_io_for_flush_and_compaction);

	rocksdb::BlockBasedTableOptions table_options;
	table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
	logOption("bloom_filter_bits_per_key", 10);

	uint64_t blockCacheSize = min(specs.totalMemory / 8, static_cast<uint64_t>(2048) * 1024 * 1024);
	table_options.block_cache = rocksdb::NewLRUCache(
		blockCacheSize,
		6,  // Number of shards for better concurrent access
		true, // strict_capacity_limit - CRITICAL for memory control
		0.5   // high_pri_pool_ratio
	);

	logOption("block_cache_size", blockCacheSize);

	table_options.block_size = specs.hasSSDs ? 16 * 1024 : 4 * 1024;
	//
	// In configureRocksDBOptions(), modify:
	//table_options.cache_index_and_filter_blocks_in_cache = false; // Save memory
	table_options.metadata_block_size = 4096;  // 4KB metadata blocks
	table_options.read_amp_bytes_per_bit = 8;  // More aggressive bloom filter
	table_options.whole_key_filtering = true;  // Save memory if prefix scan is enough
	//
	logOption("block_size", table_options.block_size);

	table_options.cache_index_and_filter_blocks = true;
	logOption("cache_index_and_filter_blocks", table_options.cache_index_and_filter_blocks);

	table_options.pin_l0_filter_and_index_blocks_in_cache = false;
	table_options.pin_top_level_index_and_filter = true; // Better than pin_l0
	logOption("pin_l0_filter_and_index_blocks_in_cache", table_options.pin_l0_filter_and_index_blocks_in_cache);

	options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

	options.optimize_filters_for_hits = true;
	logOption("optimize_filters_for_hits", options.optimize_filters_for_hits);

	options.max_open_files =  -1;//5000
	logOption("max_open_files", options.max_open_files);

	options.avoid_flush_during_shutdown = true;
	logOption("avoid_flush_during_shutdown", options.avoid_flush_during_shutdown);

	options.enable_pipelined_write = true;
	logOption("enable_pipelined_write", options.enable_pipelined_write);

	logOption("total_memory", specs.totalMemory);
	logOption("cpu_cores", specs.cpuCores);
	logOption("has_SSDs", specs.hasSSDs);

	// Write the entire report in a single invocation
	if (false)
	{
		CTools::getInstance()->writeLine(report.str(), true, true, eViewState::unspecified, "Solid Storage");
	}
}
void CSolidStorage::setCleanupIntervalSeconds(uint64_t seconds) {
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mCleanupIntervalSeconds = seconds;
}

uint64_t CSolidStorage::getCleanupIntervalSeconds() const {
	return mCleanupIntervalSeconds;
}

bool CSolidStorage::isCleanupNeeded() const {
	uint64_t currentTime = std::time(nullptr);
	return (currentTime - mLastCleanupTime) >= mCleanupIntervalSeconds;
}

void CSolidStorage::performCleanup() {
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	// Perform cleanup for each mode and database type
	for (auto mode : { eBlockchainMode::LIVE, eBlockchainMode::TestNet, eBlockchainMode::LocalData }) {
		for (auto dbType : { eDatabaseType::BlockchainDB, eDatabaseType::StateDB, eDatabaseType::StagedStateDB }) {
			if (getTotalRAMUsageGB(mode, dbType) > 5.0) { // Threshold of 5GB
				releaseMemory(mode, dbType);
			}
		}
	}

	mLastCleanupTime = std::time(nullptr);
}
void CSolidStorage::periodicMemoryCheck() {
	for (auto mode : { eBlockchainMode::LIVE, eBlockchainMode::TestNet, eBlockchainMode::LocalData }) {
		for (auto dbType : { eDatabaseType::BlockchainDB, eDatabaseType::StateDB, eDatabaseType::StagedStateDB }) {
			if (getTotalRAMUsageGB(mode, dbType) > 10.0) { // 10GB threshold
				releaseMemory(mode, dbType);
			}
		}
	}
}

/// <summary>
/// Initializes a specific RocksDB database based on the given blockchain mode and database type,
/// with comprehensive error handling.
/// </summary>
/// <param name="mode">The blockchain mode for which the database is initialized.</param>
/// <param name="dbType">The specific type of database to initialize (BlockchainDB, StateDB, StagedStateDB).</param>
/// <param name="dbRef">Reference to the database pointer to be initialized.</param>
/// <returns>True if the database was successfully initialized, otherwise false.</returns>
bool CSolidStorage::initializeDatabase(eBlockchainMode::eBlockchainMode mode, eDatabaseType::eDatabaseType dbType, rocksdb::DB** dbRef, const HardwareSpecs& specs)
{
	rocksdb::Options options;
	configureRocksDBOptions(options, specs);
	options.create_if_missing = true;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::string dataDir = getMainDataDir() + "\\";
	std::string dbPath;

	// Construct the database path based on the mode and type
	switch (dbType)
	{
	case eDatabaseType::BlockchainDB:
		dbPath = dataDir + CGlobalSecSettings::getBlockchainDBID(mode);
		break;
	case eDatabaseType::StateDB:
		dbPath = dataDir + CGlobalSecSettings::getStateDBID(mode);
		break;
	case eDatabaseType::StagedStateDB:
		dbPath = dataDir + CGlobalSecSettings::getStagedStateDBID(mode);
		break;
	}

	// Check if the directory exists, if not, create it
	if (!std::filesystem::exists(dbPath)) {
		std::error_code ec;
		if (!std::filesystem::create_directories(dbPath, ec)) {
			tools->writeLine(tools->getColoredString("Failed to create directory: ", eColor::lightPink) + dbPath + ". Error: " + ec.message(), true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}
	}

	// Log the options for debugging
	tools->writeLine("RocksDB options for " + dbPath + ":", true, true, eViewState::unspecified, "Solid Storage");
	tools->writeLine("max_background_jobs: " + std::to_string(options.max_background_jobs), true, true, eViewState::unspecified, "Solid Storage");
	tools->writeLine("write_buffer_size: " + std::to_string(options.write_buffer_size), true, true, eViewState::unspecified, "Solid Storage");
	tools->writeLine("max_write_buffer_number: " + std::to_string(options.max_write_buffer_number), true, true, eViewState::unspecified, "Solid Storage");
	// Add more option logging as needed

	rocksdb::Status status = rocksdb::DB::Open(options, dbPath, dbRef);
	if (!status.ok())
	{
		tools->writeLine(tools->getColoredString("Initialization failed for: ", eColor::lightPink) + dbPath + ". Error: " + status.ToString(), true, true, eViewState::unspecified, "Solid Storage");

		// Log more details about the error
		if (status.IsInvalidArgument()) {
			tools->writeLine("Invalid argument error. This could be due to incompatible options or incorrect path.", true, true, eViewState::unspecified, "Solid Storage");
		}
		else if (status.IsIOError()) {
			tools->writeLine("IO error. This could be due to permission issues or disk problems.", true, true, eViewState::unspecified, "Solid Storage");
		}

		// Attempt to clean the database if initial opening fails.
		destroySpecificDatabase(mode, dbType);
		tools->writeLine(tools->getColoredString("Attempting re-initialization after cleaning: ", eColor::orange) + dbPath, true, true, eViewState::unspecified, "Solid Storage");

		// Re-attempt to initialize the database
		status = rocksdb::DB::Open(options, dbPath, dbRef);
		if (!status.ok())
		{
			tools->writeLine(tools->getColoredString("Re-initialization failed for: ", eColor::lightPink) + dbPath + ". Error: " + status.ToString(), true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}
	}

	tools->writeLine(tools->getColoredString("Database initialized: ", eColor::lightGreen) + dbPath, true, true, eViewState::unspecified, "Solid Storage");
	return true;
}
/// <summary>
/// Responsible for the initialization of all required RocksDB databases for each mode and type.
/// </summary>
/// <returns>True if all databases were successfully initialized, otherwise false.</returns>
bool CSolidStorage::initialiseRocksDBs()
{
	try {
		HardwareSpecs specs = CTools::getInstance()->assessHardware();

		std::string dataDir;
		std::shared_ptr<CTools> tools = CTools::getInstance();
		if (!setupDataDir(dataDir))
		{
			tools->writeLine("Error initializing RocksDB directories..", true, true, eViewState::unspecified, "Solid Storage");
			return false;
		}

		setMainDataDir(dataDir);

		// List of all blockchain modes and database types to initialize.
		eBlockchainMode::eBlockchainMode modes[] = { eBlockchainMode::LIVE, eBlockchainMode::TestNet, eBlockchainMode::LocalData };
		eDatabaseType::eDatabaseType types[] = { eDatabaseType::BlockchainDB, eDatabaseType::StateDB, eDatabaseType::StagedStateDB };

		// Iterate through each mode and type, initializing the corresponding database.
		for (auto mode : modes)
		{
			for (auto type : types)
			{
				rocksdb::DB** dbRef = getDatabaseReference(mode, type);  // Assuming getDatabaseReference returns the correct DB pointer based on mode and type.
				if (!initializeDatabase(mode, type, dbRef, specs))
				{
					return false; // Return false if any database initialization fails.
				}
			}
		}

		return true; // Return true if all initializations succeed.
	}
	catch (std::exception& ex)
	{
		CTools::getInstance()->writeLine("Exception during RocksDB initialization: " + std::string(ex.what()), true, true, eViewState::unspecified, "Solid Storage");
		return false;
	}
}


bool CSolidStorage::initializeSQLDB()
{
	bool retflag = true;
	//initialize SQL
	int rc;
	bool retried = false;
	getTools()->writeLine("Initializing SQL Database.", true, true, eViewState::unspecified, "Solid Storage");
	 openAgain:
	getTools()->writeLine("Opening " + CGlobalSecSettings::getSQLDBID() + ".db ...", true, true, eViewState::unspecified, "Solid Storage");
	rc = sqlite3_open(reinterpret_cast<char *>(getTools()->stringToBytes(CGlobalSecSettings::getSQLDBID() + ".db").data()), &mSQLDB);
	char *zErrMsg = 0;
	char *szSQL;
	if (rc)
	{
		getTools()->writeLine("Error opening SQLite3 database", true, true, eViewState::unspecified, "Solid Storage");
		getTools()->writeLine("I will retry", true, true, eViewState::unspecified, "Solid Storage");
		if (!retried)
		{
			retried = true;
			goto openAgain;

		}
		sqlite3_close(mSQLDB);
		return false;
	}
	else
	{
		getTools()->writeLine("Opened database.", true, true, eViewState::unspecified, "Solid Storage");
	}

	// Execute SQL
	getTools()->writeLine("checking if SQL tables exists ...", true, true, eViewState::unspecified, "Solid Storage");
	bool exsits = doesSQLTableExist(DBIdentTokensTable);
	if (!exsits)
		createIdentityTokenTable();
	retflag = true;

	return retflag;
}
/// <summary>
/// Deletes given entry from Cold Storage.
/// Warning: the key is explicit. Use public deleteNode method doe trie node deletion.
/// </summary>
bool CSolidStorage::deleteValue(std::vector<uint8_t> key)
{


	if (mAbortAllWrites)
		return true;
	std::string keyS = std::string(key.begin(), key.end());
	
	
	std::unique_lock lock(mDataAccessGuardian);
	if (mStateTrieDB->Delete(rocksdb::WriteOptions(), keyS).ok())
		return true;
	else return false;

}

extern "C" int callback(void* data, int count, char** rows, char**)
{
	if (count == 1 && rows) {
		*static_cast<int*>(data) = atoi(rows[0]);
		return 0;
	}
	return 1;
}

bool CSolidStorage::doesSQLTableExist(std::string tableName)
{
	bool found = false;
	char *zErrMsg = NULL;
	const std::string sql = "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table'  AND name = '" + tableName+"'";
	int rc = 0;
	int count = 0;
	sqlite3_stmt *stmt = NULL;
	char *szSQL;
    rc = sqlite3_exec(mSQLDB, &sql.c_str()[0], callback, &count, &zErrMsg);
	//sqlite3_prepare_v2(db,
	//	"SELECT COUNT(*) FROM sqlite_master WHERE type = 'table'  AND name = ?", -1, &stmt, NULL);
	//rc = sqlite3_bind_text(stmt, 1, tableName.c_str(), strlen(tableName.c_str()), 0);
	if (count > 0)
		found = true;

	//sqlite3_finalize(stmt);
	return found;
}

bool CSolidStorage::createIdentityTokenTable()
{
	int rc = 0;
	sqlite3_stmt *stmt = NULL;
	char *szSQL;
	bool done = true;
    sqlite3_prepare_v2(mSQLDB, "CREATE TABLE IdentityTokens (ID INTEGER PRIMARY KEY, address BLOB NOT NULL,  nonce INT NOT NULL, concats INT NOT NULL, friendlyID BLOB,  pow BLOB)", -1, &stmt, NULL);     /* 2 */
	//rc = sqlite3_bind_text(stmt, 1, DBIdentTokensTable.c_str(), strlen(DBIdentTokensTable.c_str()), 0);
	rc = sqlite3_step(stmt);                                                                    /* 3 */
	if (rc != SQLITE_DONE) {
		done = false;
		printf("ERROR inserting data: %s\n", sqlite3_errmsg(sqlDB));
	}
	
	sqlite3_finalize(stmt);
	return done;
}



bool CSolidStorage::saveNode(std::vector<uint8_t> id,std::vector<uint8_t> BER_TrieNode, std::string prefix )
{

	if (mAbortAllWrites)
		return true;
	std::string key = std::string(id.begin(), id.end());
	std::string value = std::string(BER_TrieNode.begin(), BER_TrieNode.end());
	uint64_t tries = 0;
	uint64_t waitFor = 100;

	key = H_NODE_HASH + prefix+key;
	rocksdb::Status mResult;
retry:
	{
		std::unique_lock lock(mDataAccessGuardian);
		mResult = mStateTrieDB->Put(rocksdb::WriteOptions(), key, value);
	}
	if (mResult.ok())
		return true;
	else {
		//Todo: Note RocksDB DOES NOT recover from the out-of-space-error!
		//ROcksDB needs to be restarted OR we need to be checking for free memory in advance
		waitFor=waitFor * 2;
		
		getTools()->writeLine("Error saving node to Cold Storage. ("+ mResult.ToString()+").\n I'll wait for "+ std::to_string(waitFor)+" ms");
		std::this_thread::sleep_for(std::chrono::milliseconds(waitFor));
		if (waitFor > 20000)
			waitFor = 100;
		goto retry;
		return false;
	}
}

bool CSolidStorage::getAbortAllWrites()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mAbortAllWrites; 
}
bool CSolidStorage::deleteNode(std::vector<uint8_t> nodeID, std::string prefix)
{

	if (getAbortAllWrites())
		return true;
	std::string key = H_NODE_HASH + prefix + std::string(nodeID.begin(), nodeID.end());

	std::unique_lock lock(mDataAccessGuardian);

	return mStateTrieDB->Delete(rocksdb::WriteOptions(), key).ok();
}

bool CSolidStorage::saveCurrentTrieRoot(CTrieNode * node,std::string prefix)
{

	std::unique_lock lock(mDataAccessGuardian);

	if (!getIsSystemAvailable())
		return false;

	if (mAbortAllWrites)
		return true;
	std::vector<uint8_t> BER_TrieNode = node->getPackedData();
    std::vector<uint8_t> hash = CCryptoFactory::getInstance()->getSHA2_256Vec(BER_TrieNode);

	std::string key = std::string(hash.begin(), hash.end());
	std::string value = std::string(BER_TrieNode.begin(), BER_TrieNode.end());

	key = H_TRIE_ROOT_HASH + prefix;

	 if (mStateTrieDB->Put(rocksdb::WriteOptions(), key, value).ok())
		 return true;
	 else return false;
}

/// <summary>
/// Returns a new instance of TrieRoot.
/// </summary>
/// <param name="prefix"></param>
/// <param name="rootHash"></param>
/// <returns></returns>
CTrieNode * CSolidStorage::loadNode(std::string dbPrefix,std::vector<uint8_t> nodeHash)
{
	CTrieNode *r = NULL;
	std::shared_ptr<CBlock> block = NULL;

		if (nodeHash.size() == 0)
			return NULL;
	std::vector<uint8_t> bytes 	 = loadNode(nodeHash, dbPrefix);
	std::lock_guard<std::mutex> lock(mBlockchainModeGuardian);
	if(bytes.size()>0)
    r=getTools()->nodeFromBytes(bytes,mBlockchainMode);
	else
	r = NULL;
	return r;
}

bool CSolidStorage::readBinaryFromFile(std::string fileName, std::vector<uint8_t>& data)
{
	try {
		std::ifstream ifs(fileName, std::ios::in | std::ios::binary);

		if (!ifs.is_open())
			return false;
		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(ifs), {});
		data = std::move(buffer);
		return true;
	}
	catch (...)
	{
		return false;
	}

}

std::shared_ptr<CTools> CSolidStorage::getTools()
{
	std::lock_guard<std::mutex> lock(mToolsGuardian);
	return mTools;
}

void CSolidStorage::setUIDataAvailable(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mUIDataAvailable = isIt;
}

bool CSolidStorage::getUIDataAvailable()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mUIDataAvailable;
}

void CSolidStorage::setIsSystemAvailable(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mSystemAvailable = isIt;
	mAbortAllWrites = true;
}

bool CSolidStorage::getIsSystemAvailable()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mSystemAvailable;
}

bool CSolidStorage::saveStringToFile(std::string data, std::string fileName)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(data);
	return saveToUTF8File(wide, fileName);
}

bool CSolidStorage::readStringFromFile(std::string & data, std::string fileName)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide;
	if (!readFromUTF8File( wide, fileName))
		return false;
	std::string narrow = converter.to_bytes(wide);
	data = narrow;
	return true;
}

std::vector<std::string> CSolidStorage::getAllFilesInDir(std::string dirPath)
{
	std::vector<std::string> mResult;
	try {

		if (dirPath.size() == 0)
			return mResult;

		for (const auto & entry : fs::directory_iterator(dirPath))
			mResult.push_back(entry.path().u8string());

		return mResult;
	}
	catch (...) {
		return mResult;
	}
}

bool CSolidStorage::saveToUTF8File(std::wstring data, std::string fileName)
{
	try {
		std::string dataDir = getMainDataDir();
		if (dataDir.size() == 0)
		{
			return false;
		}
		std::wstring wFromFile = data;
		std::wofstream fileOut(dataDir+"\\"+fileName);
		fileOut.imbue(std::locale(fileOut.getloc(), new std::codecvt_utf8_utf16<wchar_t>));
		fileOut << wFromFile;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool CSolidStorage::readFromUTF8File(std::wstring & data, std::string fileName)
{
	try {
		std::string dataDir = getMainDataDir();
		if (dataDir.size() == 0)
		{
			return false;
		}
		std::wifstream fileIn(dataDir+"\\"+fileName);
		fileIn.imbue(std::locale(fileIn.getloc(), new std::codecvt_utf8_utf16<wchar_t>));
	
		data =	std::wstring((std::istreambuf_iterator<wchar_t>(fileIn)),
			std::istreambuf_iterator<wchar_t>());
		return true;
	}
	catch (...)
	{
		return false;
	}
}

eBlockchainMode::eBlockchainMode CSolidStorage::getBlockchainMode()
{
	std::lock_guard<std::mutex> lock(mBlockchainModeGuardian);
	return mBlockchainMode;
}

CSolidStorage::CSolidStorage(const CSolidStorage & sibling)
{
	mCf = sibling.mCf;
}

bool CSolidStorage::isTestDB()
{
	return mIsTestDB;
}                                                                                      

CSolidStorage::CSolidStorage(eBlockchainMode::eBlockchainMode mode, bool init)
{
	mCleanupIntervalSeconds = DEFAULT_CLEANUP_INTERVAL_SECONDS;
	mLastCleanupTime = std::time(nullptr);

	mUIUpdateInProgress = false;
	mUIDataAvailable = false;
	mSystemAvailable = false;
	mStateTrieDB = nullptr;
	mBlockchainDB = nullptr;

	if (staticVirginityDetector)
	{
		sLIVEStagedStateTrieDB = nullptr;
		sLIVEInstancePointer = nullptr;
		sLIVESandBoxInstancePointer = nullptr;
		sTestNetInstancePointer = nullptr;
		sTestNetSandBoxInstancePointer = nullptr;
		sLocalTestsInstancePointer = nullptr;
		staticVirginityDetector = false;
		sLocalTestStateTrieDB = nullptr;
		sLocalTestBlockchainDB = nullptr;
	}
	mCf = CCryptoFactory::getInstance();
	mTools = std::make_shared<CTools>("SolidStorage", eBlockchainMode::TestNet);// CBlockchainManager::getInstance(mode)->getTools();
	mBlockchainMode = mode;

	mAbortAllWrites = false;
	mIsTestDB = false;
	
	mSQLDB = NULL;

	if (init && !mWereRocksDBsDatabasesInitialized)
		if (!initialiseSolidStorage())
		{
			getTools()->writeLine("There was a critical error while initializing one of the RocksDBs.\n "+ 
				getTools()->getColoredString("Shutting down now..", eColor::RAPID_BLINK_FLAG | eColor::alertError), true, true, eViewState::unspecified, "Solid Storage");
			
				//Sleep(3000);
				//terminate();
			
		 //assertGN(false);
		}



	switch (mode)
	{
	case eBlockchainMode::eBlockchainMode::LIVE:
		mStateTrieDB = sLIVEStateTrieDB;
		mBlockchainDB = sLIVEBlockchainDB;
		mStagedStateTrieDB = sLIVEStagedStateTrieDB;
	 assertGN(sLIVEInstancePointer == nullptr);
		sLIVEInstancePointer = this;
		break;
	case eBlockchainMode::eBlockchainMode::LIVESandBox:
		mAbortAllWrites = true;
		mStateTrieDB = sLIVEStateTrieDB;
		mBlockchainDB = sLIVEBlockchainDB;
		mStagedStateTrieDB = sLIVEStagedStateTrieDB;
	 assertGN(sLIVESandBoxInstancePointer == nullptr);
		sLIVESandBoxInstancePointer = this;
		break;
	case eBlockchainMode::eBlockchainMode::TestNet:
		mIsTestDB = true;
		mStateTrieDB = sTestNetStateTrieDB;
		mBlockchainDB = sTestNetBlockchainDB;
		mStagedStateTrieDB = sTestNetStagedStateTrieDB;
	 assertGN(sTestNetInstancePointer == nullptr);
		sTestNetInstancePointer = this;
		break;
	case eBlockchainMode::eBlockchainMode::TestNetSandBox:
		mIsTestDB = true;
		mAbortAllWrites = true;

		mStateTrieDB = sTestNetStateTrieDB;
		mBlockchainDB = sTestNetBlockchainDB;
		mStagedStateTrieDB = sTestNetStagedStateTrieDB;
	 assertGN(sTestNetSandBoxInstancePointer == nullptr);
		sTestNetSandBoxInstancePointer = this;
		break;
	case   eBlockchainMode::eBlockchainMode::LocalData:
		mIsTestDB = true;
		mStateTrieDB = sLocalTestStateTrieDB;
		mBlockchainDB = sLocalTestBlockchainDB;
		mStagedStateTrieDB = sLocalTestStagedStateTrieDB;
	 assertGN(sLocalTestsInstancePointer == nullptr);
		sLocalTestsInstancePointer = this;
		break;

	}

	staticVirginityDetector = false;

     mRecentBlocksCacheSize = 10;
	 mSystemAvailable = true;

}

CSolidStorage::~CSolidStorage()
{
	mTools = nullptr;
}

bool CSolidStorage::saveLink(std::shared_ptr<CLinkContainer> link)
{
	return saveLink(link->getKey(), link->getValue(), link->getType());
}

 CSolidStorage *  CSolidStorage::getInstance(eBlockchainMode::eBlockchainMode mode)
 {

	 std::lock_guard<std::mutex> lock(sStaticInstancesGuardian);

	 
	 if (staticVirginityDetector)
	 {
		 sLIVEInstancePointer = nullptr;
		 sLIVESandBoxInstancePointer = nullptr;
		 sTestNetInstancePointer = nullptr;
		 sTestNetSandBoxInstancePointer = nullptr;
		 staticVirginityDetector = false;
	 }
	 switch (mode)
	 {
	 case eBlockchainMode::LIVE:
		 if (sLIVEInstancePointer == NULL)
			 sLIVEInstancePointer = new CSolidStorage(mode);
		 return sLIVEInstancePointer;
		 break;
	 case eBlockchainMode::TestNet:
		 if (sTestNetInstancePointer == NULL)
			 sTestNetInstancePointer = new CSolidStorage(mode);
		 return sTestNetInstancePointer;
		 break;
	 case eBlockchainMode::LIVESandBox:
		 if (sLIVESandBoxInstancePointer == NULL)
			 sLIVESandBoxInstancePointer = new CSolidStorage(mode);
		 return sLIVESandBoxInstancePointer;
		 break;
	 case eBlockchainMode::TestNetSandBox:
		 if (sTestNetSandBoxInstancePointer == NULL)
			 sTestNetSandBoxInstancePointer = new CSolidStorage(mode);
		 return sTestNetSandBoxInstancePointer;
		 break;
	 case  eBlockchainMode::LocalData:
		 if (sLocalTestsInstancePointer == NULL)
			 sLocalTestsInstancePointer = new CSolidStorage(mode);
		 return sLocalTestsInstancePointer;
		 break;
	 default:
		 assertGN(false);//should not happen.
		 break;
	 }
	 return nullptr;
 }

 std::string CSolidStorage::getMemUsageReport()
 {
	 std::shared_lock lock(mDataAccessGuardian);
	 std::string report;
	 std::string l;
	 
	 mStateTrieDB->GetProperty("rocksdb.estimate-table-readers-mem", &l);
	 report += "estimate-table-readers-mem: "+l+"\n";
	 mStateTrieDB->GetProperty("rocksdb.block-cache-usage", &l);
	 report += "block-cache-usage: "+l + "\n";
	 mStateTrieDB->GetProperty("rocksdb.cur-size-all-mem-tables", &l);
	 report +="cur-size-all-mem-tables: "+ l + "\n";
	 return report;
 }
 std::mutex CSolidStorage::sStaticInstancesGuardian;
 CSolidStorage * CSolidStorage::sLIVEInstancePointer = nullptr;
 CSolidStorage * CSolidStorage::sTestNetInstancePointer = nullptr;
 CSolidStorage * CSolidStorage::sLIVESandBoxInstancePointer = nullptr;
 CSolidStorage * CSolidStorage::sTestNetSandBoxInstancePointer = nullptr;
 CSolidStorage * CSolidStorage::sLocalTestsInstancePointer = nullptr;

bool CSolidStorage::saveValue(std::vector<uint8_t> key, std::vector<uint8_t> value)
{
	std::unique_lock lock(mDataAccessGuardian);
	if (getAbortAllWrites())
		return true;
	std::string key_s = std::string(key.begin(), key.end());
	std::string value_s = std::string(value.begin(), value.end());
	
	if (mStateTrieDB->Put(rocksdb::WriteOptions(), key_s, value_s).ok())
		return true;
	else return false;
}
// Produce prefix based on type
std::string CSolidStorage::prefixFromType(eLinkType::eLinkType type)
{
	std::string prefix;
	switch (type)
	{
	case eLinkType::receiptHashToBlockID:
		prefix = "rH_B_";
		break;
	case eLinkType::receiptsGUIDtoReceiptsHash:
		prefix = "rG_rH_";
		break;
	case eLinkType::transactionHashToBlockID:
		prefix = "tH_B_";
		break;
	case eLinkType::verifiableHashToBlockID:
		prefix = "vH_B_";
		break;
	case eLinkType::BHeightPKtoBlockHeader:
		prefix = "BHPK_BH_";
		break;
	case eLinkType::PoFIDtoReceiptID:
		prefix = "PoF_rID_";
		break;
	case eLinkType::friendlyIDtoAddr:
		prefix = "FID_ADR_";
		break;
	default:
		prefix = "";
		break;
	}
	return prefix;
}

// Create a stable 32-byte key for the cache from (type + original key bytes)
std::vector<uint8_t> CSolidStorage::makeCacheKey(eLinkType::eLinkType type, const std::vector<uint8_t>& ID) {
	// Combine type and ID
	std::vector<uint8_t> finalKey;
	finalKey.reserve(ID.size() + 1);
	finalKey.push_back(static_cast<uint8_t>(type));
	finalKey.insert(finalKey.end(), ID.begin(), ID.end());

	// If finalKey is not 32 bytes, hash it
	if (finalKey.size() != 32) {
		finalKey = CCryptoFactory::getInstance()->getSHA2_256Vec(finalKey);
		// Now finalKey is exactly 32 bytes
		assert(finalKey.size() == 32);
	}

	return finalKey;
}

void CSolidStorage::insertIntoLinkCache(const std::vector<uint8_t>& key, const std::vector<uint8_t>& value)
{
	std::unique_lock<std::shared_mutex> writeLock(mCacheMutex);

	CacheValue val{ value };
	auto result = mCache.emplace(key, val);
	if (!result.second) {
		// Key already exists; update its value.
		mCache[key] = val;
		// No insertion order change for an update. 
		return;
	}

	// Newly inserted key
	mCacheInsertionOrder.push_back(key);

	// Evict oldest if over capacity
	if (mCache.size() > MAX_CACHE_SIZE) {
		const auto& oldestKey = mCacheInsertionOrder.front();
		mCache.erase(oldestKey);
		mCacheInsertionOrder.pop_front();
	}
}

/// <summary>
/// Stores a Link within the Solid Storage.
/// 
/// Some objecets use their SHA-256 hash value as their identifier.
/// However; some of the objects utilize a hash-independant GUID.
/// The main reason for this is that for some objects an identifier needs to be generated 
/// before the contents and thus the hash-value can be computed. A good example for this are Transaction Receipts.
/// 
/// *Serialized objects are allways stored under their hash value.*
/// 
/// A Link describes a relationship between object's GUID and its hash value.
/// </summary>
/// <param name="ID"></param>
/// <param name="hash"></param>
/// <returns></returns>
bool CSolidStorage::saveLink(std::vector<uint8_t> ID, std::vector<uint8_t> valueP, eLinkType::eLinkType type, bool allowHotStorage) {
	if (getAbortAllWrites())
		return true; // If writes are aborted, pretend success (or return false if desired).

	if (ID.size() < 32 || ID.size() > 35) {
		// Invalid key size: must be between 32 and 35.
		return false;
	}

	std::string prefix = prefixFromType(type);
	std::string dbKey = H_LINK_PREFIX + prefix + getTools()->bytesToString(ID);
	std::string dbValue = getTools()->bytesToString(valueP);

	{
		std::unique_lock<std::shared_mutex> dbLock(mDataAccessGuardian);
		rocksdb::Status s = mStateTrieDB->Put(rocksdb::WriteOptions(), dbKey, dbValue);
		if (!s.ok()) {
			// Failed to write to DB
			return false;
		}
	}

	if (allowHotStorage) {
		// Derive a stable 32-byte cache key
		std::vector<uint8_t> cacheKey = makeCacheKey(type, ID);
		insertIntoLinkCache(cacheKey, valueP);
	}

	return true;
}

/// <summary>
/// Loads link from 
/// </summary>
/// <param name="ID"></param>
/// <param name="hash"></param>
/// <param name="prefix"></param>
/// <returns></returns>
bool CSolidStorage::loadLink(std::vector<uint8_t> keyP, std::vector<uint8_t>& valueP, eLinkType::eLinkType type, bool allowHotStorage) {

	std::vector<uint8_t> cacheKey;

	if (allowHotStorage) {

		// Attempt from cache first
		 cacheKey = makeCacheKey(type, keyP);

		{
			std::shared_lock<std::shared_mutex> readLock(mCacheMutex);
			auto it = mCache.find(cacheKey);
			if (it != mCache.end()) {
				valueP = it->second.data;
				return true;
			}
		}
	}

	// Not in cache or caching not allowed, read from DB
	std::string prefix = prefixFromType(type);
	std::string rawKey = getTools()->bytesToString(keyP);
	std::string dbKey = H_LINK_PREFIX + prefix + rawKey;

	std::string dbValue;
	{
		std::shared_lock<std::shared_mutex> dbLock(mDataAccessGuardian);
		rocksdb::Status s = mStateTrieDB->Get(getOptimizedReadOptions(), dbKey, &dbValue);

		if (!s.ok() || dbValue.empty()) {
			// Not found in DB
			return false;
		}
	}

	valueP = getTools()->stringToBytes(dbValue);

	if (allowHotStorage) {
		// Store in cache for future quick access
	
		insertIntoLinkCache(cacheKey, valueP);
	}

	return true;
}

bool CSolidStorage::addBlockToCache(std::shared_ptr<CBlock> block)
{
	if (mAbortAllWrites)
		return true;
	//assert((size_t)block->getHeader() < 0x100000000000000);
    if (mRecentBlocksCache.size() == mRecentBlocksCacheSize)
	{
         mRecentBlocksCache[0].reset();
        mRecentBlocksCache.erase(mRecentBlocksCache.begin());
	}

    mRecentBlocksCache.push_back(block);

	std::vector<std::vector<uint8_t>> serializedBlocks;

    for (int i = 0; i < mRecentBlocksCache.size(); i++)
	{
        //assert((size_t)mRecentBlocksCache[i]->getHeader() < 0x100000000000000);
		std::vector<uint8_t> packed;
	 assertGN(mRecentBlocksCache[i]->getPackedData(packed));
        serializedBlocks.push_back(packed);
		//CBlock::blockInstantiationResult res;
		//std::shared_ptr<CBlock>blo = CBlock::instantiateBlock(serializedBlocks[i], res, mIsTestDB);
	}
	saveByteVectors(CGlobalSecSettings::getCachedBlocksDBEntryID(),serializedBlocks);

	return true;
}
std::vector<std::shared_ptr<CBlock>> CSolidStorage::getCachedBlocks()
{
	std::vector<std::vector<uint8_t>> serializedBlocks;

    if (mRecentBlocksCache.size() > 0)
        return mRecentBlocksCache;

		serializedBlocks = loadByteVectors(CGlobalSecSettings::getCachedBlocksDBEntryID());
		
	for (int i = 0; i < serializedBlocks.size(); i++)
	{
		eBlockInstantiationResult::eBlockInstantiationResult mResult;
		std::string error;
        mRecentBlocksCache.push_back(CBlock::instantiateBlock(true,serializedBlocks[i],mResult,error, mBlockchainMode));
	 assertGN(mResult == success);
	}


    return mRecentBlocksCache;
}



std::vector<uint8_t> CSolidStorage::getValue(std::string key)
{
	
	if (!getIsSystemAvailable())
	{
		return std::vector<uint8_t>();
	}

	std::string value;
	std::shared_lock lock(mDataAccessGuardian);

	mStateTrieDB->Get(getOptimizedReadOptions(), key, &value);
	return std::vector<uint8_t>(value.begin(), value.end());
}

std::vector<uint8_t> CSolidStorage::getValue(std::vector<uint8_t> key)
{
	return getValue(std::string(key.begin(), key.end()));
}

bool CSolidStorage::saveValue(std::string key, std::string value)
{
	if (!getIsSystemAvailable())
	{
		return false;
	}

	if (getAbortAllWrites())
		return true;
	if (key.size() == 0)
		return false;
   
	std::unique_lock lock(mDataAccessGuardian);

		if (mStateTrieDB->Put(rocksdb::WriteOptions(), key, value).ok())
			return true;
		else
		{
			getTools()->writeLine("There was an error saving data to Cold-Storage..");
			return false;
		}
}

bool CSolidStorage::saveValue(std::string key, std::vector<uint8_t> value)
{
	if (getAbortAllWrites())
		return true;
	if (key.size() == 0)
		return false;
	
	std::string value_s = std::string(value.begin(), value.end());
	std::unique_lock lock(mDataAccessGuardian);

	if (mStateTrieDB->Put(rocksdb::WriteOptions(), key, value_s).ok())
		return true;
	else return false;

}

bool CSolidStorage::saveByteVectors(std::string key, const std::vector<std::vector<uint8_t>> & vectors)
{
	if (mAbortAllWrites)
		return true;
	std::vector<uint8_t> key_b = std::vector<uint8_t>(key.begin(), key.end());
	saveByteVectors(key_b, vectors);
	return true;
}

bool CSolidStorage::saveByteVectors(std::vector<uint8_t> key, const std::vector<std::vector<uint8_t>> &vectors)
{
	if (!getIsSystemAvailable())
		return false;
	
	if (mAbortAllWrites)
		return true;
		
	saveValue(key, getTools()->BERVector(vectors, std::thread::hardware_concurrency(), false));

	return true;
}

std::vector<std::vector<uint8_t>> CSolidStorage::loadByteVectors(std::string key)
{
	return loadByteVectors(std::vector<uint8_t>(key.begin(), key.end()));
}

std::vector<std::vector<uint8_t>> CSolidStorage::loadByteVectors(std::vector<uint8_t> key)
{
	std::vector<std::vector<uint8_t>> toRet;
	std::string key_s = std::string(key.begin(), key.end());
	std::string value;

	{
		std::shared_lock lock(mDataAccessGuardian);
		mStateTrieDB->Get(getOptimizedReadOptions(), key_s, &value);
	}

	if (value.size() == 0)
		return toRet;
	std::vector<uint8_t> loadedBytes = std::vector<uint8_t>(value.begin(), value.end());

	Botan::BER_Decoder dec1 = Botan::BER_Decoder(loadedBytes).start_cons(Botan::ASN1_Tag::SEQUENCE);
	while(dec1.more_items())
	{
		Botan::BER_Object obj;

		obj = dec1.get_next_object();
		if (obj.type_tag == Botan::ASN1_Tag::OCTET_STRING)
		{
			toRet.push_back( Botan::unlock(obj.value));
		}
		else if (obj.type_tag == Botan::NULL_TAG)
		{
			toRet.push_back(std::vector<uint8_t>(0));
		}

	}
	dec1.end_cons();
	return toRet;
}

bool CSolidStorage::saveNode(CTrieNode * node, std::string prefix )
{
	if (mAbortAllWrites)
		return true;
	//NOTE: this has been refactored to account for the fact that the Effective hash of some of the nodes (ex. StateDomains)
	//does NOT depand on all of the packed bytes. ie.e as far as the Perspective of a TrieDB is concerned.
	//the nodes within a TrieDB include now Relative/Implicit hashes of the nodes below them.
	//for some nodes Relative Hash == Implicit/Objective hash i.e. they are the same.
	std::vector<uint8_t> id = node->getHash();
	std::vector<uint8_t> BER_TrieNode = node->getPackedData();
	return saveNode(id,BER_TrieNode,prefix);
}

bool CSolidStorage::saveBlock(std::vector<uint8_t> BER_block, std::vector<uint8_t> hash)
{
	if (mAbortAllWrites)
		return true;
	std::string key = std::string(hash.begin(), hash.end());
	std::string value = std::string(BER_block.begin(), BER_block.end());

	key = H_BLOCK_PREFIX + key;
	std::unique_lock lock(mDataAccessGuardian);

	if (mStateTrieDB->Put(rocksdb::WriteOptions(), key, value).ok())
	return true;
	else return false;
}

/// <summary>
/// Saves the Data Blok to cold storage.
/// Warning: The order of transactions will change.
/// </summary>
/// <param name="block"></param>
/// <returns></returns>
bool CSolidStorage::saveBlock(std::shared_ptr<CBlock> block)
{
	std::vector<uint8_t> temp;
	if (getAbortAllWrites())
		return true;
	if (block->getHeader() == NULL )
		return false;
	getTools()->writeLine("Saving the block to Cold Storage", true, true, eViewState::unspecified, "Solid Storage");
	std::vector<uint8_t> packedData;
	if (!block->getPackedData(packedData))
		return false;
	if (block->getHeader() == nullptr)
		return false;

	if(!block->getHeader()->isKeyBlock())
	block->getHeader()->forceRootNodesStorage();

	eBlockInstantiationResult::eBlockInstantiationResult res;
	std::string errorInfo;
	
 assertGN(block->getHeader()->getPackedData(temp));
	bool mResult = saveBlock(packedData, CCryptoFactory::getInstance()->getSHA2_256Vec(temp));

	if(mResult && block->getHeader()!=nullptr)
		block->getHeader()->markLocalAvailability();

	return mResult;

}

std::vector<uint8_t> CSolidStorage::getBlockDataByHash(std::vector<uint8_t> hash)
{
	std::string key_s = H_BLOCK_PREFIX + std::string(hash.begin(), hash.end());
	std::string value;

	{
		std::shared_lock lock(mDataAccessGuardian);
		mStateTrieDB->Get(getOptimizedReadOptions(), key_s, &value);
	}

	return getTools()->stringToBytes(value);
}




std::shared_ptr<CBlock> CSolidStorage::getBlockByHash(std::vector<uint8_t>  hash,
	eBlockInstantiationResult::eBlockInstantiationResult& mResult,
	bool instantiateTries,  
	const uint64_t& bytesCount,
	bool loadEffectiveData)
{
	
	std::shared_ptr<CBlock> toRet = nullptr;
	std::string key_s = H_BLOCK_PREFIX + std::string(hash.begin(), hash.end());

	std::vector<uint8_t> bytes = getBlockDataByHash(hash);
	const_cast<uint64_t&>(bytesCount) = bytes.size();

	if (bytes.size() == 0)
	{
		mResult = eBlockInstantiationResult::eBlockInstantiationResult::blockDataUnavailableInCS;
		return nullptr;
	}

	std::string errorInfo;
	//std::vector<uint8_t> h = mCf.getSHA3_256Vec(bytes);
	toRet = CBlock::instantiateBlock(instantiateTries,bytes,mResult,errorInfo, mBlockchainMode);
	if(toRet!=nullptr)
	toRet->getHeader()->markLocalAvailability();  
	else
	{
		getTools()->writeLine("Error instantiating block " + getTools()->base58CheckEncode(hash));
	}

	// Effective Block Data Loading - BEGIN
	// [ Rationale ]: Load effective reward and payment amounts for the block from cold storage
	//                if requested. These values represent the actual values that were
	//                used during block processing, as opposed to the proposed values present within of block header serialized structures valid before hard forks and
	//				  most importantly  - before consective Checkpoints were being introduced.
	if (loadEffectiveData)
	{
		std::shared_ptr<CBlockHeader> bh = toRet->getHeader();
		if (bh)
		{
			// Load effective total block reward - BEGIN
			BigInt totalRewardEffective;
			if (getTotalBlockRewardEffective(hash, totalRewardEffective))
			{
				// Value found in cold storage - update header with effective value
				bh->setTotalBlockReward(totalRewardEffective, true);
			}
			// Load effective total block reward - END

			// Load effective paid to miner amount - BEGIN 
			BigInt paidToMinerEffective;
			if (getPaidToMinerEffective(hash, paidToMinerEffective))
			{
				// Value found in cold storage - update header with effective value
				bh->setPaidToMiner(paidToMinerEffective, true);
			}
			// Load effective paid to miner amount - END
		}
	}
	// Effective Block Data Loading - END
	return toRet;
}

/// <summary>
/// Retrieves the effective total block reward for a given block ID from solid storage.
/// </summary>
/// <param name="blockID">32-byte block identifier</param>
/// <param name="totalReward">Reference to store the retrieved reward value. Set to 0 if not found.</param>
/// <returns>True if value was found in storage, false otherwise</returns>
bool CSolidStorage::getTotalBlockRewardEffective(const std::vector<uint8_t>&blockID, BigInt & totalReward)
{
	//Validation - BEGIN
	if (blockID.size() != 32)
	{
		totalReward = BigInt(0);
		return false;
	}
	//Validation - END

	std::string key = "ETR_" + getTools()->base58CheckEncode(blockID);
	std::vector<uint8_t> serializedValue = getValue(key);

	if (!serializedValue.empty())
	{
		totalReward = getTools()->BytesToBigInt(serializedValue);
		return true;
	}

	totalReward = BigInt(0);
	return false;
}

/// <summary>
/// Stores the effective total block reward for a given block ID in solid storage.
/// </summary>
/// <param name="totalReward">The total block reward value to store</param>
/// <param name="blockID">32-byte block identifier</param>
/// <returns>True if storage was successful, false otherwise</returns>
bool CSolidStorage::setTotalBlockRewardEffective(const BigInt& totalReward, const std::vector<uint8_t>& blockID)
{
	//Validation - BEGIN
	if (blockID.size() != 32)
		return false;
	//Validation - END


	std::string key = "ETR_" + getTools()->base58CheckEncode(blockID);
	std::vector<uint8_t> serializedValue = getTools()->BigIntToBytes(totalReward);

	return saveValue(key, serializedValue);
}
/// <summary>
/// Retrieves the effective paid to miner amount for a given block ID from solid storage.
/// </summary>
/// <param name="blockID">32-byte block identifier</param>
/// <param name="paidToMiner">Reference to store the retrieved amount. Set to 0 if not found.</param>
/// <returns>True if value was found in storage, false otherwise</returns>
bool CSolidStorage::getPaidToMinerEffective(const std::vector<uint8_t>& blockID, BigInt& paidToMiner)
{
	//Validation - BEGIN
	if (blockID.size() != 32)
	{
		paidToMiner = BigInt(0);
		return false;
	}
	//Validation - END

	std::string key = "EPM_" + getTools()->base58CheckEncode(blockID);
	std::vector<uint8_t> serializedValue = getValue(key);

	if (!serializedValue.empty())
	{
		paidToMiner = getTools()->BytesToBigInt(serializedValue);
		return true;
	}

	paidToMiner = BigInt(0);
	return false;
}

/// <summary>
/// Stores the effective paid to miner amount for a given block ID in solid storage.
/// </summary>
/// <param name="paidToMiner">The amount paid to miner to store</param>
/// <param name="blockID">32-byte block identifier</param>
/// <returns>True if storage was successful, false otherwise</returns>
bool CSolidStorage::setPaidToMinerEffective(const BigInt& paidToMiner, const std::vector<uint8_t>& blockID)
{
	//Validation - BEGIN
	if (blockID.size() != 32)
		return false;
	//Validation - END


	std::string key = "EPM_" + getTools()->base58CheckEncode(blockID);
	std::vector<uint8_t> serializedValue = getTools()->BigIntToBytes(paidToMiner);

	return saveValue(key, serializedValue);
}

//
bool CSolidStorage::checkIfBlockInStorage(std::vector<uint8_t> hash)
{
	std::string key_s = H_BLOCK_PREFIX + std::string(hash.begin(), hash.end());
	std::string value;
	{
		std::shared_lock lock(mDataAccessGuardian);
		mStateTrieDB->Get(getOptimizedReadOptions(), key_s, &value);
	}

	if (value.size() == 0)
		return true;
	return false;
}


bool CSolidStorage::closeAllDBs(bool freeMemory)
{//LIVE-NET
	 

	try {

		//disable data-storage sub-system before pluggin 'em out.
		getInstance(eBlockchainMode::LIVE)->setIsSystemAvailable(false);
		getInstance(eBlockchainMode::TestNet)->setIsSystemAvailable(false);
		getInstance(eBlockchainMode::TestNetSandBox)->setIsSystemAvailable(false);
		getInstance(eBlockchainMode::LIVESandBox)->setIsSystemAvailable(false);

		if (sLIVEBlockchainDB != nullptr)
			sLIVEBlockchainDB->Close();
		if (sLIVEStateTrieDB != nullptr)
			sLIVEStateTrieDB->Close();

	
		if (sTestNetBlockchainDB != nullptr)
			sTestNetBlockchainDB->Close();

		if (sTestNetStateTrieDB != nullptr)
			sTestNetStateTrieDB->Close();

		if (sTestNetStagedStateTrieDB != nullptr)
			sTestNetStagedStateTrieDB->Close();



		if (sLocalTestBlockchainDB != nullptr)
			sLocalTestBlockchainDB->Close();
		if (sLocalTestStateTrieDB != nullptr)
			sLocalTestStateTrieDB->Close();
		if (sLocalTestStagedStateTrieDB != nullptr)
			sLocalTestStagedStateTrieDB->Close();
		if (sLIVEStagedStateTrieDB != nullptr)
			sLIVEStagedStateTrieDB->Close();

		if (freeMemory)
		{
			delete sLIVEBlockchainDB;
			sLIVEBlockchainDB = nullptr;
			delete sLIVEStateTrieDB;
			sLIVEStateTrieDB = nullptr;
			delete sTestNetBlockchainDB;
			sTestNetBlockchainDB = nullptr;
			delete sTestNetStateTrieDB;
			sTestNetStateTrieDB = nullptr;
			delete sTestNetStagedStateTrieDB;
			sTestNetStagedStateTrieDB = nullptr;
			delete sLocalTestBlockchainDB;
			sLocalTestBlockchainDB = nullptr;
			delete sLocalTestStateTrieDB;
			sLocalTestStateTrieDB = nullptr;
			delete sLocalTestStagedStateTrieDB;
			sLocalTestStagedStateTrieDB = nullptr;
			delete sLIVEStagedStateTrieDB;
			sLIVEStagedStateTrieDB = nullptr;
		}
	}
	catch (...)
	{
		return false;
	}
	return true;
}
