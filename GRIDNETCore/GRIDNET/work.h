#ifndef WORK_H
#define WORK_H

#include <thread>
#include <mutex>
#include "../interfaces/iworkcl.h"

class CWork : public IWorkCL
{
public:
    CWork();
    CWork(const CWork &w);
    CWork(std::array<unsigned char, 32> workDifficulty, std::array<unsigned char, 32>  minDifficulty,
        unsigned int workStart, unsigned int workEnd);
};

struct CompareWork
{
	bool operator()( IWork *lhs,  IWork *rhs) const
	{
        return lhs->getPriority() < rhs->getPriority();
	}
};

#endif
