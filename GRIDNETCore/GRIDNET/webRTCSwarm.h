#pragma once
#include <vector>
#include "enums.h"
#include <vector>
#include <memory>
#include <mutex>
class CWebRTCSwarmMember;
class CSDPEntity;
class CTools;
class CDataRouter;
class CEndPoint;

class CWebRTCSwarm
{
private:
	std::weak_ptr<CDataRouter> mRouter;
	std::shared_ptr<CNetworkManager> mNetworkManager;
	std::mutex mGuardian;
	std::mutex mFieldsGuardian;
	std::mutex mMembersGuardian;
	std::vector<std::shared_ptr<CWebRTCSwarmMember>> mMembers;
	uint64_t mSizeLimit;
	void initFields();
	std::shared_ptr<CTools> getTools();
	uint64_t mInactivityRemovalThreshold;
	std::vector<std::shared_ptr<CSDPEntity>> mPendingSDPEntities;
	uint64_t mLastActivity;
	std::shared_ptr<CTools> mTools;
	uint64_t mSDPEntityProcessingTimeout;
	std::vector<uint8_t> mID;
	std::shared_ptr<CEndPoint> mAbstractEndpoint;//represents the very swarm
	std::shared_ptr<CNetworkManager> getNetworkManager();
public:

	std::shared_ptr<CEndPoint> getAbstractEndpoint();//constructed on initialization only
	CWebRTCSwarm(std::shared_ptr<CNetworkManager> nm, std::vector<uint8_t> id, std::shared_ptr<CDataRouter> router, uint64_t sizeLimit = 1000, uint64_t inactivityRemovalThreshold = 3600);
	bool genAndRouteSDP(eSDPEntityType::eSDPEntityType type, std::shared_ptr<CConversation> conversation,eSDPControlStatus::eSDPControlStatus controlStatus= eSDPControlStatus::ok,
		std::vector<uint8_t> sourceID = std::vector<uint8_t>(), eNetReqType::eNetReqType reqType = eNetReqType::notify );
	uint64_t getLastActivityTimestamp();
	uint64_t routeSDPEntities();
	bool addSDPEntity(std::shared_ptr<CSDPEntity> entity);
	void refreshAbstractEndpoint(bool updateRT=true);
	bool addMember(std::shared_ptr<CWebRTCSwarmMember> member, bool updateRT=true, std::shared_ptr<CEndPoint> deliveredFrom = nullptr, uint64_t dinstance=0);
	bool removeMemberByID(std::vector<uint8_t> ID, bool ifNotAuthed = false, bool ifConnDead=false);
	void pingActvity();
	std::vector< std::shared_ptr<CWebRTCSwarmMember>> getMembers();
	std::shared_ptr<CWebRTCSwarmMember> findMemberByID(std::vector<uint8_t> ID,std::vector<uint8_t> IPAddress= std::vector<uint8_t>(), bool preferAuthenticated = true);
	std::shared_ptr<CWebRTCSwarmMember> findMemberByIP(std::vector<uint8_t> IPAddress);
	void close();
	uint64_t broadcastMsg(std::shared_ptr<CNetMsg> msg);
	void doMaintenance();
	std::vector<uint8_t> getID();



};