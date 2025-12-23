#pragma once
#include "stdafx.h"
#include "enums.h"
#include <mutex>
#include "reverseSemaphore.h"
/// <summary>
/// Declares the Command Executor (#)
/// Note: This object is on higher-lever than the GridScript VM.
/// The main objective of CCMDExecutor is to parse and execute commands input by the user.
/// The Command Executor can be in two states defined by eCMDExecutorState enum.
/// Namely:GridScriptDebugger, ViewControl
/// 
/// + GridScriptDebugger 
///		Allows to have fun with the internal integrated GridScript debugger.
/// In this mode a debugger is available user can define variables, memory regions and take use of any commands made 
/// available by GridScript. Note: some GridScript commands are available only locally through the command line.
/// These commands inlude especially the host-managment functionallity, file access functions etc. etc.
/// 
/// Note: all the commands executed while in this mode *DO NOT* affect StateDomains. No commands affecting the Global Consensus
/// will have an permanent effect here. For this a Transaction mode needs to be used with a final Transacion broadcast to the network.
/// user can play with GridScript and test the potential result of his commands on the state of the GridScript VM locally.
/// When user feels ready and confident his instructions to have the intended effect; one can use the  (BT) command to begin TransactionInput.
/// When one enteres the  Transaction Input 'mode', all the consecutive commands would be used to form a Transaction.
/// The Transaction can be then dispatched to the GRIDNET network.
/// 
/// Commands will be then embedded into the transactions IF user decides to commit (using CT command)
/// or the commands will be discarded when Abort-Transaction (AT) command is used.
/// and siptach the code as part of a Transaction to the GrinetNetwork. That transaction would be validated and affect
/// all full nodes in the system.
/// note: Commit-Transaction(CT) implies dispatch of the transaction within the Network.
/// 
///  + ViewControl
/// In this mode no commands are available. One can just perform view-control and app management.


/// 
/// </summary>
#include "IManager.h"
#include "enums.h"

class CTransactionManager;
class CBlockchainManager;
class CCMDExecutor :public  std::enable_shared_from_this<CCMDExecutor>, IManager
{
private:
	std::recursive_mutex mStatusChangeGuardian;//seldomly executed thus recursive may be
	eManagerStatus::eManagerStatus mStatus;
	eManagerStatus::eManagerStatus mStatusChange;
	std::thread mController;

	std::unique_ptr<CTools> mTools;
	std::mutex mFieldsGuardian;
	std::recursive_mutex mGuardian;//seldomly executed thus recursive may be
	eCMDExecutorState::eCMDExecutorState mState;
	std::vector<std::string> mCommandList;// commands input by the user so far 
	uint32_t mTransactionBegin;
	bool getCurrentSDAsAuthenticatedID();
	//indicated the line number within mCommandList at which the current Transaction begins. 0 if no transaction in progress
	void mControllerThreadF();
	ReverseSemaphore mOperationsGuardian;

	eBlockchainMode::eBlockchainMode mBlockchainMode;
	std::shared_ptr<SE::CScriptEngine> mScriptEngine;
	std::shared_ptr<CBlockchainManager> mBlockchainManager;
	std::string mCmd;
	bool mInitialized;
	std::weak_ptr<CDTI> mDTI;
	//Warning: EXTREME caution to be taken with the below.
	//First of, it is used ONLY by a Terminal (for sandbox experience/testing). It is NOT to be used by consensus executor.
	// (right now the latter does not even use CMDExecutor in the first place but falls back
	//to a RAW instance of CScriptEngine.
	bool mCurrentDomainAsAuthenticatedID;
	std::shared_ptr<CDTI> getDTI();
	
	void setActiveCommand(const std::string& cmd="");
	std::string getActiveCommand();
public:
	void notifyRealmReady(eBlockchainMode::eBlockchainMode realm);
	void changeRealm(eBlockchainMode::eBlockchainMode realm);
	CCMDExecutor(eBlockchainMode::eBlockchainMode mode,std::shared_ptr<CDTI> dti=nullptr);
	void prepareForRealm(eBlockchainMode::eBlockchainMode mode, std::shared_ptr<CDTI>& dti);
	void initialize(std::shared_ptr<SE::CScriptEngine> scriptEngine=nullptr);
	bool getIsInitialized();
	~CCMDExecutor();
	eCMDExecutorState::eCMDExecutorState getState();
	void setState(eCMDExecutorState::eCMDExecutorState state);
	void setColorOutputEnabled(bool isIt = true);

	bool executeInput(std::string input);
	void setCurrentSDAsAuthenticatedID(bool doIt=true);
	// Inherited via IManager
	virtual void stop() override;
	virtual void pause() override;
	virtual void resume() override;
	virtual eManagerStatus::eManagerStatus getStatus() override;
	virtual void setStatus(eManagerStatus::eManagerStatus status) override;

	// Inherited via IManager
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status) override;
	virtual eManagerStatus::eManagerStatus getRequestedStatusChange() override;
};