#include "BlockVerificationResult.h"
#include "BlockchainManager.h"
CBlockVerificationResult::~CBlockVerificationResult()
{
}
void CBlockVerificationResult::setAsLeader(bool doIt)
{
	mAssumeAsLeader = doIt;
	//mSaveLocally = true;
}
void CBlockVerificationResult::setStore(bool doIt)
{
	mSaveLocally = doIt;
}
bool CBlockVerificationResult::isLeader()
{
	return mAssumeAsLeader;
}
bool CBlockVerificationResult::isToBeStored()
{
	return mSaveLocally;
}
void CBlockVerificationResult::setForking(bool isForking)
{
	mForking = isForking;
}
bool CBlockVerificationResult::isForking()
{
	return mForking;
}
bool CBlockVerificationResult::isKeyBlock()
{
	return mIsKeyBlock;
}
void CBlockVerificationResult::setIsKeyBlock(bool isIt)
{
	mIsKeyBlock = isIt;
}
eBlockVerificationResult::eBlockVerificationResult CBlockVerificationResult::getStatus()
{
	return mStatus;
}

CBlockVerificationResult::CBlockVerificationResult(const CBlockVerificationResult & sibling)
{
	mForking = sibling.mForking;
	mStatus = sibling.mStatus;
	mBlockID = sibling.mBlockID;
	mAdditionalInfo = sibling.mAdditionalInfo;
	mSaveLocally = sibling.mSaveLocally;
	mAssumeAsLeader = sibling.mAssumeAsLeader;
	mIsKeyBlock = sibling.mIsKeyBlock;
}

std::vector<uint8_t> CBlockVerificationResult::getBlockID()
{
	return mBlockID;
}

std::vector<uint8_t> CBlockVerificationResult::getAdditionalInfo()
{
	return mAdditionalInfo;
}

CBlockVerificationResult::CBlockVerificationResult()
{
	mIsKeyBlock = false;
	mAssumeAsLeader = false;
	mForking = false;
	mAssumeAsLeader = false;
	mSaveLocally = false;
	mStatus = eBlockVerificationResult::eBlockVerificationResult::invalid;
}
void CBlockVerificationResult::setBlockID(std::vector<uint8_t> id)
{
	mBlockID = id;

}
void CBlockVerificationResult::setStatus(eBlockVerificationResult::eBlockVerificationResult status)
{
	mStatus = status;
}

CBlockVerificationResult::CBlockVerificationResult(eBlockVerificationResult::eBlockVerificationResult status, std::vector<uint8_t> blockID, std::vector<uint8_t> additionalInfo )
{
	mForking = false;
	mAssumeAsLeader = false;
	mSaveLocally = false;
	mStatus = status;
	mBlockID = blockID;
	mAdditionalInfo = additionalInfo;
}
