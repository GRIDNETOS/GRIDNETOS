#ifndef WORK_HYBRID_H
#define WORK_HYBRID_H
#include "enums.h"

#include "..\interfaces\iworkhybrid.h"
class CWorkHybrid :
	public IWorkHybrid
{
public:
	CWorkHybrid(std::shared_ptr<IWork> parentTask,eBlockchainMode::eBlockchainMode blockchainMode);
	~CWorkHybrid();
	int64_t checkForResults();
	void releaseResources();
	bool executeKernel();
	bool compileKernel();
	std::vector< std::shared_ptr<IWorkCL>> divideIntoNr(int intoNr, int shortID, size_t workStepSize);
	IComputeTask *initialiseComputeTask();
	bool setKernelArgs();
	void setWorkResult(std::shared_ptr<IWorkResult> result);
	std::vector<std::shared_ptr<IWorkResult>> getWorkResult(bool includeInterediaryResults = true);
	std::string getReport();
};

#endif