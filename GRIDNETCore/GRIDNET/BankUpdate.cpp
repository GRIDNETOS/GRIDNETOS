#include "BankUpdate.h"


void CBankUpdate::initFields()
{
	mBankID = 0;
	mVersion = 1;
}
CBankUpdate::CBankUpdate()
{
	initFields();
}
CBankUpdate::CBankUpdate(uint64_t bankID,std::vector<uint8_t> preCachedSeedHash, uint64_t preCachedSeedHashDepth)
{
	initFields();
	mBankID = bankID;
    mPreCachedSeedHash = preCachedSeedHash;
    mPreCachedSeedHashDepth = preCachedSeedHashDepth;
	mVersion = 1;
}

uint64_t CBankUpdate::getPreCachedSeedHashDepth()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mPreCachedSeedHashDepth;
}

CBankUpdate::setPreCachedSeedHashDepth(uint64_t depth)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mPreCachedSeedHashDepth = depth;
}

std::vector<uint8_t> CBankUpdate::getPoolID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mPoolID;
}

CBankUpdate::setPoolID(std::vector<uint8_t> id)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mPoolID = id;
}

uint64_t CBankUpdate::getBankID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mBankID;
}

CBankUpdate::setBankID(uint64_t bankID)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mBankID = bankID;
}

/// <summary>
/// Instantiates a CBankUpdate from BER-encoded data.
/// </summary>
/// <param name="packedData"></param>
/// <returns></returns>
std::shared_ptr<CBankUpdate> CBankUpdate::instantiate(std::vector<uint8_t> packedData)
{
	try {
		std::shared_ptr<CBankUpdate> update = std::make_shared<CBankUpdate>();

		Botan::BER_Decoder dec1 = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE);
		Botan::BER_Decoder dec2 = dec1.decode(update->mVersion).start_cons(Botan::ASN1_Tag::SEQUENCE);

		switch (token->mVersion)
		{
		case 1:

			//decode required-fields - BEGIN
			dec2.decode(reinterpret_cast<size_t&>(update->mBankID));
			dec2.decode(update->mPreCachedSeedHash, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(reinterpret_cast<size_t&>(update->mPreCachedSeedHashDepth));
			dec2.decode(update->mPoolID, Botan::ASN1_Tag::OCTET_STRING);
			//decode required-fields - END

			//finalize
			dec2.end_cons().end_cons();
			dec2.verify_end();
			return token;

		default:
			return nullptr;
			break;
		}
	}
	catch (...)
	{
		return nullptr;
	}
}

/// <summary>
/// Gets BER encoding.
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CBankUpdate::getPackedData()
{
    std::lock_guard<std::mutex> lock(mGuardian);
	Botan::DER_Encoder enc;

	enc.start_cons(Botan::ASN1_Tag::SEQUENCE).
		encode(mVersion)
		.start_cons(Botan::ASN1_Tag::SEQUENCE);

	//encode required-fields - BEGIN
	enc.encode(mBankID)
		.encode(mPreCachedSeedHash, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mPreCachedSeedHashDepth)
		.encode(mPoolID, Botan::ASN1_Tag::OCTET_STRING);
	//encode required-fields - END

	//finalize 
	enc.end_cons().end_cons();

	return enc.get_contents_unlocked();
}

uint64_t CBankUpdate::getVersion()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVersion;
}

 CBankUpdate::setVersion(uint64_t version)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mVersion = version;
}

