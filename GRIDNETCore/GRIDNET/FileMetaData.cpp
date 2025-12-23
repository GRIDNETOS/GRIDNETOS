#include "FileMetaData.h"

CFileMetaData::CFileMetaData()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	initFields();
}

bool CFileMetaData::getOwnerCanReproduce()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLegalRights.mOwnerCanReproduce;
}

bool CFileMetaData::getOwnerCanSell()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLegalRights.mOwnerCanSell;
}

bool CFileMetaData::getOwnerCanUseCommercially()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLegalRights.mOwnerCanUseCommercially;
}

bool CFileMetaData::getOwnerCanDistribute()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLegalRights.mOwnerCanDistribute;
}

bool CFileMetaData::getOwnerCanDisplayPublicly()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLegalRights.mOwnerCanDisplayPublicly;
}

bool CFileMetaData::getOwnerCanPerformPublicly()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLegalRights.mOwnerCanPerformPublicly;
}

bool CFileMetaData::getOwnerCanPrepareDerivativeWorks()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLegalRights.mOwnerCanPrepareDerivativeWorks;
}

bool CFileMetaData::getOwnerCanLicense()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mLegalRights.mOwnerCanLicense;
}

void CFileMetaData::setOwnerCanReproduce(bool allowed)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLegalRights.mOwnerCanReproduce = allowed;
}

void CFileMetaData::setOwnerCanSell(bool allowed)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLegalRights.mOwnerCanSell = allowed;
}

void CFileMetaData::setOwnerCanUseCommercially(bool allowed)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLegalRights.mOwnerCanUseCommercially = allowed;
}

void CFileMetaData::setOwnerCanDistribute(bool allowed)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLegalRights.mOwnerCanDistribute = allowed;
}

void CFileMetaData::setOwnerCanDisplayPublicly(bool allowed)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLegalRights.mOwnerCanDisplayPublicly = allowed;
}

void CFileMetaData::setOwnerCanPerformPublicly(bool allowed)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLegalRights.mOwnerCanPerformPublicly = allowed;
}

void CFileMetaData::setOwnerCanPrepareDerivativeWorks(bool allowed)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLegalRights.mOwnerCanPrepareDerivativeWorks = allowed;
}

void CFileMetaData::setOwnerCanLicense(bool allowed)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mLegalRights.mOwnerCanLicense = allowed;
}

void CFileMetaData::initFields()
{
	mVersion = 1;
}

CFileMetaData::CFileMetaData(const CFileMetaData& sibling)
{
	mLegalRights.mOwnerCanReproduce = sibling.mLegalRights.mOwnerCanReproduce;
	mLegalRights.mOwnerCanSell = sibling.mLegalRights.mOwnerCanSell;
	mLegalRights.mOwnerCanUseCommercially = sibling.mLegalRights.mOwnerCanUseCommercially;
	mLegalRights.mOwnerCanDistribute = sibling.mLegalRights.mOwnerCanDistribute;
	mLegalRights.mOwnerCanDisplayPublicly = sibling.mLegalRights.mOwnerCanDisplayPublicly;
	mLegalRights.mOwnerCanPerformPublicly = sibling.mLegalRights.mOwnerCanPerformPublicly;
	mLegalRights.mOwnerCanPrepareDerivativeWorks = sibling.mLegalRights.mOwnerCanPrepareDerivativeWorks;
	mLegalRights.mOwnerCanLicense = sibling.mLegalRights.mOwnerCanLicense;
	mVersion = sibling.mVersion;
	mDescription = sibling.mDescription;
	mDataURI = sibling.mDataURI;
	mPreviewURI = sibling.mPreviewURI;
	mIcon = sibling.mIcon;
	mName = sibling.mName;
	mLegalRights = sibling.mLegalRights;

	for (uint64_t i = 0; i < sibling.mAttributes.size(); i++)
	{
		mAttributes.push_back(sibling.mAttributes[i]);
	}
}

std::string CFileMetaData::getDescription()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDescription;
}

std::string CFileMetaData::getDataURI()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return std::string();
}

std::string CFileMetaData::getPreviewURI()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mPreviewURI;
}

std::vector<uint8_t> CFileMetaData::getIcon()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mIcon;
}

std::string CFileMetaData::getName()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mName;
}

void CFileMetaData::setDescription(const std::string & value)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDescription = value;
}

void CFileMetaData::setDataURI(const std::string& value)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDataURI = value;
}

void CFileMetaData::setPreviewURI(const std::string& value)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mPreviewURI = value;
}

void CFileMetaData::setIcon(const std::vector<uint8_t>& value)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mIcon = value;
}

void CFileMetaData::setName(const std::string& value)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mName = value;
}

std::vector<uint8_t> CFileMetaData::getPackedData()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	//Local Variables - BEGIN
	uint8_t flags;
	std::memcpy(&flags, &mLegalRights, 1);
	std::string key, value;
	//Local Variables - END

	Botan::DER_Encoder enc;

	enc.start_cons(Botan::ASN1_Tag::SEQUENCE).
		encode(mVersion)
		.encode(static_cast<size_t>(flags))
		.encode(std::vector<uint8_t>(mDescription.begin(), mDescription.end()), Botan::ASN1_Tag::OCTET_STRING)
		.encode(std::vector<uint8_t>(mDataURI.begin(), mDataURI.end()), Botan::ASN1_Tag::OCTET_STRING)
		.encode(std::vector<uint8_t>(mPreviewURI.begin(), mPreviewURI.end()), Botan::ASN1_Tag::OCTET_STRING)
		.encode(std::vector<uint8_t>(mIcon.begin(), mIcon.end()), Botan::ASN1_Tag::OCTET_STRING)
		.encode(std::vector<uint8_t>(mName.begin(), mName.end()), Botan::ASN1_Tag::OCTET_STRING);

	enc.start_cons(Botan::ASN1_Tag::SEQUENCE);


	for (uint64_t i = 0; i < mAttributes.size(); i++)
	{
		key = std::get<0>(mAttributes[i]);
		value = std::get<1>(mAttributes[i]);

		enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
		enc.encode(std::vector<uint8_t>(key.begin(), key.end()), Botan::ASN1_Tag::OCTET_STRING)//IMPORTANT: use false parameter during serialization
			.encode(std::vector<uint8_t>(value.begin(), value.end()), Botan::ASN1_Tag::OCTET_STRING);

		enc.end_cons();
	}

	enc = enc.end_cons();
	enc.end_cons();

	return enc.get_contents_unlocked();
}
bool CFileMetaData::addAttribute(const std::string& key, const std::string& value)
{
	if (!key.size() || !value.size())
		return false;

	std::lock_guard<std::mutex> lock(mGuardian);

	mAttributes.push_back(std::make_tuple(key, value));

	return true;
}

std::string CFileMetaData::getRendering(std::string newLine,uint64_t terminalWindth, bool useAnsiColors)
{
	std::lock_guard<std::mutex> lock(mGuardian);

	//Local Variables - BEGIN
	std::string toRet;
	std::shared_ptr<CTools> tools = CTools::getTools()->getInstance();
	std::vector <std::vector<std::string>> rows,rows2;
	std::vector<std::string> row;
	eColor::eColor headerColor = eColor::none;
	eColor::eColor tableHeaderColor = eColor::none;
	eColor::eColor entriesCountColor = eColor::none;
	eColor::eColor sectionHeaderColumn = eColor::none;
	if (useAnsiColors)
	{
		tableHeaderColor = eColor::lightPink;
		entriesCountColor = eColor::blue;
		headerColor = eColor::lightCyberWine;
		sectionHeaderColumn = eColor::orange;
	}
	//Local Variables - BEGIN

	//Operational Logic -  BEGIN
	
	//Essentials - BEGIN
	row.push_back(tools->getColoredString("          Essentials", sectionHeaderColumn));
	rows.push_back(row);

	if (mName.size())
	{
		row.clear();
		row.push_back(tools->getColoredString("Name:", headerColor));
		row.push_back(mName);
		rows.push_back(row);
	}

	if (mDescription.size())
	{
		row.clear();
		row.push_back(tools->getColoredString("Description:", headerColor));
		row.push_back(mDescription);
		rows.push_back(row);
	}

	if (mDataURI.size())
	{
		row.clear();
		row.push_back(tools->getColoredString("URI:", headerColor));
		row.push_back(mDataURI);
		rows.push_back(row);
	}


	if (mPreviewURI.size())
	{
		row.clear();
		row.push_back(tools->getColoredString("PreviewURI:", headerColor));
		row.push_back(mPreviewURI);
		rows.push_back(row);
	}

	if (mIcon.size())
	{
		row.clear();
		row.push_back(tools->getColoredString("Icon (Base64-Encoded):", headerColor));
		row.push_back(tools->base64CheckEncode(mIcon));
		rows.push_back(row);
	}


	
	std::string allowed = tools->getColoredString(u8"✅", eColor::lightGreen);
	std::string forbidden = tools->getColoredString(u8"❎", eColor::cyborgBlood);
	std::string legalChecks = "";
	std::string rightsSpacing = "  ";

	row.clear();

	row.push_back(tools->getColoredString("Reproduction:", sectionHeaderColumn) + (mLegalRights.mOwnerCanReproduce ? allowed : forbidden));
	row.push_back(tools->getColoredString("Sale:", sectionHeaderColumn) + (mLegalRights.mOwnerCanSell ? allowed : forbidden));
	rows2.push_back(row);
	row.clear();
	row.push_back(tools->getColoredString("Commercial Use:", sectionHeaderColumn) + (mLegalRights.mOwnerCanUseCommercially ? allowed : forbidden));
	row.push_back(tools->getColoredString("Distribution:", sectionHeaderColumn) + (mLegalRights.mOwnerCanDistribute ? allowed : forbidden));
	rows2.push_back(row);
	row.clear();
	row.push_back(tools->getColoredString("Public Display:", sectionHeaderColumn) + (mLegalRights.mOwnerCanDisplayPublicly ? allowed : forbidden));
	row.push_back(tools->getColoredString("Public Performance:", sectionHeaderColumn) + (mLegalRights.mOwnerCanPerformPublicly ? allowed : forbidden));
	rows2.push_back(row);

	row.clear();
	row.push_back(tools->getColoredString("Licensing:",sectionHeaderColumn) + (mLegalRights.mOwnerCanLicense    ? allowed : forbidden));
	row.push_back(tools->getColoredString("Derivative Works: ", sectionHeaderColumn) + (mLegalRights.mOwnerCanPrepareDerivativeWorks ? allowed : forbidden));
	rows2.push_back(row);

	
	//Essentials - END

	//Extended Meta-Data - BEGIN
	for (uint64_t i = 0; i < mAttributes.size(); i++)
	{
		row.clear();
		row.push_back(tools->getColoredString(std::get<0>(mAttributes[i])+":", headerColor));
		row.push_back(std::get<1>(mAttributes[i]));
		rows.push_back(row);
	}
	//Extended Meta-Data - END

	toRet += tools->genTable(rows, eColumnFieldAlignment::left, false, tools->getColoredString("[ NFT Meta-Data ]" , entriesCountColor), newLine, terminalWindth, std::vector<uint64_t>(), true);
	toRet +=tools->genTable(rows2, eColumnFieldAlignment::right, false, tools->getColoredString("[ Legal Rights ]" , entriesCountColor), newLine, terminalWindth);
	toRet += newLine;
	return toRet;
	//Operational Logic -  END
}


std::shared_ptr<CFileMetaData> CFileMetaData::instantiate(const std::vector<uint8_t> & packedData)
{
	//local variables - BEGIN
	std::shared_ptr<CFileMetaData> metaData;
	std::vector<uint8_t> temp1, temp2;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	LegalRights rights;
	size_t ts = 0;
	//local variables - END


	//Operational Logic - BEGIN
	try {
		metaData = std::make_shared<CFileMetaData>();
		
		Botan::BER_Decoder dec1 = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE).decode(metaData->mVersion);

		if (metaData->mVersion == 1)
		{
			dec1.decode(ts);
			std::memcpy(&(metaData->mLegalRights), &ts, 1);

			dec1.decode(temp1, Botan::ASN1_Tag::OCTET_STRING);
			metaData->mDescription = tools->bytesToString(temp1);

			dec1.decode(temp1, Botan::ASN1_Tag::OCTET_STRING);
			metaData->mDataURI = tools->bytesToString(temp1);

			dec1.decode(temp1, Botan::ASN1_Tag::OCTET_STRING);
			metaData->mPreviewURI = tools->bytesToString(temp1);

			dec1.decode(metaData->mIcon, Botan::ASN1_Tag::OCTET_STRING);
	
			dec1.decode(temp1, Botan::ASN1_Tag::OCTET_STRING);
			metaData->mName = tools->bytesToString(temp1);

			Botan::BER_Decoder dec2 = Botan::BER_Decoder(dec1.get_next_object().value);
			
			while (dec2.more_items())
			{
				Botan::BER_Decoder dec3 = Botan::BER_Decoder(dec2.get_next_object().value);

				dec3.decode(temp1, Botan::ASN1_Tag::OCTET_STRING);
				dec3.decode(temp2, Botan::ASN1_Tag::OCTET_STRING);
				metaData->addAttribute(std::string(temp1.begin(), temp1.end()), std::string(temp2.begin(), temp2.end()));
			}

			//dec2.end_cons();
			dec2.verify_end();

		}
		return metaData;

	}
	catch (...)
	{
		return nullptr;
	}
	//Operational Logic - END
}

LegalRights::LegalRights(const LegalRights& sibling)
{
	std::memcpy(this, &sibling, sizeof(LegalRights));
}

LegalRights::LegalRights()
{
	std::memset(this, 255, sizeof(LegalRights));
}

LegalRights& LegalRights::operator=(const LegalRights& t)
{
	std::memcpy(this, &t, sizeof(LegalRights));
	return *this;
}
