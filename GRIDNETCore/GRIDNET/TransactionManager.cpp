#include "CGlobalSecSettings.h"
#include "TransactionManager.h"
#include "block.h"
#include "BlockchainManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <set>
#include "CryptoFactory.h"
#include "WorkManager.h"
#include "CWorkUltimium.h"
#include "Receipt.h"
#include "block.h"
#include "settings.h"
#include "scriptengine.h"
#include "triedb.h"
#include "StateDomain.h"
#include "transaction.h"
#include "CStateDomainManager.h"
#include "keyChain.h"
#include "Verifiable.h"
#include "Verifier.h"
#include "CWorkC.h"
#include "DataConcatenator.h"
#include "LinkContainer.h"
#include "AsyncMemCleaner.h"
#include "DTI.h"
#include "conversation.h"
#include "StatusBarHub.h"
#include "GRIDNET.h"
#include "NetworkManager.h"
#include "NetMsg.h"
#include "ObjectThrottler.h"
#include "conversationState.h"
#include "BreakpointFactory.hpp"
#include "Breakpoint.hpp"
#include "TransactionDesc.hpp"
#include "GridScriptParser.h"
#include "GridScriptCompiler.h"
#include "SynchronizedLocker.hpp"
#include "FinalAction.h"
bool CTransactionManager::removeTransactionFromMemPool(std::vector<uint8_t> transID)
{
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	bool found = false;
	if (transID.size() == 0)
		return false;
	for (int i = 0; i < mMemPool.size(); i++)
	{
		if (mMemPool[i]->getType() != eNodeType::leafNode || mMemPool[i]->getSubType() != eNodeSubType::transaction)
			continue;
		if (std::memcmp(std::static_pointer_cast<CTransaction>(mMemPool[i])->getHash().data(), transID.data(), transID.size()) == 0)
		{
			mMemPool.erase(mMemPool.begin()+i);
			found = true;

		}
	}
	return found;
	
}


void CTransactionManager::pingLastTimeMemPoolAdvertised()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mLastTimeMemPoolAdvertised = std::time(0);
}

uint64_t CTransactionManager::getLastTimeMemPoolAdvertised()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mLastTimeMemPoolAdvertised;
}


/**
* @brief Removes all databases associated with current flow object from stack
*
* Similar to revertTX but without state reversion. Removes all DBs associated
* with the current object from tracking stacks. Used for cleanup when we
* don't need to revert the state.
*
* [IMPORTANT]:
* - Only removes DBs matching current flow object ID
* - Decrements stack depth counters appropriately
* - Stops when encountering DB from different object
* - Expects stack depth to match actual steps performed
*
* Thread-safe: Protected by mGuardian and mFlowStateDBGuardian
*
* @return true if operation succeeded, false otherwise
*/
bool CTransactionManager::popCurrentTXDBs()
{
	// Lock Acquisition - BEGIN
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(mFlowStateDBGuardian);
	// Lock Acquisition - END

	// Local Variables - BEGIN
	std::vector<uint8_t> currentObjectID = getCurrentFlowObjectID();
	uint64_t stepsPerformed = 0;
	uint64_t expectedTXDBsCount = mInFlowTXDBStackDepth;
	// Local Variables - END

	// Validation - BEGIN
	assertGN(getIsInFlow());
	// Validation - END

	// Operational Logic - BEGIN

	incTXsCleanedCount();

	while (mInFlowTXDBStackDepth > 0)
	{
		assertGN(!mInFlowInterDBs.empty());

		// Get the top DB's associated object ID
		std::vector<uint8_t> topDBObjectID = mInFlowInterDBs.back()->getAssociatedObjectID();

		// If we've reached a DB associated with a different object, we're done
		if (topDBObjectID != currentObjectID)
		{
			break;
		}

		// Pop one DB
		if (!popDB(1))
		{
			return false;
		}
		stepsPerformed++;
	}
	// Operational Logic - END

	// Final Validation - BEGIN
	assertGN(stepsPerformed == expectedTXDBsCount);
	assertGN(mInFlowTXDBStackDepth == 0 ||
		mInFlowInterDBs.back()->getAssociatedObjectID() != currentObjectID);
	// Final Validation - END

	return true;
}

/**
 * @brief Removes databases from the reversible DB stack without state reversion
 *
 * Unlike stepBack, this method only removes the DBs from tracking stacks
 * without attempting to revert the current state to any previous perspective.
 * Used when we want to clean up tracking without affecting current state.
 *
 * [IMPORTANT]: This affects both tracking vectors:
 * - mInFlowInterDBs
 * - mInFlowInterPerspectives
 * And decrements appropriate stack depth counters.
 *
 * Thread-safe: Protected by mGuardian and mFlowStateDBGuardian
 *
 * @param nrOfDBs Number of databases to pop from the stack
 * @return true if operation succeeded, false if requested count exceeds stack size
 */
bool CTransactionManager::popDB(size_t nrOfDBs)
{
	// Early Return - BEGIN
	if (!nrOfDBs)
	{
		return true;
	}
	// Early Return - END

	// Lock Acquisition - BEGIN
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(mFlowStateDBGuardian);
	// Lock Acquisition - END

	// Validation - BEGIN
	assertGN(getIsInFlow());
	if (nrOfDBs > mInFlowInterPerspectives.size() || nrOfDBs > mInFlowInterDBs.size())
		return false;
	// Validation - END

	// Local Variables - BEGIN
	size_t lastIndex;
	// Local Variables - END

	// Operational Logic - BEGIN
	for (size_t i = 0; i < nrOfDBs; i++)
	{
		lastIndex = mInFlowInterPerspectives.size() - 1;

		// State Cleanup - BEGIN
		CAsyncMemCleaner::cleanIt(reinterpret_cast<void**>(&mInFlowInterDBs[lastIndex]));
		mInFlowInterDBs.pop_back();

		if (getInFlowTXDBStackDepth())
		{
			decInFlowTXDBStackDepth();
		}
		mInFlowInterPerspectives.pop_back();
		// State Cleanup - END
	}
	// Operational Logic - END

	return true;
}

/**
* @brief Pushes a new State Flow Database onto the reversible DB stack
*
* Adds a new State Trie DB to tracking stacks, managing size limits:
* - Removes oldest entries if size exceeds mAllowedInterFlowSteps
* - Updates both DB and perspective tracking vectors
* - Optionally increments TX stack depth counters
*
* Transaction/Object ID Management:
* - By default, uses getCurrentFlowObjectID() as the DB's associated object ID
* - For transaction DBs (isTXDB=true), can override with explicitObjectID if provided
* - Sets the assigned object ID on the DB via setAssociatedObjectID()
*
* [IMPORTANT]: If isTXDB is true, this will increment both:
* - Current transaction depth (mInFlowTXDBStackDepth)
* - Total flow depth (mInFlowTotalTXDBStackDepth)
*
* Thread-safe: Protected by mFieldsGuardian
*
* @param db Pointer to DB to add (must not be NULL)
* @param isTXDB If true, indicates this is a transaction-related DB snapshot
* @param explicitObjectID Optional override for the DB's associated object ID when isTXDB=true
*
* @pre db must not be NULL
* @note The method will assert if db is NULL
* @note explicitObjectID is only used when isTXDB is true
*/
void CTransactionManager::pushDB(CTrieDB* db, bool isTXDB, const std::vector<uint8_t>& explicitObjectID)
{
	// Input Validation - BEGIN
	if (db == NULL)
		assertGN(false);
	// Input Validation - END

	// Object ID - BEGIN
	std::vector<uint8_t> objectID = getCurrentFlowObjectID();

	if (isTXDB && !explicitObjectID.empty())
	{
		objectID = explicitObjectID;
	}

	db->setAssociatedObjectID(objectID);
	// Object ID - END
	
	// Local Variables - BEGIN
	uint32_t i = 0;
	// Local Variables - END

	// Stack Size Management - BEGIN
	while (mInFlowInterDBs.size() >= mAllowedInterFlowSteps)
	{
		delete mInFlowInterDBs[i];
		mInFlowInterDBs.erase(mInFlowInterDBs.begin() + i);
		i++;
	}

	while (mInFlowInterPerspectives.size() >= mAllowedInterFlowSteps)
	{
		mInFlowInterPerspectives.erase(mInFlowInterPerspectives.begin() + i);
		i++;
	}
	// Stack Size Management - END

	// State Update - BEGIN
	mInFlowInterPerspectives.push_back(db->getPerspective());
	mInFlowInterDBs.push_back(db);

	if (isTXDB)
	{
		incInFlowTXDBStackDepth();
	}
	// State Update - END
}

/*

bool CTransactionManager::revertTX()
{
	uint64_t stepBackCount = 0;  // Tracks total number of reverted steps

	// Continue reverting while there are states in the stack
	assertGN(stepBack(mInFlowTXDBStackDepth));
	assertGN(!mInFlowTXDBStackDepth);

	return true; 
}*/

/**
 * @brief Reverts all changes associated with the current transaction object
 *
 * Reverts state changes by stepping back through the state stack until reaching
 * a state not associated with the current transaction object ID. Uses the DB's
 * associated object ID to determine the boundary of the current transaction,
 * ensuring complete reversion of all changes made by this transaction.
 *
 * @note This differs from basic step counting as it uses object IDs to determine
 *       transaction boundaries, making it more robust against nested transactions
 *       and complex state changes
 *
 * @return true if reversion was successful, false otherwise
 */
bool CTransactionManager::revertTX()
{
	// Local Variables - BEGIN
	std::vector<uint8_t> currentObjectID = getCurrentFlowObjectID();
	uint64_t stepsPerformed = 0;
	uint64_t expectedTXDBsCount = mInFlowTXDBStackDepth;
	// Local Variables - END

	// Operational Logic - BEGIN
	incTXsRevertedCount();
	while (mInFlowTXDBStackDepth > 0)
	{
		// Get the top DB's associated object ID
		assertGN(!mInFlowInterDBs.empty());
		std::vector<uint8_t> topDBObjectID = mInFlowInterDBs.back()->getAssociatedObjectID();

		// If we've reached a DB associated with a different (or with none) object, we're done
		if (topDBObjectID != currentObjectID)
		{
			break;
		}

		// Step back one state
		if (!stepBack(1))
		{
			return false;
		}
		stepsPerformed++;
	}

	assertGN(stepsPerformed == expectedTXDBsCount); // the actual number of steps taken needs equal expectedTXDBsCount
													// todo: optimize towards A priori defined cumulative step-backs after validation

	// Verify we've properly unwound the stack
	assertGN(mInFlowTXDBStackDepth == 0 ||
		mInFlowInterDBs.back()->getAssociatedObjectID() != currentObjectID);

	// Operational Logic - END
	return true;
}


std::string CTransactionManager::genThreadName(bool isMiningThread , bool keyBlocks)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mNamePrefixGuardian);
	if(!isMiningThread)
	return  std::string( getIsRemoteTerminal()?"REMOTE ":"")+ "Transaction Manager ["+mNamePrefix+" " +tools->transactionManagerModeToStr(mMode) +", BM:"+ tools->blockchainmodeToString(getBlockchainMode()) + "] " + std::string(getDoBlockFormation()?"BF-ON":"BF-OFF");
else
return  "BlockFactory - "+std::string(keyBlocks?"Keys":"Data")+" [" + mNamePrefix + " " + tools->transactionManagerModeToStr(mMode) + ", BM:" + tools->blockchainmodeToString(getBlockchainMode()) + "] " + std::string(getDoBlockFormation() ? "BF-ON" : "BF-OFF");
}
bool CTransactionManager::addLinkToFlow(std::shared_ptr<CLinkContainer> link)
{
	if (!getIsInFlow())
		return false;
	std::lock_guard<std::mutex> lock(mInFlowLinksGuardian);
	mInFlowLinks.push_back(link);
	return true;
}
/// <summary>
/// checks if a link with a given key-value is already present within the Flow.
/// </summary>
/// <param name="link"></param>
/// <returns></returns>
bool  CTransactionManager::isLinkInFlow(std::shared_ptr<CLinkContainer> link)
{
	std::shared_ptr<CTools> tools = getTools();
	if (!getIsInFlow())
		return false;
	std::lock_guard<std::mutex> lock(mInFlowLinksGuardian);
	for (uint64_t i = 0; i < mInFlowLinks.size(); i++)
	{
		if (tools->compareByteVectors(mInFlowLinks[i]->getKey(), link->getKey()) && mInFlowLinks[i]->getType() == link->getType())
			return true;
	}
	
	return false;
}
void CTransactionManager::clearInFlowLinks()
{
	std::lock_guard<std::mutex> lock(mInFlowLinksGuardian);
	mInFlowLinks.clear();
}

uint64_t CTransactionManager::getLastControllerLoopRun()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mLastControllerLoopRun;
}

uint64_t CTransactionManager::getPowCustomBarID()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mPoWCustomBarID;
}

void CTransactionManager::incInFlowTXDBStackDepth()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mInFlowTXDBStackDepth++;
	mInFlowTotalTXDBStackDepth++;
}

void CTransactionManager::decInFlowTXDBStackDepth()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	assertGN(mInFlowTotalTXDBStackDepth > 0);// this should never happen; we should never attempt to call in such a case

	if (mInFlowTXDBStackDepth > 0) {
		mInFlowTXDBStackDepth--;
	}

	if (mInFlowTotalTXDBStackDepth > 0)
	{
		mInFlowTotalTXDBStackDepth--;
	}
}

void CTransactionManager::resetCurrentTXDBStackDepth()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mInFlowTXDBStackDepth = 0;
}

uint64_t CTransactionManager::getInFlowTXDBStackDepth(bool getTotalDepth) {
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return getTotalDepth ? mInFlowTotalTXDBStackDepth : mInFlowTXDBStackDepth;
}

void CTransactionManager::pingtLastControllerLoopRun()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mLastControllerLoopRun = std::time(0);
}

// Thread Health Monitoring Implementation - BEGIN

void CTransactionManager::pingKeyBlockThreadHeartbeat()
{
	mKeyBlockThreadHeartbeat.store(std::time(0), std::memory_order_release);
}

void CTransactionManager::pingDataBlockThreadHeartbeat()
{
	mDataBlockThreadHeartbeat.store(std::time(0), std::memory_order_release);
}

bool CTransactionManager::isKeyBlockThreadAlive()
{
	uint64_t lastHeartbeat = mKeyBlockThreadHeartbeat.load(std::memory_order_acquire);
	if (lastHeartbeat == 0) return false; // Never started
	uint64_t currentTime = std::time(0);
	return (currentTime - lastHeartbeat) < THREAD_HEARTBEAT_TIMEOUT_SECONDS;
}

bool CTransactionManager::isDataBlockThreadAlive()
{
	uint64_t lastHeartbeat = mDataBlockThreadHeartbeat.load(std::memory_order_acquire);
	if (lastHeartbeat == 0) return false; // Never started
	uint64_t currentTime = std::time(0);
	return (currentTime - lastHeartbeat) < THREAD_HEARTBEAT_TIMEOUT_SECONDS;
}

// Public accessors for thread health status
bool CTransactionManager::getIsKeyBlockThreadAlive()
{
	return isKeyBlockThreadAlive();
}

bool CTransactionManager::getIsDataBlockThreadAlive()
{
	return isDataBlockThreadAlive();
}

void CTransactionManager::checkAndRestartBlockFormationThreads()
{
	if (!getDoBlockFormation()) return; // Block formation disabled, nothing to check

	std::shared_ptr<CTools> tools = getTools();
	bool keyThreadAlive = isKeyBlockThreadAlive();
	bool dataThreadAlive = isDataBlockThreadAlive();

	// SECURITY: If one thread is alive but the other is dead, this could indicate
	// a thread termination attack attempting to disable data block formation
	// while keeping key block formation running (or vice versa).

	if (keyThreadAlive && !dataThreadAlive)
	{
		// DATA BLOCK THREAD DIED - CRITICAL SECURITY EVENT
		tools->logEvent("SECURITY WARNING: Data block formation thread died while key block thread is still running! Restarting data block thread..",
			eLogEntryCategory::localSystem, 1, eLogEntryType::warning, eColor::alertError);

		// Try to join the dead thread first (cleanup)
		if (mRegBlockFactoryT.joinable())
		{
			// Give it a short time to join, then detach if needed
			mRegBlockFactoryT.join();
		}

		// Restart the data block formation thread
		mDataBlockThreadHeartbeat.store(0, std::memory_order_release); // Reset heartbeat
		mRegBlockFactoryT = std::thread(&CTransactionManager::dataBlockMinerThread, this);
		tools->logEvent("Data block formation thread restarted successfully.",
			eLogEntryCategory::localSystem, 5, eLogEntryType::notification, eColor::lightGreen);
	}
	else if (!keyThreadAlive && dataThreadAlive)
	{
		// KEY BLOCK THREAD DIED - CRITICAL SECURITY EVENT
		tools->logEvent("SECURITY WARNING: Key block formation thread died while data block thread is still running! Restarting key block thread..",
			eLogEntryCategory::localSystem, 1, eLogEntryType::warning, eColor::alertError);

		// Try to join the dead thread first (cleanup)
		if (mKeyBlockFactoryT.joinable())
		{
			mKeyBlockFactoryT.join();
		}

		// Restart the key block formation thread
		mKeyBlockThreadHeartbeat.store(0, std::memory_order_release); // Reset heartbeat
		mKeyBlockFactoryT = std::thread(&CTransactionManager::keyBlockMinerThread, this);
		tools->logEvent("Key block formation thread restarted successfully.",
			eLogEntryCategory::localSystem, 5, eLogEntryType::notification, eColor::lightGreen);
	}
	else if (!keyThreadAlive && !dataThreadAlive && getDoBlockFormation())
	{
		// BOTH THREADS DIED - Restart both
		tools->logEvent("SECURITY WARNING: Both block formation threads died! Restarting both..",
			eLogEntryCategory::localSystem, 1, eLogEntryType::warning, eColor::alertError);

		if (mKeyBlockFactoryT.joinable())
			mKeyBlockFactoryT.join();
		if (mRegBlockFactoryT.joinable())
			mRegBlockFactoryT.join();

		mKeyBlockThreadHeartbeat.store(0, std::memory_order_release);
		mDataBlockThreadHeartbeat.store(0, std::memory_order_release);
		mKeyBlockFactoryT = std::thread(&CTransactionManager::keyBlockMinerThread, this);
		mRegBlockFactoryT = std::thread(&CTransactionManager::dataBlockMinerThread, this);
		tools->logEvent("Both block formation threads restarted successfully.",
			eLogEntryCategory::localSystem, 5, eLogEntryType::notification, eColor::lightGreen);
	}
}

// Thread Health Monitoring Implementation - END

void CTransactionManager::setNamePrefix(std::string name)
{
	mNamePrefix = name;
}
/// <summary>
/// The controller thread is responsible for management of the two Key-Block and Reg-Block mining threads.
/// It collects the ready-made blocks and dispatches them to Blockchain Manager for further processing.
/// </summary>
void CTransactionManager::mControllerThreadF()
{
	std::string tName = genThreadName();
	std::shared_ptr<CTools> tools = getTools();
	tools->SetThreadName(tName.data());
	/* 
	Note: Key-Block is always the one containing an answer to a crypto-puzzle.

	Threads:
		(Thread 0) mControllerThreadF
		1) Controls the bellow.

		(Thread 1) CmRegBlockFactoryT Thread:
		1) Collect transactions from Network Factory or from the Local Terminal
		2) Verify transactions and form Blocks (state representations)
		3) Pass the blocks over to the Blockchain Manager
		
		(Thread 2) mKeyBlockFactoryT Thread:
		1) always keep on attempting to become a Leader for next round
		2) keep on mining on the block-proposal, while collecting Reg-Blocks produced by the currently known Leader.

		 [ DISCUSSION BEGIN ]

		The block-proposal contains a Pub-Key PK, with which, the block will be signed afterwards.

		The final block-proposal contains: signed_SK([PoW,STATE_DATA,PK,BLOCK_HEIGHT,PARENT_ID]).

		Each Key-Block *ALSO* contains PARENT_KEY_BLOCK_HEADER_HASH in addition to PARENT_ID. (needed for PoW verification)

		PoW = POW(PARENT_KEY_BLOCK_HEADER_HASH | CURRENT_HEADER_HASH | PK| BLOCK_HEIGHT) 
		PARENT_ID might be either a Key-Block or a Regular-Block.

		Note 1 (PoW): PoW does *NOT* take PARENT_ID.
		Thus, reference to a parent block (PARENT_ID) *IS* malleable (from SK_1 owner's perspective)
		until next block. (See Note 2). PoW takes PARENT_KEY_BLOCK_HEADER_HASH as an argument.
		PARENT_KEY_BLOCK_HEADER_HASH corresponds to the closest key-block. It MIGHT NOT correspond to a direct parent.
		PoWing PK_1 prevents an adversary from modifying the malleable PARENT_ID field.
		The entire block proposal (header hash) will be PoWed by the following block / consecutive leader (PoW takes PARENT_KEY_BLOCK_HEADER_HASH).

		Note 2 (Proof-Of-Fraud): in case any node notices another block-proposal *at the same blockchain-height* signed with the same PK/SK key/pair  => 
		a Proof-of-Fraud would be issued. That's to thwart miners presenting different blocks with different reg-blocks set as parent
	    to different parts of the network. note: PARENT_ID is *NOT* PoWed at this stage INDEED. (See Note 4)
		Each node  keep a list of recent [PK, block_hash, height]  encountered triplets.

		We are assured that the parent-block was selected by the agent who solved the crypto-puzzle
		(since PK_1 *IS* within the PoWed part of the current PROPOSAL)

		Leader can spend mined coins only after a safety period (time cap for reward amount set within the PROPOSAL)
		
		Note 3: In contrast with 'Bitcoin-NG' here PoW takes both the CURRENT_HEADER_HASH and PARENT_KEY_BLOCK_HEADER_HASH as arguments.
		That is to prevent  attacks where an adversary was able to pick-up an arbitrary reg-block and so buy more mining time
		at the cost of forsaking some of the regular blocks produced by previous leader. Here mining the current proposal
		is INDEPENDAT from regular-blocks produced by the CURRENT leader, thus crypto puzzle is known and constant throughout the entire
		lottery.
		 [ DISCUSSION END ]

		3) on success: within the block-proposal => update the parent to be the latest known Reg-Block
		OR key-block (if no consecutive reg-blocks were encountered).
		
		Note: For a reg-block parent reference to a key-block containing the recent crypto-puzzle is IMPLICIT. 
		i.e. there's no need to store it within the block-proposal.
		That's because there's only ONE key-block per a sequence of regular-blocks.

		Reference to it would be a waste of storage and redundant on verification the latest key-block would be retrieved.
*/
	bool wasPaused = false;
	bool threadNameChanged = false;
	bool wasFormingBlocks = getDoBlockFormation();
	uint64_t lastTimeThreadNameGen = 0;
	uint64_t time = 0;
	if (!getIsReady())
	{
		tools->writeLine("Waiting for myself to become ready..");
		while (!getIsReady())
		{
			if (getRequestedStatusChange() == eManagerStatus::eManagerStatus::stopped)
				return;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		tools->writeLine("I'm ready, commencing further..");
	}

	if (!getBlockchainManager()->getIsReady())
	{
		tools->writeLine("Waiting for Blockchain Manager to become ready..");
		while (!getBlockchainManager()->getIsReady())
		{
			if (getRequestedStatusChange() == eManagerStatus::eManagerStatus::stopped)
				return;
			tName = genThreadName();
			tools->SetThreadName(tName.data());
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
		}
		tools->writeLine("Blockchain Manager ready, commencing further..");
	}

	if (mStatus != eManagerStatus::eManagerStatus::running)
			setStatus(eManagerStatus::eManagerStatus::running);

	if (getMode() == eTransactionsManagerMode::FormationFlow)
	{
		if (!getIsRemoteTerminal())
		{
			tools->writeLine("I'm ready; Firing up block-formation sub-systems! (Key-Blocks:" + std::string(mDoBlockFormation ? "Enabled" : "Disabled")
				+ " Data-Blocks:" + std::string(mDoBlockFormation ? "Enabled" : "Disabled") + ")");

			if (getDoBlockFormation())
				mKeyBlockFactoryT = std::thread(&CTransactionManager::keyBlockMinerThread, this);
			if (getDoBlockFormation())
				mRegBlockFactoryT = std::thread(&CTransactionManager::dataBlockMinerThread, this);
		}
	}
	
	while (mStatus != eManagerStatus::eManagerStatus::stopped)
	{
		if (getIsRemoteTerminal() && mDTI.lock()->getStatus() == eDTISessionStatus::ended)
			requestStatusChange( eManagerStatus::eManagerStatus::stopped);
		pingtLastControllerLoopRun();

		// SECURITY: Check for thread termination attacks and restart dead threads
		checkAndRestartBlockFormationThreads();

		threadNameChanged = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		wasPaused = false;
		try
		{
			if (getRequestedStatusChange() == eManagerStatus::eManagerStatus::paused)
			{
				wasFormingBlocks = getDoBlockFormation();
				requestStatusChange(eManagerStatus::eManagerStatus::initial);
				while (getRequestedStatusChange() == eManagerStatus::eManagerStatus::initial)
				{
					if (!wasPaused)
					{
						tools->writeLine("My thread operations were freezed. Halting..");
						//the following should cause both of the mining threads to shut-down
						setDoBlockFormation(false, false);
						threadNameChanged = true;
						//let's wait
						if (mKeyBlockFactoryT.native_handle() != 0)
						{
							while (!mKeyBlockFactoryT.joinable())
								std::this_thread::sleep_for(std::chrono::milliseconds(100));
							mKeyBlockFactoryT.join();
						}
						if (mRegBlockFactoryT.native_handle() != 0)
						{
							while (!mRegBlockFactoryT.joinable())
								std::this_thread::sleep_for(std::chrono::milliseconds(100));

							mRegBlockFactoryT.join();
						}
						
						setStatus(eManagerStatus::eManagerStatus::paused);
						wasPaused = true;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}
			if (wasPaused)
			{
				tools->writeLine("My thread operations are now resumed. Commencing further..");
				setDoBlockFormation(wasFormingBlocks, false);
				threadNameChanged = true;
				// Reset heartbeats before starting threads
				mKeyBlockThreadHeartbeat.store(0, std::memory_order_release);
				mDataBlockThreadHeartbeat.store(0, std::memory_order_release);
				mRegBlockFactoryT = std::thread(&CTransactionManager::dataBlockMinerThread, this);
				mKeyBlockFactoryT = std::thread(&CTransactionManager::keyBlockMinerThread, this);

				mStatus = eManagerStatus::eManagerStatus::running;
			}

			// Persistent DB Update Section - BEGIN
			if (mKeepPersistentDB && mBlockchainManager->getIsReady()) {
				uint64_t currentTime = std::time(0);
				if ((currentTime - mLastPersistentDBUpdate) >= mPersistentDBUpdateInterval) {
					// Check for already running update
					bool expected = false;
					if (mPersistentDBUpdateInProgress.compare_exchange_strong(expected, true)) {
						// Create RAII guard for cleanup
						FinalAction updateGuard([this]() {
							mPersistentDBUpdateInProgress.store(false, std::memory_order_release);
							});

						const uint64_t startTime = std::time(0);
						FinalAction timeReporter([this, startTime]() {
							const uint64_t endTime = std::time(0);
							const uint64_t duration = endTime - startTime;

							// Choose color and log type based on duration (20 minutes = 1200 seconds)
							eColor::eColor timeColor;
							eLogEntryType::eLogEntryType logType;
							if (duration < 1200) { // Less than 20 minutes
								timeColor = eColor::lightGreen;
								logType = eLogEntryType::notification;
							}
							else {
								timeColor = eColor::lightPink;
								logType = eLogEntryType::warning;
							}

							std::string timeStr = mTools->timeToString(duration, true, false);
							mTools->logEvent(mTools->getColoredString("[DB Update]: ", timeColor) +
								"Persistent DB update took " + timeStr,
								eLogEntryCategory::localSystem, 1, logType);
							});

						/*
						* Double-Buffered Persistent DB Update Process
						*
						* Overview:
						* 1. Back room DB (mPersistentStateDBDouble) is populated first since it's never accessed by consumers
						* 2. After successful population, perform atomic swap with active DB
						* 3. Initialize metadata in new StateDomainManager before swapping
						*
						* Synchronization Strategy:
						* - mPersistentDBGuardian: Protects manager pointer and high-level DB operations
						* - DataGuardian: ReverseSemaphore ensuring no active DB users during swap
						* - mPersistentDBSwapMutex: Coordinates the atomic swap operation
						* - mFieldsGuardian: Protects internal fields during updates
						*
						* Lock Ordering (to prevent deadlocks):
						* 1. mPersistentDBGuardian
						* 2. DataGuardian
						* 3. mPersistentDBSwapMutex
						* 4. mFieldsGuardian
						*
						* Data Access:
						* Consumers access data solely from the front-room State Domain Manager and thus from the front-room
						* Merkle Patricia Trie(mPersistentStateDB).
						* Notice that a back-room State Domain Manager - is an ephemeral construct maintained only during an ongoing update.
						*/

						tools->logEvent(
							"Initiating Persistent State DB update...", "Domain Cache",
							eLogEntryCategory::localSystem,
							10, eLogEntryType::notification
						);

						// Phase 1: Update Back Room DB - BEGIN
						std::lock_guard<std::recursive_mutex> managerLock(mPersistentDBGuardian);

						// Get current live DB perspective
						std::vector<uint8_t> livePerspective = mLiveStateDB->getPerspective();
						bool updateSuccess = false;
						size_t nodeCount = 0;

						// Ensure back room DB exists
						if (!mPersistentStateDBDouble) {
							mPersistentStateDBDouble = new CTrieDB(*mPersistentStateDB);
						}

						// Update back room DB perspective
						tools->logEvent(
							"Updating back room database perspective...", "Domain Cache",
							eLogEntryCategory::localSystem,
							10, eLogEntryType::notification
						);

						// we now populate the back-room database
						if (mPersistentStateDBDouble->setPerspective(livePerspective)) {
							// Verify trie integrity without pruning
							if (mPersistentStateDBDouble->testTrie(
								nodeCount,
								false,          // no active pruning
								false,          // don't prune after
								true,          // show progress
								eTrieSize::mediumTrie,
								0              // test all nodes
							)) {
								updateSuccess = true;
								tools->logEvent(
									"Back room database updated successfully. Node count: " +
									std::to_string(nodeCount),
									eLogEntryCategory::localSystem,
									10
								);
							}
							else {
								tools->logEvent(
									"Error: Back room database integrity check failed", "Domain Cache",
									eLogEntryCategory::localSystem,
									10, eLogEntryType::warning
								);
								//mLastPersistentDBUpdate = currentTime; // Update timestamp to prevent immediate retry

							}
						}
						else {
							tools->logEvent(
								"Error: Failed to update back room database perspective", "Domain Cache",
								eLogEntryCategory::localSystem,
								10, eLogEntryType::warning
							);
							//mLastPersistentDBUpdate = currentTime;

						}
						// Phase 1: Update Back Room DB - END

						if (nodeCount && updateSuccess) {
							// Phase 2: Initialize New State Domain Manager - BEGIN
							tools->logEvent(
								"Initializing new State Domain Manager...", "Domain Cache",
								eLogEntryCategory::localSystem,
								10, eLogEntryType::notification
							);

							// Create new manager with updated DB
							auto newManager = std::make_shared<CStateDomainManager>(
								mPersistentStateDBDouble,// notice: we're using the back-room database base which was just populated with new Perspective
								getBlockchainMode(),
								this
							);

							// Stress Test - BEGIN
							 uint64_t testRunCount = 0;
							// Initialize domain metadata
							//while (true)
							//{
								testRunCount++;

								if (!newManager->doDomainMetadataUpdate(
									true,                                      // force update
									std::thread::hardware_concurrency(),       // use all available threads
									nullptr                                    // let it create its own thread pool
								)) {
									tools->logEvent(
										"Error: Failed to initialize domain metadata",
										eLogEntryCategory::localSystem,
										10,
										eLogEntryType::warning
									);
									//mLastPersistentDBUpdate = currentTime;

								}

								//tools->logEvent(
								//	"Performing Meta Data Update Sequence #" + std::to_string(testRunCount),
								//	eLogEntryCategory::localSystem,
								//	10
								//);
							//}
							// Stress Test - END
							
							// Phase 2: Initialize New State Domain Manager - END

							// Phase 3: Atomic Swap - BEGIN
							tools->logEvent(
								"Performing atomic swap of persistent databases...",
								eLogEntryCategory::localSystem,
								10
							);
							bool persistentStateDBLocked = false;

							// DB Swap - BEGIN
							CTrieDB* frontRoomDB = nullptr;  // Track the original front-room DB for proper lock release


							// Wait for all active users (of the current front-room database) to complete
							if (mPersistentStateDB) {
								frontRoomDB = mPersistentStateDB;  // Store reference to current front-room DB before swap
								frontRoomDB->DataGuardian.waitFree(); // wait for all consumers of current front room DB to finish their requests
								persistentStateDBLocked = true;
							}

							// Take swap lock for atomic update
							std::unique_lock<std::shared_mutex> updateLock(mPersistentDBSwapMutex);

							{
								std::lock_guard<std::recursive_mutex> lockLF(mFieldsGuardian);

								// Swap DB pointers
								CTrieDB* old = mPersistentStateDB; // save current front-room DB
								mPersistentStateDB = mPersistentStateDBDouble; // update front-room DB with back-room DB
								mPersistentStateDBDouble = old; // back-room DB is now the old front-room

								// Update manager pointer
								mPersistentStateDomainManager = newManager;
							}

							// Release locks - using the original front-room DB pointer to ensure we release the correct lock
							if (persistentStateDBLocked && frontRoomDB)
							{
								frontRoomDB->DataGuardian.release(); // Release lock on original front-room DB that we acquired the lock on
							}
							// DB Swap - END

							tools->logEvent(
								"Persistent DB update completed successfully", "Domain Cache",
								eLogEntryCategory::localSystem,
								10, eLogEntryType::notification
							);
							// Phase 3: Atomic Swap - END
						}

						// Set the timestamp after the entire process completes
						mLastPersistentDBUpdate = std::time(0);
					}
					else {
						// Update already in progress, log and skip
						tools->logEvent(
							"Persistent DB update already in progress, skipping...", "Domain Cache",
							eLogEntryCategory::localSystem,
							5, eLogEntryType::notification
						);
					}
				}
			}
			// Persistent DB Update Section - END

			// Mempool Cleanup - ALWAYS run regardless of block formation status
			// This ensures TX pre-validation occurs even on dev-nodes with block formation disabled
			if ((tools->getTime() - mLastTimeMemPoolCleared) > 30)
			{
				cleanMemPool();
				mLastTimeMemPoolCleared = tools->getTime();
			}

			if (getDoBlockFormation())
			{
				time = tools->getTime();
				if ((time - lastTimeThreadNameGen) > 10)
				{
				tName = genThreadName();
				tools->SetThreadName(tName.data());
				lastTimeThreadNameGen = time;
				}
				size_t nr = 0;



			}
			else
			{
			if (threadNameChanged)
			{
				tName = genThreadName();
				tools->SetThreadName(tName.data());
			}
			}

		}
		catch (...)
		{
		 assertGN(false);

		}

		if (getRequestedStatusChange() == eManagerStatus::eManagerStatus::stopped)
		{
			requestStatusChange(eManagerStatus::eManagerStatus::initial);
			mStatus = eManagerStatus::eManagerStatus::stopped;

		}
	}
	setStatus(eManagerStatus::eManagerStatus::stopped);
}

void CTransactionManager::keyBlockMinerThread()
{
	std::string tName = genThreadName(true, true);
	std::shared_ptr<CTools> tools = getBlockchainManager()->getTools();
	tools->SetThreadName(tName.data());

	// Check if OpenCL workers are available before starting key block formation
	// Key block formation requires PoW which needs computational devices
	std::shared_ptr<CWorkManager> wm = getWorkManager();
	if (wm && wm->getWorkers().size() == 0)
	{
		tools->logEvent(tools->getColoredString(
			"[ Key Block Formation ]: No OpenCL computational devices available. "
			"Key block formation thread will not run. Data block formation can proceed independently.",
			eColor::lightPink), eLogEntryCategory::localSystem, 1, eLogEntryType::notification);
		return; // Exit thread gracefully - data block formation can still proceed
	}

	manufactureBlocks(true);
}
void CTransactionManager::dataBlockMinerThread()
{
	std::string tName = genThreadName(true, false);
	getBlockchainManager()->getTools()->SetThreadName(tName.data());
	manufactureBlocks(false);
}

std::shared_ptr<CTools> CTransactionManager::getTools()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mTools;
}

std::shared_ptr<CObjectThrottler> CTransactionManager::getThrottler()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mThrottler;
}
/// <summary>
/// Block manufacturing routine. Used by Key-block and Regular-block
/// formation threads.
/// </summary>
/// <param name="keyBlock"></param>
void CTransactionManager::manufactureBlocks(bool keyBlock)
{

	// Local Variables - BEGIN
	std::shared_ptr<CBlock> blockProposal;
	uint64_t lastTimeInformedProductionHalted = 0;
	bool abortFormation = false;

	//Mem Pools Size - BEGIN

	const size_t N = 20;  // Observation period, can be adjusted.

	//Mem Pools Size - END

	size_t lastTimeInformedThatWaiting = 0;
	CKeyChain chain(false);
	uint64_t minNumberOfObjectsInBlock = 1;
	const uint64_t SENSITIVITY = 5; // Defines how sensitive our threshold adjustment should be to changes. Can be adjusted.
	const uint64_t MAX_MIN_OBJECTS = 100; // Upper bound for minNumberOfObjectsInBlock.
	const uint64_t MIN_MIN_OBJECTS = 1;  // Lower bound for minNumberOfObjectsInBlock.
	uint64_t nrOfAvailableObjects = 0;
	bool theresAChanceOfWinning = false;
	const double FIXED_TIME_INTERVAL = 1.0;  // assuming you expect data points every second
	std::vector<uint8_t> lastAnalyzedBlockID;
	std::shared_ptr<CBlock> leader = nullptr;
	uint64_t lastNrOfProcessabledInBlock = 0;
	uint64_t bootstrapGracePeriodStart = 0;
	//bool pastGracePeriod = false;
	uint64_t bootstrapGraceLength = 500;
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	uint64_t maxHeaviestChainProofLookingupTime = 60 * 30;
	std::shared_ptr<CTools> tools = getTools();
	uint64_t lastTimeNotifiedAboutReqObjectCount = 0;
	bool IAMTheLeader = false;
	// Local Variables - END

	//Operational Logic - BEGIN
	if (!mSettings->getCurrentKeyChain(chain, false, false, mSettings->getMinersKeyChainID()))
	{
		getTools()->writeLine("The requested-key chain is not present. Can not participate in the block-creation process.");
		abortFormation = true;
	}

	while (getDoBlockFormation())
	{
		// Thread Health Heartbeat - ping to indicate this thread is alive
		if (keyBlock)
			pingKeyBlockThreadHeartbeat();
		else
			pingDataBlockThreadHeartbeat();

		mSettings->getCurrentKeyChain(chain, false, false); //refresh key-chain
		CKeyChain leaderKeyChainState(false);

		IAMTheLeader = bm->amITheLeader(chain, leaderKeyChainState);


		if (bm->getIsOperatorLeadingAHardFork() && !keyBlock)
		{
			tools->logEvent("Hard-Fork in progress, holding on with data block formation..", eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::orange);
			Sleep(1000);
			continue;

		}

		if (bm->getIsOperatorLeadingAHardFork() &&  getHardForkBlockAlreadyDispatched())
		{
			tools->logEvent("Hard-Fork in progress, holding on with key- block formation..", eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::orange);
			Sleep(1000);
			continue;

		}
		while (CSettings::getIsGlobalAutoConfigInProgress())
		{
			bootstrapGracePeriodStart;
			tools->logEvent("Node is still bootstrapping, holding on with block formation..", eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::orange);
			Sleep(5000);
			continue;
		}

		if (bm->getHeaviestChainProofKeyLeader() == nullptr && ((std::time(0) - bootstrapGracePeriodStart) < maxHeaviestChainProofLookingupTime))
		{
			uint64_t toGo = ((std::time(0) - bootstrapGracePeriodStart) < maxHeaviestChainProofLookingupTime) ? (maxHeaviestChainProofLookingupTime - (std::time(0) - bootstrapGracePeriodStart)) : 0;
			tools->logEvent("[Bootstrap grace period]: Still outlooking the Heaviest Chain-Proof.." + std::to_string(toGo) + " sec.", eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::orange);
		}
		else {
			if (!getIsPastGracePeriod())
			{
				bootstrapGracePeriodStart = std::time(0);
				uint64_t toGo = ((std::time(0) - bootstrapGracePeriodStart) < bootstrapGraceLength) ? (bootstrapGraceLength - (std::time(0) - bootstrapGracePeriodStart)) : 0;

				while (toGo && !(getIsPastGracePeriod()))
				{
					toGo = ((std::time(0) - bootstrapGracePeriodStart) < bootstrapGraceLength) ? (bootstrapGraceLength - (std::time(0) - bootstrapGracePeriodStart)) : 0;

					tools->logEvent("[Bootstrap grace period]: Withholding block formation.. " + std::to_string(toGo) + " sec.", eLogEntryCategory::localSystem, 3, eLogEntryType::notification, eColor::orange);
					Sleep(5000);
				}
				setIsPastGracePeriod(true);
			}
		}


		//verify the chance of winning - BEGIN
		if (keyBlock)
		{


			theresAChanceOfWinning = bm->doIhaveAChanceOfWinning();


			if (!theresAChanceOfWinning && ! bm->getIsOperatorLeadingAHardFork())
			{
				tools->logEvent("Negligible chances of winning PoW race as of now.. sleeping..", eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::orange);
				Sleep(20000);
				continue;
			}
			//verify the chance of winning - END
		}

		if (!keyBlock)
		{
			leader = bm->getLeader();
			lastTimeInformedThatWaiting = tools->getTime();
			do {
				if (leader != nullptr && !leader->getHeader()->isKeyBlock() && (!tools->compareByteVectors(lastAnalyzedBlockID, leader->getID())))
				{
					lastAnalyzedBlockID = leader->getID();
					lastNrOfProcessabledInBlock = leader->getReceiptsIDs().size();
				
				}

				nrOfAvailableObjects = getUnprocessedTransactions(eTransactionSortingAlgorithm::feesHighestFirstNonce).size() + getUnprocessedVerifiables(eTransactionSortingAlgorithm::feeHighestFirst).size();
				
				//Calculate Min-Number of Objects - END

				if (nrOfAvailableObjects < minNumberOfObjectsInBlock)
				{
					if (tools->getTime() - lastTimeInformedThatWaiting > 3)
					{
						tools->logEvent("Waiting for processable objects.. (min.nr: " + std::to_string(minNumberOfObjectsInBlock) + ")", "[ Data Block Formation ]", eLogEntryCategory::localSystem, 4,  eLogEntryType::notification);
						lastTimeInformedThatWaiting = tools->getTime();
					}
				}
				else
				{
					tools->writeLine(tools->getColoredString("Collected enough processable objects: ", eColor::lightGreen) + std::to_string(nrOfAvailableObjects));
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				setReqMemPoolObjectsCached(minNumberOfObjectsInBlock);
				setTXInMemPoolCached(nrOfAvailableObjects);

			} while (nrOfAvailableObjects < minNumberOfObjectsInBlock && getDoBlockFormation());

			if (getDoBlockFormation() && (std::time(0) - lastTimeNotifiedAboutReqObjectCount) > 15)
			{
				lastTimeNotifiedAboutReqObjectCount = std::time(0);
				tools->writeLine("Min. # of objects for block formation: " + std::to_string(minNumberOfObjectsInBlock));
			}
		}
		


	
		abortFormation = false;
		if (keyBlock)
		{
			if (bm->getKeyBlockFormationLimit() != 0)
			{
				if (getFormedKeyBlocksCounter() >= bm->getKeyBlockFormationLimit())
				{
					tools->writeLine("Reached key-block formation limit.");
					return;
				}
			}
		}
		else
		{
			if (bm->getDataBlockFormationLimit() != 0)
			{
				if (getFormedDataBlocksCounter() >= bm->getDataBlockFormationLimit())
				{
					tools->writeLine("Reached data-block formation limit.");
					return;
				}
			}

		}
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		if (!isBlockProductionHalted())
		{
			//BLOCK-FORMATION - BEGIN
			setBlockFormationStatus(keyBlock, eBlockFormationStatus::forming);
			uint64_t previousHeight = bm->getCachedHeight();
			//TESING - BEGIN

			//Testing deferred pay-outs - BEGIN
			if (keyBlock)
			{
				if (mSettings->getEnableAutomaticIDSwitching(false) && ((chain.getCurrentIndex() + 1) >= getCycleBackIdentiesAfterNKeysUsed()))
				{
					tools->writeLine("Max nr. of my identities has been reached. Cycling back..");
					chain.setIndex(0);
					resetKeyBlocksGeneratesUsingSamePubKey();
					resetDataBlocksGeneratesUsingSamePubKey();
				}
				else
				{
					//am I to switch to a new Pub-Key?
					if (getKeyBlocksGeneratesUsingSamePubKey() > getNewPubKeyEveryNKeyBlocks())
					{
						if (!(mSettings->getCurrentKeyChain(chain, true, false)))//this will get next-pub key from the key-chain
						{
							tools->writeLine("The requested-key chain is not present. Can not participate in the block-creation process.");
							abortFormation = true;
							break;
						}
						std::vector<uint8_t> pk1 = chain.getPubKey();
						mSettings->getCurrentKeyChain(chain, false, false);
						std::vector<uint8_t> pk2 = chain.getPubKey();
						assertGN(!tools->compareByteVectors(pk1, pk2));
						resetKeyBlocksGeneratesUsingSamePubKey();
						resetDataBlocksGeneratesUsingSamePubKey();
					}
				}
			}

			//Testing deferred pay-outs - END
			std::shared_ptr<CBlock> leader = bm->getLeader();
			std::shared_ptr<CBlock> keyLeader = bm->getLeader(true);
			//Testing Proof-of-Fraud - BEGIN
			std::vector<uint8_t> forcedParentID;
			std::vector<uint8_t> leadersPubKey;
		
			bool genFraud = false;
			uint64_t pubFoundAtIndex = 0;
			uint64_t coinToss = 0;
			uint64_t coinTossThreashold = 15;
			std::shared_ptr<CBlock> currentBlock = nullptr;
			uint64_t priorDataBlocksCount = 0;
			uint64_t tossedParentIndex = 0;
			bool overrideAmITheLeaderCheck = false;

			if (leader != nullptr && keyLeader != nullptr && !keyBlock && getFraudulantDataBlocksToGeneratePerKeyBlock() > 0)
			{

				coinToss = tools->genRandomNumber(0, 100);
				if (keyLeader->getHeader()->getKeyHeight() >= 1 && coinToss <= coinTossThreashold)
					genFraud = true;//one in 4 blocks will be fraudulent IF only data blocks are being generated

				eBlockInstantiationResult::eBlockInstantiationResult bir;
				if (genFraud)
				{

					//we want to generate a Fraud by a PREVIOUS LEADER
					std::shared_ptr<CBlock> previousKeyLeader = keyLeader;
					//std::shared_ptr<CBlock> previousKeyLeader;

					std::shared_ptr<CBlock> lastBlockFromPreviousEpoch;// = keyLeader->getParent(bir, true, false, true);

					bool fraudOrdered = false;
					uint64_t epochsInThePast = 0;
					//The below loop will continue looking for an epoch with at least one data-block so that a fraud can be generated.
					//it does not need to be an imminent latest epoch.
					while (!fraudOrdered)
					{
						epochsInThePast++;
						lastBlockFromPreviousEpoch = previousKeyLeader->getParent(bir, true, false, true);
						previousKeyLeader = bm->getKeyBlockForBlock(previousKeyLeader);
						if (lastBlockFromPreviousEpoch == nullptr || previousKeyLeader == nullptr)
							break;
						if (lastBlockFromPreviousEpoch->getHeader()->isKeyBlock())
							continue;

						if (!chain.searchForPubKey(previousKeyLeader->getHeader()->getPubKey(), pubFoundAtIndex))
						{
							tools->writeLine("Can't generate fraud in previous epoch. PrivKey unknown");
						}
						else
						{
							chain.setIndex(pubFoundAtIndex);

							priorDataBlocksCount = lastBlockFromPreviousEpoch->getHeader()->getHeight() - previousKeyLeader->getHeader()->getHeight();

							if (priorDataBlocksCount > 0)
							{
								tossedParentIndex = 0;

								if (priorDataBlocksCount == 1)
									tossedParentIndex = 1;
								else tossedParentIndex = tools->genRandomNumber(1, priorDataBlocksCount - 1);

								currentBlock = lastBlockFromPreviousEpoch;

								for (uint64_t i = 1; i < tossedParentIndex; i++)
								{
									currentBlock = currentBlock->getParent(bir, false, false, true);
									if (!(currentBlock != nullptr && !currentBlock->getHeader()->isKeyBlock()))
										assertGN(false);

								}

								if (currentBlock != nullptr && currentBlock->getHeader()->getHeight() != leader->getHeader()->getHeight() && !currentBlock->getHeader()->isKeyBlock())
								{
									overrideAmITheLeaderCheck = true;
									forcedParentID = currentBlock->getID();
									tools->writeLine("Ordering generation of a fraudulent data-block (" + std::to_string(epochsInThePast) + " epochs in the past," + "block index:" + std::to_string(tossedParentIndex) + ")");
									fraudOrdered = true;
									incFraudulantDataBlocksOrdered();
								}

							}


						}

					}
				}

			}
			//Testing Proof-of-Fraud - END

			//TESING - END

		proceed:
			// ============================================================================
			// Phantom Leader Mode Check and Leader Verification - BEGIN
			// ============================================================================
			// [ Modified Behavior ]: When Phantom Leader Mode is enabled, the node
			//                        bypasses the leader check and proceeds with block
			//                        formation for debugging purposes. Phantom blocks
			//                        are NOT broadcasted to the network.
			// ============================================================================
			bool isPhantomModeActive = bm->getIsPhantomLeaderModeEnabled();
			bool proceedAsPhantomLeader = false;

			if (!keyBlock)
			{
				// Phantom Leader Mode bypasses the leader check for data blocks
				if (isPhantomModeActive && !IAMTheLeader)
				{
					proceedAsPhantomLeader = true;
					tools->writeLine(tools->getColoredString(
						u8"╔══════════════════════════════════════════════════════════════════╗", eColor::orange));
					tools->writeLine(tools->getColoredString(
						u8"║       PHANTOM LEADER MODE - Simulating Leader Operations         ║", eColor::orange));
					tools->writeLine(tools->getColoredString(
						u8"╚══════════════════════════════════════════════════════════════════╝", eColor::orange));
					tools->writeLine(tools->getColoredString(
						"Processing " + std::to_string(nrOfAvailableObjects) + " objects in PHANTOM mode (block will NOT be broadcasted)",
						eColor::lightPink));
					mFieldsGuardian.lock();
					mInformedNotLeader = false;
					mFieldsGuardian.unlock();
				}
				else if (!overrideAmITheLeaderCheck && !IAMTheLeader)
				{
					abortFormation = true;
					mFieldsGuardian.lock();
					if (!mInformedNotLeader)
					{
						tools->writeLine("Can't form Data-Blocks. Not the current Leader, I shall wait to become one..");
						if (isPhantomModeActive)
						{
							// This branch shouldn't be reached if phantom mode is active,
							// but provide guidance just in case
							tools->writeLine(tools->getColoredString(
								"Note: Phantom Leader Mode is enabled but not triggering. This may be a bug.",
								eColor::lightPink));
						}
						mInformedNotLeader = true;

					}
					mFieldsGuardian.unlock();
				}
				else
				{
					tools->writeLine(tools->getColoredString("Proceeding with data-block formation as the active Leader "+
						std::string(overrideAmITheLeaderCheck?tools->getColoredString(" -FORCED-  ", eColor::lightPink):""), eColor::neonGreen) +
						" (considering "+std::to_string(nrOfAvailableObjects)+ " objects)");
					mFieldsGuardian.lock();
					mInformedNotLeader = false;
					mFieldsGuardian.unlock();
				}
			}
			// Phantom Leader Mode Check and Leader Verification - END
			// ============================================================================

			if (abortFormation)
				continue;
			bm->updateStatysticsToFile(true);


			// Hard-Fork Procedure - BEGIN
			if (keyBlock && bm->getIsOperatorLeadingAHardFork())
			{
				std::vector<uint8_t> forcedLeaderID = bm->getLatestObligatoryCheckpoint()->getHash();
				tools->writeLine("Forcing a one-time Parent Block: '"+ tools->base64CheckEncode(forcedLeaderID)+"'", true, false, eViewState::GridScriptConsole, "Hard Fork");
				//Latest Obligatory Checkpoint would be forced as the parent of the to-be-produced Key Block.
				//The block would have no PoW what so ever. Still, it would be accepted by local node due to Dynamic Checkpoint which would be then added into the Source.
				//After the Block has been included as an official checkpoint into the source code, other nodes would honor it as well. And so the Glory of Decentralization could further prevail.
				forceOneTimeParentMiningBlockID(forcedLeaderID);
			}
			// Hard-Fork Procedure - END

			eBlockFormationResult bfr;
			// Block Formation - BEGIN
			{
			
				std::unique_lock<std::mutex> lock(mDataBlockFormationGuardian, std::defer_lock);
				if (!keyBlock) {
					lock.lock();
				}

				// Key chain selection:
				// - Key blocks: use our own chain
				// - Data blocks with leader override: use our own chain
				// - Phantom mode: use our own chain (we don't have leader's private key)
				// - Normal data blocks: use leader's key chain state
				bfr = formBlock(blockProposal, keyBlock,
					(keyBlock || overrideAmITheLeaderCheck || proceedAsPhantomLeader) ? chain : leaderKeyChainState,
					forcedParentID,
					proceedAsPhantomLeader);  // Phantom mode - transactions remain in mem-pool
			}
			// Block Formation - END

			if (bfr == eBlockFormationResult::blockFormed)
			{
				if (keyBlock)
				{


					//Hard-Fork Support - BEGIN
					if (blockProposal->getIsLeadingAHardFork())
					{
						
						if (bm->getIsOperatorLeadingAHardFork())
						{
							
							cpFlags flags;
							flags.obligatory = true;
							flags.active = true;//pre-activate the checkpoint
							bm->addCheckpoint(std::make_shared<CBCheckpoint>(flags, blockProposal->getHeader()->getHeight(), blockProposal->getID()));
					
							tools->writeLine(tools->getColoredString("Adding a dynamic checkpoint.", eColor::orange), true, true, eViewState::GridScriptConsole,"Hard Fork");


							/*                                                =[  THE SOFT FORK PROCEDURE  ]=
							* 
							*						

													[ DEFINITIONS ] - BEGIN

													Further, the Soft Fork Procedure is related to simply as 'Procedure'.

													Operator - a person operating an instance of GRIDNET Core.
													Release Candidate - the version of GRIDNET Core which is to be Released.
													Soft Fork Point - block height at which new version requirements begin.
													Forwards Compatible - earlier version can process blocks produced by newer version.
													Weak Enforcement - enforcement through block rejection rather than consensus rules.

													[ DEFINITIONS ] - END

													[ SECURITY CONSIDERATIONS ] - BEGIN

													The Soft Fork Procedure provides weaker guarantees than Hard Fork Procedure:
													- The release is *REQUIRED* to be fully backwards compatible.
													- MUST NOT require full revalidation of all blocks since Genesis Block (the backwards compatibility requirement).
													- Cannot enforce version requirements through state roots.

													[IMPORTANT]: Use Soft Fork only when both conditions are met:
															- Release Candidate is fully backwards compatible with current Live-Net.
							*								- Current state of the network is HEALTHY.
							*								- no faulty blocks and or/transactions need to be forked out.
							*								- processing of all past data remains EXACTLY the same.
							*								- Re-processing of all blocks MUST NOT be required.
							*								- On the other hand - NEW consensus changes MAY be introduced IF version set as obligatory. <- required version is associated with BLOCK HEIGHT ( Soft Fork Point ).

													[WARNING]: If security-critical changes are needed or earlier version is not
																forwards compatible, consider going with Hard Fork Procedure instead.

													[ SECURITY CONSIDERATIONS ] - END

													[ PROCEDURE STAGES ]

													[ INTERNAL - BEGIN ]

													[ Public Relations Stage 0 ]
													- Notify Exchanges about upcoming Core version update.
													- Explain to Community that this is a soft fork (obligatory or not).
													- Clarify WHETEHR erlier versions will remain compatible ( =  was CORE_VERSION_REQUIRED updated ? )

													[ PHASE 0 ]
													0) [ Version Updates ]
														- Increment CORE_VERSION_NUMBER in CGlobalSecSettings.h
														- Update CORE_VERSION string in CGlobalSecSettings.h
														- update getRequiredVersionNumber() in CGlobalSecSettings.cpp <- required version is associated with BLOCK HEIGHT ( Soft Fork Point ).
														- Update coreVersionToString in CGlobalSecSettings.h
														
														- SET CORE_VERSION_REQUIRED to desried version in CGlobalSecSettings.h
														- Update SOFT_FORK_POINT to target block height

														[IMPORTANT]: For additional consensus level security (MPT state-level )Consider modifying  SECURITY_KEY_REWARD_NONCE and thus CGlobalSecSettings::getVersionSpecificNonceRewardGBUs but ONLY if Soft Fork is obligatory.

													1) [ Validation Testing ]
														- Test that Release Candidate accepts blocks from earlier version
														- Test that Release Candidate produces blocks processable by earlier version
														- Verify block validation rules properly enforce version requirements
														- Test error handling for outdated Core versions

													2) [ Documentation ]
														- Document changes in Release Notes
														- Explain backwards compatibility
														- Note minimum version requirement
														- Document block height where requirement begins

													[ PHASE 1 ]
													1) Update Tier 0 nodes first
														- Monitor for any validation issues
														- Verify version requirements are enforced
														- Confirm blocks still process correctly

													2) [ Network Health Monitoring ]
														- Monitor network for signs of forking
														- Track version adoption progress
														- Watch for any unexpected behavior

													[ INTERNAL - END ]

													[ GLOBAL - BEGIN ]

													[ PHASE 2 ]
													1) Prepare release package
													2) Update Release Notes
													3) [ Staged Distribution ]
														- First to Tier 1 Operators
														- Then to Tier 2 Operators
														- Then to Tier 3 Operators
														- Finally Public Release

													4) [ Post-Release Monitoring ]
														- Track version adoption
														- Monitor for any issues
														- Provide support for upgrades

													[ GLOBAL - END ]

													[ SECURITY ADVISORY SA_101 ]

													The Soft Fork Procedure relies on CORE_VERSION_REQUIRED for enforcement:
													- Blocks from outdated Cores are rejected by validation rules
													- provides consensus-level enforcement (each node checks reported version in every block)
													- Cannot prevent malicious nodes from bypassing version check (altered binary) - fot this use SECURITY_KEY_REWARD_NONCE mechanics which effectively 'salts' the entire Merkle Patricia State Trie.

													[IMPORTANT]: If strong security guarantees are needed:
													1. Use Hard Fork Procedure instead
													2. Increment SECURITY_KEY_REWARD_NONCE ( affects top of Merkle Patricia State Trie ) - provides strong consensus association.


													[ ROLLBACK CONSIDERATIONS ]
													If issues are discovered after soft fork:
													1. Earlier version remains compatible
													2. Can reduce CORE_VERSION_REQUIRED if needed
													3. Network can continue processing blocks
													4. No revalidation of past blocks needed

*/

							/*
							
																			[ Rewriting TXs' Source Code ]

									[ Since When ]:  1.8.6
									Source code of arbitrary select transactions can be replaced on the fly during processing of on-chain data before even byte-code is decompiled.
									Refer to CGridScriptCompiler::decompile() for more. 
									[ IMPORTANT ]: when used, latest version needs to be marked as obligatory to avoid breaking consensus.

									[ Operations ]: CGlobalSecSettings::mTransactionSourceRewrites() <- list of transactions and effective source-codes.



																[ Toggling Kernel-mode Availability of GridScript commands]

									[ Since When ]:  1.8.6

									Availability of GridScript commands in kernel-mode can be toggled on demand.
									[ IMPORTANT ]: when used, latest version needs to be marked as obligatory to avoid breaking consensus.

									[ Operations ]: modify CGlobalSecSettings::mCodewordKernelModeOverrides() accordingly.
							*/


							/*												 =[  THE HARD FORK PROCEDURE  ]= 
							* 
							* [ DEFINITIONS ] - BEGIN
							* Further, the Hard Fork Procedure is related to simply as 'Procedure'.
							* Operator - a person operating an instance of GRIDNET Core.
							* Release Candidate - the version of GRIDNET Core which is to be Released.
							* Hard Fork Leader - an Operator carrying out the Hard Fork Procedure stages prior to Public Stages while having access to source code.
							* Operator's Console - the view available after pressing CTRL-E.
							* Service Up-Keep Node - a node which is left out in bootstrap.gridnet.org DNS entry. It's to accept new blocks only from Hard Fork Lead while rejecting blocks from any other UDT connection.
							*							TODO: the functionality needs yet to be implemented. For now we have eclipsing. Here, mobile apps need to be able to connect over UDT but blocks or chain-proofs are to be accepted only from the hard fork lead.
							* 
							*  [ DEFINITIONS ] - END
							 
							 [ SECURITY CONSIDERATIONS ] - BEGIN
							 The procedure effectively thwarts any heavier histories of events from being evaluated or taken under consideration at all
							 unless produced by the Release Candidate which is ultimately introduced as a result of this Procedure.
							
							 While Operators MUST be instructed to cease their Operations after the Hard Fork Point is announced, during a Hard-Fork we might be having
							 Operators still mining upon the previous history of events. The system is decentralized and this cannot be prevented.

							 Thus, while we cannot force Operators to cease their mining Operations, the procedure introduces a checkpoint referencing a new block, one produced right 
							 after the Hard Fork Point. 
							 
							 The block following this fork point needs to be produced by the software acting as the Release Candidate and thus compatible with it (not the Previous Version).

							 Should a need to alter Operators' Rewards arise (also in the past) - use [ REWARD ACTUATION TEMPLATE ] in Verifier.h

							[ SECURITY CONSIDERATIONS ] - END
							*/

							// [ Hard-Fork Procedure  ] - BEGIN
							/*
							* [ Code ]: within of the code base, relevant code fragments are marked with a [ Hard-Fork ] tag. 
							* 
							* 
							* [ Effect of a Hard Fork on End-User Experience]:
							* 
							*  [ Update Notices - BEGIN ] 
							* 
							*	Update (4/5/2024):  the revised version of a hard fork does not stop transactions from being processed.
							* 
							*  [ Update Notices - END ] 
							* 
							* 
							* [Rationale] : After the Leading Node has produced a Hard Fork Block, DNS entries pointing to bootstrap.grindet.org (used by the mobile app) are set to point only the Leading Node.
							*			    This causes mobile apps to connect only to it while the Leading Node may continue processing transactions uninterrupted.
							*               Operators need to be notified about the Hard Fork Point PRIOR to this.
							*				Notice that connectivity for mobile apps MAY be affected due to delays in DNS data propagation and DNS caching.
							* 
							* [IMPORTANT]: Official nodes need to be divided into TWO groups.
							*			   The Procedure is comprised of INTERNAL (performed by the Team) and GLOBAL ( Operators join in, exchanges notified) stages.
										   At least one official bootstrap node needs NOT to participate in Phase [0-1] of the Hard Fork Procedure.
							*			   Those nodes MUST keep running the old version of the GRIDNET Core.
							*			   [ Rationale ]:  This is to up-keep availability of data for other nodes.
														   Blocks up to and including the FORK_POINT are needed - as non-fork-leaders synchronize from Genesis Block.
							*
							*			
							*				--------------------------------------------------------------------------
							*								   v--[	Mission Critical Rules ]--v
							*				RULE 1: If consensus broken (usually the very reason for a hard fork), if new changes introduced 
														- the Leading Node MUST be synchronized from Genesis Block. This holds for all Tier 0 nodes.
														- use `chain -setextcpproc false` to avoid processing of extenral blocks and chain proofs on nodes before even Hard Fork Block is produced.
														- after at least Hard Fork Leader is fully Genesis Re-Synced, the leader may produce a Hard Fork Leading Block.  Then, after injected into source all the further Hard Fork procedures may commence.
														- such an approach allows for the Hard Fork Leader to be fully Operational with a correct Perspective right after the Hard Fork. Otherwise, the Leader would need to undergo full Genesis Resync after having produced the Hard Fork Leading Block.
														- Notice that the Leading node neesd to be Genesis re-synced  for the Final Perspective within of the Hard Fork Block to be Valid.
							*							 -> BEFORE <- generating the Hard Fork Block.
							* 
							*			RULE 2: ---->  *THERE ARE -TWO- [ CHECKPOINTS ] THAT NEED TO BE SET IN SOURCE CODE* <---- (one which overwrites the most recent checkpoint and a SINGLE new addition).
							*						SEE [FIRST CHECKPOINT] and [SECOND CHECKPOINT] later down below in [ PHASE 1 ].
							*				RULE 3:				.THE CHECPKOINTS' COUNTER NEEDS TO BE INCREASED.
							*				RUlE 4:		Operator's Nonce - if changes to key-blocks (mining rules).
							*				RULE 5: if Leading Node was re-synced so needs every other node.
							*			    RULE 6: after Hard Fork completes the Leading Node needs to end up with an accurate total number of checkpoints set in Cold Storage. Otherwise it would nag about it and attempt to resync (possibly again).
							*				RULE 7: use  'chain -setextproc [on/off] to secure pre-hard-fork point on participating nodes - eliminates need of disabling NICs
							--------------------------------------------------------------------------
							* 
							* [ INTERNAL - BEGIN ]
							*								           	[	IMPORTANT	]
							* 
							*    ***** The Leading Node must be fully re-synchronised from Genesis Block by the Release Candidate [SEE POINT 1-X] *****
							*			The Leading Node Needs not to be PoW capable. It would be able to produce consecutive Data Blocks.
							*			(and thus to accept transactions) thanks to the Hard Fork Block it is about to produce.
							* 
							* [ Public Relations Stage 0]
							* 
							* - Notify Exchanges about the upcoming Hard Fork. Specifically they need to cease outgoing transactions.
							* - Explain to the Community and Exchanges that TX processing MIGHT be affected. 
							* - Explain to the Community that mobile app connectivity MAY be affected as well.
							* 
							* 
							* [PHASE 0]
							*  - [ DNS Stage 0 ]: - backup IPs of nodes associated with the bootstrap.gridnet.org domain.
							*					  - leave out only (or introduce if not present) an entry regarding the Service Up-Keep Node.
							*    [ Rationale ]: - to prevent mobile apps from connecting with Tier 0 nodes other than the Leading Node.
							*					- to uphold processing of transactions, by the Hard Fork Leader, as soon as the Hard Fork Block is produced.
							* 
							*  - check all Tier 0 nodes, choose one with the highest Blockchain Height (BH).
							*  - (optional) wait till all Tier 0 nodes are at the very same Blockchain Height as the one above.
							*			   [ WHEN ]: when full resync is not scheduled. Otherwise we choose Tier 0 node with highest BH and it would serve as a Hard Fork Leader. 
							*															Then other Tier 0 nodes would synchronize from it from scratch (or locally anyway).
							*		
							*			[ ^ This BH is to constitute the new Hard Fork Point ^ ]
							*			The Procedure would introduce a yet another block - the [Hard Fork Block] - at height equal to the [Hard Fork Point]+1.
							* 
							*  - as soon as - execute the 'net -boot 127.0.01 -only' command on all Tier 0 nodes.
							* 	 [ Rationale ]: this effectively ensures that blockchain would NOT be updated across Tier 0 nodes, until the Procedure is complete.
							*  - execute `chain -flush` on all Tier 0 nodes.
							*		IMPORTANT: await a notification in GREEN. Ignore the grey one about success (if any).
							*	 [ Rationale ]: forces chain-proofs to be flushed onto the Cold Storage.
							*  - disable network adapters on all Tier 0 nodes on which it is possible (not obligatory).
							*	 [ Rationale ]: ensures best possible re-synchronization performance.
							*  - on a node leading the fork DISABLE NETWORKING.
							*  - ensure there's at least one Tier 0 node which is to keep running the Previous Version.
							*	 [ Rationale ]: should anything go wrong it would be used as a bootstrap node for others, should the Procedure begin anew.
							*  
							*  - update FORK_POINT in CGlobalSecSettings.h  (blockchain height)
							*  - update FORK_POINT_ID in CGlobalSecSettings.h (block identifier)
							*  
							[ Public Relations Stage 1]
							*  - notify the Community about the Hard Fork Point
							* 
							* [ IF PAST HISTORY OF THE CHAIN IS IN AN INVALID STATE, ONLY ] - BEGIN
							* 
							*  [IMPORTANT]: on the leading node - DISABLE data block formation.
							*				[ Rationale ]: if leader processed TXs right after being elected 
							* Data block formation is to be re-enabled only after the leading node did a full-re-sync.
							* 
							* [ IF PAST HISTORY OF THE CHAIN IS IN AN INVALID STATE, ONLY ] - END
							* 
							* 0) Increment CORE_VERSION_NUMBER (number) in CGlobalSecSettings.h
							* 1) Increment CORE_VERSION_NUMBER_REQUIRED (number) in CGlobalSecSettings.h
							* 1) Update CORE_VERSION (string) in CGlobalSecSettings.h
							* 2) (optional) Perform backups of all State-Domains' balances into JSON (the 'backup' GridScript command). 
							* 3) On Tier 0 nodes participating in Phase [0-1], perform backups of the 'GRIDNET' folder in AppData (native OS)
							*														The folder should be named 'GRIDNET_HEIGHT_X_TIMESTAMP.7z'. Where X is Fork Point.
							*														Proceed with main entry point nodes first, then miners.
							* 
							*
							* 6) Triple check that the Leading Node is fully synchronized at a desired FORK_POINT prior to proceeding any further.
							* 
							* 
							*  [ IMPORANT ]: a Hard Fork Block follows right AFTER each block listed below.
							* 
							* CVE_805 - BEGIN
							* 60467 // <- REQUIRED minimal blockchain height
																  // it's the height at which there's the only TX which succeeds even though block recept says invalid.
																  // before 1.6.5 Hard Fork. We want the TX to succeed. For all newer blocks - what's in the past
																  // shall be in the past.
							* CVE_805 - END
							* 
							* { Log of Past Hard Forks } <- update this.
							*  - block height: [ TODO: copy this line below and add FORK_POINT here ]
							*  - block heihgt: 90074
							* -  block height: 84165
							*  - block height: 84166
							*  - block height: 82036
							*  - block height: 79094
							*  - block height: 63248
							*  - block height: 60901
							*  - block height: 60274
							*  - block height: 42362
							*  - block height: 37737
							*  - block height: 34703
							*  - block height: 34515
							*  - block height: 31225
							*  - block height: 31140
							*  - block height: 30000
							* 
							* 
							* 
							* [ PHASE 1 ]
							* 
							* 0) { SECURITY_KEY_REWARD_NONCE }: Optional, yet most likely needed. MANDATORY if, changes to processing of GridScript or anything that could affect Perspective 
																		(rewards, PoW difficulty adjustment etc. - usually the very reason for a Hard Fork to begin with).
													[ IMPORTANT ] is the Procuedure is carried out as a result of an unexected chnage to Consensus (infant nodes unable to get get the prior Hard Fork Point).
																 Then, this sub-procedure MUST be followed as well.

									 - increment SECURITY_KEY_REWARD_NONCE in CGlobalSecSettings.h ( notice - contains a {Template} which NEEDS to be used )
									 - CGlobalSecSettings::getVersionSpecificNonceRewardGBUs() - *USE {Template}* to make the previous value of SECURITY_KEY_REWARD_NONCE effective for blocks prior to the Fork Point which being introduced. 
										{ Explanation }: a new entry in Source needs to be placed below the upper most dynamic entry - with a height of the *prior* Hard Fork
										{ IMPORTANT }: *All* previous entries with [ previous NONCE | previous fork-point ] couplings - these MUST remain. Use included {Template} to introduce a new IF-branch atop of the current one.
										{ RATIONALE }: to make processing of key-blocks prior to the Fork Point exactly the same as with old versions of GRIDNET Core.
													   The Operator's Nonce protects against cross-version attacks during Hard Forks.

							 1) { INTORUCTIO OF BLOCKS' [FIRST CHECKPOINT] }: [ Description, prior to ACTION ACP1]:  In LoadCheckpoints(), the [FIRST CHECKPOINT] (mandatory) needs to OVERWRITE the current latest one which is already present as the very *LAST* on the list. 
																>- It it to describe the current Hard Fork-Point.-<

								The software would use this Checkpoint - describing the current Hard Fork Point to enforce parent block upon the mining sub-system.

								[ACTION ACP1]: replace block height of the *bottom most* checkpoint with:
										  - height: FORK_POINT
										  - block ID: FORK_POINT_ID

								[ RATIONALE ]: The prior mandatory checkpoints are no longer needed as far as the Protocol is concerned. We keep these for accountability in code.
											   We can thus overwrite the latest prior one.
											   The other [SECOND CHECKPOINT] (mandatory) - which is to be actually ADDED(!) later on - it would describe the Hard Fork Block  which is to be presentat height FORK_POINT+1.
											   The other [SECOND CHECKPOINT] (mandatory) - which is to be actually ADDED(!) later on - it would describe the Hard Fork Block  which is to be presentat height FORK_POINT+1.

								[ INTERACTIVE AUTOMATED PHASE ] - BEGIN

								Note: During this interactive process, progress and errors would be reported by GRIDNET Core directly within of the Operator's Terminal (NOT the Events' Log).
							 
							 1-X) update Leading Node to the latest revision of source code, build it and after done execute `resync -local`
							
							2) The Leading Operator proceeds as follows:
								    - Launches the GRIDNET CORE Release Candidate.
									- Enables the CUSTOM BOOTSTRAP SEQUENCE.
									- After that he or she indicates willingness to be leading a Hard Fork (answers YES to the 'are you leading a Hard Fork question) and to further confirmations. Then, automated bootstrap sequence MAY resume (press 'e').
									  (by answering an appropriate question during the Bootstrap Sequence). The purpose is to disable certain security checks during processing of blocks
									  and for the system to enter a Hard Fork Lead mode. The purpose of this mode is to generate a single Hard Fork Block which would be then 'check-pointed' ( [SECOND CHECKPOINT] )
									  within of the source-code. During this mode no other block would be generated, processed or accepted (override in the processBlock() method).
									- (optional) once Vitals are reported as 'Good', to speed things up ( by omitting the Grace Period ) Operator may choose to execute the 'hotstart' command.

							 2A) Once a Hard Fork Block (0 PoW) is produced - one immediately following a FORK_POINT:
								- The system would include a Dynamic Ephemeral Checkpoint so that the Block Processing Sub-System could accept it (overriding PoW / timing requirements etc.).
								- The block would be scheduled for processing. The system would not accept any other block but for the Hard Fork Block.
								- On success, once Hard Fork Block is processed and appended, a three-bell sound would be emitted from the on-board speaker.
								- Information regarding the new Leading Block would be shown within the Operator's Console.
								[ SAMPLE OUTPUT ] - BEGIN
								----------------------------------------------------------------------------------------
								 Hard Fork: ✅ A Hard-Fork succeeded!
								 Hard Fork Lead: [Height]: 31226 [ID]: tQSL9qsuQNQUQQRgoPLvsjTit6PntzizLTch6sRMeYGVY4ZeJ
								 Hard Fork:  Operator, remember to include the above Checkpoint in Source.
								 Hard Fork: Chain Proofs were flushed to Cold-Storage.
								 ----------------------------------------------------------------------------------------
								[ SAMPLE OUTPUT ] - END
								Notice: the checkpoint mentioned above is the [SECOND CHECKPOINT].

								- The Fork-Leading mode would be automatically disabled. The node would continue normal operations AFTER restarted. The Operator would be notified accordingly.

								[ INTERACTIVE AUTOMATED PHASE ] - END

								
							
							 3A) [IMPORTANT]: - The [SECOND CHECKPOINT] needs to be introduced (ADDED), describing the just appended Leading Block (block height and block's identifier).
											 - The FORCED_RESYNC_SEQUENCE_NUMBER needs to be INCREMENTED if all nodes are to commence withh full autonomous resynchronisation from Genesis Block. 

											[ RATIONALE ]:  Only once incremented would nodes proceed with a full resynchronization procedure from Genesis Block.
													    	Nodes store the prior FORCED_RESYNC_SEQUENCE_NUMBER and verify during the initial Bootstrap Sequence.
															Since we use checkpoints, certain data in previous blocks might be invalid. It is thus important to mine
														    a single block (by a trusted valid node) and the reference it through a checkpoint. We currently employ a dedicated type of Key-Block which
															contains no PoW. Thus any histories of events not containing this Hard Fork Block - those would not be honored.
							 3B) [ Make Hard Fork Leader Operational ] 
									[ RATIONALE ]: the Hard Fork Leader is to continue accepting and processing TX until others become Operational.
							
								 - reboot node
				
								 - [PAY ATTTENTION]:  you would be asked whether to do a full resync (due to a higher checkpoint's counter) - answer NO
												   [ RATIONALE ]: Otherwise the entire state would be lost. Others would have nothing to sync to. We need others to be able to
																  synchronsie with he history of events which is past the Hard Fork point and the Hard Fork Leader is the only
																  node which is now in posession of such a history of events. It is now also the only one capable of confirming
																  transactions..

										[ LIVENESS ]: Liveness of the system is now already restored. Tranasctions would be processed by the Leading Node until the Release Candidate
													  is propagated across other GPU capable nodes. Follow the distrubution order describe below.
								  - execute 'net -all' to re-enable connectivity
								  - enable native OS network card (if disabled)
							 3C) After the above is COMPLETED

							 4) Update other nodes participating in this Phase. Launch them as normal.
		
							 5) Verify difficulty reported by nodes.
							 6) Verify integrity of the achieved Perspective at all nodes participating in Phase [0-1].
								(use 'chain -show -block recent' and verify the EFFECTIVE perspective to be the same across nodes involved).


							7) [ DNS Phase 1] [ Asynchronous Task ]: continue re-adding IPs of Tier 0 nodes as these synchronize with the state of the Leading Node.
									            Use the list of backed up IPs. Nodes are considered as Operational if they at at most 5 blocks behind the Leading Node.
								[ Rationale ]: we want mobile apps to beging connecting with other Tier 0  nodes as soon as these become Operational.
							 
							 [ PHASE 2 ]
							 1) Update GRIDNET Core on nodes not participating in Phase 2.
							 2) Restart these.
							 3) As soon as these nodes synchronize the Hard Fork is now Complete.

							 [ INTERNAL - END ]

							 [ GLOBAL - BEGIN ] 

							 [ PHASE 3 ]
							 0) use Installer Project to prepare new redistributable package. Pack the two resulting files as 'GRIDNETCore.zip'
							 1) Publish the Release Notes.
							 2) [Software Release]  distribute in the following order. Ensure at least 2 hour gap between groups.
									- first to Tier 1 Operators
									- then to Tier 2  Operators
									- then to Tier 3  Operators
									- then Public Release 
										- uploaded GRIDNETCore.zip to gridnet.org (main www folder)
									    - update the  Release Notes of GRIDNET Core software to now include a download link

							 [ GLOBAL - END ]

							*/
							// [ Hard-Fork Procedure ] - END
						}
					}
					//Hard-Fork Support - END

					bm->incLocalTotalRewardBy(blockProposal->getHeader()->getTotalBlockReward());
					bm->incLocallyMinedKeyBlocks();
				}
				else
				{
					// Do not increment local stats in Phantom Leader Mode
					// Phantom blocks are simulations and should not affect real statistics
					if (!proceedAsPhantomLeader)
					{
						bm->incLocallyMinedDataBlocks();

						if (keyBlock)
							incKeyBlocksGeneratesUsingSamePubKey();
						else
							incDataBlocksGeneratesUsingSamePubKey();
					}
				}
			}

			//BLOCK-FORMATION - END

			if (blockProposal != nullptr && bfr == blockFormed)
			{
				std::vector<uint8_t> blockID = blockProposal->getID();
				setBlockFormationStatus(keyBlock, eBlockFormationStatus::enqueuingForProcessing);

				tools->writeLine("My " + std::string((blockProposal->getHeader()->isKeyBlock() ? "KEY" : "DATA"))
					+ std::string(" block heading for processing! ID: ") + tools->base58CheckEncode((blockID)));

				//Packing the block enqueueing to the Blockchain Manager
				//the serialization/de-serialization takes place mainly as an additional 'check'
				//there's an overload accepting a instance of a block also.

				std::vector<uint8_t> serializedBlock;
				blockProposal->getHeader()->setTriesLoaded(true);//just a presumption
				assertGN(blockProposal->getPackedData(serializedBlock));
				blockProposal->prepareForRemoval(); //remove circular dependencies

				if (serializedBlock.size() > 1000)
					tools->writeLine("Block size:" + std::to_string(serializedBlock.size() / 1000) + " kBs");
				else
					tools->writeLine("Block size:" + std::to_string(serializedBlock.size()) + " Bytes");

				// ============================================================================
				// Phantom Leader Mode Block Handling - BEGIN
				// ============================================================================
				// [ Behavior ]: In Phantom Mode, the block is NOT dispatched to the network.
				//               Instead, a detailed processing report is generated and printed.
				//               This allows debugging transaction processing without affecting
				//               the actual blockchain state or network consensus.
				// ============================================================================
				if (proceedAsPhantomLeader && !keyBlock)
				{
					// Update phantom mode statistics
					bm->incPhantomBlocksFormedCount();
					bm->addPhantomTransactionsProcessedCount(blockProposal->getHeader()->getNrOfTransactions());
					bm->pingLastPhantomBlockFormationTime();

					// Calculate processing time (approximate based on block timestamp)
					uint64_t processingTimeMs = (std::time(0) - blockProposal->getHeader()->getSolvedAtTime()) * 1000;
					if (processingTimeMs > 60000) processingTimeMs = 0; // Sanity check

					// Collect receipts from the block for the report
					std::vector<CReceipt> phantomReceipts = blockProposal->getReceipts();

					// Generate and display the detailed phantom block report
					std::string phantomReport = bm->generatePhantomBlockReport(
						blockProposal,
						phantomReceipts,
						processingTimeMs);

					tools->writeLine(phantomReport, true, false);

					// Log the phantom block formation event
					tools->logEvent(
						"Phantom block formed with " +
						std::to_string(blockProposal->getHeader()->getNrOfTransactions()) +
						" transactions. Block ID: " + tools->base58CheckEncode(blockID) +
						" (NOT broadcasted)",
						"Phantom Mode",
						eLogEntryCategory::localSystem,
						5,
						eLogEntryType::notification,
						eColor::orange
					);

					// Display phantom-processed transaction tracking info
					// Note: Transactions are marked as phantom-processed inside processTransactions()
					// at the exact point where they would be marked as processed in normal mode.
					size_t phantomTrackedCount = getPhantomProcessedTransactionCount();
					tools->writeLine(tools->getColoredString(
						"[PHANTOM MODE] Total transactions tracked as phantom-processed: " +
						std::to_string(phantomTrackedCount),
						eColor::greyWhiteBox));

					// Important: Do NOT push the block to the blockchain manager
					// The block is discarded after the report is generated
					tools->writeLine(tools->getColoredString(
						"[PHANTOM MODE] Block was NOT dispatched. Transactions remain in mem-pool.",
						eColor::lightPink));
				}
				else
				{
					// Normal operation: dispatch the block to the network
					tools->writeLine("I'm dispatching the newly formed block to the Blockchain Manager for consideration.");

					if (bm->pushBlock(blockProposal, true))//we push an instance so that the ephemeral hard-fork flags can prevail.
					{
						if (blockProposal->getIsLeadingAHardFork())
						{
							tools->writeLine("Hard Fork Block dispatched for processing..", true, true, eViewState::GridScriptConsole, "Hard Fork");
							setHardForkBlockAlreadyDispatched();
						}
						if (getSynchronizeBlockProduction())
						{
							while (!bm->wasBlockProcessed(blockID))
								std::this_thread::sleep_for(std::chrono::milliseconds(100));//wait till the block is processed to prevent wasted PoW
							//data blocks can be formed right away. the transactions and verifiables are removed from pools upon processing right away
						}
					}
				}
				// Phantom Leader Mode Block Handling - END
				// ============================================================================

				//BLOCK PROPOSAL - END

				//setBlockFormationStatus(keyBlock, eBlockFormationStatus::processing);//Note: a-synchronous

			}
		}
		else
		{
			if ((tools->getTime() - lastTimeInformedProductionHalted) > 10)
			{
				tools->writeLine("Block production will resume in " + std::to_string(tools->getTime() - mBlockProductionHaltedTillTimeStamp) + " sec");
				lastTimeInformedProductionHalted = tools->getTime();
			}
		}
		setBlockFormationStatus(keyBlock, eBlockFormationStatus::idle);
	}

	//Operational Logic - END
}

std::shared_ptr<CBlockchainManager> CTransactionManager::getBlockchainManager()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mBlockchainManager;
}

void CTransactionManager::abortKeyBlockFormation()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mDoBlockFormation = false;
}
void CTransactionManager::abortDataBlockFormation()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mDoBlockFormation = false;
}

/// <summary>
/// Starts the Flow.
/// Starting the flow related to key-blocks requires locking the mChainGuardian mutex,
/// since calculation of previous epoch-reward might be required (and thus access to cache needed).
/// </summary>
/// <param name="initialPerspective"></param>
/// <param name="abortOnLock"></param>
/// <returns></returns>
bool CTransactionManager::startFlow(std::vector<uint8_t> initialPerspective, bool abortOnLock)
{
	//Is it a Flow Manager - BEGIN
	//Notice: Terminal and LIVE Transaction Managers cannot enter Flow (ACID) mode.
	std::shared_ptr<CTransactionManager> flowManager;
	switch (getMode())
	{
	case eTransactionsManagerMode::FormationFlow:
		flowManager = shared_from_this();
		break;
	case eTransactionsManagerMode::VerificationFlow:
		flowManager = shared_from_this();
		break;
	case eTransactionsManagerMode::LIVE:
		return false;
		break;
	case eTransactionsManagerMode::Terminal:
		return false;
		break;
	default:return false;
		break;
	}
	//Is it a Flow Manager - END

	bool wasWaiting = false;
	bool flowStateDBLocked = false;
	if (!abortOnLock)
	{
		wasWaiting = true;
		bool inFlow = false;
		bool somebodyElseSettingUpFlow = false;
		uint64_t waitStart = std::time(0);
		uint64_t now = std::time(0);
		uint64_t lastReport = 0;
		std::shared_ptr<CTools> tools = getTools();

		do 
		{
			now = std::time(0);
			//check if Flow mechanism does not seem stuck - BEGIN
			if ((now - waitStart > 60) && (now-lastReport) > 10 )
			{
				tools->logEvent("Warning: Flow mechanism seems stuck ("+ std::to_string(now - waitStart)+" sec", eLogEntryCategory::localSystem, 10, eLogEntryType::warning);
				lastReport = now;
				getBlockchainManager()->setVitalsGood(false);
			}
			//check if Flow mechanism does not seem stuck - END

			//lock the mutex responsible for initial status check (is Flow mechanism locked?)
			mInitialStatusQueryGuardian.lock();//prevents any other thread to indicate either 'waiting' or 'locked' status.

			inFlow = getIsInFlow();//check if anybody is in Flow already.
			somebodyElseSettingUpFlow = getIsSomebodyElseSettingUpFlow(); //anyone already locked the Start Flow mechanics already? besides us?
										//^ would return FALSE for this thread.

			if (!somebodyElseSettingUpFlow)//if nobody else is waiting then indicate that we are about to start the Flow.
				//notice that there might be multiple threads 'waiting', we are effectively implementing a binary semaphore-like 
				//mechanism. Other threads would be simply sleeping and waiting to acquire through setIsSettingUpFlow(true).

				setIsSettingUpFlow(true);//needed so that nobody starts the flow during the 30msec wait-time below
			//this should not block, so all threads actually wait not wasting CPU time
			if (inFlow || somebodyElseSettingUpFlow)
				mInitialStatusQueryGuardian.unlock();//do NOT unlock yet if we can proceed, OTHERWISE unlock and keep waiting. We need to notify others that we are beginning a Flow in a thread-safe way.
											//^ unlock and keep waiting.

			std::this_thread::sleep_for(std::chrono::milliseconds(3));
		} while (inFlow || somebodyElseSettingUpFlow);

		getBlockchainManager()->setVitalsGood(true);
		//mWaitingForFlowGuardian.lock();//now this can block till everything is ready to lock the mFlowGuardian
	}
	
		getBlockchainManager()->lockChainGuardian();

	if (getIsInFlow() && !abortOnLock)
	 assertGN(false);//this should not happen

	if(!getIsInFlow())
	{
		setIsSettingUpFlow(true);
		mInitialStatusQueryGuardian.unlock();
		std::lock_guard<std::recursive_mutex>	lockA(mStateDomainManager->mCurrentDBGuardian);
		std::lock_guard<std::recursive_mutex>   lock1(mFlowStateDBGuardian);
		std::lock_guard<std::recursive_mutex>	lock2(mFlowProcessingGuardian);
		mFlowStateDB->lock();//IF this function does not succeed, the Flow DB *MUST* be unlocked before this function exits.
							   //In such a case, endFlow() or abortFlow() NEED NOT be called.

		flowStateDBLocked = true;


		std::lock_guard<std::recursive_mutex>	lock4(mCurrentDBGuardian);
	

	assertGN(mInFlowInterDBs.size() == 0);
	assertGN(mBackupStateDB == NULL);
	//cleanupFlow(true);
	//mFlowStateDB->setPerspective(mLiveStateDB->getPerspective());
	if (initialPerspective.size() > 0)
	{
		if (initialPerspective.size() != 32)//initial Perspective needs to be known (typically the one from current Leader).
		{
			//Clean State - BEGIN
			getBlockchainManager()->unlockChainGuardian();//typically endFlow() and abortFlow() would take care of this.
			setIsSettingUpFlow(false);

			if (flowStateDBLocked)
			{
				flowStateDBLocked = false;
				mFlowStateDB->unlock();//unlock ONLY on error.
			}
			//Clean State - END

			return false;
		}

		if (!mFlowStateDB->setPerspective(initialPerspective))
		{
			//Clean State - BEGIN
			getBlockchainManager()->unlockChainGuardian();//typically endFlow() and abortFlow() would take care of this.
			setIsSettingUpFlow(false);

			if (flowStateDBLocked)
			{
				flowStateDBLocked = false;
				mFlowStateDB->unlock();//unlock ONLY on error.
			}
			//Clean State - END
			return false;
		}
	}else
	mFlowStateDB->pruneTrie(true,false,true,false);//prune Trie so just to make sure that we are not copying over any boiler-plate state domains over and over

	//during consecutive steps.
	mCurrentStateDB = mFlowStateDB;
	
	mScriptEngine->setTerminalMode(false);
	mScriptEngine->reset();//just to make sure
	mScriptEngine->setRegN(REG_KERNEL_THREAD);
	mScriptEngine->setTextualOutputEnabled(false);
	mBackupStateRootID = mFlowStateDB->getPerspective();
	getTools()->writeLine("Starting the Flow; Perspective: " + getTools()->base58CheckEncode(mBackupStateRootID));
	CTrieDB *inFlowTempDB = new CTrieDB(*mFlowStateDB);
	
	//assert(mFlowStateDB->copyTo(inFlowTempDB, true));
    assertGN(std::memcmp(inFlowTempDB->getPerspective().data(), mFlowStateDB->getPerspective().data(), 32) == 0);
	
	// Flow Backup (3/4) - BEGIN ( Level 0 )
	// [ Rationale ]: so that Flow mechanics can revert to a state right before any Flow processing has taken effect.
	pushDB(inFlowTempDB, false); 
	// Flow Backup (3/4) - END

	//also keep the backup DB separate - it might get removed from the queue
	mBackupStateDB = new CTrieDB(*mFlowStateDB);
	if (!mFlowStateDB->isEmpty(false));

    assertGN(std::memcmp(mBackupStateDB->getPerspective().data(), mFlowStateDB->getPerspective().data(), 32) == 0);
	mScriptEngine->setRegN(REG_KERNEL_THREAD);
	mScriptEngine->enterSandbox();
	mInSandbox = true;
	mStateDomainManager->enterFlow();
	mStateDomainManager->getDB()->clearStateDomainsModifiedDuringTheFlow();
	mVerifier->setFlowManager(flowManager);
	mVerifier->enterFlow();

	setIsInFlow(true);
	setIsSettingUpFlow(false);

	return true;
	}
	else
	{
		//already on Flow.
		mInitialStatusQueryGuardian.unlock();
		return false;
	}	
}

/// <summary>
/// Ends the current Transaction Flow.
/// IF commit not set then transaction results are NOT committed onto the Solid Storage.
/// IF you want to keep explicit changes done to HotStorage in between the calls to StartFlow and EndFlow to be kept then
/// set doInitialCleanUp to FALSE.
/// </summary>
/// <param name="commit"></param>
/// <param name="receipts"></param>
/// <returns></returns>
CTransactionManager::eDBTransactionFlowResult CTransactionManager::endFlow(bool commit,
	bool doInitialCleanUp,
	bool updateKnownStateDomains,
	const std::string &errorMsg, 
	const std::vector<CReceipt> &receipts,
	const  std::vector<uint8_t>&finalPerspective,
	bool abortP,
	std::shared_ptr<CBlock> blockProposal,
	bool doTrieVerification,
	const  BigInt& finalTotalBlockRewardEffective,
	const  BigInt& finalPaidBlockRewardEffective
)
{
	if (!getIsInFlow())
		return CTransactionManager::eDBTransactionFlowResult::failure;
	std::lock_guard<std::recursive_mutex> lockA(mFlowGuardian);	// was there an attempt to abort an unowned Flow? in such a case this would block
	if (!getIsInFlow())
	{
		return failure;
	}
	std::shared_ptr<CTools> tools = CTools::getInstance();
	bool doTurboFlow = false && ((blockProposal ? blockProposal->getDoTurboFlow() : false) && commit);
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	// RAII Logging - BEGIN
	std::stringstream debugLog;
	// Create RAII logger. It writes out debugLog at every return (normal or early).
	CEndFlowRAII endFlowLogger(debugLog, getTools());


	// Log incoming arguments & current perspective
	debugLog << tools->getColoredString("[endFlow] (ACID)", eColor::lightCyan) << " called with " + tools->getColoredString("=>", eColor::blue) <<"\n" 
		<< tools->getColoredString("Arguments", eColor::neonBlue)<< " --------------"
		<< "\n - Attempt Commit: " << (commit ? "yes" : "no")
		<< "\n - Turbo Flow: " << (doTurboFlow ? "yes" : "no")
		<< "\n - Initial Clean-Up: " << (doInitialCleanUp ? "yes" : "no")
		<< "\n - Updating Known Domains: " << (updateKnownStateDomains ? "yes" : "no")
		<< "\n - Error Message: " << (errorMsg.empty() ? "none" : errorMsg)
		<< "\n - Receipts Count: " << receipts.size()
		<< "\n - In-Flow Transactions: " << mInFlowTransactions.size()
		<< "\n - In-Flow Verifiables: " << mInFlowVerifiables.size()
		<< "\n - In-Flow Links: " << mInFlowLinks.size()
		<< "\n - Abort: " << (abortP ? "yes" : "no")
		<< "\n - Block Provided: " << (blockProposal ? "yes" : "no")
		<< "\n - Trie Verification: " << (doTrieVerification ? "enabled" : "disabled")
		<< "\n   [endFlow] Initial Perspective: " << tools->base58CheckEncode(mFlowStateDB->getPerspective())
		<< tools->getColoredString("\n\nFlow (ACID) Execution Follows", eColor::neonBlue) << " ----v\n"
		<< std::endl;

	debugLog << "----------- " << tools->getColoredString("Pre-Commit Stage -  BEGIN", eColor::orange) << " ----------- " << std::endl;
	// RAII Logging - END
#endif


	bool error = false;
	//uint64_t balanceTotal = mStateDomainManager->analyzeTotalDomainBalances();
	std::lock_guard<std::recursive_mutex>	lockB(mStateDomainManager->mCurrentDBGuardian);
	std::lock_guard<std::recursive_mutex>   lock1(mFlowStateDBGuardian);
	std::lock_guard<std::recursive_mutex>	lock2(mCurrentDBGuardian);
	//std::lock_guard<std::recursive_mutex>	lock3(mFlowStateDB->mGuardian);
	std::lock_guard<std::recursive_mutex>	lock4(mFlowProcessingGuardian);

	 doTrieVerification = true;
	 eDBTransactionFlowResult result = success;

	 // ------------------------------- CRITICAL -------------------------------
	 BigInt rewardUptilNow = 0;
	 BigInt rewardIssuedToMinerAfterTAX = 0;
	 
		 /*
			 IMPORTANT: [ SECURITY ]: Epoch 1 and Epoch 2 rewards are to be already accounted for as part of rewardValidatedUpToNow
						BEFORE this method is called.

						[ SECURITY ]:  Utlimately, Key Block Operator rewards are issued based on the rewardValidatedUpToNow value.
									   It accounts for both Epoch rewards and TX processing fees.
		 */

	 // ------------------------------- CRITICAL -------------------------------

	 tools->writeLine("Ending the " + std::string(doTurboFlow ? "Turbo" : "Standard") + " Flow");
	std::vector<std::shared_ptr<CVerifiable>> leaderVerifiables;
	//assertGN(!(commit && abort));
		
	//First we need to roll back any changes done to HotStorage so far during the flow
	if(doInitialCleanUp)
	cleanupFlow(false,true,false);

	CTrieDB *inFlowTempDB = new CTrieDB(*mFlowStateDB);
	//if (!mFlowStateDB->isEmpty(false));
	//mFlowStateDB->copyTo(inFlowTempDB, false);
	bool allGood = std::memcmp(inFlowTempDB->getPerspective().data(), mFlowStateDB->getPerspective().data(), 32) == 0;
	assertGN(allGood);
	//mInFlowInterDBs.push_back(inFlowTempDB);

	// Flow Backup (4/4) - BEGIN ( Level 0 )
	// [ Rationale ]: so that Flow mechanics can revert to a state right before commit operation began.
	pushDB(inFlowTempDB, false);
	// Flow Backup (4/4) - END

	std::vector<CReceipt> lReceipts;
	std::vector<uint8_t>issuersID;
	std::string news = "Finishing a Flow with " + std::to_string(mInFlowTransactions.size()) + " transactions";
	tools->writeLine(news);
	tools->writeLine(news);
std::vector<CReceipt*> receiptsP;

std::vector<uint8_t> initialPerspective = getPerspective(true);
//  Standard Flow - BEGIN
std::vector<uint8_t> perspectiveAfterTransactions;
std::vector<uint8_t> perspectiveAfterVerifiables;

if (!doTurboFlow)
{

	assertGN(getInFlowTXDBStackDepth(true)==0); // the total Tranasctions' State Stack Depth needs to be equal to 0 bebefore proceeding with any transactions

	//NOTE: DO NOT MARK TRANSACTIONS/VERIFIABLES AS PROCESSED JUST YET => they might need to be committed
	if (!processTransactions(mInFlowTransactions, const_cast<std::vector<CReceipt> &>(receipts), false, blockProposal, false, true, !commit))
	{
		commit = false;
		result = failure;
		doTrieVerification = false;
	}

	// Post-Processing Validation - BEGIN
			// Verify stack is clean after processing
			// All transaction states must be either cleaned up (successful) 
			// or reverted (failed), leaving total stack empty
	assertGN(getInFlowTXDBStackDepth(true) == 0);

	// Verify Transaction Accounting
	volatile uint64_t cleanedTXs = getTXsCleanedCount();    // Successfully processed
	volatile uint64_t revertedTXs = getTXsRevertedCount();  // Failed and reverted
	// Total of cleaned and reverted must match the number of transactions scheduled for processing
	assertGN(cleanedTXs + revertedTXs == mInFlowTransactions.size());
	// Post-Processing Validation - END

	perspectiveAfterTransactions = getPerspective(true);

	if (result != failure)
	{

		for (int i = 0; i < receipts.size(); i++)
		{
			rewardUptilNow += const_cast<std::vector<CReceipt> &>(receipts)[i].getERGPrice() * receipts[i].getERGUsed();
		}

		if (mTotalGNCFees != rewardUptilNow)
		{
			if (blockProposal)
			{
				if (blockProposal->getIsCheckpointed())
				{
					tools->writeLine(tools->getColoredString("Invalid fees but block let through due to a checkpoint.", eColor::orange));
					//turns of fees have changes or results change changes. let it be.
				}
				else
				{
					tools->writeLine(tools->getColoredString("Invalid fees. No checkpoint. Flow failed.", eColor::lightPink));
					commit = false;
					result = failure;
					doTrieVerification = false;
				}
			}
			else {
				assertGN(false);
			}
		}

		// Safety check: ensure mInFlowVerifiables is not empty before accessing
		if (mInFlowVerifiables.empty())
		{
			commit = false;
			result = failure;
			doTrieVerification = false;
			tools->writeLine(tools->getColoredString("Flow failed: No verifiables available (mInFlowVerifiables empty).", eColor::cyborgBlood));
		}
		else
		{
			leaderVerifiables.push_back(mInFlowVerifiables[mInFlowVerifiables.size() - 1]);
			if (leaderVerifiables[0]->getVerifiableType() != eVerifiableType::minerReward)
			{
				commit = false;
				result = failure;
				doTrieVerification = false;

			}

			mInFlowVerifiables.pop_back();
		}
		//process all the verifiables without the Leaders-Reward
		//NOTE: DO NOT MARK TRANSACTIONS/VERIFIABLES AS PROCESSED JUST YET => they might need to be committed
		if (!processVerifiables(mInFlowVerifiables, const_cast<std::vector<CReceipt> &>(receipts), rewardUptilNow, rewardIssuedToMinerAfterTAX, false, blockProposal, false, true))
		{
			commit = false;
			result = failure;
			doTrieVerification = false;
		}

		// LEADER REWARDS' Verifiable Processing - BEGIN
		// Rewards for both Epoch 1 and 2.
		// Needs to be the last Verifiable in a Key-Block.
		// [ IMPORTANT ]: both Key Blocks and Data Blocks employ Miner's Rewards verifiables.
		//				  Data Blocks need Miner's Rewards Verifiables to issue TX processing fees to Operators immediatedly upon block's processing.


			// Epochs' Processing - BEGIN
			// [ SECURITY ]: here we provision final current operator's rewards.
		BigInt epoch1Reward = 0;
		BigInt epoch2Reward = 0;

		// Epoch 1 - BEGIN
		// Solely Current Block Candidate
		if (!mBlockchainManager->getEpochReward(blockProposal, epoch1Reward, rewardUptilNow, eEpochRewardCalculationMode::ALeader))
		{
			commit = false;
			result = failure;
		}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
		debugLog << " [- Epoch 1 Reward -]: " << tools->formatGNCValue(epoch1Reward) << "\n";
		debugLog << " [ Total Cumulative Reward Stage #1 ]: " << tools->formatGNCValue(rewardUptilNow) << "\n";
#endif


		rewardUptilNow += epoch1Reward;

		// Epoch 1 - END

		if (blockProposal->getHeader()->isKeyBlock())
		{
			// Epoch 2 - BEGIN
			// Parent Key Block -> Data Block(s) -> Current Block Candidate
			if (!mBlockchainManager->getEpochReward(blockProposal, epoch2Reward, rewardUptilNow, eEpochRewardCalculationMode::BLeader))
			{
				commit = false;
				result = failure;
			}

			rewardUptilNow += epoch2Reward;
			// Epochs' 2 - END
			// Epoch Processing - END
		}
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
		debugLog << " [- Epoch 2 Reward -]: " << tools->formatGNCValue(epoch2Reward) << "\n";
		debugLog << " [ Total Cumulative Reward Stage #2 ]: " << tools->formatGNCValue(rewardUptilNow) << "\n";
#endif

		if (!processVerifiables(leaderVerifiables, const_cast<std::vector<CReceipt> &>(receipts), rewardUptilNow, rewardIssuedToMinerAfterTAX, false, blockProposal, false, true))
		{
			commit = false;
			result = failure;
			doTrieVerification = false;
		}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
		debugLog << " [ Total Cumulative Reward Stage #3 ]: " << tools->formatGNCValue(rewardUptilNow) << "\n";
#endif
		//revert the leader-verifiable
		mInFlowVerifiables.push_back(leaderVerifiables[0]);
		leaderVerifiables.clear();

		// LEADER REWARDS' Verifiable Processing - END 

	}

	perspectiveAfterVerifiables = getPerspective(true);

	//processReceipts(mInFlowRe)
	//if commit is enabled then we need to enable permanent effects during GridScript execution and verifiables validation
	const_cast<std::vector<uint8_t>&>(finalPerspective) = getPerspective(true);
	if (doTrieVerification)
	{
		mFlowStateDB->recalcPerspective();
		std::vector<uint8_t> confirmedPerspective = getPerspective(true);
		if (std::memcmp(finalPerspective.data(), confirmedPerspective.data(), 32) != 0)
		{
			tools->writeLine("The resulting perspective was corrupt");
			commit = false;
		}
	}
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	debugLog << "[ Final Perspective ]: " << tools->base58CheckEncode(mFlowStateDB->getPerspective()) << std::endl;
	debugLog << "----------- " << tools->getColoredString("Pre-Commit Stage -  END", eColor::orange) << " ----------- \n";
#endif
}
//  Standard Flow - END

if (!commit)
		{
			
			tools->writeLine("It was chosen not to commit the Flow.");
			//lets just leave silently
			cleanupFlow(true, abortP);
		}
		else
		{

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	debugLog << "----------- " << tools->getColoredString("Commit Stage -  BEGIN", eColor::headerCyan) << " ----------- \n"; 
#endif
		// ------------------------------   COMMIT - BEGIN ------------------------------




		std::shared_ptr<CVerifiable> rewardVerifiable;
		rewardUptilNow = 0;
		rewardIssuedToMinerAfterTAX = 0;
			tools->writeLine("It was chosen to COMMIT the Flow. I shall proceed.");
			cleanupFlow(false,doInitialCleanUp,false);
		 assertGN(mScriptEngine->leaveSandbox());
			mInSandbox = false;
			const_cast<std::vector<CReceipt>&>(receipts).clear();
			std::vector<CReceipt> receiptsStub;//these would be the same as prior. perspective verification ensures this.
		
			if (std::memcmp(getPerspective(true).data(), initialPerspective.data(), 32) != 0)
			{
				const_cast<std::string&>(errorMsg) = "DB in an invalid initial perspective during endflow processing.";
				getBlockchainManager()->reportCryticalError(errorMsg);
				error = true;
				result = failure;
			}
			if (!error)
			{
				assertGN(getInFlowTXDBStackDepth(true) == 0); // the total Tranasctions' State Stack Depth needs to be equal to 0 bebefore proceeding with any transactions

				processTransactions(mInFlowTransactions, const_cast<std::vector<CReceipt> &>(receipts), true,blockProposal,false,true, false);

				// Post-Processing Validation - BEGIN
				// Verify stack is clean after processing
				// All transaction states must be either cleaned up (successful) 
				// or reverted (failed), leaving total stack empty
				assertGN(getInFlowTXDBStackDepth(true) == 0);

				// Verify Transaction Accounting
				volatile uint64_t cleanedTXs = getTXsCleanedCount();    // Successfully processed
				volatile uint64_t revertedTXs = getTXsRevertedCount();  // Failed and reverted
				// Total of cleaned and reverted must match the number of transactions scheduled for processing
				assertGN(cleanedTXs + revertedTXs == mInFlowTransactions.size());
				// Post-Processing Validation - END


				if (!doTurboFlow && std::memcmp(getPerspective(true).data(), perspectiveAfterTransactions.data(), 32) != 0)
				{
					const_cast<std::string&>(errorMsg) = "DB in an invalid state after transaction processing (endflow).";
					getBlockchainManager()->reportCryticalError(errorMsg);
					error = true;
					result = failure;
				}
				for (int i = 0; i < receipts.size(); i++)
				{
					rewardUptilNow += const_cast<std::vector<CReceipt> &>(receipts)[i].getERGPrice()* receipts[i].getERGUsed();
				}
			}
			if (!error)
			{
				//------------------
				if (!mInFlowVerifiables.empty())
				{
					rewardVerifiable = mInFlowVerifiables[mInFlowVerifiables.size() - 1];
					if (rewardVerifiable->getVerifiableType() != eVerifiableType::minerReward)
					{
						error = true;
						commit = false;
						result = failure;
						doTrieVerification = false;

					}
				}
			}
			if (!error)
			{
				if (!mInFlowVerifiables.empty())
				{
					mInFlowVerifiables.pop_back();
					//process all the verifiables without the Leaders-Reward
					if (!processVerifiables(mInFlowVerifiables, const_cast<std::vector<CReceipt> &>(receipts), rewardUptilNow, rewardIssuedToMinerAfterTAX, true, blockProposal, false, true))
					{
						commit = false;
						result = failure;
						error = true;
						doTrieVerification = false;
					}
				}
			}
			if (!error)
			{
				// LEADER REWARDS' Verifiable Processing - BEGIN
				// Rewards for both Epoch 1 and 2.
				// Needs to be the last Verifiable in a Key-Block.

				
					// Epochs' Processing - BEGIN
					// [ SECURITY ]: here we provision final current operator's rewards.
					BigInt epoch1Reward = 0;
					BigInt epoch2Reward = 0;

					// Epoch 1 - BEGIN
					// Solely Current Block Candidate
					if (!mBlockchainManager->getEpochReward(blockProposal, epoch1Reward, rewardUptilNow, eEpochRewardCalculationMode::ALeader))
					{
						commit = false;
						result = failure;
					}

					rewardUptilNow += epoch1Reward;

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					debugLog << " [- Epoch 1 Reward -]: " << tools->formatGNCValue(epoch1Reward) << "\n";
					debugLog << " [ Total Cumulative Reward Stage #4 ]: " << tools->formatGNCValue(rewardUptilNow) << "\n";
#endif

					// Epoch 1 - END
					if (blockProposal->getHeader()->isKeyBlock())
					{

						// Epoch 2 - BEGIN
						// Parent Key Block -> Data Block(s) -> Current Block Candidate
						if (!mBlockchainManager->getEpochReward(blockProposal, epoch2Reward, rewardUptilNow, eEpochRewardCalculationMode::BLeader))
						{
							commit = false;
							result = failure;
						}

						rewardUptilNow += epoch2Reward;
						// Epochs' 2 - END
					}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					debugLog << " [- Epoch 2 Reward -]: " << tools->formatGNCValue(epoch1Reward) << "\n";
					debugLog << " [ Total Cumulative Reward Stage #5 ]: " << tools->formatGNCValue(rewardUptilNow) << "\n";
#endif
					
					// Epoch Processing - END
					if (rewardVerifiable)
					{
						mInFlowVerifiables.clear();
						mInFlowVerifiables.push_back(rewardVerifiable);
					}

					if (!processVerifiables(mInFlowVerifiables, const_cast<std::vector<CReceipt> &>(receipts), rewardUptilNow, rewardIssuedToMinerAfterTAX, true, blockProposal, false, true))
					{
						commit = false;
						result = failure;
						error = true;
						doTrieVerification = false;
					}
				
				// LEADER REWARDS' Verifiable Processing - END
			}
			if (!error)
			{
				if (!doTurboFlow &&  std::memcmp(getPerspective(true).data(), perspectiveAfterVerifiables.data(), 32) != 0)
				{
					const_cast<std::string&>(errorMsg) = "DB in an invalid state after verifiables processing (endflow).";
					getBlockchainManager()->reportCryticalError(errorMsg);
			
					error = true;
					result = failure;
				}
			}
				//Links are saved to Cold-Storage ONLY when it was decided that the Flow is to be commited.
		//Links do not affect the Perspective in any way.
		//Links are helper-data structures. Still they are essential for functioning of various mechanics (Receipts, Proof-of-Fraud etc.)
			if(!error)
			{
			mInFlowLinksGuardian.lock();
				for (uint64_t i = 0; i < mInFlowLinks.size(); i++)
				{
					if (!getBlockchainManager()->getSolidStorage()->saveLink(mInFlowLinks[i]))
					{
						error = true;
						break;
					}
				}
				mInFlowLinksGuardian.unlock();
				if (error)
				{
					const_cast<std::string&>( errorMsg) = "Crytical error occured.";
					getBlockchainManager()->reportCryticalError(errorMsg);
				
					error = true;
					result = failure;
				}
				//------------------
			}
		
			if (!error)
			{
				tools->writeLine("Processing StateDomain's backlog..");
				std::vector < std::vector<uint8_t>> modifiedDomains = mStateDomainManager->getDB()->getStateDomainsModifiedDuringTheFlow();
				tools->writeLine("There were " + std::to_string(modifiedDomains.size()) + " domains modified during the Flow");
				CStateDomain *domain = nullptr;
				std::vector<uint8_t> initialHash;
				uint64_t deletedBacklogs = 0;

				for (const auto &d : modifiedDomains)
				{

					domain = mStateDomainManager->findByID(d);

					if (domain == nullptr)
					{
					 assertGN(false);
					}
					std::vector<uint8_t> hashBeforeBacklogUpdate = domain->getHash();
				 assertGN(domain != nullptr);
				 assertGN(mStateDomainManager->getDB()->getStateDomainInitialHash(d, initialHash));
					if (domain->addPreviousBodyID(initialHash, perspectiveAfterVerifiables,true))
					{
						deletedBacklogs++;
					}
					domain->updateTimeStamp();
					std::vector<uint8_t> hashAfterBacklogUpdate = domain->getHash();
				 assertGN(std::memcmp(hashBeforeBacklogUpdate.data(), hashAfterBacklogUpdate.data(), 32) == 0);
				 assertGN(getBlockchainManager()->getSolidStorage()->saveNode(domain->getHash(), domain->getPackedData(), mCurrentStateDB->getSSPrefix()));

				}
				tools->writeLine("Deleted " + std::to_string(deletedBacklogs) + " State Domain backlogs");

				mStateDomainManager->getDB()->clearStateDomainsModifiedDuringTheFlow();
				tools->writeLine("StateDomain's backlog processing finished.");


				tools->writeLine("Results of the Flow were commited to the Cold Storage. ");
			}
			// ------------------------------   COMMIT - END ------------------------------
		}
if(getIsInFlow())
{
	if (error)
	{
		tools->writeLine("Errors during flow, reverting..");
	}
	cleanupFlow(true, error);
	
	mStateDomainManager->exitFlow(updateKnownStateDomains);

		//getBlockchainManager()->getStateDomainManager()->setKnownStateDomainIDs(mStateDomainManager->getKnownDomainIDs());
	mVerifier->exitFlow();
	mCurrentStateDB = mLiveStateDB;


}
mFlowStateDB->pruneTrie(true, false, true, false);
tools->writeLine("The Flow was successful! Final perspective: "+ tools->base58CheckEncode(finalPerspective));


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
if (commit)
{
	debugLog << "[ Flow Commit Final Perspective ]: " + tools->base58CheckEncode(finalPerspective) << std::endl;

	if (errorMsg.empty() == false)
	{
		debugLog << tools->getColoredString("[ Flow Error ]: ", eColor::cyborgBlood) << errorMsg << std::endl;
	}
	debugLog << "[ Result ]: " << (error ? tools->getColoredString(u8"🛑 Error", eColor::cyborgBlood) : tools->getColoredString(u8"✅ Success ", eColor::lightGreen)) << std::endl;
	debugLog << "----------- " << tools->getColoredString("Commit Stage -  END", eColor::headerCyan) << " ----------- " << std::endl;
}
else
{
	debugLog << "Commit operation omitted./n";
}
#endif

	getBlockchainManager()->unlockChainGuardian();

	// Udpate Reported Effective Rewards - BEGIN
	const_cast<BigInt&>(finalTotalBlockRewardEffective) = rewardUptilNow;
	const_cast<BigInt&>(finalPaidBlockRewardEffective) = rewardIssuedToMinerAfterTAX;
	// Udpate Reported Effective Rewards - END


setIsInFlow(false);
//mFlowGuardian.unlock();
mFlowStateDB->unlock();
		return result;
	
}

void CTransactionManager::cleanupFlow(bool cleanInFlowTransactions, bool revert, bool cleanBackupDB)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(mFlowStateDBGuardian);
	clearInFlowLinks();
 assertGN(getIsInFlow());
	getTools()->writeLine("Cleaning up the Flow");
	if (!getIsInFlow())
		return;

	if (mFlowStateDB)
	{
		mFlowStateDB->pruneTrie(true, false, false, false); // do this every time ?
	}

	//clear any changes done to data structured in RAM, deallocate memory
	if(revert)
	{
		mStateDomainManager->cleanUpFlowData();
	 assertGN(mBackupStateDB != NULL);
		getTools()->writeLine("Reverting perspective back to " + getTools()->base58CheckEncode(mBackupStateDB->getPerspective()));
	*mFlowStateDB = *mBackupStateDB;
	//if (!mBackupStateDB->isEmpty(false))
	//mBackupStateDB->copyTo(mFlowStateDB,false);
 assertGN(std::memcmp(mBackupStateRootID.data(), mFlowStateDB->getPerspective().data(), 32) == 0);
	//assert(mFlowStateDB->setPerspective(mBackupStateRootID));//TODO: revert to the first DB in mInFlowInterDBs?
	}
	mUtilizedBlockSize = 0;
	mUtilizedERG = 0;
	mTotalGNCFees = 0;
	mTreasuryTax = 0;

	mScriptEngine->enterSandbox();
	mScriptEngine->reset();
	//mScriptEngine->clearRegN(REG_KERNEL_THREAD);
	mInSandbox = true;

	if (cleanInFlowTransactions)
	{

		mInFlowTransactions.clear();
		mInFlowVerifiables.clear();
	}
	mInFlowInterPerspectives.clear();
	mInFlowTotalTXDBStackDepth = 0;
	mTXsCleanedCount = 0;
	mTXsRevertedCount = 0;

	CAsyncMemCleaner::cleanItVec(mInFlowInterDBs);

	if (mInFlowInterDBs.size() != 0)
	 assertGN(false);
	if(cleanBackupDB &&mBackupStateDB!=NULL)
	{
	CAsyncMemCleaner::cleanIt(reinterpret_cast<void**>(&mBackupStateDB));
	
 assertGN(mBackupStateDB ==nullptr);
	}
}

bool CTransactionManager::stepBack(size_t nrOfSteps)
{
	// Early Return - BEGIN
	if (!nrOfSteps)
	{
		return true;
	}
	// Early Return - END

	// Lock Acquisition - BEGIN
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(mFlowStateDBGuardian);
	// Lock Acquisition - END

	// Flow Validation - BEGIN
	assertGN(getIsInFlow());
	getTools()->writeLine("Performing " + std::to_string(nrOfSteps) + " step back" + (nrOfSteps == 1 ? "" : "s"));
	if (nrOfSteps > mInFlowInterPerspectives.size() || nrOfSteps > mInFlowInterDBs.size())
		return false;
	// Flow Validation - END

	// Calculate target state index
	size_t targetIndex = mInFlowInterPerspectives.size() - nrOfSteps;
	CTrieDB* targetDB = mInFlowInterDBs[targetIndex];

	// Apply state from target DB
	if (!mFlowStateDB->setPerspective(mInFlowInterPerspectives[targetIndex],
		targetDB->getModifiedDomainsKer()))
	{
		// Cold storage retrieval failed, perform direct state copy
		*mFlowStateDB = *targetDB;

		// Verify perspective matches after copy
		if (!getTools()->compareByteVectors(mFlowStateDB->getPerspective(),
			targetDB->getPerspective()))
			return false;
	}

	// Cleanup intermediate states
	for (size_t i = 0; i < nrOfSteps; i++)
	{
		size_t lastIndex = mInFlowInterPerspectives.size() - 1;

		// State Cleanup - BEGIN
		CAsyncMemCleaner::cleanIt(reinterpret_cast<void**>(&mInFlowInterDBs[lastIndex]));
		mInFlowInterDBs.pop_back();

		if (getInFlowTXDBStackDepth())
		{
			decInFlowTXDBStackDepth();
		}
		mInFlowInterPerspectives.pop_back();
		// State Cleanup - END
	}

	return true;
}

bool CTransactionManager::abortFlow()
{
	std::lock_guard<std::recursive_mutex> lockA(mFlowGuardian);	// was there an attempt to abort an unowned Flow? in such a case this would block

	std::lock_guard<std::recursive_mutex>	lock(mCurrentDBGuardian);
	std::lock_guard<ExclusiveWorkerMutex>	lock2(mFlowStateDB->mGuardian);
	std::lock_guard<std::recursive_mutex>	lock3(mFlowProcessingGuardian);
	getTools()->writeLine("aborting the Flow; Perspective: "+ getTools()->base58CheckEncode(mFlowStateDB->getPerspective()));
	if (!getIsInFlow())
	{
		return false;
	}

	cleanupFlow(true);
	
	
	mStateDomainManager->exitFlow(false);
	mVerifier->exitFlow();
	mCurrentStateDB = mLiveStateDB;
	
	mFlowStateDB->pruneTrie(true, false, true, false);

	getBlockchainManager()->unlockChainGuardian();


	setIsInFlow(false);

	mFlowStateDB->unlock();
	return true;
}

bool CTransactionManager::setOneTimeDiffMultiplier(uint64_t multiplier)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (multiplier == 0 || multiplier > 10)
		multiplier = 1;
	

	mWasForcedDifficultyMultiplierProcessed = false;
	mDifficultyMultiplier = multiplier;
	return true;
}

bool CTransactionManager::setDiffMultiplier(uint64_t multiplier)
{
	mDifficultyMultiplier = multiplier;
	return true;
}

bool CTransactionManager::getIsRemoteTerminal()
{
	std::shared_ptr<CDTI> dti = getDTI();
	if (dti == nullptr)
		return false;
	if (dti != nullptr && dti->getIsRemoteTerminal())
		return true;

	return false;
}




std::vector<uint8_t> CTransactionManager::getCurrentMinigTaskID()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mCurrentMiningTaskID;
}

void CTransactionManager::setCurrentMiningTaskID( std::vector<uint8_t>& id)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    mCurrentMiningTaskID = id;
}

void CTransactionManager::setIsPastGracePeriod(bool isIt)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mPastGracePeriod = isIt;
}

bool CTransactionManager::getIsPastGracePeriod()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mPastGracePeriod;
}


void  CTransactionManager::setTXInMemPoolCached(uint64_t value)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mTXsPendingInMemPoolCached = value;
}
uint64_t  CTransactionManager::getTXInMemPoolCached()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mTXsPendingInMemPoolCached;
}


void  CTransactionManager::setReqMemPoolObjectsCached(uint64_t value)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mRequiredMemPoolObjectsCached = value;
}

uint64_t CTransactionManager::getLastKeyBlockFormationAttempt()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mLastKeyBlockFormationAttempt;
}

void CTransactionManager::pingLastKeyBlockFormationAttempt()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mLastKeyBlockFormationAttempt = std::time(0);

}

uint64_t  CTransactionManager::getReqMemPoolObjectsCached()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mRequiredMemPoolObjectsCached;
}


bool CTransactionManager::getHardForkBlockAlreadyDispatched()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mHardForkBlockAlreadyDispatched;
}

void CTransactionManager::setHardForkBlockAlreadyDispatched(bool wasIt)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	 mHardForkBlockAlreadyDispatched = wasIt;
}

void CTransactionManager::setKeepPersistentDB(bool keep) {
std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
mKeepPersistentDB = keep;
}

bool CTransactionManager::getKeepPersistentDB() {
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mKeepPersistentDB;
}

void CTransactionManager::setPersistentDBUpdateInterval(uint64_t seconds) {
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mPersistentDBUpdateInterval = seconds;
}

uint64_t CTransactionManager::getPersistentDBUpdateInterval() {
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mPersistentDBUpdateInterval;
}

std::shared_ptr<CStateDomainManager> CTransactionManager::getPersistentStateDomainManager() {
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mPersistentStateDomainManager;
}

void CTransactionManager::setWasPersistentDBPopulated()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mPersistenStateDBPopulated = true;
}
bool CTransactionManager::getWasPersistentDBPopulated()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mPersistenStateDBPopulated;
}


CTransactionManager::CTransactionManager(eTransactionsManagerMode::eTransactionsManagerMode mode, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CWorkManager> cwm, std::string minersKeyChainID,
	bool anonimityModeON, eBlockchainMode::eBlockchainMode blockchainMode,
	bool doBlockFormation, bool createDetachedDB,bool doNOTlockChainGuardian, std::shared_ptr<CDTI> dti, bool muteConsole, bool keepPersistentDB)
{
	mInFlowTXDBStackDepth = 0;
	mInFlowTotalTXDBStackDepth = 0;
	mTXsCleanedCount = 0;
	mTXsRevertedCount = 0;
	
	mPersistenStateDBPopulated = false;
	mTXsPendingInMemPoolCached = 0;
	mLastKeyBlockFormationAttempt = 0;
	mRequiredMemPoolObjectsCached = 0;
	mPersistenStateDBPopulated = false;
	mHardForkBlockAlreadyDispatched = false;
	//Mem-Pool Throttler 
	const std::size_t WINDOW_SIZE = 50;
	const double ALPHA = 0.2;
	const uint64_t SENSITIVITY = 2;
	const double VOLATILITY_THRESHOLD = 0.1;
	const int64_t MAX_MIN_OBJECTS = 30;
	const int64_t MIN_MIN_OBJECTS = 1;
	const uint64_t TOTAL_SIMULATION_STEPS = 200;

	mThrottler = std::make_shared<CObjectThrottler>(ALPHA, WINDOW_SIZE, SENSITIVITY, VOLATILITY_THRESHOLD, MIN_MIN_OBJECTS, MAX_MIN_OBJECTS);
	mLastControllerLoopRun = 0;
	mPastGracePeriod = false;
	mIsLIVEDBDetached = false;
	mLastDevReportMode = 0;
	mLastDevReportModeSwitch= 0;
	mDTI = dti;
	mLastTimeMemPoolAdvertised = 0;
	 mNrOfTTransactionsFailedDueToInvalidEnvelopeSig =0;
	 mNrOfTTransactionsFailedDueToInvalidNonce= 0;
	 mNrOfTransactionsFailedDueToUnknownIssuer=0;
	 mPoWCustomBarID = CStatusBarHub::getInstance()->getNextCustomStatusBarID(blockchainMode);
	 mNrOfInvalidTransactionNotIncludedInBlock=0;
	mNrOfTransactionsFailesDueToOutofERG = 0;
	//mDoNotLockChainGuardian = doNOTlockChainGuardian;
	mNrOfTimesDeferredAssetsReleased = 0;
	mFraudulantDataBlocksOrdered = 0;
	 mProofsOfFraudProcessed = 0;
	 mTotalValueOfPenalties = 0;
	 mTotalValueOfPoFRewards = 0;
	 mNewFraudsDetected = 0;
	 mFraudsDetectedButAlreadyRewarded = 0;
	 mNrOfAttemptsToSpendLockedAssets = 0;
	 mNrOfTimesTransactionsFailedDueToInsufficientFunds = 0;
	 mNrOfTransactionsWithInvalidNonceValues = 0;

	mInKeyBlockFlow = false;
	mCycleBackIdentiesAfterNKeysUsed = 0;
	mDataBlocksGeneratesUsingSamePubKey = 0;
	mFraudulantDataBlocksToGeneratePerKeyBlock = 0;
	mKeyBlocksGeneratesUsingSamePubKey = 0;
	mNewPubKeyEveryNKeyBlocks = 0;
	mSynchronizeBlockProduction = true;
	 
	mBlockProductionHaltedTillTimeStamp = 0;
	mNotifiedWaitingForTransactions = false;
	mMinTransactionPerDataBlock = 0;
	mKeyBlocksFormedCounter = 0;
	mDataBlocksFormedCounter = 0;
	mProcessedVerifiables = 0;
	mIsSettingUpFlow = false;
	mInFlow = false;
	mKeyBlockFormationStatus = eBlockFormationStatus::idle;
	mRegBlockFormationStatus = eBlockFormationStatus::idle;
	mInformedNotLeader = false;
 assertGN(bm != nullptr);
	mMode = mode;
	mTools = std::make_shared<CTools>("Transaction Manager", blockchainMode, eViewState::eViewState::eventView, dti, muteConsole);

 assertGN(mTools != nullptr);
	mLastTimeMemPoolCleared = getTools()->getTime();
	if (doBlockFormation && minersKeyChainID.size() == 0)
	{
		getTools()->writeLine("No key-chain available; I'll now quit.");
		getBlockchainManager()->exit(true);
	}
	
	mMaxReceiptsIndexCacheSize = 10000;
	mDifficultyMultiplier = 1;
	mWasForcedPerspectiveProcessed = true;
	mWasForcedDifficultyMultiplierProcessed = true;
	mUpdatePerspectiveAfterBlockProcessed = true;
	mDoBlockFormation = doBlockFormation;
mBackupStateDB = NULL;
mStatusChange = eManagerStatus::eManagerStatus::initial;
mStatus = eManagerStatus::eManagerStatus::initial;
mUtilizedBlockSize = 0;
mUtilizedERG = 0;
mTotalGNCFees = 0;
mTreasuryTax = 0;
mMaxERGUsage = CGlobalSecSettings::getBlockERGLimit();
mBlockchainManager = bm;
mAverageERGBid = 0;
mAnonimityModeOn = anonimityModeON;
mMinersKeyChainID = minersKeyChainID;
mSettings = new CSettings(blockchainMode);
mCryptoFactory = CCryptoFactory::getInstance();
mBlockchainMode = blockchainMode;

if (!createDetachedDB)
{
	mLiveStateDB = getBlockchainManager()->getStateTrieP();
}
else//used by the test-unit ALSO by terminal-transaction-manager to entirely decouple from Live-Data or Live-flow processing
{
	mLiveStateDB = new CTrieDB(getBlockchainManager()->getStateTrie());
	mIsLIVEDBDetached = true;
}

//Flow StateDB is always new
mFlowStateDB = new CTrieDB(getBlockchainManager()->getStateTrie());
mFlowStateDB->setFlowStateDB();
if (mode != eTransactionsManagerMode::Terminal)
mCurrentStateDB = mLiveStateDB;
else
mCurrentStateDB = mFlowStateDB;

mStateDomainManager = std::make_shared<CStateDomainManager>(mCurrentStateDB, getBlockchainMode(), this);

// Persistent DB - BEGIN
mKeepPersistentDB = keepPersistentDB;
mPersistentDBUpdateInterval = 15 * 60; // Default 15 minutes
mLastPersistentDBUpdate = 0;
mPersistentStateDB = nullptr;
mPersistentStateDBDouble = nullptr;
mPersistentStateDomainManager = nullptr;

if (mKeepPersistentDB) {
	// Initialize persistent state DB
	mPersistentStateDB = new CTrieDB(getBlockchainManager()->getStateTrie());
	mPersistentStateDBDouble = new CTrieDB(*mPersistentStateDB);
	mPersistentStateDomainManager = std::make_shared<CStateDomainManager>(
		mPersistentStateDB,
		getBlockchainMode(),
		this
	);
}
// Persistent DB - END


mWorkManager = getBlockchainManager()->getWorkManager();



mGlobalSecSettings = new CGlobalSecSettings();
mInSandbox = true;
mVerifier = new CVerifier(mBlockchainManager);
mWorkManager = cwm;

mStateDomainManager->exitFlow(false);
mVerifier->exitFlow();


}

/// <summary>
/// Initializes the Transaction Manager.
/// An external VM is to be provided when an external thread is spawned as an offspring from the System-thread.
/// In such a case, the thread is set to own the Transaction Manager (through storage within a shared_pointer instead of weak_pointer).
/// This is carried out by providing a specific flag set to setTransactionsManager().
/// This effectively inverts the hierarchical relationship between CScriptEngine and TransactionsManager.
/// </summary>
/// <param name="se"></param>
/// <returns></returns>
bool CTransactionManager::initialize(std::shared_ptr<SE::CScriptEngine> se)
{
	if (CGRIDNET::getInstance()->getIsShuttingDown())
		return false;
	if (!se)
	{//if a separate detached sub-thread was not provided
		
		mScriptEngine = std::make_shared<SE::CScriptEngine>(shared_from_this(), mBlockchainManager, false, eViewState::eventView, mDTI);
		mScriptEngine->setStateDomainManager(mStateDomainManager);//replace official live sdm
	}
	else
	{//a specific sub-thread is to be available within this instance of TM
		mScriptEngine = se;//TM owns it. (shared pointer)

		//the Thread also wants to know about it
		se->setTransactionsManager(shared_from_this(), true);//CScriptEngine owns transaction manager only when a sub-thread is spawned.
	}

	{
		std::lock_guard<std::mutex> lockThreads(mThreadManagementMutex);
		// Ensure no existing thread before creating a new one
		if (mController.joinable())
		{
			requestStatusChange(eManagerStatus::stopped);
			if (mController.joinable())
				mController.join();
		}
		mController = std::thread(&CTransactionManager::mControllerThreadF, this);
	}

	return true;
}
bool CTransactionManager::updateBlocksGeneratedWithSamePubKey()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	std::vector<uint8_t> currentPubKey;
	CKeyChain chain(false);
	std::shared_ptr<CBlock> current = getBlockchainManager()->getLeader();
	std::shared_ptr<CBlock> previousKeyBlock = getBlockchainManager()->getLeader(true);

	if (current == nullptr || current->getHeader()->isKeyBlock() || previousKeyBlock==nullptr)
	{
		mDataBlocksGeneratesUsingSamePubKey = 0;
		return true;
	}

	if (!mSettings->getCurrentKeyChain(chain, false, false, mSettings->getMinersKeyChainID()))
	{
		mDataBlocksGeneratesUsingSamePubKey = 0;
		return false;
	}

	if (!getBlockchainManager()->amITheLeader(chain, chain))
	{
		mDataBlocksGeneratesUsingSamePubKey = 0;
		return false;
	}

	currentPubKey = chain.getPubKey();

	if (!getTools()->compareByteVectors(previousKeyBlock->getHeader()->getPubKey(), currentPubKey))
	{
		mDataBlocksGeneratesUsingSamePubKey = 0;
		return true;
	}

	mDataBlocksGeneratesUsingSamePubKey = current->getHeader()->getHeight() - previousKeyBlock->getHeader()->getHeight();

	return true;

}


CTransactionManager::~CTransactionManager()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	if(getIsInFlow())
	cleanupFlow(true);

	if (mController.joinable())
	{
		requestStatusChange(eManagerStatus::stopped);
		mController.join();
	}
	if (mSettings != nullptr)
		delete mSettings;
	mCryptoFactory = nullptr;

	
	if (mStateDomainManager != nullptr)
	{
		mStateDomainManager->exitFlow();
		mStateDomainManager = nullptr;
	}
	if (mGlobalSecSettings != nullptr)
		delete mGlobalSecSettings;
	if (mVerifier != nullptr)
		delete mVerifier;
	if (mScriptEngine != nullptr)
		mScriptEngine =nullptr;
	if (mFlowStateDB != nullptr)
		delete mFlowStateDB;

	if (mIsLIVEDBDetached)
		delete mLiveStateDB;

	if(mKeepPersistentDB) {
		if (mPersistentStateDB != nullptr) {
			delete mPersistentStateDB;
			mPersistentStateDB = nullptr;
		}
		mPersistentStateDomainManager = nullptr;
	}

	getTools()->writeLine("Transaction Manager killed;");

}
uint64_t CTransactionManager::getUnprocessedTransactionsCount() {
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	uint64_t count = 0;

	for (const auto& node : mMemPool) {
		// Check if node is a transaction
		if (node->getType() != eNodeType::leafNode ||
			node->getSubType() != eNodeSubType::transaction) {
			continue;
		}

		// Check if transaction is not marked as processed
		if (!node->HotProperties.isMarkedAsProcessed()) {
			count++;
		}
	}

	return count;
}

// ============================================================================
// Transaction Withholding Detection Support Implementation - BEGIN
// ============================================================================
//
// [ Purpose ]:
//   These methods support the Vitals Monitoring system in detecting potential
//   transaction withholding attacks. They provide information about pending
//   transactions in the mem-pool - specifically how long they have been waiting.
//
// [ Attack Description ]:
//   A transaction withholding attack occurs when malicious leaders produce
//   key blocks (maintaining their leadership) but intentionally omit pending
//   transactions from data blocks. This causes:
//     - Transactions to remain stuck in mem-pools network-wide
//     - The blockchain to appear functional (key blocks produced)
//     - Users unable to get their transactions confirmed
//
// [ Detection Strategy ]:
//   The Vitals Monitoring thread (mVitalsMonitoringThreadF) periodically:
//     1. Calls getMemPoolWithholdingStats() to get oldest TX timestamp
//     2. Calculates elapsed time in terms of key block intervals
//     3. If >= 3 key blocks pass with TX still waiting -> warning issued
//
// [ Integration with Phantom Leader Mode ]:
//   When withholding is detected, users are advised to enable Phantom Mode
//   ('chain -phantom on') to locally simulate transaction processing.
//   This helps distinguish between:
//     - Withholding attack (TXs process fine in phantom mode)
//     - Invalid transactions (TXs fail even in phantom mode)
//     - Software bugs (phantom mode reveals processing errors)
//
// [ Methods ]:
//   - getOldestUnprocessedTransactionTime() : Timestamp of oldest pending TX
//   - getMemPoolWithholdingStats()          : Combined stats (time + count)
//
// ============================================================================

/// @brief Gets the timestamp of when the oldest unprocessed transaction was received.
/// @details Iterates through the mem-pool to find the transaction with the earliest
///          receivedAt timestamp that has not yet been marked as processed.
///          Used by Vitals Monitoring to detect stale transactions.
/// @return Unix timestamp of the oldest unprocessed TX, or 0 if mem-pool is empty.
uint64_t CTransactionManager::getOldestUnprocessedTransactionTime()
{
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	uint64_t oldestTime = 0;
	bool firstFound = false;

	for (const auto& node : mMemPool)
	{
		// Check if node is a transaction
		if (node->getType() != eNodeType::leafNode ||
			node->getSubType() != eNodeSubType::transaction)
		{
			continue;
		}

		// Check if transaction is not marked as processed
		if (!node->HotProperties.isMarkedAsProcessed())
		{
			std::shared_ptr<CTransaction> tx = std::static_pointer_cast<CTransaction>(node);
			uint64_t receivedAt = tx->getReceivedAt();

			if (!firstFound || (receivedAt > 0 && receivedAt < oldestTime))
			{
				oldestTime = receivedAt;
				firstFound = true;
			}
		}
	}

	return oldestTime;
}

/// @brief Gets comprehensive mem-pool statistics for transaction withholding detection.
/// @details This method is called by the Vitals Monitoring thread (mVitalsMonitoringThreadF)
///          to gather data needed for withholding attack detection. It performs a single
///          pass through the mem-pool to efficiently collect both the oldest TX timestamp
///          and the total count of pending transactions.
///
///          Transactions are considered "pending" if they are:
///            - Of type leafNode/transaction (not verifiables or other objects)
///            - NOT marked as processed (HotProperties.isMarkedAsProcessed() == false)
///            - NOT marked as invalid (HotProperties.isMarkedAsInvalid() == false)
///
/// @param oldestTxTime [out] Unix timestamp when the oldest unprocessed TX was received.
///                           Set to 0 if no pending transactions exist.
/// @param txCount [out] Total number of unprocessed, valid transactions in mem-pool.
/// @return true if stats were retrieved successfully (always returns true).
///
/// @see mVitalsMonitoringThreadF() - Consumer of this data for withholding detection
/// @see checkForTransactionWithholding() - Threshold check using this data
bool CTransactionManager::getMemPoolWithholdingStats(uint64_t& oldestTxTime, uint64_t& txCount)
{
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);

	oldestTxTime = 0;
	txCount = 0;
	bool firstFound = false;

	for (const auto& node : mMemPool)
	{
		// Check if node is a transaction
		if (node->getType() != eNodeType::leafNode ||
			node->getSubType() != eNodeSubType::transaction)
		{
			continue;
		}

		// Check if transaction is not marked as processed or invalid
		if (!node->HotProperties.isMarkedAsProcessed() &&
			!node->HotProperties.isMarkedAsInvalid())
		{
			txCount++;

			std::shared_ptr<CTransaction> tx = std::static_pointer_cast<CTransaction>(node);
			uint64_t receivedAt = tx->getReceivedAt();

			// Track the oldest transaction
			if (!firstFound || (receivedAt > 0 && receivedAt < oldestTxTime))
			{
				oldestTxTime = receivedAt;
				firstFound = true;
			}
		}
	}

	return true;
}

// Transaction Withholding Detection Support Implementation - END
// ============================================================================

// ============================================================================
// Phantom Mode Transaction Tracking Implementation - BEGIN
// ============================================================================
//
// [ Purpose ]:
//   These methods track transactions that have been processed in Phantom Leader
//   Mode. Since phantom mode keeps transactions in the mem-pool (doesn't mark
//   them as processed), we need separate tracking to prevent infinite reprocessing.
//
// [ Implementation ]:
//   Uses an unordered_set with string keys (hex-encoded TX IDs) for O(1) lookup.
//   The set is protected by a mutex for thread safety.
//
// [ Lifecycle ]:
//   - markAsPhantomProcessed() : Called after phantom block formation
//   - isPhantomProcessed()     : Called when selecting TXs for phantom block
//   - clearPhantomProcessedTransactions() : Called on phantom mode reset/disable
//
// ============================================================================

/// @brief Marks a transaction as having been processed in phantom mode.
void CTransactionManager::markAsPhantomProcessed(const std::vector<uint8_t>& txID)
{
	if (txID.empty()) return;

	std::lock_guard<std::mutex> lock(mPhantomProcessedTxGuardian);

	// Convert to hex string for efficient storage and lookup
	std::string txIDStr;
	txIDStr.reserve(txID.size() * 2);
	static const char hexChars[] = "0123456789abcdef";
	for (uint8_t byte : txID)
	{
		txIDStr.push_back(hexChars[byte >> 4]);
		txIDStr.push_back(hexChars[byte & 0x0F]);
	}

	mPhantomProcessedTxIDs.insert(std::move(txIDStr));
}

/// @brief Checks if a transaction has already been processed in phantom mode.
bool CTransactionManager::isPhantomProcessed(const std::vector<uint8_t>& txID) const
{
	if (txID.empty()) return false;

	std::lock_guard<std::mutex> lock(mPhantomProcessedTxGuardian);

	// Convert to hex string for lookup
	std::string txIDStr;
	txIDStr.reserve(txID.size() * 2);
	static const char hexChars[] = "0123456789abcdef";
	for (uint8_t byte : txID)
	{
		txIDStr.push_back(hexChars[byte >> 4]);
		txIDStr.push_back(hexChars[byte & 0x0F]);
	}

	return mPhantomProcessedTxIDs.find(txIDStr) != mPhantomProcessedTxIDs.end();
}

/// @brief Clears all phantom-processed transaction tracking.
void CTransactionManager::clearPhantomProcessedTransactions()
{
	std::lock_guard<std::mutex> lock(mPhantomProcessedTxGuardian);

	size_t clearedCount = mPhantomProcessedTxIDs.size();
	mPhantomProcessedTxIDs.clear();

	if (clearedCount > 0)
	{
		getTools()->logEvent(
			"Cleared " + std::to_string(clearedCount) + " phantom-processed transaction(s) from tracking.",
			"Phantom Mode",
			eLogEntryCategory::localSystem,
			5,
			eLogEntryType::notification,
			eColor::lightCyan
		);
	}
}

/// @brief Gets the count of transactions tracked as phantom-processed.
size_t CTransactionManager::getPhantomProcessedTransactionCount() const
{
	std::lock_guard<std::mutex> lock(mPhantomProcessedTxGuardian);
	return mPhantomProcessedTxIDs.size();
}

// Phantom Mode Transaction Tracking Implementation - END
// ============================================================================

// ============================================================================
// Phantom Mode Processing Report Implementation - BEGIN
// ============================================================================
//
// [ Purpose ]:
//   These methods manage a detailed report of all transactions processed in
//   phantom mode. The report can be viewed via 'chain -phantom report' and
//   helps diagnose transaction processing issues and potential withholding.
//
// ============================================================================

/// @brief Adds an entry to the phantom processing report.
void CTransactionManager::addPhantomReportEntry(
	const std::vector<uint8_t>& receiptID,
	eTransactionValidationResult result,
	BigInt ergUsed,
	BigInt ergPrice,
	const std::string& resultText,
	const std::vector<std::string>& logEntries)
{
	std::lock_guard<std::mutex> lock(mPhantomReportGuardian);

	PhantomTxReportEntry entry;
	entry.receiptID = receiptID;
	entry.processedAt = std::time(0);
	entry.result = result;
	entry.ergUsed = ergUsed;
	entry.ergPrice = ergPrice;
	entry.resultText = resultText;
	entry.logEntries = logEntries;

	mPhantomReportEntries.push_back(std::move(entry));

	// Limit report size to prevent unbounded growth (keep last 1000 entries)
	if (mPhantomReportEntries.size() > 1000)
	{
		mPhantomReportEntries.erase(mPhantomReportEntries.begin());
	}
}

/// @brief Gets the current phantom processing report entries.
std::vector<CTransactionManager::PhantomTxReportEntry> CTransactionManager::getPhantomReport() const
{
	std::lock_guard<std::mutex> lock(mPhantomReportGuardian);
	return mPhantomReportEntries; // Return copy for thread safety
}

/// @brief Clears the phantom processing report.
void CTransactionManager::clearPhantomReport()
{
	std::lock_guard<std::mutex> lock(mPhantomReportGuardian);

	size_t clearedCount = mPhantomReportEntries.size();
	mPhantomReportEntries.clear();

	if (clearedCount > 0)
	{
		getTools()->logEvent(
			"Cleared " + std::to_string(clearedCount) + " phantom report entry(ies).",
			"Phantom Mode",
			eLogEntryCategory::localSystem,
			5,
			eLogEntryType::notification,
			eColor::lightCyan
		);
	}
}

/// @brief Gets summary statistics from the phantom report.
void CTransactionManager::getPhantomReportSummary(
	size_t& totalTxs,
	size_t& successCount,
	size_t& failCount,
	BigInt& totalErgUsed) const
{
	std::lock_guard<std::mutex> lock(mPhantomReportGuardian);

	totalTxs = mPhantomReportEntries.size();
	successCount = 0;
	failCount = 0;
	totalErgUsed = 0;

	for (const auto& entry : mPhantomReportEntries)
	{
		if (entry.result == eTransactionValidationResult::valid)
		{
			successCount++;
		}
		else
		{
			failCount++;
		}
		totalErgUsed += entry.ergUsed;
	}
}

// Phantom Mode Processing Report Implementation - END
// ============================================================================

/**
 * @brief Processes transaction breakpoints
 *
 * Handles both pre-execution and post-execution breakpoint states,
 * providing comprehensive transaction state information at each stage.
 *
 * @param trans Transaction being processed
 * @param receiptID Transaction receipt ID
 * @param keyHeight Current key block height
 * @param state Execution state (pre/post)
 * @param existingReceipt Optional existing receipt
 * @param existingBlock Optional containing block
 * @return bool True if processing should continue
 */
 /**
  * @brief Processes transaction breakpoints
  *
  * Handles both pre-execution and post-execution breakpoint states,
  * providing comprehensive transaction state information at each stage.
  *
  * @param trans Transaction being processed
  * @param receiptID Transaction receipt ID
  * @param keyHeight Current key block height
  * @param state Execution stage (pre/post)
  * @param existingReceipt Optional existing receipt (only if transaction is part of existing block)
  * @param existingBlock Optional containing block (only if transaction is part of existing block)
  * @return bool True if processing should continue
  */
bool CTransactionManager::processBreakpoints(
	const CTransaction& trans,
	const std::vector<uint8_t>& receiptID,
	uint64_t keyHeight,
	eBreakpointState::eBreakpointState stage,
	std::shared_ptr<CReceipt> existingReceipt,
	std::shared_ptr<CBlock> existingBlock)
{
	// Preliminaries - BEGIN
	std::shared_ptr<CBlockchainManager> bm = CBlockchainManager::getInstance(mBlockchainMode, false, false);
	if (!bm) return false;

	std::shared_ptr<CBreakpointFactory> bf = bm->getBreakpointFactory();
	if (!bf) return false;

	std::shared_ptr<CStateDomainManager> sdm = mStateDomainManager;
	if (!sdm) return false;

	std::shared_ptr<CTools> tools =bm->getTools();
	if (!tools) return false;

	// Early return if no active breakpoints for this transaction
	if (!bf->getTransactionBreakpointCount()==0) {
		return true;
	}

	// Preliminaries - END

	// Local Variables - BEGIN
	std::stringstream ss;
	std::string newLine = "\n";
	std::vector<std::shared_ptr<CBreakpoint>> hitBreakpoints;
	// Local Variables - END

	// Operational Logic - BEGIN

	// Determine if the transaction is part of an existing block
	bool isExistingBlock = existingReceipt && existingBlock;

	// Create transaction metadata for analysis
	CSearchResults::ResultData resultData = bm->createTransactionDescription(
		std::make_shared<CTransaction>(trans),
		existingReceipt,
		isExistingBlock ? existingBlock->getHeader()->getSolvedAtTime() : std::time(nullptr),
		true,
		isExistingBlock ? existingBlock->getHeader() : nullptr
	);

	auto txDescPtr = std::get_if<std::shared_ptr<CTransactionDesc>>(&resultData);
	if (!txDescPtr || !*txDescPtr) {
		tools->logEvent("Failed to create transaction metadata for breakpoint analysis",
			"Breakpoint", eLogEntryCategory::localSystem, 5, eLogEntryType::failure);
		return true;
	}
	std::shared_ptr<CTransactionDesc> txDesc = *txDescPtr;

	// Check active breakpoints and gather hit breakpoints
	auto checkBreakpoints = [&](const std::vector<std::shared_ptr<CBreakpoint>>& bps) {
		for (const auto& bp : bps) {
			if (bp->isActive()) {
					hitBreakpoints.push_back(bp);
					bp->setState(stage, receiptID); // transition breakpoint state to current stage
					// Set state if not already set
					bp->hit();
						
					
			}
		}
		};

	// Check all types of transaction breakpoints
	checkBreakpoints(bf->getBreakpointsForReceipt(receiptID));
	checkBreakpoints(bf->getBreakpointsForSource(txDesc->getSender()));
	checkBreakpoints(bf->getBreakpointsForDestination(txDesc->getReceiver()));

	std::string receiver = txDesc->getReceiver();
	if (!receiver.empty()) {
		checkBreakpoints(bf->getBreakpointsForDestination(receiver));
	}

	if (!hitBreakpoints.empty()) {
		tools->activateView(eViewState::eventView);
		Sleep(1000);
		// Build header based on state
		ss << newLine << tools->getColoredString(
			stage == eBreakpointState::preExecution ?
			"[ Transaction Pre-Execution Breakpoint Hit ]" :
			"[ Transaction Post-Execution Breakpoint Hit ]",
			eColor::orange) << newLine;

		// Get current state trie perspective
		std::vector<uint8_t> currentPerspective = mFlowStateDB->getPerspective();
		std::string perspectiveStr = tools->base58CheckEncode(currentPerspective);

		ss << tools->getColoredString("System State: ", eColor::lightCyan)
			<< perspectiveStr << newLine;

		// Transaction Identification
		ss << newLine << tools->getColoredString("[ Transaction Identification ]", eColor::blue) << newLine;
		ss << tools->getColoredString("Receipt ID: ", eColor::lightCyan)
			<< tools->base58CheckEncode(receiptID) << newLine;
		ss << tools->getColoredString("Source: ", eColor::lightCyan)
			<< txDesc->getSender() << newLine;
		if (!receiver.empty()) {
			ss << tools->getColoredString("Destination: ", eColor::lightCyan)
				<< receiver << newLine;
		}

		// Get and display balances
		if (sdm) {
			// Source balance
			CStateDomain* sourceDomain = sdm->findByID(tools->stringToBytes(txDesc->getSender()));
			if (sourceDomain) {
				ss << tools->getColoredString("Source Balance: ", eColor::lightCyan)
					<< tools->attoToGNCStr(sourceDomain->getBalance()) << " GNC" << newLine;
			}

			// Destination balance if available
			if (!receiver.empty()) {
				CStateDomain* destDomain = sdm->findByID(tools->stringToBytes(receiver));
				if (destDomain) {
					ss << tools->getColoredString("Destination Balance: ", eColor::lightCyan)
						<< tools->attoToGNCStr(destDomain->getBalance()) << " GNC" << newLine;
				}
			}
		}

		// Transaction Details - BEGIN
		ss << newLine << tools->getColoredString("[ Transaction Details ]", eColor::blue) << newLine;
		ss << tools->getColoredString("Value: ", eColor::lightCyan)
			<< txDesc->getValueTxt() << newLine;
		ss << tools->getColoredString("ERG Limit: ", eColor::lightCyan)
			<< txDesc->getERGLimit().str() << newLine;
		ss << tools->getColoredString("Nonce: ", eColor::lightCyan)
			<< txDesc->getNonce() << newLine;
		ss << tools->getColoredString("Key Height: ", eColor::lightCyan)
			<< keyHeight << newLine;

		// Execution State Information - BEGIN
		if (stage == eBreakpointState::postExecution || existingReceipt) {
			ss << newLine << tools->getColoredString("[ Execution Information ]", eColor::blue) << newLine;
			ss << tools->getColoredString("Status: ", eColor::lightCyan)
				<< txDesc->getResultTxt(true) << newLine;
			ss << tools->getColoredString("ERG Used: ", eColor::lightCyan)
				<< txDesc->getERGUsed().str() << newLine;

			// Tax information if present
			if (txDesc->getTax()) {
				ss << tools->getColoredString("Tax: ", eColor::lightCyan)
					<< txDesc->getTaxTxt() << newLine;
			}

			// Execution Log
			const auto& logEntries = txDesc->getLog();
			if (!logEntries.empty()) {
				ss << newLine << tools->getColoredString("[ Transaction Log ]", eColor::blue) << newLine;
				for (const auto& entry : logEntries) {
					ss << tools->getColoredString(" • ", eColor::lightCyan)
						<< entry << newLine;
				}
			}
		}
		else {
			ss << newLine << tools->getColoredString("[ Expected Execution ]", eColor::blue) << newLine;
			ss << tools->getColoredString("Status: ", eColor::lightCyan)
				<< "Pending Execution" << newLine;
		}
		// Execution State Information - END

		// Transaction Code - BEGIN
		std::string code = txDesc->getSourceCode();
		if (!code.empty()) {
			ss << newLine << tools->getColoredString("[ Transaction Code ]", eColor::blue) << newLine;
			ss << code << newLine;
		}
		// Transaction Code - END

		// Transfer Statistics - BEGIN
		if (txDesc->getValue()) {
			ss << newLine << tools->getColoredString("[ Transfer Statistics ]", eColor::blue) << newLine;
			ss << tools->getColoredString("Total Sent: ", eColor::lightCyan)
				<< txDesc->getValueTxt() << newLine;
		}
		// Transfer Statistics - END

		// Timing Information - BEGIN
		ss << newLine << tools->getColoredString("[ Timing Information ]", eColor::blue) << newLine;
		if (txDesc->getConfirmedTimestamp() > 0) {
			ss << tools->getColoredString("Confirmed: ", eColor::lightCyan)
				<< txDesc->getConfirmedTimestampTxt() << newLine;
		}
		if (txDesc->getUnconfirmedTimestamp() > 0) {
			ss << tools->getColoredString("Unconfirmed: ", eColor::lightCyan)
				<< txDesc->getUnconfirmedTimestampTxt() << newLine;
		}
		// Timing Information - END

		// Breakpoint Details - BEGIN
		ss << newLine << tools->getColoredString("[ Active Breakpoints ]", eColor::blue) << newLine;
		for (size_t i = 0; i < hitBreakpoints.size(); ++i) {
			const auto& bp = hitBreakpoints[i];

			if (bp->isActive() == false)
			{
				// notice: user might have disable all breakpoint or unitary ones during breakpoints' traversal
				continue;
			}
			ss << tools->getColoredString("[Breakpoint " + std::to_string(i + 1) + "]", eColor::orange) << newLine;
			ss << tools->getColoredString("Type: ", eColor::lightCyan)
				<< (bp->getType() == eBreakpointType::transaction ? "Transaction" : "Unknown") << newLine;
			ss << tools->getColoredString("Hit Count: ", eColor::lightCyan)
				<< bp->getHitCount() << newLine;
			ss << tools->getColoredString("Condition: ", eColor::lightCyan);

			switch (bp->getCondition()) {
			case eBreakpointCondition::receiptID:
				ss << "Receipt ID (" << tools->base58CheckEncode(bp->getValueBytes()) << ")";
				break;
			case eBreakpointCondition::txSource:
				ss << "Source (" << tools->bytesToString(bp->getValueBytes()) << ")";
				break;
			case eBreakpointCondition::txDestination:
				ss << "Destination (" << tools->bytesToString(bp->getValueBytes()) << ")";
				break;
			default:
				ss << "Unknown";
			}
			ss << newLine << "---" << newLine;
		}
		// Breakpoint Details - END

		// State Change Information (only in post-execution) - BEGIN
		if (stage == eBreakpointState::postExecution) {
			ss << newLine << tools->getColoredString("[ State Changes ]", eColor::blue) << newLine;
			bool stateChanged = false;

			// Compare with pre-execution state if available
			std::vector<uint8_t> preExecPerspective;
			for (const auto& bp : hitBreakpoints) {
				// Retrieve pre-execution perspective stored during pre-execution
				if (!bp->getPreExecutionPerspective().empty()) {
					preExecPerspective = bp->getPreExecutionPerspective();
					break;
				}
			}

			if (!preExecPerspective.empty()) {
				if (preExecPerspective != currentPerspective) {
					stateChanged = true;
				}
			}

			ss << tools->getColoredString("State Change: ", eColor::lightCyan)
				<< (stateChanged ?
					tools->getColoredString("Yes", eColor::lightGreen) :
					tools->getColoredString("No", eColor::orange)) << newLine;
		}
		// State Change Information - END

		// Command Help - BEGIN
		ss << newLine << tools->getColoredString("Commands:", eColor::blue) << newLine;
		ss << tools->getColoredString("continue", eColor::lightCyan) << " - Continue execution" << newLine;
		ss << tools->getColoredString("skip N", eColor::lightCyan) << " - Disable breakpoint N (e.g., 'skip 1')" << newLine;
		ss << tools->getColoredString("(s)kip all", eColor::lightCyan) << " - Disable all hit breakpoints" << newLine;
		ss << "                Hit " << tools->getColoredString("Enter", eColor::lightCyan) << " to Continue" << newLine;
		ss <<	"Choose option ";
		// Command Help - END

		// Log the entire breakpoint report
		//tools->writeLine(ss.str(), true, false, eViewState::eventView, "Debugging", false, false);
		//Sleep(500);
		// For pre-execution, store the current perspective in the breakpoint for later comparison
		if (stage == eBreakpointState::preExecution) {
			for (const auto& bp : hitBreakpoints) {
				bp->setPreExecutionPerspective(currentPerspective);
			}
		}

		// User Interaction Loop - BEGIN
		bool continueExecution = false;
		bool abortProcessing = false;
		while (!continueExecution && !abortProcessing) {
			std::string response = tools->askString(ss.str(), "continue", "Debugger", true, true, 0);
			response = tools->trim(response);

			if (response.empty() || tools->iequals(response, "continue")) {
				continueExecution = true;
			}
			else if (tools->iequals(response, "skip all") || tools->iequals(response, "s")) {
				// Deactivate all hit breakpoints
				for (const auto& bp : hitBreakpoints) {
					bp->deactivate();
					tools->logEvent(
						"Deactivated transaction breakpoint of type " +
						std::to_string(static_cast<int>(bp->getCondition())) +
						" for " + tools->base58CheckEncode(receiptID),
						"Breakpoint",
						eLogEntryCategory::localSystem,
						5,
						eLogEntryType::notification
					);
				}
				continueExecution = true;
			}
			else if (response.substr(0, 5) == "skip ") {
				std::string numStr = response.substr(5);
				uint64_t num;
				if (tools->stringToUint(numStr, num) && num >= 1 && num <= hitBreakpoints.size()) {
					// Deactivate specific breakpoint
					hitBreakpoints[num - 1]->deactivate();
					tools->logEvent(
						"Deactivated transaction breakpoint " + std::to_string(num) +
						" of type " + std::to_string(static_cast<int>(hitBreakpoints[num - 1]->getCondition())) +
						" for " + tools->base58CheckEncode(receiptID),
						"Breakpoint",
						eLogEntryCategory::localSystem,
						5,
						eLogEntryType::notification
					);

					// Ask for confirmation to continue
					std::string confirmMsg = tools->getColoredString(
						"Breakpoint " + std::to_string(num) + " disabled. Continue execution? (yes/no)",
						eColor::lightCyan
					) + newLine;

					if (tools->askYesNo(confirmMsg, true, "Debugger", true, true)) {
						continueExecution = true;
					}
					else {
						// Continue the loop to prompt again
					}
				}
				else {
					ss.str(""); // Clear stringstream
					ss << tools->getColoredString(
						"Invalid breakpoint number. Use 1 to " + std::to_string(hitBreakpoints.size()),
						eColor::lightPink
					) << newLine;
					tools->writeLine(ss.str());
				}
			}
			else {
				ss.str(""); // Clear stringstream
				ss << tools->getColoredString(
					"Invalid command. Use 'continue', 'skip N', or 'skip all'",
					eColor::lightPink
				) << newLine;
				tools->writeLine(ss.str());
			}
		}
		// User Interaction Loop - END

		// Clean up breakpoint states if we're in post-execution
		if (stage == eBreakpointState::postExecution) {
			for (const auto& bp : hitBreakpoints) {
				bp->clear();
			}
		}

		// Return false if processing should be aborted
		return !abortProcessing;
	}

	// Return true if no breakpoints were hit
	return true;
}


eBlockFormationStatus CTransactionManager::getBlockFormationStatus(bool keyBlock)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);

	if (keyBlock)
		return mKeyBlockFormationStatus;
	else
	return mRegBlockFormationStatus;
}

// This helper method performs only basic pre-checks on the transaction
// and returns an eTransactionValidationResult without modifying state.
// It does NOT execute or finalize the transaction and does NOT do any
// database commits or rollback logic.
eTransactionValidationResult CTransactionManager::preValidateTransaction(std::shared_ptr<CTransaction> trans,
	uint64_t keyHeight,
	std::shared_ptr<CTransactionDesc> txDesc
	) const
{
	// 0) Validate bytecode can be decompiled - terminally invalid if bytecode is corrupted
	// This check ensures transactions with invalid bytecode are rejected early and marked as such
	if (trans->getCode().size() > 0)
	{
		std::shared_ptr<CGridScriptCompiler> compiler = mBlockchainManager->getCompiler();
		if (compiler)
		{
			std::string testSourceCode;
			if (!compiler->decompile(trans->getCode(), testSourceCode))
			{
				return eTransactionValidationResult::invalidBytecode;
			}
		}
	}

	// 1) Check if both pubKey and issuer are supplied => if so, verify they match
	if (!trans->getPubKey().empty() && !trans->getIssuer().empty())
	{
		// Re-generate the address from the supplied pubKey
		std::vector<uint8_t> testAddress;
		if (!mCryptoFactory->genAddress(trans->getPubKey(), testAddress))
		{
			// Invalid pubkey -> cannot generate address
			return eTransactionValidationResult::invalid;
		}

		// Compare with the transaction's "issuer" address
		if (testAddress.size() != trans->getIssuer().size() ||
			std::memcmp(testAddress.data(), trans->getIssuer().data(), testAddress.size()) != 0)
		{
			return eTransactionValidationResult::pubNotMatch;
		}
	}

	// 2) If no pubKey given, try to retrieve it from the transaction's issuer domain (IDToken)
	std::vector<uint8_t> pubKey = trans->getPubKey();
	if (pubKey.empty())
	{
		if (trans->getIssuer().empty())
		{
			// We have neither a pubKey nor an issuer.
			return eTransactionValidationResult::unknownIssuer;
		}

		// Find the issuer’s state domain by its ID
		CStateDomain* issuerSD = mStateDomainManager->findByID(trans->getIssuer());
		if (!issuerSD)
		{
			return eTransactionValidationResult::unknownIssuer;
		}

		// Obtain identity token to get the public key
		std::shared_ptr<CIdentityToken> idToken = issuerSD->getIDToken();
		if (!idToken)
		{
			return eTransactionValidationResult::noIDToken;
		}

		// Identity token must carry a valid public key
		if (idToken->getPubKey().size() != 32)
		{
			return eTransactionValidationResult::invalid;
		}

		// We found the pub key in the IDToken
		pubKey = idToken->getPubKey();
	}

	// 3) After the above steps, we must have a valid pubKey
	if (pubKey.size() != 32)
	{
		return eTransactionValidationResult::noPublicKey;
	}

	// 4) Verify the transaction's signature using the determined pubKey
	if (!trans->verifySignature(pubKey))
	{
		return eTransactionValidationResult::invalidSig;
	}

	// 5) Check that we can compute a valid domain ID from the pubKey
	std::vector<uint8_t> domainID;
	if (!mCryptoFactory->genAddress(pubKey, domainID))
	{
		return eTransactionValidationResult::invalid;
	}

	// 6) Confirm that the issuer’s state domain actually exists
	CStateDomain* issuerSD = mStateDomainManager->findByID(domainID);
	if (!issuerSD)
	{
		return eTransactionValidationResult::unknownIssuer;
	}

	// 7) Validate the ERG price bid
	BigInt ergBid = trans->getErgPrice();
	if (ergBid == 0 || ergBid < mSettings->getMinNodeERGPrice(keyHeight))
	{
		return eTransactionValidationResult::ERGBidTooLow;
	}

	// 8) Validate the issuer’s balance can cover (ergLimit × ergPrice)
	BigInt balance = issuerSD->getBalance();
	if (balance < (trans->getErgLimit() * ergBid))
	{
		return eTransactionValidationResult::invalidBalance;
	}

	// 9) Check the nonce 
	uint64_t currentNonce = issuerSD->getNonce();
	if (trans->getNonce() <= currentNonce )
	{
		return eTransactionValidationResult::invalidNonce;
	}

	// 10) If the transaction is a "sacrificial" transaction, verify its structure/semantics
	if (false && trans->ColdProperties.isASacrifice())
	{
		uint64_t dummyValue = 0;
		// This merely checks that the sacrificial format is correct, no state changes
		if (!mScriptEngine->verifySacrificialTransactionSemantics(*trans, dummyValue))
		{
			return eTransactionValidationResult::invalidSacrifice;
		}
	}

	// 11) account for amounts issues through the Value Transfer API
	// Notice - to support this - the GridScript code package needed to have its code decompiled and high-level meta-data inefferred first.
	if (txDesc)
	{
		if (balance < ((trans->getErgLimit() * ergBid) + txDesc->getValue()))
		{
			return eTransactionValidationResult::invalidBalance;
		}
	}


	// If we pass all prechecks, we consider the transaction 'valid' at pre-validation stage
	return eTransactionValidationResult::valid;
}


/// <summary>
/// Process a transaction.
/// 
/// The transaction is signed with a private key.
/// The corresponding public key NEEDS to match a certain StateDomainID. Reason: we need to deduct the script processing costs.
/// The public key MIGHT be included. IF it is NOT included; then we expect the public key to be available within an IdentityToken
/// within the caller's state domain.
/// 
/// Transactions are processed from the viewpoint of the current Perspective within mLocalStateDB.
/// </summary>
/// <param name="trans"></param>
/// <returns></returns>
CReceipt CTransactionManager::processTransaction(CTransaction & trans, uint64_t keyHeight, std::vector<uint8_t> &confirmedDomainID, std::vector<uint8_t> receiptID, const uint64_t& nonce, 
	std::shared_ptr<CReceipt> existingReceipt, std::shared_ptr<CBlock> existingBlock, bool excuseERGusage, bool enforceFailure)
{
	std::shared_ptr<CTools> tools = getTools();


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	// RAII Logging - BEGIN
	std::stringstream debugLog;
	// Create an RAII logger that will output debugLog's content on destruction
	CProcessTransactionRAII logFinalizer(debugLog, getTools());

	bool commit = !mScriptEngine->isInSandbox(); // mimic ACID mode after GridScritp Engine.
	// Record method arguments & perspective
	debugLog << tools->getColoredString("[processTransaction]", eColor::lightCyan)<< " Called with =>\n"
		
		<< " - commit: " << (commit ? "true" : "false") << "\n"
		<< " - trans.hash: " << tools->base58CheckEncode(trans.getHash()) << "\n"
		<< " - keyHeight: " << keyHeight << "\n"
		<< " - receiptID: "
		<< (receiptID.size() ? tools->base58CheckEncode(receiptID) : "[no ID]") << "\n"
		<< " - nonce: " << nonce << "\n"
		<< " - existingReceipt: " << (existingReceipt ? "true" : "false") << "\n"
		<< " - existingBlock: " << (existingBlock ? "true" : "false") << "\n"
		<< " - excuseERGusage: "<< (excuseERGusage ? "true" : "false") << "\n"
		<< " - enforceFailure: " << (enforceFailure ? "true" : "false") << "\n"
		<< "[processTransaction] Current perspective: "
		<< tools->base58CheckEncode(mFlowStateDB->getPerspective()) << "\n"
		<< std::endl;
	// RAII Logging - END
#endif
	assertGN(getIsInFlow());
	std::lock_guard<ExclusiveWorkerMutex> lock(mFlowStateDB->mGuardian);

	//Local Variables - BEGIN
	std::lock_guard<std::recursive_mutex> lock2(mGuardian);
	mStatisticsCounterGuardian.lock();
	mProcessedTransactions++;
	mStatisticsCounterGuardian.unlock();

	// Needs to be together - BEGIN
	CReceipt preliminaryReceipt(receiptID);//returned if  we don't get to execute the actual GridScript code
	preliminaryReceipt.setVerifiableID(trans.getHash());
	// Needs to be together - END

	// [ Security CVE_2025_6]  - BEGIN
	if (!existingBlock)// todo: uncomment this on next fork. For now we reject only if just received from the network.
	{
		if (trans.getErgLimit() < CGlobalSecSettings::getMinERGLimit())
		{
			if (!existingBlock->getIsCheckpointed())
			{  // excuse this transaction if part of an already checkpointed block.
				preliminaryReceipt.setResult(eTransactionValidationResult::ERGBidTooLow); // yes, we reuse same error code.
				trans.ColdProperties.markAsInvalid();
				return preliminaryReceipt;
			}
		}
	}
	// [ Security CVE_2025_6]  - END


	//mScriptEngine->reset(); happens in a processScriptCall()

	// Transaction Source Rewrite System - BEGIN
	// [ Rationale ]: For backwards compatibility and security patches, override specific
	//                transaction bytecode processing with replacement GridScript source code.
	//                This allows consensus-critical fixes for already-committed transactions.
	std::string source;
	bool sourceRewritten = false;

	if (existingBlock && receiptID.size() > 0)
	{
		// Get block height (NOT key-height) and receipt ID
		uint64_t blockHeight = existingBlock->getHeader()->getHeight();
		std::string receiptIDBase58 = tools->base58CheckEncode(receiptID);
		std::string replacementSource;

		// Check if this transaction has a source code rewrite entry
		if (CGlobalSecSettings::getTransactionSourceRewrite(receiptIDBase58, blockHeight, replacementSource))
		{
			// Rewrite found - use replacement source instead of decompiling bytecode
			source = replacementSource;
			sourceRewritten = true;

			// Log the rewrite event for audit trail
			tools->logEvent(
				"Transaction source rewrite applied: receipt=" + receiptIDBase58 +
				", height=" + std::to_string(blockHeight) +
				", replacement_length=" + std::to_string(replacementSource.length()),
				eLogEntryCategory::VM,
				0
			);
		}
	}

	// If no rewrite was applied, perform standard bytecode decompilation
	if (!sourceRewritten)
	{
		source = mScriptEngine->getSourceCode(trans.getCode());//translate byte-code to source code
	}
	// Transaction Source Rewrite System - END
	std::shared_ptr<CIdentityToken> ID; //it MIGHT be needed
	CStateDomain * issuerSD = nullptr; //the state domain belonging to the one who issued the transaction
	uintptr_t stackTopValue = -1; //the stack topmost value after script execution finished
	BigInt ERGUsed = 0;
	BigInt balanceA;
	std::vector<uint8_t> pubKey; //we need a public key to be able to verify the CTransaction's signature.
	//we need to deduct processing fees from somewhere so it also authorizes deduction from the issuer's state domain.
	//the public key might be included within the CTransaction but it may be not and then we will try to fetch the public key
	//from an Identity Token if it is registered within the source State Domain.
	//If public key is included, the source domain ID is not required.

	//mInFlowInterPerspectives.push_back(mFlowStateDB->getPerspecftive());//store the current Perspective so to make the transaction retractable during Flow processing
	CTrieDB *inFlowTempDB = new CTrieDB(*mFlowStateDB);
	//Local Variables - END
	
	//if (!mFlowStateDB->isEmpty(false));//check if ever hit
	//mFlowStateDB->copyTo(inFlowTempDB, true);//error: the content of mFlowStateDB must have been changed somewhere in between
	if (std::memcmp(inFlowTempDB->getPerspective().data(), mFlowStateDB->getPerspective().data(), 32) != 0)
	{
		delete inFlowTempDB;
		preliminaryReceipt.setResult(eTransactionValidationResult::incosistentData);
		trans.ColdProperties.markAsInvalid();
		return preliminaryReceipt; 
	}



	

	// Flow Backup (2/4) - BEGIN ( Level 2 )
	// [ Rationale ]: so that Flow mechanics can revert to state right before TX processing began.
	pushDB(inFlowTempDB, true);
	// Flow Backup (2/4) - END

		pubKey = trans.getPubKey();// is the public key included? 

		if (trans.getPubKey().size() != 0 && trans.getIssuer().size() != 0)
		{
			std::vector<uint8_t> testAdr;
			mCryptoFactory->genAddress(trans.getPubKey(), testAdr);
			if (std::memcmp(testAdr.data(), trans.getIssuer().data(), testAdr.size()) != 0)
			{
				preliminaryReceipt.setResult(eTransactionValidationResult::pubNotMatch);
				return preliminaryReceipt;
			}
		}

		//check for an explicit pub-key
		if (trans.getPubKey().size() == 32)
		{
			pubKey = trans.getPubKey();
		}
		else
		{
			//check for a StateDomain ID and try to fetch the IDToken, from which we would extract the public key
			if (trans.getIssuer().size() > 0)
			{
				// [ Security ] Silenced Domains - BEGIN
				if (existingBlock == nullptr) // applies only to new incoming transactions which are not part of a block yet.
				{
					bool silence = false;
					std::string issuerID = tools->bytesToString(trans.getIssuer());
					// Static cache for silenced domains set (assuming getSilencedDomains() is stable)
					static std::unordered_set<std::string> silencedSet = []() {  // No capture needed
						std::unordered_set<std::string> set;
						std::vector<std::string> silenced = CGlobalSecSettings::getSilencedDomains();
						for (const auto& domain : silenced)
						{
							set.insert(domain);
						}
						return set;
						}();
					// Check if issuer is silenced
					if (silencedSet.find(issuerID) != silencedSet.end())
					{
						silence = true;
					}
					if (silence)
					{
						preliminaryReceipt.setResult(eTransactionValidationResult::invalidSig);
						preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
						return preliminaryReceipt;
					}
				}
				// [ Security ] Silenced Domains - END

				issuerSD = mStateDomainManager->findByID(trans.getIssuer());
				if (issuerSD == NULL)
				{
					preliminaryReceipt.setResult(eTransactionValidationResult::unknownIssuer);
					preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
					return preliminaryReceipt;
				}
				ID = issuerSD->getIDToken();

				if (ID == NULL)
				{
					preliminaryReceipt.setResult(eTransactionValidationResult::noIDToken);
					preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
					return preliminaryReceipt;
				}
				else if (ID->getPubKey().size() != 32)
				{
					preliminaryReceipt.setResult(eTransactionValidationResult::invalid);
					preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
					return preliminaryReceipt;
					//this should not happen. valid IDToken needs to have a public key.

				}
				else//seems like a valid pub key is present
					pubKey = ID->getPubKey();

			}
			else
			{
				preliminaryReceipt.setResult(eTransactionValidationResult::unknownIssuer);
				preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
				return preliminaryReceipt;
			}
		}

		//finally do we have a valid pub-key?
		if (pubKey.size() != 32)
		{
			preliminaryReceipt.setResult(eTransactionValidationResult::noPublicKey);
			preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
			return preliminaryReceipt;
		}

		//verify the main signature (there might and will be additional signatures inside of the GridScript code)
		if (!trans.verifySignature(pubKey))
		{
			//error: signature verification failed
			preliminaryReceipt.setResult(eTransactionValidationResult::invalidSig);
			preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
			return preliminaryReceipt;
		}

		//compute the State Domain identifier (even if present)
		std::vector<uint8_t> stateDomainID;
		if (!mCryptoFactory->genAddress(pubKey, stateDomainID))
		{
			preliminaryReceipt.setResult(eTransactionValidationResult::invalid);
			preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
			return preliminaryReceipt;
		}

		//fetch state domain based on the StateDomainID
		issuerSD = mStateDomainManager->findByID(stateDomainID);

		if (issuerSD == nullptr)
		{	// we wouldn't have from where to deduct the processing fees from
			preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
			preliminaryReceipt.setResult(eTransactionValidationResult::unknownIssuer);
			confirmedDomainID.clear();
			return preliminaryReceipt;
		}

		confirmedDomainID = issuerSD->getAddress();

		//verify the 'Envelope Nonce'
		//this nonce verification IS required to prevent replay attacks when code is executing in pre-authentication mode
		//i.e. when Security Token is empty with only its stub present for function calls.
		//it is also NEEDED when code is NOT executing in pre-authentication mode.
		//i.e. otherwise agent issuing an invalid transaction (i.e. with invalid nonce within
		//security token of send() or xvalue() invocation) would be charged anyway the processing fees, since the transaction had been already
		//validated in his name. Thus, the 'global' transaction's nonce - the "Envelope Nonce" is needed to mitigate such situations and
		//possibly

		const_cast<uint64_t&> (nonce) = issuerSD->getNonce();

		if ((nonce +1) != trans.getNonce())
		{
			// Compatibility Layer CVE_803 - BEGIN
			/*
			* CVE_803 was about invalid handling of nonce values on failing transactions.
			* This compatibility layer ensures the transaction is allowed if the block it's contained within had it processed successfully.
			*/
			if (existingBlock && existingReceipt &&
				existingBlock->getIsCheckpointed() && // only for checkpointed blocks
				(existingReceipt->getResult() == eTransactionValidationResult::valid || existingReceipt->getResult() == eTransactionValidationResult::validNoTrieEffect))
			{
				// exisiting checkpointed block - do nothing.
				
				//preliminaryReceipt.setTargetNonce(nonce + 1); <- not needed. It's always assumed as such after authenticated code was processed.
				// thus, such an event would occut only once for an account affected by this CVE.
			}// Compatibility Layer CVE_803 - END
			else
			{
				// Block Formation - BEGIN
				
				//a potential incoming replay attack
				preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
				preliminaryReceipt.setResult(eTransactionValidationResult::invalidNonce);//do not even store the TX

				if (trans.getNonce() > issuerSD->getNonce())
				{
					// check if TX nonce higher than the one associated with an account
					// only then do we allow for deferred processing. That is under the assumption that nonce can only increase.
					trans.HotProperties.markRequiresTrigger(true);
					trans.HotProperties.setTriggerType(static_cast<uint8_t>(TriggerType::sourceDomainID));
				}
				return preliminaryReceipt;
				// Block Formation - END
			}
			
		}

		// Compatibility Layer CVE_805  (Part 2) - BEGIN (see also CVE_804, CVE_804)
		if (enforceFailure)
		{
		// -------- Update State-Domain intrinsic stats - BEGIN  -------- 
		// [ Rationale ]: so that issuers see same results in Terminal , UI dApps and mobile apps.
		// [ Applicability ]: valid only for value transfers.
		
			// Local Variables - BEGIN
			std::shared_ptr<CAccessToken> sysToken = CAccessToken::genSysToken();//generate kernel-mode Access Token
			std::shared_ptr<CSecDescriptor> sysDesc = CSecDescriptor::genSysOnlyDescriptor();// generate kernel-mode Security Descriptor

			std::string TXStatsContainerID = CGlobalSecSettings::getTXStatsContainerID();
			eDataType::eDataType dType;
			uint64_t nulledCost = 0;
			std::vector<uint8_t> TXInfoBytes;
			std::vector<std::vector<uint8_t>> TXNibbles;//A tuple of: [FLAGS, REG_RECEIPT_ID, fully qualified address, BigSInt value] byte-vectors
			// Local Variables - END

			// Infer Value Transfer Meta-Data (optional) - BEGIN
			
			// [ Rationale ]: a GridScript code package MAY not be a value transfer at all. In case it here were we attempt to derive additional higher level information.
			//				  The information will be used to fill meta-data fields more accurately.

			// Source Code Retrieval - BEGIN
			std::string sourceCode = trans.getSourceCode();

			BigInt txValue = 0;
			std::string  recipient;
			std::shared_ptr<CGridScriptCompiler> compiler =  mBlockchainManager->getCompiler();
			std::shared_ptr<CSendCommandParsingResult> parsingResult;
			assertGN(compiler, "Compiler not available.");
			bool interpretatinSucceeded = false;

			// Rationale: if cached source code not yet available.
			if (sourceCode.empty()) {
				// Transaction Source Rewrite System - BEGIN
				// Check if this transaction has a source code rewrite entry before decompilation
				bool sourceRewritten = false;

				if (existingBlock && receiptID.size() > 0)
				{
					uint64_t blockHeight = existingBlock->getHeader()->getHeight();
					std::string receiptIDBase58 = tools->base58CheckEncode(receiptID);
					std::string replacementSource;

					if (CGlobalSecSettings::getTransactionSourceRewrite(receiptIDBase58, blockHeight, replacementSource))
					{
						// Rewrite found - use replacement source instead of decompiling
						sourceCode = replacementSource;
						sourceRewritten = true;
						trans.setSourceCode(sourceCode); // cache it

						// Log the rewrite event for audit trail
						tools->logEvent(
							"Transaction source rewrite applied (enforceFailure): receipt=" + receiptIDBase58 +
							", height=" + std::to_string(blockHeight) +
							", replacement_length=" + std::to_string(replacementSource.length()),
							eLogEntryCategory::VM,
							0
						);
					}
				}

				// If no rewrite was applied, perform standard bytecode decompilation
				if (!sourceRewritten)
				{
					if (compiler->decompile(trans.getCode(), sourceCode))
					{
						trans.setSourceCode(sourceCode);// make decompiled source code available for re-use
					}
				}
				// Transaction Source Rewrite System - END
			}
			// Source Code Retrieval - END
			
			// Source Code Interpretation
			if (!sourceCode.empty())
			{
				parsingResult = parseSendCommand(sourceCode, keyHeight);
				if (parsingResult && parsingResult->isSuccess())
				{
					interpretatinSucceeded = true;
					txValue = parsingResult->getAmount();
					recipient = parsingResult->getRecipient();
				}
				// Infer Value Transfer Meta-Data (optional) - END

				if (interpretatinSucceeded)// [ Rationale ]: originally also ONLY value transfers make their way into the 'TXStatsContainerID' domain-scope data store.
				{
					// -------- Update Issuer - BEGIN --------

					// Retrieve current TX stats

					TXInfoBytes = issuerSD->loadValueDB(sysToken, TXStatsContainerID, dType);

					//attempt to decode these (may be empty)
					mTools->VarLengthDecodeVector(TXInfoBytes, TXNibbles);

					SE::txStatFlags sf;
					sf.valueTransfer = 1;

					//push flags 
					TXNibbles.push_back(sf.getBytes());  // type of code package
					//push receipt ID
					TXNibbles.push_back(receiptID);
					// push current destination
					TXNibbles.push_back(mTools->stringToBytes(recipient));// destination - get from meta data / decompile?
					// push current value

					BigSInt sValue = ((BigSInt)-1 * (BigSInt)txValue);

					TXNibbles.push_back(mTools->BigSIntToBytes(txValue > 0 ? sValue : 0)); // egress value transfer value

					// BER (re)encode all TX stats
					TXInfoBytes = mTools->VarLengthEncodeVector(TXNibbles, 1);

					std::vector<uint8_t> perspectivePrior = getPerspective();
					std::vector<uint8_t> perspectivePosterior;
					// save BER encoded TX stats
					if (!issuerSD->saveValueDB(sysToken, TXStatsContainerID, TXInfoBytes, eDataType::bytes, perspectivePosterior,
						nulledCost,
						mInSandbox, // <-  [ IMPORTANT ]: enable Sandbox Mode only if NOT comitting.
						true,
						"", "", nullptr, false, false, true, sysDesc))
					{
						preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
						preliminaryReceipt.setResult(eTransactionValidationResult::invalid);
						return preliminaryReceipt;
					}


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					if (!mTools->compareByteVectors(perspectivePosterior, perspectivePrior))
					{
						debugLog << mTools->getColoredString("Force-Failed transaction meta data had no Trie effect",eColor::lightPink) << ".\n";
					}
#endif

					//assertGN(!mTools->compareByteVectors(perspectivePoterior, perspectivePrior), "Failed transaction meta data had no effect.");

					// -------- Update Issuer - END  --------
				}

			}
			// simulate exact  in-block result.	
			

			return *existingReceipt;
			// -------- Update State-Domain intrinsic stats - END  -------- 
		}
		// Compatibility Layer CVE_805  (Part 2) - END
	
		//Validate ERG-Price - BEGIN
		BigInt ergBid = trans.getErgPrice();
		if (ergBid == 0 || ergBid < mSettings->getMinNodeERGPrice(keyHeight))
		{
			//Warning: the higher variance in this value - the higher network fragmentation.
			//By default, min ERG price is set to 1.
			preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
			preliminaryReceipt.setResult(eTransactionValidationResult::ERGBidTooLow);
			return preliminaryReceipt;
		}
		//Validate ERG-Price - BEGIN
		
		//TODO: check if transaction issuer has sufficient resources to cover the processing costs.
		
		if (!(balanceA =issuerSD->getBalance()))
		{
			//in such a case the transaction wouldn't even be included within the blockchain at all
			//as that would be a waste of storage
			//we are concerned ONLY with transaction that do modify the state of the decentralized database
			//transaction processing needs to COST something.
			//we cannot allow for anonymous 0-cost transactions as this way we would open-up to DOS/SPAM attacks
			preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
			preliminaryReceipt.setResult(eTransactionValidationResult::invalidBalance);
			return preliminaryReceipt;
		}

		//validate available resources
		if (balanceA < trans.getErgLimit() * trans.getErgPrice())
		{
			preliminaryReceipt.HotProperties.markDoNotIncludeInChain();
			preliminaryReceipt.setResult(eTransactionValidationResult::invalidBalance);
			return preliminaryReceipt;
		}

		//let's proceed to code execution
		mScriptEngine->setERGLimit(trans.getErgLimit());
	
		//properly handle the Sacrificial Transaction
		if (trans.ColdProperties.isASacrifice())
		{
			
			uint64_t value;
			if (!mScriptEngine->verifySacrificialTransactionSemantics(trans, value))
			{
				CReceipt rec(getBlockchainMode());
				rec.setResult(eTransactionValidationResult::invalidSacrifice);
				rec.logEvent("Invalid sacrifice");
				if (receiptID.size() >= 32)
					rec.setID(receiptID);
				return rec;
			}

	    }
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	// Debug Logging - BEGIN
	debugLog << "[processTransaction] About to run script. Perspective before script: "
		<< tools->base58CheckEncode(mFlowStateDB->getPerspective())
		<< std::endl;
	// Debug Logging - END
#endif
		
	//now, do the actual GridScript code processing
	mScriptEngine->setRegN(REG_KERNEL_THREAD);//SECURITY; triple check.
	CReceipt codeProcessingReceipt = mScriptEngine->processScript(source,
		issuerSD->getAddress(),
		stackTopValue, ERGUsed, 
		keyHeight,
		true, 
		true, 
		receiptID,
		true,
		false,
		false,
		false,
		false,
		0,
		false,
		false,
		excuseERGusage // <- [ SECURITY ] 
		);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	// Debug Logging - BEGIN
	debugLog << "[processTransaction] Script finished. Perspective after script: "
		<< tools->base58CheckEncode(mFlowStateDB->getPerspective())
		<< ", result: " << codeProcessingReceipt.translateStatus()
		<< std::endl;
	// Debug Logging - END
#endif
	mScriptEngine->cleanMemory(false);
	codeProcessingReceipt.setERGprice(trans.getErgPrice());

	if (receiptID.size() >= 32)
		codeProcessingReceipt.setID(receiptID);
	codeProcessingReceipt.setVerifiableID(trans.getHash());
	issuerSD = mStateDomainManager->findByID(stateDomainID);//the object might have been destroyed replaced etc.

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	// Debug Logging - BEGIN
	debugLog << "[processTransaction] Script finished. Perspective after script clean-up: "
		<< tools->base58CheckEncode(mFlowStateDB->getPerspective())
		<< ", result: " << codeProcessingReceipt.translateStatus()
		<< std::endl;
	// Debug Logging - END
#endif
	
	//deduct ERG
	//Extremely important: we deduct ERG *AFTER* the transaction has been processed and AFTER when the Perspective has been possibly REVERTED
	//in case the Transaction was decided to be invalid. Otherwise there would be no cost entitled to invalid transactions.
	//we process ONLY transactions originating from valid, verified accounts with non zero balances.
	//the claim ergPrice times maxErgUsage needs to be covered by the current state-domain's balance.
	//otherwise, the Transaction wouldn't even make it into the Blockchain in the first place
	//and the 'issuer' wouldn't even receive a verified Receipt.
	//that's a SPAM/DOS protection.

	return codeProcessingReceipt;

}
bool CTransactionManager::saveSysValue(bool doInSandbox, eSysDir::eSysDir dir, std::vector<uint8_t> key, std::vector<uint8_t> value, uint64_t& cost, bool revertPath)
{
	//Local variables - BEGIN
	std::vector<uint8_t> backupPath, currentDir;
	std::string pathBackUpStr;
	std::string errorStr;
	std::vector<uint8_t> newPerspectiveID;
	bool hiddenV = false;
	bool res = false;
	//Local variables - END

	CStateDomain* system = getStateDomainManager()->findByID(getTools()->stringToBytes(CGlobalSecSettings::getVMSystemDirName()));

	if (system == nullptr)
		return false;


	return system->saveValueDB(CAccessToken::genSysToken(), key, value, eDataType::bytes, newPerspectiveID, cost, doInSandbox, hiddenV, CGlobalSecSettings::getVMSysDirPath(dir));

	//create a file/value denoting the sacrificial TX as already 'consumed'
}
/// <summary>
/// Important: proposal needs to be set AND have a valid key-block height.
/// </summary>
/// <param name="ver"></param>
/// <param name="receiptID"></param>
/// <param name="proposal"></param>
/// <param name="feesValidatedUpToNow"></param>
/// <returns></returns>
/// <summary>
/// Important: proposal needs to be set AND have a valid key-block height.
/// </summary>
/// <param name="ver"></param>
/// <param name="receiptID"></param>
/// <param name="proposal"></param>
/// <param name="feesValidatedUpToNow"></param>
/// <returns></returns>
/// <summary>
/// Important: proposal needs to be set AND have a valid key-block height.
/// </summary>
/// <param name="ver"></param>
/// <param name="receiptID"></param>
/// <param name="proposal"></param>
/// <param name="feesValidatedUpToNow"></param>
/// <returns></returns>
CReceipt CTransactionManager::processVerifiable(
	CVerifiable ver, // <- [ IMPORTANT ]: this paload NEEDS to remain being a COOPY. It may be MODIFIED down below.
	BigInt& rewardValidatedUpToNow, // modified within
	BigInt& rewardIssuedToMinerAfterTAX,// modified within
	std::vector<uint8_t> receiptID, 
	std::shared_ptr<CBlock> proposal)

{



	/*
		IMPORTANT: [ SECURITY ]: Epoch 1 and Epoch 2 rewards are to be already accounted for as part of rewardValidatedUpToNow
				   BEFORE this method is called.

				   [ SECURITY ]:  Utlimately, Key Block Operator rewards are issued based on the rewardValidatedUpToNow value.  
								  It accounts for both Epoch rewards and TX processing fees.
	*/




//Local Variables - BEGIN
std::lock_guard<std::recursive_mutex>	lock(mGuardian);
assertGN(getIsInFlow());
std::shared_ptr<CTools> tools = getTools();

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
// RAII Logging - BEGIN
std::stringstream debugLog;
CProcessVerifiablesRAII loggerRAII(debugLog, getTools());

// Log method arguments & perspective
debugLog << tools->getColoredString("[processVerifiable]", eColor::lightCyan) + " Called with => \n"
<< " - Verifiable Type: " << tools->getColoredString(tools->getVerifiableTypeStr(ver.getVerifiableType()), eColor::orange) << "\n"
<< " - ver.hash: " << tools->base58CheckEncode(ver.getHash()) << "\n"
<< " - receiptID: " << (receiptID.size() ? tools->base58CheckEncode(receiptID) : "[none]") << "\n"
<< " - proposal: " << (proposal ? "true" : "false") << "\n"
<< "[processVerifiable] Current perspective: " << tools->base58CheckEncode(mFlowStateDB->getPerspective()) << "\n"
<< "[ Initial Reward Validated ]: " << tools->formatGNCValue(rewardValidatedUpToNow) << "\n"
<< "[ Initial Issued ]: " << tools->formatGNCValue(rewardIssuedToMinerAfterTAX) << "\n"
<< std::endl;
// RAII Logging - END
#endif

incProcessedVerifiablesCounter();
uint32_t progress = 0;
uint32_t count = 100;
int previousFlashed = 0;
if (receiptID.size() <= 32)
receiptID = tools->getReceiptIDForVerifiable(getBlockchainMode());
std::vector<std::string> mightBeThereAlready{ "Treasury" , CGlobalSecSettings::getVMSystemDirName() };
CReceipt rec(receiptID);
CReceipt receipt;
CStateDomain* domain;
rec.setReceiptType(eReceiptType::verifiable);
rec.setVerifiableID(ver.getHash());
std::vector<std::shared_ptr<CStateDomain>> affectedDomainStates;
std::vector<uint8_t> perspective = getPerspective();
assertGN(proposal != nullptr);
uint64_t currentKeyHeight = proposal->getHeader()->getKeyHeight();
uint64_t rewardLockPeriod = 0;
std::shared_ptr<CStatusBarHub> barHub = CStatusBarHub::getInstance();
//BigInt totalRewards = 0;
BigInt availableBalance = 0;
BigInt tax = 0;
BigInt ergused = 0;
eDataType::eDataType dType;
std::shared_ptr<CAccessToken> sysToken = CAccessToken::genSysToken();
std::string code = "VMMaintenance";
uint64_t res = 0, bh = 0;
std::vector<std::vector<uint8_t>> TXNibbles;
bool maintenanceFailed = false;

std::vector<uint8_t> perpetratorID, reportersID, stubB;
std::vector <std::shared_ptr<CStateDomain>> domainsAffected;
std::shared_ptr<CBlockHeader> kbh, dbh1, dbh2;
std::shared_ptr<CSecDescriptor> sysDesc = CSecDescriptor::genSysOnlyDescriptor();
std::shared_ptr<CLinkContainer> lc;
BigInt penalty = 0;
uint64_t nulledCost = 0;  // Used for operations that don't incur costs
BigInt PoFReward = 0;
std::string TXStatsContainerID = CGlobalSecSettings::getTXStatsContainerID();
std::vector<uint8_t> TXInfoBytes;
bool mightBe = false;
uint64_t stub = 0;
SE::txStatFlags sf;
CStateDomain* system = nullptr;
bool deemedAsInvalid = false;
CStateDomain* treasury = mStateDomainManager->findByID(CGlobalSecSettings::getTreasuryDomainID()); 
//				 ^------- 
// CVE-2025-GRIDNET-003-1: null pointer dereference on Treasury domain.
//						 [ Case ]: Issuance of mining reward to 1JWvujzgQvyAMdzAjh1eBA9vCycWcpvZGP caused memory corruption due to null-poinyer dereference while attempting to deliver tax
//								   to Treasury (1JWGpRxSUyDo2wKCNbMcHVxMKmYX4dYzh3). 
//								   Note common prefix for both domains ('1JW') notice that the actual prefix may be gently longer (we use niblles, which act at granularity lower than unitary ASCII code).
// 
//						 [ Cause ]: Reward issuance to a new domain MIGHT uproot any other node (domain) in the Merkle Patricia Trie.
//								    When this happens nodes MAY be deleted from memory and re-created at an adjusted MPT position.
// 
//		                 [ Fix ]: Pointer to treasury domain MUST be refreshed after State MPT is modified ( block reward issued to Operator etc.).
//						 		



uint64_t keyHeight = getBlockchainManager()->getKeyHeight();
eBlockchainMode::eBlockchainMode bMode = getBlockchainMode();
if (!treasury && keyHeight)
{
	rec.setResult(eTransactionValidationResult::invalid);
	return rec;
}



//Local Variables - BEGIN

//Operational Logic - BEGIN



BigSInt value = 0;
std::string stubStr;
std::shared_ptr<CIdentityToken> id;


// Security Check - BEGIN
eVerifiableType::eVerifiableType verType = ver.getVerifiableType();

if (proposal->getHeader()->isKeyBlock())
{
	// Key Block - BEGIN
	switch (verType)
	{
	case eVerifiableType::minerReward:
		break;
	case eVerifiableType::GenesisRewards:
		// allowed only once as part of the Genesis Key Block
		break;
	case eVerifiableType::uncleBlock:
		deemedAsInvalid = true;
		rec.setResult(eTransactionValidationResult::invalid);
		return rec;
		break;
	case eVerifiableType::dataPropagation:
		deemedAsInvalid = true;
		rec.setResult(eTransactionValidationResult::invalid);
		return rec;
		break;
	case eVerifiableType::powerGeneration:
		break;
	case eVerifiableType::powerTransit:
		deemedAsInvalid = true;
		rec.setResult(eTransactionValidationResult::invalid);
		return rec;
		break;
	case eVerifiableType::proofOfFraud:
		deemedAsInvalid = true;
		rec.setResult(eTransactionValidationResult::invalid);
		return rec;
		break;
	default:
		deemedAsInvalid = true;
		rec.setResult(eTransactionValidationResult::invalid);
		return rec;
		break;
	}
	// Key Block - END
}
else
{
	// Data Block - BEGIN
	switch (verType)
	{
	case eVerifiableType::minerReward:
		// it's fine - Data Blocks use minerReward Verifiables for Transactions' Fees redemptions to Operators on a per Data Block basis.
		break;
	case eVerifiableType::GenesisRewards:
		deemedAsInvalid = true;
		rec.setResult(eTransactionValidationResult::invalid);
		return rec;
		break;
	case eVerifiableType::uncleBlock:
		break;
	case eVerifiableType::dataPropagation:
		break;
	case eVerifiableType::powerGeneration:
		break;
	case eVerifiableType::powerTransit:
		break;
	case eVerifiableType::proofOfFraud:
		break;
	default:
		deemedAsInvalid = true;
		rec.setResult(eTransactionValidationResult::invalid);
		return rec;
		break;
	}

	// Data Block - END
}
// Security Check - END

	// [ SECURITY ]: Verify, Actuate and  Arm Payload 
	// Notice: payload MAY be actuated during verification.
	//         Example: Operator's Reward getting overriden by new block processing heuristics.
	//		   [ IMPORTANT 1 ]: in such a case 'verifyActuateAndArm()' needs to take care of backwards compatibility (within).
	//		   [ IMPORTANT 2 ]: verifyActuateAndArm() is thus allowed to MODIFY payload (the Verifiable).
    //							Useful when verifyActuateAndArm() needs to alter pending Operator's Reward 'on the fly'.
    //		   [ IMPORTANT 3 ]: It needs to be always wnsured that processVarifiable() take coopy of payload (Verifiable) not a reference to in-block object. 
	if (mVerifier->verifyActuateAndArm(ver, affectedDomainStates, proposal, rewardValidatedUpToNow))
	{

		// SECURITY  -  BEGIN
		if (ver.isArmed() == false)
		{

			assertGN(false);
		//	deemedAsInvalid = true;
			//rec.setResult(eTransactionValidationResult::invalid);
			//return rec;
		}
		// SECURITY  -  END

		switch (ver.getVerifiableType())
		{
		case eVerifiableType::minerReward:
			domainsAffected = ver.getAffectedStateDomains();
			//IMPORTANT: security performed earlier by verify() method.
			//NO VERIFICATION HERE; just change the balance
			//the verifiable needs to be the last one processed
			
			// FORMATION / Processing: Actuate Total Rewards #2(b)
			//rewardValidatedUpToNow += static_cast<BigInt>(domainsAffected[0]->getPendingBalanceChange());

			for (int i = 0; i < domainsAffected.size(); i++)
			{
				//what to do if recipient of the veritable is unknown? CREATE IT.
				domain = mStateDomainManager->findByID(domainsAffected[i]->getAddress());

				std::string domainAddress = tools->bytesToString(domainsAffected[i]->getAddress());
				std::vector<uint8_t> reportedAddress = proposal->getHeader()->getMinersID();

				if (reportedAddress.empty())
				{
					// in a data block public key needs to be available. See the beginning of CBlockchainExplorer::processBlock()
					deemedAsInvalid = true;
					rec.setResult(eTransactionValidationResult::incosistentData);
					return rec;
				}

				std::string reportedAddressStr = tools->bytesToString(reportedAddress);
				
				if (domain == nullptr)
				{
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					// Debug Logging - BEGIN
					debugLog << "[miner->create] BEFORE call => "
						<< " perspective: " << tools->base58CheckEncode(perspective)
						<< ", address: " << tools->bytesToString(domainsAffected[i]->getAddress())
						<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
						<< std::endl;
					// Debug Logging - END
#endif
				

					assertGN(getStateDomainManager()->create(perspective, &domain, mInSandbox, domainsAffected[i]->getAddress()));

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					// Debug Logging - BEGIN
					debugLog << "[miner->create] AFTER call => "
						<< " new perspective: " << tools->base58CheckEncode(perspective) 
						<< ", domain address: " << (domain ? tools->bytesToString(domain->getAddress()) : "[null]")
						<< std::endl;
					// Debug Logging - END
#endif
				}
				value = domainsAffected[i]->getPendingPreTaxBalanceChange();

				std::string temp = tools->attoToGNCStr((BigInt)value);

				//Calculate Tax - BEGIN
				double taxRate = CGlobalSecSettings::getMiningTax(proposal->getHeader()->getKeyHeight());
				if (value > 101) // avoid issues with 1%-100% rounding
				{
					if (taxRate >= 0.0 && taxRate <= 1.0) {
						long long taxFrac = static_cast<long long>(std::llround(taxRate * 1000000.0));
						BigInt bigFrac(taxFrac);
						BigInt bigDen(1000000);

						// Cast value to BigInt for calculation
						BigInt valueBI = static_cast<BigInt>(value);
						tax = (valueBI * bigFrac) / bigDen;
					}
				}
				temp = tools->attoToGNCStr(tax);
				//Calculate Tax - END


				//put tax into Treasury - BEGIN

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[treasury->changeBalanceBy] - TAX - BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", perspective: " << tools->base58CheckEncode(perspective)
					<< ", amount: " << tools->formatGNCValue(tax)
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif
				treasury = mStateDomainManager->findByID(CGlobalSecSettings::getTreasuryDomainID()); // MUST be renewed (CVE-2025-GRIDNET-003)
				if (treasury && !treasury->changeBalanceBy(perspective, tax, mInSandbox))
				{
					deemedAsInvalid = true;
					rec.setResult(eTransactionValidationResult::invalid);
					return rec;
				}
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[treasury->changeBalanceBy] - TAX -  AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", new perspective: " << tools->base58CheckEncode(perspective)
					<< ", domain balance: " << tools->formatGNCValue(domain->getBalance()) // or tools->attoToGNCStr(domain->getBalance())
					<< std::endl;
				// Debug Logging - END
#endif		
				//put tax into the Treasury - END

				temp = tools->attoToGNCStr((BigInt)(value - tax));
				BigInt rewardAfterTAX = 0;
				

				if (value > tax)
				{
					rewardAfterTAX = BigInt(value - tax);
				}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[miner->changeBalanceBy] - REWARD - BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", perspective: " << tools->base58CheckEncode(perspective)
					<< ", Reward Before TAX: " << tools->formatGNCValue(value)
					<< ", Reward After TAX: " << tools->formatGNCValue(rewardAfterTAX)
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif


				// Post Tax Verification - BEGIN
				if (!proposal->getIsCheckpointed())
				{
					if (rewardAfterTAX != proposal->getHeader()->getPaidToMiner())
					{
						deemedAsInvalid = true;
						rec.setResult(eTransactionValidationResult::invalid);
						return rec;
					}
				}
				// Post Tax Verification - END

				// Actual Reward - BEGIN
				if (!domain->changeBalanceBy(perspective, rewardAfterTAX, mInSandbox))
				{
					deemedAsInvalid = true;
					rec.setResult(eTransactionValidationResult::invalid);
					return rec;
				}


				rewardIssuedToMinerAfterTAX += rewardAfterTAX;

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1

				// Debug Logging - BEGIN
				debugLog << "[miner->changeBalanceBy] - REWARD -  AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", new perspective: " << tools->base58CheckEncode(perspective)
					<< ", domain balance: " << tools->formatGNCValue(domain->getBalance()) // or tools->attoToGNCStr(domain->getBalance())
					<< std::endl;
				// Debug Logging - END
#endif

				// Actual Reward - END

				rewardLockPeriod = getBlockchainManager()->getMinersRewardLockPeriod(proposal);
				temp = tools->attoToGNCStr(static_cast<BigInt>(domainsAffected[i]->getPendingPreTaxBalanceChange()));

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[miner->incLockedAssets] BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", perspective: " << tools->base58CheckEncode(perspective)
					<< ", keyHeight: " << currentKeyHeight
					<< ", reward: " << tools->formatGNCValue(rewardAfterTAX)
					<< ", lockUntil: " << (currentKeyHeight + rewardLockPeriod)
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif

				if (!domain->incLockedAssets(perspective, currentKeyHeight, rewardAfterTAX, mInSandbox, currentKeyHeight + rewardLockPeriod))
				{
					deemedAsInvalid = true;
					rec.setResult(eTransactionValidationResult::invalid);
					return rec;
				}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[miner->incLockedAssets] AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", new perspective: " << tools->base58CheckEncode(perspective)
					<< std::endl;
				// Debug Logging - END
#endif
				
				// Mining Reward Statistics - BEGIN
// Record statistical information for mining reward distribution


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN

				debugLog << "[miner->loadValueDB] - TX STATS - BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", key: " << TXStatsContainerID
					<< std::endl;
				// Debug Logging - END
#endif
				// Retrieve existing transaction statistics if any
				TXInfoBytes = domain->loadValueDB(sysToken, TXStatsContainerID, dType);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[miner->loadValueDB] - TX STATS - AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", loaded bytes size: " << TXInfoBytes.size()
					<< std::endl;
				// Debug Logging - END
#endif

				// Parse existing statistics entries
				mTools->VarLengthDecodeVector(TXInfoBytes, TXNibbles);

				// Generate transaction type flags
				// Set appropriate mining reward type in both enums
				SE::txStatFlags sf;
				sf.blockReward = 1;  // Set mining reward bit

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[TX Stats Build] About to record transaction entries:\n"
					<< "  transaction type identifier (blockReward=1): "
					<< tools->base58CheckEncode(sf.getBytes()) << "\n"
					<< "  receiptID: " << tools->base58CheckEncode(receiptID) << "\n"
					<< "  reportedAddress: " << tools->bytesToString(reportedAddress) << "\n"
					<< "  actualReward (post-tax): " << tools->formatGNCValue(rewardAfterTAX)
					<< " [in bytes => "
					<< tools->base58CheckEncode(mTools->BigSIntToBytes(rewardAfterTAX)) << "]"
					<< std::endl;
				// Debug Logging - END
#endif
				// Record transaction entries: [flags, receiptID, address, value]
				TXNibbles.push_back(sf.getBytes());                 // Transaction type identifier
				TXNibbles.push_back(receiptID);                     // Link to verifiable receipt
				TXNibbles.push_back(reportedAddress);               // Miner's wallet address
				TXNibbles.push_back(mTools->BigSIntToBytes(rewardAfterTAX)); // Post-tax reward amount

				// Update domain's transaction history
				TXInfoBytes = mTools->VarLengthEncodeVector(TXNibbles, 1);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[miner->saveValueDB] - TX STATS -  BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", perspective: " << tools->base58CheckEncode(perspective)
					<< ", key: " << TXStatsContainerID
					<< ", bytesSize: " << TXInfoBytes.size()
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif
				if (!domain->saveValueDB(sysToken, TXStatsContainerID, TXInfoBytes,
					eDataType::bytes, perspective, nulledCost, mInSandbox, true,
					"", "", nullptr, false, false, true, sysDesc))
				{
					deemedAsInvalid = true;
					rec.setResult(eTransactionValidationResult::invalid);
					return rec;
				}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[miner->saveValueDB] - TX STATS -  AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", new perspective: " << tools->base58CheckEncode(perspective)
					<< std::endl;
				// Debug Logging - END
#endif

				// Mining Reward Statistics - END

			}
			
			break;
		case eVerifiableType::proofOfFraud:
			/*
				IMPORTANT: we need to issue Proof-of-Fraud rewards (and penalties) right over here because Proof-of-Fraud may ba included
						  as part of Data Blocks. Data blocks do not (and certainly past data blocks) do not contain Miner Reward verifiable
						  so we cannot rely on Miner Reward verifiable as a trigger for Proof of Fraud processing.
			
			*/

			//NO VERIFICATION HERE; just change the balance
			if (!ver.getPoFProof(kbh, dbh1, dbh2, getBlockchainManager()->getMode()))
			{
				deemedAsInvalid = true;
				rec.setResult(eTransactionValidationResult::invalid);//unable to instantiate block-headers from within the Proof-of-Fraud; all 3 are needed. kbh contains public key and PoW (spam protection)
				return rec;
			}

			if (ver.getPubKey().size() == 32)
			{
				mCryptoFactory->genAddress(ver.getPubKey(), reportersID);
			}
			else
			{
				rec.setResult(eTransactionValidationResult::invalid);
				return rec;
			}

			//1) issue a penalty - i.e.no need to check if the asset-lock-time period expired.
			//even if the lock-time expired, we'll deduct assets from miner's domainount IF available (small chances), but doesnt cost us more than a few more CPU cycles.

			//totalRewards = kbh->getPaidToMiner();//totalGNCFees + consensusReward + coinbaseReward
			//DOES NOT include the previous-epoch reward.
			//we need to deduct ONLY the amount the node received based on this.
			//we do not deduct the reward for data-blocks nor do we deduct the past-epoch reward
			perpetratorID = kbh->getMinersID();
			domain = getStateDomainManager()->findByID(perpetratorID);

			if (domain == nullptr)
			{
				deemedAsInvalid = true;
				//this should NOT happen, the verifiable was Verified previously
				rec.setResult(eTransactionValidationResult::invalid);
				return rec;
			}

			availableBalance = domain->getBalance();

			if (availableBalance == 0)
			{
				rec.setResult(eTransactionValidationResult::invalid);
				return rec;
			}

			if (availableBalance > 0)// if not we're too late
			{
				BigInt standardPoFPenalty = CGlobalSecSettings::getPenaltyForPoF();

				penalty = ((availableBalance < standardPoFPenalty) ? availableBalance : standardPoFPenalty);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[fraudster->changeBalanceBy ] - PENALTY - BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", perspective: " << tools->base58CheckEncode(perspective)
					<< ", amount: " << tools->attoToGNCStr(penalty)
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif
				domain->changeBalanceBy(perspective, -1 * penalty, mInSandbox);


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[fraudster->changeBalanceBy] - PENALTY -  AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", new perspective: " << tools->base58CheckEncode(perspective)
					<< ", domain balance: " << domain->getBalance().str() // or tools->attoToGNCStr(domain->getBalance())
					<< std::endl;
				// Debug Logging - END
#endif
			}

// Proof-of-Fraud Statistics ( Fraudster Part ) - BEGIN
// Record statistical information for mining reward distribution


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN

			debugLog << "[fraudster->loadValueDB] - TX STATS - BEFORE => "
				<< " domainID: " << tools->bytesToString(domain->getAddress())
				<< ", key: " << TXStatsContainerID
				<< std::endl;
			// Debug Logging - END
#endif
				// Retrieve existing transaction statistics if any
			TXInfoBytes = domain->loadValueDB(sysToken, TXStatsContainerID, dType);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN
			debugLog << "[fraudster->loadValueDB] -  TX STATS - AFTER => "
				<< " domainID: " << tools->bytesToString(domain->getAddress())
				<< ", loaded bytes size: " << TXInfoBytes.size()
				<< std::endl;
			// Debug Logging - END
#endif

				// Parse existing statistics entries
			mTools->VarLengthDecodeVector(TXInfoBytes, TXNibbles);

			// Generate transaction type flags
			// Set appropriate mining reward type in both enums
			sf = SE::txStatFlags();
			sf.fraudProcessing = 1;  // Set Proof of Fraud bit

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN
			debugLog << "[TX Stats Build] About to record Proof of Fraud entries (PENALTY):\n"
				<< "  transaction type identifier (fraufProcessing=1): "
				<< tools->base58CheckEncode(sf.getBytes()) << "\n"
				<< "  receiptID: " << tools->base58CheckEncode(receiptID) << "\n"
				<< "  reportedAddress: " << tools->bytesToString(reportersID) << "\n"
				<< "  actualPenalty: " << tools->formatGNCValue(penalty)
				<< " [in bytes => "
				<< tools->base58CheckEncode(mTools->BigSIntToBytes(penalty)) << "]"
				<< std::endl;
			// Debug Logging - END
#endif
				// Record transaction entries: [flags, receiptID, address, value]
			TXNibbles.push_back(sf.getBytes());							// Transaction type identifier
			TXNibbles.push_back(receiptID);								// Link to verifiable receipt
			TXNibbles.push_back(reportersID);							// Reporter's wallet address
			TXNibbles.push_back(mTools->BigSIntToBytes(penalty));       // Penalty amount

			// Update domain's transaction history
			TXInfoBytes = mTools->VarLengthEncodeVector(TXNibbles, 1);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN
			debugLog << "[fraudster->saveValueDB] - TX STATS -  BEFORE => "
				<< " domainID: " << tools->bytesToString(domain->getAddress())
				<< ", perspective: " << tools->base58CheckEncode(perspective)
				<< ", key: " << TXStatsContainerID
				<< ", bytesSize: " << TXInfoBytes.size()
				<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
				<< std::endl;
			// Debug Logging - END
#endif
			if (!domain->saveValueDB(sysToken, TXStatsContainerID, TXInfoBytes,
				eDataType::bytes, perspective, nulledCost, mInSandbox, true,
				"", "", nullptr, false, false, true, sysDesc))
			{
				deemedAsInvalid = true;
				rec.setResult(eTransactionValidationResult::invalid);
				return rec;
			}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN
			debugLog << "[fraudster->saveValueDB] - TX STATS -  AFTER => "
				<< " domainID: " << tools->bytesToString(domain->getAddress())
				<< ", new perspective: " << tools->base58CheckEncode(perspective)
				<< std::endl;
			// Debug Logging - END
#endif

// Proof-of-Fraud Statistics ( Fraudster Part ) - END

			//2) reward agent who reported - BEGIN
			PoFReward = CGlobalSecSettings::getRewardForPoFProcessing();

				domain = getStateDomainManager()->findByID(reportersID);

				if (domain == nullptr)
				{
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					// Debug Logging - BEGIN
					debugLog << "[reporter->create] BEFORE call => "
						<< " perspective: " << tools->base58CheckEncode(perspective)
						<< ", address: " << tools->bytesToString(reportersID)
						<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
						<< std::endl;
					// Debug Logging - END
#endif
					assertGN(getStateDomainManager()->create(perspective, &domain, mInSandbox, reportersID));

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					// Debug Logging - BEGIN
					debugLog << "[reporter->create] AFTER call => "
						<< " new perspective: " << tools->base58CheckEncode(perspective)
						<< ", domain address: " << (domain ? tools->bytesToString(domain->getAddress()) : "[null]")
						<< std::endl;
					// Debug Logging - END
#endif

				}
				//BigFloat totalRewardsF = totalRewards.convert_to<BigFloat>();
				//rewardIssued = (totalRewardsF * BigFloat("0.25")).convert_to<BigInt>();

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[reporter->changeBalanceBy] -  Fraud Repot Reward - BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", perspective: " << tools->base58CheckEncode(perspective)
					<< ", amount: " << tools->attoToGNCStr(PoFReward)
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif
				domain->changeBalanceBy(perspective, PoFReward, mInSandbox);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[reporter->changeBalanceBy] -  Fraud Repot Reward - AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", new perspective: " << tools->base58CheckEncode(perspective)
					<< ", domain balance: " << domain->getBalance().str() // or tools->attoToGNCStr(domain->getBalance())
					<< std::endl;
				// Debug Logging - END
#endif
			

// Proof-of-Fraud Statistics ( Reward Part ) - BEGIN
// Record statistical information for Operator who reported Fraud


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN

				debugLog << "[reporter->loadValueDB] - TX STATS - BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", key: " << TXStatsContainerID
					<< std::endl;
				// Debug Logging - END
#endif
				// Retrieve existing transaction statistics if any
				TXInfoBytes = domain->loadValueDB(sysToken, TXStatsContainerID, dType);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[reporter->loadValueDB] -  TX STATS - AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", loaded bytes size: " << TXInfoBytes.size()
					<< std::endl;
				// Debug Logging - END
#endif

				// Parse existing statistics entries
				mTools->VarLengthDecodeVector(TXInfoBytes, TXNibbles);

				// Generate transaction type flags
				// Set appropriate mining reward type in both enums
				sf = SE::txStatFlags();
				sf.fraudProcessing = 1;  // Set Proof of Fraud bit

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[TX Stats Build] About to record Proof of Fraud entries (REWARD):\n"
					<< "  transaction type identifier (fraufProcessing=1): "
					<< tools->base58CheckEncode(sf.getBytes()) << "\n"
					<< "  receiptID: " << tools->base58CheckEncode(receiptID) << "\n"
					<< "  perpetratorAddress: " << tools->bytesToString(perpetratorID) << "\n"
					<< "  actualReward: " << tools->formatGNCValue(PoFReward)
					<< " [in bytes => "
					<< tools->base58CheckEncode(mTools->BigSIntToBytes(PoFReward)) << "]"
					<< std::endl;
				// Debug Logging - END
#endif
				// Record transaction entries: [flags, receiptID, address, value]
				TXNibbles.push_back(sf.getBytes());							// Transaction type identifier
				TXNibbles.push_back(receiptID);								// Link to verifiable receipt
				TXNibbles.push_back(perpetratorID);								// Who the perpetrator was
				TXNibbles.push_back(mTools->BigSIntToBytes(PoFReward));  // Rewarded amount

				// Update domain's transaction history
				TXInfoBytes = mTools->VarLengthEncodeVector(TXNibbles, 1);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[reporter->saveValueDB] - TX STATS -  BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", perspective: " << tools->base58CheckEncode(perspective)
					<< ", key: " << TXStatsContainerID
					<< ", bytesSize: " << TXInfoBytes.size()
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif
				if (!domain->saveValueDB(sysToken, TXStatsContainerID, TXInfoBytes,
					eDataType::bytes, perspective, nulledCost, mInSandbox, true,
					"", "", nullptr, false, false, true, sysDesc))
				{
					deemedAsInvalid = true;
					rec.setResult(eTransactionValidationResult::invalid);
					return rec;
				}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[reporter->saveValueDB] - TX STATS -  AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", new perspective: " << tools->base58CheckEncode(perspective)
					<< std::endl;
				// Debug Logging - END
#endif

// Proof-of-Fraud Statistics ( Reward Part ) - END

			//3) Reward Miner - END

			//what to do if recipient of the veritable is unknown? CREATE IT? why not; we wont know its KeyChain though.

			// FORMATION / Processing: Actuate Total Rewards #2(c)
			rewardValidatedUpToNow += PoFReward; // [ SECURITY ]  Only Data Blocks may contain non-Miner Reward Verifiables / Tranasctions so it's safe to alter this.
											     // Recall: utlimately Key Block Operator rewards are issued based on this value.
												 //         this value thus accounts for both Epoch rewards and TX processing fees.

			//mark the PoF as processed
			lc = std::make_shared<CLinkContainer>(tools->getProofOfFraudID(kbh->getHash()),
				receiptID, eLinkType::PoFIDtoReceiptID);
			addLinkToFlow(lc);
			incProofsOfFraudProcessed();
			incTotalValueOfFraudPenaltiesBy(penalty);
			incTotalValueOfPoFRewardsBy(PoFReward);
			break;

		case eVerifiableType::GenesisRewards:
			tools->writeLine("Commencing with the awards-issuance procedure..");
			tools->writeLine("Initalizing VMMaintenance..");
			tools->writeLine("Acquiring Overwatch privileges...");
			mScriptEngine->setRegN(REG_EXECUTING_BY_OVERWATCH);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN
			debugLog << "[mScriptEngine->processScript] BEFORE => "
				<< " perspective: " << tools->base58CheckEncode(perspective)
				<< ", code: " << code
				<< ", res: " << res
				<< ", ergused: " << ergused.str()
				<< std::endl;
			// Debug Logging - END
#endif
			tools->writeLine("Executing maintenance #GridScript code..");
			receipt = mScriptEngine->processScript(code, tools->stringToBytes("system"), res, ergused, bh, false);
			if (receipt.getResult() != eTransactionValidationResult::valid)
			{
				tools->writeLine(tools->getColoredString("Maintenance #GridScript code failure", eColor::cyborgBlood));
				maintenanceFailed = true;
			}
			else
			{
				tools->writeLine("Maintenance #GridScript code" + tools->getColoredString(" SUCCEEDED", eColor::lightGreen));
			}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN
			debugLog << "[mScriptEngine->processScript] AFTER => "
				<< " perspective: " << tools->base58CheckEncode(perspective)
				<< ", receipt result: " << receipt.translateStatus()
				<< ", used ERG: " << ergused.str()
				<< ", res: " << res
				<< std::endl;
			// Debug Logging - END
#endif
			tools->writeLine("Abandoning Overwatch privileges now.");
			mScriptEngine->clearRegN(REG_EXECUTING_BY_OVERWATCH);

			if (maintenanceFailed)
			{
				deemedAsInvalid = true;
				rec.setResult(eTransactionValidationResult::invalid);
				return rec;
			}
			tools->writeLine("Initalizing Completed.");

			system = getStateDomainManager()->findByID(tools->stringToBytes(CGlobalSecSettings::getVMSystemDirName()));
			if (!system)
			{
				deemedAsInvalid = true;
				rec.setResult(eTransactionValidationResult::invalid);
				return rec;
			}

			for (uint64_t i = 0; i < affectedDomainStates.size(); i++)
			{
				id = affectedDomainStates[i]->getTempIDToken();

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[domain->create] BEFORE call => "
					<< " perspective: " << tools->base58CheckEncode(perspective)
					<< ", address: " << tools->bytesToString(affectedDomainStates[i]->getAddress())
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif

				//within the below, do not check for a result as it might had been an attempt to re-create, say, the "Treasury"
				//it took place within updateSystemDomains() of CTransactionManager
				if (!getStateDomainManager()->create(perspective, &domain, mInSandbox, affectedDomainStates[i]->getAddress()))
				{
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					// Debug Logging - BEGIN
					debugLog << "[domain->create] AFTER call => "
						<< " new perspective: " << tools->base58CheckEncode(perspective)
						<< ", domain address: " << (domain ? tools->bytesToString(domain->getAddress()) : "[null]")
						<< std::endl;
					// Debug Logging - END
#endif

					domain = getStateDomainManager()->findByID(affectedDomainStates[i]->getAddress());

					if (!domain)
					{
						deemedAsInvalid = true;
						tools->writeLine(tools->getColoredString("Fatal Error creating genesis domains. I'll now quit.", eColor::cyborgBlood));
						getBlockchainManager()->exit(true);
						rec.setResult(eTransactionValidationResult::invalid);
						return rec;
					}

					mightBe = false;
					if (std::find(mightBeThereAlready.begin(), mightBeThereAlready.end(), tools->bytesToString(affectedDomainStates[i]->getAddress())) != mightBeThereAlready.end())
					{
						mightBe = true;
					}
					else if (std::find(mightBeThereAlready.begin(), mightBeThereAlready.end(), id->getFriendlyID()) != mightBeThereAlready.end())
					{
						mightBe = true;
					}

					if (!mightBe)
					{
						deemedAsInvalid = true;
						tools->writeLine(tools->getColoredString("Fatal Error creating genesis domains. I'll now quit.", eColor::cyborgBlood));
						getBlockchainManager()->exit(true);
						rec.setResult(eTransactionValidationResult::invalid);
						return rec;
					}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					// Debug Logging - BEGIN
					debugLog << "[domain->setBalance] BEFORE => "
						<< " domainID: " << tools->bytesToString(domain->getAddress())
						<< ", perspective: " << tools->base58CheckEncode(perspective)
						<< ", newBalance: 0"
						<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
						<< std::endl;
					// Debug Logging - END
#endif

					domain->setBalance(perspective, 0, mInSandbox);//make sure the balanse is Nulled, before we add assets as per the Gensis Facts

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					// Debug Logging - BEGIN
					debugLog << "[domain->setBalance] AFTER => "
						<< " domainID: " << tools->bytesToString(domain->getAddress())
						<< ", new perspective: " << tools->base58CheckEncode(perspective)
						<< ", domain balance: " << domain->getBalance().str()
						<< std::endl;
					// Debug Logging - END
#endif
				}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[domain->create] AFTER call => "
					<< " new perspective: " << tools->base58CheckEncode(perspective)
					<< ", domain address: " << (domain ? tools->bytesToString(domain->getAddress()) : "[null]")
					<< std::endl;
				// Debug Logging - END
#endif

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[domain->changeBalanceBy] - Pending Reward - BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", perspective: " << tools->base58CheckEncode(perspective)
					<< ", amount: " << tools->formatGNCValue(affectedDomainStates[i]->getPendingPreTaxBalanceChange())
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif
				assertGN(domain->changeBalanceBy(perspective, affectedDomainStates[i]->getPendingPreTaxBalanceChange(), mInSandbox));

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[domain->changeBalanceBy] - Pending Reward -  AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", new perspective: " << tools->base58CheckEncode(perspective)
					<< ", domain balance: " << domain->getBalance().str() // or tools->attoToGNCStr(domain->getBalance())
					<< std::endl;
				// Debug Logging - END
#endif
				// Genesis Rewards Statistics - BEGIN
				// Record statistical information for initial token distribution during genesis

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[domain->loadValueDB] BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", key: " << TXStatsContainerID
					<< std::endl;
				// Debug Logging - END
#endif

				// Retrieve existing transaction statistics if any exist for this domain
				TXInfoBytes = domain->loadValueDB(sysToken, TXStatsContainerID, dType);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[domain->loadValueDB] AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", loaded bytes size: " << TXInfoBytes.size()
					<< std::endl;
				// Debug Logging - END
#endif
				// Parse existing statistics entries
				TXNibbles.clear();  // Clear previous entries
				mTools->VarLengthDecodeVector(TXInfoBytes, TXNibbles);

				// Set transaction flags for genesis distribution
				
				sf.genesisReward = 1;  // Mark as genesis distribution

				// Build the statistics entry with 4 components:
				// 1. Transaction type flags
				// 2. Receipt identifier
				// 3. Recipient address
				// 4. Distribution amount
				TXNibbles.push_back(sf.getBytes());                    // Transaction type identifier
				TXNibbles.push_back(receiptID);                        // Link to verifiable receipt
				TXNibbles.push_back(affectedDomainStates[i]->getAddress()); // Genesis recipient address
				TXNibbles.push_back(mTools->BigSIntToBytes(           // Initial allocation amount
					affectedDomainStates[i]->getPendingPreTaxBalanceChange()));

				// BER encode and save updated statistics
				TXInfoBytes = mTools->VarLengthEncodeVector(TXNibbles, 1);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[domain->saveValueDB]  - TX STATS -BEFORE => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", perspective: " << tools->base58CheckEncode(perspective)
					<< ", key: " << TXStatsContainerID
					<< ", bytesSize: " << TXInfoBytes.size()
					<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
					<< std::endl;
				// Debug Logging - END
#endif
				if (!domain->saveValueDB(sysToken, TXStatsContainerID, TXInfoBytes,
					eDataType::bytes, perspective, nulledCost, mInSandbox, true,
					"", "", nullptr, false, false, true, sysDesc))
				{
					deemedAsInvalid = true;
					rec.setResult(eTransactionValidationResult::invalid);
					return rec;
				}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[domain->saveValueDB] - TX STATS - AFTER => "
					<< " domainID: " << tools->bytesToString(domain->getAddress())
					<< ", new perspective: " << tools->base58CheckEncode(perspective)
					<< std::endl;
				// Debug Logging - END
#endif
				// Genesis Rewards Statistics - END

				if (id)
				{

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					// Debug Logging - BEGIN
					debugLog << "[domain->setIDToken] BEFORE => "
						<< " domainID: " << tools->bytesToString(domain->getAddress())
						<< ", perspective: " << tools->base58CheckEncode(perspective)
						<< ", ID.friendlyID: " << id->getFriendlyID()
						<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
						<< std::endl;
					// Debug Logging - END
#endif
					domain->setIDToken(*id, stub, mInSandbox);


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					// Debug Logging - BEGIN
					debugLog << "[domain->setIDToken] AFTER => "
						<< " domainID: " << tools->bytesToString(domain->getAddress())
						<< ", new perspective: " << tools->base58CheckEncode(perspective)
						<< std::endl;
					// Debug Logging - END
#endif
					//create link within System's dir
					if (id->getFriendlyID().size() > 0)
					{
						stubStr = id->getFriendlyID();

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
						// Debug Logging - BEGIN
						debugLog << "[saveSysValue] BEFORE => "
							<< " friendlyID: " << stubStr
							<< ", domainID: " << tools->bytesToString(affectedDomainStates[i]->getAddress())
							<< ", stub: " << stub
							<< ", mInSandbox: " << (mInSandbox ? "true" : "false")
							<< std::endl;
						// Debug Logging - END
#endif
						if (!saveSysValue(mInSandbox, eSysDir::identityTokens, tools->stringToBytes(stubStr), affectedDomainStates[i]->getAddress(), stub))
						{
							deemedAsInvalid = true;
							rec.setResult(eTransactionValidationResult::invalid);
							return rec;
						}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
						// Debug Logging - BEGIN
						debugLog << "[saveSysValue] AFTER => "
							<< " perspective (if changed): " << tools->base58CheckEncode(perspective)
							<< std::endl;
						// Debug Logging - END
#endif
					}
				}

				progress = (int)(((double)i / (double)affectedDomainStates.size()) * 100);
				if (progress - previousFlashed > 0)
				{
					previousFlashed = progress;
					barHub->setCustomStatusBarText(bMode, 997, "completed: " + std::to_string((int)progress) + "% - money is falling from the sky..");
				}
				assertGN(getStateDomainManager()->findByID(affectedDomainStates[i]->getAddress()) != nullptr);

				assertGN(domain->getBalance() == affectedDomainStates[i]->getPendingPreTaxBalanceChange());
			}
			break;

		default:
			break;
		}

	}
	else
	{
		deemedAsInvalid = true;
		rec.setResult(eTransactionValidationResult::invalid);
		return rec;
	}
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	debugLog << "[ Final Reward Validated ]: " << tools->formatGNCValue(rewardValidatedUpToNow) << "\n";
	debugLog << "[ Final Issued ]: " << tools->formatGNCValue(rewardIssuedToMinerAfterTAX) << "\n";
#endif
	if (!deemedAsInvalid)
	{
		rec.setResult(eTransactionValidationResult::valid);
	}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	// Debug Logging - BEGIN
	debugLog << tools->getColoredString("[processVerifiable]", eColor::lightCyan)+ " Final result : "
		<< rec.translateStatus() << "\n"
		<< "Perspective at exit: "
		<< tools->base58CheckEncode(mFlowStateDB->getPerspective())
		<< std::endl;
	// Debug Logging - END
#endif
	//Operational Logic - END
	return rec;
}
/// <summary>
/// Does processing of a BER-encoded transaction.
/// </summary>
/// <param name="BERpackedTransaction"></param>
/// <param name="keyHeight"></param>
/// <returns></returns>
CReceipt CTransactionManager::processTransaction(std::vector<uint8_t> BERpackedTransaction, uint64_t keyHeight)
{
	assertGN(getIsInFlow());
	eTransactionValidationResult result = valid;
	CReceipt receipt(getBlockchainMode());
	if (BERpackedTransaction.size() > mGlobalSecSettings->getMaxTransactionSize())
		receipt.setResult(invalid);

	CTransaction *t = CTransaction::instantiate(BERpackedTransaction, getBlockchainMode());
	size_t ERG_used = 0;
	std::vector<uint8_t> confimredSDID;
	CReceipt res = processTransaction(*t, keyHeight,confimredSDID);
	delete t;

	return receipt;
}

std::vector<uint8_t> CTransactionManager::getCurrentFlowObjectID()
{
	return mCurrentFlowObjectID;
}

void CTransactionManager::setCurrentFlowObjectID(const std::vector<uint8_t>& id)
{
	mCurrentFlowObjectID = id;
}

/// <summary>
/// The purpose of this function is to share contents of the local Memory Pool with other peers.
/// That might be beneficial for both the local node (receives cuts from TX processing fees) and for the transaction issuer
/// since increases likelihood of having the TX processed. 
/// </summary>
/// <returns></returns>
uint64_t CTransactionManager::advertiseMemPoolContents()
{
	//Local Variables - BEGIN
	std::shared_ptr<CNetworkManager> nm = getBlockchainManager()->getNetworkManager();
	if (!nm)
		return 0;
	std::shared_ptr<CTools> tools = getTools();
	std::vector<uint8_t> hash;//to check if we've already delivered a particular data-package to a given remote peer.
	std::vector<std::shared_ptr<CTransaction>> txs = getUnprocessedTransactions(feesHighestFirstNonce,false);// advertise all contents, requiring triggers or not.
	std::vector<std::shared_ptr<CConversation>> convs = nm->getAllConversations(false, true, true,  true);
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::string remoteIP;
	std::string txID;
	pingLastTimeMemPoolAdvertised();
	
	if (txs.size() == 0)
	{
		tools->logEvent("Nothing to share from mem-pool..", eLogEntryCategory::network, 1, eLogEntryType::notification);
		return 0;
	}

	if (txs.size() == 0)
	{
		tools->logEvent("Nobody to share the local mem-pool with..", eLogEntryCategory::network, 3, eLogEntryType::warning,eColor::lightPink);
		return 0;
	}
	uint64_t advertisedToCount = 0;
	std::vector<uint8_t> txBytes;
	std::shared_ptr<CNetTask> task;
	std::shared_ptr<CNetMsg> msg;
	//Local Variables - END


	if (txs.size() == 0)
	{
		tools->logEvent("Attempting to share mem-pool ("+std::to_string(txs.size())+" elements) with "+ std::to_string(convs.size())+ " nodes..", eLogEntryCategory::network, 1, eLogEntryType::notification);
		return 0;
	}

	for (uint64_t i = 0; i < txs.size(); i++)
	{
		txBytes = txs[i]->getPackedData();//employs caching and optimization
		hash = txs[i]->getHash();//employs cached value from the above call
		txID = tools->getColoredString(tools->base58CheckEncode(hash),  eColor::lightCyan);//for logging only

		//traverse all the active conversations..
		for (uint64_t y = 0;y< convs.size(); y++)
		{
			if (convs[y]->getFlags().exchangeBlocks == false || convs[y]->getState()->getCurrentState() != eConversationState::running)
				continue;

			remoteIP = convs[y]->getIPAddress();

			if (nm->getWasDataSeen(hash, "localhost", false, remoteIP)==false)// check whether we've already shared TX with the other node
			{
				tools->logEvent("[DSM Sync]: I'll notify " + remoteIP +" ("+ tools->transportTypeToString( convs[y]->getProtocol())+")"+" about " +txID, eLogEntryCategory::network, 1, eLogEntryType::notification);//logging
				nm->sawData(hash, remoteIP);//so that we know that we've handed the TX over to the particular remote peer

				//create an a-sync network notification task
				msg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::transaction, eNetReqType::notify, txBytes);
				task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg, 1);
				task->setNetMsg(msg);
				if (convs[y]->addTask(task))
				{
					advertisedToCount++;
				}
			}
			else
			{
				tools->logEvent("[DSM Sync]: "+ remoteIP + " already got to know about " + txID, eLogEntryCategory::network, 0, eLogEntryType::notification);
			}
		}
	}

	if (advertisedToCount)
	{
		tools->logEvent(tools->getColoredString("Successfully dispatched ", eColor::lightGreen) + std::to_string(advertisedToCount) + " elements from mem-pool across " + std::to_string(convs.size()) + " nodes.", eLogEntryCategory::network, 10, eLogEntryType::notification);
	}
	//else
	//{
		//tools->logEvent("Unable to share TXs", eLogEntryCategory::network, 10, eLogEntryType::warning);
	//}
	
	return advertisedToCount;

}



/// <summary>
///
/// Forms either a Key-Block or a regular Block.
///
/// Key-Block constitutes mainly a vote to become a Leader, coupled with a Proof-of-Work.
/// Key Block does NOT contain any transactions etc.
///
/// A regular block contains these instead.
///
/// No matter whether the node is a leader or not, it would attempt to become one in next round by mining on the last known Key-Block.
/// Thus we've got two threads constantly forming blocks. One for Key Blocks and the other for regular blocks.
///
/// Regular blocks are signed according to the public key within the latest key-block.
///
/// If no leader is known and the blockchain is empty, the function would attempt to generate a Genesis Key Block followed by a Genesis Rewards Block.
/// Attempts to form a block.
///
/// Note: The function needs to come up with exactly the same results for the same set of transactions/verifiables as the processBlock function
/// within the BlockchainManager. Otherwise, the hash value of the Root of the resulting State Trie would NOT match. (we are checking for this).
///
/// Valid as well as invalid transactions make it to the final block.
/// We need invalid transactions for accountability of penalties and processing fees. (the processing of invalid transactions also needs to be paid for).
///
/// All transactions, even those invalid, leave a footprint inside a receipt.
/// Receipts are included in consecutive blocks as a receipt contains hash of the block in which the transactions was contained.
/// Invalid transactions are also marked as invalid within the block right away using ColdStorageFlags.
///
/// When an invalid transaction is encountered, the state of Transaction flow is reverted one state back.
/// Call to abortFlow() instead of endFlow() is made. No permanent changes to StateDB are to be made within this function.
/// For this - Blockchain Manager is responsible   with its validateBlock() and processBlock() functions.
///
/// The transaction processing function resets the state of the GridScript Engine; however the state of the entire GRIDNET VM (including the StateDB)
/// persists throughout the execution of consecutive scripts. These changes are not reflected onto the SolidStorage but are kept in RAM only
/// and erased/reverted when the processing Flow ends within the abortFlow() function.
///
/// ============================================================================
/// PHANTOM LEADER MODE
/// ============================================================================
/// When phantomMode is set to true, this function operates in "Phantom Leader Mode":
///
/// - PURPOSE: Allows non-leader nodes to simulate transaction processing for
///   debugging and transaction withholding attack detection purposes.
///
/// - TRANSACTION RETENTION: In phantom mode, transactions are NOT marked as
///   processed (markAsProcessed=false). This ensures that transactions remain
///   in the mem-pool after phantom block formation, allowing them to be properly
///   processed when the node becomes a legitimate leader or when a valid block
///   is received from the network.
///
/// - BLOCK NON-PROPAGATION: Blocks formed in phantom mode are NOT broadcast to
///   the network. They exist only for local analysis and debugging.
///
/// - STATE CHANGES: All state changes made during phantom block formation are
///   reverted via abortFlow(), ensuring no permanent modifications to StateDB.
///
/// - USE CASE: Activated via 'chain -phantom on' command when investigating
///   potential transaction withholding attacks or debugging transaction
///   processing issues without affecting network consensus.
/// ============================================================================
/// </summary>
/// <param name="block">The block proposal to be formed</param>
/// <param name="formKeyBlock">True to form a Key Block, false for Data Block</param>
/// <param name="chain">The key chain state to use for block formation</param>
/// <param name="forcedParentID">Optional forced parent block ID (for hard forks)</param>
/// <param name="phantomMode">When true, transactions remain in mem-pool after processing</param>
/// <returns>Block formation result status</returns>
eBlockFormationResult  CTransactionManager::formBlock(std::shared_ptr<CBlock> &block, bool formKeyBlock, CKeyChain chain, std::vector<uint8_t> forcedParentID, bool phantomMode)
{
	// Alpha Preliminaries - BEGIN
	uint64_t now = std::time(0);
	std::stringstream debugLog;
	bool error = false;
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	debugLog << " [ Block Formation Log ]" << "\n\n";
	// Create an RAII logger that will output debugLog's content on destruction
	CProcessTransactionRAII logFinalizer(debugLog, getTools());
#endif

	uint64_t lastKeyFormationAttempt = getLastKeyBlockFormationAttempt();
	if (formKeyBlock)
	{
		if ((now <= lastKeyFormationAttempt) || (now - lastKeyFormationAttempt) < CGlobalSecSettings::getMinKeyBlockFormationAttemptInterval())
		{
			mTools->logEvent("Can't attempt key block formation that often.", eLogEntryCategory::localSystem, 0);
			return eBlockFormationResult::aborted;
		}
		else
		{
			pingLastKeyBlockFormationAttempt();
		}
	}
	// Alpha Preliminaries - END

	/*
	WARNING: Flow-processing facilitates a critical section. There's a mutex protection built-into the Flow-processing mechanics.
	Still, also, any previous attempts to access the current FlowDB, need to ensure to lock the guardian of the DB first.
	The mutex is public and recursive.

	i.e. std::lock_guard<std::recursive_mutex>lg(mCurrentDB->mGuardian);
	*/
	//LOCAL VARIABLES - BEGIN ------------------------------------------
	std::shared_ptr<CVerifiable>ver;
	CVerifiable verR;
	std::vector<CReceipt>currentReceipts;
	eBlockFormationResult retval = eBlockFormationResult::initial;
	std::shared_ptr<CBlock> currentLeader = getBlockchainManager()->getLeader();
	std::shared_ptr<CBlock> currentKeyBlockLeader;
	std::vector<uint8_t> miningPerspective;//<- the perspective which we would attempt to set before commencing with a Flow.
										   // The perspective thus needs to be available locally.
										   // 'normally' this would imply setting the perspective as instructed by the parent block.
										   // However, there might be scenarios in which the parental perspective is overridden by a Checkpoint,
										   // which is expected to happen if the parental block (current Leader) was covered by one.

	CStateDomain * genesisDomain = NULL;
	std::vector<uint8_t> perspectiveID;
	std::vector<uint8_t> domainID;
	bool PoWSuccess = false;
	std::shared_ptr<CTools> tools = getTools();
	std::vector<uint8_t> mainTarget, minTarget;
	std::vector<std::shared_ptr<CTransaction>> transactionsToBeProcessed;
	std::vector<std::shared_ptr<CVerifiable>> verifiablesToBeProcessed;
	eBlockInstantiationResult::eBlockInstantiationResult bResult;
	//Things to be processed and so included inside of the Block for accountability purposes:
	/* All the transactions and verifiables are sorted by ERG-Bids; and processed till the max-block-size is reached.
	There's no reason to check for the minimal ERG-price. Transactions without high-enough ERG would not be processed at all by the current node.
	Still, at the same time, another node might choose to process these transactions and include them inside the Block.*/


	// ---------------------- CRITICAL ----------------------
	/*
			IMPORTANT: [ SECURITY ]: Epoch 1 and Epoch 2 rewards are to be already accounted for as part of rewardValidatedUpToNow
					   BEFORE this method is called.

					   [ SECURITY ]:  Utlimately, Key Block Operator rewards are issued based on the rewardValidatedUpToNow value.
									  It accounts for both Epoch rewards and TX processing fees.
	*/
	BigInt rewardValidatedUpToNow = 0;
	BigInt rewardIssuedToMinerAfterTAX = 0;
	// ---------------------- CRITICAL ----------------------

	eTransactionValidationResult previousRes = invalid;
	size_t maxBlockSize = CGlobalSecSettings::getMaxDataBlockSize();
	BigInt maxERGUsage = CGlobalSecSettings::getMaxERGPerBlock();
	size_t utilizedBlockSize = 0;
	BigInt utilizedERG = 0;
	BigInt thisBlockGNCFees = 0;
	BigInt coinbaseReward = 0;// 90 GNC


	size_t addedTransactionsCount = 0;

	BigInt blockReward = 0;
	BigInt pastEpochReward = 0;
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	CStateDomain* treasury = mStateDomainManager->findByID(CGlobalSecSettings::getTreasuryDomainID());	
	if (!treasury && bm->getKeyHeight())
	{
		error = true;
		tools->writeLine(tools->getColoredString("Won't form block Treasury not present.", eColor::cyborgBlood));
		return eBlockFormationResult::aborted;
	}
	//the below is used during the Hard-Fork Procedure
	std::vector<uint8_t> forcedMiningBlockID = (formKeyBlock && getForcedMiningBlockID().size() > 0) ? getForcedMiningBlockID() : forcedParentID;
	//thus the alternative key-path=test has higher priority over the Proof-of_Fraud test


	bool formGenesisKeyBlock = false;
	//std::shared_ptr<CBlock> recentKeyBlock;
	double targetMainDiff = 0;
	double targetMinDiff = 0;
	//LOCAL VARIABLES - END ------------------------------------------


	// Current Leader Check - BEGIN
	//if I'm to mine reg-blocks check if I'm the current leader first

	//COMMON SECTION - BEGIN
	if (forcedMiningBlockID.size() == 0)
		currentLeader = bm->getLeader();
	else
	{
		//used for HARD-FORK PROCEDURE:
		if ((currentLeader = bm->getBlockByHash(forcedMiningBlockID, bResult, true)) == nullptr)
		{
			error = true;
			tools->writeLine(tools->getColoredString("[HARD-FORK PROCEDURE]:", eColor::cyborgBlood)+" I could not set the 'forced' leader. Aborting.");
			return eBlockFormationResult::aborted;

		}

		forceOneTimeParentMiningBlockID();//clear it. It's one time only.
	}
	// Current Leader Check - END

	//Retrieve key-block for current leader 
	if (currentLeader != nullptr)
	{
		if (!currentLeader->getHeader()->isKeyBlock())
			currentKeyBlockLeader = bm->getKeyBlockForBlock(currentLeader);
		else
			currentKeyBlockLeader = currentLeader;
	}

	//as of now,the below has no effect, each leader can propose ERG price, others honor the value.
	//IMPORTANT: we could have this taken into account. As of now the leader has an ultimate say at what price-point other nodes are to process.
	//that might be opening up the doors to abuse. Still, allowing the a custom node-specific minimal cost could lead to a higher fragmentation of the network (entire blocks getting rejected).
	BigSInt minERGPrice = mSettings->getMinNodeERGPrice(currentLeader ? (currentLeader->getHeader()->getHeight() + 1) : 0);

	if (currentLeader != nullptr && !currentLeader->isGenesis() && currentKeyBlockLeader == nullptr)
	{
		 assertGN(false);
		return recentKeyBlockUnavailable;//this should not happen
	}
	//COMMON SECTION - END

	//REG-BLOCK SECTION - BEGIN
	if (!formKeyBlock)
	{
		if (currentLeader == nullptr)
		{
			error = true;
			return eBlockFormationResult::awaitingGenesisKeyBlock;
		}

	}
	//REG-BLOCK SECTION - END


	//KEY-BLOCK SECTION - BEGIN
	if (formKeyBlock)
	{
		//Do we need a Genesis Block?
		if (currentLeader == NULL)
			formGenesisKeyBlock = true;
	}
	//KEY-BLOCK SECTION - END

	if (formGenesisKeyBlock)
	{
		tools->writeLine("The blockchain is empty! I'll aim to generate the Genesis Key Block");
		tools->writeLine("Initiating VMMaintenance procedure (Overwatch' privileges overridden).");
		
		if (mScriptEngine->isInSandbox())
		{
			mScriptEngine->leaveSandbox();
		}
	
		tools->writeLine("Forming the Genesis Rewards Block..");
		tools->writeLine("WARNING: During the Genesis Block formation 'money' will be generated out of the blue!");
		tools->writeLine("During this stage, initial ICO investors etc. will be rewarded.");
		tools->writeLine("Preparing to generate the initial State Domains..");
		std::string factFileContent;
		if (bm->getSolidStorage()->readStringFromFile(factFileContent, CGlobalSecSettings::getGenesisFactFileName()) && factFileContent.size())
			tools->writeLine("Genesis Facts-File found (" + CGlobalSecSettings::getGenesisFactFileName() + ") ! Loading..");
		else
		{
			tools->writeLine("Genesis Facts-File missing ("+ bm->getSolidStorage()->getMainDataDir()+"\\"+ CGlobalSecSettings::getGenesisFactFileName() + "),- unable to generate the Genesis Block!");
			bm->exit(true);
		}

		tools->writeLine("Genesis Rewards' fingerprint: " + tools->base58CheckEncode(mCryptoFactory->getSHA2_256Vec(tools->stringToBytes(factFileContent))));
		std::vector<uint8_t> hash, actualHash;
		bool invalidBase58Data = false;
		if (!tools->base58CheckDecode(CGlobalSecSettings::getBase58GenesisFactFileHash(), hash))
		{
			invalidBase58Data = true;
		}

		actualHash = bm->getCryptoFactory()->getSHA2_256Vec(tools->stringToBytes(factFileContent));
		if (invalidBase58Data || !tools->compareByteVectors(hash, actualHash))
		{
			bm->getSettings()->setIsGlobalAutoConfigInProgress(false);
			bm->getSettings()->setLoadPreviousConfiguration(true);
			tools->askYesNo(tools->getColoredString("Invalid hash "+ std::string((invalidBase58Data?" within *SOURCE*":""))+" of the Genesis facts - file!\n",eColor::cyborgBlood) + CGlobalSecSettings::getBase58GenesisFactFileHash() + " - "+ tools->getColoredString("EXPECTED", eColor::orange)+"\n" +
				tools->base58CheckEncode(actualHash) + " - "+tools->getColoredString("found instead.",eColor::blue)+"\n Press Enter to shut - down.", true);
			bm->exit();
			error = true;
			return eBlockFormationResult::aborted;
		}
		else
			tools->writeLine("The integrity of the Genesis Facts File verified.");
		//generate the Genesis State-Domain
		std::shared_ptr<CVerifiable> genesisVerifiable = std::make_shared<CVerifiable>(eVerifiableType::GenesisRewards);
		genesisVerifiable->setProof(tools->stringToBytes(factFileContent));
		verifiablesToBeProcessed.push_back(genesisVerifiable);
	    
		
	}
	else
	{
		//Pre-Flight - BEGIN
		std::vector<std::shared_ptr<CBCheckpoint>>  checkpoints = bm->getCheckpoints();
		uint64_t heightProposal = currentLeader->getHeader()->getHeight() + 1;
		if (checkpoints.size())
		{
			for (uint64_t i = checkpoints.size() - 1; i > 0; i--)
			{
				if (checkpoints[i]->getIsActive() && checkpoints[i]->getFlags().obligatory && checkpoints[i]->getHeight() >= heightProposal)
				{
					error = true;
					tools->logEvent("Can't generate block at height secured by an Obligatory Checkpoint (" + std::to_string(heightProposal)+")", "Mining", eLogEntryCategory::localSystem, 8,eLogEntryType::warning);
					return eBlockFormationResult::aborted;
				}

			}
		}

		//Pre-Flight - END

		if (!getInformedWaitingForTransactions())
		{
			//Normal operation/routine
			tools->logEvent("Aiming to generate a " + std::string((formKeyBlock ? "key" : "regular")) + " block at height: " + std::to_string(heightProposal), "Mining", eLogEntryCategory::localSystem, 5);
		
			setInformedWaitingForTransactions(true);
		}
	}

	if (!formKeyBlock)
	{
		transactionsToBeProcessed = getUnprocessedTransactions(feesHighestFirstNonce);
		verifiablesToBeProcessed = getUnprocessedVerifiables(feeHighestFirst);//gets also the proof-of-fraud verifiables

		// ============================================================================
		// Phantom Mode: Filter out already phantom-processed transactions
		// ============================================================================
		// In phantom mode, transactions remain in mem-pool but we track which ones
		// have already been processed in phantom blocks. Filter them out here to
		// avoid infinite reprocessing of the same transactions.
		// ============================================================================
		if (phantomMode && transactionsToBeProcessed.size() > 0)
		{
			size_t originalCount = transactionsToBeProcessed.size();
			std::vector<std::shared_ptr<CTransaction>> filteredTransactions;
			filteredTransactions.reserve(transactionsToBeProcessed.size());

			for (auto& tx : transactionsToBeProcessed)
			{
				if (!isPhantomProcessed(tx->getHash()))
				{
					filteredTransactions.push_back(std::move(tx));
				}
			}

			transactionsToBeProcessed = std::move(filteredTransactions);

			if (originalCount != transactionsToBeProcessed.size())
			{
				tools->writeLine(tools->getColoredString(
					"[PHANTOM MODE] Filtered out " + std::to_string(originalCount - transactionsToBeProcessed.size()) +
					" already phantom-processed TX(s). " + std::to_string(transactionsToBeProcessed.size()) + " remaining.",
					eColor::greyWhiteBox));
			}
		}
		// Phantom Mode Filter - END

		if (transactionsToBeProcessed.size() == 0 && verifiablesToBeProcessed.size() == 0)
		{
			error = true;
			return eBlockFormationResult::notEnoughTransactions;
		}
	}

	setInformedWaitingForTransactions(false);

	//check if all good; detect an idle initial status
	if (!formGenesisKeyBlock && currentLeader == nullptr)
	{
		tools->writeLine("Critical error: The Genesis Blocks were NOT accepted during validation.");
		bm->exit(true);
		//this should not happen; the block with GenesisDomain should have been accepted locally during last round.
	}

	//ensure parent is available in cold-storage
	if (currentLeader != nullptr && bm->getSolidStorage()->getBlockByHash(currentLeader->getID(), bResult, false) == 0)
	{
		error = true;
		return eBlockFormationResult::leaderNotAvailableInCS;
	}


	//instantiate a stub block-instance; fill it below.
	std::shared_ptr<CBlock> proposal = CBlock::newBlock(currentLeader, bResult, getBlockchainMode(), formKeyBlock, true); // <- latest Core version would be imposed upon block
	proposal->setProducedLocally(true);

	std::vector<uint8_t> pubKey = chain.getPubKey();

	if (pubKey.size() != 32)
	{
		error = true;
		tools->writeLine(tools->getColoredString("Can't form data block - an unknown active identity.", eColor::lightPink));
		return eBlockFormationResult::aborted;
	}
	proposal->getHeader()->setPubSig(pubKey);
	tools->writeLine("Operating Identity:" + tools->getColoredString(tools->bytesToString(proposal->getHeader()->getMinersID()), eColor::lightCyan));
	
	//NOTE: in case of a key-block, the parent might be updated after PoW, to account for new blocks currentLeader might have released by then.

	if (currentKeyBlockLeader != nullptr)
	{
		// IMPORTANT Key Height Adjustment - BEGIN
		if (formKeyBlock)
		{
		
			assertGN(proposal->getHeader()->setParentKeyBlockID(currentKeyBlockLeader->getID()));//the field can be set only for a key-block
		}
		// IMPORTANT Key Height Adjustment - END

		
	}
	//what's the valid basic reward at that height? (judging by the parent) - REGULAR BLOCKS DO NOT YIELD ANY ADDITIONAL BASIC rewards
	//there shall be NO INCENTIVE for leader to add artificial empty regular blocks. They do NOT secure the chain.

	if (formKeyBlock)
		coinbaseReward = bm->getCoinbaseRewardForKeyHeight(proposal->getHeader()->getKeyHeight());
	else
		coinbaseReward = 0;

	mFieldsGuardian.lock();

	std::vector<uint8_t> forcedPerspective = getForcedMiningPerspective();

	if (!mWasForcedPerspectiveProcessed && forcedPerspective.size() > 0)
	{//forced mining Perspective
		miningPerspective = forcedPerspective;
		mWasForcedPerspectiveProcessed = true;
		forceMiningPerspectiveOfNextBlock();
	}
	else
	{//normal current perspective

		if (currentLeader != nullptr)
			miningPerspective = currentLeader->getHeader()->getPerspective(eTrieID::state);
		else
			miningPerspective = bm->getPerspective();
	}
	//uint64_t currentB = mStateDomainManager->analyzeTotalDomainBalances();
	mFieldsGuardian.unlock();


	tools->writeLine("An attempt to form a block is commencing..");
	uint64_t nowPoW = std::time(0);// tools->getTime();// we check the current time
	
	if (currentLeader && !bm->getIsOperatorLeadingAHardFork())
	{
		if (nowPoW < currentLeader->getHeader()->getSolvedAtTime())
		{
			error = true;
			tools->logEvent("Daddy should always know the right time..fix your clock.", "Security", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
			return eBlockFormationResult::aborted;
		}
	}
	//[Extremely Important]:
	/* the timestamp present within the block needs to be exactly the same as the timestamp used during required
	* difficulty calculation. Otherwise we would risk the block being rejected due to a time-skew between the two.
	*/
	proposal->getHeader()->setSolvedAtTime(nowPoW);//needs to be there for Data Blocks as well!
	//for a key-block the perspective would not be affected i.e. it would remain the same as from the view-point of the current Leader

	//Important: PoW needs to be done outside of the Flow, it takes a lot of time. 
	//otherwise flow-related mutexes would be blocked for too long.
	

	// Blockchain Heights - BEGIN
	if (currentKeyBlockLeader)
	{
		uint64_t currentKeyBlockHeight = currentKeyBlockLeader->getHeader()->getKeyHeight();

		if (formKeyBlock)
		{
			// increment by one compared to current key height
			proposal->getHeader()->setKeyHeight(++currentKeyBlockHeight);// this CANNOT change after PoW is computed
			//				[ IMPORANT ]:		 ^---- notice Prefix notation (value incremented before passed into method).	
			// on the contrary, regular block height CAN.
		}
		else
		{
			proposal->getHeader()->setKeyHeight(currentKeyBlockHeight);
		}
	}
	// Blockchain Heights - END

	//KEY-BLOCK DATA  - BEGIN
	//Proof-of-Work Section - BEGIN

	if (formKeyBlock)
	{
		
		proposal->getHeader()->setPubSig(pubKey);
		
		if (mDifficultyMultiplier != 1)
			tools->writeLine("Multiplying the expected difficulty by a factor of " + std::to_string(mDifficultyMultiplier));
		bool ok = false;
		//we extend the current chain so we might as well use optimization and so we do

		//IMPORTANT: do NOT use optimization when calculating difficulty for new blocks.
		targetMainDiff = (bm->getMinDIfficultyForBlock(currentLeader, nowPoW,false,false, ok) + 0.1)*mDifficultyMultiplier;//difficulty multiplier deprecated for testing only.
		
		if (targetMainDiff > 1000)
		{
			targetMinDiff = min(1000, targetMainDiff / 1000);
		}
		
		if (targetMainDiff > 1000000000)
		{
			tools->logEvent("You wouldn't be able to mine that block. Difficulty too high. Check your system clock.","Mining",eLogEntryCategory::localSystem, 100, eLogEntryType::failure, eColor::lightPink);
		}
		else
		{
			tools->logEvent("Preparing mining task at Target Difficulty: " + tools->getColoredString(std::to_string(targetMainDiff), eColor::lightCyan)+
				" Min Difficulty: " + tools->getColoredString(std::to_string(targetMinDiff) , eColor::lightCyan),"Mining",eLogEntryCategory::localSystem,10);
		}
		if (!ok)
		{
			error = true;
			tools->logEvent("Unable to compute difficulty. Aborting block formation", "Mining", eLogEntryCategory::localSystem, 100, eLogEntryType::failure, eColor::cyborgBlood);
			return eBlockFormationResult::PoWFailed;
		}
	

		mainTarget = tools->diff2target(targetMainDiff);
		minTarget = tools->diff2target(targetMinDiff);

		if (!getWasForcedPerspectiveProcessed())
		{
			setWasForcedPerspectiveProcessed();
			tools->writeLine("I've done a one-time difficulty multiplication as instructed.(*" + std::to_string(mDifficultyMultiplier) + ")");
			setDiffMultiplier(1);
		}
		proposal->getHeader()->setPackedTarget(tools->target2packedTarget(mainTarget));


		//EXPLANATION:
		/// We first do PoW, then sign the header. Thanks to this we can start digging on the latest key-block leader right away
		/// and still be accepting and verifying data-blocks flowing in. AFTER we find a solution to the crypto-puzzle we 
		/// compute our reward and  include the final Perspective (exact fingerprint of the  decentralized state machine's state).
		/// We do signatures based solely on block headers since otherwise this would not allow for efficient chain-proofs ( more data would be required and we want chain-proofs to be autonomous)
		/// 

		std::array<unsigned char, 32> workTargetMainArray, workTargetMinArray;
		std::memcpy(&workTargetMainArray[0], &mainTarget[0], 32);
		std::memcpy(&workTargetMinArray[0], &minTarget[0], 32);

		// Proof-of-Work ( Part 1 ) Work Scheduling - BEGIN
		std::shared_ptr<CWorkUltimium> work = std::make_shared<CWorkUltimium>(nullptr, getBlockchainMode(), workTargetMainArray, workTargetMinArray, 0x00000000, 0xffffffff);

		// Include the miner's public key into the block.
		// Sign the block already; the block will be PoW'ed later on. PoW prevents altering the miner's public key.
		// There's no external trusted chain of trust; thus PoW is the only thing ensuring data cannot be tempered with easily.
		std::vector<uint8_t> dataToWorkOn;

		proposal->getHeader()->setNonce(0); //just to make sure

		
		
		//We do PoW solely based on the below information. IF any other node includes a Proof-of-Fraud (block signed using same public-key for the same block height)
		//the corresponding leader looses all its rewards for the key-block and child data-blocks generated by it. The node who detected the fraud receives the entire value of the
		//fraudulent node's Identity Token. (NOTE: we cannot generate money out of thin air here since that would open a door for another type of misuse where cheaters detect frauds if their
		//own and receive reward anyway)
		//Yes, it might be better to be a fraud-detector than 'miner' if frauds do happen.

		// Proof-pf-Work input data - BEGIN
		// these fields cannot change after PoW is completed.
		CDataConcatenator concat;// data based on WHICH to generate PoW
		std::vector<uint8_t> parentKeyBlockID = proposal->getHeader()->getParentKeyBlockID();

		if (!parentKeyBlockID.empty())
		{// thus,
		//	- Parent Key-Block it CANNOT change during proof of work computation. 
		//  - Data block CAN change. It can be set AFTER PoW is computed.
			concat.add(parentKeyBlockID); //i.e. hash of the entire parental key-block ID
		}

		concat.add(proposal->getHeader()->getPubKey());
		concat.add(proposal->getHeader()->getKeyHeight());// thus key-height it cannot change. Blockchain Height - it CAN (as we are waiting for new data blocks flowing in).
		// Proof-pf-Work input data - END
		concat.add(proposal->getHeader()->getSolvedAtTime());
		work->setDataToWorkOn(concat.getData());
		work->markDataAsHeader();

		setCurrentMiningTaskID(work->getGUID());//register GUID so that the task can be stopped later on.

		mWorkManager->registerTask(work);

		// Proof-of-Work ( Part 1 ) Work Scheduling - END

		//check if attempting to lead a fork - BEGIN
		std::shared_ptr<CBlockHeader> hl = bm->getHeaviestChainProofLeader();

		if (hl)
		{
			if (proposal->getHeader()->getHeight() < hl->getHeight())
			{
				//we are producing a new key-block at a height which is lower than the network-proclaimed heaviest history of events.
				//the global chain died out, there are invalid blocks produced by a powerful attacker etc.
				bm->setIsForkingAlternativeHistory(true);
			}
			else
			{
				bm->setIsForkingAlternativeHistory(false);
			}
		}
		//check if attempting to lead a fork - END


		//Hard-Fork Support - BEGIN
		if (bm->getIsOperatorLeadingAHardFork() && (currentLeader->getIsCheckpointed() || bm->isCheckpointed(currentLeader->getHeader())) && (proposal->getIsCheckpointed() == false))//Notice: a checkpoint MUST be included in-code a priori for this to be effective.
		{
			tools->writeLine("Marking Block as a Hard-Fork Leader..", true, false, eViewState::GridScriptConsole, "Hard Fork");
			proposal->setIsLeadingAHardFork();//Notice: Operator must indicate to be willing to lead this hard fork as well.
		}

		if (bm->getIsOperatorLeadingAHardFork() && !proposal->getIsLeadingAHardFork())
		{
			error = true;
			return eBlockFormationResult::aborted;
		}
		
		if (!(bm->getIsOperatorLeadingAHardFork() && formKeyBlock))// IMPORTANT: omit PoW for hard-fork leading key block
		{//Hard-Fork Support - END

			// Proof-of-Work ( Part 2) Results Retrieval - BEGIN

			/*
			* [ SECURITY ]: normaly we would (and should) first set all fields of the block, sign the block and only at the very end - perform Proof-of-Work
			*				on the resulting serialized Block Header data structure.
			*				Thing is, we need to account (in the Key-Block header) for the latest data block. In other words, the Proof-of-Work is being computed
			*				while we are continuesly accepting for new data blocks to arrive. Only once the proof of work is completed do we fetch the latest
			*				Perspective from the Global LIVE State Merkle Patricia Trie and proceed with a Flow (ACID) processing any pending changes to be invoked
			*				by the Key Block being formed right over here.
			* 
			*				[ Notice ]: the Final Perspective from latest known Data/Key Block is assumed as Initial Perspective in the Key Block being formed.
			*/

			PoWSuccess = powLoop(proposal);

			bool testPoW = mCryptoFactory->verifyNonce(*proposal->getHeader());

			if (!PoWSuccess || !testPoW)
			{
				error = true;
				tools->writeLine("Proof-of-Work Loop Failed.");
				return eBlockFormationResult::PoWFailed;
			}

			//Proof-of-Work - END
		}
		else
		{
			proposal->getHeader()->setPackedTarget(0);
			proposal->getHeader()->setNonce(0);
		}
		retval = eBlockFormationResult::blockFormed;//looks like all Good in terms of a Key-Block
	}

	//Proof-of-Work  ( Part 2)  - END
	//KEY-BLOCK DATA  - END

	//UPDATE LATEST PARENT DATA-BLOCK FOR KEY-BLOCK - BEGIN

		//get latest data - block from previous key-leader
		//update key-block's parent  - for it might have changed - to the latest one known produced by previous leader (look ONLY within mVerifiedChainProof - only THESE blocks were verified transactions processed etc.)
	if (formKeyBlock && currentKeyBlockLeader != nullptr)
	{
		// To Do: notice that Parent Key Block cannot change during Key Block formation process.
		//		  Rationale: Proof-of-Work is based upon it.
		//		  Idea: in future implementations we might allow for the Proof of Work input to be based on a different (deeper in the chain reference point).
		//				This way we could keep updating Parent Key block while the proof of work is computed.
		// With a 10 minute expected key block interval this is not an issue though. Might be an issue only during post Hard-Fork periods when the key block
		// interval drops radically.

		// Post PoW Block Modifications Part 1
		// [ SECURITY ] : Notice - everything down below happens AFTER Proof-of-Work was computed on the block being formed.
		//				  Anything we change down below - it CANNOT affect Proof-of-Work input data
		std::shared_ptr<CBlock> latestKeyLeadersDataOffspring = bm->getLatestDataBlockForKeyLeader(currentKeyBlockLeader->getHeader()->getPubKey());
		//																// ^- only Blockchain Height can change. Parent Key block needs to remain the same.
		//	[ RATIONALE]:	 - we need to account for latest Data Block. We've kept mining while data blocks were flowing in (and being processed).
		//	[ Game Theory ]: - since we receive 60% of fees from every data-block produced by previous leader - it's best to proclaim the latest data-block as parent.
		//  [ Cuncurrency ]: - notice that it is of paremount important to check for current data block with getLatestDataBlockForKeyLeader() instead of calling 
		//					   getLeader() - that is because - both Key Block and following data blocks MIGHT have changed while we were mining.
	

		if (latestKeyLeadersDataOffspring != nullptr)
		{//RECALL: only at this stage do we know full-rewards and perspective only at this stage do we know all the transactions which we confirm from previous leader
			proposal->getHeader()->setParent(latestKeyLeadersDataOffspring); // Notice: NOT setKeyParent()
			//proposal->getHeader()->setKeyHeight(latestKeyLeadersDataOffspring->getHeader()->getKeyHeight() + 1);
			proposal->getHeader()->setHeight(latestKeyLeadersDataOffspring->getHeader()->getHeight() + 1); // Notice: NOT setKeyHeiht()
			miningPerspective = latestKeyLeadersDataOffspring->getHeader()->getPerspective(eTrieID::state);//also the initial Perspective might have changed
		}
	}
	//UPDATE LATEST PARENT DATA-BLOCK FOR KEY-BLOCK - END

	//Checkpoints' Support - BEGIN
	//notice: if the parent block was covered by a checkpoint, its resulting perspective is EXPECTED to differ from the one which is indicated
	//        within of its header. In such a case, we need to fetch the 'Effective Perspective' from the local cache.
	if (currentLeader)
	{
		std::vector<uint8_t> effectivePerspective = bm->getEffectivePerspective(currentLeader->getID());
		
		//v- Warning: the below is under the assumption that we store 'Effective Perspectives' ONLY for blocks that have been covered by a checkpoint.
		if (effectivePerspective.size())
		{
			miningPerspective = effectivePerspective;
		}
	}

	//Checkpoints' Support - END

	//				*	*	* START_THE_FLOW	*	*	*
	assertGN(startFlow(miningPerspective));

	// System Final Perspective (Part 2) - BEGIN
	// notice: there is also Final Perspective (in Part 2)
	assertGN(proposal->getHeader()->setPerspective(miningPerspective, eTrieID::state, false));
	// System Final Perspective (Part 2) - END

	debugInitial = miningPerspective;
	//IMPORTANT: transactions, verifiables will be processed externally here; within the function.
	/*so that we know the total ERG used and are able to generate Reward Verifiables
	then this will commence together during EndFlow when verified by BlockchainManager; here we will just abort*/

	// REGULAR-BLOCK RELATED DATA  BEGIN
	std::vector<uint8_t> initialPerspective;
	if (!formKeyBlock)
	{
		//				*	*	* PROCESS TRANSACTIONS	*	*	*

		// Initial Block State - BEGIN
		initialPerspective = getPerspective(true);
		// Initial Block State - END

		// Transaction Processing Logic - BEGIN
		if (transactionsToBeProcessed.size() == 0)
		{
			tools->writeLine("No transactions to process.");
		}
		else
		{
			// Stack Depth Validation - BEGIN
			// Ensure clean starting state - no lingering transaction states
			// Total Stack must be empty before starting block processing
			assertGN(getInFlowTXDBStackDepth(true) == 0);
			// Stack Depth Validation - END

			// Process Block Transactions - BEGIN
			// Note: In phantom mode, markAsProcessed is false to retain transactions in mem-pool
			processTransactions(transactionsToBeProcessed, currentReceipts,
				!phantomMode,   // markAsProcessed - false in phantom mode to keep txs in mem-pool
				proposal,
				true,   // generateReceipts
				false,  // disableConsistencyCheck
				false); // useMockSignatures
			// Process Block Transactions - END

			// Post-Processing Validation - BEGIN
			// Verify stack is clean after processing
			// All transaction states must be either cleaned up (successful) 
			// or reverted (failed), leaving total stack empty
			assertGN(getInFlowTXDBStackDepth(true) == 0);

			// Verify Transaction Accounting
			volatile uint64_t cleanedTXs = getTXsCleanedCount();    // Successfully processed
			volatile uint64_t revertedTXs = getTXsRevertedCount();  // Failed and reverted
			// Total of cleaned and reverted must match the number of transactions scheduled for processing
			assertGN(cleanedTXs + revertedTXs == transactionsToBeProcessed.size());
			// Post-Processing Validation - END
		}
		// Transaction Processing Logic - END

		//				*	*	* PROCESS UNCLE BLOCKS	*	*	*
	}
	std::vector<uint8_t> perspectiveAfterTransactions = getPerspective(true);

	debugAfterT = perspectiveAfterTransactions;
	// REGULAR-BLOCK RELATED DATA - END
	//NOTE: in the new architecture the transaction reward is shared between the current and consecutive leader.

	processUncleBlocks(bm->getRecentUncles());

	BigInt validatedFees = 0; // involve somebody getting charged (although can be seen as consensus reward from Operator's perspective)

	for (int i = 0; i < currentReceipts.size(); i++)
	{
		validatedFees += currentReceipts[i].getERGPrice() * currentReceipts[i].getERGUsed();
	}

	if (validatedFees != mTotalGNCFees)
	{
		assertGN(false);
	}

	// FORMATION: Actuate Total Rewards #1 
	rewardValidatedUpToNow += validatedFees;

	//				*	*	* PROCESS VERIFIABLES	- WITHOUT MINER's REWARD*	*	*
	// we do not process leader's reward just yet, since other verifiables may yield additional rewards  - (within consensusReward var.)
	
	// Notice: the below method WOULD (processVerifiables) assign PREVIOUS EPOCH rewards ALREADY had the Miner's Rewards Verifiable
	// been injected into the block  already - which is NOT the case, yet.
	// Note: In phantom mode, markAsProcessed is false to retain verifiables in mem-pool
	processVerifiables(verifiablesToBeProcessed,
		currentReceipts,				// appends receipts resulting from Verifiables' processing.
		rewardValidatedUpToNow,			// validates (in case of miner's rewards and possiblity adjusts rewards - in evry other case).
		rewardIssuedToMinerAfterTAX,	// reward issued to miner after Tax (the actual amount issued during call to changeDomainBalance())
		!phantomMode,					// mark verifiables as processed - false in phantom mode
		proposal,						// block proposal
		true,							// appends pending valid verifiables to block proposal.
		false							// do not aim to verify pending verifiables against block proposal (being formed).
	);


	std::vector<uint8_t> perspectiveAfterVerifiables = getPerspective(true);
	debugAfterV = perspectiveAfterVerifiables;

	if (!formKeyBlock && currentReceipts.size() == 0)
	{
		error = true;
		tools->writeLine("During block formation it turned out - all transaction and verifiable candidates were not 'processable'.");
		abortFlow();
		return eBlockFormationResult::notEnoughTransactions;
	}

	// Account for Fees - BEGIN
	// [ SECURITY ]: local computation [ TRUSTED ]
	for (int i = 0; i < currentReceipts.size(); i++)
	{
		bool found = false;
		if (currentReceipts[i].getReceiptType() == eReceiptType::eReceiptType::transaction)
		{
			for (int y = 0; y < transactionsToBeProcessed.size(); y++)
			{
				if (tools->compareByteVectors(currentReceipts[i].getVerifiableID(), transactionsToBeProcessed[y]->getHash()))
				{
					thisBlockGNCFees += (currentReceipts[i].getERGUsed()) * transactionsToBeProcessed[y]->getErgPrice();
					utilizedERG += currentReceipts[i].getERGUsed();
					found = true;
					break;
				}
			}

		}
		else if (currentReceipts[i].getReceiptType() == eReceiptType::eReceiptType::verifiable)
		{
			for (int y = 0; y < verifiablesToBeProcessed.size(); y++)
			{
				if (tools->compareByteVectors(currentReceipts[i].getVerifiableID(), verifiablesToBeProcessed[y]->getHash()))
				{
					thisBlockGNCFees += (currentReceipts[i].getERGUsed()) * verifiablesToBeProcessed[y]->getErgPrice();
					utilizedERG += currentReceipts[i].getERGUsed();
					found = true;
					break;
				}
			}
		}
		assertGN(found);
	}


		BigInt totalFeesAsPerReceipts = 0;
		for (int i = 0; i < currentReceipts.size(); i++)
		{
			totalFeesAsPerReceipts += currentReceipts[i].getERGPrice()*currentReceipts[i].getERGUsed();
		}
		// Account for Fees - END

		 assertGN(thisBlockGNCFees == totalFeesAsPerReceipts);

		 
		 // Epochs' Processing - BEGIN
		 // [ SECURITY ]: here we provision final current operator's rewards.
		 BigInt epoch1Reward = 0;
		 BigInt epoch2Reward = 0;

		 // Epoch 1 - BEGIN
		 // Solely Current Block Candidate
		 if (!bm->getEpochReward(proposal, epoch1Reward, rewardValidatedUpToNow, eEpochRewardCalculationMode::ALeader))
		 {
			 error = true;
			 return eBlockFormationResult::cantCalculatePastEpochReward;
		 }

		 rewardValidatedUpToNow += epoch1Reward;

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
		 debugLog << " [- Epoch 1 Reward -]: " << tools->formatGNCValue(epoch1Reward) << "\n";
		 debugLog << " [ Total Cumulative Reward Stage #1 ]: " << tools->formatGNCValue(rewardValidatedUpToNow) << "\n";
#endif

		 // Epoch 1 - END

		 // Epoch 2 - BEGIN
		 // Parent Key Block -> Data Block(s) -> Current Block Candidate
		 if (!bm->getEpochReward(proposal, epoch2Reward, rewardValidatedUpToNow, eEpochRewardCalculationMode::BLeader))
		 {
			 return eBlockFormationResult::cantCalculatePastEpochReward;
		 }

		 rewardValidatedUpToNow += epoch2Reward;

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
		 debugLog << " [- Epoch 2 Reward -]: " << tools->formatGNCValue(epoch2Reward) << "\n";
		 debugLog << " [ Total Cumulative Reward Stage #2 ]: " << tools->formatGNCValue(rewardValidatedUpToNow) << "\n";
#endif
		 // Epochs' 2 - END
		 // Epoch Processing - BEGIN	


		//				*	*	* REWARD MINER	*	*	*  - BEGIN
		//The miner's reward will be generated right here and then verified during processing.
		
		// - current miner is allowed to claim ONLY 40% of rewardValidatedUpToNow
		// - whereas the consecutive leader is allowed to claim 60% of everything (to be validated, the attacks from bitcoin-ng 
		//   are NOT relevant here since mining takes place only on key-blocks) from past data-blocks , through a Verifiable.
		

			// Tests - BEGIN
			    std::string epoch1RewardStr = epoch1Reward.str();
				std::string epoch2RewardStr = epoch2Reward.str();
			// Tests - END
			

			// [ IMPORTANT ]: we do not actually NEED this verifiable anyway, since we CANNOT trust the data contained within it.
			//           It's present for improved accountability and to trigger rewards issuance process during verification.
			//Its content is verified, also the number of its occurrences within a block.
			std::shared_ptr<CVerifiable> leaderReward = std::make_shared<CVerifiable>(eVerifiableType::eVerifiableType::minerReward);
		    assertGN(leaderReward->addBalanceChange(proposal->getHeader()->getMinersID(), rewardValidatedUpToNow)); // used during verification of miner's rewards
			//						^- this is actually serialized into the Verifiable.
			//						Other nodes would verify whether the value with matches the expected value.
			//we want to store the entire reward the leader is to receive within the very Verifiable
			//the Verifiable's Engine will just verify if myReward ==feesValidatedUpToNow+
			std::vector<uint8_t> perspectiveAfterLeaderReward;// = getPerspective(true);
			verifiablesToBeProcessed.clear();
			verifiablesToBeProcessed.push_back(leaderReward);


			//Calculate Tax - BEGIN 
			BigInt tax = 0;
			double taxRate = CGlobalSecSettings::getMiningTax(proposal->getHeader()->getKeyHeight());
			if (rewardValidatedUpToNow > 101) // avoid issues with 1%-100% rounding
			{
				if (taxRate >= 0.0 && taxRate <= 1.0) {
					long long taxFrac = static_cast<long long>(std::llround(taxRate * 1000000.0));
					BigInt bigFrac(taxFrac);
					BigInt bigDen(1000000);

					// Cast pendingBalanceChange to BigInt for calculation
					BigInt pendingBalanceBI = static_cast<BigInt>(rewardValidatedUpToNow);
					tax = (pendingBalanceBI * bigFrac) / bigDen;
				}
			}
			rewardIssuedToMinerAfterTAX = static_cast<BigInt>(rewardValidatedUpToNow) - tax;
			//Calculate Tax - END


			//				*	*	* REWARD MINER	*	*	*  - END


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			debugLog << " [ Total Block Reward ]: " << tools->formatGNCValue(rewardValidatedUpToNow) << "\n";
			debugLog << " [ Reward Issued to Operator ]: " << tools->formatGNCValue(rewardIssuedToMinerAfterTAX) << "\n";
#endif

			// -------  CRITICAL ------- BEGIN
			// [ SECURITY ]:		 v---computed locally, verified by others.
			proposal->getHeader()->setTotalBlockReward(rewardValidatedUpToNow);// Epoch 1 Reward + Epoch 2 Reward (including TAX).
			// the value is re-computed by each node and verified to match.
			// Hard Forks: notice the concept of Effective Block Rewards associated with headers (not serialized).
// -------  CRITICAL ------- END
// [ SECURITY ]:		 v---computed locally, verified by others.
			proposal->getHeader()->setPaidToMiner(rewardIssuedToMinerAfterTAX); // Epoch 1 Reward + Epoch 2 Reward, excluding tax. Value which actually affected the Operator's state domain.
			// Epoch 1 is equal to Coinbase Reward if Genesis Block.
			// Hard Forks: notice the concept of Effective Block Rewards associated with headers (not serialized).


			/* We're telling the Verifiable's engine: "hey; Look; that's the TOTAL amount of fees I've been able to validate up to now within this very block
			now:"
			1) verify if my calculation of coinbase reward is correct
			2) verify if my cut from everything  is correct
			3) verify if my calculations of previous epoch reward are correct

			The same process would take place during block's VALIDATION and during endFlow() when data is committed.
			*/ 
			
			if (!processVerifiables(verifiablesToBeProcessed,
				currentReceipts,			// so that processing of Verifiables can result in Receipts
				// FORMATION: Actuate Total Rewards #2(a)
				rewardValidatedUpToNow,		// keep building up total reward
				rewardIssuedToMinerAfterTAX,// this will be used later down below to set after-tax reward field in block header
				false,						// do not mark Verifiabels as processed (yet)
				proposal,					// block proposal being formed
				true,						// keep building up block
				false)						// do not verify Verifiabels against block (the block is being formed)
				)
			{
				error = true;
				return eBlockFormationResult::aborted;
			}

			if (proposal->getVerifiablesCount() == 0)
			{
				error = true;
				// at least Operator's Reward Verifiable is required
				return eBlockFormationResult::aborted;
			}
			
			// Tests - BEGIN
			std::string totalRewardVerfiedUptilNowStr = rewardValidatedUpToNow.str();
			// Tests - END


			// consensus reward (which is built up iteratively during block formation) needs to match
			// expected block reward for both epochs.
			
			//totalBlockReward = rewards from transactions + rewards from any verifiables + coinbasereward
			//it does NOT include reward from previous epoch.
			
			//the reward within the Verifiable DOES include the reward from previous epoch.

		//				*	*	* PROCESS RECEIPTS	*	*	* (update: just add them to the block, there's no processing involved during block formation)
		for (int i = 0; i < currentReceipts.size(); i++)
		{
			 assertGN(proposal->addReceipt(currentReceipts[i]));
			 currentReceipts[i].HotProperties.markAsProcessed();
		}
		//				*	*	* END_THE_FLOW	*	*	*

		std::vector<uint8_t>  finalPerspective = mFlowStateDB->getPerspective();
		debugFinal = finalPerspective;
		abortFlow();

		// Post PoW Block Mof Part 2
		// [ SECURITY ] : Notice - everything down below happens AFTER Proof-of-Work was computed on the block being formed.
		//				  Anything we change down below - it CANNOT affect Proof-of-Work input data.
	
		//[ NOTE ]: the perspective reflects ALL changes from HotStorage, but not
		//	Cold Storage,at this stage.  Here were arejust creating a block PROPOSAL which has a change to affect the Decentralized State Machine once accepted.
		//	It is then up to the Blockchain Manager to account for it.

		//here, set the final Perspective within the Block Proposal.

		// System Final Perspective (Part 2) - BEGIN
		// notice: there is also System Initial Perspective ( Part 1)
		  assertGN(proposal->getHeader()->setPerspective(finalPerspective, eTrieID::state)); // [ SECURITY ]: computed locally and verified by others
		//										^--- Final Perspective, there is also the Initial Perspective (before Flow began).
	    // System Final Perspective (Part 2) - END

		proposal->getHeader()->setNrOfTransactions(proposal->getTransactionsIDs().size()); // [ SECURITY ]: computed locally and verified by others
		proposal->getHeader()->setNrOfVerifiables(proposal->getVerifiablesIDs().size());   // [ SECURITY ]: computed locally and verified by others

		// [ SECURITY ]:		 v---computed locally, verified by others.
		proposal->getHeader()->setErgUsed(utilizedERG); 

	
		// -------------------- ECC AUTHENTICATION  - BEGIN 
		//				*	*	*	 SIGN THE BLOCK HEADER		*	*	*
		if (!mCryptoFactory->signBlock(proposal, chain.getPrivKey()))
		{
			error = true;
			tools->writeLine("I could not sign the block");
			return aborted;
		}
		// -------------------- ECC AUTHENTICATION  - END
		
		// -------------------- ECC AUTHENTICATION  (VERIFICATION) - BEGIN 
		bool sigValid = mCryptoFactory->verifyBlockSignature(proposal, formKeyBlock ? std::vector<uint8_t>() : currentKeyBlockLeader->getHeader()->getPubKey());
		
		if (!sigValid)
		{
			error = true;
			tools->writeLine("Invalid key-leader signature in local block proposal.");
			retval = eBlockFormationResult::aborted;
		}
		// -------------------- ECC AUTHENTICATION  (VERIFICATION) - END
		
		// Checks - BEGIN
		if (!formKeyBlock)
		{
			if (proposal->getTransactionsCount() ==0 && proposal->getVerifiablesCount() == 0)
			{
				tools->writeLine("The potential regular block is empty; dropped.");
				retval = eBlockFormationResult::emptyRegularBlock;
				error = true;
			}

			// Arm Data Block - BEGIN
			if (!error)
			{
				retval = eBlockFormationResult::blockFormed;
			}
			// Arm Data Block - END
		}
		// Checks - END

		if (retval == blockFormed)
		{
			tools->writeLine("The new block was formed properly.");
			tools->writeLine("Block-Statistics:");
			tools->writeLine("Block Type: " + std::string(formKeyBlock ? "Key Block" : "Regular Block"));

			if (!formKeyBlock)
			{
				tools->writeLine(std::to_string(proposal->getTransactionsCount()) + " transactions");
				tools->writeLine(std::to_string(proposal->getVerifiablesCount()) + " verifiables");
				tools->writeLine(proposal->getHeader()->getErgUsed().str() + " ERG used");
				tools->writeLine(proposal->getHeader()->getTotalBlockReward().str() + " GNC reward in Transaction Fees");
				tools->writeLine("I'll now remove collected transactions from MemPool..");

				/*
				* [ NOTE ]: Attitude towards memory-pool of pending objects.
				uint64_t rmovedCount = 0;
				for (int i = 0; i < proposal->getTransactionsIDs().size(); i++)
				{
				 assertGN(removeTransactionFromMemPool(proposal->getTransactionsIDs()[i]));
				}
				tools->writeLine("Freed " + std::to_string(rmovedCount) + " transactions:)");*/
				//do not remove transactions from mem-pool right now, they might be needed in near future in case of a fork
				//anyway, there's a cleanMemPoolFunction executed from time to time.
			}
			else
			{
				BigInt reward = proposal->getHeader()->getTotalBlockReward(true);
				tools->writeLine(tools->formatGNCValue(reward) + " of Key-Block Reward");
				tools->writeLine(std::to_string(proposal->getHeader()->getDifficulty()) + " difficulty");
			}

			block = std::move(proposal);
		}
		else
		{
			tools->writeLine("Something went wrong with new block formation.");
		}
	
		if (retval ==eBlockFormationResult::blockFormed )
		{
			if (block->getHeader()->isKeyBlock())
				incFormedKeyBlocksCounter();
			else
				incFormedDataBlocksCounter();
		}
		return retval;
}


bool CTransactionManager::powLoop(std::shared_ptr<CBlock> proposal)
{ 
	// Local Variables - BEGIN
	std::shared_ptr<CTools> tools = getTools();
	getTools()->writeLine("Commencing Proof-of-Work computations..");
	eWorkState workState = mWorkManager->getWorkState(getCurrentMinigTaskID());
	size_t lastTimeReported = tools->getTime();
	double lastReportedMhps = 0;
	double currenMhps = 0;
	size_t mTimeStarted = std::time(0);
	eWorkState previousState = eWorkState::enqueued;
	uint64_t warnings = 0;
	uint64_t now = std::time(0);
	std::shared_ptr<CStatusBarHub> barHub = CStatusBarHub::getInstance();
	eBlockchainMode::eBlockchainMode bMode = getBlockchainMode();
	std::shared_ptr<CBlockchainManager>  bm = getBlockchainManager();
	eBlockchainMode::eBlockchainMode mode = bm->getMode();
	uint64_t powBarID = getPowCustomBarID();
	std::vector<uint8_t> taskID = getCurrentMinigTaskID();
	std::shared_ptr<CWorkManager>  wm = getWorkManager();
	// Local Variables - END

	// Operational Logic - BEGIN

	while (getDoBlockFormation() && !(workState == eWorkState::doneAndCollected || workState == eWorkState::aborted))
	{
		now = std::time(0);
		
		workState = wm->getWorkState(taskID);

		if (previousState != workState)
			mTimeStarted = std::time(0);
		if (workState == eWorkState::unknown)
		{
			wm->abortWork(taskID);
			tools->writeLine("For some reason the result of the mining task is unknown; aborting..");
			return false;
		}

		currenMhps = wm->getCurrentTotalMhps();
		size_t diff = std::time(0) - mTimeStarted;
		if ((now - mLastDevReportModeSwitch)>5)
		{
			mLastDevReportMode = (mLastDevReportMode + 1) % 3;
			mLastDevReportModeSwitch = now;
		}
		if ((now- lastTimeReported) >= 0 || (lastReportedMhps==0 && currenMhps>0))
		{
			lastReportedMhps = wm->getCurrentTotalMhps();
			double aver = wm->getAverageMhps();

			//detect zombie process - BEGIN
			if (workState == eWorkState::running && aver == 0 && ((std::time(0) - mTimeStarted) > 90))
			{
				warnings++;
				if (warnings > 10)
				{
					tools->writeLine(tools->getColoredString("Aborting mining operation.. turned out to be a Zombie..", eColor::cyborgBlood));
					wm->abortWork(taskID);
				}
			}
			std::shared_ptr<CWorkManager> workManager = wm;
			//detect zombie process - END
			std::string  d = (diff % 2 ? "/" : "-");
			std::stringstream report;
			if (workState == eWorkState::enqueued)
				barHub->setCustomStatusBarText(bMode, powBarID, "Compiling Kernel.. (" + tools->secondsToString(diff) + " elapsed )");
			
			else
			{
				// Status Bar Report - BEGIN
				std::vector<std::shared_ptr<CWorker>> workers = workManager->getWorkers();
				switch (mLastDevReportMode)
				{
				case 0:
					report << tools->getColoredString("Total Mining Speed (MHps): ", eColor::lightCyan) + tools->to_string_with_precision(currenMhps, 4) + (currenMhps > aver ? tools->getColoredString(" >", eColor::lightGreen) : tools->getColoredString(" <", eColor::cyborgBlood)) + " aver.: " + tools->to_string_with_precision(aver, 4) + " (" + std::to_string((diff) / 60) + "min. " + std::to_string(diff % 60) + "sec. elapsed)";

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

				
				 barHub->setCustomStatusBarText(mode, powBarID, report.str());
				
				// Status Bar Report - END
			}
			lastTimeReported = tools->getTime();
		}

		if (workState == eWorkState::doneAndCollected)
		{
			std::vector<std::shared_ptr<IWorkResult>> results = wm->getWorkResults(taskID);

			for (int i = 0; i < results.size(); i++)
			{
				if (!std::dynamic_pointer_cast<CPoW>(results[i])->isPartialProof())
				{
					proposal->getHeader()->setNonce(std::dynamic_pointer_cast<CPoW>(results[i])->getNonce());
					break;
				}
			}
		}
		else if (workState == eWorkState::aborted)
		{
			break;
		}
		previousState = workState;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	}
	setCurrentMiningTaskID(std::vector<uint8_t>());
	barHub->invalidateCustomStatusBar(mode, powBarID);

	if (!getDoBlockFormation())
	{
		wm->abortWork(taskID);
	}
	if (workState == eWorkState::doneAndCollected && proposal->getHeader()->getNonce() > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
	// Operational Logic - END

}

void CTransactionManager::processUncleBlocks(std::vector < std::shared_ptr<CBlock>>  &blocks)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	getTools()->writeLine("Processing known uncle blocks..");
	//fetch recent uncle-blocks
	std::vector<std::shared_ptr<CBlock>> uncles =blocks;
	return;

	for (int u = 0; u < uncles.size(); u++)
	{
		uint64_t rewardValue = getBlockchainManager()->calculateRewardForUncleBlock((uncles[u]));
		CVerifiable reward = CVerifiable(eVerifiableType::uncleBlock, uncles[u]->getID());
		reward.addBalanceChange(uncles[u]->getHeader()->getMinersID(), rewardValue);
		//verifiables.push_back(std::shared_ptr<CVerifiable>(new CVerifiable(reward)));
	}
}
bool CTransactionManager::registerTransactionWithReceiptID(const CTransaction & trans, std::vector<uint8_t> receiptID)
{
	std::lock_guard<std::recursive_mutex> lock(mReceiptsGuardian);
	if (!getTools()->validateTransactionSemantics(trans))
		return false;
	if (mTransactionsReceiptsIndex.size() > mMaxReceiptsIndexCacheSize)
	{
		auto it = mTransactionsReceiptsIndex.cbegin();

		while (it != mTransactionsReceiptsIndex.cend() && (mTransactionsReceiptsIndex.size() > mMaxReceiptsIndexCacheSize))
		{
			it = mTransactionsReceiptsIndex.erase(it);
		}
		
	}

	mTransactionsReceiptsIndex[const_cast<CTransaction&>(trans).getHash()] = receiptID;
	
	return true;
}

bool CTransactionManager::registerVerifiablenWithReceiptID(const CVerifiable & ver, std::vector<uint8_t> receiptID)
{
	std::lock_guard<std::recursive_mutex> lock(mReceiptsGuardian);
	if (!getTools()->validateVerifiableSemantics(ver))
		return false;
	if (mTransactionsReceiptsIndex.size() > mMaxReceiptsIndexCacheSize)
	{
		auto it = mTransactionsReceiptsIndex.cbegin();

		while (it != mTransactionsReceiptsIndex.cend() && (mTransactionsReceiptsIndex.size() > mMaxReceiptsIndexCacheSize))
		{
			it = mTransactionsReceiptsIndex.erase(it);
		}

	}

	mTransactionsReceiptsIndex[const_cast<CVerifiable&>(ver).getHash()] = receiptID;

	return true;
}



bool CTransactionManager::getWasForcedPerspectiveProcessed()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mWasForcedPerspectiveProcessed;
}

void CTransactionManager::setWasForcedPerspectiveProcessed(bool was)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mWasForcedPerspectiveProcessed = was;
}

void CTransactionManager::setFraudulantDataBlocksToGeneratePerKeyBlock(uint64_t nr)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mFraudulantDataBlocksToGeneratePerKeyBlock = nr;
}

uint64_t CTransactionManager::getFraudulantDataBlocksToGeneratePerKeyBlock()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mFraudulantDataBlocksToGeneratePerKeyBlock;
}

uint64_t CTransactionManager::getKeyBlocksGeneratesUsingSamePubKey()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mKeyBlocksGeneratesUsingSamePubKey;
}

void CTransactionManager::incKeyBlocksGeneratesUsingSamePubKey()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mKeyBlocksGeneratesUsingSamePubKey++;
}

void CTransactionManager::incFraudulantDataBlocksOrdered()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mFraudulantDataBlocksOrdered++;
}

uint64_t CTransactionManager::getFraudulantDataBlocksOrdered()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mFraudulantDataBlocksOrdered;
}


void CTransactionManager::incNrOfTimesDeferredAssetsReleased()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNrOfTimesDeferredAssetsReleased++;
}

std::shared_ptr<CDTI> CTransactionManager::getDTI()
{
	std::lock_guard<std::mutex> lock(mDTIGuardian);
	return mDTI.lock();
}

uint64_t CTransactionManager::getTXsRevertedCount()
{
	return  mTXsRevertedCount;

}

uint64_t CTransactionManager::getTXsCleanedCount()
{
	 return mTXsCleanedCount;
}

void CTransactionManager::incTXsRevertedCount()
{
	  ++mTXsRevertedCount;
}

void CTransactionManager::incTXsCleanedCount()
{
	++mTXsCleanedCount;
}


uint64_t CTransactionManager::getNrOfTimesDeferredAssetsReleased()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNrOfTimesDeferredAssetsReleased;
}

void CTransactionManager::incNewFraudsDetected()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNewFraudsDetected++;
}

uint64_t CTransactionManager::getNewFraudsDetected()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNewFraudsDetected;
}

void CTransactionManager::incNrOfTimesTransactionsFailedDueToInsufficientFunds()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNrOfTimesTransactionsFailedDueToInsufficientFunds++;
}

uint64_t CTransactionManager::getNrOfTimesTransactionsFailedDueToInsufficientFunds()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNrOfTimesTransactionsFailedDueToInsufficientFunds;
}



void CTransactionManager::incFraudsDetectedButAlreadyRewarded()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mFraudsDetectedButAlreadyRewarded++;
}

uint64_t CTransactionManager::getFraudsDetectedButAlreadyRewarded()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mFraudsDetectedButAlreadyRewarded;
}


void CTransactionManager::incTotalValueOfFraudPenaltiesBy(BigInt val)
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mTotalValueOfPenalties +=val;
}

BigInt CTransactionManager::getTotalValueOfFraudPenalties()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mTotalValueOfPenalties;
}

void CTransactionManager::incTotalValueOfPoFRewardsBy(BigInt val)
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mTotalValueOfPoFRewards += val;
}

BigInt CTransactionManager::getTotalValueOfPoFRewards()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mTotalValueOfPoFRewards;
}

void CTransactionManager::incNrOfTransactionsFailesDueToOutofERG()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNrOfTransactionsFailesDueToOutofERG++;
}

uint64_t CTransactionManager::getNrOfTransactionsFailesDueToOutofERG()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNrOfTransactionsFailesDueToOutofERG;
}

//
void CTransactionManager::incNrOfTransactionsFailesDueToInvalidEnvelopeSig()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNrOfTTransactionsFailedDueToInvalidEnvelopeSig++;
}

uint64_t CTransactionManager::getNrOfTransactionsFailesDueToInvalidEnvelopeSig()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNrOfTTransactionsFailedDueToInvalidEnvelopeSig;
}

void CTransactionManager::incNrOfTransactionsFailesDueToInvalidNonce()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNrOfTTransactionsFailedDueToInvalidNonce++;
}

uint64_t CTransactionManager::getNrOfTransactionsFailesDueToInvalidNonce()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNrOfTTransactionsFailedDueToInvalidNonce;
}

void CTransactionManager::incNrOfTransactionsFailesDueToUnknownIssuer()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNrOfTransactionsFailedDueToUnknownIssuer++;
}

uint64_t CTransactionManager::getNrOfTransactionsFailesDueToUnknownIssuer()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNrOfTransactionsFailedDueToUnknownIssuer;
}

void CTransactionManager::incNrOfTransactionsFailesDueToNotIncludedInBlock()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNrOfInvalidTransactionNotIncludedInBlock++;
}

uint64_t CTransactionManager::getNrOfTransactionsFailesDueToNotIncludedInBlock()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNrOfInvalidTransactionNotIncludedInBlock;
}

void CTransactionManager::incNrOfAttemptsToSpendLockedAssets()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNrOfAttemptsToSpendLockedAssets++;
}

uint64_t CTransactionManager::getNrOfAttemptsToSpendLockedAssets()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNrOfAttemptsToSpendLockedAssets;
}

void CTransactionManager::incProofsOfFraudProcessed()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mProofsOfFraudProcessed++;
}

uint64_t CTransactionManager::getProofsOfFraudProcessed()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mProofsOfFraudProcessed;
}

void CTransactionManager::incNrOfTransactionsWithInvalidNonceValues()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mNrOfTransactionsWithInvalidNonceValues++;
}

uint64_t CTransactionManager::getNrOfTransactionsWithInvalidNonceValues()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mNrOfTransactionsWithInvalidNonceValues;
}


void CTransactionManager::resetKeyBlocksGeneratesUsingSamePubKey()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mKeyBlocksGeneratesUsingSamePubKey = 0;
}

uint64_t CTransactionManager::getDataBlocksGeneratedUsingSamePubKey()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	return mDataBlocksGeneratesUsingSamePubKey;
}

void CTransactionManager::incDataBlocksGeneratesUsingSamePubKey()
{
	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mDataBlocksGeneratesUsingSamePubKey++;
}

void CTransactionManager::resetDataBlocksGeneratesUsingSamePubKey()
{	std::lock_guard<std::recursive_mutex> lock(mStatDataGuardian);
	mDataBlocksGeneratesUsingSamePubKey = 0;
}

void CTransactionManager::setNewPubKeyEveryNKeyBlocks(uint64_t blockCount)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	if (mSettings->getLoadPreviousConfiguration())
		getTools()->logEvent("New identity will be used for mining every " + std::to_string(blockCount) + " key-blocks.", eLogEntryCategory::localSystem, 3);
	else
		getTools()->writeLine("New identity will be used for mining every " + std::to_string(blockCount) + " key - blocks.");
	mNewPubKeyEveryNKeyBlocks = blockCount;
}

uint64_t CTransactionManager::getNewPubKeyEveryNKeyBlocks()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mNewPubKeyEveryNKeyBlocks;
}

void CTransactionManager::setCycleBackIdentiesAfterNKeysUsed(uint64_t keysCount)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	getTools()->logEvent("Up to " + std::to_string(keysCount) + " identities (from same key-chain) will be used for mining purposes.", eLogEntryCategory::localSystem, 3);
	mCycleBackIdentiesAfterNKeysUsed = keysCount;
}

uint64_t CTransactionManager::getCycleBackIdentiesAfterNKeysUsed()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mCycleBackIdentiesAfterNKeysUsed;
}


eBlockchainMode::eBlockchainMode CTransactionManager::getBlockchainMode()
{
	std::lock_guard<std::mutex> lock(mBlockchainModeGuardian);
	
	return mBlockchainMode;
}

bool CTransactionManager::getSynchronizeBlockProduction()
{
	std::lock_guard<std::mutex> lock(mSynchronizeBlockProductionGuardian);
	
	return mSynchronizeBlockProduction;
}

void CTransactionManager::setSynchronizeBlockProduction(bool doIt)
{
	std::lock_guard<std::mutex> lock(mSynchronizeBlockProductionGuardian);
	mSynchronizeBlockProduction = doIt;
}

bool CTransactionManager::haltBlockProductionFor(size_t seconds)
{
	std::lock_guard<std::mutex> lock(mBlockProductionHaltedTillTimeStampGuardian);
	getTools()->writeLine("Block production will halt for " + std::to_string(seconds) + " seconds");
	mBlockProductionHaltedTillTimeStamp = getTools()->getTime()+ seconds;
	return true;
}

bool CTransactionManager::isBlockProductionHalted()
{
	std::lock_guard<std::mutex> lock(mBlockProductionHaltedTillTimeStampGuardian);
	if  (mBlockProductionHaltedTillTimeStamp > getTools()->getTime())
		return true;
	else
	return false;
}

size_t CTransactionManager::getBlockProductionResumptionTimestamp()
{
	std::lock_guard<std::mutex> lock(mBlockProductionHaltedTillTimeStampGuardian);
	return mBlockProductionHaltedTillTimeStamp;
}

std::vector<uint8_t> CTransactionManager::getForcedMiningBlockID()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mForcedMiningParentBlockID;
}

bool CTransactionManager::getInformedWaitingForTransactions()
{
	std::lock_guard<std::mutex> lock(mNotificationsGuardian);
	return mNotifiedWaitingForTransactions;
}

void  CTransactionManager::setInformedWaitingForTransactions(bool didThat)
{
	std::lock_guard<std::mutex> lock(mNotificationsGuardian);
	mNotifiedWaitingForTransactions = didThat;
}

bool CTransactionManager::getDoBlockFormation()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mDoBlockFormation;
}

void CTransactionManager::setDoBlockFormation(bool doIt, bool resetThreads)
{
	if (doIt)
	{
		assertGN(getMode() == eTransactionsManagerMode::FormationFlow);
	}

	if (doIt && resetThreads)
		getTools()->writeLine("Enabling block formation. Waiting for the system to halt..");

	mFieldsGuardian.lock();
	mDoBlockFormation = doIt;
	mFieldsGuardian.unlock();

	if (resetThreads)
	{
		pause();
		resume();
	}

	if (doIt && resetThreads)
		getTools()->writeLine("Block formation is now enabled.");
	else if (!doIt && resetThreads)
		getTools()->writeLine("Block formation is now disabled.");
}


eTransactionsManagerMode::eTransactionsManagerMode CTransactionManager::getMode()
{
	std::lock_guard<std::mutex> lock(mModeGuardian);
	return mMode;
}

uint64_t CTransactionManager::getTransactionsCounter()
{
	std::lock_guard<std::mutex> lock(mStatisticsCounterGuardian);
	return mProcessedTransactions;
}

uint64_t CTransactionManager::getVerifiablesCounter()
{
	std::lock_guard<std::mutex> lock(mStatisticsCounterGuardian);
	return mProcessedVerifiables;
}

void CTransactionManager::incProcessedVerifiablesCounter()
{
	std::lock_guard<std::mutex> lock(mStatisticsCounterGuardian);
	mProcessedVerifiables++;
}

uint64_t CTransactionManager::getFormedKeyBlocksCounter()
{
	std::lock_guard<std::mutex> lock(mBlocksCounterGuardian);
	return mKeyBlocksFormedCounter;
}

uint64_t CTransactionManager::getFormedDataBlocksCounter()
{
	std::lock_guard<std::mutex> lock(mBlocksCounterGuardian);
	return mDataBlocksFormedCounter;
}

void CTransactionManager::incFormedKeyBlocksCounter()
{
	std::lock_guard<std::mutex> lock(mBlocksCounterGuardian);
	mKeyBlocksFormedCounter++;
}

void CTransactionManager::incFormedDataBlocksCounter()
{
	std::lock_guard<std::mutex> lock(mBlocksCounterGuardian);
	mDataBlocksFormedCounter++;
}

bool CTransactionManager::getIsInFlow()
{
	std::lock_guard<std::mutex> lock(mIsInFlowGuardian);

	return mInFlow!=0;
}

void CTransactionManager::setIsInFlow(bool is)
{
	std::lock_guard<std::mutex> lock(mIsInFlowGuardian);

	if (is)
	{
		std::thread::id  threadID = std::this_thread::get_id();
		std::stringstream ss;
		ss << threadID;
		uint64_t iID = std::stoul(ss.str());
		mInFlow = iID;
		mFlowGuardian.lock();
	}
	else
	{
		mInFlow = 0;
		mFlowGuardian.unlock();
	}
}

void CTransactionManager::setIsSettingUpFlow(bool is)
{
	std::lock_guard<std::mutex> lock(mIsInFlowGuardian);
	std::thread::id  threadID = std::this_thread::get_id();
	std::stringstream ss;
	ss << threadID;
	uint64_t iID = std::stoul(ss.str());
	if (is)
		mIsSettingUpFlow = iID;
	else
	{
		if(mIsSettingUpFlow == iID)//unlock only if current thread locked the mechanism.
		mIsSettingUpFlow = 0;
	}
}

bool CTransactionManager::getIsSomebodyElseSettingUpFlow()
{
	std::lock_guard<std::mutex> lock(mIsInFlowGuardian);
	std::thread::id  threadID = std::this_thread::get_id();
	std::stringstream ss;
	ss << threadID;
	uint64_t iID = std::stoul(ss.str());
	// std::hash<std::thread::id>()(std::this_thread::get_id())
	if (mIsSettingUpFlow == iID || mIsSettingUpFlow == 0)
		return false;
	else
	return true;
}

/**
 * @brief Registers a transaction for processing with immediate receipt ID generation
 *
 * This method processes a new transaction and generates a receipt ID that will match
 * the actual receipt ID once the transaction is included in a blockchain block.
 * Performs duplication checks and semantic validation before registration.
 *
 * @param trans The transaction to register
 * @param receiptIDToRet Output parameter for the generated receipt ID
 * @param genMetaData Flag to control transaction metadata generation
 * @return bool True if registration successful, false otherwise
 */
bool CTransactionManager::registerTransaction(const CTransaction& trans,
	std::vector<uint8_t>& receiptIDToRet,
	bool genMetaData) {
	// Local Variables - BEGIN
	std::shared_ptr<CTools> tools = getTools();
	std::shared_ptr<CTransaction> transPtr;
	std::string txHashEncoded;
	std::string baseCheckID;
	bool metaDataRegistered = true;
	// Local Variables - END

	// Pre-flight Validation - BEGIN
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	receiptIDToRet.clear();

	if (!tools->validateTransactionSemantics(trans)) {
		return false;
	}
	// Pre-flight Validation - END

	// Transaction Processing - BEGIN
	transPtr = std::make_shared<CTransaction>(trans);
	txHashEncoded = tools->base58CheckEncode(static_cast<CTransaction>(trans).getHash());

	if (isObjectInMemPool(transPtr)) {
		tools->writeLine("TX " + txHashEncoded +
			tools->getColoredString(" is already known.", eColor::orange));
		return false;
	}
	// Transaction Processing - END

	// Receipt Generation - BEGIN
	receiptIDToRet = tools->getReceiptIDForTransaction(getBlockchainMode(), static_cast<CTransaction>(trans).getHash());
	baseCheckID = tools->base58CheckEncode(receiptIDToRet);
	registerTransactionWithReceiptID(trans, receiptIDToRet);
	// Receipt Generation - END

	// Memory Pool Registration - BEGIN
	if (!addToMemPool(transPtr)) {
		tools->writeLine("Registering transaction " + txHashEncoded +
			" within the Transaction Manager failed");
		return false;
	}
	// Memory Pool Registration - END

	// Metadata Generation and Registration - BEGIN
	if (genMetaData) {
		std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
		CSearchResults::ResultData txMeta = bm->createTransactionDescription(
			transPtr, nullptr, 0, true, nullptr, "");

		if (!txMeta.isNull()) {
			metaDataRegistered = bm->addMemPoolTXMeta(receiptIDToRet, txMeta);
			if (!metaDataRegistered) {
				tools->writeLine(tools->getColoredString(
					"Warning: Failed to register transaction metadata in blockchain manager",
					eColor::cyborgBlood));
			}
		}
		CSearchResults::ResultData retMeta = bm->getMemPoolTXMeta(receiptIDToRet);
		assertGN(!retMeta.isNull());
	}
	// Metadata Generation and Registration - END

	// Status Reporting - BEGIN
	tools->writeLine("Transaction: " + txHashEncoded +
		tools->getColoredString(" registered", eColor::lightGreen) +
		". Receipt: " + baseCheckID +
		(genMetaData && !metaDataRegistered ?
			" (metadata registration failed)" : ""));
	// Status Reporting - END

	return true;
}

/// <summary>
/// Registers a Verifiable for processing.  The receipt ID is returned instantly.
/// In future, the actual receipt ID will match the one provided by this function, once the block with the Verifiable is appended to the blockchain.
/// Does check if verifiable is already within the pool. Return false if already there.
/// </summary>
/// <param name="trans"></param>
/// <param name="receiptID"></param>
/// <returns></returns>
bool CTransactionManager::registerVerifiable(const CVerifiable & ver, std::vector<uint8_t>& receiptIDToRet)
{
	std::shared_ptr<CTools> tools = getTools();


	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	receiptIDToRet.clear();
	if (!tools->validateVerifiableSemantics(ver))
		return false;

	std::shared_ptr<CVerifiable> v = std::shared_ptr<CVerifiable>(new CVerifiable(ver));
	if (isObjectInMemPool(v))
		return false;
	receiptIDToRet = tools->getReceiptIDForVerifiable(getBlockchainMode());//it'll be a random vector..
	registerVerifiablenWithReceiptID(ver, receiptIDToRet);

	if (!addToMemPool(v))
	{
		tools->writeLine("Registering Verifiable " + tools->base58CheckEncode(const_cast<CVerifiable&>(ver).getHash()) + "within the Transaction Manager failed");
		return false;
	}

	tools->writeLine("Verifiable: " + tools->base58CheckEncode(const_cast<CVerifiable&>(ver).getHash()) + " registered. Receipt: " + tools->base58CheckEncode(receiptIDToRet));
	return true;
}



std::vector<uint8_t> CTransactionManager::getReceiptIDForTransaction(const CTransaction & trans)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return getReceiptIDForTransaction(const_cast<CTransaction&>(trans).getHash());
}

std::vector<uint8_t> CTransactionManager::getReceiptIDForTransaction(std::vector<uint8_t> transactionID,std::shared_ptr<CBlock> block)
{
	std::lock_guard<std::recursive_mutex> lock(mReceiptsGuardian);
	std::vector<uint8_t> receiptID;
	std::map<std::vector<uint8_t>, std::vector<uint8_t>>::iterator it;
	
	it = mTransactionsReceiptsIndex.find(transactionID);
	if (it != mTransactionsReceiptsIndex.end())
		receiptID = it->second;
	return receiptID;

}

std::vector<uint8_t> CTransactionManager::getReceiptIDForVerifiable(const CVerifiable & ver)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return getReceiptIDForVerifiable(const_cast<CVerifiable&>(ver).getHash());
}

std::vector<uint8_t> CTransactionManager::getReceiptIDForVerifiable(std::vector<uint8_t> verifiableID, std::shared_ptr<CBlock> block)
{
	std::lock_guard<std::recursive_mutex> lock(mReceiptsGuardian);
	std::vector<uint8_t> receiptID;
	std::map<std::vector<uint8_t>, std::vector<uint8_t>>::iterator it;

	it = mTransactionsReceiptsIndex.find(verifiableID);
	if (it != mTransactionsReceiptsIndex.end())
		receiptID = it->second;
	return receiptID;

}
/// <summary>
/// Process a bunch of transactions in an ACID flow.
/// This function is very multi-functional so to say.
/// It returns true if everything went fine.
///  It returns receipts of processed transactions.
/// 
/// Now to complicate things; if a Block is provided. The function can be used to process transactions and to insert them into the block,
/// OR, if verifyAgainstBlock param is set; then the function will verify transactions against receipts present within the block.
/// 
/// If a Transaction result is found not to match its corresponding receipt within the block OR if the receipt within the Block is missing, 
/// False would be returned.
/// 
/// IF a Block is provided; the function needs to EITHER verify transactions against the block OR inject them inside.
/// </summary>
/// <param name="transactionsToBeProcessed"></param>
/// <param name="receipts"></param>
/// <param name="markAsProcessed"></param>
/// <param name="proposal"></param>
/// <param name="addToBlock"></param>
/// <param name="verifyAgainstBlock"></param>
/// <returns></returns>
bool CTransactionManager::processTransactions(std::vector<std::shared_ptr<CTransaction>>& transactionsToBeProcessed, std::vector<CReceipt> &receipts, bool markAsProcessed, std::shared_ptr<CBlock> proposal, bool addToBlock, bool verifyAgainstBlock, bool doProcessBreakpoints)
{

	/* [ Truths of Faith ]
	   
	   - all transactions that affect a state-domain and/or system in ANY way need to increment Vector IV (nonce) associated with a corresponding issuer's state domain
	     This holds for logs, meta-data, anything.
	   - once we revet all intermittent StateDBs from transaction's StateDB stack - this implies the transaction cannot affect the system in ANY way. Not a single byte.
	   - processing of a TX may change over time. This needs to be handled through backwards compatibility layers. What's in the past stays in the past.
	   - the fact that processsTransaction() succeeds does not imply that ACID processing of that transaction suceeded as well. The method is only part of it.
		 Fees are deducted after processsTransaction(). 
	   - GridScript VM does NOT keep track of Value Transfer API calls. Domain is checked to cover for  ERG Usage x ERG Lilimit prior to code execution but if all assets are spent thorugh a call to say
	     'send'- then processsTransaction() would have no way of knowing. Thus, it needs to be chacked after  processsTransaction() whether fees' deduction succeeded.
		 If, fees deduction failed - when forming block - the transaction is REJECTED - it does not affect the system in ANY way even result is not stored nor logs, nothing.
		 Fees deduction failure at this stage is not expected to occur for transactions already present on chain.
	   - processTransactions() method accounts for all transactions currently present in mem-pool. Never return upon failure of a single transaction. Use revertTX() to pop all
	     Merkle Patricia State tries from TX stack and 'continue' with further transactions.

	*/
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	//doProcessBreakpoints = false;
	std::stringstream debugLog;
	CProcessTransactionRAII logFinalizer(debugLog, getTools());
#endif

	//Critical Elements - BEGIN
	//these should never happen
 assertGN(getIsInFlow());
 assertGN(proposal != nullptr);
	//Critical Elements - BEGIN
	
	//Local Variables - BEGIN
	uint64_t countOfDomainsAffectingTheBlockchain = 0;
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(mFlowProcessingGuardian);
	std::string news = "Processing " + std::to_string(transactionsToBeProcessed.size()) + " transactions..";
	//bool wasInterDBDeleted = false;
	bool penaltyDBNeedsToBeRemoved = false;
	std::vector<uint8_t> receiptID;
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	std::string notes;
	std::shared_ptr<CConversation> deliveryConv;


	std::shared_ptr<CTools> tools = getTools();

	std::vector<uint8_t> transID;

	// BUG #3 Fix: Track issuers whose transactions succeeded
	// so we can retry their previously-deferred transactions in the same block
	std::set<std::vector<uint8_t>> issuersWithSuccessfulTx;
	//Local Variables - END


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	// Debug Logging - BEGIN
	debugLog << tools->getColoredString("[processTransactions]", eColor::lightCyan) << " Called with arguments => "
		<< "markAsProcessed: " << (markAsProcessed ? "true" : "false") << ", "
		<< "addToBlock: " << (addToBlock ? "true" : "false") << ", "
		<< "verifyAgainstBlock: " << (verifyAgainstBlock ? "true" : "false") << ", "
		<< "doProcessBreakpoints: " << (doProcessBreakpoints ? "true" : "false") << std::endl;

	// Log current root of the Merkle Patricia Trie
	debugLog << "[processTransactions] Current root of the Merkle Patricia Trie: "
		<< tools->base58CheckEncode(mFlowStateDB->getPerspective())
		<< std::endl;
	// Debug Logging - END
#endif
	
	//Operational Logic - BEGIN
	
	tools->writeLine(news);


	if (proposal != NULL && ((addToBlock == false && verifyAgainstBlock == false) || (addToBlock&&verifyAgainstBlock)))
		return false;

	if (transactionsToBeProcessed.size() == 0)
		return true;

		for (int i = 0;  getRequestedStatusChange() != eManagerStatus::eManagerStatus::stopped && i < transactionsToBeProcessed.size() && mUtilizedBlockSize < CGlobalSecSettings::getMaxDataBlockSize()
			&& mUtilizedERG < mMaxERGUsage; i++)
		{

			transID = transactionsToBeProcessed[i]->getHash();

			notes = "";
			deliveryConv = transactionsToBeProcessed[i]->getIssuingConversation();
			//refresh variables - BEGIN
			//wasInterDBDeleted = false;
			penaltyDBNeedsToBeRemoved = false; // once per-block thresholds reached
			receiptID.clear();
			CReceipt blockReceipt(getBlockchainMode());
			std::vector<uint8_t> confirmedDomainID;
		
			//refresh variables - END

			//Operations Affecting State-Domains - BEGIN
			
			if (transactionsToBeProcessed[i]->HotProperties.isMarkedAsProcessed())
				continue;
			

			if (verifyAgainstBlock)

			{
				if (!proposal->getHeader()->getReceiptForTransaction(transactionsToBeProcessed[i]->getHash(), blockReceipt, false))
				{
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					debugLog << "[Error] No receipt found for TX: "
						<< tools->base58CheckEncode(transactionsToBeProcessed[i]->getHash())
						<< std::endl;
#endif
					return false; // block is invalid
				}
				receiptID = blockReceipt.getGUID(); // receipt ID already serialized within
			}
			else {
				receiptID = getReceiptIDForTransaction(transactionsToBeProcessed[i]->getHash(), proposal); // we need to generate Receipt ID for newly formed block
			}
			
			//process code
			uint64_t sourceDomainNonce = 0;
			std::shared_ptr<CReceipt> blockReceiptShrd = std::make_shared<CReceipt>();
			(*blockReceiptShrd) = blockReceipt;


			// Transaction Breakpoints' Support - BEGIN
			// Pre-execution breakpoint processing
			if (doProcessBreakpoints && bm->getBreakpointFactory()->getTransactionBreakpointCount(true))
			{
				// if 'addToBlock' argument is true and verifyAgainstBlock is false - then the block is just being formed and the transaction is to be appended to it.
				// if 'addToBlock' argument is false - then we are processing a transaction which is part of a block already present within the blockchian.
				//							       in such a case the blockReceipt would be available.
			
				if (!processBreakpoints(*transactionsToBeProcessed[i], 
					receiptID,
					proposal?proposal->getHeader()->getKeyHeight():0,
					eBreakpointState::preExecution, verifyAgainstBlock ? std::make_shared<CReceipt>(blockReceipt) : nullptr, verifyAgainstBlock ? proposal : nullptr)) {
					// If breakpoint processing indicates we should abort
					mTools->logEvent("Unable to process transaction breakpoints", "Debugging", eLogEntryCategory::localSystem, 5, eLogEntryType::failure);
				}
			}
			// Transaction Breakpoints' Support - END
			std::vector<uint8_t> perspectiveBefore = getPerspective(mInSandbox ? -1 : 0);

			// Debug Logging - BEGIN
			// We already have the transaction ID in 'transID', so let's log perspective before processing
		
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			debugLog << "[Before processTransaction] TX: "
				<< tools->base58CheckEncode(transID)
				<< ", perspectiveBefore: "
				<< tools->base58CheckEncode(perspectiveBefore)
				<< std::endl;
#endif
			// Debug Logging - END

		

			// Backwards Compatibility Layer (Pre-Execution Part 1) - BEGIN
			bool excuseERGusage = false;
			bool enforceFailure = false;

		
			if (proposal->getIsCheckpointed())	 // block needs to be checkpointed (valid)
			{
				// Compatibility Layer CVE_804 see also CVE_806 ( Part 1 )- BEGIN (see also CVE_803, CVE_804)
				// [ Rationale ]: if transaction formerly succeeded we must allow it through now as well - even though ERG costs associated with code execution might have changed (since then).
				if (blockReceipt.getResult() == eTransactionValidationResult::valid) // the transaction (code bundle) needs to have previously succeeded (achieved complete execution))
				{
					excuseERGusage = true;
				}
				// Compatibility Layer CVE_804 ( Part 1 ) - END
				
				// Compatibility Layer CVE_805 ( Part 1 ) - BEGIN (see also CVE_803, CVE_804)
				// [ Activation ]:  operational since block height 85366 
				// [ Assumptions ]: we benefit from the fact that tranasctions' (GridScrit code bundles') procesisng is ATOMIC.
				//				    In other words, if processing of a single GridScript instruction fails (an exception is thrown) the entire code package is rolled back (ACID).
				// [ Rationale ]:  errors in the past are in the past. Transactopn's processing failures are to prevail.
				
				if (blockReceipt.getResult() == eTransactionValidationResult::invalid // Corresponds to "GridScritpt error"
					&& proposal->getHeader()->getHeight() > 60467 // <- REQUIRED minimal blockchain height
																  // it's the height at which there's the only TX which succeeds even though block recept says invalid.
																  // before 1.6.5 Hard Fork. We want the TX to succeed. For all newer blocks - what's in the past
																  // shall be in the past.
					)
				{
					enforceFailure = true;
				}

				// Compatibility Layer CVE_805 ( Part 1 ) - END
			}
	
			bool flowFailedForTX = false; // we can NEVER abort entire ACID flow for a transaction which is part of a checkpointed history of events.
			bool postTxFeesFailed = false;//<- right now this has no effect. We assume tranasctions which are already on chain should never result in this.

			// Backwards Compatibility Layer (Pre-Execution Part 1) - END
			

			resetCurrentTXDBStackDepth(); // we need to clear current transaction stack POINTER.

			// CAUTION: only when verifying against block, in which the TX already exists, we pass the block and the existing Receipt.
			//          Only a COPY of this Receipt is passed along.
			//          Fees would be detected after processTransaction().


			// [ IMPORTANT }: need to be set prior to transaction execution
			setCurrentFlowObjectID(tools->getReceiptIDForTransaction(eBlockchainMode::TestNet, transactionsToBeProcessed[i]->getHash()));

			CReceipt effectiveReceipt = processTransaction(*transactionsToBeProcessed[i],
				proposal->getHeader()->getKeyHeight(),
				confirmedDomainID,
				receiptID,
				sourceDomainNonce,
				verifyAgainstBlock?blockReceiptShrd:nullptr,
				verifyAgainstBlock ? proposal:nullptr,
				excuseERGusage, // GridScript VM is to account for but excuse excessive ERG usage. An exception would NOT be thrown.
				enforceFailure
			);

			std::stringstream report;
			bool crossValidationReportAvailable = false;

			// Finalizer - BEGIN
			// [ Assumptions ]:
			//					- reverting back to state depper than 1 is not needed after transaction was processed - as part of a single ACID Flow
			//					- this allows to save on RAM
			//					- full state reversal is performed only at block processing level
			FinalAction txCleanup([this, &crossValidationReportAvailable ,&report,&flowFailedForTX ,&postTxFeesFailed,&effectiveReceipt, &tools]() {
			
				// Finality Checks - BEGIN
				
				// Finality Checks - END

				// Pop ACID databases - BEGIN
				if (effectiveReceipt.HotProperties.getDoNotIncludeInChain())
				{ // [ Rationale ]: transaction is not making part of the global history of events so we pop all ACID databases relevant to this transaction and revert to a database
				  //				prior to when this transaction was scheduled.
				  
					// Revert State - BEGIN
					assertGN(revertTX()); // WARNING: the only place where revertTX() may be called; othewise it would double revert causing mismatch in counters;
					// Revert State - END
				}
				else
				{
					// Free memory - no state reversal - BEGIN
					// [ Rationale ]: transaction IS making part of the global history of events so we pop all ACID databases relevant to this transaction and DO NOT revert to a database
					//				prior to when this transaction was scheduled. This is irrelevant to GridScript code processing results (value transfer may have suceeded OR failed).


					//            v------  WARNING: it's the only place where popCurrentTXDBs() may be called; othewise it would double clean causing mismatch in counters.
					assertGN(popCurrentTXDBs()); // pop all intermediary and final ACID databases related to this transaction procesing (sub)state(s).
					// Note: this does NOT revert processing result(s).
					// do this for valid completed transactions, as well as, invalid ones.

					// Free memory - no state reversal - END
				}
				// Pop ACID databases - END


				// Clear Current ACID object - BEGIN
				setCurrentFlowObjectID(std::vector<uint8_t>());
				// Clear Current ACID object - END

				// Logging - BEGIN
				
				// Cross Hard Fork Validation - BEGIN
				if (!crossValidationReportAvailable)
				{
					return;
				}
				if (!flowFailedForTX)
				{
					report << "[ Final Result ]: " << effectiveReceipt.translateStatus() << "\n";
				}
				else
				{
					report << "[ CRITICAL ERROR ]: " << "Flow of the enture block was aborted!" << "\n";
				}
				if (postTxFeesFailed)
				{
					report << "[ CRITICAL ERROR ]: " << "Post TX fees deduction failed!" << "\n";
				}

				report << "^--------------------------------^" << "\n\n";
					tools->writeToFile("integrity.txt", report.str());
				});
				// Cross Hard Fork Validation - END
				// 
			// Logging - END

			// Finalizer - END
			
			// Backwards Compatibility Layer CVE_804 (Post-Execution Part 2) - BEGIN
			if (excuseERGusage)
			{
				BigInt effectiveERGUsed = effectiveReceipt.getERGUsed();
				BigInt blockERGUsed = blockReceipt.getERGUsed();

				// ERG Usage Compatibility - BEGIN

				effectiveReceipt.setERGUsed(blockERGUsed); // make block ERG usage effective

				// Debug Logging - BEGIN
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			
				if (effectiveERGUsed > blockERGUsed)
				{
					debugLog << "[ERG Usage Compatibility]: " << 
						mTools->getColoredString("excused " + (effectiveERGUsed - blockERGUsed).str() + " additional ERG", eColor::lightPink) << std::endl;

				}
#endif
				// Debug Logging - END
				
				// ERG Usage Compatibility - END
			}
			// Backwards Compatibility Layer CVE_804 (Post-Execution Part 2) - END

			// Backwards Compatibility Layer CVE_805 (Post-Execution Part 2) - BEGIN
			
			if (enforceFailure)
			{
				// [ Notice ] : when failure is being enforced - the block receipt became the effective receipt ( done in processTransaction()).

				BigInt effectiveERGUsed = effectiveReceipt.getERGUsed();
				BigInt blockERGUsed = blockReceipt.getERGUsed();

				// Backwards Failure Compatibility - BEGIN

				effectiveReceipt.setERGUsed(blockERGUsed); // make block ERG usage effective

				// Debug Logging - BEGIN
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1


					debugLog << "[Backwards Failure Compatibility]: " <<
						mTools->getColoredString("killed processing.", eColor::lightPink) << std::endl;

			
#endif
				// Debug Logging - END

				// Backwards Failure Compatibility - END
			}
			// Backwards Compatibility Layer CVE_805  (Post-Execution Part 2) - END

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
			// Debug Logging - BEGIN
			// Now let's log perspective after processing
			std::vector<uint8_t> perspectiveAfter = getPerspective(mInSandbox ? -1 : 0);
			debugLog << "[After processTransaction] TX: "
				<< tools->base58CheckEncode(transID)
				<< ", perspectiveAfter: "
				<< tools->base58CheckEncode(perspectiveAfter)
				<< std::endl;
			// Debug Logging - END

#endif

			// Block Verification - BEGIN 

			// Cross Hard Fork compatibilitu Checks - BEGIN
			// (i.e. not block formulation)
			if (verifyAgainstBlock)
			{

				// Nonce Increment Enforcement - BEGIN
				effectiveReceipt.HotProperties.markDoNotIncludeInChain(false);// since the TX is already on chain thus we stem from an assumption that in the past nonce must have been incremented.
				// even if processing result changes, we need to make sure nonce is updated otherwise we open up accounts to attacks or
				//	invalid processing of future TXs, of those already present on chain, issued in the name of same account (nonce would be too low after the processing result changes).
// Nonce Increment Enforcement - END

				// Consensus Discrepancy Detection - BEGIN
				// We detect two levels of discrepancies:
				// Level 1: Result status differs (valid vs invalid, etc.)
				// Level 2: Result status same, but error message/reason differs

				std::vector<std::string> effectiveTrace = effectiveReceipt.getLog();
				std::vector<std::string> originalTrace = blockReceipt.getLog();

				// Level 1: Check if result status differs
				bool level1Discrepancy = (effectiveReceipt.getResult() != blockReceipt.getResult());

				// Level 2: Check if error messages differ (compare last log entries)
				bool level2Discrepancy = false;
				std::string effectiveError = "";
				std::string originalError = "";

				if (!level1Discrepancy && effectiveTrace.size() > 0 && originalTrace.size() > 0)
				{
					// Get last entries from both logs (typically the final error message)
					effectiveError = effectiveTrace[effectiveTrace.size() - 1];
					originalError = originalTrace[originalTrace.size() - 1];

					if (effectiveError != originalError)
					{
						level2Discrepancy = true;
					}
				}

				// Report if any discrepancy detected
				if (level1Discrepancy || level2Discrepancy)
				{
					//Checkpoints' Support - BEGIN
					if (proposal->getIsCheckpointed())
					{
						// Inconsistencies' Logging - BEGIN

						// File Log - BEGIN
						crossValidationReportAvailable = true;

						if (level1Discrepancy)
						{
							report << "v---- [ Level 1 Discrepancy: Result Status Differs ] ----v" << "\n";
						}
						else if (level2Discrepancy)
						{
							report << "v---- [ Level 2 Discrepancy: Error Message Differs ] ----v" << "\n";
						}

						// Critical Discrepancies: BEGIN
						// Note: if any of these happen - a backwards compatibility layer needs to be introduced
						if (proposal->getIsCheckpointed())
						{
							if (flowFailedForTX)
							{
								report << "Critical: FLOW FAILED FOR THIS TRANSACTION " << "\n";
							}
							if (blockReceipt.getResult() == eTransactionValidationResult::valid && effectiveReceipt.getResult() == eTransactionValidationResult::invalidNonce)
							{
								report << "Critical: Nonce Incompatibility. " << "\n";
							}
						}
						// Critical Discrepancies: END

						report << "[ Discrepancy Level ]: " << (level1Discrepancy ? "Level 1 (Result Status)" : "Level 2 (Error Message)") << "\n";
						report << "[ Receipt ]: " << tools->base58CheckEncode(mTools->getReceiptIDForTransaction(eBlockchainMode::TestNet, transactionsToBeProcessed[i]->getHash())) << "\n";
						report << "[ TX ID ]: " << tools->base58CheckEncode(transactionsToBeProcessed[i]->getHash()) << "\n";
						report << "[ Date ]: " << tools->timeToString(proposal->getHeader()->getSolvedAtTime()) << "\n";
						report << "[ Block Height ]: " << std::to_string(proposal->getHeader()->getHeight()) << "\n";
						report << "[ Previous Result ]: " << blockReceipt.translateStatus() << "\n";
						report << "[ Current Pre-Result ]: " << effectiveReceipt.translateStatus() << "\n";

						if (level2Discrepancy)
						{
							report << "[ Previous Error Message ]: " << originalError << "\n";
							report << "[ Current Error Message ]: " << effectiveError << "\n";
						}

						report << "[ Source Domain ]: " << tools->bytesToString(confirmedDomainID) << "\n";
						report << "[ Source Code ]: " << mScriptEngine->getSourceCode(transactionsToBeProcessed[i]->getCode()) << "\n";
						report << "[ TX Nonce ]: " << std::to_string(transactionsToBeProcessed[i]->getNonce()) << "\n";
						report << "[ Domain Nonce ]: " << std::to_string(sourceDomainNonce) << "\n";

						// Stack Traces - BEGIN

						// Effective Stack Trace - BEGIN
						report << "[ Effective Stack Trace ]: " << "\n";

						for (uint64_t i = 0; i < effectiveTrace.size(); i++)
						{
							report << " " << std::to_string(i) << "): " << effectiveTrace[i] << "\n";
						}
						// Effective Stack Trace - END

						// Original Stack Trace - BEGIN
						report << "[ Original Stack Trace ]: " << "\n";

						for (uint64_t i = 0; i < originalTrace.size(); i++)
						{
							report << " " << std::to_string(i) << "): " << originalTrace[i] << "\n";
						}
						// Original Stack Trace - END

						// Stack Traces - END

						// File Log - END

						// Terminal Log - BEGIN
						std::string discrepancyType = level1Discrepancy ? "Level 1 (Result Status)" : "Level 2 (Error Message)";
						tools->writeLine(tools->getColoredString("Excused a " + discrepancyType + " discrepancy", eColor::orange) + " when processing block: " + tools->base58CheckEncode(proposal->getID())
							+ " transaction: " + tools->base58CheckEncode(transactionsToBeProcessed[i]->getHash()));

						// Terminal Log - END

						// Inconsistencies' Logging - END

					}
					//Checkpoints' Support - END
					else {
						//inconsistent result and block proposal is not covered by a Checkpoint.
						if (level1Discrepancy)
						{
							tools->writeLine("Level 1 Discrepancy (Result Status) when processing block: " + tools->base58CheckEncode(proposal->getID())
								+ " transaction:" + tools->base58CheckEncode(transactionsToBeProcessed[i]->getHash()) + " Previously: " + blockReceipt.translateStatus() +
								" Now: " + effectiveReceipt.translateStatus());
						}
						else if (level2Discrepancy)
						{
							tools->writeLine("Level 2 Discrepancy (Error Message) when processing block: " + tools->base58CheckEncode(proposal->getID())
								+ " transaction:" + tools->base58CheckEncode(transactionsToBeProcessed[i]->getHash()) + "\n Previously: " + originalError +
								"\n Now: " + effectiveError);
						}

					}
				}
				// Consensus Discrepancy Detection - END

				// Cross Hard Fork compatibilitu Checks - END
			}
			// Block Verification - END

			// Post-execution breakpoint processing - BEGIN
			if (doProcessBreakpoints)
			{
				processBreakpoints(*transactionsToBeProcessed[i], receiptID, proposal ? proposal->getHeader()->getKeyHeight() : 0,
					eBreakpointState::postExecution,
					std::make_shared<CReceipt>(effectiveReceipt),//  receipt contains the current receipt; blockReceipt contains receipt if transaction already part of a block
					proposal
				);
			}

			// Post-execution breakpoint processing - END

			// TX Authenticated and On-Chain - BEGIN
			
		
			if (std::memcmp(effectiveReceipt.getVerifiableID().data(), transID.data(), 32) != 0)
				return false; // block is invalid

			//Impose processing fees, increase nonce - BEGIN
			//CRITICAL for any TX affecting the state-machine
			//1)were nonce not incremented we would be opening up for double spend attacks
			//2) were fees not deducted we would be wasting resources (even for invalid transactions fees need to be deducted)
			//is the ID is not confirmed the TX wouldn't be processed at all and no mark in the state machine would be left of it.
			if (!effectiveReceipt.HotProperties.getDoNotIncludeInChain() && confirmedDomainID.size() > 0)
			{// TXs ending up in chain - BEGIN

				// INVALID TX - BEGIN (Part 1)
				/*
				* [ ORDER IMPORTANT ]
				* - Step Back
				* - increment Nonce
				* - deduct fees
				*/



				if (effectiveReceipt.getResult() != valid) // [ IMPORTANT ]: this is GridScript code execution result.
				{															   // [ Rationale ]: GridScript code execution always is ATOMIC (either everything executed or nothing).

					// [ Rationale ]: whatever the state, if the code bundle was authenticated and we know, as it's being stored - the Nonce NEEDS to be incremented.

					// BEFORE NONCE is INCREMENTED - BEGIN
					// holds only for authenticated code bundles.


					// CVE_805 - do NOT perform ACID rollback if transaction was forced to fail.
					//                  [ Rationale ]: no code execution actually took place.
					//									Instead, as part of processTransaction() we associated failure meta-data entries with source account, before omiting code execution
					//									and returning Block Receipt as effective Receipt. This ensures we charge the former amount of ERG (even though code execution was skipped).
					//									Since we want meta-data to prevail, a rollback is canceled.
					// 
					//         v-----------------------^
					if (!enforceFailure)
					{
						// Roll Back (1/3) - BEGIN


						assertGN(stepBack(1)); // anything done by the transaction is reverted by stepping into the previous Perspective. We just NEED to increment NONCE
						// return false;	//  (if authenticated and to be stored) and impose the transaction processing fees onto the issuer's account. 

					 // Roll Back (1/3)- END
					}

					// BEFORE NONCE is INCREMENTED - END

					if (markAsProcessed)// CVE_805 - isolate support only to rollback mitigation.
					{
						transactionsToBeProcessed[i]->HotProperties.markAsInvalid();
						transactionsToBeProcessed[i]->ColdProperties.markAsInvalid();
					}
				}
				// INVALID TX - END (Part 1)

			   // [ IMPORTANT ]: create the back-up database AFTER the Nonce has been incremented.
			   // [ RATIONALE ]: if anything goes wrong, we want the authenication Ticket (represented by a nonce) to be marked as used up.

				CTrieDB* stateBeforeFees = new CTrieDB(*mFlowStateDB);

				// Flow Backup (1/4) - BEGIN ( Level 1 )
				// [ Rationale ]: so that Flow mechanics can revent to state right before fees were deducted and nonce was incremented.
				pushDB(stateBeforeFees, true);// add a DB containing state before any fees were deducted.
				// IMPORTANT: the database will be popped right after fees are successfully deducted
				// Flow Backup (1/4) - END

				// EXTREME CAUTION - BEGIN
				// [ IMPORTANT ]: nonce needs to be incremented whenever a TX is authenticated and appended into the chain.
				//				  AFTER a roll-back ( the stepBack() method) - in case of an invalid transaction.
				//				  Account needs to be able to cover storage fees for TX to be appended.

				CStateDomain* domain = mStateDomainManager->findByID(confirmedDomainID);

				if (domain == nullptr)
				{
					flowFailedForTX = true;
				}
				/* [ Notice ]:
							   - this happens after  { Roll Back (1/3) } - so that results of an invalid TX are reverted and have no effect.
							   - this happens after { Flow Backup (1/4) } - so if the final fee decution fails, the nonce value in not incremented.
				*
				* */
				//  [ CRITICAL ] - nonce needs to be incremented whenever domain's state changes (be it even the associated meta-data)
				//				   This implies that whenever we roll-back beyong this point - NOTHING can affect the domain.
				if (!domain->incNonce(mInSandbox))//increment the 'Envelope Nonce'
				{
					flowFailedForTX = true;
					effectiveReceipt.HotProperties.markDoNotIncludeInChain();

				}
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[Domain Change] Domain Nonce incremented."
					<< "\n\tDomainID: " << tools->bytesToString(confirmedDomainID)
					<< "\n\tPerspective: " << tools->base58CheckEncode(getPerspective(mInSandbox ? -1 : 0))
					<< std::endl;
				// Debug Logging - END
#endif
				// EXTREME CAUTION - END

				// Thresholds Exceeded - BEGIN
				if (!(mUtilizedBlockSize < CGlobalSecSettings::getMaxDataBlockSize()
					&& mUtilizedERG < mMaxERGUsage))
				{
					penaltyDBNeedsToBeRemoved = true;
#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
					debugLog << "[Threshold Alert] Resource limits exceeded:"
						<< "\n\tBlock size: " << mUtilizedBlockSize << "/" << CGlobalSecSettings::getMaxDataBlockSize()
						<< "\n\tERG usage: " << mUtilizedERG << "/" << mMaxERGUsage
						<< std::endl;
#endif
				}
				// Thresholds Exceeded - END

				// this will be needed ONLY if block size or max fees are exceeded - and we need to revert.
				std::vector<uint8_t> perspective;

				// Account for fees - BEGIN

				// Affecting State Transactions Only - BEGIN
				
				// on Critical Error (flowFailedForTX) - do not attempt ANYTHING.
				if(!flowFailedForTX)
				{
					if (!domain->changeBalanceBy(perspective,
						static_cast<BigSInt>(effectiveReceipt.getERGUsed() * effectiveReceipt.getERGPrice()) * static_cast<BigSInt>(-1), mInSandbox))
					{
						// CVE_806 - Excuse Excessive Post-TX processing fees ( Part 1 ) - BEGIN
						// [ Rationale ]: CVE_804 covered the case for when ERG fees associated with processing of particular GridScript intructions changed.
						//				  Here, on the other hand, we cover the case for when GridScript code internally depletes an account (for instance through 'send' or another account)
						//				  rendering the remaining balance insufficient to to cover for agreed upon (and validated) maximum ERG usage fees.
						//				  Notice that similarly as in Ethereum, Core, before agreeing to process a transaction it checks whether the amount of assets available
						//				  is enough to cover for ERG Limit x ERG Price in fees. Then, during execution GridScript VM constantly monitors ERG usage but the VM
						//				  ERG usage monitoring does NOT corellate changes to account's balance stemming from value transfer APIs.
						//				  
						postTxFeesFailed = true; // fees failed that's a fact <- right now this has no effect. We assume tranasctions which are already on chain should never result in this.
											

						// Block production - BEGIN
						if (!verifyAgainstBlock)
						{
							flowFailedForTX = true; // revert the entire ACID flow. The TX has no effect. We do NOT want to store TX, meta-data, increment nonce, nothing.
							effectiveReceipt.setResult(eTransactionValidationResult::invalidBalance); // in the end that's the final error code
							effectiveReceipt.HotProperties.markDoNotIncludeInChain();
							transactionsToBeProcessed[i]->markAsProcessed();
							transactionsToBeProcessed[i]->HotProperties.markAsInvalid();
							//assertGN(revertTX()); // WARNING: TX to be reverted only through RAII finalizer; othewise it would double revert causing mismatch in counters; if we cannot impose fees of any sort - do not affect the system in ANY way.
							// Do NOT store the TX on-chain.
							// do not increment Nonce, do NOT store meta-data of ANY sort.

							continue; // to another TX if available; do NOT return just yet (there many transactions)

						}// Block production - END
						else
						{// Block Verification

							// non-checkpointed block
							report << "[ Current Final Result ]: " << "Critical; failed to charge fees. Aborting Flow." << "\n";

							if (!proposal->getIsCheckpointed())// <- we cannot abort ACID flow entirely since the TX is already on checkpointed chain. We thus rely on postTxFeesFailed instead.
							{
								flowFailedForTX = true; // CRITICAL: do not increment nonce; do not alter meta-data-nothing; abort everything.
							}
							return false;// we assume the entire block as invalid. If that's the case for existing blocks - we need to introduce a yet another backwards compatibility layer
							// around here.
							
							// down below ( Part 2 Operations)
							/*
								   - if block checkpointed - this is NOT expected to occur.
								   - if block not checkpointed - we reject the entire block - the transaction should not be here to begin with.
							*/
						}
						// CVE_806 - Excuse Excessive Post-TX processing fees ( Part 1 ) - END


					}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				else {
					debugLog << "[Fee Processing] Fee deduction successful:"
						<< "\n\tAmount deducted: " << (effectiveReceipt.getERGUsed() * effectiveReceipt.getERGPrice())
						<< "\n\tRemaining balance: " << domain->getBalance(mInSandbox)
						<< std::endl;
				}
#endif
				// Account for fees - END

					popDB(1);

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[Domain Change] Domain Balance changed. DomainID: "
					<< tools->bytesToString(confirmedDomainID)
					<< ", perspective: "
					<< tools->base58CheckEncode(getPerspective(mInSandbox ? -1 : 0))
					<< std::endl;
				// Debug Logging - END
#endif
				// Log Receipt - BEGIN
				// Retrieve current receipts

				// Local Variables - BEGIN
				std::shared_ptr<CAccessToken> sysToken = CAccessToken::genSysToken();
				std::string receiptsContainerID = CGlobalSecSettings::getReceiptsContainerID();
				eDataType::eDataType dType;
				std::shared_ptr<CSecDescriptor> sysDesc = CSecDescriptor::genSysOnlyDescriptor();
				uint64_t nulledCost = 0;
				std::vector<uint8_t> receiptBytes = domain->loadValueDB(sysToken, receiptsContainerID, dType);
				std::vector<std::vector<uint8_t>> receiptIDs;//32-byte IDs
				// Local Variables - END

				//attempt to decode these (may be empty)
				tools->BERVectorToCPPVector(receiptBytes, receiptIDs);

				// push the current Receipt ID
				receiptIDs.push_back(receiptID);

				// BER (re)encode all receipts
				receiptBytes = tools->BERVector(receiptIDs);

				// save BER encoded receipts
				if (!domain->saveValueDB(sysToken, receiptsContainerID, receiptBytes, eDataType::bytes, perspective, nulledCost, mInSandbox, true, "", "", nullptr, false, false, true, sysDesc))
				{
					flowFailedForTX = true; // unrecoverable error; ABORT everything.
				}

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
				// Debug Logging - BEGIN
				debugLog << "[Domain Change] Domain data saved. DomainID: "
					<< tools->bytesToString(confirmedDomainID)
					<< ", perspective: "
					<< tools->base58CheckEncode(getPerspective(mInSandbox ? -1 : 0))
					<< std::endl;
				// Debug Logging - END
#endif

				// Log Receipt - END
			}

			// Affecting State Transactions Only - END

			


				// Full TX-scope ACID/Flow Reversal - BEGIN
				// [ Rationale] : the following is to revert  ANY changes; including those being imposed after processTransaction(); notice fees are imposed after processTransaction()
				//		      Fees after processTransaction() were introduced to allow for better backwards compatibility when transactions are forced to fail.
				if (flowFailedForTX) // an unrecoverable critical ACID error happened. Note: this is more critical than a FAILED transaction.
				{					 // For a failed transaction we may very well associate meta data with account (down below).
								     // Notice: flowFailedForTX may NEVER be set for a transaction which is already part of the system.
					// assertGN(revertTX());// <= WARNING: TX to be reverted only through RAII finalizer; othewise it would double revert causing mismatch in counters; 
					continue; // in such a case we continue with furher tranasctions immediatedly.
							  // [ Rationale ]: this code package cannot affect the system in ANY way (nonce/logs, nothings).
				}
				// Full TX-scope ACID/Flow Reversal - END


				// Log Failed Transaction's Result - BEGIN -----------------------------------------------------------------------------------------------
				// [ Rationale ]: failed transactions are fully rolled back during ACID (Flow) processing.
				// [ Notice ]: this is only for value transfers. Other GridScript code packages are logged in any case as part of Receipts Meta-Data (accessible through the `receipts` user-mode utility.
				//			   Whether a code package is a transaction is determined during byte-code decompilation and sandboxed interpretation stage.

				if (effectiveReceipt.getResult() != valid && !enforceFailure) // only for tranasctions which have failed and 
					//which were not forced to fail (explicit meta data entry was associated with a state-domain earlier, above).
				{

					// -------- Update State-Domain intrinsic stats - BEGIN  -------- 
					// [ Rationale ]: so that issuers see same results in Terminal, UI dApps and mobile apps even
					// though the transaction was fully rolled back during ACID processing.
					// [ Applicability ]: valid only for value transfers.

					// Local Variables - BEGIN
					std::shared_ptr<CAccessToken> sysToken = CAccessToken::genSysToken();//generate kernel-mode Access Token
					std::shared_ptr<CSecDescriptor> sysDesc = CSecDescriptor::genSysOnlyDescriptor();// generate kernel-mode Security Descriptor
					std::string TXStatsContainerID = CGlobalSecSettings::getTXStatsContainerID();
					eDataType::eDataType dType;
					uint64_t nulledCost = 0;
					std::vector<uint8_t> TXInfoBytes;
					std::vector<std::vector<uint8_t>> TXNibbles;//A tuple of: [FLAGS, REG_RECEIPT_ID, fully qualified address, BigSInt value] byte-vectors
					// Local Variables - END

					// Infer Value Transfer Meta-Data (optional) - BEGIN

					// [ Rationale ]: a GridScript code package MAY not be a value transfer at all. In case it here were we attempt to derive additional higher level information.
					//				  The information will be used to fill meta-data fields more accurately.

					// Source Code Retrieval - BEGIN
					std::string sourceCode = transactionsToBeProcessed[i]->getSourceCode();

					BigInt txValue = 0;
					std::string  recipient;
					std::shared_ptr<CGridScriptCompiler> compiler = mBlockchainManager->getCompiler();
					std::shared_ptr<CSendCommandParsingResult> parsingResult;
					assertGN(compiler, "Compiler not available.");
					bool interpretatinSucceeded = false;

					// Rationale: if cached source code not yet available.
					if (sourceCode.empty()) {
						// Transaction Source Rewrite System - BEGIN
						// Check if this transaction has a source code rewrite entry before decompilation
						bool sourceRewritten = false;

						if (proposal && receiptID.size() > 0)
						{
							uint64_t blockHeight = proposal->getHeader()->getHeight();
							std::string receiptIDBase58 = tools->base58CheckEncode(receiptID);
							std::string replacementSource;

							if (CGlobalSecSettings::getTransactionSourceRewrite(receiptIDBase58, blockHeight, replacementSource))
							{
								// Rewrite found - use replacement source instead of decompiling
								sourceCode = replacementSource;
								sourceRewritten = true;
								transactionsToBeProcessed[i]->setSourceCode(sourceCode); // cache it

								// Log the rewrite event for audit trail
								tools->logEvent(
									"Transaction source rewrite applied (stats): receipt=" + receiptIDBase58 +
									", height=" + std::to_string(blockHeight) +
									", replacement_length=" + std::to_string(replacementSource.length()),
									eLogEntryCategory::VM,
									0
								);
							}
						}

						// If no rewrite was applied, perform standard bytecode decompilation
						if (!sourceRewritten)
						{
							if (compiler->decompile(transactionsToBeProcessed[i]->getCode(), sourceCode))
							{
								transactionsToBeProcessed[i]->setSourceCode(sourceCode);// make decompiled source code available for re-use
							}
						}
						// Transaction Source Rewrite System - END
					}
					// Source Code Retrieval - END

					// Source Code Interpretation
					if (!sourceCode.empty())
					{
						parsingResult = parseSendCommand(sourceCode, proposal->getHeader()->getKeyHeight());
						if (parsingResult && parsingResult->isSuccess())
						{
							interpretatinSucceeded = true;
							txValue = parsingResult->getAmount();
							recipient = parsingResult->getRecipient();
						}
						// Infer Value Transfer Meta-Data (optional) - END

						if (interpretatinSucceeded)// [ Rationale ]: originally also ONLY value transfers make their way into the 'TXStatsContainerID' domain-scope data store.
						{
							// -------- Update Issuer - BEGIN --------

							// Retrieve current TX stats

							TXInfoBytes = domain->loadValueDB(sysToken, TXStatsContainerID, dType);

							//attempt to decode these (may be empty)
							mTools->VarLengthDecodeVector(TXInfoBytes, TXNibbles);

							SE::txStatFlags sf;
							sf.valueTransfer = 1;

							//push flags 
							TXNibbles.push_back(sf.getBytes());  // type of code package
							//push receipt ID
							TXNibbles.push_back(receiptID);
							// push current destination
							TXNibbles.push_back(mTools->stringToBytes(recipient));// destination - get from meta data / decompile?
							// push current value

							BigSInt sValue = ((BigSInt)-1 * (BigSInt)txValue);

							TXNibbles.push_back(mTools->BigSIntToBytes(txValue > 0 ? sValue : 0)); // egress value transfer value

							// BER (re)encode all TX stats
							TXInfoBytes = mTools->VarLengthEncodeVector(TXNibbles, 1);

							std::vector<uint8_t> perspectivePrior = getPerspective();
							std::vector<uint8_t> perspectivePosterior;
							// save BER encoded TX stats
							if (!domain->saveValueDB(sysToken, TXStatsContainerID, TXInfoBytes, eDataType::bytes, perspectivePosterior,
								nulledCost,
								mInSandbox, // <-  [ IMPORTANT ]: enable Sandbox Mode only if NOT comitting.
								true,
								"", "", nullptr, false, false, true, sysDesc))
							{
								assertGN(false, "fatal error");
							}


#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
							if (!mTools->compareByteVectors(perspectivePosterior, perspectivePrior))
							{
								debugLog << mTools->getColoredString("Failed transaction meta data had no Trie effect", eColor::lightPink) << ".\n";
							}
#endif

							// -------- Update Issuer - END  --------
						}

					}

					// -------- Update State-Domain intrinsic stats - END  --------
				}
				// Log Failed Transaction's Result - END  -----------------------------------------------------------------------------------------------

				countOfDomainsAffectingTheBlockchain++;//any transformation of the global state needs to entitle cost

				// Support of Deferred Transactions - BEGIN
				// Source Domain Trigger - BEGIN
				// [ Rationale ]: transactions that were deferred due to a nonce value higher than anticipated.
				//				we now 'untrigger' these to enable for their resumed processing.
				//  [Notice ]: a transaction may be then postponed and trigger enabled all over again should the transaction not be ready for processing as of yet.
				//			 Which would be the case if nonce value associated with account is still too low.

				// BUG #3 Fix: Track this issuer for potential retry of their deferred transactions
				issuersWithSuccessfulTx.insert(confirmedDomainID);

				untriggerTransactions(TriggerType::sourceDomainID, confirmedDomainID);
				// Source Domain Trigger - END
				// Support of Deferred Transactions - END

				// TXs ending up in chain - END
			 }
			 //Impose processing fees, increase nonce - END
			// TX Authenticated and On-Chain - END

		   // INVALID TX - BEGIN (Part 2)

			if (effectiveReceipt.getResult() != valid)
			{// An invalid transaction. We step back in time, mark it as processed, as invalid, include it into the block and generate a receipt.

				if(!deliveryConv)
					  notes = tools->getColoredString("[Result]: ", deliveryConv?eColor::none:eColor::blue) + tools->getColoredString("Invalid Transaction", deliveryConv?eColor::none:eColor::cyborgBlood) + ": " + " Reason: (" + effectiveReceipt.translateStatus() + ")" + (effectiveReceipt.getLog().size() > 0 ? "Top Log Entry:' " + effectiveReceipt.getLog()[0] + "'" : "") + " Receipt:" + tools->base58CheckEncode(effectiveReceipt.getGUID());
				if (deliveryConv)
				{
					deliveryConv->notifyOperationStatus(eOperationStatus::failure, eOperationScope::Consensus, std::vector<uint8_t>(), 0, notes);
				}

				tools->writeLine(notes);

				if (effectiveReceipt.HotProperties.getDoNotIncludeInChain() || confirmedDomainID.size() == 0)
				{
					if (!verifyAgainstBlock)
					{
						tools->writeLine("The transaction " + tools->base58CheckEncode(effectiveReceipt.getVerifiableID()) + " was not processable. Discarding..");

						//reporting - BEGIN
						eTransactionValidationResult vRes = effectiveReceipt.getResult();
						switch (vRes)
						{
						case valid:
							break;
						case invalid:
							break;
						case unknownIssuer:
							incNrOfTransactionsFailesDueToUnknownIssuer();
							break;
						case insufficientERG:
							break;
						case ERGBidTooLow:
							break;
						case pubNotMatch:
							break;
						case noIDToken:
							break;
						case noPublicKey:
							break;
						case invalidSig:
							incNrOfTransactionsFailesDueToInvalidEnvelopeSig();
							break;
						case invalidBalance:
							break;
						case incosistentData:
							break;
						case validNoTrieEffect:
							break;
						case invalidSacrifice:
							break;
						case invalidNonce:
							incNrOfTransactionsFailesDueToInvalidNonce();
							break;
						default:
							break;
						}

						incNrOfTransactionsFailesDueToNotIncludedInBlock();

						//reporting END
					}
					else
					{
						//Checkpoints' Support - BEGIN
						if ((proposal->getIsCheckpointed()))
						{
							/*
								Disclaimer: example of why this could happen:
									1) a TX_N was valid when block was produced and when it was appended to the blockchain, yet still
									2) we introduced a change to how instructions are evaluated (to fix a bug)
									3) that rendered a previously invalid TX_N-1, issued by the very same entity as valid, the accounts nonce value was thus incremented 
									4) now nonce for TX_N does not match current value+1
									Check: nonce value should increment each time TX is appended to the blockchain not only when TX valid.
									*/
							tools->writeLine(tools->getColoredString("Neglecting an inconsistent critical result", eColor::orange) + " when processing block: " + tools->base58CheckEncode(proposal->getID()) + "transaction: " + tools->base58CheckEncode(transactionsToBeProcessed[i]->getHash()));
						}
						//Checkpoints' Support - END
						else {
							return false;//the transaction shouldn't even be here. The block is INVALID.
						}
					}
				}
			}	// INVALID TX - END (Part 2)
			else
			{
				// VALID TX - BEGIN
				if(!deliveryConv)
				notes = tools->getColoredString("[Result]:",deliveryConv?eColor::none:eColor::blue)+ tools->getColoredString("Valid Transaction",deliveryConv?eColor::none:eColor::lightGreen)+": " + tools->base58CheckEncode(effectiveReceipt.getVerifiableID()) + " Receipt: " + tools->base58CheckEncode(effectiveReceipt.getGUID());
				
				if (deliveryConv)
				{
					deliveryConv->notifyOperationStatus(eOperationStatus::success, eOperationScope::Consensus, std::vector<uint8_t>(), 0, notes);
				}

				tools->writeLine(notes);
				// VALID TX - END
			}
		

			//Operations Affecting State-Domains - END

			// On-Chain TX - BEGIN

			if ((!effectiveReceipt.HotProperties.getDoNotIncludeInChain() && addToBlock) || verifyAgainstBlock)
			{
				mUtilizedERG += effectiveReceipt.getERGUsed();//TODO:: ensure to account into reward only DEDUCTABLE erg (omit ERG used by invalid transactions with zero balance)
				//this should be checked prior to transaction processing within processTransaction()
				mTotalGNCFees += effectiveReceipt.getERGUsed() * effectiveReceipt.getERGPrice();
				mUtilizedBlockSize += transactionsToBeProcessed[i]->getPackedSize();
				//validate expected block size
				if (!(mUtilizedBlockSize < CGlobalSecSettings::getMaxDataBlockSize() && mUtilizedERG < mMaxERGUsage))
				{
					if (!verifyAgainstBlock)
					{
						// Block Formation - BEGIN ( Part 1 )
						
						//at the current moment we're doing block formation.
						//Thus, it is not the time for us to decide whether block is valid or not
						//IF the block-size was exceeded, we just gently and silently revert to a previous Perspective
						//and discard the transaction
						//note: we also need to deduct and fees imposed by the transaction from the reported ones.
						//the transaction would be possibly processed within another block
						//note, we need to make sure the transaction is not being left marked as 'processed'; thus; its HotStorage flags are cleared.
						if (mUtilizedBlockSize < CGlobalSecSettings::getMaxDataBlockSize())
						{
							tools->writeLine(tools->getColoredString("Block size exceeded; Reverting.", eColor::cyborgBlood));
						}

						if (mUtilizedERG < mMaxERGUsage)
						{
							tools->writeLine(tools->getColoredString("Block's ERG Limit reached. Reverting.", eColor::cyborgBlood));
						}

						mUtilizedERG -= effectiveReceipt.getERGUsed();
						mTotalGNCFees -= effectiveReceipt.getERGUsed() * effectiveReceipt.getERGPrice();
						mUtilizedBlockSize -= transactionsToBeProcessed[i]->getPackedSize();
						transactionsToBeProcessed[i]->HotProperties.clear();

						// Roll Back (2/3) - BEGIN
						assertGN(stepBack(1));
						// Roll Back (2/3) - END

						penaltyDBNeedsToBeRemoved = false;
						//wasInterDBDeleted = true;
						if (countOfDomainsAffectingTheBlockchain > 0)
							countOfDomainsAffectingTheBlockchain--;
						addToBlock = false;//disable the global flag causing ALL the consecutive transactions to be discarded
						markAsProcessed = false;//so that the lines below do not override the unprocessed flags on the last transaction.
						break; //lets leave the block as it was

						// Block Formation - END ( Part 1 )
					}
					else
					{

						return false;//the transaction shouldn't even be here. the block is INVALID.
					}
				}
			}
			// On-Chain TX - END

				if (penaltyDBNeedsToBeRemoved)
				{
					mInFlowInterDBs.pop_back();
					mInFlowInterPerspectives.pop_back();
				}
				//if (!wasInterDBDeleted)
				//{
				//	mInFlowInterDBs.pop_back();
				//	mInFlowInterPerspectives.pop_back();
				//}
			
				// Block Formation - BEGIN

				if (!verifyAgainstBlock)
				{//we are not verifying - we're forming the block instead.
					if (proposal != NULL && (!effectiveReceipt.HotProperties.getDoNotIncludeInChain() && addToBlock))
					{
						receipts.push_back(verifyAgainstBlock ? blockReceipt : effectiveReceipt);//do NOT add receipt to block IF the transaction itself won't be added
						transactionsToBeProcessed[i]->invalidateNode();
						assertGN(proposal->addTransaction(*transactionsToBeProcessed[i]));

					}
				}
				// Block Formation - END
				
				// Block Verification - BEGIN
				else
				{
					//if we're verifying the block return receipts anyway. Their result will be verified against what is within block already.
					receipts.push_back(verifyAgainstBlock ? blockReceipt : effectiveReceipt);
				}
				// Block Verification - END
			
				// Hot Storage - BEGIN
				if (markAsProcessed)
				{
					// Normal mode: mark transaction as processed in mem-pool
					transactionsToBeProcessed[i]->HotProperties.markAsProcessed();

					if (effectiveReceipt.HotProperties.getDoNotIncludeInChain())
					{
						if (effectiveReceipt.getResult() == eTransactionValidationResult::invalidNonce
							&& transactionsToBeProcessed[i]->HotProperties.isTriggerRequired())
						{
							 // keep the TX within  mem-pool, the associated `nonce` is higher than the one on file.
							 // thus there's a chance for a TX to be processed in the future.
							//  Only upon processing of a valid transaction associated with same account, would the 'trigger' fire
							//  (which implies removal of the flag) and would we attempt to reprocess the TX anew.
						}
						else
						{
							transactionsToBeProcessed[i]->HotProperties.markForDeletion(); // note: it would be deleted from mem pool on current leader ONLY.
																						   // on all other nodes 'context -c getRecentTransactions -mem' would keep reporting expected invalid nonce
																						   // pre-processing result.
						}
					}
				}
				else
				{
					// ============================================================================
					// Phantom Mode Transaction Tracking
					// ============================================================================
					// When markAsProcessed is false (phantom mode), we track the transaction
					// in the phantom-processed map instead of marking it in mem-pool.
					// This simulates the normal processing flow - at this exact point the
					// transaction WOULD have been marked as processed in normal mode.
					// ============================================================================
					if (bm && bm->getIsPhantomLeaderModeEnabled())
					{
						// Mark transaction as phantom-processed (using TX hash for tracking)
						markAsPhantomProcessed(transactionsToBeProcessed[i]->getHash());

						// Add entry to phantom processing report with full textual info
						// Use Receipt ID (getGUID) for report display
						// Always include translateStatus() for human-readable result text
						// Include log entries from receipt for GridScript output, errors, etc.
						addPhantomReportEntry(
							effectiveReceipt.getGUID(),
							effectiveReceipt.getResult(),
							effectiveReceipt.getERGUsed(),
							effectiveReceipt.getERGPrice(),
							effectiveReceipt.translateStatus(),
							effectiveReceipt.getLog());
					}
				}
				// Hot Storage - END

		}

		// ============================================================================
		// BUG #3 Fix: Retry Previously-Deferred Transactions in Same Block
		// ============================================================================
		// When a transaction succeeds, it untriggers deferred transactions from the
		// same issuer. Those transactions were not in our original batch (they had
		// isTriggerRequired=true). Now they're untriggered and should be processed
		// in the same block formation cycle to avoid unnecessary delays.
		// ============================================================================
		if (!verifyAgainstBlock && markAsProcessed && !issuersWithSuccessfulTx.empty() &&
			mUtilizedBlockSize < CGlobalSecSettings::getMaxDataBlockSize() &&
			mUtilizedERG < mMaxERGUsage)
		{
			// Fetch newly-untriggered transactions for issuers that had successful TXs
			std::vector<std::shared_ptr<CTransaction>> retriedTransactions =
				getUnprocessedTransactions(eTransactionSortingAlgorithm::feesHighestFirstNonce, true);

			// Filter to only include transactions from issuers we untriggered
			std::vector<std::shared_ptr<CTransaction>> toRetry;
			for (auto& tx : retriedTransactions) {
				if (issuersWithSuccessfulTx.count(tx->getIssuer()) > 0 &&
					!tx->HotProperties.isMarkedAsProcessed()) {
					toRetry.push_back(tx);
				}
			}

			if (!toRetry.empty()) {
				tools->writeLine(tools->getColoredString(
					"Retrying " + std::to_string(toRetry.size()) +
					" previously-deferred transactions in same block...", eColor::lightGreen));

				// Process retry batch (recursive call with limited scope - no further retries)
				// We pass an empty set for issuersWithSuccessfulTx tracking to prevent infinite recursion
				for (size_t ri = 0; ri < toRetry.size() &&
					mUtilizedBlockSize < CGlobalSecSettings::getMaxDataBlockSize() &&
					mUtilizedERG < mMaxERGUsage; ri++)
				{
					if (toRetry[ri]->HotProperties.isMarkedAsProcessed())
						continue;

					std::vector<uint8_t> retryReceiptID = getReceiptIDForTransaction(toRetry[ri]->getHash(), proposal);
					std::vector<uint8_t> retryConfirmedDomainID;
					uint64_t retrySourceNonce = 0;

					resetCurrentTXDBStackDepth();
					setCurrentFlowObjectID(tools->getReceiptIDForTransaction(eBlockchainMode::TestNet, toRetry[ri]->getHash()));

					CReceipt retryReceipt = processTransaction(
						*toRetry[ri],
						proposal->getHeader()->getKeyHeight(),
						retryConfirmedDomainID,
						retryReceiptID,
						retrySourceNonce,
						nullptr, nullptr, false, false);

					// Handle result similar to main loop
					if (retryReceipt.HotProperties.getDoNotIncludeInChain()) {
						assertGN(revertTX());
					} else {
						assertGN(popCurrentTXDBs());

						// Add to block if successful
						if (addToBlock && retryConfirmedDomainID.size() > 0) {
							assertGN(proposal->addTransaction(*toRetry[ri]));
							proposal->addReceipt(retryReceipt);
							receipts.push_back(retryReceipt);
							countOfDomainsAffectingTheBlockchain++;

							tools->writeLine(tools->getColoredString(
								"[Retry Success]: ", eColor::lightGreen) +
								tools->base58CheckEncode(toRetry[ri]->getHash()));
						}
					}

					setCurrentFlowObjectID(std::vector<uint8_t>());

					// Handle marking based on result
					if (retryReceipt.HotProperties.getDoNotIncludeInChain()) {
						if (retryReceipt.getResult() == eTransactionValidationResult::invalidNonce &&
							toRetry[ri]->HotProperties.isTriggerRequired()) {
							// TX failed with invalidNonce and was deferred again (nonce too high)
							// Do NOT mark as processed - it should be retried when triggered
							// The trigger will be fired when the correct nonce TX succeeds
						}
						else if (retryReceipt.getResult() == eTransactionValidationResult::invalidNonce) {
							// invalidNonce but NOT trigger-required means nonce is stale (too low)
							// Mark as processed and delete - it's a replay/duplicate
							toRetry[ri]->HotProperties.markAsProcessed();
							toRetry[ri]->HotProperties.markForDeletion();
						}
						else {
							// Other failure - mark as processed and delete
							toRetry[ri]->HotProperties.markAsProcessed();
							toRetry[ri]->HotProperties.markForDeletion();
						}
					}
					else {
						// Success - mark as processed (will be in block)
						toRetry[ri]->HotProperties.markAsProcessed();
					}
				}
			}
		}
		// BUG #3 Fix - END


		if (countOfDomainsAffectingTheBlockchain != proposal->getTransactionsCount())
		{
			//Checkpoints' Support - BEGIN
			if ((proposal->getIsCheckpointed()))
			{
				tools->writeLine(tools->getColoredString("Neglecting inconsistent number of affected domains ", eColor::orange) + " in block: " + tools->base58CheckEncode(proposal->getID()));
			}
			else {
				return false; // the block is INVALID
			}
		}


	return true;
	//Operational Logic - END
}

/// <summary>
/// Attempts to process, sort and select verifiables from the memory Pool.
/// Important: Not all verifiables are guaranteed to be processed. (due to the block size limit etc).
/// 
/// Verifiables need to be processes as last objects since they might verify amounts issued by transactions.
/// The coinbase-verifiable needs to be the last verifiable. (since its value is based on the transactions and all the prior verifiables).
/// </summary>
/// <param name="incommingVerifiables"></param>
/// <param name="receipts"></param>
/// <param name="proposal"></param>
bool CTransactionManager::processVerifiables(std::vector<std::shared_ptr<CVerifiable>> &incommingVerifiables, 
	std::vector<CReceipt> &receipts,
	BigInt &rewardValidatedUptilNow,	 // <- stored within of a dedicated Block Header field
	BigInt& rewardIssuedToMinerAfterTAX, // <- stored within of a dedicated Block Header field
	bool markAsProcessed,std::shared_ptr<CBlock> proposal,
	bool addToBlock, // so that we know that block is being prepared - not verified
	bool verifyAgainstBlock )
{

	if (incommingVerifiables.empty())
	{
		return true;
	}

		/*
			IMPORTANT: [ SECURITY ]: Epoch 1 and Epoch 2 rewards are to be already accounted for as part of rewardValidatedUpToNow
					   BEFORE this method is called.

					   [ SECURITY ]:  Utlimately, Key Block Operator rewards are issued based on the rewardValidatedUpToNow value.
									  It accounts for both Epoch rewards and TX processing fees.

			- only key blocks are allowed to have a single Miner Rewards verifiable
			- key blocks can have NO other type of verifiable.
		*/

	std::shared_ptr<CTools> tools = getTools();

#if ENABLE_FLOW_DETAILED_LOGS_TM == 1
	// RAII Logging - BEGIN
	std::stringstream debugLog;
	// Create the RAII logger: it will write `debugLog.str()` upon *every* return.
	CProcessVerifiablesRAII loggerRAII(debugLog, getTools());


	// Record method arguments and current perspective
	debugLog << "[processVerifiables] Called with arguments:\n"
		<< " - incommingVerifiables.size(): " << incommingVerifiables.size() << "\n"
		<< " - markAsProcessed: " << (markAsProcessed ? "true" : "false") << "\n"
		<< " - addToBlock: " << (addToBlock ? "true" : "false") << "\n"
		<< " - verifyAgainstBlock: " << (verifyAgainstBlock ? "true" : "false") << "\n"
		<< "[processVerifiables] Current perspective: "
		<< tools->base58CheckEncode(mFlowStateDB->getPerspective()) << "\n"
		<< std::endl;
	// RAII Logging - END
#endif

	assertGN(getIsInFlow());
	std::vector<std::shared_ptr<CStateDomain>> affectedDomainStates;
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(mFlowProcessingGuardian);
	bool isKeyBlock = proposal->getHeader()->isKeyBlock();
	std::string news = "Processing " + std::to_string(incommingVerifiables.size()) + " verifiables..";
	tools->writeLine(news);

	

	for (int i = 0; i < incommingVerifiables.size() && getRequestedStatusChange() != eManagerStatus::eManagerStatus::stopped; i++)
	{
		if (incommingVerifiables[i]->HotProperties.isMarkedAsProcessed())
			continue;

	
		// Loop Local Variables - BEGIN
		CReceipt blockReceipt(getBlockchainMode());
		bool processed = false;
		std::vector<uint8_t> receiptID;
		CVerifiable v = *incommingVerifiables[i];
		eVerifiableType::eVerifiableType verType = v.getVerifiableType();
		// Loop Local Variables - END

		// SECURITY - BEGIN
		if (isKeyBlock)
		{
			if (proposal->getHeader()->getHeight() == 0)
			{
				if ((verType != eVerifiableType::GenesisRewards && verType != eVerifiableType::minerReward))// yes; Genesis Block contains both
				{
					return false;
				}
			}
			else if (verType != eVerifiableType::minerReward)
			{
				return false; // ILLEGAL verifiable (in the context of a key-block)
			}
		}
		// SECURITY - END
		

		// first check if verifiable is valid
		if (mVerifier->verifyActuateAndArm(v,
			affectedDomainStates,
			proposal, 
			rewardValidatedUptilNow,
			addToBlock
		))
		{
			CReceipt r(getBlockchainMode());

			if (verifyAgainstBlock)
			{
				if (!proposal->getHeader()->getReceiptForVerifiable(incommingVerifiables[i]->getHash(), incommingVerifiables[i]->getVerifiableType(), blockReceipt, false))
					return false;
			
				receiptID = blockReceipt.getGUID();
				
			}
			else
				receiptID = tools->getReceiptIDForVerifiable(getBlockchainMode());

			// process the verifiable
			r = processVerifiable(v, rewardValidatedUptilNow, rewardIssuedToMinerAfterTAX, receiptID, proposal);

			if (verifyAgainstBlock)
			{
				if (r.getResult() != blockReceipt.getResult())
				{
					tools->writeLine("Inconsistent results when processing block: " + tools->base58CheckEncode(proposal->getID()) + "Verifiable: " + tools->base58CheckEncode(incommingVerifiables[i]->getGUID()));
					return false;
				}
			}

			if (r.getResult() != eTransactionValidationResult::valid)
			{
			// Roll Back (3/3)- BEGIN

			 assertGN(stepBack(1));// anything done by the verifiable is reverted by stepping into the previous Perspective. We just NEED TO; 
									// affect the issuer's account with transaction processing fees
			// Roll Back (3/3)- END

				if (markAsProcessed)
				{
					incommingVerifiables[i]->HotProperties.markAsInvalid();
					incommingVerifiables[i]->ColdProperties.markAsInvalid();
				}
			}

			if (verifyAgainstBlock)
				receipts.push_back(blockReceipt);
			else
			{
				// Verification Failed - BEGIN
				if (!verifyAgainstBlock)
				{
					// we're creating new block
					if (r.getResult() == eTransactionValidationResult::valid)
					{
						receipts.push_back(r);//do not add invalid verifiables
					}
					else
						addToBlock = false;
				}
				else
				{
					// we're validating an existing block. Here ever Verifiable needs to be valid.
					return false;
				}
				// Verification Failed - END
			}

		}
		else
		{ 
			// Block Formation - BEGIN
			if (!verifyAgainstBlock)
			{
				tools->logEvent("Validation of Verifiable FAILED.", eLogEntryCategory::VM, 1, eLogEntryType::failure);
				addToBlock = false;
				if (markAsProcessed)
					incommingVerifiables[i]->HotProperties.markAsProcessed();
			}
			// Block Formation -END
			else
			{ // Block Validation - BEGIN
				return false;
			  // Block Validation - END
			}
		}
		if (markAsProcessed)
		incommingVerifiables[i]->HotProperties.markAsProcessed();
	

		if(addToBlock &&proposal != NULL)
	 assertGN(proposal->addVerifiable(*incommingVerifiables[i]));
	}
	return true;
}

void CTransactionManager::processReceipts(std::vector<CReceipt>& receiptsToBeProcessed, std::shared_ptr<CBlock> proposal)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
 assertGN(getIsInFlow());
	tools->writeLine("Processing receipts..");
	for (int i = 0; i < receiptsToBeProcessed.size() && getRequestedStatusChange() != eManagerStatus::eManagerStatus::stopped; i++)
	{
		if (receiptsToBeProcessed[i].HotProperties.isMarkedAsProcessed())
			continue;
		//verify if transaction in a given block realy exists and if it is the same
		//if so then include the receipt in the current block
		//std::shared_ptr<CBlock> block = getBlockchainManager()->getBlockByHash(receiptsToBeProcessed[i]->getBlockHash());
		if (proposal != NULL)
		{
			CTransaction tr;
			CVerifiable ver;
			switch (receiptsToBeProcessed[i].getReceiptType())
			{
			case 0:
				if(proposal->getTransaction(receiptsToBeProcessed[i].getVerifiableID(), tr))
			 assertGN(proposal->addReceipt(receiptsToBeProcessed[i]));
				break;

			case 1 :
				//if(proposal->getVerifiable(receiptsToBeProcessed[i].getVerifiableID(), ver))
				//assert(proposal->addReceiptToHotCache(receiptsToBeProcessed[i]));
				break;
			default:
			 assertGN(proposal->addReceipt(receiptsToBeProcessed[i]));
				return;//should not happen
				break;

			}

		}
		else
		{
			receiptsToBeProcessed[i].HotProperties.markAsInvalid();
		//remvoe receipt block does not exist anymore
		}
		receiptsToBeProcessed[i].HotProperties.markAsProcessed();
		//assert(proposal->addReceiptToHotCache(receiptsToBeProcessed[i],false));
	}
}

void CTransactionManager::cleanMemPool()
{

	sync::SynchronizedLocker lockDBs(mFlowStateDB, mLiveStateDB);
	std::lock_guard<std::recursive_mutex> lockA(mFlowGuardian);
	std::lock_guard lockDBF(mDataBlockFormationGuardian);

	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	tools->writeLine("Initiating Mem-Pool clean-up");
	size_t currentTime = tools->getTime();
	size_t timeDiff = 0; 
	static const uint64_t PRE_VALIDATION_INTERVAL = 5;  // seconds
	CSearchResults::ResultData txMeta;
	size_t removedCount=0;
	std::shared_ptr<CTransaction> txS;
	std::shared_ptr<CTransactionDesc> transDesc;
	uint64_t currentHeight = mBlockchainManager->getCachedHeight();
	std::shared_ptr<CTXInfo> txInfo;
	std::vector<uint8_t> receiptID;
	std::shared_ptr <CTransactionDesc> desc;
	std::shared_ptr<CReceipt> rec;
	uint64_t blockHeight = 0;
	for (int i = 0; i < mMemPool.size(); i++)
	{
		// Refresh Variables - BEGIN
		blockHeight = 0;
		txInfo = nullptr;
		rec = nullptr;
		desc = nullptr;
		// Refresh Variables - END

		if (mMemPool[i]->HotProperties.isMarkedForDeletion())
		{
			deleteMemPoolObject(i);
			continue;

		}
		if (mMemPool[i]->HotProperties.isMarkedAsInvalid())
		{
			deleteMemPoolObject(i);
			removedCount++;
			continue;
		}
		
		switch (mMemPool[i]->getType())
		{ 
		case eNodeType::leafNode:
			switch (mMemPool[i]->getSubType())
			{
			case eNodeSubType::transaction://Transaction

				// Get Receipt ID - BEGIN
				receiptID = mTools->getReceiptIDForTransaction(
					getBlockchainMode(),
					std::static_pointer_cast<CTransaction>(mMemPool[i])->getHash()
				);
				// Get Receipt ID - END

				// Chain Status Check - BEGIN
				// [ Rationale ]: this check is needed if TX was processed by another node.


				// Eliminate on-chain TXs - BEGIN
				txInfo = mBlockchainManager->getTransactionInfoByReceiptID(receiptID);
				
				if (txInfo)
				{
					rec = txInfo->getReceipt();
					desc = txInfo->getDescription();
				}

				if (rec)
				{
				     blockHeight = rec->getBlockHeight();

					// Check if transaction is deep enough in chain to be removed from mempool
					if (currentHeight && blockHeight &&
						currentHeight > blockHeight &&
						(currentHeight - blockHeight) > 2)
					{
						deleteMemPoolObject(i); // Remove from mem pool as tx is confirmed deeply enough
						removedCount++;
						continue;
					}
					else
					{
						// Keep tx in mempool temporarily in case of chain reorganization
						continue;
					}

					// Eliminate on-chain TXs - END
				}
				else
				{
					// IMPORTANT: the below creates a Shrodinger's Effect. Proper synchronization needed.
					//		
					//           
					//mMemPool[i]->HotProperties.markAsProcessed(false); // if transaction was not processed  make sure it is not marked as such.
					// [ Warning ]: during a brief period after a data block was formed and before the block was processed there's a Schrodinger's Cat effect.
					//				During this time period the transaction is both processed and unprocessed.
					//				It gets marked as processed in mem-pool right after successfull block formation.
					//				We may thus want to synchronize, meaning block memory pool cleanup maneuvers during any [ Data Block Produced X ] ->  [ Data Block X Processed] sequence. 
					//				todo: mLocalBlockInPipeline of CBlockchainManager was introduced for this very purpose. Not yet effective, further research needed.
					//				notice that there are many edge case scenarios - pushd local block may never get to be processed, the mem pool might be wiped clean etc.
				}
				// Chain Status Check - END

				// Metadata Retrieval/Generation - BEGIN
				txMeta = mBlockchainManager->getMemPoolTXMeta(receiptID);
				txS = std::make_shared<CTransaction>(*std::static_pointer_cast<CTransaction>(mMemPool[i]));  // Create copy after checks

				if (txMeta.isNull()) {
					// Generate new metadata
					txMeta = mBlockchainManager->createTransactionDescription(
						txS,
						nullptr,  // No receipt for mempool transactions
						0,        // No confirmation timestamp
						true,     // Generate detailed metadata
						nullptr   // No block header
					);

					// Double-check on-chain status before caching
					if (!txMeta.isNull() && !mBlockchainManager->getTransactionInfoByReceiptID(receiptID)) {
						mBlockchainManager->addMemPoolTXMeta(receiptID, txMeta);
					}
				}
				// Metadata Retrieval/Generation - END

				// Transaction Meta-Data Collection - BEGIN
				if (!txMeta.isNull()) {
					// Extract CTransactionDesc from ResultData variant
					if (std::holds_alternative<std::shared_ptr<CTransactionDesc>>(txMeta)) {
						transDesc = std::get<std::shared_ptr<CTransactionDesc>>(txMeta);
					}
				}
				// Transaction Meta-Data Collection - END

				// Pre-Validation Update - BEGIN
		

				if (transDesc) {
					uint64_t now = std::time(0);
					uint64_t lastValidation = transDesc->getLastPreValidation();

					// Check if pre-validation needed (handle both timeout and never-validated case)
					if (lastValidation == 0 || now >= lastValidation + PRE_VALIDATION_INTERVAL) {
						// Safe from overflow since we checked lastValidation == 0

						// the State DB might be in use during block formation
						eTransactionValidationResult preValResult = preValidateTransaction(txS, mBlockchainManager->getCachedHeight(true), transDesc);
						transDesc->setLastPreValidationResult(preValResult);

						// Mark terminally invalid transactions for removal from mem-pool
						// Terminally invalid = no chance of ever being processed (bytecode corruption, invalid signature, etc.)
						if (preValResult == eTransactionValidationResult::invalidBytecode ||
							preValResult == eTransactionValidationResult::invalidSig)
						{
							mMemPool[i]->HotProperties.markAsInvalid();
							mTools->logEvent("Transaction marked as terminally invalid (pre-validation): " +
								mTools->base58CheckEncode(txS->getHash()) + " - reason: " +
								(preValResult == eTransactionValidationResult::invalidBytecode ? "invalid bytecode" : "invalid signature"),
								eLogEntryCategory::VM, 0, eLogEntryType::warning);
						}
					}
				}
				// Pre-Validation Update - END

				timeDiff = currentTime - std::static_pointer_cast<CTransaction>(mMemPool[i])->getReceivedAt();
				if (mMemPool[i]->HotProperties.isMarkedAsInvalid())
				{
					if (timeDiff > 150) //5 minutes
					{
						deleteMemPoolObject(i);
						removedCount++;
						continue;
					}
					break;
				}
				if (!mMemPool[i]->HotProperties.isMarkedAsProcessed())
				{
					if (timeDiff > 86400) //24 hours
					{
						deleteMemPoolObject(i);
						removedCount++;
						continue;
					}
					
				}
				else
				{
					if (timeDiff > 1800) //30 minutes for already processed transactions
					{
						deleteMemPoolObject(i);
						removedCount++;
						continue;
					}
				}
				break;
			case eNodeSubType::verifiable://Verifibale
				timeDiff = currentTime - std::static_pointer_cast<CVerifiable>(mMemPool[i])->getReceivedAt();
				if (mMemPool[i]->HotProperties.isMarkedAsInvalid())
				{
					if (timeDiff > 150) //5 minutes
					{
						deleteMemPoolObject(i);
						removedCount++;
						continue;
					}
					break;
				}

				if (!mMemPool[i]->HotProperties.isMarkedAsProcessed())
				{
					if (timeDiff > 86400) //24 hours
					{
						deleteMemPoolObject(i);
						removedCount++;
						continue;
					}
				}
				else
				{
					if (timeDiff > 1800) //30 minutes for already processed verifiables
					{
						deleteMemPoolObject(i);
						removedCount++;
						continue;
					}
				}
				break;
			default:
				deleteMemPoolObject(i);//shouldnt be here
				removedCount++;
				break;
			}
			break;
		default:
			deleteMemPoolObject(i);//shouldnt be here
			removedCount++;
			}
			
		}
	if (removedCount > 0)
	{
		tools->writeLine(std::to_string(removedCount) + " objects were removed from MemPool during clean-up");
	}

}

void CTransactionManager::deleteMemPoolObject(int &i)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	switch (mMemPool[i]->getSubType())
	{
	case eNodeSubType::transaction://transaction
		tools->writeLine("Freeing transaction " + tools->base58CheckEncode(std::static_pointer_cast<CTransaction>(mMemPool[i])->getHash()) + " from MemPool");
		break;
	case eNodeSubType::verifiable://verifiable
		tools->writeLine("Freeing verifiable " + tools->base58CheckEncode(std::static_pointer_cast<CVerifiable>(mMemPool[i])->getHash()) + " from MemPool");
		break;
	default:
		tools->writeLine("Removing object of an unknown type from MemPool. hash: ( " + tools->base58CheckEncode(mCryptoFactory->getSHA2_256Vec(mMemPool[i]->getPackedData()))+ ")");
		break;
	}
	mMemPool.erase(mMemPool.begin() + i);
	if(i>0)
	i--;
}



/// <summary>
/// Generates a simple transaction from source State Domain to Destination State Domain.
///
/// </summary>
/// <param name="ammount"></param>
/// <param name="source"></param>
/// <param name="destination"></param>
/// <param name="sig">signature can be empty for a issuing reward to miners. such transaction needs to be the first and only one in a block.</param>
/// <returns></returns>
CTransaction CTransactionManager::genSimpleTransaction(size_t ammount, std::vector<uint8_t> source, std::vector<uint8_t> destination, std::vector<uint8_t> sig)
{
	return CTransaction();
}


std::vector<uint8_t> CTransactionManager::getForcedMiningPerspective()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mNextMiningPerspective;
}
bool CTransactionManager::forceMiningPerspectiveOfNextBlock(std::vector<uint8_t> ID)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (ID.size() != 32)
		return false;
	mWasForcedPerspectiveProcessed = false;
	mNextMiningPerspective = ID;
	return true;
}

bool CTransactionManager::forceOneTimeParentMiningBlockID(std::vector<uint8_t> ID)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mForcedMiningParentBlockID = ID;
	return true;
}

std::shared_ptr<SE::CScriptEngine>  CTransactionManager::getScriptEngine()
{
	return mScriptEngine;
}

CTrieDB * CTransactionManager::getFlowDB()
{
	std::lock_guard<std::recursive_mutex> lock(mFlowStateDBGuardian);
	return mFlowStateDB;
}

CTrieDB * CTransactionManager::getLiveDB()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mLiveStateDB;
}

std::shared_ptr<CWorkManager> CTransactionManager::getWorkManager()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mWorkManager;
}

std::shared_ptr<CStateDomainManager> CTransactionManager::getStateDomainManager()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mStateDomainManager;
}


std::vector<uint8_t> CTransactionManager::getPerspective(int flowPerspective)
{

	bool wantsFlowPerspective = false;
	if (flowPerspective == -1)
		wantsFlowPerspective = mInFlow;
	else
		wantsFlowPerspective = (flowPerspective > 0) ? true : false;

	if (wantsFlowPerspective)
	{
		std::lock_guard<std::recursive_mutex> lock(mFlowStateDBGuardian);
		std::lock_guard<ExclusiveWorkerMutex> lock3(mFlowStateDB->mGuardian);
		return mFlowStateDB->getPerspective();
	}
	else {
		std::lock_guard<std::recursive_mutex> lock2(mLiveStateDBGuardian);
		std::lock_guard<ExclusiveWorkerMutex> lock3(mLiveStateDB->mGuardian);
		return mLiveStateDB->getPerspective();
	}
}

bool CTransactionManager::setPerspective(std::vector<uint8_t> perspectiveID)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock1(mCurrentDBGuardian);
	std::lock_guard<ExclusiveWorkerMutex> lock2(mCurrentStateDB->mGuardian);
	
	if (!mStateDomainManager->setPerspective(perspectiveID))
		return false;
	if (!mCurrentStateDB->setPerspective(perspectiveID))
		return false;
	return true;
}

/// <summary>
/// Attempts to find a transaction with a given ID within the MemPool.
/// </summary>
/// <returns></returns>
std::shared_ptr<CTrieNode> CTransactionManager::findMemPoolObjectByID(std::vector<uint8_t> id,  eObjectSubType::eObjectSubType objectType)
{
	//std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(mMemPoolGuardian);
	std::shared_ptr<CTransaction> t;
	std::shared_ptr<CVerifiable> v;
	if (id.size() != 32)
		return t;

	std::vector<uint8_t> foundID;
	for (int i = 0; i < mMemPool.size(); i++)
	{

		switch (mMemPool[i]->getType())
		{
		case eNodeType::leafNode:
			switch (mMemPool[i]->getSubType())
			{
			case eObjectSubType::eObjectSubType::Transaction://Transaction
				if (objectType != eObjectSubType::eObjectSubType::Transaction)
					break;
				t = std::static_pointer_cast<CTransaction>(mMemPool[i]);
				foundID = t->getHash();
				if (id.size() == 32)
				{
					if (std::memcmp(foundID.data(), id.data(), 32) == 0)
						return t;
				}

				break;
			case eObjectSubType::eObjectSubType::Verifiable://Transaction
				if (objectType != eObjectSubType::eObjectSubType::Verifiable)
					break;

				v = std::static_pointer_cast<CVerifiable>(mMemPool[i]);
				foundID = v->getHash();
				if (id.size() == 32)
				{
					if (std::memcmp(foundID.data(), id.data(), 32) == 0)
						return v;
				}
			default:

				break;
			}
			break;
		default:
			break;

		}
	}

	
	return nullptr;
}

std::shared_ptr<CTransaction> CTransactionManager::findTransactionByID(std::vector<uint8_t> id)
{
	return std::static_pointer_cast<CTransaction>(findMemPoolObjectByID(id, eObjectSubType::eObjectSubType::Transaction));
}

std::shared_ptr<CVerifiable> CTransactionManager::findVerifiableByID(std::vector<uint8_t> id)
{
	return std::static_pointer_cast<CVerifiable>(findMemPoolObjectByID(id, eObjectSubType::eObjectSubType::Verifiable));
}



/// <summary>
/// Registers an object within the Mem-Pool.
/// </summary>
/// <param name="object"></param>
/// <returns></returns>
bool CTransactionManager::addToMemPool(std::shared_ptr<CTrieNode> object)
{
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);


	if (mMemPool.size() > mGlobalSecSettings->getMaxMempoolSize())
		return false;

	// Validate type before adding
	if (object->getType() != eNodeType::leafNode ||
		(object->getSubType() != eNodeSubType::transaction &&
			object->getSubType() != eNodeSubType::verifiable&&
			object->getSubType() != eNodeSubType::receipt)) {
		return false;
	}

	//check if object not present yet
	std::vector<uint8_t> hash = object->getHash();

	if(mMemPool.size()> mGlobalSecSettings->getMaxMempoolSize())
	return false;
	mMemPool.push_back(object);
	return true;
}

bool CTransactionManager::isObjectInMemPool(std::shared_ptr<CTrieNode> object)
{
	std::shared_ptr<CTools> tools = getTools();
	for (uint64_t i = 0; i < mMemPool.size(); i++)//quite naive;todo: think of improvements; the hash is cached at each object which is at least that good.
	{
		if (tools->compareByteVectors(mMemPool[i]->getHash(), object->getHash()))
			return true;//object already there
	}
	return false;
}

void CTransactionManager::setBlockFormationStatus(bool keyBlock, eBlockFormationStatus status)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	if(!keyBlock)
	mRegBlockFormationStatus = status;
	else
		mKeyBlockFormationStatus = status;
}




/// <summary>
/// gets an average ERGBid based on transactions within the mempool.
/// </summary>
/// <returns></returns>
BigInt CTransactionManager::getAverageERGBid()
{
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	BigInt total=0;
	if (mMemPool.size() == 0)
		return 1;
	for (int i = 0; i < mMemPool.size(); i++)
	{
		if (mMemPool[i]->getType() != 3)
			continue;
		switch (mMemPool[i]->getSubType())
		{
		case 2:
			total += std::static_pointer_cast<CTransaction>(mMemPool[i])->getErgPrice();
			break;
		case 4:
			total += std::static_pointer_cast<CVerifiable>(mMemPool[i])->getErgPrice();
			break;
		default:
			break;
		}
		
	}
	if(total == 0)
		return 1;
	return static_cast<BigInt>(static_cast<BigFloat>(total) / static_cast<BigFloat>(mMemPool.size()));
}

/// <summary>
/// Useful during tests. For instance, after using a forced perspective through forceMiningPerspectiveOfNextBlock
/// if mUpdatePerspectiveAfterBlockProcessed is not used; empty blocks would be generated also on the forced alternative path.
/// </summary>
/// <param name="doIt"></param>
void CTransactionManager::doPerspectiveSync(bool doIt)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mUpdatePerspectiveAfterBlockProcessed = doIt;
}

bool  CTransactionManager::getDoPerspectiveSync()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mUpdatePerspectiveAfterBlockProcessed;
}

bool CTransactionManager::addTransactionToFlow(std::shared_ptr<CTransaction> trans)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!getIsInFlow()) return false;
	mInFlowTransactions.push_back(trans);
		return true;
}

bool CTransactionManager::addVerifiableToFlow(std::shared_ptr<CVerifiable>  ver)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (!getIsInFlow()) return false;
	mInFlowVerifiables.push_back(ver);
	return true;
}
/// <summary>
/// Untriggers all transactions in the memory pool based on the specified trigger type and trigger ID.
/// </summary>
/// <param name="triggerType">The type of the trigger to be unchecked, defined in the TriggerTypeNamespace.</param>
/// <param name="triggerID">The ID of the trigger, typically representing a domain or account address.</param>
/// <remarks>
/// This function iterates through all transactions in the memory pool and untriggers those that match the specified trigger type and ID.
/// </remarks>
void CTransactionManager::untriggerTransactions(TriggerType triggerType, const std::vector<uint8_t>& triggerID) {
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	std::shared_ptr<CTools> tools = getTools();

	// Operational Logic - BEGIN


	for (int i = 0; i < mMemPool.size(); i++)
	{
		if (!(mMemPool[i]->getType() == 3 && mMemPool[i]->getSubType() == 2))
			continue;

		if (mMemPool[i]->HotProperties.isTriggerRequired()==false)
		{// if trigger is required, omit the object from regular processing.
			continue;
		}

		if (static_cast<TriggerType>(std::static_pointer_cast<CTransaction>(mMemPool[i])->HotProperties.getTriggerType()) == triggerType &&
			tools->compareByteVectors(std::static_pointer_cast<CTransaction>(mMemPool[i])->getIssuer(), triggerID)) {

			auto tx = std::static_pointer_cast<CTransaction>(mMemPool[i]);
			tx->HotProperties.markRequiresTrigger(false);

			// CRITICAL FIX: Also clear the processed flag so the TX can be picked up again
			// Without this, the TX would be stuck forever because getUnprocessedTransactions()
			// filters out transactions with isMarkedAsProcessed() == true
			if (tx->HotProperties.isMarkedAsProcessed()) {
				tx->HotProperties.markAsProcessed(false);
			}
		}
	}
	// Operational Logic - END
}


/// <summary>
/// Retrieves a list of unprocessed transactions from the memory pool, applying various filters and sorting mechanisms.
/// </summary>
/// <param name="alg">Specifies the sorting algorithm to be used for ordering the transactions. This parameter accepts values from the eTransactionSortingAlgorithm enumeration, which includes feeHighestFirst, recentFirst, and feesHighestFirstNonce.</param>
/// <param name="ommitTriggering">A boolean flag that, when set to true, excludes transactions requiring external triggers from the returned list.</param>
/// <param name="triggerID">A vector of bytes representing the trigger identifier. If provided, the function returns only those transactions that require a trigger and originate from the state-domain associated with this trigger ID.</param>
/// <returns>Returns a vector of shared pointers to CTransaction objects, representing the sorted and filtered unprocessed transactions in the memory pool.</returns>
/// <remarks>
/// The function performs the following operations:
/// 1. Filters transactions based on their types, subtypes, trigger requirements, and trigger ID matching.
/// 2. Sorts transactions based on the specified algorithm:
///    - feeHighestFirst: Sorts transactions based on the transaction fee in descending order.
///    - recentFirst: Sorts transactions based on the timestamp in descending order.
///    - feesHighestFirstNonce: Performs a two-stage sort where it first sorts transactions by fee in descending order. Then, for transactions with the same fee and originating from the same account, it sorts them by nonce in ascending order.
/// The function ensures thread safety by locking the memory pool during execution.
/// </remarks>
std::vector<std::shared_ptr<CTransaction>> CTransactionManager::getUnprocessedTransactions(eTransactionSortingAlgorithm alg,
	bool ommitTriggering, const std::vector<uint8_t>& triggerID)
{
	
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	std::vector<std::shared_ptr<CTransaction>> sortedUnprocessed;
	std::shared_ptr<CTools> tools = getTools();

	// Pre-Flight - BEGIN
	for (int i = 0; i < mMemPool.size(); i++)
	{

		if (!(mMemPool[i]->getType() == 3 && mMemPool[i]->getSubType() == 2))
			continue;

		if (ommitTriggering && mMemPool[i]->HotProperties.isTriggerRequired())
		{// if trigger is required, omit the object from regular processing.
			continue;
		}

		if (triggerID.empty() == false && // if we expect a transaction to require a trigger (argument provided)
			(std::static_pointer_cast<CTransaction>(mMemPool[i])->HotProperties.isTriggerRequired()==false ||// current TX does not require a trigger
			tools->compareByteVectors(std::static_pointer_cast<CTransaction>(mMemPool[i])->getIssuer(), triggerID) == false))// or required trigger does not match the provided one.
		{
			// if a trigger is provided (through a trigger identifier - which would be the source state domain address,
			// we would return only transactions 
			// - requiring a pending trigger AND
			// - having originated from the state-domain described to tiggerID.

			// in Layman's Terms: the trigger identifier corresponds to the originating state-domain.
			// [todo]: include support of additional triggers (not only based on source state domain).
			//         For this we would add another field to HotProperties.

			continue; //omit the transaction from further processing.
		}
			 
		
		// Skip transactions that are already processed OR marked as invalid
		// (invalid = terminally invalid, e.g., invalid bytecode, invalid signature)
		if (!mMemPool[i]->HotProperties.isMarkedAsProcessed() &&
			!mMemPool[i]->HotProperties.isMarkedAsInvalid())
		{
			sortedUnprocessed.push_back(std::static_pointer_cast<CTransaction>(mMemPool[i]));
		}
	}

	// Pre-Flight - END
	
	// Operational Logic - BEGIN

	// [Phase 1] sort processable objects by a chosen rating function.

	switch (alg)
	{

		//[Phase 1-A] Global ordering based on highest fee first.
	case feeHighestFirst:
		std::sort(sortedUnprocessed.begin(), sortedUnprocessed.end(),
			[](std::shared_ptr<CTransaction>a, std::shared_ptr<CTransaction>  b) { return a->getErgPrice() > b->getErgPrice(); });
		break;

	case feesHighestFirstNonce:
		// ============================================================================
		// Nonce-Aware Fee-Priority Sorting Algorithm
		// ============================================================================
		// This algorithm ensures:
		// 1. Transactions from the SAME issuer are always ordered by nonce (ascending)
		//    to prevent invalidNonce failures and unnecessary deferrals.
		// 2. Among transactions that are "ready" (have the lowest pending nonce for
		//    their issuer), the one with the highest fee is processed first.
		//
		// Algorithm:
		// - Group transactions by issuer
		// - Sort each issuer's transactions by nonce (ascending)
		// - Iteratively pick the highest-fee transaction among those with the
		//   lowest nonce for their respective issuer
		// ============================================================================
		{
			// Step 1: Group transactions by issuer
			std::map<std::vector<uint8_t>, std::vector<std::shared_ptr<CTransaction>>> issuerGroups;
			for (auto& tx : sortedUnprocessed) {
				issuerGroups[tx->getIssuer()].push_back(tx);
			}

			// Step 2: Sort each issuer's transactions by nonce (ascending)
			for (auto& group : issuerGroups) {
				std::sort(group.second.begin(), group.second.end(),
					[](const std::shared_ptr<CTransaction>& a, const std::shared_ptr<CTransaction>& b) {
						return a->getNonce() < b->getNonce();
					});
			}

			// Step 3: Track current index for each issuer (which TX is next to be considered)
			std::map<std::vector<uint8_t>, size_t> issuerIndex;
			for (auto& group : issuerGroups) {
				issuerIndex[group.first] = 0;
			}

			// Step 4: Build result by always picking highest-fee transaction among
			//         those with the lowest nonce for their respective issuer
			size_t totalTx = sortedUnprocessed.size();
			sortedUnprocessed.clear();
			sortedUnprocessed.reserve(totalTx);

			while (sortedUnprocessed.size() < totalTx) {
				std::shared_ptr<CTransaction> bestTx = nullptr;
				std::vector<uint8_t> bestIssuer;

				// Find the highest-fee transaction among all issuers' next-in-line transactions
				for (auto& group : issuerGroups) {
					size_t idx = issuerIndex[group.first];
					if (idx < group.second.size()) {
						auto& tx = group.second[idx];
						if (bestTx == nullptr || tx->getErgPrice() > bestTx->getErgPrice()) {
							bestTx = tx;
							bestIssuer = group.first;
						}
					}
				}

				if (bestTx) {
					sortedUnprocessed.push_back(bestTx);
					issuerIndex[bestIssuer]++;
				}
				else {
					// Safety: should never happen, but prevent infinite loop
					break;
				}
			}
		}
		break;

	case recentFirst:
		std::sort(sortedUnprocessed.begin(), sortedUnprocessed.end(),
			[](std::shared_ptr<CTransaction> a, std::shared_ptr<CTransaction>  b) { return a->getTime() > b->getTime(); });
		break;

	}
	// [Phase 2] make sure that TXs from same issuer WITH SAME ERG Price are ordered by NONCE.

	/*
	*  Here we use declaration uint64_t CTransaction::getNonce() and uint64_t CTransaction::getErgPrice()
	* 
	* Below we would 
	* 
	* 
	for (int i = 0; i < mMemPool.size(); i++)
	{
		if (!(mMemPool[i]->getType() == 3 && mMemPool[i]->getSubType() == 2))
			continue;
		if (!mMemPool[i]->HotProperties.isMarkedAsProcessed())
		{
			std::static_pointer_cast<CTransaction>(mMemPool[i])->getNonce();
		}
	}
	*/

	return sortedUnprocessed;
	// Operational Logic - END

}

std::vector<std::shared_ptr<CVerifiable>> CTransactionManager::getUnprocessedVerifiables(eTransactionSortingAlgorithm alg)
{
	//each node that processes a verifiable will need to be rewarded.
	//we'll be using the incentivization transmission mechanics do calculate the reward
	//for now, the reward is constant for Proofs-of-Fraud, only these are transmitted through the network.
	//for now, verifiables are sorted by time received and reward for Proof-of-Fraud is constant
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	//getTools()->writeLine("Fetching unprocessed transactions..");
	std::vector<std::shared_ptr<CVerifiable>> sortedUnprocessed;


	for (int i = 0; i < mMemPool.size(); i++)
	{
		if (!(mMemPool[i]->getType() == eNodeType::leafNode && mMemPool[i]->getSubType() == eNodeSubType::verifiable))
			continue;
		if (!mMemPool[i]->HotProperties.isMarkedAsProcessed())
		{
			sortedUnprocessed.push_back(std::static_pointer_cast<CVerifiable>(mMemPool[i]));
		}
	}

	switch (alg)
	{
	case feeHighestFirst:
		std::sort(sortedUnprocessed.begin(), sortedUnprocessed.end(),
			[](std::shared_ptr<CVerifiable>a, std::shared_ptr<CVerifiable>  b) { return a->getErgPrice() < b->getErgPrice(); });
		break;

	case recentFirst:
		std::sort(sortedUnprocessed.begin(), sortedUnprocessed.end(),
			[](std::shared_ptr<CVerifiable> a, std::shared_ptr<CVerifiable>  b) { return a->getReceivedAt() < b->getReceivedAt(); });
		break;

	}

	return sortedUnprocessed;

}


size_t CTransactionManager::getMemPoolSize()
{
	std::lock_guard<std::recursive_mutex> lock(mMemPoolGuardian);
	return mMemPool.size();
}



void CTransactionManager::stop()
{
	{
		std::lock_guard<std::mutex> lockThreads(mThreadManagementMutex);
		mStatusChange = eManagerStatus::eManagerStatus::stopped;

		if (!mController.joinable() && getStatus() != eManagerStatus::eManagerStatus::stopped)
		{
			mController = std::thread(&CTransactionManager::mControllerThreadF, this);
		}
	} // Release lock before potential wait

	if (std::this_thread::get_id() != mController.get_id())
	{
		while (getStatus() != eManagerStatus::eManagerStatus::stopped && getStatus() != eManagerStatus::eManagerStatus::initial)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		{
			std::lock_guard<std::mutex> lockThreads(mThreadManagementMutex);
			if (mController.joinable())
				mController.join();
		}
	}

	getTools()->writeLine("Transaction Manager killed;");
}

void CTransactionManager::pause()
{
	std::lock_guard<std::mutex> lockThreads(mThreadManagementMutex);

	mStatusChange = eManagerStatus::eManagerStatus::paused;

	// FIXED: Only create thread if needed
	if (!mController.joinable() && getStatus() != eManagerStatus::eManagerStatus::paused)
	{
		mController = std::thread(&CTransactionManager::mControllerThreadF, this);
	}

	while (getStatus() != eManagerStatus::eManagerStatus::paused && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		mThreadManagementMutex.unlock(); // Unlock during wait
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		mThreadManagementMutex.lock();
	}
}

void CTransactionManager::resume()
{
    std::lock_guard<std::mutex> lockThreads(mThreadManagementMutex);
    
    mStatusChange = eManagerStatus::eManagerStatus::running;
    
    // FIXED: Only one thread creation check
    if (!mController.joinable() && getStatus() != eManagerStatus::eManagerStatus::running)
    {
        mController = std::thread(&CTransactionManager::mControllerThreadF, this);
    }

    while (getStatus() != eManagerStatus::eManagerStatus::running && getStatus() != eManagerStatus::eManagerStatus::initial)
    {
		mThreadManagementMutex.unlock(); // Unlock during wait
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
		mThreadManagementMutex.lock();
    }
}

eManagerStatus::eManagerStatus CTransactionManager::getStatus()
{
	return mStatus;
}

void CTransactionManager::setStatus(eManagerStatus::eManagerStatus status)
{
	std::string res;
	mStatus = status;
	switch (status)
	{
	case eManagerStatus::eManagerStatus::running:
		res += "Running ";
		if (getDoBlockFormation())
			res += " Forming Key-blocks ";
		if (getDoBlockFormation())
			res += " Forming Data-blocks ";
		getTools()->writeLine(res);
		break;
	case eManagerStatus::eManagerStatus::paused:
		getTools()->writeLine( " is now paused");
		break;
	case eManagerStatus::eManagerStatus::stopped:
		getTools()->writeLine("I'm now stopped");
		break;
	default:
		getTools()->writeLine("I'm nowin an unknown state;/");
		break;
	}
}

void CTransactionManager::requestStatusChange(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	mStatusChange = status;
}

eManagerStatus::eManagerStatus CTransactionManager::getRequestedStatusChange()
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	return mStatusChange;
}






