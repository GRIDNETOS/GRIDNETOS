#pragma once
#include "stdafx.h"
#include <vector>
#include "enums.h"
#include <vector>
#include <memory>
#include <mutex>

struct dfsFlags
{
	bool suspendThrows : 1;//suspendThrows during execution of this command
	bool error : 1;
	bool subThreadProcessing: 1;//when is to be processed by a sub-thread
	bool reserved2 : 1;
	bool reserved3 : 1;
	bool reserved4 : 1;
	bool reserved5 : 1;
	bool reserved6 : 1;

	dfsFlags(const dfsFlags& sibling) {
		std::memcpy(this, &sibling, sizeof(dfsFlags));
	}

	dfsFlags()
	{
		std::memset(this, 0, sizeof(dfsFlags));
	}
};

class CDFSMsg
{
public:

	//construction
	CDFSMsg(eDFSCmdType::eDFSCmdType type);
	CDFSMsg(const CDFSMsg& sibling);
	std::vector<uint8_t> getThreadID();
	void setThreadID(std::vector<uint8_t> id);
	CDFSMsg();


	//ID
	uint64_t getRequestID();
	void setRequestID(uint64_t reqID);
    eDFSCmdType::eDFSCmdType getType();

	//data
	void setData1(std::vector<uint8_t> data);
	void setData2(std::vector<uint8_t> data);
	std::vector<uint8_t> getData1();
	std::vector<uint8_t> getData2();


	//(de)serialization
	std::vector<uint8_t> getPackedData();
	static std::shared_ptr<CDFSMsg> instantiate(std::vector<uint8_t> data);
	dfsFlags getFlags();
	void setFlags(dfsFlags flags);

private:
	std::vector<uint8_t> mThreadID;//empty by default
	dfsFlags mFlags;
	void initFields();
	eDFSCmdType::eDFSCmdType mType;
	uint64_t mVersion ;
	std::mutex mGuardian;
	std::vector<uint8_t> mData1;
	std::vector<uint8_t> mData2;
	uint64_t mRequestID;

};
