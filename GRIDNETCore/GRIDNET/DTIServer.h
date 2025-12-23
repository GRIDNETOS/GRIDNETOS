#pragma once
#include "stdafx.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>
#include <limits>  
#include <stdlib.h>
#include <algorithm>  
#include "ScriptEngine.h"
#include "arith_uint256.h"
#include "uint256.h"
#include <iostream>
#include <cctype>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include "enums.h"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include "IManager.h"
#include <deque>

#define ssh_callbacks_init(p) do {\
   (p)->size=sizeof(*(p)); \
} while(0);
void callbackConn(ssh_bind sshbind, void* userdata);
class CConversation;
class CNetworkManager;
class CDTI;
class CTools;
class CQRIntentResponse;
class CNetMsg;
class CVMMetaSection;
typedef struct ssh_bind_struct* ssh_bind;
/// <summary>
/// DTI (Decentralized Terminal Interface) Server takes care of managing DTI Sessions.
/// The lifeteime of DTI Server is controlled by the Network Manager but all of its internal operations are
/// fully independent.
/// </summary>
class CDTIServer: public  std::enable_shared_from_this<CDTIServer>, public IManager
{
private:
	std::mutex mGameQueueGuardian;
	std::deque<std::weak_ptr<CDTI>> mGameQueue;//players are match-maked  from this queue
	ssh_bind mSSHBind;
	std::mutex mSSHBindGuardian;
	eManagerStatus::eManagerStatus mStatus;
	eManagerStatus::eManagerStatus mStatusChange;
	std::recursive_mutex mStatusGuardian, mGuardian;
	std::mutex mFieldsGuardian;
	std::recursive_mutex mSessionPoolGuardian;
	std::mutex mRecentWallLinesGuardian;
	std::deque<std::string> mRecentWallLines;
	ssh_session mSSHSession;
	uint64_t mLastControllerLoopRun;
	bool mShutdown;
	std::thread mDTIServerThread, mControllerThread;
	ssh_bind_callbacks_struct bcs;
	//ssh_bind_callbacks bc = &bcc;
	ssh_bind_incoming_connection_callback b;
	std::shared_ptr<CNetworkManager> getNetworkManager();
	void DTIServerThreadF();
	bool initializeServer();
	std::vector<std::shared_ptr<CDTI>> mDTIConnections;
	void doCleaningUp(bool forceKillAllSessions=false);
	size_t mLastTimeCleaned;
	size_t mMaxSessionInactivity;
	size_t mMaxWarningsCount;
	std::shared_ptr<CTools> mTools;
	bool createDTISession(ssh_session session);

	
	
	size_t mMaxSessions;
	size_t getActiveSessionsCount();
	std::mutex mStatusChangeGuardian;
	std::shared_ptr<CNetworkManager> mNetworkManager;

	//Sessions' Double Buffering - BEGIN
	uint64_t mDTISessionsBufferTimestamp;//we employ double-buffering of Remote Terminal Sessions
	//the buffer is used for low-priority operations not to stuck the main processing of the Terminal Service.
	std::mutex mDoubleSessionsBufferGuardian;
	std::vector<std::shared_ptr<CDTI>> mDTIConnectionsDB;
	bool updateSessionsDB();
	uint64_t mDBUpdateInterval;
	void pingDBUpdate();
	bool mDBInProgress;

	bool getDBInProgress();
	void setDBInProgress(bool isIt=true);

	uint64_t getDBTimestamp();
	void setDBTimestamp(uint64_t time);
	void pingtLastControllerLoopRun();
	//Sessions' Double Buffering - END

public:
	uint64_t getLastControllerLoopRun();
	void registerDTISession(std::shared_ptr<CDTI> dti);
	void enquePlayer(std::shared_ptr<CDTI> dti);
	std::shared_ptr<CDTI> dequePlayer();

	std::shared_ptr<CDTI> getPlayerFor(std::shared_ptr<CDTI> dti);

	uint64_t getPlayersQueueLength();
	
	void broadcastMsgToAll(std::string msg, std::string whoSays, eViewState::eViewState view = eViewState::Wall, eColor::eColor color = eColor::none,std::shared_ptr<CDTI> exceptFor=nullptr);
	void addWallLine(std::string text, std::string whoSays);
	std::vector<std::string> getRecentWallLines();
	uint64_t getDBUpdateInterval();
	void setDBUpdateInterval(uint64_t interval);
	uint64_t getSessionsCount();
	std::vector<std::shared_ptr<CDTI>> getSessions(eViewState::eViewState inView = eViewState::unspecified, bool onlyActive = true);
	~CDTIServer();
	void destroySSH();
	std::vector<uint8_t> registerQRIntentResponse(std::shared_ptr<CQRIntentResponse> response);
	std::vector<uint8_t> registerVMMetaDataResponse(std::shared_ptr<CNetMsg> msg, std::vector<std::shared_ptr<CVMMetaSection>> sections);
	std::shared_ptr<CDTI> getDTIbyID(std::vector<uint8_t> id);
	size_t getMaxSessionsCount();
	std::string getSSHClientIP(ssh_session session);
	std::string getSSHClientIP(ssh_bind bind);
	std::string getSSHClientIP(socket_t socket);
	void setMaxSessionsCount(size_t nr);
	size_t getLastTimeCleanedUp();
	void setLastTimeCleanedUp(size_t timestamp=0);
	CDTIServer(std::shared_ptr<CNetworkManager> nm);
	bool initialize();
	// Inherited via IManager
	virtual void stop() override;
	virtual void pause() override;
	virtual void resume() override;
	void mControllerThreadF();
	virtual eManagerStatus::eManagerStatus getStatus() override;
	virtual void setStatus(eManagerStatus::eManagerStatus status) override;

	// Inherited via IManager
	virtual void requestStatusChange(eManagerStatus::eManagerStatus status) override;
	virtual eManagerStatus::eManagerStatus getRequestedStatusChange() override;
};