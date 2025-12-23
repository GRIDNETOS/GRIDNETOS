#pragma once
#include <vector>
#include <mutex>
#include "enums.h"

class CVMMetaEntry;

class CVMMetaSection
{
public:
	CVMMetaSection(eVMMetaSectionType::eVMMetaSectionType eType, uint64_t version);
	void addEntry(std::shared_ptr<CVMMetaEntry> entry);
	std::vector <std::shared_ptr<CVMMetaEntry>> getEntries();
	uint64_t getVersion();
	eVMMetaSectionType::eVMMetaSectionType getType();

	std::vector<uint8_t> getMetaData();
	void setMetaData(std::vector<uint8_t> metadata);
private:
	std::mutex mGuardian;
	eVMMetaSectionType::eVMMetaSectionType mType;
	uint64_t mVersion;
	std::vector<uint8_t> mMetaData;//contains arbitrary bytes. It is advised to store information having section-scope relevance.
	/*
	Examples:
	NOTIFICATIONS:
	e.x directory path for all files delivered within a section OR a token-pool ID, in case the section contains for instance serialized CBankUpdate structures
	targeting the same Token Pool. In such a case we improve transmission's efficiency.
	REQUESTS:
	
	*/
	std::vector <std::shared_ptr<CVMMetaEntry>> mEntries;

};
