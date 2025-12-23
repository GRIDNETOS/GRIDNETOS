
#include "Scheduler.h"
#include "stdafx.h"
#include <chrono>
#include <thread>
#include "oclengine.h"
#include <iomanip>
#include "Tools.h"

bool CWorkManager::divideWork(const std::vector<uint8_t> &workID, int divideIntoNr)
{

    //TODO: divide work proportionally to worker's computational power
    unsigned int searchSpaceSize = (pow(2, 32) - 1);

    unsigned int workStepSize = searchSpaceSize / divideIntoNr;
    IWork * work=NULL;
    std::vector<IWork* >::iterator it;

    it = registeredTasks.begin();
    for (it = registeredTasks.begin(); it != registeredTasks.end(); it++) {
         IWork* w = *it;
         if (std::memcmp(w->getID().data(), workID.data(), w->getID().size()) == 0)
         {
             work = *it;
             break;
         }
    }


    if (work == NULL)
        return false;
    if (work->getCurrentTasks().size() > 1)
        return false;//work already divided
    std::string ids;
    CTools tools;
    if (work->getType() == eTaskType::openCL)
    {
        for (int i = 0; i < divideIntoNr; i++)
        {
            //std::array<unsigned char, 32> vr = *(work->getMinDifficulty());
            IWorkCL *w = dynamic_cast<IWorkCL*>(work);
            CWorkCL *subTask = new CWorkCL(*(w->getMainDifficulty()), *(w->getMinDifficulty()), i*workStepSize, (i + 1)*workStepSize);
            subTask->setDataToWorkOn(*(w->getDataToWorkOn()));
            subTask->setParentTask(w);

            if (w->isTestWork())
                subTask->markAsTestWork();
            w->addSubTask(subTask);

            //TODO change ID to string
            std::string id;
            ids += tools.to_string(subTask->getID());
            if (i < divideIntoNr - 1)
                ids += ",";
        }
    }
    //TODO change ID to string
    std::cout << " Work " << tools.to_string(work->getID()) << " was divided into " << std::to_string(divideIntoNr) << " parts. IDs:  "<<ids<<"\n";
    return true;

}
void CWorkManager::controllerMain()
{
    std::vector<IWork*> current_tasks;
    using namespace std::literals::chrono_literals;
    while(engine->wasInitialised()==false)
        std::this_thread::sleep_for(100ms);

    std::vector<CWorker*> aWorkers;
    while (keepRunning)
    {

        std::vector<IWork*>::iterator it1;

        for (it1  = registeredTasks.begin(); it1 != registeredTasks.end();)
        {
            IWork* work = *it1;

            current_tasks = work->getCurrentTasks();

            std::vector<IWork*>::iterator itTasks;

            bool removed = false;
            for(itTasks = current_tasks.begin(); itTasks != current_tasks.end(); itTasks++)
            {
                if((*itTasks)->getType() == eTaskType::internal)
                {
                    switch ((*itTasks)->getState())
                    {
                    case eWorkState::initial:
                        (*itTasks)->setState(eWorkState::enqueued, false);
                        break;

                    case eWorkState::enqueued:
                        (*itTasks)->startTask();
                        break;
                    case eWorkState::aborted:
                        it1 = registeredTasks.erase(it1);
                        removed = true;
                        break;
                    }
                }
                if(registeredTasks.size() == 0)
                {
                    break;
                }
            }
            if (!removed)
                ++it1;
        }

        std::this_thread::sleep_for(100ms);
        bool horsePowerKnown = true;


        for (int i = 0; i < aWorkers.size(); i++)
        {
            if (aWorkers[i]->getAverageMhps() == 0)
                horsePowerKnown = false;
        }
        bool stillGotSomeUnitializedWorkers = false;
        for (int i = 0; i < availableWorkers.size(); i++)
        {
            switch (availableWorkers[i]->getState())
            {
            case workerState::initial:
                stillGotSomeUnitializedWorkers = true;
                availableWorkers[i]->initializeWorker();
                if (availableWorkers[i]->getAverageMhps() == 0)
                {
                    //horse-power of this worker is unknown yet
                    //let us enque some test-work

                }
                break;
            case workerState::stopping:
                availableWorkers[i]->joinThreads();
                availableWorkers[i]->cleanUp();
                availableWorkers[i]->setState(workerState::stopped);

                break;
            case workerState::ready:
                //lets give it some work in a loop below
                break;
            case workerState::running:
                //air control standing by
                ////the worker is currently performing mining operations
                ///lets check the reported mhps
                break;
            case workerState::stopped:
                //available_workers[i]->joinThread();
                availableWorkers[i]->activateWorker();
                //do some cleaning up, make it ready again
                break;
            case workerState::warming_up:
                stillGotSomeUnitializedWorkers = true;

                //lets see how it goes
                break;
            }
        }

        //if(currentJob == -1)//no current task assigned
        //{
        aWorkers.clear();
        aWorkers = getHealthyAvailableWorkers();
        if (stillGotSomeUnitializedWorkers)

            setState(eMState::waitingForWorkers);


        else
        {

            setState(eMState::fullyOperational);
            std::vector<IWork*>::iterator it;

            bool removed;
            for (it  = registeredTasks.begin(); it != registeredTasks.end(); )
            {
                removed= false;
                //const CWorkCL &w = *it;
                //IWork * work = (const_cast<CWorkCL*>(&w));
                IWork* work = *it;
                if(work->getType() != eTaskType::openCL)
                {
                    ++it;
                    continue;
                }
                if (wereHPWeightsCalculated == false && work->isTestWork() == false)
                    continue;// performance of workers has not been avaluated yet
                             //we do not know how to distribute real work
                if (work->isTestWork() && wereHPWeightsCalculated)
                {
                    std::cout << "removing test task, performance already known\n";
                    it = registeredTasks.erase(it);
                        break;
                }


                //CWorkCL *work = &registeredTasks[i];
                switch (work->getState())
                {

                case eWorkState::aborted:
                    it = registeredTasks.erase(it);
                    removed = true;
                    //registeredTasks.erase(registeredTasks.begin() + 1);
                    //if (i > 1)i--;

                    break;

                case eWorkState::done:
                    //work completed, nothing more to do here
                    //TODO: move to another queu
                    break;

                case eWorkState::initial:
                    //lets wait till the work is enqueued
                    break;
                case eWorkState::paused:
                    //nothing to do with the lazy one
                    break;

                case eWorkState::reWorkNeeded:
                    //lets change somethig inside the block, so we get a new nonce-search-space
                    //and resume

                    break;

                case eWorkState::running:
                    break;
                case eWorkState::onTheWayToFactory:
                    break;

                case eWorkState::enqueued:
                    if (work->getCurrentTasks().size()==1 && aWorkers.size() > 1) //this task was not divided yet
                        //among available workers
                    {
                        divideWork(work->getID(), aWorkers.size());
                    }

                    current_tasks.clear();
                    current_tasks = work->getCurrentTasks();
                    for (int a = 0; a < current_tasks.size(); a++)
                    {
                        switch (current_tasks[a]->getState())
                        {
                        case eWorkState::initial:
                            current_tasks[a]->setState(eWorkState::enqueued, false);
                            break;

                        case eWorkState::enqueued:
                            for (int b = 0; b < aWorkers.size(); b++)
                            {
                                if (aWorkers[b]->getState() == workerState::ready)
                                {
                                    if (current_tasks[a]->getType() == eTaskType::openCL)
                                    {
                                        dynamic_cast<IWorkCL*>(current_tasks[a])->assignTaskTo(aWorkers[b]);
                                    }
                                    break;
                                }
                            }
                            break;
                        }

                    }


                    break;


                }
                if (removed == false)
                    ++it;
            }
        }
    }

}

void CWorkManager::sortRegisteredTasks()
{
    std::sort(registeredTasks.begin(), registeredTasks.end(), [](IWork* lhs, IWork* rhs) {
		return lhs->getPriority() > rhs->getPriority();
    });
}

void CWorkManager::setWorkPriority(const std::vector<uint8_t> &taskID, uint32_t priority)
{
    IWork* task = getTaskOfGivenID(taskID);

    if(task)
    {
        task->setPriority(priority);
    }
}
void CWorkManager::setState(eMState state)
{
    if (state == this->state)
        return;
    else
    {
        switch (state)
        {
        case eMState::waitingForWorkers:
            std::cout << "scheduler is still waiting for workers..\n";
            break;
        case eMState::fullyOperational:
            std::cout << "scheduler is now fully operational\n";
            break;
        case eMState::initial:
            std::cout << "scheduler is in initial state\n";
            break;

        case eMState::noWorkersAvailable:
            std::cout << " no workers available !\n";
            break;
        }
        this->state = state;
    }
}

std::vector<CWorker*> CWorkManager::getHealthyAvailableWorkers()
{
     //this function return only unoccupied ready workers
     std::vector<CWorker*> hw;
    //int nr = 0;
     for (int i = 0; i < availableWorkers.size(); i++)
         if (availableWorkers[i]->getState() == workerState::ready)
             hw.push_back(availableWorkers[i]);

    return hw;
}

std::vector<CPoW> CWorkManager::getFinishedWork()
{
    return finishedWork;
}

void CWorkManager::Initialise()
{
    std::cout << "Initializing work scheduler..\n";
    currentJob = -1;
    if (!initialised) {
        controllerThread = std::thread(&CWorkManager::controllerMain, this);
        initialised = true;
    }
}

IWork *CWorkManager::getTaskOfGivenID(const std::vector<uint8_t> &workID)
{
    std::vector<IWork*>::iterator it;

    for(it = registeredTasks.begin(); it != registeredTasks.end(); it++)
    {
        if(std::memcmp((*it)->getID().data(), workID.data(), workID.size()) == 0)
            return *it;
    }
    return nullptr;
}

void CWorkManager::abort(const std::vector<uint8_t> &workID)
{
    IWork* task = getTaskOfGivenID(workID);
    if(task)
    {
        task->abort();
    }
}
std::vector<std::vector<uint8_t>> CWorkManager::getTaskIDs(eWorkState state)
{
    std::vector<IWork*>::iterator it;
    std::vector<std::vector<uint8_t>> vec;
    for(it = registeredTasks.begin(); it != registeredTasks.end(); it++)
    {
        if((*it)->getState() == state)
            vec.push_back((*it)->getID());
    }

    return vec;
}
uint64_t CWorkManager::getRunningTime(const std::vector<uint8_t> &taskID)
{
    IWork* task = getTaskOfGivenID(taskID);

    if(task)
        return task->getRunningTime();

    return 0;
}
uint32_t CWorkManager::getNrOfTasks(eWorkState state)
{
    uint32_t counter = 0;

    std::vector<IWork*>::iterator it;

    for(it = registeredTasks.begin(); it != registeredTasks.end(); it++)
    {
        if((*it)->getState() == state)
            counter++;
    }
    return counter;
}
uint32_t CWorkManager::getNrOfTasks() const
{
    return registeredTasks.size();
}
void CWorkManager::registerTask(IWork *work)
{
    work->setState(eWorkState::enqueued,false);
    registeredTasks.push_back(work);
    sortRegisteredTasks();
}

void CWorkManager::assignTaskTo(IWorkCL *work, CWorker *worker)
{
    assigned_tasks.push_back( std::make_tuple(work, worker));
}

void CWorkManager::unregisterTask(const std::vector<uint8_t> &workID)
{
    for (int i = 0; i < assigned_tasks.size(); i++)
    {
        if(std::memcmp(assigned_tasks[i]._Myfirst._Val->getID().data(), workID.data(), workID.size()) == 0)
        {

            assigned_tasks.erase(assigned_tasks.begin() +i );
        }
    }

}

CWorkManager::CWorkManager(COCLEngine *pEngine)
{
    wereHPWeightsCalculated = false;
    if (pEngine == NULL)
        throw std::abort;
    engine = pEngine;
    //context = engine->getContext();
    keepRunning = true;
    std::vector<CWorker *> workers = pEngine->getWorkers();

    for (int i = 0; i < workers.size(); i++)
    {
        availableWorkers.push_back(workers[i]);
    }
}

CWorkManager::~CWorkManager()
{
    keepRunning = false;
}
