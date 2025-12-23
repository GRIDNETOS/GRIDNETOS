#include "CFileSystemServer.h"
#include "BlockchainManager.h"
#include "NetworkManager.h"
#include "WebSocketServer.h"
#include "CFileSystemSession.h"
#include "CGlobalSecSettings.h"
#include "CFileSystemSession.h"
#include "conversationState.h"

CFileSystemServer::CFileSystemServer(std::shared_ptr<CBlockchainManager> bm)
{
	mBlockchainManager = bm;
	mNetworkManager = bm->getNetworkManager();
	assert(mNetworkManager != nullptr);
	mWebSocketsServer = mNetworkManager->getWebSocketsServer();
	assert(mWebSocketsServer != nullptr);

}

bool CFileSystemServer::initialize()
{
	//the server is responsible mainly for managing states ofthe  underlying file-system sessions.
	return true;
}

void CFileSystemServer::stop()
{
	std::lock_guard<std::mutex> lock(mGuardian);

	//firs ensure no new session come-in
	if (getStatus() == eManagerStatus::eManagerStatus::stoppped)
		return;

	mStatusChange = eManagerStatus::eManagerStatus::stoppped;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::stoppped)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CFileSystemServer::mControllerThreadF, this);

	while (getStatus() != eManagerStatus::eManagerStatus::stoppped && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	if (mControllerThread.joinable())
		mControllerThread.join();

	//then clean-up data structures / free memory

	doCleaningUp(true);//kill all active ones


	mTools->writeLine("File-System's Remote Session Manager killed;");
}

void CFileSystemServer::pause()
{
	if (getStatus() == eManagerStatus::eManagerStatus::paused)
		return;

	mStatusChange = eManagerStatus::eManagerStatus::paused;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::paused)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CFileSystemServer::mControllerThreadF, this);

	while (getStatus() != eManagerStatus::eManagerStatus::paused && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	doCleaningUp(true);
	mTools->writeLine("File-system Server paused;");
}

void CFileSystemServer::resume()
{
	if (getStatus() == eManagerStatus::eManagerStatus::running)
		return;
	mStatusChange = eManagerStatus::eManagerStatus::running;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::running)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CFileSystemServer::mControllerThreadF, this);


	while (getStatus() != eManagerStatus::eManagerStatus::running && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	mTools->writeLine("File-system Server resumed;");
}

void CFileSystemServer::mControllerThreadF()
{
	std::string tName = "File-System Server Controller";
	mTools->SetThreadName(tName.data());
	setStatus(eManagerStatus::eManagerStatus::running);
	bool wasPaused = false;
	if (mStatus != eManagerStatus::eManagerStatus::running)
		setStatus(eManagerStatus::eManagerStatus::running);
	uint64_t justCommitedFromHeaviestChainProof = 0;

	mFileSystemServerThread = std::thread(&CFileSystemServer::fileSystemServerThreadF, this);

	while (mStatus != eManagerStatus::eManagerStatus::stoppped)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		wasPaused = false;
		doCleaningUp();
		if (mStatusChange == eManagerStatus::eManagerStatus::paused)
		{
			setStatus(eManagerStatus::eManagerStatus::paused);
			mStatusChange = eManagerStatus::eManagerStatus::initial;

			while (mStatusChange == eManagerStatus::eManagerStatus::initial)
			{

				if (!wasPaused)
				{
					mTools->writeLine("My thread operations were freezed. Halting..");
					if (mFileSystemServerThread.native_handle() != 0)
					{
						while (!mFileSystemServerThread.joinable())
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
						mFileSystemServerThread.join();
					}

					wasPaused = true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		if (wasPaused)
		{
			mTools->writeLine("My thread operations are now resumed. Commencing further..");
			mFileSystemServerThread = std::thread(&CFileSystemServer::fileSystemServerThreadF, this);
			mStatus = eManagerStatus::eManagerStatus::running;
		}


		if (mStatusChange == eManagerStatus::eManagerStatus::stoppped)
		{
			mStatusChange = eManagerStatus::eManagerStatus::initial;
			mStatus = eManagerStatus::eManagerStatus::stoppped;

		}

	}
	doCleaningUp(true);//kill all the connections; do not allow for Zombie-connections
	setStatus(eManagerStatus::eManagerStatus::stoppped);
}

eManagerStatus::eManagerStatus CFileSystemServer::getStatus()
{
	std::lock_guard<std::mutex> lock(mStatusGuardian);
	return mStatus;
}

void CFileSystemServer::setStatus(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusGuardian);
	if (mStatus == status)
		return;
	mStatus = status;
	switch (status)
	{
	case eManagerStatus::eManagerStatus::running:

		mTools->writeLine("I'm now running");
		break;
	case eManagerStatus::eManagerStatus::paused:
		mTools->writeLine(" is now paused");
		break;
	case eManagerStatus::eManagerStatus::stoppped:
		mTools->writeLine("I'm now stopped");
		break;
	default:
		mTools->writeLine("I'm nowin an unknown state;/");
		break;
	}
}

void CFileSystemServer::requestStatusChange(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	mStatusChange = status;
}

eManagerStatus::eManagerStatus CFileSystemServer::getRequestedStatusChange()
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	return mStatusChange;
}

size_t CFileSystemServer::getLastTimeCleanedUp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastTimeCleaned;
}

size_t CFileSystemServer::getActiveSessionsCount()
{
	std::lock_guard<std::mutex> lock(mSessionsGuardian);
	return mSessions.size();
}

std::shared_ptr<CFileSystemSession> CFileSystemServer::getSessionByID(std::vector<uint8_t> id)
{
	std::lock_guard<std::mutex> lock(mSessionsGuardian);
	for (uint64_t i = 0; i < mSessions.size(); i++)
	{
		if (mTools->compareByteVectors(mSessions[i]->getID(),id))
			return mSessions[i];
	}
	return nullptr;
}

std::shared_ptr<CFileSystemSession> CFileSystemServer::getSessionByIP(std::string IP)
{
	std::lock_guard<std::mutex> lock(mSessionsGuardian);
	for (uint64_t i = 0; i < mSessions.size(); i++)
	{
		if (mTools->doStringsMatch(mSessions[i]->getClientIP(), IP))
			return mSessions[i];
	}
	return nullptr;
}

size_t CFileSystemServer::getMaxSessionsCount()
{
	return CGlobalSecSettings::getMaxSimultaniousFileSystemSessionsCount();
}

void CFileSystemServer::setLastTimeCleanedUp(size_t timestamp)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mLastTimeCleaned = timestamp;
}

uint64_t CFileSystemServer::cleanSessions()
{
		std::lock_guard<std::mutex> lock(mSessionsGuardian);
		size_t time = mTools->getTime();
		uint64_t removedCount = 0;
		//std::vector <std::shared_ptr<CConversation>>::iterator it;
		//for (std::vector< std::shared_ptr<CConversation>::iterator it = v.begin(); it != v.end(); ++it)
		for (std::vector <std::shared_ptr<CFileSystemSession>>::iterator it = mSessions.begin(); it != mSessions.end(); ) {

			if ((*it)->getState()->getLastActivity() > time)
			{
				++it;
				continue;
			}

			if ((*it)->getState()->getCurrentState() == eDFSSessionStatus::ended || ((time - (*it)->getState()->getLastActivity()) > 90))
			{
				(*it)->end();
				it = mSessions.erase(it);
				removedCount++;
			}
			else {
				++it;
			}
		}
		return removedCount;
	
}

void CFileSystemServer::fileSystemServerThreadF()
{
}

bool CFileSystemServer::initializeServer()
{
	return false;
}

uint64_t CFileSystemServer::doCleaningUp(bool forceKillAllSessions)
{
	std::lock_guard<std::mutex> lock(mSessionsGuardian);
	setLastTimeCleanedUp(mTools->getTime());
	if (forceKillAllSessions)
	{
		for (int i = 0; i < mSessions.size(); i++)
		{
			mSessions[i]->end();
		}
		setLastTimeCleanedUp(mTools->getTime());
		return mSessions.size();
	}
	else
	{
		return cleanSessions();
		setLastTimeCleanedUp(mTools->getTime());
	}
}
