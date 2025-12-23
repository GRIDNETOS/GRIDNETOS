#include "CMDExecutor.h"

#include "BlockchainManager.h"
#include "transactionmanager.h"
#include "ScriptEngine.h"
#include "Receipt.h"
#include "BlockchainManager.h"
#include "DTI.h"
#include <memory>


CCMDExecutor::CCMDExecutor(eBlockchainMode::eBlockchainMode mode, std::shared_ptr<CDTI> dti)
{
	mStatusChange = eManagerStatus::eManagerStatus::initial;
	prepareForRealm(mode, dti);
}
void CCMDExecutor::prepareForRealm(eBlockchainMode::eBlockchainMode mode, std::shared_ptr<CDTI>& dti)
{
	//if(dti!=nullptr)
	//dti->writeLine("Preparing for " + CTools::getInstance()->blockchainmodeToString(mode) + " Realm");


	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	mTransactionBegin = 0;
	mBlockchainMode = mode;
	mInitialized = false;
	mCurrentDomainAsAuthenticatedID = false;
	mState = eCMDExecutorState::eCMDExecutorState::GridScriptDebugger;

	mDTI = dti;
	mStatus = eManagerStatus::initial;
	if(mTools==nullptr)
	mTools = std::make_unique<CTools>("Executor", eBlockchainMode::LocalData, eViewState::GridScriptConsole, dti);
	
	//if (mTools == nullptr)
	//	mTools->writeLine("Preparing for " + CTools::getInstance()->blockchainmodeToString(mode) + " Realm");

	if(!mController.joinable())
	mController = std::thread(&CCMDExecutor::mControllerThreadF, this);
}

void CCMDExecutor::initialize(std::shared_ptr<SE::CScriptEngine> scriptEngine)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (mBlockchainMode == eBlockchainMode::LocalData)
	{
		mTools->writeLine(mTools->getColoredString("Command will execute in local sandox until all systems initialize..", eColor::orange));
	}

	mBlockchainManager = CBlockchainManager::getInstance(mBlockchainMode);

	if (scriptEngine == nullptr)
	{//WARNING: THIS cannot be reached for DTI sessions. These have autonomous VMs. main vm cannot be used.

		mScriptEngine = mBlockchainManager->getTerminalTransactionsManager()->getScriptEngine();//could be valid only for admin's local terminal
	//Reason: ScriptEngine needs access to TransactionsManager, the global Terminal Transactions Manager can be accessible only from the admin's terminal

		mScriptEngine->setCmdExecutor(shared_from_this());
		mScriptEngine->setDefaultOutput(eViewState::GridScriptConsole);
	}
	else
		mScriptEngine = scriptEngine;//Note: needs to be a detached and a seperate one for a remote terminal session

	mScriptEngine->setCommitTargetEx(mBlockchainMode);
	SE::vmFlags flags = mScriptEngine->getFlags();
	flags.cmdExecutorAvailable = true;
	mScriptEngine->setFlags(flags);

	mInitialized = true;

}
bool  CCMDExecutor::getIsInitialized()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mInitialized;
}
CCMDExecutor::~CCMDExecutor()
{
	//delete mTools;
}
eCMDExecutorState::eCMDExecutorState CCMDExecutor::getState()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mState;
}

void CCMDExecutor::setState(eCMDExecutorState::eCMDExecutorState state)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	switch (state)
	{
	case eCMDExecutorState::eCMDExecutorState::GridScriptDebugger:
		//mTools->writeLine("Switching to GridScript Debugger");
		break;
	case eCMDExecutorState::eCMDExecutorState::ViewControl:
		//mTools->writeLine("Switching to View Control");
		break;

	}
	mState = state;
}

void CCMDExecutor::setColorOutputEnabled(bool isIt)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mScriptEngine->setColorOutputEnabled(isIt);
}

void CCMDExecutor::setActiveCommand(const std::string& cmd)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mCmd = cmd;
}

std::string CCMDExecutor::getActiveCommand()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mCmd;
}


bool CCMDExecutor::executeInput(std::string input)
{
	
	if (!mOperationsGuardian.tryWaitFree())
	{
		//will be unlocked once command is processed
		return false;//executor is busy right now
	
	}
	else
	{
		setActiveCommand(input);	
		return true;
	}
}

void CCMDExecutor::setCurrentSDAsAuthenticatedID(bool doIt)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mCurrentDomainAsAuthenticatedID = doIt;
}

bool CCMDExecutor::getCurrentSDAsAuthenticatedID()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mCurrentDomainAsAuthenticatedID;
}
void CCMDExecutor::mControllerThreadF()
{
	std::string tName = "CLI Command Executor";
	mTools->SetThreadName(tName.data());
	//mTools->writeLine("CLI Command Executor booting up..");
	setStatus(eManagerStatus::eManagerStatus::running);
	bool wasPaused = false;
	if (mStatus != eManagerStatus::eManagerStatus::running)
		setStatus(eManagerStatus::eManagerStatus::running);

	while (mStatus != eManagerStatus::eManagerStatus::stopped)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		wasPaused = false;

		if (mStatusChange == eManagerStatus::eManagerStatus::paused)
		{
			setStatus(eManagerStatus::eManagerStatus::paused);
			mStatusChange = eManagerStatus::eManagerStatus::initial;

			while (mStatusChange == eManagerStatus::eManagerStatus::initial)
			{
				if (!wasPaused)
				{
					mTools->writeLine("My thread operations were freezed. Halting..");
					wasPaused = true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		std::string cmd = getActiveCommand();
		if (cmd.size() > 0)
		{
			setActiveCommand("");
			uint64_t topMostValue;
			BigInt ERGUsed = 0;

			//std::shared_ptr<CBlock> leader = mBlockchainManager->getLeader(true);// WARNING: retrieval can be extremely slow under a heavy load.
			uint64_t currentKeyHeight = 0;
			//if (leader != nullptr)
			currentKeyHeight = mBlockchainManager->getCachedHeight(true);//leader->getHeader()->getKeyHeight();

			//std::vector<uint8_t> currentSD;
			//mScriptEngine->getRegN(3, currentSD);
			std::shared_ptr<CDTI> dti = getDTI();
			if (mScriptEngine->getIsWaitingForVMMetaData())
			{
				if (dti != nullptr)
				{
					dti->writeLine(mTools->getColoredString("VM busy waiting for data.", eColor::greyWhiteBox));
				}
			}

			if (mBlockchainManager->getIsReady() == false)
			{
				if (dti != nullptr)
				{
					dti->writeLine(mTools->getColoredString("Not ready. Node is bootstrapping.", eColor::orange));
					return;
				}
			}

			if (dti != nullptr)
			{
				dti->setIsInShell(false);//to avoid cursor synchronization with user input-buffer
			}

			//Outside of Shell Processing - BEGIN

			CReceipt r = mScriptEngine->processScript(cmd, std::vector<uint8_t>(), topMostValue, ERGUsed, currentKeyHeight, false, false, std::vector<uint8_t>(), false);

			if (r.getResult() == eTransactionValidationResult::invalid)
			{
				std::string error = "";

				if (r.getLog().size() > 0)

				{
					error = r.getLog()[0];
					mTools->writeLine(mTools->getColoredString(error, eColor::cyborgBlood), true, true, eViewState::eViewState::GridScriptConsole);

				}
				mScriptEngine->clearErrorFlag();

			}
			//Outside of Shell Processing - END

			if (dti != nullptr)
			{
				dti->requestShell();//request back Shell
				setActiveCommand("");
			
				//mGuardian.unlock();//locked already by CCMDExecutor::executeInput()
			}
			if (wasPaused)
			{
				mTools->writeLine("My thread operations are now resumed. Commencing further..");
				mStatus = eManagerStatus::eManagerStatus::running;
			}


			if (mStatusChange == eManagerStatus::eManagerStatus::stopped)
			{
				mStatusChange = eManagerStatus::eManagerStatus::initial;
				mStatus = eManagerStatus::eManagerStatus::stopped;

			}
			mOperationsGuardian.release();
		}
	
	}
	setStatus(eManagerStatus::eManagerStatus::stopped);
}

std::shared_ptr<CDTI> CCMDExecutor::getDTI()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
		return mDTI.lock();
}

void CCMDExecutor::notifyRealmReady(eBlockchainMode::eBlockchainMode realm)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

		switch (realm)
		{
		case eBlockchainMode::LIVE:
			mTools->writeLine(mTools->getColoredString("Live-Net realm is ready.", eColor::toxicGreen));
			break;
		case eBlockchainMode::TestNet:
			mTools->writeLine(mTools->getColoredString("Live-Net realm is ready", eColor::toxicGreen));
			break;
		case eBlockchainMode::LIVESandBox:
			break;
		case eBlockchainMode::TestNetSandBox:
			break;
		case eBlockchainMode::LocalData:
			break;
		case eBlockchainMode::Unknown:
			break;
		default:
			break;
		}

}

/*
The function instructs for a chnage of a realm within which the instructions are to be executed.
Note: The realm will be available ONLY in a sandbox mode. Thus no permanent changes to the data store would be possible.
The CMDExecutor object is only for in-terminal usage.
Typically the function is executed by the VM spwned for terminal purposes, in response to SCT (setCommitTarget command).
*/
void CCMDExecutor::changeRealm(eBlockchainMode::eBlockchainMode realm)
{
	//lock critical sections
	std::lock_guard<std::recursive_mutex> lock(mGuardian);//recursive mutex introduced performance hit, even so we eliminate the very unlikely possibiltiy of anoher function taking ownership
	//once stop() unwinds. Performance hit is acceptable since the mutex is to be locked seldomly and only in reaction to user's final, confirmed instructions.
	//the very CCMDExecutor object is not for intrinsic VM's usage.

	if (mBlockchainMode != realm && mBlockchainMode==eBlockchainMode::LocalData)
	{
		switch (realm)
		{
		case eBlockchainMode::LIVE:
			mTools->writeLine(mTools->getColoredString("Exiting Local Sandbox..", eColor::neonGreen));
			break;
		case eBlockchainMode::TestNet:
			mTools->writeLine(mTools->getColoredString("Exiting Local Sandbox..", eColor::neonGreen));
			break;
		case eBlockchainMode::LIVESandBox:
			break;
		case eBlockchainMode::TestNetSandBox:
			break;
		case eBlockchainMode::LocalData:
			break;
		case eBlockchainMode::Unknown:
			break;
		default:
			break;
		}
	}


	mBlockchainMode = realm;
	//stop all the operations <- there's no need the changeRealm() gets executed by the CCMDExecutor's own Thread 
	//stop();

	//free objects, reset internal state and replace current objects with the new-realm's specific ones
	//Rationale: objects pointed to by smart-pointers would be deallocated on re-assignmnet, the entire inner-state gets reset since
	//prepareForRealm() is (the only thing) executed by the contructor itself.
	std::shared_ptr<CDTI> dti = mDTI.lock();
	prepareForRealm(realm, dti);

	//activate operations anew
	//Rationale: the thread gets spawned anew
	initialize(dti !=nullptr ? dti->getScriptEngine() : nullptr);//for a remote session the scriptengine with a detached Transaction Manager (one to which the DTI has access to already needs to be passed)
	//for local admin session nullptr is passed which casuses the global Terminal script engine to be used
}

void CCMDExecutor::stop()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (getStatus() == eManagerStatus::eManagerStatus::stopped)
		return;
	mStatusChange = eManagerStatus::eManagerStatus::stopped;

	if ( reinterpret_cast<uint64_t>(mController.native_handle()) == 0 && getStatus() != eManagerStatus::eManagerStatus::stopped)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mController = std::thread(&CCMDExecutor::mControllerThreadF, this);
	
	if (mController.get_id() != std::this_thread::get_id())//otherwise its the very thread running this code i.e. the CCMDExecutor attempts to stop itself which is valid
		///when users invokes SCT (setCommitTarget)
	{
		while (getStatus() != eManagerStatus::eManagerStatus::stopped && getStatus() != eManagerStatus::eManagerStatus::initial)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		if (mController.joinable())
			mController.join();
	}
	mTools->writeLine("Command Executor killed;");
}

void CCMDExecutor::pause()
{
}

void CCMDExecutor::resume()
{
}

eManagerStatus::eManagerStatus CCMDExecutor::getStatus()
{
	return mStatus;
}

void CCMDExecutor::setStatus(eManagerStatus::eManagerStatus status)
{
}

void CCMDExecutor::requestStatusChange(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::recursive_mutex> lock(mStatusChangeGuardian);
	mStatusChange = status;
}

eManagerStatus::eManagerStatus CCMDExecutor::getRequestedStatusChange()
{
	std::lock_guard<std::recursive_mutex> lock(mStatusChangeGuardian);
	return mStatusChange;
}
