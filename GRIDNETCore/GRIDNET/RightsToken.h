#ifndef RIGHTS_TOKEN_H
#define RIGHTS_TOKEN_H

#include "stdafx.h"
class CAccessEntry;
class CAccessToken;
class CEffectiveRights;
class CPollElem;
class CPollFileElem;
class CStateDomainManager;

class CRTFlags
{

private:
	//Warning: these need to be FIRST - BEGIN

	bool mHasACL : 1;
	bool mEveryoneRead : 1;//we need these default values for agents not covered by specific ACL entries
	bool mEveryoneExecute : 1;
	bool mEveryoneWrite : 1;
	bool mReserved1 : 1;
	bool mReserved2 : 1;
	bool mReserved3: 1;
	bool mSysOnly : 1;
	//Warning: these need to be FIRST - END


public:
	CRTFlags& operator = (const CRTFlags& t);
	bool operator == (const CRTFlags& t);
	bool operator!=(const CRTFlags& t);
	void clear();
	void setDefaults();
	CRTFlags(const CRTFlags& sibling);
	CRTFlags();
	CRTFlags(uint8_t byte);
	void setSysOnly(bool isIt = true);
	bool getIsSysOnly();

	void setHasACL(bool hasIt = true);
	bool getHasACL();

	void setEveryoneWrite(bool hasIt = true);
	bool getEveryoneWrite();
	void setEveryoneRead(bool hasIt);
	bool getEveryoneRead();
	void setEveryoneExecute(bool hasIt = true);
	bool getEveryoneExecute();


	 uint64_t getNr() const;
};
class CSecDescriptor
{

private:
	std::shared_ptr<CPollFileElem> mPollExt;
	std::mutex mGuardian;
	size_t mVersion;
	CRTFlags mFlags;
	std::vector<uint8_t> mImplicitOwner;//NOT serialized. based on containing directory
	std::vector<uint8_t> mOwner;
	std::vector<uint8_t> mExtensionBytes;//byte-array comprising a BER-encoded sequence of elements.
	//each element within the sequence comprises a standalone 'extension'.
	/*
	[Position 0] - Votables Extension
	/*
	* Notice:
	* + actions defined in [SEQUENCE 0] at index < 10 are 'system polls' - with corresponding 'system actions'.
	* + the corresponding actions trigger autonomously ONLY and ONCE 
	    the CURRENT scoring value >= the corresponding REQUIRED scoring value.
	*   Both values are contained within tuples stored in [0.N].
	* + ONLY the file owner can set the REQUIRED scoring value.
	* + UNLESS the 'ACTIVE' field in POLL_FLAGS is SET, only the file owner can modify poll's properties.
	* + AFTER the poll has been activated, only voting operations are permitted.
	* + the poll can be modified by the owner, ONLY up until it becomes activated.
	* + the REQUIRED scoring value, once set and poll activated - it CANNOT be changed (even by owner) AFTER the 'ACTIVE' field in POLL_FLAGS is SET.
	* + users can interact with the extension through the POLL user-mode #GridScript utility.
	* + by default all polls are running indefinitely. 
	* + poll settings can be altered through the corresponding POLL_FLAGS
	* + a poll becomes operational ONCE the 'ACTIVE' field in POLL_FLAGS is SET.
	* + 
	
	* 
	* [SYSTEM ACTIONS - BEGIN]
	* The below holds true AFTER the poll has been activated.
	* + if removal scoring set - owner looses write-access permissions - unless elected through the ongoing poll.
	* + if execution scoring set - the file cannot be executed - unless by person elected through the ongoing poll.
	* + if write scoring set - nobody can write to this file -  unless elected through the ongoing poll.
	* + if ownership-change scoring set - the file becomes owned by the system immediately. 
										  Unless somebody gets elected through the ongoing poll.
	* [SYSTEM ACTIONS - END]
	* 
	* 
	* [POLL_FLAGS - BEGIN]
	* ACTIVE - transitions poll into ACTIVE state. Only limited interactions would be allowed.
	* oneTime (false by default) - otherwise the poll continues indefinitely even after event was triggered.
	* [POLL_FLAGS - END]
	* /
	 [EXT. SEQUENCE]  - BEGIN
	 [SEQUENCE 0] - BEGIN
	 Votables Extension - BEGIN (layout of a BER sequence) - comprised of two SEQUENCEs
	 //Reserved VOTINGS - BEGIN
		[SEQUENCE 0.0] - BEGIN
		[0.0.0] - uint - required removal scoring - can't be removed BEFORE [0.0.1] == [0.0.0]
		[0.0.1] - uint - current removal scoring
		[0.0.2] - byte - POLL_FLAGS
		[SEQUENCE 0.0] - END
		[SEQUENCE 0.1] - BEGIN
		[0.1.0] - uint - required execution scoring
		[0.1.1] - uint - current execution scoring
		[0.1.2] - byte - POLL_FLAGS
		[SEQUENCE 0.1] - END
		[SEQUENCE 0.2] - BEGIN
		[0.2.0] - uint - required write scoring
		[0.2.1] - uint - current write scoring (of the current winner!)
		[0.2.2] - byte - POLL_FLAGS
		[SEQUENCE 0.2] - END
		[SEQUENCE 0.3] - BEGIN
		[0.3.0] - uint - required ownerhip-change scoring
		[0.3.1] - uint - current ownerhip-change scoring (of the current winner!)
		[0.3.2] - byte - POLL_FLAGS
		[SEQUENCE 0.3] - END

		[SEQUENCE 0.4-9] - BEGIN
		[0.3.0] - uint - required RESERVED scoring
		[0.3.1] - uint - current RESERVED scoring (of the current winner!)
		[0.3.2] - byte - POLL_FLAGS
		[SEQUENCE 0.4-9] - END

		//user defined polls INDEX>=10
	//Reserved VOTINGS - END
		//(..)
		[0.N.0] - uint - required OPTIONAL_ACTION_N scoring
		[0.N.1] - uint - current OPTIONAL_ACTION_N  scoring
		[0.N.2] - byte - POLL_FLAGS
	 [SEQUENCE 0] - END
	 
	 [SEQUENCE 1] - BEGIN (VOTERS  - TWO LEVELS) 
		[SEQUENCE 1.N]  - BEGIN - VOTERS LEVEL-1 FOR EVENT IN [SEQUENCE 0.N]
		//Notice: Level-1 'voters' may be 'candidates' being voted for by others (Level-2 voters below).
			[SEQUENCE 1.N.Y] - VOTER Y - BEGIN
				[1.N.Y.0] - byteVector - Y'th STATE_DOMAIN_ID
						[SEQUENCE 1.N.Y.1]  - BEGIN - VOTERS LEVEL-2 FOR EVENT DEFINED IN [SEQUENCE 0.N]
							[1.N.Y.1.Z] - byteVector - Z'th STATE_DOMAIN_ID (VOTER LEVEL 2); Z>=0 - ID of Voter Level-2 voting for Voter Level-1 (Y) (defined by 1.N.Y.0)
						[SEQUENCE 1.N.Y.1]  - END -   VOTERS LEVEL-2 FOR EVENT DEFINED IN [SEQUENCE 0.N]
			[SEQUENCE 1.N.Y] - VOTER Y- END
		[SEQUENCE 1.N] -  END
	 [SEQUENCE 1] - END
	 [EXT. SEQUENCE]  - END
	 Votables Extension - END


	[Position 0+N] - 'reserved' for additional extensions
	*/
	 void initFields();
	 std::vector<std::shared_ptr<CAccessEntry>> mAccessEntries;

public:
	void setImplicitOwner(const std::vector<uint8_t>& ownerID);
	std::vector<uint8_t> getImplicitOwner();
	bool getIsDefault();//whether it's worthwhile to store this descriptor in Cold Storage.
	std::shared_ptr<CPollFileElem> getPollExt();

	void setPollExt(std::shared_ptr<CPollFileElem> pollExt);

	std::vector<uint8_t> getOwner();
	void setOwner(std::vector<uint8_t> id);
	std::vector<uint8_t> getExtensionBytes();
	void setExtensionBytes(std::vector<uint8_t> bytes);


	bool getEveryoneRead();
	bool getEveryoneWrite();
	bool getEveryoneExecute();
	CSecDescriptor();

	CSecDescriptor(bool sysOnly );

	CSecDescriptor& operator=(const CSecDescriptor& t);

	CSecDescriptor(const CSecDescriptor& sibling);
	CSecDescriptor(std::shared_ptr<CSecDescriptor> sibling);

	bool setCRTFlags(CRTFlags flags);
	//serialization - BEGIN
	std::vector<uint8_t> getPackedData();
	static std::shared_ptr<CSecDescriptor> instantiate(const std::vector<uint8_t> & data);
	static std::shared_ptr<CSecDescriptor> genSysOnlyDescriptor();
	//serialization - END

	void setVersion(size_t version);
	size_t getVersion();

	std::vector <std::shared_ptr<CAccessEntry>> getACEEntries(bool includeDynamic = false, std::shared_ptr<CStateDomainManager> sdm=nullptr);
	std::vector <std::shared_ptr<CAccessEntry>> getDynamicACEEntries(std::shared_ptr<CStateDomainManager> sdm);

	//modification
	bool addACE(std::shared_ptr<CAccessEntry> ACE);

	bool removeACEAtIndex(uint64_t ind);

	std::shared_ptr<CEffectiveRights> getEffectiveRights(std::shared_ptr<CAccessToken> accessToken, std::shared_ptr<CStateDomainManager> sdm= nullptr, const bool& updateSecDesc = false);

	std::shared_ptr<CAccessEntry> getACEforEntity(const std::vector<uint8_t>& sdID);

	std::shared_ptr<CAccessEntry> getACEforEntityEx(const std::vector<uint8_t>&sdID);

	//sec validation
	bool getIsAllowed(std::shared_ptr<CAccessToken> accessToken, std::shared_ptr<CStateDomainManager> sdm, bool &updateSecDesc);//the requested permissions are described within of the access token; if present - otherwise defaults are used.

	std::shared_ptr<CAccessEntry> getAccessEntry(std::vector<uint8_t> sdID);
	bool getIsSysOnly();
	void setIsSysOnly(bool isIt=true);
};

#endif
