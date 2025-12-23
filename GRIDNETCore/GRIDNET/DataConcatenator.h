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
	bool addGSC(std::vector<uint8_t> vec);  // FIXED version (use for future versions)
	bool addGSCBugged(std::vector<uint8_t> vec);  // BUGGY version (maintains backward compatibility)
	bool addStackElement(void *scriptEngine,uint32_t stackDepth);
	std::vector<uint8_t> getData();

	// BigInt serialization methods
	bool addBigInt(BigInt val, bool gridScriptCompatibilityMode=true);  // FIXED version (use for v3+)
	bool addBigIntBugged(BigInt val, bool gridScriptCompatibilityMode=true);  // BUGGY version (use for v1-v2)

	// BigFloat serialization methods
	bool addBigFloat(BigFloat val, bool gridScriptCompatibilityMode=true);  // FIXED version (use for v3+)
	bool addBigFloatBugged(BigFloat val, bool gridScriptCompatibilityMode=true);  // BUGGY version (use for v1-v2)

	// BigSInt serialization methods
	bool addBigSInt(BigSInt val, bool gridScriptCompatibilityMode=true);  // FIXED version (use for v3+)
	bool addBigSIntBugged(BigSInt val, bool gridScriptCompatibilityMode=true);  // BUGGY version (use for v1-v2)
};

// ✓ FIXED IMPLEMENTATION (use for Transaction v3+)
// CVE-2025-GRIDNET-001: Proper BigInt authentication in transaction signatures
inline bool CDataConcatenator::addBigInt(BigInt val, bool gridScriptCompatibilityMode)
{
	std::vector<uint8_t> bytes = CTools::getInstance()->BigIntToBytes(val);

	mInternalTemp.resize(mInternalTemp.size() + bytes.size());
	std::memcpy(mInternalTemp.data() + mOffset, bytes.data(), bytes.size());  // ✓ Copy from bytes.data()
	mOffset += bytes.size();  // ✓ Increment offset
	return true;
}

// ❌ BUGGY IMPLEMENTATION (maintains consensus for Transaction v1-v2)
// CVE-2025-GRIDNET-001: ERG Price/Limit fields not authenticated in signature
// This bug allows potential manipulation of ERG bid/limit values after signing
// CRITICAL: Only use for backward compatibility with v1-v2 transactions!
inline bool CDataConcatenator::addBigIntBugged(BigInt val, bool gridScriptCompatibilityMode)
{
	std::vector<uint8_t> bytes = CTools::getInstance()->BigIntToBytes(val);

	mInternalTemp.resize(mInternalTemp.size() + bytes.size());
	std::memcpy(mInternalTemp.data() + mOffset, &mInternalTemp[0], bytes.size());  // ❌ BUG: Copies from start of buffer
	// ❌ BUG: mOffset is NOT incremented (should be: mOffset += bytes.size();)
	return true;
}
// ✓ FIXED IMPLEMENTATION (use for Transaction v3+)
// CVE-2025-GRIDNET-001: Proper BigFloat authentication in transaction signatures
inline bool CDataConcatenator::addBigFloat(BigFloat val, bool gridScriptCompatibilityMode)
{
	std::vector<uint8_t> bytes = CTools::getInstance()->BigFloatToBytes(val);

	mInternalTemp.resize(mInternalTemp.size() + bytes.size());
	std::memcpy(mInternalTemp.data() + mOffset, bytes.data(), bytes.size());  // ✓ Copy from bytes.data()
	mOffset += bytes.size();  // ✓ Increment offset
	return true;
}

// ❌ BUGGY IMPLEMENTATION (maintains consensus for Transaction v1-v2)
// CVE-2025-GRIDNET-001: BigFloat fields not authenticated in signature
// This bug allows potential manipulation of BigFloat values after signing
// CRITICAL: Only use for backward compatibility with v1-v2 transactions!
inline bool CDataConcatenator::addBigFloatBugged(BigFloat val, bool gridScriptCompatibilityMode)
{
	std::vector<uint8_t> bytes = CTools::getInstance()->BigFloatToBytes(val);

	mInternalTemp.resize(mInternalTemp.size() + bytes.size());
	std::memcpy(mInternalTemp.data() + mOffset, &mInternalTemp[0], bytes.size());  // ❌ BUG: Copies from start of buffer
	// ❌ BUG: mOffset is NOT incremented (should be: mOffset += bytes.size();)
	return true;
}

// ✓ FIXED IMPLEMENTATION (use for Transaction v3+)
// CVE-2025-GRIDNET-001: Proper BigSInt authentication in transaction signatures
inline bool CDataConcatenator::addBigSInt(BigSInt val, bool gridScriptCompatibilityMode)
{
	std::vector<uint8_t> bytes = CTools::getInstance()->BigSIntToBytes(val);

	mInternalTemp.resize(mInternalTemp.size() + bytes.size());
	std::memcpy(mInternalTemp.data() + mOffset, bytes.data(), bytes.size());  // ✓ Copy from bytes.data()
	mOffset += bytes.size();  // ✓ Increment offset
	return true;
}

// ❌ BUGGY IMPLEMENTATION (maintains consensus for Transaction v1-v2)
// CVE-2025-GRIDNET-001: BigSInt fields not authenticated in signature
// This bug allows potential manipulation of BigSInt values after signing
// CRITICAL: Only use for backward compatibility with v1-v2 transactions!
inline bool CDataConcatenator::addBigSIntBugged(BigSInt val, bool gridScriptCompatibilityMode)
{
	std::vector<uint8_t> bytes = CTools::getInstance()->BigSIntToBytes(val);

	mInternalTemp.resize(mInternalTemp.size() + bytes.size());
	std::memcpy(mInternalTemp.data() + mOffset, &mInternalTemp[0], bytes.size());  // ❌ BUG: Copies from start of buffer
	// ❌ BUG: mOffset is NOT incremented (should be: mOffset += bytes.size();)
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
