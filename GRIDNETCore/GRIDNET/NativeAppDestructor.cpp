#include "stdafx.h"
#include "NativeAppDestructor.h"
#include "DTI.h"
#include "ScriptEngine.h"

CNativeAppDestructor::CNativeAppDestructor(std::shared_ptr<CDTI> dti, std::shared_ptr<SE::CScriptEngine> se, bool clearScreen)
{
	mDTI = dti;
	mSE = se;
	mClearScreen = clearScreen;
	mNestedCalls++;
	if (mSE)
	{
		mSE->appDestructorCalled();
	}
}

uint64_t CNativeAppDestructor::mNestedCalls = 0;
CNativeAppDestructor::~CNativeAppDestructor()
{
	
	if (mSE != nullptr)
	{
		mSE->appDestructorExits();

		//Revert to normal state- BEGIN
		if (mSE->getAppDestrucotorLevel() == 0)//only if last instance (to handle cases when two destructors are initialized one for user-mode
			//function and the other for kernel-mode function).
		{
			mSE->revertStateAfterAppQuit();
			mSE->setIsWaitingForVMMetaData(false);

			mSE->setQRIntentResponse(nullptr, false, true);
			//Revert to normal state - END
		}
	}

	if (mDTI != nullptr)
	{
		if (mClearScreen)
		{
			mDTI->clearScreen();
			
		}
		mDTI->setInputGoesToBuffer(true);
		mDTI->setIsViewSwitchEnabled(true);
		mDTI->requestShell();
		
	}

}
