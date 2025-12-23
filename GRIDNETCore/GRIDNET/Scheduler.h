#ifndef WORK_MANAGER_H
#define WORK_MANAGER_H

#include "stdafx.h"
#include "Scheduler.h"
#include "oclengine.h"
#include "workcl.h"
#include "worker.h"
#include <set>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include "CL/cl.h"
#endif
enum class eMState {
	initial,
	waitingForWorkers,
	fullyOperational,
	noWorkersAvailable
};

 class CWorkManager {
private:

	eMState state;
    bool divideWork(const std::vector<uint8_t> &workIndex, int divideIntoNr);
	CWorkCL currentMainTask;
    void controllerMain();
	std::vector<std::tuple<IWork*, CWorker*>> assigned_tasks;
	std::vector<CWorker*> availableWorkers;
	std::vector<CPoW> finishedWork;
    std::vector<IWork*> registeredTasks;
	bool initialised = false;
    int currentJob;
    std::thread controllerThread;
	int nrOfCPUHardwareCores;
	cl_context context;
    bool keepRunning;
	COCLEngine *engine;
	bool wereHPWeightsCalculated;
    IWork* getTaskOfGivenID(const std::vector<uint8_t> &workID);
    void sortRegisteredTasks();
public:
	void setState(eMState state);
	std::vector<CWorker*> CWorkManager::getHealthyAvailableWorkers();
	std::vector<CPoW> getFinishedWork();
	void Initialise();
	CWorkCL getWork(CWorker worker);
	void registerTask(IWork *work);
	void assignTaskTo(IWorkCL *work, CWorker *worker);
    void unregisterTask(const std::vector<uint8_t> &workID);
    void abort(const std::vector<uint8_t> &workID);
	CWorkManager(COCLEngine *engine);
	~CWorkManager();
    uint32_t getNrOfTasks() const;
    uint32_t getNrOfTasks(eWorkState state);
    std::vector<std::vector<uint8_t>> getTaskIDs(eWorkState state);
    uint64_t getRunningTime(const std::vector<uint8_t> &taskID);
    void setWorkPriority(const std::vector<uint8_t> &taskID, uint32_t priority);
};

#endif
