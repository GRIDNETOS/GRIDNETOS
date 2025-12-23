#include "NetworkDevice.h"

CNetworkDevice::CNetworkDevice(eDeviceType type, std::vector<uint8_t> MAC, uint32_t priority)
{
	mType = type;
	mMAC = MAC;
	mPriority = priority;
}

uint32_t CNetworkDevice::getPriority()
{
	return mPriority;
}



void CNetworkDevice::setPriority(uint32_t priority)
{
	mPriority = priority;
}
