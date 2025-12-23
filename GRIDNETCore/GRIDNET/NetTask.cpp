#include "NetTask.h"

std::mutex CNetTask::sLastTaskIDNrGuardian;

uint64_t CNetTask::getNewTaskIDNr()
{
	std::lock_guard<std::mutex> lock(sLastTaskIDNrGuardian);
	return ++sLastTaskIDNr;
}

uint64_t CNetTask::getTimeoutAfter()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mProcessingTimeout;
}

void CNetTask::getTimeoutAfter(uint64_t after)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	 mProcessingTimeout = after;
}

/// <summary>
/// If task includes a reference to a remote task then this function returns its ID.
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
uint64_t CNetTask::getRemoteTaskID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mRemoteTaskID ;
}

void CNetTask::setRemoteTaskID(uint64_t taskID)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mRemoteTaskID = taskID;
}

eOperationScope::eOperationScope CNetTask::getOperationScope()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mOperationScope;
}

void CNetTask::setOperationScope(eOperationScope::eOperationScope scope)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	 mOperationScope = scope;
}

std::string CNetTask::getDescription()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return "[Type]:" + CTools::getInstance()->netTaskTypeToString(mType);// +"[Status]:" + CTools::getInstance()->netTaskResultToString(mState)
}

std::shared_ptr<CConversation> CNetTask::getConversation()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mConversation;
}

void CNetTask::setConversation(std::shared_ptr<CConversation> conversation)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mConversation = conversation;
}

void CNetTask::setOrigin(std::shared_ptr<CEndPoint> origin)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mOrigin = origin;
}

std::shared_ptr<CEndPoint> CNetTask::getOrigin()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mOrigin;
}

std::shared_ptr<CNetMsg> CNetTask::getNetMsg()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mNetMsg;
}

void CNetTask::setNetMsg(std::shared_ptr<CNetMsg> msg)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mNetMsg = msg;
}

uint64_t CNetTask::getMetaRequestID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mMetaRequestID;
}

uint64_t CNetTask::getRequiredMetaFieldsToReceive()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mRequiredMetaFieldsToReceive;
}

uint64_t CNetTask::getID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mID;
}

bool CNetTask::addResultDataEntry(std::vector<uint8_t> data)
{
	std::lock_guard<std::mutex> lock(mResultDataEntriesGuardian);
	 mResultDataEntries.push_back(data);
	 return true;
}

std::vector<std::vector<uint8_t>> CNetTask::getResultDataEntries()
{
	std::lock_guard<std::mutex> lock(mResultDataEntriesGuardian);
	return mResultDataEntries;
}

uint64_t CNetTask::sLastTaskIDNr = 0;


CNetTask::CNetTask(eNetTaskType::eNetTaskType type, uint64_t priority, uint64_t metaRequestID, uint64_t reqFieldsCount)
{
	mRequiredMetaFieldsToReceive = reqFieldsCount;
	mMetaRequestID = metaRequestID;
	mID = getNewTaskIDNr();
	mState = eNetTaskState::eNetTaskState::initial;
	mPriority = priority;
	mType = type;
	mTimeCreated = std::time(0);
	mTimeCompleted = 0;
	mOperationScope = eOperationScope::dataTransit;
	mRemoteTaskID = 0;
	mProcessingTimeout = 60;

}
CNetTask::CNetTask(const CNetTask & sibling)
{
	mProcessingTimeout = sibling.mProcessingTimeout;
	mRemoteTaskID = sibling.mRemoteTaskID;
	mMetaRequestID = sibling.mMetaRequestID;
	mState = sibling.mState;
	mID = sibling.mID;
	 mType = sibling.mType;
	mPriority = sibling.mPriority;
	mData = sibling.mData;
	 mID = sibling.mID;
	mTimeCreated = sibling.mTimeCreated;
	 mTimeCompleted = sibling.mTimeCompleted;
	 mConversation = sibling.mConversation;
	 mOperationScope = sibling.mOperationScope;
}

void CNetTask::setTimeout(size_t after)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mProcessingTimeout = after;
}
void CNetTask::setPriority(uint64_t priority)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mPriority = priority;
}

uint64_t CNetTask::getPriority() const
{
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mGuardian));
	return mPriority;
}

eNetTaskType::eNetTaskType CNetTask::getType()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mType;
}

void CNetTask::setData(std::vector<uint8_t> data)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mData = data;
}

std::vector<uint8_t> CNetTask::getData()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mData;
}

size_t CNetTask::getTimeCreated() const
{
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mGuardian));
	return mTimeCreated;
}

size_t CNetTask::getTimeCompleted()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mTimeCompleted;
}

void CNetTask::markAsCompleted()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mTimeCompleted = std::time(0);
}

bool CNetTask::isCompleted()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	if (mTimeCompleted != 0)
		return true;
	else return false;
}

eNetTaskState::eNetTaskState CNetTask::getState()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mState;
}

void CNetTask::setState(eNetTaskState::eNetTaskState state)
{
	std::shared_ptr<CTools> tools = CTools::getInstance();
	
	eColor::eColor stateColor = eColor::none;

	

	switch (state)
	{
	case eNetTaskState::initial:
		break;
	case eNetTaskState::assigned:
		break;
	case eNetTaskState::working:
		stateColor = eColor::eColor::orange;
		break;
	case eNetTaskState::completed:
		stateColor = eColor::eColor::lightGreen;
		markAsCompleted();
		break;
	case eNetTaskState::aborted:
		stateColor = eColor::eColor::cyborgBlood;
		break;
	default:
		break;
	}

	tools->logEvent("NetTask of [Type]:" + tools->netTaskTypeToString(mType) + " switching state to " + tools->getColoredString( tools->netTaskStateToString(state), stateColor), eLogEntryCategory::network, 1);

	std::lock_guard<std::mutex> lock(mGuardian);//do not lock the mutex earlier; used by markAsCompleted() as well.
	mState = state;
}

bool CNetTask::operator<(const CNetTask& other)
{
	uint64_t lp = getPriority();
	uint64_t rp = other.getPriority();
	if (lp != rp)
		return lp < rp;
	else
		return getTimeCreated() > other.getTimeCreated();
}
