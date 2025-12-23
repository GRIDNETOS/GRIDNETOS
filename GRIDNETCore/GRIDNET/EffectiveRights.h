#pragma once
class CEffectiveRights
{
private:
	//these pretty much correspond to what is available within AEFlags
	bool mRead : 1;
	bool mWrite : 1;
	bool mExecute : 1;
	bool mAssets : 1;//valid only when associated with a state-domain.
	bool mOwnership : 1;//DYNAMIC (not available with ACE entries).
	bool mSpending : 1;//valid only for when accessing a state-domain object
	bool mRemoval : 1;
	bool mVoting : 1;
	std::mutex mGuardian;

public:
	static std::shared_ptr<CEffectiveRights> getDefaultRights();
	CEffectiveRights(bool read = true, bool write = false, bool execute = true, bool ownership = false, bool spending=false, bool removal = false, bool voting=false);
	bool getCanRead();
	bool getCanVote();
	bool getCanSpend();
	bool getCanRemove();
	bool getCanWrite();
	bool getCanExecute();
	std::string toString(std::string newLine="\r\n");
	bool getIsOwner();

	void setCanSpend(bool isIt = true);
	void setCanRemove(bool isIt = true);
	void setCanRead(bool isIt=true);
	void setCanWrite(bool isIt = true);
	void setCanExecute(bool isIt = true);
	void setIsOwner(bool isIt = true);
	void setCanVote(bool isIt = true);

};