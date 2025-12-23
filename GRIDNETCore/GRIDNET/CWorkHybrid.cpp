#include "CWorkHybrid.h"



CWorkHybrid::CWorkHybrid(std::shared_ptr<IWork> parentTask,eBlockchainMode::eBlockchainMode blockchainMode) : IWorkHybrid(parentTask,blockchainMode)
{
}


CWorkHybrid::~CWorkHybrid()
{
}

std::vector< std::shared_ptr<IWorkCL>> CWorkHybrid::divideIntoNr(int intoNr, int shortID, size_t workStepSize)
{
	std::vector< std::shared_ptr<IWorkCL>> vec;
	return vec;
}

int64_t CWorkHybrid::checkForResults()
{//use getWorker();
	//assert(getWorker()!=NULL);
	return 1;
}
void CWorkHybrid::releaseResources()
{
}
bool CWorkHybrid::executeKernel()
{//use getWorker();
	//assert(getWorker()!=NULL);
	return true;
}
bool CWorkHybrid::compileKernel()
{
	return true;
}
IComputeTask *CWorkHybrid::initialiseComputeTask()
{
	return nullptr;
}
bool CWorkHybrid::setKernelArgs()
{
	return true;
}

void CWorkHybrid::setWorkResult(std::shared_ptr<IWorkResult>result)
{
}

std::vector<std::shared_ptr<IWorkResult>> CWorkHybrid::getWorkResult(bool includeInterediaryResults)
{
	return std::vector<std::shared_ptr<IWorkResult>>();
}

std::string CWorkHybrid::getReport()
{
	return std::string();
}
