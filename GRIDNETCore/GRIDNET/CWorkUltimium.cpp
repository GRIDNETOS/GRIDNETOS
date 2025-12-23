#include "CWorkUltimium.h"
#include "WorkManager.h"
#include "CryptoFactory.h"
#include "ccomputetask.h"
#include "cmemparam.h"
#include "cvalueparam.h"
#include <algorithm>
#include <vector>
#include "CGlobalSecSettings.h"
#include "BlockchainManager.h"
#include <fstream>
#include <filesystem>
using namespace std::chrono_literals;

// Initialize static variables
std::mutex CWorkUltimium::s_mutex;
int CWorkUltimium::s_partialPoWLogCount = 0;
time_t CWorkUltimium::s_logWindowStartTime = 0;
double CWorkUltimium::s_progressThreshold = 0.0;

CWorkUltimium::CWorkUltimium(std::shared_ptr<IWork>  parentTask, eBlockchainMode::eBlockchainMode blockchainMode) : IWorkCL(parentTask, blockchainMode)
{
	mReleased = false;
	mDataIsHeader = parentTask != nullptr ? std::static_pointer_cast<CWorkUltimium>(parentTask)->isDataHeader() : true;
	setState(eWorkState::initial);
	markAsTestWork();
	setCurrentOffset(0);
	setPausedAt(0);
	setIsFinished(false);
	setWorkStart(0);
	setWorkEnd(0);
	setWorkOffset(0);
	//mWorkOffset = 0;
	//mWorkEnd = 0;
	mMinTargetOffset = 0;
	mMainTargetOffset = 0;
	mTestBuffer = nullptr;
	mHeaderBuffer = nullptr;
	mOutputBuffer = nullptr;
	mHashBuffer = nullptr;
	mGlobalThreads = 0;
	mMaxStepSize = 0;
	mLocalThreadsCount = 0;

	mProgram = nullptr;
	mKernelShavite = nullptr;
	mKernelKeccak = nullptr;
	mKernelHamsi = nullptr;
	mKernelBlake = nullptr;
	mKernelJH = nullptr;
	mKernelSkein = nullptr;

	mAverageMhpsDuringTest = 0;
	mHashBufferSize = 0;


	mStep = 0;
	mCurrentMhps = 0;
}
CWorkUltimium::CWorkUltimium(std::shared_ptr<CWorkUltimium>parentTask, eBlockchainMode::eBlockchainMode blockchainMode, std::array<unsigned char, 32> workTarget, std::array<unsigned char, 32>  minTarget,
	uint64_t workStart, uint64_t workEnd) : IWorkCL(parentTask, blockchainMode)
{
	mReleased = false;
	mWorkEnd = 0;
	mWorkOffset = 0;
	if (parentTask != nullptr)
		mDataIsHeader = parentTask->isDataHeader();
	else mDataIsHeader = false;
	setShortID(lastShortID + 1);
	lastShortID = getShortID();
	mState = eWorkState::initial;
	setParentTask(nullptr);
	mTools  = CWorkManager::getInstance()->getTools();
 assertGN(mTools != nullptr);
	setWorkOffset(0);
	setPausedAt(0);
	setIsFinished(false);

	setMainWorkDifficulty(workTarget);
	setMinDifficulty(minTarget); 

	setCurrentOffset(workStart);
	setWorkStart(workStart);
	setWorkEnd(workEnd);
	
	mDataIsHeader = false;
	mMinTargetOffset = 0;
	mMainTargetOffset = 0;
	mTestBuffer = nullptr;
	mHeaderBuffer = nullptr;
	mOutputBuffer = nullptr;
	mHashBuffer = nullptr;
	mGlobalThreads = 0;
	mMaxStepSize = 0;
	mLocalThreadsCount = 0;
	mCurrentNonce = 0;
	mProgram = nullptr;
	mKernelShavite = nullptr;
	mKernelKeccak = nullptr;
	mKernelHamsi = nullptr;
	mKernelBlake = nullptr;
	mKernelJH = nullptr;
	mKernelSkein = nullptr;

	mAverageMhpsDuringTest = 0;
	mHashBufferSize = 0;

	mStep = 0;
	mCurrentMhps = 0;

	/*if(parentTask)
	{
		mMinTargetOffset = parentTask->mMinTargetOffset;
		mMainTargetOffset = parentTask->mMainTargetOffset;
		mTestBuffer = parentTask->mTestBuffer;
		mHeaderBuffer = parentTask->mHeaderBuffer;
		mOutputBuffer = parentTask->mOutputBuffer;
		mHashBuffer = parentTask->mHashBuffer;
		mGlobalThreads = parentTask->mGlobalThreads;
		mMaxStepSize = parentTask->mMaxStepSize;
		mLocalThreadsCount = parentTask->mLocalThreadsCount;
		mWorkOffset = parentTask->mWorkOffset;

		mProgram = parentTask->mProgram;
		mKernelShavite = parentTask->mKernelShavite;
		mKernelKeccak = parentTask->mKernelKeccak;
		mKernelHamsi = parentTask->mKernelHamsi;
		mKernelBlake = parentTask->mKernelBlake;
		mKernelJH = parentTask->mKernelJH;
		mKernelSkein = parentTask->mKernelSkein;

		mBlankResults = parentTask->mBlankResults;
		mResults = parentTask->mResults;
		mTestResults = parentTask->mTestResults;

		mAverageMhpsDuringTest = parentTask->mAverageMhpsDuringTest;
		mHashBufferSize = parentTask->mHashBufferSize;
		mSingleCumputeMemReq = parentTask->mSingleCumputeMemReq;
		mRet = parentTask->mRet;
		mCommandQueue = parentTask->mCommandQueue;

		mDeviceTarget = parentTask->mDeviceTarget;
		mStep = parentTask->mStep;
		mCurrentMhps = parentTask->mCurrentMhps;
	}*/
}



void CWorkUltimium::cleanUp()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	releaseResources();
}

void CWorkUltimium::markDataAsHeader()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mDataIsHeader = true;
}

bool CWorkUltimium::isDataHeader()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mDataIsHeader;
}

void CWorkUltimium::releaseResources()
{
	std::shared_ptr<CWorker> worker = getWorker();
	if (!worker)
		return;

	worker->setKernelCompiled(false);//notify worker that compiled kernel is no longer available.

	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(worker->mCCGuardian);
	std::lock_guard<std::recursive_mutex> lockMB(mBuffersGuardian);

	if (getWasReleased())
		return;
	
	getTools()->writeLine("I'm releasing my resources..",true,true,eViewState::unspecified,"Ultimium");
	releaseKernelsAndProgram();
	cl_int res = 0;
	if (mTestBuffer != nullptr)
	{
		res = clReleaseMemObject(getTestBuffer());
		assertGN(res == CL_SUCCESS);
		setTestBuffer(nullptr);
	}
	if (mHeaderBuffer != nullptr)
	{
		res = clReleaseMemObject(getHeaderBuffer());
		assertGN(res == CL_SUCCESS);
		setHeaderBuffer(nullptr);
	}
	if (mOutputBuffer != nullptr)
	{
		res = clReleaseMemObject(getOutputBuffer());
		assertGN(res == CL_SUCCESS);
		setOutputBuffer(nullptr);
	}
	if (mHashBuffer != nullptr)
	{
		res = clReleaseMemObject(getHashBuffer());
		assertGN(res == CL_SUCCESS);
		setHashBuffer(nullptr);
	}
	mFieldsGuardian.lock();
	mWorker.reset();
	mFieldsGuardian.unlock();
	setWasReleased(true);
	getTools()->writeLine("released!", true, true, eViewState::unspecified, "Ultimium");
}

void CWorkUltimium::releaseKernelsAndProgram()
{
	std::lock_guard<std::recursive_mutex> lock(mKernelGuardian);
	cl_int res = 0;
	if (mKernelShavite != nullptr)
	{
		res = clReleaseKernel(mKernelShavite);
		mKernelShavite = nullptr;
	}
	if (mKernelKeccak != nullptr)
	{
		res = clReleaseKernel(mKernelKeccak);
		mKernelKeccak = nullptr;
	}
	if (mKernelHamsi != nullptr)
	{
		res = clReleaseKernel(mKernelHamsi);
		mKernelHamsi = nullptr;
	}
	if (mKernelBlake != nullptr)
	{
		res = clReleaseKernel(mKernelBlake);
		mKernelBlake = nullptr;
	}
	if (mKernelJH != nullptr)
	{
		res = clReleaseKernel(mKernelJH);
		mKernelJH = nullptr;
	}
	if (mKernelSkein != nullptr)
	{
		res = clReleaseKernel(mKernelSkein);
		mKernelSkein = nullptr;
	}
	if (getProgram() != nullptr)
	{
		if (!(CWorkManager::getInstance()->isProgramCachingEnabled()))
			res = clReleaseProgram(getProgram()); //these are now stored within the Workmanager's program cache
		setProgram(nullptr);
	}
}



CWorkUltimium::CWorkUltimium(const CWorkUltimium &work) : IWorkCL(work)
{
	std::lock_guard lock(work.mFieldsGuardian);
	mTools = work.mTools;
	mDataIsHeader = false;
	mMainDifficulty = work.mMainDifficulty;
	mMinDifficulty = work.mMinDifficulty;
	mCurrentNonce = work.mCurrentNonce;
	//mUnverifiedNonces = work.mUnverifiedNonces; // DO NOT copy
	mAccomplishedPoWs = work.mAccomplishedPoWs;
	mWorkEnd = work.mWorkEnd;
	mWorkOffset = work.mWorkOffset;
	mMinTargetOffset = work.mMinTargetOffset;
	mMainTargetOffset = work.mMainTargetOffset;
	mTestBuffer = work.mTestBuffer;
	mHeaderBuffer = work.mHeaderBuffer;
	mOutputBuffer = work.mOutputBuffer;
	mHashBuffer = work.mHashBuffer;
	mGlobalThreads = work.mGlobalThreads;
	mMaxStepSize = work.mMaxStepSize;
	mLocalThreadsCount = work.mLocalThreadsCount;
	mReleased = false;

	mProgram = nullptr;
	mKernelShavite = work.mKernelShavite;
	mKernelKeccak = work.mKernelKeccak;
	mKernelHamsi = work.mKernelHamsi;
	mKernelBlake = work.mKernelBlake;
	mKernelJH = work.mKernelJH;
	mKernelSkein = work.mKernelSkein;


	mBlankResults = work.mBlankResults;
	//mResults = work.mResults;
	mTestResults = work.mTestResults;

	mAverageMhpsDuringTest = work.mAverageMhpsDuringTest;
	mHashBufferSize = work.mHashBufferSize;
	mSingleCumputeMemReq = work.mSingleCumputeMemReq;
	mDeviceTarget = work.mDeviceTarget;
	mStep = work.mStep;
	mCurrentMhps = work.mCurrentMhps;
}

CWorkUltimium::~CWorkUltimium()
{


	for (auto pow : mAccomplishedPoWs)
		pow.reset();
}

IComputeTask *CWorkUltimium::initialiseComputeTask()
{
	/* IComputeTask *computeTask = new CComputeTask(".\\kernel\\gridnetTest.cl", "");
	 mHashBufferSize = getCurrentStepSize() * mSingleCumputeMemReq;
	 CComputeTask task1("", "");

	 //header
	 CMemParam p1(eParamType::memoryBuffer, eMemType::clMem, eAccessType::read, 80);
	 //output
	 CMemParam p2(eParamType::memoryBuffer, eMemType::clMem, eAccessType::write, BUFFERSIZE);
	 //test output
	 CMemParam p3(eParamType::memoryBuffer, eMemType::clMem, eAccessType::write, TEST_BUFFERSIZE);
	 //hash buffer
	 CMemParam p4(eParamType::memoryBuffer, eMemType::clMem, eAccessType::write, mHashBufferSize);

	 task1.addParam(p1);
	 task1.addParam(p2);
	 task1.addParam(p3);
	 task1.addParam(p4);

	 CComputeTask task2("", "blake");
	 p1.setCLMem(mHeaderBuffer);
	 p4.setCLMem(mHashBuffer);
	 task2.addParam(p1);
	 task2.addParam(p4);

	 CComputeTask task3("", "keccak");
	 p4.setCLMem(mHashBuffer);
	 task3.addParam(p4);

	 CComputeTask task4("", "hamsi");
	 p4.setCLMem(mHashBuffer);
	 task4.addParam(p4);

	 CComputeTask task5("", "shavite");
	 p4.setCLMem(mHashBuffer);
	 task5.addParam(p4);

	 CComputeTask task6("", "jh");
	 p4.setCLMem(mHashBuffer);
	 task6.addParam(p4);

	 CComputeTask task7("", "skein");
	 CValueParam p5(eParamType::numerical);
	 CValueParam p6(eParamType::numerical);
	 p6.setValue(mMinTargetOffset);
	 p5.setValue((uint64_t)(getMinDifficulty()->data() + mMinTargetOffset));
	 p4.setCLMem(mHashBuffer);
	 p2.setCLMem(mOutputBuffer);
	 p3.setCLMem(mTestBuffer);
	 task7.addParam(p4);
	 task7.addParam(p2);
	 task7.addParam(p5);
	 task7.addParam(p6);
	 task7.addParam(p3);

	 computeTask->addSubTask(task1);
	 computeTask->addSubTask(task2);
	 computeTask->addSubTask(task3);
	 computeTask->addSubTask(task4);
	 computeTask->addSubTask(task5);
	 computeTask->addSubTask(task6);
	 computeTask->addSubTask(task7);

	 return computeTask;*/
	return nullptr;
}
std::vector<std::shared_ptr<IWorkResult>> CWorkUltimium::getWorkResult(bool includeInterediaryResults)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	std::vector<std::shared_ptr<IWorkResult>> toRet = mAccomplishedPoWs;
	if (includeInterediaryResults)
		return toRet;
	else {
		for (int i = 0; i < toRet.size(); i++)
		{
			if (dynamic_cast<CPoW&>(*toRet[i]).isPartialProof())
			{
				toRet.erase(toRet.begin() + i);
				i--;
			}
		}
	}
	return toRet;
}
void CWorkUltimium::setWorkResult(std::shared_ptr<IWorkResult> result)
{

 assertGN(result != nullptr);
	addPoW(std::dynamic_pointer_cast<CPoW>(result));

}
/**
 * Enhanced report generation including progress percentage for partial PoWs
 * Adds progress indicator showing how close partial solutions were to main target
 */
std::string CWorkUltimium::getReport()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	std::stringstream ss;
	std::shared_ptr<CTools> tools = getTools();
	if (mAccomplishedPoWs.size() > 0)
	{
		for (int i = 0; i < mAccomplishedPoWs.size(); i++)
		{
			auto pow = std::dynamic_pointer_cast<CPoW>(mAccomplishedPoWs[i]);
			bool isPartial = pow->isPartialProof();

			// Header with color-coded success indicator
			ss << ">[" << tools->getColoredString(isPartial ? " PARTIAL" : " SUCCESS",
				isPartial ? eColor::orange : eColor::lightGreen)
				<< ": PoW - " + tools->getColoredString(std::to_string(i), eColor::lightCyan) + " ]<\n";

			// Show trophy ASCII art only for full proofs
			if (!isPartial)
			{
				std::stringstream ss2;
				ss2 << "              .-=========-.\n";
				ss2 << "              \'-=======-'/\n";
				ss2 << "              _|   .=.   |_\n";
				ss2 << "             ((|  {{1}}  |))\n";
				ss2 << "              \\|   /|\\   |/\n";
				ss2 << "               \\__ '`' __/\n";
				ss2 << "                 _`) (`_\n";
				ss2 << "          jgs  _/_______\\_\n";
				ss2 << "              /___________\\\n";
				ss2 << "\n";
				ss << tools->getColoredString(ss2.str(), eColor::orange);
			}
			else
			{
				// Calculate progress for partial proofs
				std::vector<uint8_t> hashVec = pow->getHash();
				std::array<unsigned char, 32> hash;
				std::copy(hashVec.begin(), hashVec.begin() + 32, hash.begin());
				double progress = calculatePartialProgress(hash, getMainDifficulty());

				// Progress bar visualization [=====>----] style
				const int barWidth = 20;
				int filledWidth = static_cast<int>(progress * barWidth / 100.0);

				ss << tools->getColoredString("Progress: [", eColor::orange);
				for (int i = 0; i < barWidth; i++) {
					if (i < filledWidth) ss << "=";
					else if (i == filledWidth) ss << ">";
					else ss << "-";
				}
				ss << "] " << std::fixed << std::setprecision(2) << progress << "%\n";
			}

			// Standard info output
			ss << tools->getColoredString("Found Nonce: ", eColor::blue) +
				std::to_string(pow->getNonce()) + "\n";
			ss << tools->getColoredString("Min. Target: ", eColor::blue) +
				std::to_string(pow->getAssignedTarget().GetCompact()) + "\n";

			// Convert array to vector for main target
			std::array<unsigned char, 32> mainDiff = getMainDifficulty();
			std::vector<uint8_t> mainDiffVec(mainDiff.begin(), mainDiff.end());

			ss << tools->getColoredString("Main Target: ", eColor::blue) +
				std::to_string(tools->targetVec2arithUint(mainDiffVec).GetCompact()) + "\n";

			ss << "\n";
		}
	}
	else
	{
		ss << tools->getColoredString("No PoW solutions found yet", eColor::lightWhite) << "\n";
	}
	return ss.str();
}
void CWorkUltimium::createComputeTask()
{
}


//
//  Create an OpenCL program from the kernel source file
//
cl_program CWorkUltimium::createProgram(cl_context context, cl_device_id device, const char* fileName)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::shared_ptr<CTools> tools = getTools();
	cl_int errNum;
	cl_program program;

	std::ifstream kernelFile(fileName, std::ios::in);
	if (!kernelFile.is_open())
	{
		tools->logEvent("[OpenCL]: Failed to open  OpenCL kernel source file for reading.. ", eLogEntryCategory::localSystem, 10, eLogEntryType::failure, eColor::cyborgBlood,true);
		return nullptr;
	}

	std::ostringstream oss;
	oss << kernelFile.rdbuf();

	std::string srcStdStr = oss.str();
	const char* srcStr = srcStdStr.c_str();

	cl_int errCode;
	program = clCreateProgramWithSource(context, 1, (const char**)&srcStr, nullptr, &errCode);
	if (errCode != CL_SUCCESS)
	{
		tools->logEvent("[OpenCL]: Failed to create CL program from source with error code. ", eLogEntryCategory::localSystem, 10, eLogEntryType::failure, eColor::cyborgBlood,true);
		return nullptr;
	}

	errNum = clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
	if (errNum != CL_SUCCESS)
	{
		// Determine the reason for the error
		char buildLog[16384];
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
			sizeof(buildLog), buildLog, nullptr);
		std::stringstream ss;
		ss << "[OpenCL]: Error in program: " << std::endl <<
			buildLog << std::endl;
		tools->logEvent(ss.str(), eLogEntryCategory::localSystem, 10, eLogEntryType::failure, eColor::cyborgBlood,true);
		clReleaseProgram(program);
		return nullptr;
	}

	return program;
}

unsigned int CWorkUltimium::getMinTargetOffset()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mMinTargetOffset;
}

void CWorkUltimium::setMinTargetOffset(unsigned int offset)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	 mMinTargetOffset = offset;
}

unsigned int CWorkUltimium::getMainTargetOffset()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mMainTargetOffset;
}

void CWorkUltimium::setMainTargetOffset(unsigned int offset)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	 mMainTargetOffset = offset;
}

cl_mem CWorkUltimium::getTestBuffer()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mTestBuffer;
}

cl_mem CWorkUltimium::getHeaderBuffer()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mHeaderBuffer;
}

cl_mem CWorkUltimium::getOutputBuffer()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mOutputBuffer;
}

cl_mem CWorkUltimium::getHashBuffer()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mHashBuffer;
}

void CWorkUltimium::setTestBuffer(cl_mem buf)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mTestBuffer = buf;
}

void CWorkUltimium::setHeaderBuffer(cl_mem buf)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mHeaderBuffer = buf;
}

void CWorkUltimium::setOutputBuffer(cl_mem buf)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mOutputBuffer = buf;
}

void CWorkUltimium::setHashBuffer(cl_mem buf)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mHashBuffer = buf;
}

void CWorkUltimium::setHashBufferSize(size_t size)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mHashBufferSize = size;
}

size_t CWorkUltimium::getHashBufferSize()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mHashBufferSize;
}


bool CWorkUltimium::getDataIsHeader()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mDataIsHeader;
}

void CWorkUltimium::setDataIsHeader(bool isIt)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	 mDataIsHeader = isIt;
}

/**
 * Processes unverified nonces from GPU and performs full verification
 *
 * Verification Process:
 * 1. Check against minimum difficulty (partial PoW)
 * 2. If successful, check against main difficulty (full PoW)
 * 3. Store successful results with computed hashes
 *
 * @return Status code:
 *         1  = Found full PoW
 *         0  = Found partial PoW
 *         -1 = Invalid nonce reported
 */
int64_t CWorkUltimium::checkForResults()
{
	//=== Setup Phase - BEGIN ===//
	// Core components
	std::shared_ptr<CTools> tools = getTools();
	std::shared_ptr<CCryptoFactory> fac = CCryptoFactory::getInstance();
	std::vector<unsigned int> nonces = getUnverifiedNonces();
	std::shared_ptr<CWorker> worker = getWorker();

	// Early exit conditions
	if (getWasReleased() && !worker)
		return false;

	// State tracking
	int64_t isWorkAccomplished = 0;
	CUltimium ultimium;
	bool isFinalPoW = false;
	//=== Setup Phase - END ===//

	//=== Target Setup Phase - BEGIN ===//
	// Load difficulty targets
	std::array<unsigned char, 32> minTarget = getMinDifficulty();
	std::array<unsigned char, 32> mainTarget = getMainDifficulty();

	// Get window offsets for target comparison
	unsigned int minTargetOffset = getMinTargetOffset();
	unsigned int mainTargetOffset = getMainTargetOffset();

	// Validate offset bounds
	if ((minTargetOffset + sizeof(uint64_t)) > 32)
	{
		tools->writeLine(tools->getColoredString("Target offset overflown!", eColor::cyborgBlood)
			+ " by " + worker->getShortName(), true, true, eViewState::unspecified, "Ultimium");
		return -1;
	}

	// Extract target values for window comparison
	cl_ulong localMinTarget = *(((uint64_t*)minTarget.data()) + minTargetOffset);
	cl_ulong localMainTarget = *(((uint64_t*)mainTarget.data()) + mainTargetOffset);

	// Prepare vector format targets
	std::vector<uint8_t> minDiffV(32, 0);
	std::vector<uint8_t> mainDiffV(32, 0);
	std::memcpy(minDiffV.data(), minTarget.data(), 32);
	std::memcpy(mainDiffV.data(), mainTarget.data(), 32);
	//=== Target Setup Phase - END ===//

	//=== Verification Loop Phase - BEGIN ===//
	std::vector<uint8_t> dataToWorkOn = getDataToWorkOn();
	bool isHeader = getDataIsHeader();

	for (auto it = nonces.begin(); it != nonces.end(); it++)
	{
		isFinalPoW = false;
		std::vector<uint8_t> hashResult;

		// Check against minimum difficulty
		if (fac->verifyNonce(*it, dataToWorkOn, tools->targetVec2arithUint(minDiffV),
			isHeader, hashResult))
		{
			worker->pingLastTimeValidResultProduced();

			// Check against main difficulty
			if (fac->verifyNonce(*it, dataToWorkOn, tools->targetVec2arithUint(mainDiffV),
				isHeader))
			{
				tools->writeLine(tools->getPrize(), true, true, eViewState::unspecified, "Ultimium");
				isFinalPoW = true;
				isWorkAccomplished = 1;
			}

			// Store successful result
			std::shared_ptr<CPoW> pow(std::make_shared<CPoW>(*it, dataToWorkOn, !isFinalPoW));
			pow->setAssignedTarget(localMinTarget);
			pow->setHash(hashResult);

			if (!isFinalPoW)
			{
				std::vector<uint8_t> hashVec = pow->getHash();
				std::array<unsigned char, 32> hash;
				std::copy(hashVec.begin(), hashVec.begin() + 32, hash.begin());
				double percentage = calculatePartialProgress(hash, getMainDifficulty());
				pow->setProgressPercentage(percentage);
				eColor::eColor color = pow->getProgressColor(percentage);

				// Acquire mutex lock for thread safety
				{
					std::lock_guard<std::mutex> lock(CWorkUltimium::s_mutex);

					// Get current time
					time_t currentTime = time(nullptr);

					// Check if we need to reset the log window
					if (currentTime - CWorkUltimium::s_logWindowStartTime >= 60)
					{
						// Reset for new minute window
						CWorkUltimium::s_logWindowStartTime = currentTime;
						CWorkUltimium::s_partialPoWLogCount = 0;
						CWorkUltimium::s_progressThreshold = 0.0;
					}

					// Decide whether to log based on threshold and log count
					if (CWorkUltimium::s_partialPoWLogCount < CWorkUltimium::s_maxLogsPerMinute)
					{
						if (percentage >= CWorkUltimium::s_progressThreshold)
						{
							// Prepare and log the message
							std::stringstream ss;
							ss << worker->getShortName()
								<< tools->getColoredString(u8" PoW Found! ", eColor::orange)
								<< u8"✨ [ How Close ]: "
								<< tools->getColoredString(tools->cleanDoubleStr(std::to_string(percentage), 2) + "%", color)
								<< " " << tools->getColoredString(pow->getWizardlyComment(percentage), color);

							tools->writeLine(ss.str(), true, false, eViewState::eventView, "Mining");

							// Update log count and progress threshold
							CWorkUltimium::s_partialPoWLogCount += 1;
							CWorkUltimium::s_progressThreshold = percentage;
						}
					}
				}
			}

			addPoW(pow);
		}
		else
		{
			tools->writeLine(tools->getColoredString("False-nonce reported", eColor::lightPink) +
				" by " + worker->getShortName() + " (nonce: " + std::to_string(*it) + ")",
				true, true, eViewState::unspecified, "Ultimium");
			isWorkAccomplished = -1;
		}
	}
	//=== Verification Loop Phase - END ===//

	//=== Cleanup Phase - BEGIN ===//
	clearUnverifiedNonce();
	return isWorkAccomplished;
	//=== Cleanup Phase - END ===//
}
void CWorkUltimium::setDataToWorkOn(const std::vector<uint8_t>& data, const std::vector<uint8_t>& additionalData)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mInitialData = data;
	if (false && getDataIsHeader())
	{
		if (data.size() != CGlobalSecSettings::getHeaderLength())//it should be a block header
			throw std::invalid_argument("invalid header length");

		mDataToWorkOn.resize(data.size());
		memcpy(mDataToWorkOn.data(), data.data(), data.size());
	}
	else
	{
		std::vector<uint8_t> dataBlock;
		dataBlock.resize(80, 0);// size of the data block processed (hashed) by OpenCL
		CUltimium mUltimium;
		std::array<unsigned char, 64> hash = mUltimium.digest64(data);

		//fill-in the 512-bit keccak hash of input data
		std::memcpy(dataBlock.data(), hash.data(), 64);
		//copy over the additional data
		/*
		std::vector<uint8_t> addition;
		addition.resize(64);

		if (additionalData.size() == 64)
			std::memcpy(addition.data(), additionalData.data(), 64);
		else
			if (additionalData.size() >= 64)
			 assertGN(false);
			else
				if (additionalData.size() < 64)
				{
					std::memcpy(addition.data(), additionalData.data(), additionalData.size());
					std::memset(addition.data() + additionalData.size(), 0, 1);//fill the rest wih 0s
				}
		std::memcpy(dataBlock.data() + 64, addition.data(), 64);

		*/
		mDataToWorkOn = dataBlock;
	}
}

bool CWorkUltimium::optimisePerformance(unsigned int processingTime, bool wasError)
{
	std::shared_ptr<CWorker> worker = getWorker();

	std::lock_guard<std::recursive_mutex> lock(mBuffersGuardian);
	std::lock_guard<std::recursive_mutex> lock2(worker->mCCGuardian);
	//local variables - BEGIN
	
	assertGN(worker != nullptr);
	size_t maxStepSize = getMaxStepSize();
	assertGN(maxStepSize < UINT32_MAX);
	size_t currentStepSize = getCurrentStepSize();
	assertGN(currentStepSize);
	cl_context context = worker->getContext();
	assertGN(context);
	size_t previousStep = currentStepSize;
	uint64_t systemTDR = getTools()->getTDR();
	assertGN(systemTDR);
	size_t hashBufferSize = getHashBufferSize();
	assertGN(hashBufferSize);
	size_t singleComputeMemReq = getSingleCumputeMemReq();
	assertGN(singleComputeMemReq);
	if (systemTDR > 10)
		systemTDR = 10;//we do not want to get past this value.
	size_t max = (double)0.9 * (double)(1000 * systemTDR);
	assertGN(max);
	size_t min = (double)0.7 * (double)(1000 * systemTDR);
	int result = 0;
	cl_mem hashBuf = getHashBuffer();
	assertGN(hashBuf);
	int res = 0;
	//local variables - END
	
	//operational logic - BEGIN
	
	//make sure we do not try to create a larger memory buffer than allowed on this computational unit.
	if (processingTime > max)
	{
		currentStepSize = std::min((size_t)((double)currentStepSize * (double)0.7), maxStepSize);
	}
	else if (processingTime < min)
	{
		currentStepSize = std::min((size_t)((double)currentStepSize * (double)1.5), maxStepSize);
	}


	if (currentStepSize != previousStep)//have we tweaked something?
	{
		//it's not in use now since kernel execution and memory reads were synchronized (blocking).
		result = clReleaseMemObject(hashBuf);//1) release previous hash buffer. Its size is going to change.

		if (result != CL_SUCCESS)
		{
			setStepSize(0);
			setState(eWorkState::aborted);
			setHashBufferSize(0);
			worker->setState(eWorkerState::crashed);
			return false;
		}

		//_sleep(100);
		
		//2) now, recreate the hash-buffer.
		hashBufferSize = currentStepSize * singleComputeMemReq;
		
		hashBuf = clCreateBuffer(context, CL_MEM_READ_WRITE, hashBufferSize, NULL, &res);
		
		if (res != CL_SUCCESS)
		{
			setStepSize(0);
			setState(eWorkState::aborted);
			setHashBufferSize(0);
			worker->setState(eWorkerState::crashed);
			return false;
		}

		setHashBufferSize(hashBufferSize);
		setHashBuffer(hashBuf);

		//let us inform kernels about the updated memory address.
		if (setKernelArgs() == false)
		{
			setStepSize(0);
			setState(eWorkState::aborted);
			worker->setState(eWorkerState::crashed);
			return false;

		}
		setStepSize(currentStepSize);

		return true;
	}
	//operational logic - END
	return true;
}



std::vector< std::shared_ptr<IWorkCL>> CWorkUltimium::divideIntoNr(int intoNr, int shortID, size_t workStepSize)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	lockState();
	std::vector<std::shared_ptr<IWorkCL>> vec;
	std::array<unsigned char, 32> minDiff = getMinDifficulty();
	std::array<unsigned char, 32> mainDiff = getMainDifficulty();

	for (int i = 0; i < intoNr; i++)
	{
		std::shared_ptr<CWorkUltimium> subTask = std::make_shared<CWorkUltimium>(std::make_shared<CWorkUltimium>(*this),mBlockchainMode, mainDiff, minDiff, getWorkStart() + i * workStepSize, getWorkStart() + (i + 1) * workStepSize);
		subTask->setGUID(CTools::getInstance()->genRandomVector(32));
		vec.push_back(subTask);
	}
	unlockState();
	return vec;
}

void CWorkUltimium::cleanupResources()
{
}

cl_program CWorkUltimium::getProgram()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mProgram;
}

void CWorkUltimium::setProgram(cl_program program)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mProgram = program;
}


bool CWorkUltimium::getWasReleased()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mReleased;
}

void CWorkUltimium::setWasReleased(bool wasIt)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	 mReleased = wasIt;
}

void CWorkUltimium::setLocalThreadsCount(size_t count)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mLocalThreadsCount = count;
}


size_t CWorkUltimium::getLocalThreadsCount()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mLocalThreadsCount;
}

std::shared_ptr<CTools> CWorkUltimium::getTools()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mTools;
}

/// <summary>
/// Executes all kernels and does the memory thing.
/// </summary>
/// <returns></returns>
bool CWorkUltimium::executeKernel()
{
	std::shared_ptr<CWorker> worker = getWorker();
	std::shared_ptr<CTools> tools = getTools();

	if (getLocalThreadsCount() == 0) {
		tools->logEvent("Error: Local threads count is 0",
			eLogEntryCategory::localSystem, 10,
			eLogEntryType::failure, eColor::cyborgBlood, false);

		setState(eWorkState::done, false);
		worker->setState(eWorkerState::crashed);
		return false;
	}



	if (getWorkLeft() <= 0 || getWorkLeft() < getLocalThreadsCount() || getCurrentStepSize() < getLocalThreadsCount())
	{
		setState(eWorkState::done, false);
		worker->setState(eWorkerState::stopping);
		return false;
	}




	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::lock_guard<std::recursive_mutex> lock2(mBuffersGuardian);
	std::lock_guard<std::recursive_mutex> lock3(worker->mCCGuardian);
	//Local Variables - BEGIN
	if (getWasReleased())
		return false;

	if (getProgram() == nullptr)
		return false;
	size_t kernelExecTime = 0;

	double currentMhps = 0;
	uint64_t tdr = tools->getTDR();
	assertGN(getWorker() != nullptr);
	uint32_t resultsBufSize = BUFFERSIZE;
	
	const size_t localThreadsCount = getLocalThreadsCount();
	uint32_t testsBufSize = TEST_BUFFERSIZE;
	int NR_OF_RESULTS_INDEX = FOUND;
	bool errored = false;

	cl_command_queue cc = worker->getCommandQueue();

	//Local Variables - END
	
	//Operational Logic - BEGIN

	// Parameters' Trimming - BEGIN
	checkStepBounds();// ensure everything is within bounds.

	//get trimmed operational values
	size_t currentStepSize = getCurrentStepSize();
	size_t offset = getCurrentOffset();
	// Parameters' Trimming - END

	//parameters' optimization would follow after current Step.
	assertGN(currentStepSize > 0);
	assertGN(cc);
	/*if (!cc)
	{
		tools->logEvent("Won't proceed with mining operations. Command queue not ready.", eLogEntryCategory::localSystem, 10, eLogEntryType::warning, eColor::lightPink);
		worker->setState(eWorkerState::crashed);
		worker->unlockCC();
		return false;
	}*/

	//each buffer is 32bits long
	mFieldsGuardian.lock();

	if (!mKernelBlake || !mKernelKeccak || !mKernelHamsi || !mKernelShavite || !mKernelJH || !mKernelSkein)
	{
		tools->logEvent("Won't proceed with mining operations. Kernels are not ready.", eLogEntryCategory::localSystem, 10, eLogEntryType::warning, eColor::lightPink);
		worker->setState(eWorkerState::crashed);
		return false;// throw "error"; //return false
	}

	//Resize host buffers - BEGIN
	if (mBlankResults.size() < resultsBufSize)
		//BUFFERSIZE is in bytes while flowing vector sizes are of 32bit uints thus devision
		mBlankResults.resize(resultsBufSize / 4, 0);
	if (mResults.size() < resultsBufSize)
		mResults.resize(resultsBufSize / 4, 0);
	if (mTestResults.size() < testsBufSize)
		mTestResults.resize(testsBufSize / 8, 0);

	mBuffersGuardian.lock();
	//Resize host buffers - END

	//clear the output buffer containing previous results
	cl_int res = worker->enqueueWriteBuffer(mOutputBuffer,false, 0,
		resultsBufSize, mBlankResults.data(), 0, nullptr, nullptr);

	mBuffersGuardian.unlock();
	mFieldsGuardian.unlock();
	if (res != CL_SUCCESS)
		errored = true;

	if (errored)
	{
		worker->setState(eWorkerState::crashed);
		return false;// throw "error"; //return false;
	}
	while (getState() == eWorkState::paused)
		std::this_thread::sleep_for(10ms);

	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

	bool overflowed = checkForNonceOverflow();//automatically updates step size in case of a potential overflow


	if (currentStepSize > 0 && (getWorkLeft() > 0 || getWorkEnd() == 0) && localThreadsCount > 0)
	{
		assertGN(cc != nullptr);

		if (!((offset + currentStepSize) <= UINT32_MAX))
		{
		   
			tools->writeLine("Invalid range passed to kernel. [Offset]: "+ std::to_string(offset)+ " [Step]: "
				+ std::to_string(currentStepSize), true, true, eViewState::eventView, "Ultimium");
			currentStepSize = UINT32_MAX - offset;// WARNING: currentStepSize needs to be a multiple of local work group size
		}


		assertGN((offset + currentStepSize) <= UINT32_MAX);

		
		std::string wName = worker->getShortName();
		// enqueue kernels
		
		std::lock_guard<std::recursive_mutex> lock(mKernelGuardian);
		bool itsOK = false;
	
		itsOK = enqueueKernel(cc, mKernelBlake, &offset, &currentStepSize, &localThreadsCount, false);
		
		if (!itsOK)
		{
		
			worker->setState(eWorkerState::crashed);
			return false;// throw "error"; //return false;
		}

		itsOK = enqueueKernel(cc, mKernelKeccak, &offset, &currentStepSize, &localThreadsCount, false);
	
		if (!itsOK)
		{
		
			worker->setState(eWorkerState::crashed);
			return false;// throw "error"; //return false;
		}

		itsOK = enqueueKernel(cc, mKernelHamsi, &offset, &currentStepSize, &localThreadsCount, false);

		if (!itsOK)
		{
	
			worker->setState(eWorkerState::crashed);
			return false;// throw "error"; //return false;
		}
		itsOK = enqueueKernel(cc, mKernelShavite, &offset, &currentStepSize, &localThreadsCount, false);

		if (!itsOK)
		{

			worker->setState(eWorkerState::crashed);
			return false;// throw "error"; //return false;
		}
		itsOK = enqueueKernel(cc, mKernelJH, &offset, &currentStepSize, &localThreadsCount, false);

		if (!itsOK)
		{
		
			worker->setState(eWorkerState::crashed);
			return false;// throw "error"; //return false;
		}

		itsOK = enqueueKernel(cc, mKernelSkein, &offset, &currentStepSize, &localThreadsCount, false);
		if (!itsOK)
		{
			worker->setState(eWorkerState::crashed);
			return false;// throw "error"; //return false;
		}

		//execute scheduled kernels
		res = worker->executeCL();

		if (res != CL_SUCCESS)
		{
			worker->setState(eWorkerState::crashed);
			return false;// throw "error"; //return false;
		}
		
		//do memory stuff
	
		memset(mBlankResults.data(), 0, mBlankResults.size());
	
		res = worker->enqueueReadBuffer(mOutputBuffer, false, 0,//blocking
			resultsBufSize, mResults.data(), 0, nullptr, nullptr);

		if (res != CL_SUCCESS)
		{
			return false;
		}


		//lets clear the outputBuffer
		res = worker->enqueueWriteBuffer(mOutputBuffer, false, 0,
			resultsBufSize, mBlankResults.data(), 0, nullptr, nullptr);

		if (res != CL_SUCCESS)
			errored = true;
	
		res = worker->enqueueReadBuffer(mTestBuffer, false, 0,//blocking
			testsBufSize, mTestResults.data(), 0, nullptr, nullptr);

		if (res != CL_SUCCESS)
		{
			return false;
		}

		 kernelExecTime = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start)).count();
		
		if (kernelExecTime > ((float)(tdr*1000) * (float)0.8))
		{
			tools->logEvent("Warning: OpenCL kernel execution time getting dangerously close to Max. Execution Time (TDR).", eLogEntryCategory::localSystem,10,eLogEntryType::warning, eColor::lightPink);
		}
		worker->setKernelExecTime(kernelExecTime);
		

		 currentMhps = (((double)1000 * (double)getCurrentStepSize()) / (double)kernelExecTime) / (double)1000000;

		setEstimatedTimeLeft(getWorkLeft() / (currentMhps * 1000000));

		worker->setMhps(currentMhps);
		
		

			setCurrentOffset(getCurrentOffset() + getCurrentStepSize());
			//the nonce is used to keep track of overall work accomplishment; task bounds checking

			if (getCurrentStepSize() > 0)//do not try to optimize if no work is left
			{
				if (!optimisePerformance(kernelExecTime, false))
				{
					tools->logEvent("Error: error while optimizing performance for" + worker->getShortName(), eLogEntryCategory::localSystem, 3, eLogEntryType::notification, eColor::cyborgBlood, true);
					worker->setState(eWorkerState::crashed);
					return false;
				}
			}

			checkStepBounds();//might be redundant. there's a check within setStepSize function. just to make sure.

			size_t resultsCount = 0;
			mFieldsGuardian.lock();
			resultsCount = mResults[NR_OF_RESULTS_INDEX];
			mFieldsGuardian.unlock();

			if (resultsCount) {
				lockCollections();//so that the verification thread wait until we add all intermediary results.

				if (resultsCount >= MAXBUFFERS-1)//check if the number of found nonces is greater than we are able to store. this is most likely an error
				{
					unlockCollections();
					tools->logEvent("Error: The number of found nonces overflowed for " + worker->getShortName(), eLogEntryCategory::localSystem, 3, eLogEntryType::notification, eColor::cyborgBlood, true);
					
					worker->setState(eWorkerState::crashed);
					return false;
				}
		
				std::array<unsigned char, 32> minDiff = getMinDifficulty();
				std::array<unsigned char, 32> mainDiff = getMainDifficulty();
				unsigned int minTargetOffset = getMinTargetOffset();

				for (unsigned int i = 0; i < resultsCount && i < MAXBUFFERS - 1; i++) 
				{/*
					std::vector<uint8_t> d; d.resize(32, 0);
					std::memcpy(d.data(), mainDiff.data(), 32);
					cl_ulong test_res = mTestResults[i];
					cl_ulong localMinTarget = *(((uint64_t*)minDiff.data()) + minTargetOffset);
					std::vector<uint8_t> target;
					target.resize(32, 0);

					std::memcpy(target.data(), minDiff.data(), 32);
					*/
					addUnverifiedNonce(mResults[i]);
				}
			
				unlockCollections();
				mFieldsGuardian.lock();
				memset(mTestResults.data(), 0, mTestResults.size());
				memset(mResults.data(), 0, mResults.size());
				mFieldsGuardian.unlock();
			}

		
		if (errored)
		{
			worker->setState(eWorkerState::crashed);
			return false;
		}

	}

	if (getWorkLeft() <= 0 || getWorkLeft()< getLocalThreadsCount()||  getCurrentStepSize() < getLocalThreadsCount())
	{
		setState(eWorkState::done, false);
		worker->setState(eWorkerState::stopping);
		return false;
	}
	//Operational Logic - END
	return true;
}

/// <summary>
/// The function checks Cold Storage for the needed components.
/// </summary>
/// <returns></returns>
bool CWorkUltimium::checkComponents(bool report, const std::vector<uint8_t> & currentEncHash)
{
	// Local Variables - BEGIN
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();

	std::vector<uint8_t> expectedHash, expectedEncHash;

	if (!tools->base58CheckDecode(CGlobalSecSettings::getExpectedEncUltimumHash(), expectedEncHash))// base58-check decode the hash
	{
		tools->writeLine(tools->getColoredString("Invalid in-code OpenCL kernel hash.", eColor::lightPink) + "\n Current hash: " + tools->base58CheckEncode(expectedEncHash), eLogEntryCategory::localSystem);
		return false;
	}
	// Check integrity of main OpenCL file - BEGIN
	std::string mainKernelFile = ".\\kernel\\gridnetTest.cl";
	std::vector<uint8_t> hash = tools->getFileHash(mainKernelFile);
	

	std::vector<uint8_t> keyB = cf->getSHA3_256Vec(tools->stringToBytes(INTEGRITY_ENC_KEY));
	Botan::secure_vector<uint8_t> keyBS(keyB.begin(), keyB.end());
	std::vector<uint8_t> encHash = cf->encChaCha20(keyBS, hash);
	const_cast<std::vector<uint8_t>&>(currentEncHash) = encHash;

	// Decrypt the hash
	try {
		expectedHash = cf->decChaCha20(keyB, expectedEncHash);
	}
	catch (...)
	{
		tools->writeLine(tools->getColoredString("Invalid in-code OpenCL kernel hash.", eColor::lightPink) + "\n Current hash: " + tools->base58CheckEncode(expectedEncHash), eLogEntryCategory::localSystem);
		return false;
	}
	// Local Variables - END


	// Operational Logic - BEGIN
	

	if(report)
	tools->writeLine("Checking integrity of OpenCL components..", true, true,eViewState::eventView,"Security");

	if (hash.empty())
	{
		tools->writeLine("Ultimium OpenCL kernel file not found.", true, true, eViewState::eventView, "Security");
		return false;
	}

	tools->writeLine("Expected Enc Ultimium OpenCL fingerprint: " + CGlobalSecSettings::getExpectedEncUltimumHash(), true, true, eViewState::eventView, "Security");
	tools->writeLine("Present  Enc Ultimium OpenCL fingerprint: " + tools->base58CheckEncode(encHash), true, true, eViewState::eventView, "Security");

	if (tools->compareByteVectors(expectedHash, hash) == false)
	{
		tools->writeLine("Invalid OpenCL kernel file found.",  true, true, eViewState::eventView, "Security");

		return false;
	}
	

	std::vector<std::string> files =
	{
		".\\kernel\\blake.cl",
		".\\kernel\\hamsi.cl",
		".\\kernel\\jh.cl",
		".\\kernel\\keccak.cl",
		".\\kernel\\shavite.cl",
		".\\kernel\\skein.cl",
		".\\kernel\\gridnetTest.cl",
		".\\kernel\\aes_helper.cl",
	    ".\\kernel\\hamsi_helper.cl"
	};

	// Check integrity of main OpenCL file - END

	for (uint64_t i = 0; i < files.size(); i++)
	{
		if (!std::filesystem::exists(files[i]))
		{
			if (report)
			tools->writeLine("[OpenCL]: file '" + files[i] + "'" + tools->getColoredString(" is missing.", eColor::cyborgBlood),  true, true, eViewState::eventView, "Security");
			return false;
		}
		else
		{
			if (report)
			tools->writeLine("[OpenCL]: file '"+ files[i]+"'" +tools->getColoredString(" is present.", eColor::lightGreen), true, true, eViewState::eventView, "Security");
		}

	}

	tools->writeLine("[OpenCL]:"+tools->getColoredString(" Ultimium Components are present.", eColor::lightGreen),true, true, eViewState::eventView, "Security");
	
	if (!CSolidStorage::initialiseSolidStorage())
	{
		tools->writeLine(tools->getColoredString("The Storage sub-system failed to initialize. Aborting..", eColor::alertError ), true, true, eViewState::eventView, "Security");
		_sleep(3000);
		CTools::stopAllOperations();
		exit(0);
	}
	
	
	return true;

	// Operational Logic - END
}

bool CWorkUltimium::compileKernel()
{

	bool useExtensions = true;
	/*
	 * OpenCL Work Size Configuration:
	 * - mStep ↔ globalWorkSize (total work items)
	 * - mLocalThreadsCount ↔ localWorkSize (work items per work group)
	 * - getCurrentOffset() ↔ globalWorkOffset (starting offset)
	 *
	 * Hash Chain Execution:
	 * blake→keccak→hamsi→shavite→jh→skein must use identical work group sizes.
	 * Work group size must be optimized for:
	 * 1. Device's SIMD width (warp/wavefront)
	 * 2. Hardware compute units and architecture
	 * 3. Vendor-specific alignment requirements
	 *
	 * Program Caching Strategy:
	 * - Cached programs skip compilation but still need kernel creation
	 * - Work group analysis is always performed for optimal sizing
	 * - All kernels must be recreated even with cached program
	 */
	std::shared_ptr<CWorker> worker = getWorker();

	// Pre-Flight Checks
	if (getProgram()) {
		setState(eWorkState::prepared);
		return true;
	}

	std::shared_ptr<CTools> tools = getTools();
	eWorkState currentState = getState();
	{
		std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
		if (currentState == eWorkState::preparing) {
			tools->writeLine("Kernel compilation already in progress for task " +
				std::to_string(getShortID()), true, true, eViewState::unspecified, "Ultimium");
			return false;
		}
		setState(eWorkState::preparing);
	}

	// Local Variables - Using vectors for better memory management
	size_t logSize = 0;
	std::vector<char> log;

	if (!worker) {
		return false;
	}

	// Assess Prior Attempts - BEGIN
	std::string lastErrorLog;
	const auto& compilationLog = worker->getCompilationHistory();

	uint64_t lastFailure = 0;
	uint64_t lastSuccess = 0;

	if (!compilationLog.empty()) {
		const auto& lastAttempt = compilationLog.back();
		if (!lastAttempt.success) {
			lastFailure = lastAttempt.timestamp;
			lastErrorLog = lastAttempt.errorMessage;
		}
	}
	// Assess Prior Attempts - END


	cl_device_id dev = worker->getDevice();
	cl_context context = worker->getContext();
	cl_int res = CL_SUCCESS;
	const char* fileName = ".\\kernel\\gridnetTest.cl";
	std::vector<char> source_str;
	size_t source_size = 0;
	size_t globalMaxWorkGroupSize = SIZE_MAX;
	size_t maxWorkGroupSize = SIZE_MAX;

	// Query comprehensive device capabilities with error checking
	std::stringstream deviceInfo;
	deviceInfo << "Device Capabilities Report\n";
	deviceInfo << "========================\n";

	// Basic device information
	char deviceVendor[256] = { 0 };
	char deviceName[256] = { 0 };
	char deviceVersion[256] = { 0 };
	cl_uint computeUnits = 0;
	cl_ulong localMemSize = 0;
	cl_ulong maxMemAlloc = 0;
	size_t maxWorkItemSizes[3] = { 0 };
	size_t deviceMaxWorkGroupSize = 0;

	// Retrieve device information with error checking
	res = clGetDeviceInfo(dev, CL_DEVICE_VENDOR, sizeof(deviceVendor), deviceVendor, nullptr);
	if (res != CL_SUCCESS) {
		tools->logEvent("[OpenCL]: Failed to query device vendor.", eLogEntryCategory::localSystem,
			10, eLogEntryType::failure, eColor::cyborgBlood, false);
		return false;
	}
	res = clGetDeviceInfo(dev, CL_DEVICE_NAME, sizeof(deviceName), deviceName, nullptr);
	if (res != CL_SUCCESS) {
		tools->logEvent("[OpenCL]: Failed to query device name.", eLogEntryCategory::localSystem,
			10, eLogEntryType::failure, eColor::cyborgBlood, false);
		return false;
	}
	res = clGetDeviceInfo(dev, CL_DEVICE_VERSION, sizeof(deviceVersion), deviceVersion, nullptr);
	if (res != CL_SUCCESS) {
		tools->logEvent("[OpenCL]: Failed to query device version.", eLogEntryCategory::localSystem,
			10, eLogEntryType::failure, eColor::cyborgBlood, false);
		return false;
	}
	res = clGetDeviceInfo(dev, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(computeUnits), &computeUnits, nullptr);
	if (res != CL_SUCCESS) {
		tools->logEvent("[OpenCL]: Failed to query compute units.", eLogEntryCategory::localSystem,
			10, eLogEntryType::failure, eColor::cyborgBlood, false);
		return false;
	}
	res = clGetDeviceInfo(dev, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(localMemSize), &localMemSize, nullptr);
	if (res != CL_SUCCESS) {
		tools->logEvent("[OpenCL]: Failed to query local memory size.", eLogEntryCategory::localSystem,
			10, eLogEntryType::failure, eColor::cyborgBlood, false);
		return false;
	}
	res = clGetDeviceInfo(dev, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(maxMemAlloc), &maxMemAlloc, nullptr);
	if (res != CL_SUCCESS) {
		tools->logEvent("[OpenCL]: Failed to query max memory allocation.", eLogEntryCategory::localSystem,
			10, eLogEntryType::failure, eColor::cyborgBlood, false);
		return false;
	}
	res = clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(maxWorkItemSizes), maxWorkItemSizes, nullptr);
	if (res != CL_SUCCESS) {
		tools->logEvent("[OpenCL]: Failed to query max work item sizes.", eLogEntryCategory::localSystem,
			10, eLogEntryType::failure, eColor::cyborgBlood, false);
		return false;
	}
	res = clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(deviceMaxWorkGroupSize), &deviceMaxWorkGroupSize, nullptr);
	if (res != CL_SUCCESS) {
		tools->logEvent("[OpenCL]: Failed to query max work group size.", eLogEntryCategory::localSystem,
			10, eLogEntryType::failure, eColor::cyborgBlood, false);
		return false;
	}

	// Build and log device capabilities report
	deviceInfo << "Vendor: " << deviceVendor << "\n";
	deviceInfo << "Device: " << deviceName << "\n";
	deviceInfo << "Version: " << deviceVersion << "\n";
	deviceInfo << "Compute Units: " << computeUnits << "\n";
	deviceInfo << "Local Memory: " << (localMemSize / 1024) << "KB\n";
	deviceInfo << "Max Allocation: " << (maxMemAlloc / (1024 * 1024)) << "MB\n";
	deviceInfo << "Max Work Group Size: " << deviceMaxWorkGroupSize << "\n";
	


	// Identify vendor and get architecture-specific capabilities
	std::string vendor(deviceVendor);
	std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::tolower);

	// Default SIMD width (will be refined by vendor)
	size_t simdWidth = 32;  // Conservative default for unknown vendors
	std::string vendorOptions;
	std::string architectureInfo;
	// Use official vendor-specific constants and capabilities
	if (vendor.find("nvidia") != std::string::npos) {
		simdWidth = 32;  // NVIDIA warp size is fixed at 32 threads
		worker->setEstimatedCompilationTime(60 * 60 * 2); // 2 hours
		// NVIDIA-specific constants from cl_nv_device_attribute_query
		cl_uint ccMajor = 0, ccMinor = 0;
#ifdef CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV
		// Query NVIDIA compute capability if extension is available
		res = clGetDeviceInfo(dev, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(cl_uint), &ccMajor, nullptr);
		res |= clGetDeviceInfo(dev, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(cl_uint), &ccMinor, nullptr);
		if (res == CL_SUCCESS) {
			architectureInfo = "Compute Capability: " + std::to_string(ccMajor) + "." + std::to_string(ccMinor);

			// Log extended NVIDIA architecture information if available
			cl_uint regsPerBlock = 0;
			if (clGetDeviceInfo(dev, 0x4002, sizeof(cl_uint), &regsPerBlock, nullptr) == CL_SUCCESS) {
				architectureInfo += "\nRegisters per Block: " + std::to_string(regsPerBlock);
			}
		}
#endif
		// Only use officially supported OpenCL compiler options for NVIDIA
		// These are guaranteed to work across all NVIDIA OpenCL implementations
		if (useExtensions)
		{
			vendorOptions = " -cl-fast-relaxed-math"            // Enable fast math operations
				" -cl-mad-enable"                    // Enable multiply-add fusion
				" -cl-no-signed-zeros"              // Ignore sign of zero
				" -cl-finite-math-only";            // Assume no INF/NaN
		}
	}
	else if (vendor.find("amd") != std::string::npos ||
		vendor.find("advanced micro devices") != std::string::npos) {
		simdWidth = 64;  // AMD wavefront size is typically 64 threads
		worker->setEstimatedCompilationTime(60 * 15); // 15 minutes comptile time
		// AMD-specific capabilities querying
		cl_uint simdPerCU = 0;
		cl_uint simdInstrWidth = 0;

		// Try to query AMD-specific information using their extensions
		if (clGetDeviceInfo(dev, 0x4028, sizeof(cl_uint), &simdPerCU, nullptr) == CL_SUCCESS) {
			architectureInfo = "SIMD units per CU: " + std::to_string(simdPerCU);
		}
		if (clGetDeviceInfo(dev, 0x4029, sizeof(cl_uint), &simdInstrWidth, nullptr) == CL_SUCCESS) {
			architectureInfo += "\nSIMD Instruction Width: " + std::to_string(simdInstrWidth);
		}

		// Query local memory type (scratchpad vs cache)
		cl_uint localMemType = 0;
		if (clGetDeviceInfo(dev, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(cl_uint), &localMemType, nullptr) == CL_SUCCESS) {
			architectureInfo += "\nLocal Memory Type: " +
				std::string(localMemType == CL_LOCAL ? "Dedicated Local Memory" : "Global Cache");
		}
		if (useExtensions)
		{
			// AMD OpenCL compiler options - stick to well-supported flags
			vendorOptions = " -cl-fast-relaxed-math"            // Enable fast math
				" -cl-mad-enable"                    // Enable multiply-add fusion
				" -cl-no-signed-zeros"              // Ignore sign of zero
				" -cl-finite-math-only";            // Assume no INF/NaN
		}
	}
	else
	{
		worker->setEstimatedCompilationTime(60 * 60 * 2); // 2 estiamted compile time (default)
	}

	// Log SIMD and architecture information
	deviceInfo << "SIMD Width: " << simdWidth << "\n";
	if (!architectureInfo.empty()) {
		deviceInfo << architectureInfo << "\n";
	}

	
	// Retireve OpenCL source and timestemp - BEGIN
	// Retrieve OpenCL source and timestamp
	uint64_t kernelFileModifiedAt = 0;

	

	try {
		// Get file timestamp
		std::filesystem::path kernelPath(fileName);
		kernelFileModifiedAt = std::filesystem::last_write_time(kernelPath)
			.time_since_epoch()
			.count();

		// Read file using ifstream for RAII
		std::ifstream kernelFile(fileName, std::ios::binary | std::ios::ate);
		if (!kernelFile.is_open()) {
			tools->logEvent("Failed to load kernel source file: " + std::string(fileName),
				eLogEntryCategory::localSystem, 10,
				eLogEntryType::failure, eColor::cyborgBlood, false);
			worker->setState(eWorkerState::crashed);
			return false;
		}

		// Get size and resize buffer
		source_size = kernelFile.tellg();
		kernelFile.seekg(0, std::ios::beg);
		source_str.resize(source_size);

		// Read entire file
		if (!kernelFile.read(source_str.data(), source_size)) {
			tools->logEvent("Failed to read kernel source file completely.",
				eLogEntryCategory::localSystem, 10,
				eLogEntryType::failure, eColor::cyborgBlood, false);
			worker->setState(eWorkerState::crashed);
			return false;
		}
	}
	catch (const std::exception& e) {
		tools->logEvent("Error accessing kernel file: " + std::string(e.what()),
			eLogEntryCategory::localSystem, 10,
			eLogEntryType::failure, eColor::cyborgBlood, false);
		worker->setState(eWorkerState::crashed);
		return false;
	}

	// Retireve OpenCL source and timestemp - END

	// Generate Program Cache Key
	// https://anteru.net/blog/2014/associating-opencl-device-ids-with-gpus/ - on how to retrieve unique device identifiers
	 // We rely on device name for identification since:
	 // 1. Some platforms (ARM) might not have PCI bus at all
	// 2. Device name is somewhat error-prone but practical
	// 3. Only problematic with identical device names but different architectures (unlikely)
	// 4. For NVIDIA/AMD, additional architecture info provides extra uniqueness
	std::vector<uint8_t> idDataBuffer;
	std::vector<uint8_t> workGroupID = worker->getGroupID();
	std::string deviceNameStr = worker->getName();     // Full device name
	std::string shortName = worker->getShortName(); // Short identifier
	std::string vendorStr(deviceVendor);           // Vendor-specific info

	// Build comprehensive cache key including:
	// 1. Source code - ensures program matches current implementation
	// 2. Device name - ensures cache matches hardware
	// 3. Work Group ID - maintains original work group association
	// 4. Vendor info - ensures vendor-specific optimizations are preserved
	// 5. Timestamp -  the last time OpenCL file was modified.
	idDataBuffer.insert(idDataBuffer.end(), source_str.begin(), source_str.end());
	idDataBuffer.insert(idDataBuffer.end(), deviceNameStr.begin(), deviceNameStr.end());
	idDataBuffer.insert(idDataBuffer.end(), shortName.begin(), shortName.end());
	idDataBuffer.insert(idDataBuffer.end(), vendorStr.begin(), vendorStr.end());
	// Add timestamp to idDataBuffer
	std::vector<uint8_t> timeBytes(sizeof(time_t));
	std::memcpy(timeBytes.data(), &kernelFileModifiedAt, sizeof(time_t));
	idDataBuffer.insert(idDataBuffer.end(), timeBytes.begin(), timeBytes.end());

	// Debug info - maintain original functionality
	std::string test = tools->bytesToString(idDataBuffer);

	// Generate final cache key
	std::vector<uint8_t> key = tools->stringToBytes(
		tools->base58CheckEncode(CCryptoFactory::getInstance()->getSHA2_256Vec(idDataBuffer)));

	if (key.size() < 32) {
		tools->logEvent("[OpenCL]: Couldn't generate Kernel's ID.",
			eLogEntryCategory::localSystem, 10, eLogEntryType::failure,
			eColor::cyborgBlood, false);
		worker->setState(eWorkerState::crashed);
		return false;
	}

	key.resize(15, 0);//should be enough

	// Report kernel ID
	deviceInfo << tools->getColoredString("Kernel ID: ", eColor::neonBlue) << tools->base58CheckEncode(key) <<"\n";


	tools->writeLine(deviceInfo.str(), true, true, eViewState::unspecified, "Ultimium");

	// Check Program Cache
	// Even if we get a cached program, we'll still need to:
	// 1. Create kernels
	// 2. Query work group sizes
	// 3. Setup optimal configurations
	bool optimizedProgramFromCache = false;

	if (CWorkManager::getInstance()->isProgramCachingEnabled()) {
		cl_program prog = CWorkManager::getInstance()->getProgramFromCache(key, true, context, dev);
		if (prog) {
			setProgram(prog);
			optimizedProgramFromCache = true;
			tools->writeLine("Retrieved optimized program from cache for " + worker->getShortName(),
				true, true, eViewState::unspecified, "Ultimium");
		}
	}

	// Initialize work group size based on device architecture
	bool wasFirstPassCompilation = true;
	// Start with device max, will be refined based on kernel requirements
	globalMaxWorkGroupSize = deviceMaxWorkGroupSize;
	// Ensure initial size is aligned to SIMD width
	globalMaxWorkGroupSize = (globalMaxWorkGroupSize / simdWidth) * simdWidth;
	setLocalThreadsCount(globalMaxWorkGroupSize);

	// Setup build info logging
	std::stringstream buildInfo;
	bool compilationSuccess = false;
	int compilationPass = 0;

	// Program compilation and kernel creation loop
	// Note: Even with cached program, we still create kernels and analyze work groups
	do {
		compilationPass++;
		buildInfo.str("");
		buildInfo << (compilationPass == 1 ? "First" : "Second") << " Pass Compilation for "<< shortName << "\n";
		buildInfo << "=====================================\n";

		// Only compile if not using cached program
		if (!optimizedProgramFromCache) {
			// Create program from source
			const char* srcStr = source_str.data();
			cl_program program = clCreateProgramWithSource(context, 1,
				(const char**)&srcStr, &source_size, &res);
			if (res != CL_SUCCESS) {
				tools->logEvent("[OpenCL]: Failed to create program from source. Error: " +
					std::to_string(res), eLogEntryCategory::localSystem, 10,
					eLogEntryType::failure, eColor::cyborgBlood, false);
				worker->setState(eWorkerState::crashed);
				return false;
			}
			setProgram(program);

			// Version detection and compiler options setup
			// Parse OpenCL version with error checking
			std::string oclVersion = "-cl-std=CL1.2"; // Default fallback
			std::string versionStr(deviceVersion);
			int oclMajor = 1, oclMinor = 2;
			if (sscanf(deviceVersion, "OpenCL %d.%d", &oclMajor, &oclMinor) == 2) {
				oclVersion = "-cl-std=CL" + std::to_string(oclMajor) + "." +
					std::to_string(oclMinor);
			}
			else {
				tools->writeLine("Warning: Could not parse OpenCL version string. Using OpenCL 1.2",
					true, true, eViewState::unspecified, "Ultimium");
			}

			// Conditionally include version-specific options
			std::string finalVendorOptions = vendorOptions;
			if (oclMajor >= 3) {
				if (useExtensions)
				{
					// OpenCL 3.0 specific optimizations
					finalVendorOptions += " -cl-unsafe-math-optimizations"
						" -cl-single-precision-constant";
				}
			}

			// Prepare comprehensive compiler options

			std::string compilerOptions = "-I \"" + std::string(fileName) +
				"\" -I \".\\kernel\" -I \".\" " +      // Added "." back as include path
				oclVersion + " -D WORKSIZE=" + std::to_string(getLocalThreadsCount()) +
				finalVendorOptions;

			buildInfo << "OpenCL Version: " << oclVersion << "\n";
			buildInfo << "Compiler Options: " << compilerOptions << "\n";
			buildInfo << "Target Work Group Size: " << getLocalThreadsCount() << "\n";

			tools->writeLine(buildInfo.str(), true, true, eViewState::unspecified, "Ultimium");

			
			// report recent failure(s)
			if (lastFailure)
			{ 
				
				tools->writeLine("Warning, compilation for " +shortName+ tools->getColoredString(" has already failed ", eColor::cyborgBlood)
					+ std::to_string(worker->getFailedCompilationCount())+" time(s) recently at " +tools->timeToString(lastFailure)+ ". Last log :\n" +
					lastErrorLog,
					true, true, eViewState::unspecified, "Ultimium");
			}

			// Build Program
			res = clBuildProgram(getProgram(), 1, &dev, compilerOptions.c_str(), nullptr, nullptr);
			if (res != CL_SUCCESS) {
				// Retrieve and Log Build Errors
				clGetProgramBuildInfo(getProgram(), dev, CL_PROGRAM_BUILD_LOG,
					0, nullptr, &logSize);
				log.resize(logSize);
				clGetProgramBuildInfo(getProgram(), dev, CL_PROGRAM_BUILD_LOG,
					logSize, log.data(), nullptr);
				std::string logStr = std::string(log.begin(), log.end());
				worker->addCompilationLog(false, logStr);
				tools->writeLine("Compilation failed. Build Log:\n" +
					logStr,
					true, true, eViewState::unspecified, "Ultimium");
				worker->setState(eWorkerState::crashed);
				return false;
			}
			else
			{
				worker->addCompilationLog(true,"Compilation succeeded at " +tools->timeToString(std::time(0)));
			}
		}
		
		
		// Assess Maximum Work Group Sizes for Each Kernel
        // This happens for both cached and newly compiled programs
		std::lock_guard<std::recursive_mutex> lock(mKernelGuardian);
		std::stringstream kernelInfo;
		kernelInfo << "Kernel Analysis Report\n";
		kernelInfo << "=====================\n";
		kernelInfo << "SIMD Width: " << simdWidth << "\n";
		kernelInfo << (optimizedProgramFromCache ? "Using Cached Program\n" : "Using Newly Compiled Program\n");

		// Initialize Kernels and Query Maximum Work Group Sizes
		struct KernelInfo {
			const char* name;
			cl_kernel* kernelPtr;
		};
		KernelInfo kernels[] = {
			{"shavite", &mKernelShavite},
			{"keccak", &mKernelKeccak},
			{"hamsi", &mKernelHamsi},
			{"blake", &mKernelBlake},
			{"jh", &mKernelJH},
			{"skein", &mKernelSkein}
		};

		// Track optimal sizes for each kernel
		bool kernelCreateFailed = false;
		for (const auto& k : kernels) {
			kernelInfo << "\nKernel '" << k.name << "':\n";

			// Create kernel - required for both cached and new programs
			*(k.kernelPtr) = clCreateKernel(getProgram(), k.name, &res);
			if (res != CL_SUCCESS || *(k.kernelPtr) == nullptr) {
				kernelInfo << tools->getColoredString("  Failed to create kernel. Error: " +
					std::to_string(res) + "\n", eColor::cyborgBlood);
				kernelCreateFailed = true;
				break;
			}

			// Get kernel's maximum work group size
			res = clGetKernelWorkGroupInfo(*(k.kernelPtr), dev, CL_KERNEL_WORK_GROUP_SIZE,
				sizeof(size_t), &maxWorkGroupSize, nullptr);
			if (res != CL_SUCCESS) {
				kernelInfo << tools->getColoredString("  Failed to query work group info. Error: " +
					std::to_string(res) + "\n", eColor::cyborgBlood);
				kernelCreateFailed = true;
				break;
			}
			// Kernel Local Workgroup Size Post Processing - BEGIN
			// Get preferred multiple for this kernel
			// Get preferred multiple for this kernel
			size_t preferredMultiple = 0;  // Initialize to 0
			res = clGetKernelWorkGroupInfo(*(k.kernelPtr), dev,
				CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
				sizeof(size_t), &preferredMultiple, nullptr);

			// If query failed or returned 0, fall back to SIMD width
			if (res != CL_SUCCESS || preferredMultiple == 0) {
				kernelInfo << "  Using SIMD width " << simdWidth << " as preferred multiple\n";
				preferredMultiple = simdWidth;
			}
			else {
				kernelInfo << "  Using kernel's preferred multiple: " << preferredMultiple << "\n";
			}

			// Validate preferred multiple
			if (preferredMultiple == 0) {
				kernelInfo << tools->getColoredString("  Critical Error: Preferred work group multiple is 0\n",
					eColor::cyborgBlood);
				kernelCreateFailed = true;
				break;
			}

			// Get local memory requirements
			cl_ulong localMemRequired = 0;
			res = clGetKernelWorkGroupInfo(*(k.kernelPtr), dev,
				CL_KERNEL_LOCAL_MEM_SIZE,
				sizeof(cl_ulong), &localMemRequired, nullptr);
			if (res != CL_SUCCESS) {
				kernelInfo << "  Failed to query local memory requirements. Assuming none.\n";
				localMemRequired = 0;
			}

			// Validate work group size before SIMD alignment
			if (maxWorkGroupSize == 0) {
				kernelInfo << tools->getColoredString("  Critical Error: Invalid work group size 0 before SIMD alignment\n",
					eColor::cyborgBlood);
				kernelCreateFailed = true;
				break;
			}

			// SIMD alignment with validation
			if (maxWorkGroupSize < preferredMultiple) {
				kernelInfo << tools->getColoredString("  Critical Error: Max work group size " +
					std::to_string(maxWorkGroupSize) +
					" is below minimum SIMD width " +
					std::to_string(preferredMultiple) + "\n",
					eColor::cyborgBlood);
				kernelCreateFailed = true;
				break;
			}

			// Round down to nearest SIMD width multiple with validation
			maxWorkGroupSize = (maxWorkGroupSize / preferredMultiple) * preferredMultiple;
			if (maxWorkGroupSize == 0) {
				kernelInfo << tools->getColoredString("  Critical Error: Work group size became 0 after SIMD alignment\n",
					eColor::cyborgBlood);
				kernelCreateFailed = true;
				break;
			}

			// Update global maximum while maintaining SIMD alignment
			if (maxWorkGroupSize < globalMaxWorkGroupSize) {
				if (maxWorkGroupSize == 0) {
					kernelInfo << tools->getColoredString("  Critical Error: Attempted to set global max work group size to 0\n",
						eColor::cyborgBlood);
					kernelCreateFailed = true;
					break;
				}
				globalMaxWorkGroupSize = maxWorkGroupSize;
			}

			// Final validation of global maximum
			if (globalMaxWorkGroupSize == 0) {
				kernelInfo << tools->getColoredString("  Critical Error: Global maximum work group size is 0\n",
					eColor::cyborgBlood);
				kernelCreateFailed = true;
				break;
			}

			// Log detailed kernel configuration
			kernelInfo << "  Initial Max Work Group Size: " << maxWorkGroupSize << "\n";
			kernelInfo << "  Preferred Multiple: " << preferredMultiple << "\n";
			kernelInfo << "  Local Memory Required: " << (localMemRequired / 1024) << "KB\n";
			kernelInfo << "  Final Aligned Size: " << maxWorkGroupSize << "\n";
			kernelInfo << "  Current Global Max: " << globalMaxWorkGroupSize << "\n";
			// Kernel Local Workgroup Size Post Processing - END
		}

		// Handle kernel creation failures
		if (kernelCreateFailed) {
			tools->writeLine(kernelInfo.str(), true, true, eViewState::unspecified, "Ultimium");
			worker->setState(eWorkerState::crashed);
			return false;
		}

		// Ensure final size is aligned to SIMD width and within bounds
		// This is crucial for optimal performance on both NVIDIA and AMD
		globalMaxWorkGroupSize = std::min(globalMaxWorkGroupSize, deviceMaxWorkGroupSize);
		globalMaxWorkGroupSize = (globalMaxWorkGroupSize / simdWidth) * simdWidth;

		// Update work group size
		setLocalThreadsCount(globalMaxWorkGroupSize);

		setMaxStepSize(((double)worker->getMaxMemAlloc()) / (double)getSingleCumputeMemReq());
		setMaxStepSize(getMaxStepSize() / 2);//remove
		setStepSize((0.2 * getMaxStepSize()));    //getStepSize(); //TODO: make adjustable

		if (worker->getType() == CWorker::eWorkerType::CPU)
			setStepSize(10000);//pre-defined for CPUs (not integrated GPUs!


		// Log final configuration details
		kernelInfo << "\nFinal Configuration:\n";
		kernelInfo << "  Global Max Work Group Size: " << globalMaxWorkGroupSize << "\n";
		kernelInfo << "  SIMD Width Alignment: " << simdWidth << "\n";
		kernelInfo << "  Final Work Group Size: " << getLocalThreadsCount() << "\n";
		kernelInfo << "  Memory Constraints Applied: " << (localMemSize / 1024) << "KB available\n";
		kernelInfo << " Step Size: " << getStep() << "\n";
		kernelInfo << " Max Step Size: " << getMaxStepSize() << "\n";


		tools->writeLine(kernelInfo.str(), true, true, eViewState::unspecified, "Ultimium");

		// Compilation pass decision logic
		if (compilationPass == 1 && !optimizedProgramFromCache) {
			// First pass complete, need second pass with optimized work group size
			tools->writeLine("Starting second pass compilation with optimized work group size",
				true, true, eViewState::unspecified, "Ultimium");
			releaseKernelsAndProgram();
			continue;
		}
		else {
			// Either second pass complete or using cached program
			compilationSuccess = true;

			if (!optimizedProgramFromCache) {
				// Only cache newly compiled programs
				if (CWorkManager::getInstance()->isProgramCachingEnabled()) {
					CWorkManager::getInstance()->putProgramIntoCache(key, getProgram(),
						true, context, dev);
					tools->writeLine("Optimized program cached for future use",
						true, true, eViewState::unspecified, "Ultimium");
				}
			}
			else {
				tools->writeLine("Using cached program with optimized configuration",
					true, true, eViewState::unspecified, "Ultimium");
			}
			break;
		}

	} while (compilationPass < 2);

	// Final success check and cleanup
	if (!compilationSuccess) {
		worker->setState(eWorkerState::crashed);
		return false;
	}
	else
	{
		//Prepare memory buffers - BEGIN

		setHashBufferSize(getCurrentStepSize()* getSingleCumputeMemReq());
		if (getHashBufferSize() > worker->getMaxMemAlloc())
		{
			tools->writeLine("error", true, true, eViewState::unspecified, "Ultimium");
		}

		cl_mem buf = clCreateBuffer(context, CL_MEM_READ_WRITE, getDataToWorkOn().size(), nullptr, &res);
		if (res != CL_SUCCESS)
		{

			worker->setState(eWorkerState::crashed);
			return false;
		}
		setHeaderBuffer(buf);

		buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, BUFFERSIZE, nullptr, &res);


		if (res != CL_SUCCESS)
		{
			worker->setState(eWorkerState::crashed);
			return false;
		}
		setOutputBuffer(buf);

		buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, TEST_BUFFERSIZE, nullptr, &res);


		if (res != CL_SUCCESS)
		{
			worker->setState(eWorkerState::crashed);
			return false;
		}

		setTestBuffer(buf);
		//test buffer is used for debugging to retrieve hash value from a given atomic hash function
		buf = clCreateBuffer(context, CL_MEM_READ_WRITE, getHashBufferSize(), nullptr, &res);


		if (res != CL_SUCCESS)
		{
			worker->setState(eWorkerState::crashed);
			return false;
		}
		setHashBuffer(buf);
		//Prepare memory buffers - END

		if (getCurrentStepSize() == 0)
		{
			tools->writeLine("won't proceed with Kernel activation for task " + std::to_string(getShortID()) + " as no feasible work left.\n I'll ask for a Re-Work instead.", true, true, eViewState::unspecified, "Ultimium");
			tools->writeLine("", true, true, eViewState::unspecified, "Ultimium");
			setState(eWorkState::reWorkRequest, false);
			worker->setState(eWorkerState::stopping);
		}
	}

	setState(eWorkState::prepared);
	return true;
}

bool CWorkUltimium::setKernelArgs()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	std::shared_ptr<CWorker> worker = getWorker();
	assertGN(worker != nullptr);
	cl_int res = 0;
	if (getWorkEnd() <= 0)
	{
		getTools()->writeLine("Error: task assignment aborted. Work size is 0", true, true, eViewState::unspecified, "Ultimium");
		return false;
	}
	//rest of kernel variables are set in InitializeWorker()

	setMinTargetOffset(detectTargetWindowOffset(getMinDifficulty()));
	setMainTargetOffset(detectTargetWindowOffset(getMainDifficulty()));
	res = clSetKernelArg(mKernelBlake, 0, sizeof(mHeaderBuffer), (void*)&mHeaderBuffer);
	if (res != CL_SUCCESS)
		return false;
	res = clSetKernelArg(mKernelBlake, 1, sizeof(mHashBuffer), (void*)&mHashBuffer);
	if (res != CL_SUCCESS)
		return false;

	cl_uint isHeader = 0;
	if (getDataIsHeader())
		isHeader = 1;
	res = clSetKernelArg(mKernelBlake, 2, sizeof(cl_uint), (void*)&isHeader);
	if (res != CL_SUCCESS)
		return false;

	//test blake

	//test keccak
	/*
	mRet = clSetKernelArg(mKernelBlake, 2, sizeof(mTestBuffer), (void *)&mTestBuffer);
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
	mRet = clSetKernelArg(mKernelBlake, 3, sizeof(mOutputBuffer), (void *)&mOutputBuffer);
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
	mRet = clSetKernelArg(mKernelBlake, 4, sizeof(cl_ulong), (void *)(work->getMinDifficulty()->data() + mMinTargetOffset));
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
	mRet = clSetKernelArg(mKernelBlake, 5, sizeof(unsigned int), (void *)&mMinTargetOffset);
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
		*/
		//end test blake

	res = clSetKernelArg(mKernelKeccak, 0, sizeof(mHashBuffer), (void*)&mHashBuffer);
	if (res != CL_SUCCESS)
		return false;

	res = clSetKernelArg(mKernelHamsi, 0, sizeof(mHashBuffer), (void*)&mHashBuffer);
	/*
	//test hamsi
	mRet = clSetKernelArg(mKernelHamsi, 1, sizeof(mTestBuffer), (void *)&mTestBuffer);
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
	mRet = clSetKernelArg(mKernelHamsi, 2, sizeof(mOutputBuffer), (void *)&mOutputBuffer);
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
	mRet = clSetKernelArg(mKernelHamsi, 3, sizeof(cl_ulong), (void *)(work->getMinDifficulty()->data() + mMinTargetOffset));
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
	mRet = clSetKernelArg(mKernelHamsi, 4, sizeof(unsigned int), (void *)&mMinTargetOffset);
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;



	//end test hamsi
	*/
	if (res != CL_SUCCESS)
		return false;
	res = clSetKernelArg(mKernelShavite, 0, sizeof(mHashBuffer), (void*)&mHashBuffer);
	if (res != CL_SUCCESS)
		return false;
	/*
		//test shavite
		mRet = clSetKernelArg(mKernelShavite, 1, sizeof(mTestBuffer), (void *)&mTestBuffer);
		if (mRet != CL_SUCCESS)
			somethingWentWrong = true;
		mRet = clSetKernelArg(mKernelShavite, 2, sizeof(mOutputBuffer), (void *)&mOutputBuffer);
		if (mRet != CL_SUCCESS)
			somethingWentWrong = true;
		mRet = clSetKernelArg(mKernelShavite, 3, sizeof(cl_ulong), (void *)(work->getMinDifficulty()->data() + mMinTargetOffset));
		if (mRet != CL_SUCCESS)
			somethingWentWrong = true;
		mRet = clSetKernelArg(mKernelShavite, 4, sizeof(unsigned int), (void *)&mMinTargetOffset);
		if (mRet != CL_SUCCESS)
			somethingWentWrong = true;



		//end test shavite*/

	res = clSetKernelArg(mKernelJH, 0, sizeof(mHashBuffer), (void*)&mHashBuffer);
	if (res != CL_SUCCESS)
		return false;

	/*//test JH
	mRet = clSetKernelArg(mKernelJH, 1, sizeof(mTestBuffer), (void *)&mTestBuffer);
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
	mRet = clSetKernelArg(mKernelJH, 2, sizeof(mOutputBuffer), (void *)&mOutputBuffer);
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
	mRet = clSetKernelArg(mKernelJH, 3, sizeof(cl_ulong), (void *)(work->getMinDifficulty()->data() + mMinTargetOffset));
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;
	mRet = clSetKernelArg(mKernelJH, 4, sizeof(unsigned int), (void *)&mMinTargetOffset);
	if (mRet != CL_SUCCESS)
		somethingWentWrong = true;



	//end test JH
	*/
	//hashes, output, target, target_offset
	//changed: initialize task-specific data later on by scheduler
	res = clSetKernelArg(mKernelSkein, 0, sizeof(mHashBuffer), (void*)&mHashBuffer);
	if (res != CL_SUCCESS)
		return false;
	res = clSetKernelArg(mKernelSkein, 1, sizeof(mOutputBuffer), (void*)&mOutputBuffer);
	if (res != CL_SUCCESS)
		return false;


	//test Skein
	std::array<unsigned char, 32> minDiff = getMinDifficulty();
	cl_ulong  target = *(((uint64_t*)minDiff.data()) + mMinTargetOffset);//for debug
	res = clSetKernelArg(mKernelSkein, 2, sizeof(cl_ulong), (void*)(((cl_ulong*)minDiff.data()) + mMinTargetOffset));
	//the  entire 32-byte (256bit) hash is threated as a number. using 64-bit windows (the size of cl_ulong)
	//min_target_offset is added to adjust the window which is used by kernel for difficulty comparisons. (more info in the detectTargetWindowOffset function comments)
	//TODO: what happens if min_target_offset and min_target_offset are not the same?
	if (res != CL_SUCCESS)
		return false;
	res = clSetKernelArg(mKernelSkein, 3, sizeof(cl_uint), (void*)&mMinTargetOffset);
	if (res != CL_SUCCESS)
		return false;
	res = clSetKernelArg(mKernelSkein, 4, sizeof(mTestBuffer), (void*)&mTestBuffer);
	if (res != CL_SUCCESS)
		return false;
	//end test Skein

	//int siz = sizeof(unsigned long);
	//cl_ulong s;
	//res = clSetKernelArg(mKernelSkein, 2, sizeof(cl_ulong), (void *)(work->getMinDifficulty()->data() + target_offset));
	if (res != CL_SUCCESS)
		return false;

	//mRet = clSetKernelArg(mKernelSkein, 3, sizeof(unsigned int), (void *)&target_offset);
	//if (mRet != CL_SUCCESS)somethingWentWrong = true;
	//mRet = clSetKernelArg(mKernelSkein, 4, sizeof(mTestBuffer), (void *)&mTestBuffer);
	//if (mRet != CL_SUCCESS)somethingWentWrong = true;

	std::vector<uint8_t> data = getDataToWorkOn();

	res = worker->enqueueWriteBuffer(mHeaderBuffer, false, 0, data.size(), data.data(), 0, nullptr, nullptr);//IMPORTANT: it MUST be synchronous - data 
	//is on the stack.
	//_sleep(100);
	if (res != CL_SUCCESS)
		return false;


	return true;
}


/// <summary>
/// The function compares the target using 64-bit windows (size of uin32_t)
/// The same window will be compared by kernel.
/// </summary>
/// <param name="target"></param>
/// <returns></returns>
///
//the hash is 512bits, the PoW uses only  256 bits so 3rd index would be the first (4th 64-bit ulong number), lowest one (little endian)
 //we should also check if the upper window is equal to zero once the offset gets lower than 3
 //left out for now for performance reasons.
unsigned int CWorkUltimium::detectTargetWindowOffset( std::array<unsigned char, 32> target)
{
	for (int i = 3; i >= 0; i--)
	{
		uint64_t t = reinterpret_cast<uint64_t&>(*(target.data() + i * 8)); // Read little-endian 64-bit word
		if (t != 0)
			return i;
	}
	return 0; // If all windows are zero, handle appropriately
}


size_t CWorkUltimium::getSingleCumputeMemReq()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mSingleCumputeMemReq;
}

void CWorkUltimium::setMainWorkDifficulty(const std::array<unsigned char, 32> &difficulty)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mMainDifficulty = difficulty;
}

void CWorkUltimium::setMinDifficulty(const std::array<unsigned char, 32> &difficulty)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mMinDifficulty = difficulty;
}

std::array<unsigned char, 32> CWorkUltimium::getMinDifficulty()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mMinDifficulty;
}

std::array<unsigned char, 32> CWorkUltimium::getMainDifficulty()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mMainDifficulty;
}



void CWorkUltimium::addUnverifiedNonce(unsigned int nonce)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mUnverifiedNonces.push_back(nonce);
}

void CWorkUltimium::clearUnverifiedNonce()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	mUnverifiedNonces.clear();
}

/**
 * Enhanced version of addPoW that includes progress calculation
 */
void CWorkUltimium::addPoW(std::shared_ptr<CPoW> pow) {
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);

	// Update timestamps
	std::shared_ptr<CWorker> worker = getWorker();
	if (worker) {
		worker->pingLastTimeResultProduced();
	}
	pingLastTimeResultProduced();

	// Calculate progress for partial solutions
	if (pow->isPartialProof()) {
		
		// Enhanced logging with progress
		//std::stringstream ss;
		//ss << std::fixed << std::setprecision(2) << progress;
		/////getTools()->logEvent(
			//getTools()->getColoredString(" found partial PoW", eColor::orange) +
			//" (nonce: " + std::to_string(pow->getNonce()) +
			//") - " + ss.str() + "% of target difficulty","Ultimium",eLogEntryCategory::localSystem,3);
	}

	// Add to accomplished PoWs if unique
	bool alreadyThere = false;
	for (int i = 0; i < mAccomplishedPoWs.size(); i++) {
		if (std::memcmp((std::dynamic_pointer_cast<CPoW>(mAccomplishedPoWs[i]))->getGUID().data(),
			pow->getGUID().data(), 32) == 0) {
			alreadyThere = true;
			break;
		}
	}
	if (!alreadyThere)
		mAccomplishedPoWs.push_back(pow);
}



/**
 * Calculates how close a hash was to meeting the full target difficulty.
 * Returns percentage value from 0-100.
 *
 * For target comparison:
 * [Window 0][Window 1][Window 2][Window 3]  (Little-endian layout)
 *  LSB                           MSB
 *
 * Example: If main target requires window 2 to be <= 0x100
 * and hash has window 2 = 0x180, this is roughly 75% close
 *
 * @param hash Full 256-bit hash value that met partial target
 * @param mainTarget Main (full) target that hash didn't quite meet
 * @return Percentage (0-100) of how close hash was to meeting main target
 */
double CWorkUltimium::calculatePartialProgress(const std::array<unsigned char, 32>& hash,
	const std::array<unsigned char, 32>& mainTarget) {
	// First find the significant window of the main target
	unsigned int mainWindow = detectTargetWindowOffset(mainTarget);

	// Get uint64 values for the significant windows
	uint64_t targetValue = reinterpret_cast<const uint64_t&>(*(mainTarget.data() + mainWindow * 8));
	uint64_t hashValue = reinterpret_cast<const uint64_t&>(*(hash.data() + mainWindow * 8));

	// Check windows above significant window
	for (int i = 3; i > mainWindow; i--) {
		uint64_t higherHashValue = reinterpret_cast<const uint64_t&>(*(hash.data() + i * 8));
		if (higherHashValue != 0) {
			// If any higher window is non-zero, hash is very far from target
			return 0.0;
		}
	}

	if (hashValue <= targetValue) {
		// Hash actually meets the target at this window
		return 100.0;
	}

	// Calculate how close the hash was
	// Use log scale since difficulty is exponential
	double ratio = static_cast<double>(targetValue) / static_cast<double>(hashValue);

	// Convert to percentage, max 99.99% for partial solutions
	double percentage = std::min(99.99, ratio * 100.0);

	return percentage;
}

std::vector<unsigned int> CWorkUltimium::getUnverifiedNonces()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	return mUnverifiedNonces;
}
