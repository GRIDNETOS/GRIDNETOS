#pragma once
#include <vector>
namespace eDFSEntType
{
	enum eDFSEntType
	{
		block,
		transaction,
		verifiable,
		msg,
		chainProof,
		longestPath,
		sec,
		receiptBody,
		receiptID,
		ping,
		hello,//contains client information, version etc.
		bye,
		QRIntentResponse
	};

}
namespace eNetReqType {
	enum eNetReqType
	{
		notify,
		request,
		process
	};
}
class CNetMsg
{
private:
	uint8_t mVersion;
	eNetEntType::eNetEntType mEntityType;
	eNetReqType::eNetReqType mRequestType;
	std::vector<uint8_t> mData;
	std::vector<uint8_t> mSecData;
	uint64_t mValue;// any kind of unsigned numerical value. might contain Total Pow for a proposed chain etc.
	uint64_t mBid;//how much ERG to use for data transmission
public:
	CNetMsg(eNetEntType::eNetEntType entityType, eNetReqType::eNetReqType reqType, std::vector<uint8_t> data = std::vector<uint8_t>(), std::vector<uint8_t> secData = std::vector<uint8_t>(), uint64_t bid = 0, uint64_t val = 0);
	CNetMsg();
	eNetEntType::eNetEntType getEntityType() const;
	eNetReqType::eNetReqType  getRequestType() const;
	uint64_t getValue();
	void setValue(uint64_t val);
	uint64_t geBid();
	void setBid(uint64_t val);
	std::vector<uint8_t> getData() const;
	bool setData(std::vector<uint8_t> data);
	bool setSecData(std::vector<uint8_t> data);
	std::vector<uint8_t> getSecData();
	std::vector<uint8_t> getPackedData();
	static bool instantiate(std::vector<uint8_t> packedData, CNetMsg& msg);
	inline friend  bool operator==(const CNetMsg& lhs, const CNetMsg& rhs) {

		return (lhs.mVersion == rhs.mVersion &&
			lhs.mEntityType == rhs.mEntityType &&
			lhs.mData == rhs.mData &&
			lhs.mSecData == rhs.mSecData &&
			lhs.mValue == rhs.mValue
			);

	}

};
