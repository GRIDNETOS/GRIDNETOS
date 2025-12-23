#include "stdafx.h"
#include "Poll.h"
#include "IdentityToken.h"
#include "StateDomain.h"
#include "CStateDomainManager.h"
#include "AccessToken.h"

PollFileElemFlags::PollFileElemFlags(bool canCreatePollWithPermission)
{
	clear();
	mCanCreatePollWithPermission = canCreatePollWithPermission;
}

PollFileElemFlags& PollFileElemFlags::operator=(const PollFileElemFlags& t)
{
	std::memcpy(this, &t, sizeof(PollFileElemFlags));
	return *this;
}

bool PollFileElemFlags::operator==(const PollFileElemFlags& t)
{
	return std::memcmp(this, &t, sizeof(PollFileElemFlags));
}

bool PollFileElemFlags::operator!=(const PollFileElemFlags& t)
{
	return std::memcmp(this, &t, sizeof(PollFileElemFlags)) != 0;
}

bool PollFileElemFlags::getCreatePollWithPermission()
{
	return mCanCreatePollWithPermission;
}

void PollFileElemFlags::setCreatePollWithPermission(bool isIt)
{
	mCanCreatePollWithPermission = isIt;
}

void PollFileElemFlags::clear()
{
	std::memset(this, 0, 1);
}

void PollFileElemFlags::setDefaults()
{
	clear();
}

bool PollElemFlags::getAutoActions()
{
	return mAutoActions;
}

void PollElemFlags::setAutoActions(bool isIt)
{
	mAutoActions = isIt;
}

bool PollElemFlags::getOnlySelectCanVote()
{
	return mOnlySelectCanVote;
}

void PollElemFlags::setOnlySelectCanVote(bool isIt)
{
	mOnlySelectCanVote = isIt;
}

PollElemFlags::PollElemFlags(bool active, bool oneTime)
{
	clear();
	mActive = active;
	mOneTime = oneTime;
}

PollElemFlags& PollElemFlags::operator=(const PollElemFlags& t)
{
	std::memcpy(this, &t, sizeof(PollElemFlags));
	return *this;
}

bool PollElemFlags::operator==(const PollElemFlags& t)
{
	return std::memcmp(this, &t, sizeof(PollElemFlags));
}

bool PollElemFlags::operator!=(const PollElemFlags& t)
{
	return std::memcmp(this, &t, sizeof(PollElemFlags)) != 0;
}



void PollElemFlags::clear()
{
	std::memset(this, 0, sizeof(PollElemFlags));
}

void PollElemFlags::setDefaults()
{
	clear();
}

bool PollElemFlags::getWasInherited()
{
	return mInherited;
}

void PollElemFlags::setWasInherited(bool wasIt)
{
	mInherited = wasIt;
}

bool PollElemFlags::getIsMemberOfExclusiveGroup()
{
	return mExclusiveGroup;
}

void PollElemFlags::setMemberOfExcluiveGroup(bool isIt)
{
	mExclusiveGroup = isIt;
}
void PollElemFlags::setDisableInheritance(bool isIt)
{
	mNoInheritance = isIt;
}

bool PollElemFlags::getDisableInheritance()
{
	return mNoInheritance;
}

uint64_t PollElemFlags::getNr()
{
		uint64_t nr = 0;
		std::memcpy(&nr, this, sizeof(PollElemFlags));
		return static_cast<uint64_t>(nr);
}

PollElemFlags::PollElemFlags(const PollElemFlags& sibling)
{
	std::memcpy(this, &sibling, sizeof(PollElemFlags));
}

PollElemFlags::PollElemFlags(uint64_t packed)
{
	std::memcpy(this, &packed, sizeof(PollElemFlags));
}

bool PollElemFlags::getFired()
{
	return mFired;
}

void PollElemFlags::setFired(bool didIt)
{
	mFired = didIt;
}

bool PollElemFlags::getIsActive()
{
	return mActive;
}

void PollElemFlags::setIsActive(bool isIt)
{
	mActive = true;
}

bool PollElemFlags::getIsOneTime()
{
	return mOneTime;
}

void PollElemFlags::setIsOneTime(bool isIt)
{
	mOneTime = isIt;
}



/// <summary>
/// votingForSDID is optional and used when making a 2nd-Level vote.
/// </summary>
/// <param name="voterID"></param>
/// <param name="votingPower"></param>
/// <param name="votingForSDID"></param>
/// <returns></returns>
eVoteResult::eVoteResult CPollElem::vote(std::vector<uint8_t>& voterID, std::shared_ptr<CStateDomainManager> sdm, const std::vector<uint8_t>& votingForSDID)
{

	std::lock_guard<std::mutex> lock(mGuardian);

	//Local Variables - BEGIN
	std::shared_ptr<CTools> tools = CTools::getInstance();
	BigInt power = 0;
	std::shared_ptr<CVoter> entry;
	std::shared_ptr<CIdentityToken> id;
	CStateDomain* sd;
	std::shared_ptr<CPollFileElem> fileElem;
	PollElemFlags flags;
	//Local Variables - END
	
	//Operational Logic - BEGIN
	
	//initial checks - BEGIN
	if (!sdm || !voterID.size())
		return eVoteResult::failure;


	//Exclusive Group Checks - BEGIN
	if (mFlags.getIsMemberOfExclusiveGroup())
	{
		if (!fileElem)
			return eVoteResult::failure;//missing data
		std::vector<std::shared_ptr<CPollElem>>  polls = fileElem->getPolls();

		for (uint64_t i = 0; i < polls.size(); i++)
		{
			flags = polls[i]->getFlags();

			if (flags.getFired() && flags.getIsMemberOfExclusiveGroup())
			{
				return eVoteResult::exclusiveGroupAlreadyFired;
			}
		}
	}
	//Exclusive Group Checks - END
	
	//check voting power - BEGIN
	sd = sdm->findByID(voterID);

	if (!sd)
		return eVoteResult::noVotingPower;

	id = sd->getIDToken();

	if (!id)
	{
		return eVoteResult::noVotingPower;
	}

	//check voting power - END

	power = id->getConsumedCoins();

	if ( power == 0)
	{
		return eVoteResult::noVotingPower;
	}

	//initial checks - END
	if(votingForSDID.size())
	{
		//2nd Level Vote - BEGIN
		//check for double vote
		
		entry = getVoteBySDID(votingForSDID);
		if (entry)
		{
			if (entry->getVoteBySDID(voterID))
			{
				return eVoteResult::alreadyVoted;//already voted for that candidate!
			}
			entry->addL2Voter(std::make_shared<CVoter>(voterID));
			return eVoteResult::success;
		}

		//2nd Level Vote - END
	}
	else
	{
		//1st Level Vote - BEGIN
		if (getVoteBySDID(voterID))
		{
			return eVoteResult::alreadyVoted;//caller already is a Level-1 Candidate.
		}

		mVoters.push_back(std::make_shared<CVoter>(voterID));
		return eVoteResult::success;
	    //1st Level Vote - END
	}
	return eVoteResult::failure;
	//Operational Logic - END
}

std::shared_ptr<CVoter> CPollElem::getVoteBySDID(const std::vector<uint8_t>& voterID)
{
	std::shared_ptr<CTools> tools = CTools::getInstance();

	std::lock_guard<std::mutex> lock(mGuardian);

	for (uint64_t i = 0; i < mVoters.size(); i++)
	{
		if (tools->compareByteVectors(voterID, mVoters[i]->getID()))
		{
			return mVoters[i];
		}
	}
	return nullptr;
}

void CPollElem::initFields()
{
	mLocalPollElemID = 0;
	mVersion = 1;
	mNeedsProcessing = false;
	mExclusiveGroupID = 0;
}

CPollElem::CPollElem()
{
	initFields();
}

CPollElem::CPollElem(std::shared_ptr<CPollFileElem> fileElem, const uint64_t localPollID, const BigInt& requiredScoring, const PollElemFlags flags, const  std::string& description)
{
	initFields();
	mRequiredScoring = requiredScoring;
	mFlags = flags;
	mLocalPollElemID = localPollID;//PollIDs < 10 - Reserved System Polls
	mFileElem = fileElem;
	mDescription = description;
}

uint64_t CPollElem::getExclusiveGroupID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mExclusiveGroupID;
}


void CPollElem::setExclusiveGroupID(uint64_t id)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	 mExclusiveGroupID = id;
}

std::shared_ptr<CPollFileElem> CPollElem::getFileElem()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mFileElem.lock();
}

void CPollElem::setNeedsProcessing(bool doesIt)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mNeedsProcessing = doesIt;
}

bool CPollElem::getNeedsProcessing()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mNeedsProcessing;
}

void CPollElem::setFileElem(std::shared_ptr<CPollFileElem> fileElem)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mFileElem = fileElem;
}


CPollElem& CPollElem::operator=(const CPollElem& sibling)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mNeedsProcessing = sibling.mNeedsProcessing;
	mFileElem = sibling.mFileElem;
	mVersion = sibling.mVersion;
	mLocalPollElemID = sibling.mLocalPollElemID;
	mRequiredScoring = sibling.mRequiredScoring;
	mFlags = sibling.mFlags;
	mDescription = sibling.mDescription;
	mExclusiveGroupID = sibling.mExclusiveGroupID;
	for (uint64_t i = 0; i < sibling.mVoters.size(); i++)
	{
		mVoters.push_back(std::make_shared<CVoter>(*sibling.mVoters[i]));
	}
	return *this;
}

CPollElem::CPollElem(const CPollElem& sibling)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mNeedsProcessing = sibling.mNeedsProcessing;
	mFileElem = sibling.mFileElem;
	mVersion = sibling.mVersion;
	mLocalPollElemID = sibling.mLocalPollElemID;
	mRequiredScoring = sibling.mRequiredScoring;
	mFlags = sibling.mFlags;
	mDescription = sibling.mDescription;
	mExclusiveGroupID = sibling.mExclusiveGroupID;

	for (uint64_t i = 0; i < sibling.mVoters.size(); i++)
	{
		mVoters.push_back(std::make_shared<CVoter>(*sibling.mVoters[i]));
	}
}

uint64_t CPollElem::getLocalPollID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLocalPollElemID;
}

std::string CPollElem::getDescription()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDescription;
}

void CPollElem::setDescription(const std::string& desc)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDescription = desc;
}

void CPollElem::clearVoters()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mVoters.clear();
}

void CPollElem::reset()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mVoters.clear();
	mFlags.setFired(false);
}

std::shared_ptr<CVoter> CPollElem::getVoterByID(const std::vector<uint8_t>& id)
{
	if (!id.size())
		return nullptr;

	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::lock_guard<std::mutex> lock(mGuardian);
	for (uint64_t i = 0; i < mVoters.size(); i++)
	{
		if (tools->compareByteVectors(id, mVoters[i]->getID()))
		{
			return  mVoters[i];
		}
	}
	return nullptr;
}

void CPollElem::addVoter(std::shared_ptr<CVoter> voter)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mVoters.push_back(voter);
}

uint64_t CPollElem::getVersion()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVersion;
}

BigInt CPollElem::getRequiredScoring()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mRequiredScoring;
}
PollElemFlags CPollElem::getFlags()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mFlags;
}
bool CPollElem::activate()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	if (mFlags.getIsActive())
		return false;

	mFlags.setIsActive(true);
	return true;
}

bool CPollElem::deactivate()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	if (!mFlags.getIsActive())
		return false;

	mFlags.setIsActive(false);
	return true;
}

/// <summary>
/// Fires Post Trigger Actions.
/// e.x:for example, finalize other members of a mutually elusive group
/// </summary>
bool CPollElem::firePostTriggerActions(std::shared_ptr<CAccessToken> accessToken , bool sandboxmode,  CStateDomain* domain , CTrieNode** object , const std::vector<uint8_t>& newPerspective)
{
	bool toRet = true;
	//Exclusive Group Support - BEGIN
	PollElemFlags flags = getFlags();
	uint64_t cost = 0;
	//Obligatory Automatic Actions - BEGIN
	if (flags.getIsMemberOfExclusiveGroup())
	{
		uint64_t groupID = getExclusiveGroupID();
		std::shared_ptr<CPollFileElem> extension = getFileElem();
		std::vector<std::shared_ptr<CPollElem>>  polls = extension->getPolls();
		for (uint64_t i = 0; i < polls.size(); i++)
		{
			if (polls[i]->getFlags().getIsMemberOfExclusiveGroup() && polls[i]->getExclusiveGroupID() == groupID)
			{
				polls[i]->deactivate();
			}
		}
	}
	//Obligatory Automatic Actions - END
	
	//Opt-In Automatic Actions - BEGIN
	//Object Removal - BEGIN
	
	if (flags.getAutoActions() && accessToken && domain && object && (*object))
	{
		if (getLocalPollID() == eSystemPollID::removalAccessRightsPoll)
		{
			//remove file.
			toRet = domain->removeElement((*object)->getPath(true), const_cast<std::vector<uint8_t>&>(newPerspective), cost, "", "" , sandboxmode, false, accessToken);
			if (toRet)
			{
				(*object) = nullptr;
			}
		}
	}
	//Object Removal - END
	
	//Opt-In Automatic Actions - END
	
	//Exclusive Group Support - END

	return toRet;
}
bool CPollElem::checkTrigger(std::shared_ptr<CStateDomainManager> sdm, const std::vector<uint8_t>& bestCandidateID, const BigInt& bestCandidateScore)
{
	if (!sdm)
		return false;

	BigInt currentScoring = getCurrentTotalScoring(sdm, bestCandidateID, bestCandidateScore);
	
	std::lock_guard<std::mutex> lock(mGuardian);
	if (mFlags.getIsActive()==false)
		return false;

	BigInt scoreReq = mRequiredScoring;
	if (currentScoring >= mRequiredScoring)
	{
		//check if this has happened before, and if so,-  if we can process (again)..
		if (mFlags.getFired() && mFlags.getIsOneTime())
		{
			//we cannot;it was supposed to be a one-time-thing; abort.
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}
VoterFlags::VoterFlags()
{
	std::memset(this, 0, sizeof(VoterFlags));
}

VoterFlags& VoterFlags::operator=(const VoterFlags& t)
{
	std::memcpy(this, &t, sizeof(VoterFlags));
	return *this;
}

bool VoterFlags::operator==(const VoterFlags& t)
{
	return std::memcmp(this, &t, sizeof(VoterFlags));
}

bool VoterFlags::operator!=(const VoterFlags& t)
{
	return std::memcmp(this, &t, sizeof(VoterFlags)) != 0;
}

void VoterFlags::clear()
{
	std::memset(this, 0, sizeof(VoterFlags));
}

void VoterFlags::setDefaults()
{
	clear();
}

uint64_t VoterFlags::getNr()
{
	uint64_t nr = 0;
	std::memcpy(&nr, this, sizeof(VoterFlags));
	return static_cast<uint64_t>(nr);
}

VoterFlags::VoterFlags(const VoterFlags& sibling)
{
	std::memcpy(this, &sibling, sizeof(VoterFlags));
}

VoterFlags::VoterFlags(uint64_t packed)
{
	std::memcpy(this, &packed, sizeof(PollExFlags));
}

void CPollElem::setFlags(PollElemFlags flags)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mFlags = flags;
}
uint64_t CPollElem::getVotersCount()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVoters.size();
}

void CPollElem::setRequiredScoring(const BigInt& val)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mRequiredScoring = val;
}

BigInt CPollElem::getCurrentTotalScoring(std::shared_ptr<CStateDomainManager> sdm, const std::vector<uint8_t>& bestCandidateID, const BigInt& bestCandidateScore )
{
	//Local Variables - BEGIN
	if (!sdm)
		return 0;
	std::lock_guard<std::mutex> lock(mGuardian);
	BigInt res = 0;
	//CStateDomain* sd = nullptr;
	//std::shared_ptr<CIdentityToken> id;
	BigInt totalScore = 0;
	//std::vector<uint8_t> bestCandidateID;
	BigInt currentPeerScore = 0;
	//Local Variables - END

	//Operational Logic - BEGIN
	for (uint64_t i = 0; i < mVoters.size(); i++)
	{
		currentPeerScore = mVoters[i]->getScore(sdm, false);//accounts for level-2 voters
		totalScore += currentPeerScore;
		if (currentPeerScore > bestCandidateScore)
		{
			const_cast<BigInt&>(bestCandidateScore) = currentPeerScore;
			const_cast<std::vector<uint8_t>&>(bestCandidateID) = mVoters[i]->getID();
		}
	}
	//Operational Logic - END
	return totalScore;
}


std::vector <std::shared_ptr<CVoter>> CPollElem::getLevel1Candidates(std::shared_ptr<CStateDomainManager> sdm, const std::vector<uint8_t>& bestCandidateID, const BigInt& bestCandidateScore, bool resolveIDTokens )
{
	
	//Local Variables - BEGIN
	std::vector <std::shared_ptr<CVoter>> toRet;
	if (!sdm)
		return toRet;

	std::lock_guard<std::mutex> lock(mGuardian);
	BigInt res = 0;
	BigInt totalScore = 0;
	BigInt currentPeerScore = 0;
	//Local Variables - END
	CStateDomain* candidate = nullptr;
	std::shared_ptr<CIdentityToken> idToken;

	//Operational Logic - BEGIN
	for (uint64_t i = 0; i < mVoters.size(); i++)
	{
		currentPeerScore = mVoters[i]->getScore(sdm, false);//accounts for level-2 voters
		totalScore += currentPeerScore;
		if (currentPeerScore > bestCandidateScore)
		{
			const_cast<BigInt&>(bestCandidateScore) = currentPeerScore;
			const_cast<std::vector<uint8_t>&>(bestCandidateID) = mVoters[i]->getID();
		}
		if (resolveIDTokens)
		{
			candidate = sdm->findByID(mVoters[i]->getID());
			if (candidate)
			{
				idToken = candidate->getIDToken();
				if (idToken)
				{
					mVoters[i]->setIDToken(idToken);
					toRet.push_back(mVoters[i]);
				}

			}
		
		}

	}
	return toRet;
	//Operational Logic - END

}

bool CPollElem::getIndividualScoring(const std::vector<uint8_t>& id, BigInt& val, std::shared_ptr<CStateDomainManager> sdm)
{
	if (!sdm)
		return false;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::lock_guard<std::mutex> lock(mGuardian);

	//Operational Logic - BEGIN
	for (uint64_t i = 0; i < mVoters.size(); i++)
	{
		if (tools->compareByteVectors(mVoters[i]->getID(), id))
		{
			val = mVoters[i]->getScore(sdm);
			return true;
		}
	}
	//Operational Logic - END
	val = 0;
	return false;
}

bool CPollElem::getHighestScoring(std::vector<uint8_t>& id, BigInt& val, std::shared_ptr<CStateDomainManager> sdm)
{
	if (!sdm)
		return false;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::lock_guard<std::mutex> lock(mGuardian);
	std::vector<uint8_t> bestSDID;
	BigInt bestScore = 0;
	//Operational Logic - BEGIN
	for (uint64_t i = 0; i < mVoters.size(); i++)
	{
	
		val = mVoters[i]->getScore(sdm);
		if (val > bestScore)
		{
			bestScore = val;
			bestSDID = mVoters[i]->getID();
		}
			
		
	}
	//Operational Logic - END
	id = bestSDID;
	val = bestScore;
	if (val && bestSDID.size())
	{
		return true;
	}
	return false;
}


std::vector<uint8_t> CPollElem::getPackedData()
{
	Botan::DER_Encoder enc;
	//std::vector<uint8_t> flagsV(1);
	size_t flags = 0;
	std::vector<std::vector<uint8_t>> level2Voters;
	flags = mFlags.getNr();
	std::shared_ptr<CTools> tools = CTools::getInstance();
	enc = enc.start_cons(Botan::ASN1_Tag::SEQUENCE).
		encode(mVersion)
		.encode(mLocalPollElemID)
		.encode(flags)
		.encode(tools->BigIntToBytes(mRequiredScoring), Botan::ASN1_Tag::OCTET_STRING)
		.encode(tools->stringToBytes(mDescription), Botan::ASN1_Tag::OCTET_STRING)
		.start_cons(Botan::ASN1_Tag::SEQUENCE);


	for (uint64_t i = 0; i < mVoters.size(); i++)
	{
		//encode Level-1 voter ID
		enc = enc.encode(mVoters[i]->getFlags().getNr());
		enc = enc.encode(mVoters[i]->getID(), Botan::ASN1_Tag::OCTET_STRING);
		enc = enc.start_cons(Botan::ASN1_Tag::SEQUENCE);//level-2 sequence obligatory

		//now encode Level-2 voters if any:
		mVoters[i]->getLevel2VotersIDs(level2Voters);
		for (uint64_t y = 0; y < level2Voters.size(); y++)
		{
			enc = enc.encode(level2Voters[y], Botan::ASN1_Tag::OCTET_STRING);
		}
		enc = enc.end_cons();//end Level-2 voters sequence
	}

	enc.end_cons().end_cons();

	return enc.get_contents_unlocked();
}

std::shared_ptr<CPollElem> CPollElem::instantiate(std::vector<uint8_t>& bytes)
{
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::shared_ptr<CPollElem> poll = std::make_shared<CPollElem>();
	size_t voterFlags = 0;
	if (bytes.size() == 0)
		return nullptr;
	try {
		//std::shared_ptr<CRightsToken> token = std::make_shared<CRightsToken>();//new version
		uint64_t sourceVersion = 1;
		Botan::BER_Decoder dec1 = Botan::BER_Decoder(bytes).start_cons(Botan::ASN1_Tag::SEQUENCE);
		dec1.decode(sourceVersion);
		size_t flags = 0;
		std::vector<uint8_t> temp;
		if (sourceVersion == 1)//would be upgraded when saved
		{
	
			dec1.decode(poll->mLocalPollElemID);
			dec1.decode(flags);
		
	
			poll->mVersion = sourceVersion;
			poll->mFlags = PollElemFlags(flags);
			dec1.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			poll->mRequiredScoring = tools->BytesToBigInt(temp);
			dec1.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			poll->mDescription = tools->bytesToString(temp);
			std::shared_ptr<CVoter> voter, voterL2;
			Botan::BER_Decoder dec2 = Botan::BER_Decoder(dec1.get_next_object().value);
			while (dec2.more_items())
			{
				dec2.decode(voterFlags);

				dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				voter = std::make_shared<CVoter>(temp,VoterFlags(voterFlags));

				Botan::BER_Decoder dec3 = Botan::BER_Decoder(dec2.get_next_object().value);

				while (dec3.more_items())
				{
					dec3.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
					voterL2 = std::make_shared<CVoter>(temp, VoterFlags(voterFlags));
					voter->addL2Voter(voterL2);
				}
				poll->mVoters.push_back(voter);
			}
			dec1.verify_end();
			return poll;
		}

	}
	catch (std::exception ex)
	{
		return nullptr;
	}
	return nullptr;
}

VoterFlags CVoter::getFlags()
{
	return mFlags;
}

void CVoter::setFlags(const VoterFlags& flags)
{
	mFlags = flags;
}

CVoter::CVoter(const std::vector<uint8_t>& id,  const VoterFlags& flags)
{
	mID = id;
	mFlags = flags;
}

CVoter& CVoter::operator=(const CVoter& sibling)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	mID = sibling.mID;
	mFlags = sibling.mFlags;
	for (uint64_t i = 0; i < sibling.mLevel2Voters.size(); i++)
	{
		mLevel2Voters.push_back(std::make_shared<CVoter>(*sibling.mLevel2Voters[i]));
	}

	return *this;
}

CVoter::CVoter(const CVoter& sibling)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	mID = sibling.mID;
	mFlags = sibling.mFlags;
	for (uint64_t i = 0; i < sibling.mLevel2Voters.size(); i++)
	{
		mLevel2Voters.push_back(std::make_shared<CVoter>(*sibling.mLevel2Voters[i]));
	}

}

std::shared_ptr<CIdentityToken> CVoter::getIDToken()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mIDToken;
}

void CVoter::setIDToken(std::shared_ptr<CIdentityToken> idToken)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mIDToken = idToken;
}

std::vector<uint8_t> CVoter::getID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mID;
}



/// <summary>
/// Computes score of a Level-1 Voter.
/// If onlySelf parameter specified, Level-2 voters are not taken into account.
/// </summary>
/// <param name="sdm"></param>
/// <param name="onlySelf"></param>
/// <returns></returns>
BigInt CVoter::getScore(std::shared_ptr< CStateDomainManager> sdm, bool onlySelf)
{
	
	std::lock_guard<std::mutex> lock(mGuardian);
	CStateDomain* sd = sdm->findByID(mID);
	if (!sd)
		return 0;

	std::shared_ptr<CIdentityToken> id = sd->getIDToken();
	if (!id)
		return 0;

	BigInt score = mFlags.mDoNotCountPower ? 0: id->getConsumedCoins();
	if(onlySelf)
	return score;

	BigInt s = 0;

	for (uint64_t i = 0; i < mLevel2Voters.size(); i++)
	{
		sd = sdm->findByID(mLevel2Voters[i]->getID());
		if (!sd)
			continue;
		id = sd->getIDToken();
		if (!id)
			 continue;

score += id->getConsumedCoins();
	}

	return score;

}


/// <summary>
/// Retrieved a 2nd-Level vote.
/// Notice: voting power is returned only in an instance of StateDomainManager is provided.
/// </summary>
/// <param name="voterID"></param>
/// <param name="votingPower"></param>
/// <param name="sdm"></param>
/// <returns></returns>
bool CVoter::getVoteBySDID(std::vector<uint8_t>& voterID, const BigInt& votingPower, std::shared_ptr< CStateDomainManager> sdm)
{
	//Local Variables - BEGIN
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::lock_guard<std::mutex> lock(mGuardian);
	CStateDomain* sd = nullptr;
	std::shared_ptr<CIdentityToken> id;
	//Local Variables - END

	//Operational Logic - BEGIN
	for (uint64_t i = 0; i < mLevel2Voters.size(); i++)
	{
		if (tools->compareByteVectors(voterID, mLevel2Voters[i]->getID()))
		{
			if (!sdm)
			{
				const_cast<BigInt&>(votingPower) = 0;
				return true;
			}
			sd = sdm->findByID(mID);
			if (!sd)
			{
				const_cast<BigInt&>(votingPower) = 0;
				return false;
			}
			std::shared_ptr<CIdentityToken> id = sd->getIDToken();
			if (!id)
			{
				const_cast<BigInt&>(votingPower) = 0;
				return false;
			}

			const_cast<BigInt&>(votingPower) = id->getConsumedCoins();
			return true;
		}
	}
	//Operational Logic - BEGIN

	return false;
}

void CVoter::getLevel2VotersIDs(std::vector<std::vector<uint8_t>>& IDs)
{
	IDs.clear();
	std::lock_guard<std::mutex> lock(mGuardian);

	for (uint64_t i = 0; i < mLevel2Voters.size(); i++)
	{
		IDs.push_back(mLevel2Voters[i]->getID());
	}
}

void CVoter::addL2Voter(std::shared_ptr<CVoter> voter)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLevel2Voters.push_back(voter);
}

uint64_t CVoter::getVotersCount()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLevel2Voters.size();
}

std::shared_ptr<CPollElem> CPollFileElem::getPollAtIndex(uint64_t index)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	if (mPolls.size() == 0 || index > mPolls.size() - 1)
	{
		return nullptr;
	}
	return mPolls[index];
}

std::shared_ptr<CPollElem> CPollFileElem::getPollWithLocalID(uint64_t id)
{

	std::lock_guard<std::mutex> lock(mGuardian);

	for (uint64_t i = 0; i < mPolls.size(); i++)
	{
		if (mPolls[i]->getLocalPollID() == id)
		{
			return mPolls[i];
		}
	}
	return nullptr;
}

void CPollFileElem::initFields()
{
	mVersion = 1;
}

PollFileElemFlags CPollFileElem::getFlags()
{
	std::lock_guard lock(mGuardian);
	return mFlags;
}

CPollFileElem::CPollFileElem()
{
	initFields();
}

std::vector<std::shared_ptr<CPollElem>> CPollFileElem::getPolls()
{
	std::lock_guard lock(mGuardian);
	return mPolls;
}

CPollFileElem& CPollFileElem::operator=(const CPollFileElem& t)
{
	std::lock_guard lock(mGuardian);
	mVersion = t.mVersion;

	for (uint64_t i = 0; i < t.mPolls.size(); i++)
	{
		mPolls.push_back(std::make_shared<CPollElem>(*t.mPolls[i]));
	}
	return *this;
}

CPollFileElem::CPollFileElem(const CPollFileElem& t)
{
	std::lock_guard lock(mGuardian);
	mVersion = t.mVersion;

	for (uint64_t i = 0; i < t.mPolls.size(); i++)
	{
		mPolls.push_back(std::make_shared<CPollElem>(*t.mPolls[i]));
	}
}

void CPollFileElem::resetPollsState()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	for (uint64_t i = 0; i < mPolls.size(); i++)
	{
		mPolls[i]->reset();
	}
}

/// <summary>
/// Finalize all polls being members of a mutually exclusive group.
/// </summary>
/// <returns></returns>
bool CPollFileElem::finalizeExclusiveGroup(uint64_t groupID)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	bool finalized = false;
	PollElemFlags flags;
	for (uint64_t i = 0; i < mPolls.size(); i++)
	{
		flags = mPolls[i]->getFlags();
		if (flags.getIsMemberOfExclusiveGroup() && mPolls[i]->getExclusiveGroupID() == groupID)
		{
			finalized = true;
			mPolls[i]->deactivate();
		}
	}
	return finalized;
}

bool CPollFileElem::updatePoll(std::shared_ptr<CPollElem> poll)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	for (uint64_t i = 0; i < mPolls.size(); i++)
	{
		if (mPolls[i]->getLocalPollID() == poll->getLocalPollID())
		{
			mPolls[i] = poll;
			return true;
		}
	}
	return false;
}

bool CPollFileElem::addPoll(std::shared_ptr<CPollElem> poll)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mPolls.push_back(poll);
	return true;
}
uint64_t CPollFileElem::getVersion()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVersion;
}
uint64_t CPollFileElem::genNewExclusiveGroupID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	uint64_t highestGroupID = 0;

	for (uint64_t i = 0; i < mPolls.size(); i++)
	{
		if (mPolls[i]->getExclusiveGroupID() > highestGroupID)
		{
			highestGroupID = mPolls[i]->getExclusiveGroupID();
		}
	}
	return (++highestGroupID);
	
}
uint64_t CPollFileElem::getPollsCount()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mPolls.size();
}

std::vector<uint8_t> CPollFileElem::getPackedData()
{


	Botan::DER_Encoder enc;
	std::vector<uint8_t> flagsV(1);
	std::vector<std::vector<uint8_t>> level2Voters;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	enc = enc.start_cons(Botan::ASN1_Tag::SEQUENCE).
		encode(mVersion)
		.encode(flagsV, Botan::ASN1_Tag::OCTET_STRING)
		.start_cons(Botan::ASN1_Tag::SEQUENCE);


	for (uint64_t i = 0; i < mPolls.size(); i++)
	{
		//encode Level-1 voter ID
		enc = enc.encode(mPolls[i]->getPackedData(), Botan::ASN1_Tag::OCTET_STRING);
	}

	enc.end_cons().end_cons();

	return enc.get_contents_unlocked();
}

std::shared_ptr<CPollFileElem> CPollFileElem::instantiate(std::vector<uint8_t>& bytes)
{

	std::vector<uint8_t> flagsV;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::shared_ptr <CPollFileElem> fElem;
	std::shared_ptr <CPollElem> poll;
	if (bytes.size() == 0)
		return nullptr;
	try {
		//std::shared_ptr<CRightsToken> token = std::make_shared<CRightsToken>();//new version
		uint64_t sourceVersion = 1;
		Botan::BER_Decoder dec1 = Botan::BER_Decoder(bytes).start_cons(Botan::ASN1_Tag::SEQUENCE);
		dec1.decode(sourceVersion);
		dec1.decode(flagsV, Botan::ASN1_Tag::OCTET_STRING);
		std::vector<uint8_t> temp;
		if (sourceVersion == 1)//would be upgraded when saved
		{
			fElem = std::make_shared<CPollFileElem>();
			fElem->mVersion = sourceVersion;
			fElem->mFlags = PollFileElemFlags(flagsV[0]);
			Botan::BER_Decoder dec2 = Botan::BER_Decoder(dec1.get_next_object().value);
			while (dec2.more_items())
			{

				dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
				poll = CPollElem::instantiate(temp);

				if (!poll)
					return nullptr;

				fElem->addPoll(poll);
			}
			dec1.verify_end();
			return fElem;
		}

	}
	catch (std::exception ex)
	{
		return nullptr;
	}
	return nullptr;
}



PollExFlags::PollExFlags()
{
	std::memset(this,0, sizeof(PollExFlags));
}

PollExFlags& PollExFlags::operator=(const PollExFlags& t)
{
	std::memcpy(this, &t, sizeof(PollExFlags));
	return *this;
}

bool PollExFlags::operator==(const PollExFlags& t)
{
	return std::memcmp(this, &t, sizeof(PollExFlags));
}

bool PollExFlags::operator!=(const PollExFlags& t)
{
	return std::memcmp(this, &t, sizeof(PollExFlags)) != 0;
}

void PollExFlags::clear()
{
	std::memset(this, 0, sizeof(PollExFlags));
}

void PollExFlags::setDefaults()
{
	clear();
}

uint64_t PollExFlags::getNr()
{
	uint64_t nr = 0;
	std::memcpy(&nr, this, sizeof(PollExFlags));
	return static_cast<uint64_t>(nr);
}

PollExFlags::PollExFlags(const PollExFlags& sibling)
{
	std::memcpy(this, &sibling, sizeof(PollExFlags));
}

PollExFlags::PollExFlags(uint64_t byte)
{
	std::memcpy(this, &byte, sizeof(PollExFlags));
}
/////


ChownExFlags::ChownExFlags()
{
	std::memset(this, 0, sizeof(ChownExFlags));
}

ChownExFlags& ChownExFlags::operator=(const ChownExFlags& t)
{
	std::memcpy(this, &t, sizeof(ChownExFlags));
	return *this;
}

bool ChownExFlags::operator==(const ChownExFlags& t)
{
	return std::memcmp(this, &t, sizeof(ChownExFlags));
}

bool ChownExFlags::operator!=(const ChownExFlags& t)
{
	return std::memcmp(this, &t, sizeof(ChownExFlags)) != 0;
}

void ChownExFlags::clear()
{
	std::memset(this, 0, sizeof(ChownExFlags));
}

void ChownExFlags::setDefaults()
{
	clear();
}

uint64_t ChownExFlags::getNr()
{
	uint64_t nr = 0;
	std::memcpy(&nr, this, sizeof(ChownExFlags));
	return static_cast<uint64_t>(nr);
}

ChownExFlags::ChownExFlags(const ChownExFlags& sibling)
{
	std::memcpy(this, &sibling, sizeof(ChownExFlags));
}

ChownExFlags::ChownExFlags(uint64_t byte)
{
	std::memcpy(this, &byte, sizeof(ChownExFlags));
}
/////////////////////////

SetFACLFlags::SetFACLFlags()
{
	std::memset(this, 0, sizeof(SetFACLFlags));
}

SetFACLFlags& SetFACLFlags::operator=(const SetFACLFlags& t)
{
	std::memcpy(this, &t, sizeof(SetFACLFlags));
	return *this;
}

bool SetFACLFlags::operator==(const SetFACLFlags& t)
{
	return std::memcmp(this, &t, sizeof(SetFACLFlags));
}

bool SetFACLFlags::operator!=(const SetFACLFlags& t)
{
	return std::memcmp(this, &t, sizeof(SetFACLFlags)) != 0;
}

void SetFACLFlags::clear()
{
	std::memset(this, 0, sizeof(SetFACLFlags));
}

void SetFACLFlags::setDefaults()
{
	clear();
}

uint64_t SetFACLFlags::getNr()
{
	uint64_t nr = 0;
	std::memcpy(&nr, this, sizeof(SetFACLFlags));
	return static_cast<uint64_t>(nr);
}

SetFACLFlags::SetFACLFlags(const SetFACLFlags& sibling)
{
	std::memcpy(this, &sibling, sizeof(SetFACLFlags));
}

SetFACLFlags::SetFACLFlags(uint64_t byte)
{
	std::memcpy(this, &byte, sizeof(SetFACLFlags));
}