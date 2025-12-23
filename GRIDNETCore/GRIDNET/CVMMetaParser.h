#pragma once
#include <vector>
#include <mutex>
#include "enums.h"


class CVMMetaSection;
class CVMMetaEntry;

class CVMMetaParser
{
public:
	CVMMetaParser();
	std::vector<std::shared_ptr<CVMMetaEntry>> getEntriesByRequestID(uint64_t, eVMMetaEntryType::eVMMetaEntryType eType);
	std::vector<std::shared_ptr< CVMMetaSection>> decode(std::vector<uint8_t> data);
	void reset();
	uint64_t getVersion();
private:
	std::vector<std::shared_ptr< CVMMetaSection>> mSections;
	std::mutex mGuardian;
	size_t mVersion;
	size_t mSuppSectionsVersion;

};