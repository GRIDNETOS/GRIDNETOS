#include "stdafx.h"
#include "TerminalLine.h"

CTerminalLine::CTerminalLine(std::string str, bool endLine, bool includeOwner, eViewState::eViewState view, std::string forcedOwnername, bool broadcastToAllTerminals, bool asynchronous, uint64_t priority , std::shared_ptr<CDTI> dti , std::string ownerName, eViewState::eViewState  defaultView , eBlockchainMode::eBlockchainMode blockchainMode)
{
	mText = str;
	mEndLine = endLine;
	mIncludeOwner = includeOwner;
	mView = view;
	mForcedOwnerName = forcedOwnername;
	mBroadcastToAllTerminals = broadcastToAllTerminals;
	mAsynchronous = asynchronous;
	mPriority = priority;
	mDTI = dti;
	mOwnerName = ownerName;
	mDefaultView = defaultView;
	mBlockchainMode = blockchainMode;

}

void CTerminalLine::setDTI(std::shared_ptr<CDTI> dti)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mDTI = dti;
}

std::shared_ptr<CDTI> CTerminalLine::getDTI()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mDTI;
}

std::string CTerminalLine::getText()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mText;
}

std::string CTerminalLine::getOwnerName()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mOwnerName;
}

bool CTerminalLine::getEndLine()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mEndLine;
}

eViewState::eViewState CTerminalLine::getDefaultView()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mDefaultView;
}

eBlockchainMode::eBlockchainMode CTerminalLine::getBlockchainMode()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mBlockchainMode;
}

bool CTerminalLine::getIncludeOwner()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mIncludeOwner;
}

eViewState::eViewState CTerminalLine::getView()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mView;
}

std::string CTerminalLine::getForcedOwnerName()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mForcedOwnerName;
}

bool CTerminalLine::getBroadcastToAllTerminals()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mBroadcastToAllTerminals;
}

bool CTerminalLine::getAsynchronous()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mAsynchronous;
}

uint64_t CTerminalLine::getPriority()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mPriority;
}
