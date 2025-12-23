#include "powjob.h"

CPoWJob::CPoWJob(int jobDifficulty)
{

	jobDifficulty = 0;
	jobIdentifier = 1000 + (rand() % (int)(1000000 - 1000 + 1));
}

void CPoWJob::setJobDifficulty(int diff)
{
	this->jobDifficulty = diff;
}

void CPoWJob::setJobIdentifier(int id)
{
	this->jobIdentifier = id;
}

std::vector<CPoW>* CPoWJob::getCurrentTasks()
{
	return nullptr;
}

std::vector<CPoW> * CPoWJob::getAccomplishedJob()
{
	if(!isFinished)
	return nullptr;

	else return &(this->accomplishedJob);
}

void CPoWJob::cancelJob()
{
	//stop running threads

	//clear data inside of database

	//clear in-memory data, free resources



	
}
