#pragma once
#include "stdafx.h"
#include "enums.h"

/// <summary>
/// Described a task to be accomplished within a conversation.
//The task might portrait an atomic event like sending a transaction,
//in which case the mData field contains serialized Transaction,
//or a NetTask might trigger a sequence of events handled by a conversation.
//the current Task is always available from CConversation through a call to getCurrentTask()
/// </summary>
class CNetMsg;
class CEndPoint;
class CConversation;
class CNetTask
{
private:
	std::shared_ptr<CConversation> mConversation;//the conversation on behalf of which the task has been created
	std::mutex mGuardian;
	eNetTaskState::eNetTaskState mState;
	eNetTaskType::eNetTaskType mType;
	uint64_t mPriority;
	std::vector<uint8_t> mData;
	std::shared_ptr<CNetMsg> mNetMsg;//used when routing
	uint64_t mID;
	uint64_t mRemoteTaskID;//decoupled from mID since the other peer might be using another seed value
	//and we would end up with same IDs for multiple tasks locally - if working on a remotely issued assignment
	size_t mTimeCreated;
	size_t mTimeCompleted;
	void markAsCompleted();
	std::vector<std::vector<uint8_t>> mResultDataEntries;
	static uint64_t sLastTaskIDNr;
	static std::mutex sLastTaskIDNrGuardian;
	std::mutex mResultDataEntriesGuardian;
	static uint64_t getNewTaskIDNr();
	uint64_t mMetaRequestID;
	uint64_t mRequiredMetaFieldsToReceive;
	std::shared_ptr<CEndPoint> mOrigin;//might be IPv4/6 or an abstract websock conversation; used four routing table updates (Route task).
	eOperationScope::eOperationScope mOperationScope;
	uint64_t mProcessingTimeout;

	//uint64_t mTries = 0;
public:
	uint64_t getTimeoutAfter();//seconds
	void getTimeoutAfter(uint64_t after);
	uint64_t getRemoteTaskID();
	void setRemoteTaskID(uint64_t taskID);
	eOperationScope::eOperationScope getOperationScope();
	void setOperationScope(eOperationScope::eOperationScope scope);
	std::string getDescription();
	std::shared_ptr<CConversation> getConversation();
	void setConversation(std::shared_ptr<CConversation> conversation);
	void setOrigin(std::shared_ptr<CEndPoint> origin);
	std::shared_ptr<CEndPoint> getOrigin();

	//uint64_t getTimesTried();
	///void incTimesTried();
	std::shared_ptr<CNetMsg> getNetMsg();

	void setNetMsg(std::shared_ptr<CNetMsg> msg);
	uint64_t getMetaRequestID();
	uint64_t getRequiredMetaFieldsToReceive();
	uint64_t getID();
	bool addResultDataEntry(std::vector<uint8_t> data);
	std::vector<std::vector<uint8_t>> getResultDataEntries();
	CNetTask( eNetTaskType::eNetTaskType type, uint64_t priority = 1,uint64_t metaRequestID=0,  uint64_t reqFieldsCount=0);
	CNetTask(const CNetTask & sibling);
	void setTimeout(size_t after);
	void setPriority(uint64_t priority);
	uint64_t getPriority() const;
	eNetTaskType::eNetTaskType getType();
	void setData(std::vector<uint8_t> data);
	std::vector<uint8_t> getData();
	size_t getTimeCreated() const;
	size_t getTimeCompleted();
	bool isCompleted();
	eNetTaskState::eNetTaskState getState();
	void setState(eNetTaskState::eNetTaskState state);

	bool operator<(const CNetTask& other);

};
struct CCmpNetTasks
{
	bool operator()(const std::shared_ptr<CNetTask> lhs, std::shared_ptr<CNetTask> rhs) const
	{
		if (!lhs || !rhs)
		{
			return  false;
		}
		uint64_t lp = lhs->getPriority();
		uint64_t rp = rhs->getPriority();
		if (lp != rp)
			return lhs->getPriority() < rhs->getPriority();
		else
			return lhs->getID() > rhs->getID();
	}
};


