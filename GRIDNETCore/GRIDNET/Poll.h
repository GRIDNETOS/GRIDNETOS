#pragma once
class CStateDomainManager;
class CIdentityToken;
class CStateDomain;
class CAccessToken;
class VoterFlags;


class VoterFlags//ephemeral flags issued when calling the kernel mode function. NOT STORED anywhere.
{
private:

public:

	bool mDoNotCountPower : 1;//when included by a third party into a poll.
	bool mReserved2 : 1;
	bool mReserved3 : 1;
	bool mReserved4 : 1;
	bool mReserved5 : 1;
	bool mReserved6 : 1;
	bool mReserved7 : 1;
	bool mReserved8 : 1;

	VoterFlags();
	VoterFlags& operator = (const VoterFlags& t);
	bool operator == (const VoterFlags& t);
	bool operator != (const VoterFlags& t);
	void clear();
	void setDefaults();

	uint64_t getNr();
	VoterFlags(const VoterFlags& sibling);
	VoterFlags(uint64_t packed);


};


class CVoter
{
private:
	std::mutex mGuardian;
	std::vector<uint8_t> mID; //serialized
	std::vector <std::shared_ptr<CVoter>> mLevel2Voters;//serialized
	std::shared_ptr<CIdentityToken> mIDToken;//not serialized.
	VoterFlags mFlags;
public:
	VoterFlags getFlags();
	void setFlags(const VoterFlags &flags);
	CVoter(const std::vector<uint8_t>& id,  const VoterFlags & flags = VoterFlags());
	CVoter& operator=(const CVoter& t);
	CVoter(const CVoter& sibling);
	std::shared_ptr<CIdentityToken> getIDToken();
	void setIDToken(std::shared_ptr<CIdentityToken> idToken);
	std::vector<uint8_t> getID();
	BigInt getScore(std::shared_ptr< CStateDomainManager> sdm, bool onlySelf = false);
	bool getVoteBySDID(std::vector<uint8_t>& voterID, const BigInt& votingPower = 0, std::shared_ptr< CStateDomainManager> sdm = nullptr);
	void getLevel2VotersIDs(std::vector < std::vector<uint8_t>>& IDs);
	void addL2Voter(std::shared_ptr<CVoter> voter);
	uint64_t getVotersCount();
};






/*
	[3][0] - description present
			[3][1] - exclusive
			[3][2] - once
			[3][3] - attach to state-domain
			[3][4] - threshold value denominated in GBUs
			[3][5] - is a Level-2 Vote*/

class PollExFlags//ephemeral flags issued when calling the kernel mode function. NOT STORED anywhere.
{
private:

public:

	bool mExclusive : 1;
	bool mOnce : 1;
	bool mAttachToSD : 1;
	bool mLevel2Vote : 1;//if member of an exclusive group; once the poll-elem fires, all the other PollElems within the encapsulating CPollFileElem are deactivated.
	bool mThresholdInGBUs : 1;
	bool mDescriptionPresent : 1;
	bool mShowStats : 1;
	bool mAutoActions : 1;
	bool mNoInheritance : 1;//we've moved beyond a single byte already.
	bool mReserved2 : 1;
	bool mReserved3 : 1;
	bool mReserved4 : 1;
	bool mReserved5 : 1;
	bool mReserved6: 1;
	bool mReserved7 : 1;
	bool mReserved8 : 1;

	PollExFlags();
	PollExFlags& operator = (const PollExFlags& t);
	bool operator == (const PollExFlags& t);
	bool operator != (const PollExFlags& t);
	void clear();
	void setDefaults();

	uint64_t getNr();
	PollExFlags(const PollExFlags& sibling);
	PollExFlags(uint64_t byte);


};



class ChownExFlags
{
private:

public:

	bool mRecursive : 1;
	bool mAlterStateDomain : 1;
	bool mReserved1 : 1;
	bool mReserved2 : 1;//if member of an exclusive group; once the poll-elem fires, all the other PollElems within the encapsulating CPollFileElem are deactivated.
	bool mReserved3 : 1;
	bool mReserved4 : 1;
	bool mReserved5 : 1;
	bool mReserved6 : 1;

	ChownExFlags();
	ChownExFlags& operator = (const ChownExFlags& t);
	bool operator == (const ChownExFlags& t);
	bool operator != (const ChownExFlags& t);
	void clear();
	void setDefaults();

	uint64_t getNr();
	ChownExFlags(const ChownExFlags& sibling);
	ChownExFlags(uint64_t byte);


};

class SetFACLFlags
{
private:

public:

	bool mRecursive : 1;
	bool mAlterStateDomain : 1;
	bool mModACL : 1;
	bool mModCRT : 1;//if member of an exclusive group; once the poll-elem fires, all the other PollElems within the encapsulating CPollFileElem are deactivated.
	bool removePermission : 1;//Polls
	bool mReserved4 : 1;
	bool mReserved5 : 1;
	bool mReserved6 : 1;

	SetFACLFlags();
	SetFACLFlags& operator = (const SetFACLFlags& t);
	bool operator == (const SetFACLFlags& t);
	bool operator != (const SetFACLFlags& t);
	void clear();
	void setDefaults();

	uint64_t getNr();
	SetFACLFlags(const SetFACLFlags& sibling);
	SetFACLFlags(uint64_t byte);


};


class PollFileElemFlags
{
private:
	bool mCanCreatePollWithPermission : 1;// whether a permission holder can create polls regarding the permission. By default only object owner can create and/or modify polls.
	bool mReserved1 : 1;
	bool mReserved2 : 1;
	bool mReserved3 : 1;
	bool mReserved4 : 1;
	bool mReserved5 : 1;
	bool mReserved6 : 1;
	bool mReserved7 : 1;
public:
	PollFileElemFlags(bool CanCreatePollWithPermission = false);
	PollFileElemFlags& operator = (const PollFileElemFlags& t);
	bool operator == (const PollFileElemFlags& t);
	bool operator != (const PollFileElemFlags& t);
	void clear();
	void setDefaults();

	bool getCreatePollWithPermission();
	void setCreatePollWithPermission(bool isIt = true);

};

class PollElemFlags
{
private:
	bool mActive : 1;
	bool mOneTime : 1;
	bool mFired : 1;
	bool mExclusiveGroup : 1;//if member of an exclusive group; once the poll-elem fires, all the other PollElems within the encapsulating CPollFileElem are deactivated.
	bool mCanCreatePollWithPermission : 1;// whether a permission holder can create polls regarding the permission. By default only object owner can create and/or modify polls.
	bool mOnlySelectCanVote : 1;//by default anyone can vote  in this poll.
	bool mAutoActions : 1;// for instance a file gets auto-deleted whenever a system poll corresponding to 'removal' privilege triggers. By default, the winner is expected to remove the file manually on his sole discretion should he choose to.
	bool mNoInheritance : 1;
	//2nd byte:
	bool mInherited : 1;
	bool mReserved : 7;
public:

	bool getAutoActions();
	void setAutoActions(bool isIt);
	bool getOnlySelectCanVote();
	void setOnlySelectCanVote(bool isIt = true);
	PollElemFlags(bool active = false, bool oneTime = false);
	PollElemFlags& operator = (const PollElemFlags& t);
	bool operator == (const PollElemFlags& t);
	bool operator != (const PollElemFlags& t);
	void clear();
	void setDefaults();
	bool getWasInherited();
	void setWasInherited(bool wasIt);
	bool getIsMemberOfExclusiveGroup();
	void setMemberOfExcluiveGroup(bool isIt = true);
	void setDisableInheritance(bool isIt = true);
	bool getDisableInheritance();
	uint64_t getNr();
	PollElemFlags(const PollElemFlags& sibling);
	PollElemFlags(uint64_t packed);


	bool getFired();
	void setFired(bool didIt = true);

	bool getIsActive();
	void setIsActive(bool isIt = true);

	bool getIsOneTime();
	void setIsOneTime(bool isIt = true);


};
class CPollFileElem;
class CPollElem
{private:
	bool mNeedsProcessing;
	uint64_t mExclusiveGroupID;
	uint64_t mVersion;
	uint64_t mLocalPollElemID;
	std::string mDescription;
	std::mutex mGuardian;
	BigInt mRequiredScoring;//serialized
	PollElemFlags mFlags;
	eVoteResult::eVoteResult vote(std::vector<uint8_t>& selfID, std::shared_ptr<CStateDomainManager> sdm, const std::vector<uint8_t>& votingForSDID = std::vector<uint8_t>());
	std::shared_ptr<CVoter> getVoteBySDID(const std::vector<uint8_t>& voterID);
	std::vector<std::shared_ptr<CVoter>> mVoters;
	std::weak_ptr<CPollFileElem> mFileElem;
	void initFields();
public:
	CPollElem();
	CPollElem(std::shared_ptr<CPollFileElem> fileElem, const uint64_t localPollID,const BigInt& requiredScoring, const PollElemFlags flags,const std::string & description="");
	uint64_t getExclusiveGroupID();

	void setExclusiveGroupID(uint64_t id);
	std::shared_ptr<CPollFileElem> getFileElem();
	void setNeedsProcessing(bool doesIt = true);
	bool getNeedsProcessing();
	void setFileElem(std::shared_ptr<CPollFileElem> fileElem);

	CPollElem& operator=(const CPollElem& t);
	CPollElem(const CPollElem& sibling);
	uint64_t getLocalPollID();

	std::string getDescription();
	void setDescription(const std::string & desc);
	void clearVoters();
	void reset();
	std::shared_ptr<CVoter> getVoterByID(const std::vector<uint8_t>& id);
	void addVoter(std::shared_ptr<CVoter> voter);
	uint64_t getVersion();
	BigInt getRequiredScoring();
	PollElemFlags getFlags();
	bool activate();
	bool deactivate();
	bool firePostTriggerActions(std::shared_ptr<CAccessToken> accessToken = nullptr,bool sandboxmode=true, CStateDomain* domain = nullptr , CTrieNode** object = nullptr, const std::vector<uint8_t> &newPerspective = std::vector<uint8_t>());
	bool checkTrigger(std::shared_ptr<CStateDomainManager> sdm,const std::vector<uint8_t>& bestCandidateID= std::vector<uint8_t>(), const BigInt& bestCandidateScore=0);
	void setFlags(PollElemFlags flags);
	uint64_t getVotersCount();
	void setRequiredScoring(const BigInt &val);
	BigInt getCurrentTotalScoring(std::shared_ptr< CStateDomainManager> sdm, const std::vector<uint8_t> &bestCandidateID = std::vector<uint8_t>(), const BigInt & bestCandidateScore=0);
	std::vector<std::shared_ptr<CVoter>> getLevel1Candidates(std::shared_ptr<CStateDomainManager> sdm, const std::vector<uint8_t>& bestCandidateID = std::vector<uint8_t>(), const BigInt& bestCandidateScore=0, bool resolveIDTokens = true);
	bool getIndividualScoring(const std::vector<uint8_t>& id, BigInt& val, std::shared_ptr< CStateDomainManager> sdm);
	bool getHighestScoring(std::vector<uint8_t>& id, BigInt& val, std::shared_ptr< CStateDomainManager> sdm);
	
	std::vector<uint8_t> getPackedData();
	static std::shared_ptr<CPollElem> instantiate(std::vector<uint8_t>& bytes);
	
};


class CPollFileElem
{//Notice there is a one-to-many relationship between CPollFileElem and CPollElem.
private:
	std::mutex mGuardian;
	PollFileElemFlags mFlags;
	uint64_t mVersion;
	std::vector<std::shared_ptr<CPollElem>> mPolls;

	void initFields();
public:
	uint64_t genNewExclusiveGroupID();
	PollFileElemFlags getFlags();
	CPollFileElem();
	std::vector<std::shared_ptr<CPollElem>> getPolls();
	CPollFileElem& operator=(const CPollFileElem& t);
	CPollFileElem(const CPollFileElem& sibling);

	void resetPollsState();
	bool finalizeExclusiveGroup(uint64_t groupID);
	bool updatePoll(std::shared_ptr<CPollElem> poll);
	std::shared_ptr<CPollElem> getPollAtIndex(uint64_t index);
	std::shared_ptr<CPollElem> getPollWithLocalID(uint64_t id);
	bool addPoll(std::shared_ptr<CPollElem> poll);
	uint64_t getVersion();
	uint64_t getPollsCount();
	std::vector<uint8_t> getPackedData();
	static std::shared_ptr<CPollFileElem> instantiate(std::vector<uint8_t>& bytes);
};