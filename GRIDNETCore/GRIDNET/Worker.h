#ifndef WORKER_H
#define WORKER_H

class CWorkUltimium;
#include "stdafx.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include "CL/cl.h"

#endif
#include <thread>
#include "ccomputetask.h"
#include "../interfaces/iworkcl.h"

#define MAXBUFFERS (32768 + 1)                // 8193 buffers: 8192 for nonces, 1 for counter
#define BUFFERSIZE (sizeof(uint32_t) * MAXBUFFERS)
#define TEST_BUFFERSIZE (sizeof(cl_ulong) * MAXBUFFERS)
#define FOUND (MAXBUFFERS - 1)               // Counter at the last index
#define MEM_SIZE (128)
//#define MAX_SOURCE_SIZE (0x100000)

enum class eWorkerState {
	initial,
	running,
	ready,
	warming_up,
	getting_work,
	stopped,
	stopping,
	crashed,
	testing,
	after_joining,
	joining,
	resting,
	compiling////IMPORTANT: task which is 'compiling' state cannot be aborted. Mostly due to a blocking nature of clBuidProgram()
};

class IWorkCL;

class CWorker {
public:
	struct CompilationLog {
		bool success;
		std::string errorMessage;
		std::time_t timestamp;
	};
private:
	

	std::vector<CompilationLog> mCompilationHistory;
	mutable std::recursive_mutex mCompilationHistoryMutex;
	uint64_t mSuccessfulCompilations;
	uint64_t mFailedCompilations;
	uint64_t mEstimatedCompilationTime;
public:

	void setEstimatedCompilationTime(uint64_t time);
	uint64_t getEstimatedCompilationTime();
	// Compilation Tracking - BEGIN
	
	// Add compilation log
	void addCompilationLog(bool success, const std::string& error);

	// Get compilation statistics
	uint64_t getSuccessfulCompilationCount() const;
	uint64_t getFailedCompilationCount() const;

	// Get most recent compilation log
	std::string getLastCompilationLog() const;

	// Check if this worker has had compilation failures
	bool hadPriorCompilationFailure() const;

	// Get all compilation logs
	std::vector<CompilationLog> getCompilationHistory() const;
	// Compilation Tracking - END

	void lockState();
	void unlockState();
	bool isWorkerThreadJoinable();
	static enum eWorkerType
	{
		GPU,
		CPU
	};
	void setType(eWorkerType type);
	eWorkerType getType();
	void joinThreads();
	CWorker();
	CWorker(std::vector<uint8_t> groupID);
	~CWorker();

	void cleanUp();

	void setContext(cl_context pContext);
	cl_context getContext();
	void evaluateOCLResult(int result);
	bool setState(eWorkerState state, std::string description="");
	eWorkerState getState();
	bool isKernelCompiled();
	bool getKernelEverCompiled();
	void setKernelCompiled(bool isCompiled);
	size_t getGlobalThreadsNr();
	bool initializeWorker();
	std::string getStateName(bool colorize = true);
    bool compileKernel();
	void lockCC();
	void unlockCC();
	cl_int executeCL();
	cl_int enqueueWriteBuffer(cl_mem buffer, bool async, size_t offset, size_t cb, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
	cl_int enqueueReadBuffer(cl_mem buffer, bool async,size_t offset, size_t cb, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);
    bool activateWorker();
	void deactivateWorker();
	uint64_t getAvrKernelExecTime();
	uint64_t getRestingSince();
	uint64_t getCompilingSince();
	uint64_t getCrashCount(bool windowed);
	std::shared_ptr<IWorkCL> getCurrentTask();
	void setDevice(cl_device_id device);
	cl_device_id getDevice();
	std::recursive_mutex mStateMutex;
	void setNamePrefix(std::string name);
	void setShortName(std::string name);
	std::string getShortName();
	std::string getName();
	size_t getNextStep();
	double  getMhps();
	uint64_t getKernelExecTime();
	void setKernelExecTime(uint64_t time);
	void setMhps(double currentMhps);

	void setMaxComputeUnits(unsigned int maxComputeUnits);

	void setMaxWorkGroupSize(unsigned int maxWorkSize);


	void setMaxMemAlloc(cl_ulong maxMemAlloc);

	cl_ulong  getMaxMemAlloc();
	unsigned int getMaxComputeUnits();
	unsigned int getMaxWorkGroupSize();
	void setCurrentTask(std::shared_ptr<IWorkCL> task);
	double getAverageMhps();
	void reportMhpsSample(double currentMhps);
	unsigned int getCurrentStepOffset() const;
	void setCurrentStepOFfset(unsigned int offset);
	void reportMhps();
	size_t getTimeInTests();
	void setTimeInTests(size_t time);
	bool setCommandQueue(cl_command_queue cc);
	cl_command_queue getCommandQueue();
	std::string getReport();
	std::vector<uint8_t> getGroupID();
	std::shared_ptr<CTools> getTools();
	std::recursive_mutex mCCGuardian;
	bool  isCompilationComplete() const;
	// Method to get the result of the compilation once it's complete
	bool  getCompilationResult();
	// Existing setter method
	void setCompilationFuture(std::future<bool> future);
	uint64_t getLastTimeResultProduced();
	void pingLastTimeResultProduced();

	uint64_t getWarnignsCount();
	void clearWarningsCount();
	void incWarningsCount();
	uint64_t getLastTimeValidResultProduced();
	void pingLastTimeValidResultProduced();
private:
	uint64_t mLastTimeValidResultProduced = 0;

	uint64_t mLastTimeResultProduced = 0;
	std::future<bool> mCompilationResult;
	uint64_t mCompilingSince = 0;
	uint64_t mRestingSince = 0;
	uint64_t mCrashCountWindowed = 0;
	uint64_t mCrashCountTotal = 0;
	uint64_t mAvrKernelExecTime = 0;
	uint64_t mCrashCountWindowClearedTimestamp = 0;
	std::recursive_mutex mFieldsGuardian;
	std::recursive_mutex mCurrentTaskChangeGuardian;
	std::recursive_mutex mGuardian;
	void initFields();
	std::vector<uint8_t> mGroupID;//group contains clones of a single hardware device
	bool enqueueWriteBuffer(std::vector<IComputeTask> &tasks);
	bool compileKernelByComputeTask();
	bool executeKernelByComputeTask(std::vector<IComputeTask> &tasks);
	bool setStepSize(unsigned int step);
	unsigned int getCurrentStepSize();
	void checkStepBounds();
	uint64_t mTaskCount = 0;
	uint64_t mTasksCompleted = 0;

	bool mKernelCompiled=false;
	bool mKernelCompiledAtLeastOnce = false;

	size_t mTimeInTests = 0;
	int mStepsCount = 0;
	unsigned int mStep = 0;
	unsigned int mSingleCumputeMemReq = 128;// size of hash_buffer entry required by single kernel
	size_t mMaxStepSize = 0;
	size_t mLocalThreadsCount = 0;
	size_t mWorkOffset = 0;
	bool createBuffers();
	std::thread mWorkThread;
	std::thread mCollectorThread;

	std::shared_ptr<CTools> mTools;
	eWorkerState mState = eWorkerState::initial;
	cl_device_id mDevice = nullptr;
	std::string mWorkerName = "unknown";
	std::string mShortName = "unknown";
	unsigned int mMaxComputeUnits=0;
	unsigned int mMaxWorkGroupSize = 0;
	cl_ulong mMaxMemAlloc = NULL;
	cl_context mContext = NULL;
	void collectors_thread();

	bool isWorkingState();

	void worker_thread();
	std::shared_ptr<IWorkCL> mCurrentTask;
	bool restoreSettings();
	CWorker::eWorkerType mWorkerType;
	double mCurrentMhps=0;
	uint64_t mKernelExecTime=0;
	std::array<double, 10> mMhpsSamples{ 0 };
	int mLastMhpsSampleIndex=0;
	std::mutex mLockCollectorThread;
	std::mutex mLockWorkerThreadThread;
	cl_command_queue mCommandQueue=0;
	uint64_t mWarningsCount = 0;
};

#endif
