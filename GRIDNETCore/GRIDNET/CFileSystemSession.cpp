#include "CFileSystemSession.h"
#include "BlockchainManager.h"
#include "conversationState.h"
#include "CGlobalSecSettings.h"
#include "NetTask.h"
#include "NetworkManager.h"
#include "DTIServer.h"
#include "DTI.h"

CFileSystemSession::CFileSystemSession(std::shared_ptr<CNetworkManager> nm, std::vector<uint8_t> DTISessionID)
{
	mID = CTools::getInstance()->genRandomVector(8);
	mFileSystemServer = nm->getFileSystemServer();
	mDTIServer = nm->getDTIServer();
	mBlockchainMode = nm->getBlockchainMode();

	if (DTISessionID.size() > 0)
	{
		std::shared_ptr<CDTI> dti = mDTIServer->getDTIbyID(DTISessionID);
		if (dti != nullptr)
		{
			mDTI = dti;
			mTransactionManager = dti->getTransactionManager();
			mScriptEngine = dti->getScriptEngine();
		}
	}

	if (!(mDTI != nullptr && mScriptEngine != nullptr))
	{
		mTransactionManager = std::make_shared<CTransactionManager>(eTransactionsManagerMode::Terminal, CBlockchainManager::getInstance(mBlockchainMode), nullptr, "", false, mBlockchainMode,
			false, true, false, shared_from_this()); // doBlockFormation=false, createDetachedDB=true, doNOTlockChainGuardian=false, DTI
		mScriptEngine = mTransactionManager->getScriptEngine();
		mScriptEngine->reset();
		mScriptEngine->setAllowLocalSecData(false);//do NOT allow for access to local sec-data store over SSH
	}

}

std::vector<uint8_t> CFileSystemSession::getID()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mID;
}

size_t CFileSystemSession::getLastActivityTimestamp()
{
    return mLastInteractionTimeStamp;
}

bool CFileSystemSession::end(bool waitTillDone)
{
	//setStatu
	mState->setCurrentState(eConversationState::eConversationState::ending);
	while (waitTillDone && mState->getCurrentState() != eConversationState::eConversationState::ended)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	}
	mConversationThread.join();
	return true;
}

std::shared_ptr<CConversationState> CFileSystemSession::getState()
{
	return mState;
}


/// <summary>
/// In contrast with UDT-conversations there's is no lower granularity than the level of File-System tasks.
// i.e. UDT tasks represnt processes which are facilitated using CNetMsgs
/// </summary>
/// <param name="task"></param>
/// <returns></returns>
eDFSTaskProcessingResult::eDFSTaskProcessingResult CFileSystemSession::processFileSystemTask(std::shared_ptr<CNetTask> task)
{
	return eDFSTaskProcessingResult::eDFSTaskProcessingResult();
}

bool CFileSystemSession::addTask(std::shared_ptr<CNetTask> task)
{
	if (task == nullptr)
		return false;
	std::lock_guard<std::mutex> lock(mQueueGuardian);
	mTasks.push(task);
	return true;
}
std::shared_ptr<CNetTask> CFileSystemSession::getCurrentTask(bool deqeue)
{
	std::lock_guard<std::mutex> lock(mQueueGuardian);
	std::shared_ptr<CNetTask> toRet = nullptr;
	if (!mTasks.empty())
	{
		toRet = mTasks.top();
		if (deqeue)
			mTasks.pop();
	}
	return toRet;
}
void CFileSystemSession::dequeTask()
{
	std::lock_guard<std::mutex> lock(mQueueGuardian);
	if (mTasks.empty())
		return;
	mTasks.pop();

}
void CFileSystemSession::FileSystemSessionHandlerThreadF(std::shared_ptr<ix::WebSocket> socket)
{
	bool wasConnecting = false;

	if (socket == nullptr)
		return;

	//Main part of the conversation  - BEGIN
	mBlockchainManager->getTools()->writeLine("Session has begun.", true, true, eViewState::unspecified, "File Session");
	CConversationState state;

	size_t timeout = CGlobalSecSettings::getMaxWebSocketTimeout();
	
	std::vector<uint8_t> buffer;
	buffer.resize(CGlobalSecSettings::getMaxNetworkPackageSize());
	size_t start = mBlockchainManager->getTools()->getTime();

	//create a 'Hello' task
	std::shared_ptr<CNetTask> newTask = std::make_shared<CNetTask>(eNetTaskType::startConversation, 1000);//highest priority 'hello' task gonna be the first one
	addTask(newTask);//enque it
	state.ping();
	std::shared_ptr<CNetTask> currentTask;

	//bool endConversationRequested = false;
	bool communicationErrored = false;

	//tasks are only local, from other parties are received only atomic intents/packages

	size_t interPingTimeDiff = 0;


	while (!communicationErrored && mState->getCurrentState() != eConversationState::eConversationState::ended &&
		interPingTimeDiff < 45)//when the 2nd timeout expires the connection is  shut down forcefuly
		//on 1st timeout we attempt to attempt a gentle negotiated disconnection
	{
		interPingTimeDiff = mBlockchainManager->getTools()->getTime() - state.getLastActivity();
		currentTask = getCurrentTask();//assess the current Task

		if (interPingTimeDiff > 60 && mState->getCurrentState() != eConversationState::ending)
		{
			//attempt to gently end the conversation before a timeout
			mState->setCurrentState(eConversationState::eConversationState::ending);
			newTask = std::make_shared<CNetTask>(eNetTaskType::endConversation, 1000);//assign highest priority
			addTask(newTask);
			state.ping();
		}

		//first - if connecting to another peer then let us process our task first (if it is not empty)
		//if being contacted by another peer -> then process task of the client first

		if (currentTask != nullptr)
		{
			eNetTaskProcessingResult::eNetTaskProcessingResult  result = processFileSystemTask(currentTask, comSocket);
			if (mState->getCurrentState() == eConversationState::ending)
			{
				mState->setCurrentState(eConversationState::ended);
			}

			if (result == eNetTaskProcessingResult::aborted || result == eNetTaskProcessingResult::succedded)
				dequeTask();//precessing of the network task already finished
		}

		//then, let us see what the other peer has to say - both parties should have sent hello Msgs by now

		int rsize = UDT::recvmsg(comSocket, reinterpret_cast<char*>(buffer.data()), buffer.size());

		if (rsize != UDT::ERROR)
		{
			totalNrOfBytesExchanged += rsize;//global counter
			state.ping();
			std::vector<uint8_t> receivedMsg(buffer.begin(), buffer.begin() + rsize);
			CNetMsg msg;

			if (CNetMsg::instantiate(receivedMsg, msg))
			{
				//do what there's to be done with the received msg reply/process with blockchain manager etc.
				if (mState->getCurrentState() != eConversationState::ending)//if conversation is ending we're not interested
					processMsg(msg, comSocket);

				if (msg.getEntityType() == eNetEntType::bye)
					mState->setCurrentState(eConversationState::eConversationState::ended);
			}
			else communicationErrored = true;
		}
		else
		{
			UDT::ERRORINFO errorInfo = UDT::getlasterror();
			const int errCode = errorInfo.getErrorCode();

			if (errCode != CUDTException::ETIMEOUT)
				communicationErrored = true;

			//there was an error in transmission
			//communicationErrored = true;
		}
	}

	UDT::close(comSocket);
	mState->setCurrentState(eConversationState::eConversationState::ended);

	mBlockchainManager->getTools()->writeLine("Conversation has ended.", true, true, eViewState::unspecified, "Conversation");
	//Main part of the conversation  - END
}

void CFileSystemSession::mControllerThreadF()
{
}
