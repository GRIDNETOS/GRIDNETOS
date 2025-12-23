#pragma once
#include <memory>
#include <vector>
#include <mutex>
#include "enums.h"

class CWebRTCSwarm;
class CSDPEntity;
class CWebRTCBroker
{
private:

	//local variables - BEGIN
	std::recursive_mutex mGuardian;//protected section guardian
	std::vector<std::shared_ptr<CWebRTCSwarm>> mSwarms;
	std::vector<CSDPEntity> mPendingSIPTasks;
	//local variables - END

	bool processHandshakeRequest(std::vector<uint8_t> SIPRequest);
	bool processHandshakeResponse(std::vector<uint8_t> SIPResponse);

	//threading - BEGIN
	void SIPTasksMaintenanceThreadF();
	uint64_t mSIPQueueMaintenanceInterval;
	//threading - END

	void initFields();

public:
	//statistics - BEGIN
	uint64_t getPendingSIPEntitiesCount();
	//statistics - END

	std::shared_ptr<CWebRTCSwarm> findSwarmByID(std::vector<uint8_t> id);
	std::shared_ptr<CWebRTCSwarm> findSwarmWithMember(std::vector<uint8_t> memberID);

	//matchmaking
	std::vector<uint8_t> doHandshake(std::vector<uint8_t> SIPHandshakeA);
};
