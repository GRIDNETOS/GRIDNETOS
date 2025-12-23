#pragma once
#include "stdafx.h"
#include "NetMsg.h"
#include "enums.h"
/// <summary>
/// Represents the current state of a specific P2P conversation.
/// The state is verfied against the incomming message etc.
/// </summary>
/// 
class CConversation;
class CConversationState {
public:
	CConversationState(std::shared_ptr<CConversation> conversation=nullptr);
	CConversationState(const CConversationState & sibling);
	typedef struct lastAction
	{
		eNetEntType::eNetEntType entType;
		eNetReqType::eNetReqType reqType;

	};
private:
	eBlockchainMode::eBlockchainMode mBlockchainMode;
	std::shared_ptr<CTools> mTools;
	std::mutex mFieldsGuardian;
	eConversationState::eConversationState mState;
	size_t mLastStateChangeTimeStamp;
	uint64_t mLastActivity;
	size_t mStartTimeStamp;
	size_t mEndTimeStamp;
	std::weak_ptr<CConversation> mConversation;
public:
	std::shared_ptr<CConversation> getConversation();
	void setConversation(std::shared_ptr<CConversation> conversation);
	size_t getTimeEnded();
	size_t getTimeStarted();
	uint64_t getDuration();
	void ping();
	eConversationState::eConversationState getCurrentState();
	bool setCurrentState(eConversationState::eConversationState state);
	size_t getLastActivity();
};
