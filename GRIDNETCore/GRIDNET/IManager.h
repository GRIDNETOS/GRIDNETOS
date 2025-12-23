#pragma once
#include "enums.h"
#include <mutex>
class IManager
{

public:
	void setIsReady(bool is = true);

	bool getIsReady();
	virtual void stop() = 0;
	virtual void pause() = 0;
	virtual void resume() = 0;
	virtual eManagerStatus::eManagerStatus getStatus()=0;
	uint64_t getTimeSinceOperational();
private:
	uint64_t mTimestampSinceOperational=0;
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status)=0;
	virtual eManagerStatus::eManagerStatus getRequestedStatusChange()=0;
	std::recursive_mutex mIsReadyGuardian;
	bool mIsReady = false;
	virtual void setStatus(eManagerStatus::eManagerStatus status)=0;

};
