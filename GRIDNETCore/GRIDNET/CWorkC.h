#ifndef WORK_K
#define WORK_H

#include "../interfaces/iwork.h"
#include "enums.h"
class CWork :
	public IWork
{
public:
	bool isAccessible();
	CWork(eBlockchainMode::eBlockchainMode blockchainMode);
	~CWork();
	void cleanUp();
	void setIndex(int index) { mIndex = index; }
	int getIndex() { return mIndex; }
protected:
	void mainThread();
private:
	int mIndex = 0;
};


#endif // !WORK_K