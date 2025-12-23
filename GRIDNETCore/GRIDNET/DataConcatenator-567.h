#pragma once
#include <list>
#include <vector>
#include <cassert>
#include <string>
namespace SE {
	class CScriptEngine;
}
class CDataConcatenator
{
private:
	std::vector<uint8_t> mInternalTemp;
	uint64_t mOffset;
	uint64_t mSizeLimit;
public:
	CDataConcatenator(uint64_t mSizeLimit=0);
	void reset();
	bool addGSC(bool val);
	bool addGSC(uint32_t val);

	template<typename T>
	bool add(T val,bool gridScriptCompatibilityMode=true);
	bool add(std::string str);
	bool add(std::vector<uint8_t> vec);
	bool addGSC(std::vector<uint8_t> vec);
	bool addStackElement(void *scriptEngine,uint32_t stackDepth);
	std::vector<uint8_t> getData();
	bool addBigInt(BigInt val, bool gridScriptCompatibilityMode=true);
	bool addBigFloat(BigFloat val, bool gridScriptCompatibilityMode=true);
	bool addBigSInt(BigSInt val, bool gridScriptCompatibilityMode=true);
};

inline bool CDataConcatenator::addBigInt(BigInt val, bool gridScriptCompatibilityMode)
{
	std::vector<uint8_t> bytes = CTools::getInstance()->BigIntToBytes(val);

	mInternalTemp.resize(mInternalTemp.size() + bytes.size());
	std::memcpy(mInternalTemp.data() + mOffset, &mInternalTemp[0], bytes.size());
	return true;
}
inline bool CDataConcatenator::addBigFloat(BigFloat val, bool gridScriptCompatibilityMode)
{
	std::vector<uint8_t> bytes = CTools::getInstance()->BigFloatToBytes(val);

	mInternalTemp.resize(mInternalTemp.size() + bytes.size());
	std::memcpy(mInternalTemp.data() + mOffset, &mInternalTemp[0], bytes.size());
	return true;
}
inline bool CDataConcatenator::addBigSInt(BigSInt val, bool gridScriptCompatibilityMode)
{
	std::vector<uint8_t> bytes = CTools::getInstance()->BigSIntToBytes(val);

	mInternalTemp.resize(mInternalTemp.size() + bytes.size());
	std::memcpy(mInternalTemp.data() + mOffset, &mInternalTemp[0], bytes.size());
	return true;
}
template<typename T>
inline bool CDataConcatenator::add(T val, bool gridScriptCompatibilityMode)
{

	if (gridScriptCompatibilityMode)
	{
		uint64_t var64Bit = 0;
		uint64_t size = sizeof(val);


		if (size == 1)
			var64Bit = static_cast<uint64_t>(val);
		else
		if (size == 4)
			var64Bit = static_cast<uint64_t>(val);
		else if (size == 8)
			var64Bit = val;
		else
		 assertGN(false);

		if (mSizeLimit != 0)
			if (mInternalTemp.size() + sizeof(var64Bit) > mSizeLimit)
				return false;
		mInternalTemp.resize(mInternalTemp.size() + sizeof(var64Bit));
		std::memcpy(mInternalTemp.data() + mOffset, &var64Bit, sizeof(var64Bit));
		mOffset += sizeof(var64Bit);
		return true;
	}
	else
	{
		mInternalTemp.resize(mInternalTemp.size() + sizeof(T));
		std::memcpy(mInternalTemp.data() + mOffset, &val, sizeof(T));
		return true;

	}
}
