#pragma once
#include <memory>

class CDTI;
class CScriptEngine;

/// <summary>
/// The very purpose of this object is to return to validate in-Shell Terminal's state once it gets out of scope.
/// </summary>
class CNativeAppDestructor
{
public:
	CNativeAppDestructor(std::shared_ptr<CDTI> dti, std::shared_ptr<SE::CScriptEngine> se, bool clearScreen=true);
	~CNativeAppDestructor();
private:
	std::shared_ptr<CDTI> mDTI;
	std::shared_ptr<SE::CScriptEngine> mSE;
	bool mClearScreen;
	static uint64_t mNestedCalls;
};
