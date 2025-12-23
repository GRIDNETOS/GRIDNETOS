#pragma once
#include "stdafx.h"

class CPoWJob
{
private:
	int jobDifficulty;
	int jobIdentifier;
	std::vector<CPoW> currentTasks;
	std::vector<CPoW> accomplishedJob;
	bool isFinished = false;

public:
	CPoWJob::CPoWJob(int jobDifficulty);
	void setJobDifficulty(int diff);
	void setJobIdentifier(int id);
	std::vector<CPoW> * getCurrentTasks();
	std::vector<CPoW> * getAccomplishedJob();
	void cancelJob();

};