#include "CWorkC.h"
#include <thread>

#include "BlockchainManager.h"
using namespace std::chrono_literals;

bool CWork::isAccessible()
{
	return true;
}

CWork::CWork(eBlockchainMode::eBlockchainMode blockchainMode) : IWork(blockchainMode)
{
	
}


CWork::~CWork()
{
}

void CWork::cleanUp()
{
}

void CWork::mainThread()
{
	eWorkState state = getState();
	while (state == eWorkState::initial ||
		state == eWorkState::running ||
		state == eWorkState::onTheWayToFactory ||
		state == eWorkState::preparing)
	{
		std::this_thread::sleep_for(1000ms);
		CTools::getTools()->writeLine("Task ID: " + std::to_string(getIndex()) + " running time in ms: " +std::to_string( getRunningTime()),true,true,
			eViewState::eventView);
	}
}
