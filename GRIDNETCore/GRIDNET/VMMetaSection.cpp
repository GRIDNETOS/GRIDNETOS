#include "VMMetaSection.h"

CVMMetaSection::CVMMetaSection(eVMMetaSectionType::eVMMetaSectionType eType, uint64_t version)
{
	mType = eType;
	mVersion = version;
}

void CVMMetaSection::addEntry(std::shared_ptr<CVMMetaEntry> entry)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mEntries.push_back(entry);
}

std::vector<std::shared_ptr<CVMMetaEntry>> CVMMetaSection::getEntries()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mEntries;
}

uint64_t CVMMetaSection::getVersion()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVersion;
}

eVMMetaSectionType::eVMMetaSectionType CVMMetaSection::getType()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mType;
}

std::vector<uint8_t> CVMMetaSection::getMetaData()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mMetaData;
}

void CVMMetaSection::setMetaData(std::vector<uint8_t> metadata)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mMetaData = metadata;
}
