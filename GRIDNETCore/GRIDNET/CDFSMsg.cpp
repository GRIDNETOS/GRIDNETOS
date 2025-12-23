#include "CDFSMsg.h"



CDFSMsg::CDFSMsg(eDFSCmdType::eDFSCmdType type)
{
	initFields();
	mType = type;
}

CDFSMsg::CDFSMsg(const CDFSMsg& sibling)
{
	mData1 = sibling.mData1;
	mData2 = sibling.mData2;
	mType = sibling.mType;
	mVersion = sibling.mVersion;
	mThreadID = sibling.mThreadID;
}

std::vector<uint8_t> CDFSMsg::getThreadID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mThreadID;
}

void CDFSMsg::setThreadID(std::vector<uint8_t> id)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mThreadID = id;
}

CDFSMsg::CDFSMsg()
{
	mVersion = 1;
}

uint64_t CDFSMsg::getRequestID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mRequestID;
}

void CDFSMsg::setRequestID(uint64_t reqID)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	 mRequestID= reqID;
}

eDFSCmdType::eDFSCmdType CDFSMsg::getType()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mType;
}

/// <summary>
/// pathA
/// </summary>
/// <param name="data"></param>
void CDFSMsg::setData1(std::vector<uint8_t> data)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mData1 = data;
}
/// <summary>
/// pathB/ content
/// </summary>
/// <param name="data"></param>
void CDFSMsg::setData2(std::vector<uint8_t> data)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mData2 = data;
}
/// <summary>
/// pathA
/// </summary>
/// <param name="data"></param>
std::vector<uint8_t> CDFSMsg::getData1()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mData1;
}
/// <summary>
/// pathB/ content
/// </summary>
/// <param name="data"></param>
std::vector<uint8_t> CDFSMsg::getData2()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mData2;
}

std::vector<uint8_t> CDFSMsg::getPackedData()
{
	Botan::DER_Encoder enc;
	std::vector<uint8_t> flagsBytes(1);
	flagsBytes[0] = reinterpret_cast<uint8_t&>(mFlags);

	enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
		.encode(static_cast<size_t>(mVersion))
		.encode(static_cast<size_t>(mType))
		.start_cons(Botan::ASN1_Tag::SEQUENCE)
		.encode(static_cast<size_t>(mRequestID))
		.encode(mData1, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mData2, Botan::ASN1_Tag::OCTET_STRING)
		.encode(flagsBytes, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mThreadID, Botan::ASN1_Tag::OCTET_STRING)
		.end_cons().end_cons();

	return enc.get_contents_unlocked();
}




std::shared_ptr<CDFSMsg> CDFSMsg::instantiate(std::vector<uint8_t> data)
{
	try {
		std::shared_ptr<CDFSMsg> msg = std::make_shared<CDFSMsg>();
		size_t version,typE= 0;
		Botan::BER_Decoder dec = Botan::BER_Decoder(data).start_cons(Botan::ASN1_Tag::SEQUENCE).
			decode(version).decode(typE);
		std::vector<uint8_t>flagBytes;

		msg->mVersion= version;
		msg->mType = static_cast<eDFSCmdType::eDFSCmdType>(typE);

		Botan::BER_Object obj;
		if (msg->mVersion == 1)
		{
			size_t result = 0;
			std::vector<uint8_t> adr;
			obj = dec.get_next_object();
			if (obj.type_tag != Botan::ASN1_Tag::SEQUENCE)
				return nullptr;

			Botan::BER_Decoder  dec2 = Botan::BER_Decoder(obj.value);
		
			dec2.decode(msg->mRequestID);
			dec2.decode(msg->mData1, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(msg->mData2, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(flagBytes, Botan::ASN1_Tag::OCTET_STRING);

			if (flagBytes.size() != 1)
				return nullptr;
			reinterpret_cast<uint8_t&>(msg->mFlags) = flagBytes[0];

			dec2.decode(msg->mThreadID, Botan::ASN1_Tag::OCTET_STRING);
			dec2.verify_end();
		}
		else 	return nullptr;
		return msg;
	}
	catch (...)
	{
		return nullptr;
	}
}

dfsFlags CDFSMsg::getFlags()
{
	return mFlags;
}

void CDFSMsg::setFlags(dfsFlags flags)
{
	mFlags = flags;
}

void CDFSMsg::initFields()
{
	mVersion = 1;
}


