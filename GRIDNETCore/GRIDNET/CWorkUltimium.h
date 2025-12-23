#ifndef WORKCL_H
#define WORKCL_H
#include "enums.h"
#include <thread>
#include <mutex>
#include "../GRIDNET/Tools.h"
#include "../interfaces/iworkcl.h"

class CWorkUltimium : public IWorkCL
{
public:
	CWorkUltimium(std::shared_ptr<IWork> parentTask,eBlockchainMode::eBlockchainMode blockchainMode);
	CWorkUltimium(std::shared_ptr<CWorkUltimium>parentTask, eBlockchainMode::eBlockchainMode blockchainMode, std::array<unsigned char, 32> workTarget, std::array<unsigned char, 32>  minTarget,
		uint64_t workStart, uint64_t workEnd);
	CWorkUltimium(const CWorkUltimium &work);
	~CWorkUltimium();
	void setMainWorkDifficulty(const std::array<unsigned char, 32> &difficulty);
	void setMinDifficulty(const std::array<unsigned char, 32> &difficulty);
	virtual std::array<unsigned char, 32> getMinDifficulty();
	virtual std::array<unsigned char, 32> getMainDifficulty();



    void addUnverifiedNonce(unsigned int nonce);
	void clearUnverifiedNonce();
    void addPoW(std::shared_ptr<CPoW> pow);
	double calculatePartialProgress(const std::array<unsigned char, 32>& hash, const std::array<unsigned char, 32>& mainTarget);
	std::vector<unsigned int> getUnverifiedNonces();
	int64_t checkForResults() override;
	std::recursive_mutex mBuffersGuardian;
	void setDataToWorkOn(const std::vector<uint8_t> &data, const std::vector<uint8_t> &additionalData= std::vector<uint8_t>());
	bool executeKernel() override;
	static bool checkComponents(bool report=true, const std::vector<uint8_t>& currentEncHash= std::vector<uint8_t>());
	bool compileKernel() override;
	IComputeTask *initialiseComputeTask() override;
	std::vector<std::shared_ptr<IWorkResult>> getWorkResult(bool includeInterediaryResults=true) override;
	void setWorkResult(std::shared_ptr<IWorkResult> result) override;
	std::string getReport() override;
	bool setKernelArgs() override;
	void cleanUp();
	void markDataAsHeader();
	bool isDataHeader();
	void releaseResources() override;
	void releaseKernelsAndProgram();
	std::vector<std::shared_ptr<IWorkCL>> divideIntoNr(int intoNr, int shortID, size_t workStepSize) override;
	void cleanupResources();
	static unsigned int detectTargetWindowOffset(std::array<unsigned char, 32> difficulty);
private:

	// Partial Results Logging - BEGIN
	// Static member variables for logging control
	static std::mutex s_mutex;                       // For thread safety
	static int s_partialPoWLogCount;                 // Logs in the current minute
	static time_t s_logWindowStartTime;              // Start time of the current minute window
	static constexpr int s_maxLogsPerMinute = 20;     // Maximum logs allowed per minute
	static double s_progressThreshold;               // Dynamic threshold for logging

	// Partial Results Logging - END

	cl_program getProgram();
	void setProgram(cl_program program);
	bool mReleased=false;
	bool getWasReleased();
	void setWasReleased(bool wasIt=true);
	void setLocalThreadsCount(size_t count);
	size_t getLocalThreadsCount();
	std::shared_ptr<CTools> getTools();
  
	size_t getSingleCumputeMemReq();
    bool optimisePerformance(unsigned int processingTime, bool wasError);
    void createComputeTask();
	
	cl_program createProgram(cl_context context, cl_device_id device, const char* fileName);
    std::array<unsigned char, 32> mMainDifficulty;
    std::array<unsigned char, 32> mMinDifficulty;
    unsigned int mCurrentNonce=0;
    std::vector<unsigned int> mUnverifiedNonces;
    std::vector<std::shared_ptr<IWorkResult>> mAccomplishedPoWs;
    unsigned int mMinTargetOffset=0;
    unsigned int mMainTargetOffset=0;

	unsigned int getMinTargetOffset();
	void setMinTargetOffset(unsigned int offset);

	unsigned int getMainTargetOffset();
	void setMainTargetOffset(unsigned int offset);

	cl_mem getTestBuffer();
	cl_mem getHeaderBuffer();
	cl_mem getOutputBuffer();
	cl_mem getHashBuffer();

	void setTestBuffer(cl_mem buf);
	void setHeaderBuffer(cl_mem buf);
	void setOutputBuffer(cl_mem buf);
	void setHashBuffer(cl_mem buf);
	void setHashBufferSize(size_t size);
	size_t getHashBufferSize();

    cl_mem mTestBuffer = NULL;
    cl_mem mHeaderBuffer = NULL;
    cl_mem mOutputBuffer = NULL;
    cl_mem mHashBuffer = NULL;
    size_t mGlobalThreads=0;

    size_t mWorkOffset=0;
	bool mDataIsHeader=true;
	bool getDataIsHeader();
	void setDataIsHeader(bool isIt = true);

	cl_program mProgram = NULL;
	cl_kernel mKernelShavite = NULL;
	cl_kernel mKernelKeccak = NULL;
	cl_kernel mKernelHamsi = NULL;
	cl_kernel mKernelBlake = NULL;
	cl_kernel mKernelJH = NULL;
	cl_kernel mKernelSkein = NULL;
	

	std::vector<uint32_t> mBlankResults;
	std::vector<uint32_t>  mResults;//the nuber at the 0xFF index stores the number of found nonces which are contained within the prior bytes.
	std::vector < cl_ulong> mTestResults;//used to retrieve intermediary hash results etc. (debugging)

	double mAverageMhpsDuringTest=0;
	size_t mHashBufferSize = 0;
	size_t mSingleCumputeMemReq = 128;// size of hash_buffer entry required by single kernel
	//cl_int mRet = NULL;

	std::array<unsigned char, 32> mDeviceTarget;
	double mCurrentMhps=0;

	std::shared_ptr<CTools> mTools;// = CTools("Ultimium Task");
};

struct CompareWork
{
	bool operator()(std::shared_ptr<IWork> lhs, std::shared_ptr<IWork> rhs) const
	{
		if (lhs->getPriority() == rhs->getPriority())
			lhs->setPriority(lhs->getPriority() + 1);

		return lhs->getPriority() < rhs->getPriority();
	}
};

#endif
