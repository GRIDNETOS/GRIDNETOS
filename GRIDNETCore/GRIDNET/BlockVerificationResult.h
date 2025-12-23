#pragma once
#include "BlockchainManager.h"
#include <vector>
#include "enums.h"
class CBlockVerificationResult
{
private:
	eBlockVerificationResult::eBlockVerificationResult mStatus;
	std::vector<uint8_t> mBlockID;
	std::vector<uint8_t> mAdditionalInfo;
	bool mSaveLocally;
	bool mAssumeAsLeader;
	bool mForking;
	bool mIsKeyBlock;
public:
	~CBlockVerificationResult();
	void setAsLeader(bool doIt = true);
	void setStore(bool doIt = true);
	bool isLeader();
	bool isToBeStored();
	void setForking(bool isForking = true);
	bool isForking();
	bool isKeyBlock();
	void setIsKeyBlock(bool isIt = true);
	eBlockVerificationResult::eBlockVerificationResult getStatus();
	CBlockVerificationResult(const CBlockVerificationResult& sibling);
	std::vector<uint8_t> getBlockID();
	std::vector<uint8_t> getAdditionalInfo();
	CBlockVerificationResult();
	void setBlockID(std::vector<uint8_t> id);
	void setStatus(eBlockVerificationResult::eBlockVerificationResult status);
	 CBlockVerificationResult(eBlockVerificationResult::eBlockVerificationResult status,
		 std::vector<uint8_t> blockID = std::vector<uint8_t>(),
		 std::vector<uint8_t> additionalInfo= std::vector<uint8_t>());
};