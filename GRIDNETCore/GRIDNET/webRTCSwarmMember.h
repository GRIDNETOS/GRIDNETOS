#pragma once
#include <vector>
#include "enums.h"
#include <vector>
#include <memory>
#include <mutex>
class CSDPEntity;
class CConversation;

class CWebRTCSwarmMember
{
private:
	std::mutex mGuardian;
	std::mutex mFieldsGuardian;
	std::vector<uint8_t> mID;
	std::vector<uint8_t> mSwarmID;
	std::shared_ptr<CConversation> mConversation;//webSocket conversation
	std::shared_ptr<CSDPEntity> mSDPRequest;//reusable SDP request
	std::shared_ptr<CSDPEntity> mSDPResponse; //reusable SDP response
	size_t mLastTimeActive;
	bool mMarkedForDeletion;
	uint64_t mJoinRequestTimestamp;
	void initFields();

public:
	CWebRTCSwarmMember(std::shared_ptr<CConversation> conversation =nullptr, std::vector<uint8_t> ID= std::vector<uint8_t>(), std::vector<uint8_t> swarmID = std::vector<uint8_t>());
	uint64_t getJoinRequestTimestamp();
	void pingJoinRequest();
	std::vector<uint8_t> getID();
	std::shared_ptr<CConversation>  getConversation();
	void setConversation(std::shared_ptr<CConversation> conversation);
	void ping(bool onlyLocalData=true);
	size_t getLastTimeActive();
	void markForDeletion(bool doIt=true);
	bool getIsMarkedForDeletion();
	
};