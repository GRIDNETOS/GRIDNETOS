#include "stdafx.h"
#include "EffectiveRights.h"

std::shared_ptr<CEffectiveRights> CEffectiveRights::getDefaultRights()
{
	return std::make_shared< CEffectiveRights>();
}

CEffectiveRights::CEffectiveRights(bool read, bool write, bool execute, bool ownership, bool spending, bool removal, bool voting)
{
	mRead = read;
	mOwnership = ownership;
	mWrite = write;
	mExecute = execute;
	mSpending = spending;
	mRemoval = removal;
	mVoting = voting;
}

bool CEffectiveRights::getCanRead()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mRead;
}

bool CEffectiveRights::getCanVote()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVoting;
}

bool CEffectiveRights::getCanSpend()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mSpending;
}

bool CEffectiveRights::getCanRemove()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mRemoval;
}

bool CEffectiveRights::getCanWrite()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mWrite;
}

bool CEffectiveRights::getCanExecute()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mExecute;
}

std::string CEffectiveRights::toString(std::string newLine)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	std::string toRet="[";

	if (mRead)
		toRet += "r";
	else
	{
		toRet += "-";
	}

	if (mWrite)
		toRet += "w";
	else
	{
		toRet += "-";
	}
	if (mExecute)
		toRet += "x";
	else
	{
		toRet += "-";
	}
	toRet += "]";

	if (mOwnership)
	{
		toRet +=  CTools::getInstance()->getColoredString(" [Owner]", eColor::lightCyan);
	}
	return toRet;
}

bool CEffectiveRights::getIsOwner()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mOwnership;
}

void CEffectiveRights::setCanSpend(bool isIt)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mSpending = isIt;
}

void CEffectiveRights::setCanRemove(bool isIt)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mRemoval = isIt;
}

void CEffectiveRights::setCanRead(bool isIt)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mRead = isIt;
}

void CEffectiveRights::setCanWrite(bool isIt)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mWrite = isIt;
}

void CEffectiveRights::setCanExecute(bool isIt)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mExecute = isIt;
}

void CEffectiveRights::setIsOwner(bool isIt)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mOwnership = isIt;
}

void CEffectiveRights::setCanVote(bool isIt)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mVoting = isIt;
}
