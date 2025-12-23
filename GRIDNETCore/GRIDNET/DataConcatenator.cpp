#include "DataConcatenator.h"
#include "ScriptEngine.h"
CDataConcatenator::CDataConcatenator(uint64_t sizeLimit)
{
	mOffset = 0;
	mSizeLimit = sizeLimit;
}
/// <summary>
/// Resets the state of the concatenator.
/// </summary>
void CDataConcatenator::reset()
{
	mInternalTemp.clear();
	mOffset = 0;
}

/// <summary>
/// Add a GridScript compatible representation of a native variable.
/// The value would look the same on the GridScript 64-bit stack.
/// Used to represent same results as with GridScript's 'concat' instruction.
/// </summary>
/// <param name="val"></param>
/// <returns></returns>
bool CDataConcatenator::addGSC(bool val)
{
	uint64_t gridScriptCompatibleBool;//64 bits
	if (val)
		gridScriptCompatibleBool = 1;
	else
		gridScriptCompatibleBool = 0;
	if (mSizeLimit != 0)
		if (mInternalTemp.size() + sizeof(gridScriptCompatibleBool) > mSizeLimit)
			return false;
		mInternalTemp.resize(mInternalTemp.size() + sizeof(gridScriptCompatibleBool));
		std::memcpy(mInternalTemp.data() + mOffset, &gridScriptCompatibleBool, sizeof(gridScriptCompatibleBool));
		mOffset += sizeof(gridScriptCompatibleBool);
		return true;
}
/// <summary>
/// Add a GridScript compatible representation of a native variable.
/// The value would look the same on the GridScript 64-bit stack.
/// Used to represent same results as with GridScript's 'concat' instruction.
/// </summary>
/// <param name="val"></param>
/// <returns></returns>
bool CDataConcatenator::addGSC(uint32_t val)
{
	uint64_t gridScriptCompatibleNr;//64 bits
	if (val)
		gridScriptCompatibleNr = val;
	else
		gridScriptCompatibleNr = 0;
	if (mSizeLimit != 0)
	if (mInternalTemp.size() + sizeof(gridScriptCompatibleNr) > mSizeLimit)
		return false;
	mInternalTemp.resize(mInternalTemp.size() + sizeof(gridScriptCompatibleNr));
	std::memcpy(mInternalTemp.data() + mOffset, &gridScriptCompatibleNr, sizeof(gridScriptCompatibleNr));
	mOffset += sizeof(gridScriptCompatibleNr);
	return true;
}


/// <summary>
/// Concatenates a byte-vector.
/// The function IS GridScript compatible.
/// </summary>
/// <param name="vec"></param>
/// <returns></returns>
bool CDataConcatenator::add(std::vector<uint8_t> vec)
{
	if (vec.size() == 0)
		return true;

	if(mSizeLimit!=0)
	if (mInternalTemp.size() + vec.size() > mSizeLimit)
		return false;
	mInternalTemp.resize(mInternalTemp.size() + vec.size());
	std::memcpy(mInternalTemp.data() + mOffset,vec.data(), vec.size());
	mOffset += vec.size();
	return true;
}
// ✓ FIXED IMPLEMENTATION (use for future versions)
// Properly increments mOffset when adding empty vector
bool CDataConcatenator::addGSC(std::vector<uint8_t> vec)
{
	uint64_t emptyStackVariable = 0;
	if (vec.size() == 0)
	{
		mInternalTemp.resize(mInternalTemp.size() + sizeof(emptyStackVariable));
		std::memcpy(mInternalTemp.data() + mOffset, &emptyStackVariable, sizeof(emptyStackVariable));
		mOffset += sizeof(emptyStackVariable);  // ✓ Increment offset
		return true;
	}
	else
		return add(vec);
}

// ❌ BUGGY IMPLEMENTATION (maintains consensus for existing signatures)
// CVE-2025-GRIDNET-002: Empty vector case doesn't increment mOffset
// This bug could affect signatures when empty vectors are passed to addGSC
// CRITICAL: Only use for backward compatibility with existing signatures!
bool CDataConcatenator::addGSCBugged(std::vector<uint8_t> vec)
{
	uint64_t emptyStackVariable = 0;
	if (vec.size() == 0)
	{
		mInternalTemp.resize(mInternalTemp.size() + sizeof(emptyStackVariable));
		std::memcpy(mInternalTemp.data() + mOffset, &emptyStackVariable, sizeof(emptyStackVariable));
		// ❌ BUG: mOffset is NOT incremented (should be: mOffset += sizeof(emptyStackVariable);)
		return true;
	}
	else
		return add(vec);
}
bool CDataConcatenator::addStackElement(void *se, uint32_t stackDepth)
{

	//CHECKMEMPTRS(dTop- stackDepth);
	
	SE::AAddr dTop = static_cast<SE::CScriptEngine*>(se)->getdTop();
	if (!static_cast<SE::CScriptEngine*>(se)->checkMemInRegisteredRegion(SE::Cell(dTop-stackDepth), 1))
		return false;
	if (static_cast<SE::CScriptEngine*>(se)->getCDD(dTop - stackDepth).getDataType() != eDataType::eDataType::pointer)
	{
		add(*(dTop - stackDepth));//do not verify/covert verify data types over here; just treat everything as a 64-bit unsigned integer. even doubles etc.
	}
	else {
		std::vector<uint8_t> data = static_cast<SE::CScriptEngine*>(se)->getVec(dTop - stackDepth);
		add(data);
	}
	return true;
}
/// <summary>
/// Concatenates a string.
/// </summary>
/// <param name="vec"></param>
/// <returns></returns>
bool CDataConcatenator::add(std::string str)
{
	if (mSizeLimit != 0)
		if (mInternalTemp.size() + str.size() > mSizeLimit)
			return false;
	mInternalTemp.resize(mInternalTemp.size() + str.size());
	std::memcpy(mInternalTemp.data() + mOffset, str.data(), str.size());
	mOffset += str.size();
	return true;
}

/// <summary>
/// Returns the result of concatenations.
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CDataConcatenator::getData()
{
	return mInternalTemp;
}
