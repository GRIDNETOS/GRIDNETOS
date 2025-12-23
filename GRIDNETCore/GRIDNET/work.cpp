#include "work.h"
#include "Scheduler.h"
#include <algorithm>


CWork::CWork() : IWorkCL()
{
    setState(eWorkState::initial);
    markAsTestWork();
    setCurrentNonce(0);
    setPausedAt(0);
    setIsFinished(false);
    setWorkStart(0);
    setWorkEnd(0);
    setWorkOffset(0);
}
CWork::CWork(const CWork &w)
{
	
}
CWork::CWork(std::array<unsigned char, 32> workDifficulty, std::array<unsigned char, 32>  minDifficulty,
	unsigned int workStart, unsigned int workEnd) : IWorkCL()
{
    setState(eWorkState::initial);
    setParentTask(nullptr);
    setCurrentNonce(0);
    setWorkOffset(0);
    setPausedAt(0);
    setIsFinished(false);
	
    setMainWorkDifficulty(workDifficulty);
    setMinDifficulty(minDifficulty);

    setWorkStart(workStart);
    setWorkEnd(workEnd);
}

