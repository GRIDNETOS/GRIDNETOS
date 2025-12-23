#pragma once
#include "enums.h"

//Consensus Task's Flags - BEGIN
struct ctFlags
{
	bool isCTCompleted : 1;
	bool reserved : 7;

	ctFlags(const ctFlags& sibling) {
		std::memcpy(this, &sibling, sizeof(ctFlags));
	}

	ctFlags()
	{
		std::memset(this, 0, sizeof(ctFlags));
	}

	ctFlags(std::vector<uint8_t> data) {
		if (data.size() >= 1)
		{
			std::memcpy(this, &data[0], sizeof(ctFlags));
		}
	}

	static std::shared_ptr<ctFlags> instantiate(std::vector<uint8_t> data);//alow for nullptr to be returned in case of invalid data the reason behind pointer returned
	std::vector<uint8_t> getPackedData();


};
//Consensus Task's Flags - END

class CConsensusTask
{

public:
	CConsensusTask(std::string description="", eConsensusTaskType::eConsensusTaskType type = eConsensusTaskType::readyForCommit, uint64_t processID = 0, std::vector<uint8_t> threadID = std::vector<uint8_t>(), std::vector<uint8_t> conversationID = std::vector<uint8_t>(), ctFlags flags = ctFlags());

	static std::shared_ptr<CConsensusTask> instantiate(std::vector<uint8_t> data);
	std::vector<uint8_t> getPackedData();
	uint64_t getID();
	void setID(uint64_t id);
	eConsensusTaskType::eConsensusTaskType getType();
	void setType(eConsensusTaskType::eConsensusTaskType type);
	std::string getDescription();
	void setDescription(std::string desc);
	uint64_t getVersion();
	ctFlags getFlags();
	void setFlags(ctFlags flags);
	uint64_t getProcessID();
	void setProcessID(uint64_t id);
	std::vector<uint8_t> getThreadID();
	void setThreadID(std::vector<uint8_t> id);

private:
	std::mutex mGuardian;
	uint64_t mID;
	eConsensusTaskType::eConsensusTaskType mType;
	std::string mDescription;
	uint64_t  mProcessID;
	std::vector<uint8_t> mConversationID;
	std::vector<uint8_t>  mThreadID;
	ctFlags  mFlags;
	uint64_t mVersion = 1;
};
