#pragma once
#include <vector>
#include <memory>
#include <mutex>

#include <string>
#include "enums.h"
typedef int UDTSOCKET;
class CConversation;
class CBlockchainManager;
class CNetworkManager;
class CNetMsg;
class CNetworkDevice;

class CEndPoint
{
	

public:
	 CEndPoint(std::vector<uint8_t> address, eEndpointType::eEndpointType type = eEndpointType::IPv4, CNetworkDevice * device=0, uint32_t portNumber=0);
	 CEndPoint(const CEndPoint & sybling);
	 CEndPoint();
	 //void establishConversation(UDTSOCKET  socket, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm);
	 ~CEndPoint();
	 void addTask(CNetMsg msg);
	  uint32_t getPort() const;
	 bool endConversation(bool blockTillFinished=true);
	 size_t getLastTimeSeen();
	 void ping();
	 std::vector<uint8_t> getPubKey();
	 void setPubKey(std::vector<uint8_t>& pubKey);
	 eEndpointType::eEndpointType getType();
	  std::vector<uint8_t> getAddress() const;
	 std::string getDescription(bool includeAddresses=true, bool includeEndpointTypes=true) const;

	 friend bool operator == (const CEndPoint& e1, const CEndPoint& e2);
	 friend bool operator != (const CEndPoint& e1, const CEndPoint& e2);
private:
	void initFields();
	std::mutex mGuardian;
	std::shared_ptr<CConversation> mUDTConversation;
	//CConversation * mConversation;
	std::vector<CNetMsg> mPendingTasks;
	//std::unique_ptr<CConversation> mConversation;
	bool mEnabled;
	size_t mLastTimeSeen;
	std::vector<uint8_t> mAddress;
	uint32_t mPortNumber;
	eEndpointType::eEndpointType mType;
	CNetworkDevice **mNetworkDevice;
	std::vector<uint8_t> mPubKey;
};
