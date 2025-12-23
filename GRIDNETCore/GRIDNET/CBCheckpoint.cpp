#include "CBCheckpoint.h"

CBCheckpoint::CBCheckpoint(cpFlags flags, size_t height, const std::vector<uint8_t>& blockImage, const std::vector<uint8_t>& pespective)
{
	mHeight = height;
	mIsObligatory = false;
	mHash = blockImage;
	mPerspective = pespective;// If, a Perspective is provided, it would be checked AFTER the corresponding block is processed during the Flow.
						      // If, there is a mismatch between the expected perspective and the one provided here, the block at mHeight would be REJECTED.
	mFlags = flags;
}

void CBCheckpoint::setIsActive(bool isIt)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mFlags.active = isIt;
}

bool CBCheckpoint::getIsActive()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mFlags.active;
}

cpFlags CBCheckpoint::getFlags()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mFlags;
}

void CBCheckpoint::setFlags(cpFlags flags)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mFlags = flags;
}

size_t CBCheckpoint::getHeight()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mHeight;
}

std::vector<uint8_t> CBCheckpoint::getHash()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mHash;
}

std::vector<uint8_t> CBCheckpoint::getPerspective()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mPerspective;
}
