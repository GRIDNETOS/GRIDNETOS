#pragma once
#include <vector>
#include <mutex>
#include <stdafx.h>

class AEFlags
{
private:
	//these pretty much correspond to what is available within CEffectiveRights
	bool mRead : 1;
	bool mWrite : 1;
	bool mExecute : 1;
	bool mAssets : 1;//valid only when associated with a state-domain.
	bool mRemoval : 1;
	bool mIsPollPermission : 1;//grants access to a particular poll associated with the object.
	bool mOwnership : 1;//used by polls/dynamic ACEs
	bool mIsDynamicEntry : 1;


public:
	AEFlags(bool write = false, bool read = true, bool execute = true, bool assets=false, bool ownership = false, bool isDynamic = false);
	AEFlags& operator = (const AEFlags& t);
	void clear();
	void setDefaults();
	
	AEFlags(const AEFlags& sibling);
	AEFlags(uint8_t byte);
	uint64_t getNr();

	/*
* IMPORTANT: ownership is controlled by the 'owner' field of the CSecDescriptor. Not through ACEs.
*/
	void setIsDynamicEntry(bool isIt = true);
	bool getIsDynamicEntry();
	void setRead(bool allowed=true);
	void setOwner(bool isIt = true);
	void setWrite(bool allowed = true);
	void setAssets(bool allowed = true);
	void setExecute(bool allowed = true);
	void setRemoval(bool allowed = true);
	bool getRead();
	bool getRemoval();
	bool getAssets();
	bool getWrite();
	bool getExecute();
	bool getVoting();
	bool getOwnership();
	void setVoting(bool isIt = true);
};

class CAccessEntry
{
private:
	std::vector<uint8_t> mSDID;
	AEFlags mFlags;
	std::mutex mGuardian;
	uint64_t mVersion;
	std::vector<std::uint64_t> mPollIDs;

	//There can be extensions to both the CAccessEntries and CRightsTokens
	//std::vector<uint8_t> mExtensions;//each extension has its own byte-array
	
	//version >= 2 elements follow:

public:

	/*
	* IMPORTANT: ownership is controlled by the 'owner' field of the CSecDescriptor. Not through ACEs.
	*/
	bool setPermisions(bool write, bool execute, bool read, bool removal, bool assets);
	std::vector<uint8_t> getPackedData();
	static std::shared_ptr<CAccessEntry> instantiate(std::vector<uint8_t> bytes);

	//Entry with an empty 'sdID' means 'everyone'.
	CAccessEntry(std::vector<uint8_t> sdID= std::vector<uint8_t>(), bool write = false, bool read = true, bool execute = false, bool assets=false);
	CAccessEntry(const CAccessEntry& sibling);
	CAccessEntry& operator = (const CAccessEntry& t);
	std::vector<uint8_t> getSDID();
	void initFields();
	AEFlags getAccessFlags();
	void setIsDynamic(bool isIt);
	bool getIsDynamic();
	void grantOwnership(bool doit = true);
	void grantWrite(bool doit = true);
	void grantRead(bool doit = true);
	void grantExecute(bool doit = true);
	void grantAssets(bool doit = true);
	void grantRemoval(bool doit = true);
	void grantPollAccess(uint64_t pollID);
	bool removePollAccess(uint64_t pollID);
	bool getIsPollAccessGranted(uint64_t pollID);
	//do NOT return per Pointer modify always through CAccessEntrie's setPermissions() as might require additional computations
	
};