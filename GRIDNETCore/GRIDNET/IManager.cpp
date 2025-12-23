#include "IManager.h"
#include <memory>
void IManager::setIsReady(bool is)
{
	std::lock_guard<std::recursive_mutex> lock(mIsReadyGuardian);
	mIsReady = is;
	if (is)
	{
		mTimestampSinceOperational = std::time(0);
	}
}

bool IManager::getIsReady()
{
	std::lock_guard<std::recursive_mutex> lock(mIsReadyGuardian);
	return mIsReady;
}

uint64_t IManager::getTimeSinceOperational()
{
	std::lock_guard<std::recursive_mutex> lock(mIsReadyGuardian);
	if (mTimestampSinceOperational == 0)
	{
		return 0;
	}

	return (std::time(0) - mTimestampSinceOperational);
}
