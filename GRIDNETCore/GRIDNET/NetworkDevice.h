#pragma once
#include <stdint.h>
#include <vector>
class CNetworkDevice
{
	static enum eDeviceType
	{
		Ethernet,
		Bluetooth,
		WiFi
	};
public:
	CNetworkDevice(eDeviceType type, std::vector<uint8_t> MAC, uint32_t priority = 1);
	uint32_t getPriority();
	void setPriority(uint32_t priority);

private:
	uint32_t mPriority;
	std::vector<uint8_t> mMAC;
	uint32_t mType;
};

struct CompareNetworkDevice {
	class NetworkDevice;
	bool operator()(CNetworkDevice *lhs, CNetworkDevice *rhs) const
	{
		return lhs->getPriority() < rhs->getPriority();
	}
};