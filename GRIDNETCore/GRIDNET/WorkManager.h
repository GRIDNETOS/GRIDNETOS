#ifndef WORK_MANAGER_H
#define WORK_MANAGER_H

#include "stdafx.h"
#include "WorkManager.h"
#include "oclengine.h"
#include "worker.h"
#include <set>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include "CL/cl.h"
#endif
#include "IManager.h"
#include <map> 
enum class eWorkManagerState {
	initial,
	waitingForWorkers,
	fullyOperational,
	noWorkersAvailable,
	shuttingDown,
	shutDown,
	running
	,invalid
};

class CWorkManager: public std::enable_shared_from_this<CWorkManager>,public IManager {
private:
	uint64_t mLastDevReportModeSwitch;
	uint64_t mLastDevReportMode;
	std::map<std::vector<uint8_t>, void*> mProgramCache;// key is a hash of an AND between computational group ID and hash of the kernel source
	std::recursive_mutex mGuardian;
	std::recursive_mutex mOCLEngineGuardian;
	std::recursive_mutex mWorkersGuardian;
	uint64_t mLastCompileWarningTimestamp = 0;
	uint64_t mLastCompiltingWarningLevel = 0;
	std::recursive_mutex mTasksGuardian;
	bool mHorsePowerKnown=false;
	bool getIsHorsePowerKnown();

	void setIsHorsePowerKnown(bool itIt=true);
	bool mStillGotSomeUnitializedWorkers = false;
	bool getGotuninitializedWorkers();
	void setGotuninitializedWorkers(bool isIt = true);
	double mTotalEstimatedHashRate=0;
	bool performanceUnknownTold = false;
	std::vector<std::shared_ptr<CWorker>> mReadyWorkers;
	std::vector<std::shared_ptr<CWorker>> getReadyWorkers();
	void handleCompilationWarnings(std::shared_ptr<CWorker> worker);
	uint64_t getPendingTasksCount(bool onlyTopLevel=false);
	void normalRoutine();

	void cleanUp();
	void stopWorkersAndCollect(std::shared_ptr<IWork>  work);
	void shuttingDownRoutine();
	size_t mCurrentMaxWorkID=0;
	eWorkManagerState mInternalState;
    bool divideWork(const std::vector<uint8_t> &workIndex, int divideIntoNr, bool assignExactCount=true);
    void controllerThreadF();
	std::vector<std::tuple<IWork*, std::shared_ptr<CWorker>>> mAssignedTasks;
	std::vector<std::shared_ptr<CWorker>> mWorkers;

    std::vector<std::shared_ptr<IWork>> mRegisteredTasks;
	bool mInitialised = false;
    int mCurrentJob = 0;
    std::thread mControllerThread;
	int nrOfCPUHardwareCores = 0;
 
	std::shared_ptr<COCLEngine> mOCLEngine;

    std::shared_ptr<IWork> getTaskOfGivenID(const std::vector<uint8_t> &workID);
    void sortRegisteredTasks();
	std::shared_ptr<CTools> mTools;
	size_t mMaxTimeInTestsPerWorker;
	bool mFirstPass = true;
	eManagerStatus::eManagerStatus mStatus;
	bool mEnableProgramCaching;
	eManagerStatus::eManagerStatus mStatusChange;
	std::mutex mStatusChangeGuardian;
	size_t mMaxNrOfRegisteredTasks;
	std::mutex mProgramCacheGuardian;
	double mAverageMhps = 0;
	static std::mutex sInstanceGuardian;
	cl_program CreateProgramFromBinary(cl_context context, cl_device_id device, const char* fileName);
	bool SaveProgramBinary(cl_program program, cl_device_id device, const char* fileName);
	std::recursive_mutex mFieldsGuardian;
public:	
	std::shared_ptr<CTools> getTools();
	uint64_t getLastCompileWarningTimestamp();
	void pingLastCompileWarningTimestamp();
	uint64_t getLastCompilingWarningLevel();
	void setLastCompilingWarningLevel(uint64_t level);
	std::string getStatusBarReport();
	static std::shared_ptr<CWorkManager> mInstance;
	std::shared_ptr<COCLEngine> getOCLEngine();
	bool isProgramCachingEnabled();
	static std::shared_ptr<CWorkManager> getInstance(std::shared_ptr<COCLEngine> engine=nullptr);
	std::vector<std::shared_ptr<CWorker>> getWorkersByGroupID(std::vector<uint8_t> groupID);
	cl_program getProgramFromCache(std::vector<uint8_t> key, bool useColdStorage, cl_context ,cl_device_id devID);
	bool putProgramIntoCache(std::vector<uint8_t> key, cl_program prog, bool useColodStorage, cl_context, cl_device_id devID);
	double getCurrentTotalMhps();
	double getAverageMhps();
	std::vector<std::shared_ptr<CWorker>>  getWorkers();
	void handleCompilationFailure(std::shared_ptr<CWorker> worker);
	bool isPerformanceKnown();
	eWorkState getWorkState(std::vector<uint8_t> id);
	eWorkState getWorkState(size_t shortID);
	std::vector<std::shared_ptr<IWorkResult>> getWorkResults(std::vector<uint8_t> workID,bool includeIntermediaries=true);
	void setState(eWorkManagerState state);
	std::vector<std::shared_ptr<CWorker>> getHealthyAvailableWorkers();
	void Initialise();
	bool wasInitialised();

	void registerTask(std::shared_ptr<IWork> work);
	void assignTaskTo(IWorkCL *work, std::shared_ptr<CWorker> worker);
    void unregisterTask(const std::vector<uint8_t> &workID);
    void abortWork(const std::vector<uint8_t> &workID);
	uint64_t abortAllTasks(bool waitTillAllWorkersReady =false, uint64_t timeout=60);
	bool setWorkItemStatus(std::vector<uint8_t> id,eWorkState state);
	CWorkManager(std::shared_ptr<COCLEngine> engine);
	void prepareWorkers();
	double getTotalComputationalPower(bool includeOnlyReadyWorkers = false);
	void setMaxTimeInTestsPerWorker(size_t time);
	~CWorkManager();
    uint32_t getNrOfTasks() const;
    uint32_t getNrOfTasks(eWorkState state);
    std::vector<std::vector<uint8_t>> getTaskIDs(eWorkState state,bool onlyMainTasks=false);
    uint64_t getRunningTime(const std::vector<uint8_t> &taskID);
    void setWorkPriority(const std::vector<uint8_t> &taskID, uint32_t priority);



	// Inherited via IManager
	virtual void stop() override;

	virtual void pause() override;

	virtual void resume() override;

	virtual eManagerStatus::eManagerStatus getStatus() override;

	virtual void setStatus(eManagerStatus::eManagerStatus status) override;


	// Inherited via IManager
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status) override;

	virtual eManagerStatus::eManagerStatus getRequestedStatusChange() override;

};

#endif
