#ifndef OCL_ENGINE_H
#define OCL_ENGINE_H

#include "stdafx.h"
#include "worker.h"
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include "CL/cl.h"
#endif


class COCLEngine
{
private:
    bool mInitialised = false;

    std::vector<cl_context> mContexts;
    cl_uint mRetNumPlatforms;
    cl_program mProgram;
	std::unique_ptr<CTools> mTools;
    std::vector<cl_platform_id> mPlatforms;
    std::vector<std::shared_ptr<CWorker>> mWorkers;
    std::vector<std::shared_ptr<CWorker>> mChosenWorkers;
	std::recursive_mutex mFieldsGuardian;
public:
	bool getWasInitialised();
	~COCLEngine();
	COCLEngine();
	bool Initialize(bool useCPU = false, bool useGPU=false);
	std::vector<cl_platform_id>*  getPlatforms();
	
	std::vector<std::shared_ptr<CWorker>>  getWorkers();
	std::vector<std::shared_ptr<CWorker>>  getWorkersByGroupID(std::vector<uint8_t> groupID);

};

#endif
