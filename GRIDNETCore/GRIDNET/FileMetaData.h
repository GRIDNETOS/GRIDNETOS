#pragma once
#include "stdafx.h"
#include <memory>
#include "enums.h"
#include <mutex>
class CTools;
class CStateDomain;
class CCryptoFactory;

struct LegalRights
{
	bool mOwnerCanReproduce : 1;
	bool mOwnerCanSell : 1;
	bool mOwnerCanUseCommercially : 1;
	bool mOwnerCanDistribute : 1;
	bool mOwnerCanDisplayPublicly : 1;
	bool mOwnerCanPerformPublicly : 1;
	bool mOwnerCanPrepareDerivativeWorks : 1;
	bool mOwnerCanLicense : 1;

	LegalRights(const LegalRights& sibling);
	LegalRights();
	LegalRights& operator = (const LegalRights& t);
};

class CFileMetaData
{

private:

	LegalRights mLegalRights;

	std::mutex mGuardian;
	uint64_t mVersion;
	std::string mDescription;
	std::string mDataURI;
	std::string mPreviewURI;
	std::vector<uint8_t> mIcon;
	std::string mName;
	std::vector<std::tuple<std::string, std::string>> mAttributes;
	void initFields();

public:

	CFileMetaData(const CFileMetaData& sibling);
	std::string getDescription();
	std::string getDataURI();
	std::string getPreviewURI();
	std::vector<uint8_t> getIcon();
	std::string getName();

	void setDescription(const std::string &value);
	void setDataURI(const std::string& value);
	void setPreviewURI(const std::string& value);
	void setIcon(const std::vector<uint8_t>& value);
	void setName(const std::string& value);

	static std::shared_ptr<CFileMetaData> instantiate(const std::vector<uint8_t> &bytes);
	CFileMetaData();

	bool getOwnerCanReproduce();
	bool getOwnerCanSell();
	bool getOwnerCanUseCommercially();
	bool getOwnerCanDistribute();
	bool getOwnerCanDisplayPublicly();
	bool getOwnerCanPerformPublicly();
	bool getOwnerCanPrepareDerivativeWorks();
	bool getOwnerCanLicense();

	void setOwnerCanReproduce(bool allowed = true);
	void setOwnerCanSell(bool allowed = true);
	void setOwnerCanUseCommercially(bool allowed = true);
	void setOwnerCanDistribute(bool allowed = true);
	void setOwnerCanDisplayPublicly(bool allowed = true);
	void setOwnerCanPerformPublicly(bool allowed = true);
	void setOwnerCanPrepareDerivativeWorks(bool allowed = true);
	void setOwnerCanLicense(bool allowed = true);

	std::vector<uint8_t> getPackedData();
	bool addAttribute(const std::string & key, const std::string & value);

	std::string getRendering(std::string newLine,uint64_t terminalWindth=80, bool useAnsiColors=true);

};