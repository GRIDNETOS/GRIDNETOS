#include "VMMetaEntry.h"

CVMMetaEntry::CVMMetaEntry(eVMMetaEntryType::eVMMetaEntryType eType, uint64_t version , uint64_t reqID ,uint64_t appID, std::vector<uint8_t> vmID)
{
	mType = eType;
	mVersion = version;
	mReqID = reqID;
	mAppID = appID;
	mVMID = vmID;
}

std::vector<uint8_t>  CVMMetaEntry::getVMID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVMID;
}

uint64_t CVMMetaEntry::getReqID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mReqID;
}

uint64_t CVMMetaEntry::getProcessID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mAppID;
}

std::vector <std::vector<uint8_t>> CVMMetaEntry::getFields()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDataFields;
}

eVMMetaEntryType::eVMMetaEntryType CVMMetaEntry::getType()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mType;
}

void CVMMetaEntry::addField(std::vector<uint8_t> field)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDataFields.push_back(field);
}
