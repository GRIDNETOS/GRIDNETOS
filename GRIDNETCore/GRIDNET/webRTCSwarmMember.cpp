#include "conversation.h"
#include "webRTCSwarmMember.h"
#include "SDPEntity.h"

#include <memory>
#include <vector>
#include <mutex>
#include "enums.h"

void CWebRTCSwarmMember::initFields()
{
	mJoinRequestTimestamp = 0;
	mLastTimeActive = 0;
	mMarkedForDeletion = false;
}

CWebRTCSwarmMember::CWebRTCSwarmMember(std::shared_ptr<CConversation> conversation, std::vector<uint8_t> ID, std::vector<uint8_t> swarmID)
{
	initFields();
	mConversation = conversation;
	if (ID.size() == 0)
	{
		std::shared_ptr<CTools> tools =  CTools::getInstance();
		ID = tools->stringToBytes(tools->getRandomStr(8));
	}
	mID = ID;
	mSwarmID = swarmID;
}

uint64_t CWebRTCSwarmMember::getJoinRequestTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mJoinRequestTimestamp;;
}

void CWebRTCSwarmMember::pingJoinRequest()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mJoinRequestTimestamp = std::time(0);
}

std::vector<uint8_t> CWebRTCSwarmMember::getID()
 {
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 return mID;
 }


/// <summary>
/// Gets a WebSocket Conversation.
/// </summary>
/// <param name="conversation"></param>
 std::shared_ptr<CConversation> CWebRTCSwarmMember::getConversation()
 {
	 std::lock_guard<std::mutex> lock(mGuardian);
	 return mConversation;
 }

 /// <summary>
 /// Sets a WebSocket Conversation.
 /// </summary>
 /// <param name="conversation"></param>
 void CWebRTCSwarmMember::setConversation(std::shared_ptr<CConversation> conversation)
 {
	 std::lock_guard<std::mutex> lock(mGuardian);
	 mConversation = conversation;
 }


 void CWebRTCSwarmMember::ping(bool onlyLocalData)
 {
	 std::lock_guard<std::mutex> lock(mGuardian);
	 mLastTimeActive = CTools::getInstance()->getTime();
	 //remember there's also a Web-Socket-level (lower-level) activity timestamp which is updated with each incoming datagram
 }

 size_t CWebRTCSwarmMember::getLastTimeActive()
 {
	 std::lock_guard<std::mutex> lock(mGuardian);
	 return mLastTimeActive;
 }

 void CWebRTCSwarmMember::markForDeletion(bool doIt)
 {
	 std::lock_guard<std::mutex> lock(mGuardian);
	 mMarkedForDeletion = doIt;
 }

 bool CWebRTCSwarmMember::getIsMarkedForDeletion()
 {
	 std::lock_guard<std::mutex> lock(mGuardian);
	 return mMarkedForDeletion;
 }
