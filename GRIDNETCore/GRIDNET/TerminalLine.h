#pragma once
#include "enums.h"
#include <mutex>
class CDTI;

class CTerminalLine
{
private:
	std::mutex mFieldsGuardian;
	std::string mText;
	bool mEndLine;
	bool mIncludeOwner;
	eViewState::eViewState mView;
	std::string mForcedOwnerName;
	bool mBroadcastToAllTerminals;
	bool mAsynchronous;
	std::shared_ptr<CDTI>mDTI;
	std::string mOwnerName;
	eViewState::eViewState  mDefaultView;
	eBlockchainMode::eBlockchainMode  mBlockchainMode;

	uint64_t mPriority;//as discuseed with the OldWizard on 06.07.21 the priority is now neglected for performance and implementation efficiency reasons. Thus a standard std::queue is to be employed.
public:
	CTerminalLine(std::string str = "", bool endLine = true, bool includeOwner = true, eViewState::eViewState view = eViewState::eViewState::unspecified, std::string forcedOwnername = "", bool broadcastToAllTerminals = false, bool asynchronous = true, uint64_t priority=1, std::shared_ptr<CDTI> dti=nullptr, std::string ownerName="", eViewState::eViewState  defaultView = eViewState::eventView, eBlockchainMode::eBlockchainMode blockchainMode= eBlockchainMode::LocalData);
	eBlockchainMode::eBlockchainMode getBlockchainMode();
	std::string getOwnerName();
	eViewState::eViewState getDefaultView();
	void setDTI(std::shared_ptr<CDTI> dti);
	std::shared_ptr<CDTI> getDTI();
	std::string getText();
	bool getEndLine();
	bool getIncludeOwner();
	eViewState::eViewState getView();
	std::string getForcedOwnerName();
	bool getBroadcastToAllTerminals();
	bool getAsynchronous();
	uint64_t getPriority();

};
