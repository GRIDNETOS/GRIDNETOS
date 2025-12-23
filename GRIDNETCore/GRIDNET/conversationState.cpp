#pragma once
#include "conversationState.h"
#include <mutex>
#include "BlockchainManager.h"
#include "conversation.h"
#include "NetworkManager.h"
#include "ConnTracker.h"

CConversationState::CConversationState(std::shared_ptr<CConversation> conversation)
{
	mConversation = conversation;
	mState = eConversationState::eConversationState::initial;
	mTools = CTools::getInstance();
	mLastStateChangeTimeStamp = mTools->getTime();
	mLastActivity = mLastStateChangeTimeStamp;
	mStartTimeStamp = mTools->getTime();
	mEndTimeStamp = 0;
	mTools = CTools::getTools();//assuming it's used only for Time.
}
CConversationState::CConversationState(const CConversationState & sibling)
{
	mConversation = sibling.mConversation;
	mState = sibling.mState;
	mLastActivity = sibling.mLastActivity;
	mLastStateChangeTimeStamp = sibling.mLastStateChangeTimeStamp;
	mStartTimeStamp = sibling.mStartTimeStamp;
	mEndTimeStamp = sibling.mEndTimeStamp;
}

std::shared_ptr<CConversation> CConversationState::getConversation()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mConversation.lock();
}

void CConversationState::setConversation(std::shared_ptr<CConversation> conversation)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mConversation = conversation;
}

size_t CConversationState::getTimeEnded()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mEndTimeStamp;
}

size_t CConversationState::getTimeStarted()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mStartTimeStamp;
}

uint64_t CConversationState::getDuration() {
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	// Case where the connection has not started.
	if (mStartTimeStamp == 0) {
		return 0;
	}

	// Get current time as a fall-back for ongoing connection.
	uint64_t currentTimeStamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

	// If the connection is still ongoing, use the current time as the end timestamp.
	uint64_t effectiveEndTimeStamp = (mEndTimeStamp == 0) ? currentTimeStamp : mEndTimeStamp;

	// Case where the end timestamp is somehow earlier than the start timestamp.
	if (effectiveEndTimeStamp < mStartTimeStamp) {
		// This is an anomaly. Depending on how you want to handle it,
		// returning 0 could be a safe option to indicate an error or unexpected situation.
		return 0;
	}

	// Normal case, calculate the duration.
	return effectiveEndTimeStamp - mStartTimeStamp;
}
void CConversationState::ping()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastActivity = mTools->getTime();

}
eConversationState::eConversationState CConversationState::getCurrentState()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mState;
}

size_t CConversationState::getLastActivity()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastActivity;
}

bool CConversationState::setCurrentState(eConversationState::eConversationState state)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	// Prelinimaries - BEGIN
	if (mState == state)
		return true;

	if (mState == eConversationState::ended && state == eConversationState::ending)
		return true;

	// Prelinimaries - BEGIN

	// Operational Logic - BEGIN
	auto conv = mConversation.lock();

	// Apply State - BEGIN
	mState = state;
	// Apply State - END

	// State Processing (optional)- BEGIN
	switch (state)
	{
	case eConversationState::running:

		break;
	case eConversationState::ended:

		if (conv)
		{
			conv->setIsScheduledToStart(false);
			conv->clearLocalChainProof();
		}
		mEndTimeStamp = mTools->getTime();
		break;
	default:
		
		break;
	}
	// State Processing - END

	// Notify External Components - BEGIN
	if (conv)
	{
		std::shared_ptr<CNetworkManager> nm = conv->getNetworkManager();
		// Sub-Systems Registration - BEGIN
		nm->getConnTracker()->pingState(conv->getIPAddress(), state); // make sure the conversation is regisstred with the connection tracking sub-system
		nm->registerConversation(conv);//  make sure Conversation is registered with the firewall sub-system
		// Sub-Systems Registration - END

	}
	// Notify External Components - END

	mLastStateChangeTimeStamp = mTools->getTime();
	return true;
	// Operational Logic - END
}
