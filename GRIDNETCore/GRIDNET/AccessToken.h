#pragma once
#include "stdafx.h"

class CStateDomainManager;
class RequestedPermissions
{
private:
	bool mRead : 1;
	bool mWrite : 1;
	bool mExecute : 1;
	bool mRemoval : 1;
	bool mOwnershipChange : 1;
	bool mSpending : 1;
	bool mVoting : 1;
	bool mReserved2 : 1;
public:
	RequestedPermissions(bool read = true, bool write = true, bool execute = true, bool ownershipChange = false, bool removal = false, bool spending = false, bool voting = false);
	RequestedPermissions& operator = (const RequestedPermissions& t);
	bool operator == (const RequestedPermissions& t);
	bool operator != (const RequestedPermissions& t);
	void clear();
	void setDefaults();

	bool getIsWrite() const;
	bool getIsVoting() const;
	bool getIsRead() const;
	bool getIsExec() const;
	bool getIsSpending() const;
	bool getIsRemoval() const;
	bool getIsOwnerChange() const;


	uint64_t getNr();
	RequestedPermissions(const RequestedPermissions& sibling);
	RequestedPermissions(uint8_t byte);

};

class CAccessToken
{

private:
	RequestedPermissions mReqPerm;
	std::mutex mGuardian;
	bool mIsSystem;
	//bool mIsOwner;
	BigInt mVotingPower;
	void initFields();
	bool mRecentOperationNotAuthorized;
	std::vector<uint8_t> mEntityID;
	std::string mFriendlyID;
	std::vector<uint64_t> mAccessedPolls;
	std::shared_ptr<CIdentityToken> mIDToken;
	bool mImpersonationToken;
public:
	void setEntityID(const std::vector<uint8_t>& id);
	std::shared_ptr<CIdentityToken> getIDToken();
	void setIDToken(std::shared_ptr<CIdentityToken> token);
	RequestedPermissions getRequestedPermissions();
	void setRequestedPermissions(const RequestedPermissions& reqPerm);
	std::string getFriendlyID();
	void setFriendlyID(const std::string& id);
	bool getIsSystem();
	//bool getIsOwner();
	//void setIsOwner(bool isIt);
	BigInt getVotingPower();
	void setVotingPower(BigInt votingPower);
	bool getRecentOperationNotAuthorized();
	void setRecentOperationNotAuthorized(bool wasIt = true);
	void clearState();
	std::vector<uint8_t> getEntityID();
	void addAccessedPoll(uint64_t pollID);
	std::vector<uint64_t> getAccessedPollIDs();
	CAccessToken();
	CAccessToken(const RequestedPermissions& reqPerm);
	CAccessToken(bool sysOnly);
	CAccessToken (std::vector<uint8_t> entityID, bool isOwner = false);

	CAccessToken& operator=(const CAccessToken& t);

	CAccessToken(const CAccessToken& sibling);
	CAccessToken(std::shared_ptr<CAccessToken> sibling);


	static std::shared_ptr<CAccessToken> genSysToken();
	bool getIsImpersonationToken();
	static std::shared_ptr<CAccessToken> genTokenForEntityByID(const std::vector<uint8_t> id, std::shared_ptr<CStateDomainManager> sdm, bool isImpersonationToken = false);


};



