#pragma once
#include <vector>
#include <mutex>
#include "enums.h"

class CVMMetaEntry
{
public:
	CVMMetaEntry(eVMMetaEntryType::eVMMetaEntryType eType,  uint64_t version=1, uint64_t reqID = 0, uint64_t appID = 0, std::vector<uint8_t> vmID= std::vector<uint8_t>());
	std::vector<uint8_t> getVMID();
	uint64_t getReqID();
	uint64_t getProcessID();
	std::vector <std::vector<uint8_t>> getFields();
	eVMMetaEntryType::eVMMetaEntryType  getType();
	void addField(std::vector<uint8_t> field);

private:
	std::mutex mGuardian;
	std::vector<uint8_t> mVMID;
	uint64_t mVersion;
	std::vector <std::vector<uint8_t>> mDataFields;
	uint64_t mReqID;
	uint64_t mAppID;
	eVMMetaEntryType::eVMMetaEntryType mType;

};
