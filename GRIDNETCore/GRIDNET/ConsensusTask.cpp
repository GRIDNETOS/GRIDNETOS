#include "stdafx.h"
#include "ConsensusTask.h"


std::shared_ptr<ctFlags> ctFlags::instantiate(std::vector<uint8_t> data)
{
    if (data.size() != 1)
        return nullptr;

    return std::make_shared<ctFlags>(data);
}

std::vector<uint8_t> ctFlags::getPackedData()
{
    std::vector<uint8_t> bytes(1);
    std::memcpy(&bytes[0], this, 1);
    return bytes;
}

CConsensusTask::CConsensusTask(std::string description, eConsensusTaskType::eConsensusTaskType type, uint64_t processID, std::vector<uint8_t> threadID, std::vector<uint8_t> conversationID, ctFlags flags)
{
    mID = CTools::getInstance()->genRandomNumber(1000, 10000);
    mType = type;
    mDescription = description;
    mProcessID = processID;
    mThreadID = threadID;
    mFlags = flags;
	mConversationID = conversationID;
    mVersion = 1;
}

std::shared_ptr<CConsensusTask> CConsensusTask::instantiate(std::vector<uint8_t> packedData)
{
	//Local Variables - BEGIN
    std::shared_ptr<CConsensusTask> msg = std::make_shared<CConsensusTask>();
	size_t version, typE = 0;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::vector<uint8_t> temp;
	std::shared_ptr<ctFlags> flags;
	Botan::BER_Object obj;
	//Local Variables - END

	//Operational Logic - BEGIN
	try {

		Botan::BER_Decoder dec = Botan::BER_Decoder(packedData).start_cons(Botan::ASN1_Tag::SEQUENCE).
			decode(version).decode(typE);
		
		msg->mVersion = version;
		msg->mType = static_cast<eConsensusTaskType::eConsensusTaskType>(typE);

		if (msg->mVersion == 1)
		{
			obj = dec.get_next_object();
			if (obj.type_tag != Botan::ASN1_Tag::SEQUENCE)
				return nullptr;

			Botan::BER_Decoder  dec2 = Botan::BER_Decoder(obj.value);

			dec2.decode(msg->mID);
			dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
			msg->mDescription = tools->bytesToString(temp);
			dec2.decode(msg->mProcessID);
			dec2.decode(msg->mThreadID, Botan::ASN1_Tag::OCTET_STRING);
			dec2.decode(temp, Botan::ASN1_Tag::OCTET_STRING);
		    flags = ctFlags::instantiate(temp);//data checks within
			dec2.decode(msg->mConversationID, Botan::ASN1_Tag::OCTET_STRING);

			if (!flags)
			{
				return nullptr;
			}

			msg->mFlags = *flags;//[to:CodesInChaos]: copy necessary?
			dec2.decode(msg->mThreadID, Botan::ASN1_Tag::OCTET_STRING);
			dec2.verify_end();
		}
		else return nullptr;
		return msg;
	}
	catch (...)
	{
		return nullptr;
	}
	//Operational Logic - END

}

std::vector<uint8_t> CConsensusTask::getPackedData()
{
	//Local Variables - BEGIN
	std::lock_guard<std::mutex> lock(mGuardian);
	Botan::DER_Encoder enc;
	//Local Variables - END

	//Operational Logic - BEGIN
	std::shared_ptr<CTools> tools = CTools::getInstance();
	enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
		.encode(static_cast<size_t>(mVersion))
		.encode(static_cast<size_t>(mType))
		.start_cons(Botan::ASN1_Tag::SEQUENCE)
		//routing additions - BEGIN
		.encode(static_cast<size_t>(mID))
		.encode(tools->stringToBytes(mDescription), Botan::ASN1_Tag::OCTET_STRING)
		.encode(static_cast<size_t>(mProcessID))
		.encode(mThreadID, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mFlags.getPackedData(), Botan::ASN1_Tag::OCTET_STRING)
		.encode(mConversationID, Botan::ASN1_Tag::OCTET_STRING)
		.end_cons()
		.end_cons();
	

	return enc.get_contents_unlocked();
	//Operational Logic - END
}

uint64_t CConsensusTask::getID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mID;
}

void CConsensusTask::setID(uint64_t id)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mID = id;
}

eConsensusTaskType::eConsensusTaskType CConsensusTask::getType()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mType;
}

void CConsensusTask::setType(eConsensusTaskType::eConsensusTaskType type)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mType = type;
}

std::string CConsensusTask::getDescription()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mDescription;
}

void CConsensusTask::setDescription(std::string desc)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mDescription = desc;
}

uint64_t CConsensusTask::getVersion()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mVersion;
}

ctFlags CConsensusTask::getFlags()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mFlags;
}

void CConsensusTask::setFlags(ctFlags flags)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mFlags = flags;
}

uint64_t CConsensusTask::getProcessID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mProcessID;
}

void CConsensusTask::setProcessID(uint64_t id)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mProcessID = id;
}

std::vector<uint8_t> CConsensusTask::getThreadID()
{
	std::lock_guard<std::mutex> lock(mGuardian);
	return mThreadID;
}

void CConsensusTask::setThreadID(std::vector<uint8_t> id)
{
	std::lock_guard<std::mutex> lock(mGuardian);
	mThreadID = id;
}
