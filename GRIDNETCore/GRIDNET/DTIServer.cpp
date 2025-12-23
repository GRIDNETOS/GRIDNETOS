#include "DTIServer.h"
#include "BlockchainManager.h"
#include "ScriptEngine.h"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include "DTI.h"

#include "Ws2tcpip.h"
#include "CGlobalSecSettings.h"
#include "WebSocketServer.h"
#include "conversation.h"
#include "VMMetaSection.h"
#include "VMMetaGenerator.h"
#ifdef HAVE_ARGP_H
#include <argp.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "NetMsg.h"
#include <NetworkManager.h>
#include "ThreadPool.h"
#include "cqrintentresponse.h"
#ifndef KEYS_FOLDER
#ifdef _WIN32
#define KEYS_FOLDER
#else
#define KEYS_FOLDER "/etc/ssh/"
#endif
#endif

#ifdef WITH_PCAP
const char* pcap_file = "debug.server.pcap";
ssh_pcap_file pcap;
void set_pcap(ssh_session session);
void set_pcap(ssh_session session) {
	if (!pcap_file)
		return;
	pcap = ssh_pcap_file_new();
	if (ssh_pcap_file_open(pcap, pcap_file) == SSH_ERROR) {
		printf("Error opening pcap file\n");
		ssh_pcap_file_free(pcap);
		pcap = NULL;
		return;
	}
	ssh_set_pcap_file(session, pcap);
}

void cleanup_pcap(void);
void cleanup_pcap() {
	ssh_pcap_file_free(pcap);
	pcap = NULL;
}
#endif

static int auth_password(const char* user, const char* password) {
	if (strcmp(user, "aris"))
		return 0;
	if (strcmp(password, "lala"))
		return 0;
	return 1; // authenticated
}
#ifdef HAVE_ARGP_H
const char* argp_program_version = "GRIDNET Decentralized Terminal Interface"
SSH_STRINGIFY(LIBSSH_VERSION);
const char* argp_program_bug_address = "<info@gridnet.org>";

/* Program documentation. */
static char doc[] = "libssh -- a Secure Shell protocol implementation";

/* A description of the arguments we accept. */
static char args_doc[] = "BINDADDR";

/* The options we understand. */
static struct argp_option options[] = {
  {
	.name = "port",
	.key = 'p',
	.arg = "PORT",
	.flags = 0,
	.doc = "Set the port to bind.",
	.group = 0
  },
  {
	.name = "hostkey",
	.key = 'k',
	.arg = "FILE",
	.flags = 0,
	.doc = "Set the host key.",
	.group = 0
  },
  {
	.name = "dsakey",
	.key = 'd',
	.arg = "FILE",
	.flags = 0,
	.doc = "Set the dsa key.",
	.group = 0
  },
  {
	.name = "rsakey",
	.key = 'r',
	.arg = "FILE",
	.flags = 0,
	.doc = "Set the rsa key.",
	.group = 0
  },
  {
	.name = "verbose",
	.key = 'v',
	.arg = NULL,
	.flags = 0,
	.doc = "Get verbose output.",
	.group = 0
  },
  {NULL, 0, 0, 0, NULL, 0}
};

/* Parse a single option. */
static error_t parse_opt(int key, char* arg, struct argp_state* state) {
	/* Get the input argument from argp_parse, which we
	 * know is a pointer to our arguments structure.
	 */
	ssh_bind sshbind = state->input;

	switch (key) {
	case 'p':
		ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, arg);
		break;
	case 'd':
		ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_DSAKEY, arg);
		break;
	case 'k':
		ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY, arg);
		break;
	case 'r':
		ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY, arg);
		break;
	case 'v':
		ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_LOG_VERBOSITY_STR, "3");
		break;
	case ARGP_KEY_ARG:
		if (state->arg_num >= 1) {
			/* Too many arguments. */
			argp_usage(state);
		}
		ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDADDR, arg);
		break;
	case ARGP_KEY_END:
		if (state->arg_num < 1) {
			/* Not enough arguments. */
			argp_usage(state);
		}
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };
#endif /* HAVE_ARGP_H */

void callbackConn(ssh_bind sshbind, void* userdata)
{
	struct sockaddr_storage tmp;
	struct sockaddr_in* sock;
	int len = 100;
	char ip[100] = "\0";
	getpeername(ssh_bind_get_fd(sshbind), (struct sockaddr*)&tmp, &len);
	sock = (struct sockaddr_in*)&tmp;
	inet_ntop(AF_INET, &sock->sin_addr, ip, len);
	std::string ip_str = ip;


	//std::string ip = getSSHClientIP(sshbind);

}
std::shared_ptr<CNetworkManager> CDTIServer::getNetworkManager()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mNetworkManager;
}


void CDTIServer::DTIServerThreadF()
{

	ssh_message message;
	ssh_channel chan = 0;
	bool abort = false;
	int result;
	mTools->SetThreadName("DTI Server");
	long timeout = 5;
	long compressionLevel = 4;
	bool bf = false;
	int verbosity = SSH_LOG_PROTOCOL;
	int blocking = 0;
	socket_t clientSocket;
	uint64_t lastDOSReport = 0;
	
	std::shared_ptr<CTools> tools = getNetworkManager()->getTools();
	uint64_t now = std::time(0);
	std::string clientIP;
	//ssh_bind_set_blocking(mSSHBind, blocking);
	CTools::getInstance()->setThreadPriority(ThreadPriority::HIGHEST);
	//accept incoming connections - BEGIN
	while (getStatus() == eManagerStatus::eManagerStatus::running)
	{
		// Refresh Variables - BEGIN
		now = std::time(0);
		// Refresh Variables - END
		
		Sleep(30);

		mSSHBindGuardian.lock();
		if (mSSHBind == nullptr)
		{
			mSSHBindGuardian.unlock();
			continue;
		}

		//Operational Logic - BEGIN
		abort = false;
		mSSHSession = ssh_new();

		ssh_options_set(mSSHSession, ssh_options_e::SSH_OPTIONS_TIMEOUT, &timeout);
		ssh_options_set(mSSHSession, ssh_options_e::SSH_OPTIONS_PROCESS_CONFIG, &bf);
		ssh_options_set(mSSHSession, ssh_options_e::SSH_OPTIONS_COMPRESSION_LEVEL, &compressionLevel);
		ssh_options_set(mSSHSession, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
		ssh_options_set(mSSHSession, SSH_OPTIONS_KEY_EXCHANGE, "diffie-hellman-group14-sha256,curve25519-sha256,ecdh-sha2-nistp256");
		//ssh_set_auth_methods(mSSHSession, SSH_AUTH_METHOD_NONE); <=doesn't seem to have any effect

		clientSocket = ssh_bind_accept_alone(mSSHBind); //just accept on the TCP socket, NOTHING more; gotta see who it is

		if (clientSocket == SSH_ERROR)
		{
			ssh_free(mSSHSession);
			mSSHBindGuardian.unlock();
			continue;
		}

		//see who it is
		clientIP = getSSHClientIP(clientSocket);
		eIncomingConnectionResult::eIncomingConnectionResult result;

		//Check if connection allowed - BEGIN
		if (mNetworkManager->getPublicIP().compare(clientIP) != 0)
			result = mNetworkManager->isConnectionAllowed(clientIP, true, eTransportType::SSH);

		abort = (result != eIncomingConnectionResult::allowed);
		//Check if connection allowed - END

		//Abort if so to be  (ex. DOS attack)- BEGIN
		if (abort)
		{
			if (abort)
			{

				if ((std::time(0) - lastDOSReport) > 10) {
					// Log the potential DoS attack.
					if (result == eIncomingConnectionResult::DOS)
					{
						tools->logEvent("mitigating DOS attack at the DTI (SSH) layer from " + clientIP, "Security", eLogEntryCategory::network,
							10, eLogEntryType::notification, eColor::cyborgBlood, true);
					}
					else
						if (result == eIncomingConnectionResult::limitReached)
						{
							tools->logEvent("Won't accept connection at the DTI (SSH) layer from " + clientIP+" Limits reached.", "Security", eLogEntryCategory::network,
								10, eLogEntryType::notification, eColor::cyborgBlood, true);
						}

						else
							if (result == eIncomingConnectionResult::onlyBootstrapNodesAllowed)
							{
								tools->logEvent("Won't accept connection at the DTI (SSH) layer from " + clientIP + " Now allowing only Bootstrap nodes.", "Security", eLogEntryCategory::network,
									10, eLogEntryType::notification, eColor::cyborgBlood, true);
							}
							else
								if (result == eIncomingConnectionResult::insufficientResources)
								{
									tools->logEvent("Won't accept connection at the DTI (SSH) layer from " + clientIP + " Insufficient Resources..", "Security", eLogEntryCategory::network,
										10, eLogEntryType::notification, eColor::cyborgBlood, true);
								}
					lastDOSReport = now;
				}
				//do NOT send ANYTHING to anyone.

				//ssh_disconnect(mSSHSession); <= nope
				ssh_kill_socket(clientSocket);
				//ssh_silent_disconnect(mSSHSession);//do so abruptly instead
				//ssh_free(mSSHSession);
				mSSHBindGuardian.unlock();
				continue;
			}
		}
		//Abort if so to be - BEGIN

		//Prepare an SSH Session - BEGIN

		//establish an SSH session; do SSH handshakes, play with Diffie-Hellman and stuff
		if (ssh_bind_session_init(clientSocket, mSSHBind, mSSHSession) == SSH_ERROR)
		{
			ssh_free(mSSHSession);
			mSSHBindGuardian.unlock();
			continue;
		}

		//spawn a new Shell
		createDTISession(mSSHSession);
		//Prepare an SSH Session - END
		
		//Operational Logic - END
		mSSHBindGuardian.unlock();
	}
	//accept incoming connections - END

}
/*
int ssh_bind_accept(ssh_bind sshbind, ssh_session session) {
	socket_t fd = SSH_INVALID_SOCKET;
	int rc;
	if (sshbind->bindfd == SSH_INVALID_SOCKET) {
		ssh_set_error(sshbind, SSH_FATAL,
			"Can't accept new clients on a not bound socket.");
		return SSH_ERROR;
	}

	if (session == NULL) {
		ssh_set_error(sshbind, SSH_FATAL, "session is null");
		return SSH_ERROR;
	}

	fd = accept(sshbind->bindfd, NULL, NULL);
	if (fd == SSH_INVALID_SOCKET) {
		ssh_set_error(sshbind, SSH_FATAL,
			"Accepting a new connection: %s",
			strerror(errno));
		return SSH_ERROR;
	}
	rc = ssh_bind_accept_fd(sshbind, session, fd);

	if (rc == SSH_ERROR) {
		CLOSE_SOCKET(fd);
		ssh_socket_free(session->socket);
	}
	return rc;
}
*/
/// <summary>
/// Initialized the DTI Server.
/// </summary>
/// <returns></returns>
bool CDTIServer::initializeServer()
{

	ssh_init();
	ssh_key key;
	ssh_pki_generate(ssh_keytypes_e::SSH_KEYTYPE_ECDSA_P521, 521, &key);
	std::lock_guard<std::mutex> lock(mSSHBindGuardian);
	mSSHBind = ssh_bind_new();
	ssh_callbacks_init(&bcs);
	bcs.incoming_connection = &callbackConn;

	ssh_bind_set_callbacks(mSSHBind, &bcs, nullptr);

	mSSHSession = ssh_new();
	std::string ip = "0.0.0.0";
	std::string keysFolder = ".\\SSH_KEYS\\ecds_key";
	uint64_t port = 22;
	int iError = -1;
	iError = ssh_bind_options_set(mSSHBind, SSH_BIND_OPTIONS_BINDPORT, &port);
	//char * b = "Welcome to GRIDNET-OS\r\n";
	//std::string banner = b; mTools->getBanner();
	//iError = ssh_bind_options_set(mSSHBind, SSH_BIND_OPTIONS_BANNER, banner.data());
	iError = ssh_bind_options_set(mSSHBind, SSH_BIND_OPTIONS_BINDADDR, ip.data());
	iError = ssh_bind_options_set(mSSHBind, SSH_BIND_OPTIONS_IMPORT_KEY, key);//todo: import key from file instead

	if (iError < 0)
	{
		mTools->writeLine("Bind options set failed " + std::string(ssh_get_error(mSSHBind)));
		return false;
	}

	if (ssh_bind_listen(mSSHBind) < 0) {
		mTools->writeLine("Error starting SSH socket: " + std::string(ssh_get_error(mSSHBind)));
		return false;
	}
	mControllerThread = std::thread(&CDTIServer::mControllerThreadF, this);

	return true;

}

/// <summary>
/// The function return CDTI pointer if (still) available.
/// The function might return nullptr if
/// 1)no players in queue
/// 2) the particular player is no longer available.
/// 
/// For efficiency no explicit queue cleaning implemented.
/// The queue gets cleaned by itself once players query it.
/// </summary>
/// <returns></returns>
std::shared_ptr<CDTI> CDTIServer::getPlayerFor(std::shared_ptr<CDTI> dti)
{
	if (dti == nullptr)
		return nullptr;

	std::lock_guard<std::mutex> lock(mGameQueueGuardian);
	bool localDTIPopped = false;
	std::shared_ptr<CDTI> otherPlayer;

	if (mGameQueue.size() > 1)
	{

		if (mGameQueue.front().lock() == nullptr || mGameQueue.front().lock()->getStatus() == eDTISessionStatus::ended)
		{
			mGameQueue.pop_front();

			if (mGameQueue.size() <= 1)
				return nullptr;
		}

		//check if it's the player's time to play - he/she needs to be at the queue's very front
		if (!mTools->compareByteVectors(mGameQueue.front().lock()->getID(), dti->getID()))
			return nullptr;

		//if so pop the player (for now, might need to be put-back at the front)
		mGameQueue.pop_front();

		//match-make him with a player right behind
		otherPlayer = mGameQueue.front().lock();
		if (otherPlayer == nullptr || otherPlayer->getStatus() != eDTISessionStatus::alive)
		{//failed
			mGameQueue.pop_front();//pop the nullptr (session must have ended)


			//failed thus place the player back at the very front of the queue, make him wait for another player to come
			mGameQueue.push_front(dti);
		}
		else
		{
			mGameQueue.pop_front();//we've matched players , pop the 2nd one
		}
	}
	dti->setIsServer(true);
	if (otherPlayer != nullptr)
	{
		otherPlayer->setIsServer(false);
		otherPlayer->getScriptEngine()->setExternalData(dti->getID());
	}
	return otherPlayer;
}

void CDTIServer::enquePlayer(std::shared_ptr<CDTI> dti)
{
	std::lock_guard<std::mutex> lock(mGameQueueGuardian);
	mGameQueue.push_back(dti);
}

/// <summary>
/// Cleanes up the session-pool.
/// Sets the last timestamp of doing so.
/// </summary>
void CDTIServer::doCleaningUp(bool forceKillAllSessions)
{
	std::lock_guard<std::recursive_mutex> lock(mSessionPoolGuardian);
	std::vector<std::shared_ptr<CDTI>>::iterator session = mDTIConnections.begin();
	size_t currentTime = mTools->getTime();
	std::shared_ptr<SE::CScriptEngine> thread;
	std::shared_ptr<CConversation> conversation;
	std::vector<uint8_t> threadID, conversationID;
	while (session != mDTIConnections.end()) {

		//Collect IDs - BEGIN
		thread = (*session)->getScriptEngine();
		if (thread)
		{
			threadID = thread->getID();
		}
		else
		{
			threadID.clear();
		}

		conversation = (*session)->getConversation();

		if (conversation)
		{
			conversationID = conversation->getID();
		}
		else
		{
			conversationID.clear();
		}

		//Collect IDs - END

		currentTime = mTools->getTime();
		//force kill OR check for inactivity.
		//if so, request
		if (forceKillAllSessions || ((*session)->getStatus() == eDTISessionStatus::alive
			&& (currentTime - (*session)->getLastInteractionTimeStamp()) > mMaxSessionInactivity)) {
			if(!(*session)->getHasQuestionPending())
			(*session)->writeLine(mTools->getColoredString(u8"\r\n⚠Killing your session due to inactivity..", eColor::lightPink));
			//Notify through VM-Meta-Data - BEGIN
			SE::vmFlags flags;
			
			std::shared_ptr<CConversation> conv = (*session)->getConversation();
			if (conv)
			{
				conv->notifyVMStatus(eVMStatus::disabled, flags, (*session)->getUIProcessID(), threadID, (*session)->getID(), conversationID);
				//Notify through VM-Meta-Data - END
			}
			(*session)->requestStatusChange(eDTISessionStatus::ended);//the session has been inactive for too long; killing and deallocating
		}
		//issue warnings - BEGIN 
		std::shared_ptr<SE::CScriptEngine> se = (*session)->getScriptEngine();

		if ((se && se->getRunningAppID() == eCoreServiceID::hexi)==false)
		{
			if (mMaxSessionInactivity > 30 && (currentTime - (*session)->getLastInteractionTimeStamp()) > (mMaxSessionInactivity - 30)
				&& currentTime - (*session)->getEventTime(eDTIEvent::warningIssued) > 60)
			{
				(*session)->logEvent(eDTIEvent::warningIssued);
				if (!(*session)->getHasQuestionPending())
					(*session)->writeLine(mTools->getColoredString(u8"[⚠WARNING #" + std::to_string((*session)->getWarningsCount()) + "]: Your session is about to end in 30 seconds due to inactivity..\a", eColor::lightPink));

				//Notify through VM-Meta-Data - BEGIN
				SE::vmFlags flags;
				flags.cmdExecutorAvailable = true;

				std::shared_ptr<CConversation> conv = (*session)->getConversation();
				if (conv)
				{
					conv->notifyVMStatus(eVMStatus::aboutToShutDown, flags, (*session)->getUIProcessID(), threadID, (*session)->getID(), conversationID);
				}
				//Notify through VM-Meta-Data - END
			}
		}

		if (mMaxWarningsCount > 1 && (*session)->getWarningsCount() > mMaxWarningsCount)
		{
			if (!(*session)->getHasQuestionPending())
			(*session)->writeLine(mTools->getColoredString(u8">⚠Terminating< Visit again when you are well-behaved.", eColor::lightPink));

			//Notify through VM-Meta-Data - BEGIN
			SE::vmFlags flags;

			std::shared_ptr<CConversation> conv = (*session)->getConversation();
			if (conv)
			{
				conv->notifyVMStatus(eVMStatus::disabled, flags, (*session)->getUIProcessID(), threadID, (*session)->getID(), conversationID);
			}
			//Notify through VM-Meta-Data - END

			(*session)->requestStatusChange(eDTISessionStatus::ended);
		}
		else if (mMaxWarningsCount > 1 && (*session)->getWarningsCount() > mMaxWarningsCount - 1 &&
			currentTime - (*session)->getEventTime(eDTIEvent::warningIssued) > 3)
		{
			(*session)->logEvent(eDTIEvent::warningIssued);
			if (!(*session)->getHasQuestionPending())
			(*session)->writeLine(mTools->getColoredString("u8[⚠WARNING #" + std::to_string((*session)->getWarningsCount()) + "]: Your session is about to be end due to abuse!", eColor::lightPink));
		
		
		}
		//issue warnings - END

		//check if DTI session ended, if so => forget it.
		if ((*session)->getStatus() == eDTISessionStatus::ended)
			session = mDTIConnections.erase(session);
		else
			session++;
	}
	setLastTimeCleanedUp();
}
/// <summary>
/// Creates and pools-back a new DTI Session;
/// </summary>
/// <param name="session"></param>
/// <returns></returns>
bool CDTIServer::createDTISession(ssh_session session)
{
	//is the session count below the set threshold?
	if (getActiveSessionsCount() >= getMaxSessionsCount())
		return false;
	std::shared_ptr<CDTI> dti = std::make_shared<CDTI>(mNetworkManager->getBlockchainMode(), eTransportType::SSH);

	//note the following is asynchronous thus we do not really get the state right away.
	//still we do not want to wait for the initizalizations to finish
	//what we want is to be able to server additional clients ASAP.
	//the DTI session will be taken care of by a specialized/ client-dedicated controller thread.
	//The specialized controller thread will also spawn two distinct threads taking care of user input and output.
	if (dti->initialize(session))
	{
		std::lock_guard<std::recursive_mutex> lock(mSessionPoolGuardian);
		mDTIConnections.push_back(dti);
		return true;
	}
	return false;
}

/// <summary>
/// Registers a DTI Session within the first (main)buffer. (recall double-buffering of session).
/// </summary>
/// <param name="dti"></param>
void CDTIServer::registerDTISession(std::shared_ptr<CDTI> dti)
{
	std::lock_guard<std::recursive_mutex> lock(mSessionPoolGuardian);
	mDTIConnections.push_back(dti);
}

void CDTIServer::broadcastMsgToAll(std::string msg, std::string whoSays = "", eViewState::eViewState view, eColor::eColor color, std::shared_ptr<CDTI> exceptFor)
{
	std::shared_ptr<CTools> tools = CTools::getInstance();
	msg =(whoSays.size() > 0 ? tools->getColoredString(std::string("[" + whoSays + "]"), eColor::lightCyan): "") + tools->getColoredString(msg, color);

	addWallLine(msg, "");
	std::vector<std::shared_ptr<CDTI>> sessions = getSessions();
	for (uint64_t i = 0; i < sessions.size(); i++)
	{
		if (sessions[i] == exceptFor)
		{
			continue;
		}
		if (tools->doStringsMatch(sessions[i]->getUserID(), whoSays))
		{
			continue;
		}
		sessions[i]->writeLine(msg, true, view);
	}
}

/// <summary>
/// Gets the number of active DTI Sessions
/// </summary>
/// <returns></returns>
size_t CDTIServer::getActiveSessionsCount()
{
	std::lock_guard<std::recursive_mutex> lock(mSessionPoolGuardian);
	return mDTIConnections.size();
}

/// <summary>
/// Facilitates double-buffering of locally known Remote Terminal Sessions.
/// The function return true if the second buffer was updates, which happens after
/// mDBUpdateInterval has elapsed.
/// The function returns true if either mDBUpdateInterval not elapsed yet OR a double buffering operation is currently in progress.
/// </summary>
/// <returns></returns>
bool CDTIServer::updateSessionsDB()
{/*	uint64_t mDTISessionsBufferTimestamp;//we employ double-buffering of Remote Terminal Sessions
	//the buffer is used for low-priority operations not to stuck the main processing of the Terminal Service.
	std::mutex mDoubleSessionsBufferGuardian;
	std::vector<std::shared_ptr<CDTI>> mDTIConnectionsDB;
	bool updateSessionsDB();*/

	//Validation - BEGIN
	if (getDBInProgress())
		return false;//we do not want this to be a blocking operation thus return immediatedly

	if ((getDBTimestamp() + getDBUpdateInterval()) > mTools->getTime())
	{
		return false;
	}
	//Validation - END
	std::lock_guard<std::mutex> lock1(mDoubleSessionsBufferGuardian);
	std::lock_guard<std::recursive_mutex> lock2(mSessionPoolGuardian);// enter the critical section

	// Operational Logic - BEGIN
	setDBInProgress();
	mDTIConnectionsDB.clear();

	for (uint64_t i = 0; i < mDTIConnections.size(); i++)
	{
		mDTIConnectionsDB.push_back(mDTIConnections[i]);
	}

	setDBInProgress(false);
	pingDBUpdate();
	// Operational Logic - END

	return true;
}

void CDTIServer::pingDBUpdate()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mDTISessionsBufferTimestamp = mTools->getTime();
}

bool CDTIServer::getDBInProgress()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mDBInProgress;
}

void CDTIServer::setDBInProgress(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mDBInProgress = isIt;
}

uint64_t CDTIServer::getDBTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mDTISessionsBufferTimestamp;
}

void CDTIServer::setDBTimestamp(uint64_t time)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mDTISessionsBufferTimestamp = time;
}

void CDTIServer::addWallLine(std::string text, std::string whoSays)
{
	std::lock_guard<std::mutex> lock(mRecentWallLinesGuardian);

	std::string prefix = "";

	if (mRecentWallLines.size() > 120)
		mRecentWallLines.pop_front();

	if (whoSays.size())
	{
		mTools->colorizeString(whoSays, eColor::lightCyan);
	}


	if (whoSays.size() > 0)
		prefix += ("[" + whoSays + "]: ");


	mRecentWallLines.emplace_back(prefix + text);

}

std::vector<std::string> CDTIServer::getRecentWallLines()
{
	std::lock_guard<std::mutex> lock(mRecentWallLinesGuardian);
	return std::vector<std::string>(mRecentWallLines.begin(), mRecentWallLines.end());
}

/// <summary>
/// Retrieves the DBUpdateInterval (in seconds)
/// </summary>
/// <returns></returns>
uint64_t CDTIServer::getDBUpdateInterval()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mDBUpdateInterval;
}

/// <summary>
/// Sets the DBUpdateInterval (in seconds)
/// </summary>
/// <returns></returns>
void CDTIServer::setDBUpdateInterval(uint64_t interval)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mDBUpdateInterval = interval;
}

uint64_t CDTIServer::getSessionsCount()
{
	std::lock_guard<std::mutex> lock(mDoubleSessionsBufferGuardian);
	return  mDTIConnectionsDB.size();
}

/// <summary>
/// Return locally registered Remote Terminal Sessions as per the provided parameters.
/// Sessions returned come from the second, low priority buffer.
/// </summary>
/// <param name="onlyActive"></param>
/// <param name="inView"></param>
/// <returns></returns>

std::vector<std::shared_ptr<CDTI>> CDTIServer::getSessions(eViewState::eViewState inView, bool onlyActive)
{
	std::lock_guard<std::mutex> lock(mDoubleSessionsBufferGuardian);

	std::vector<std::shared_ptr<CDTI>> toRet;

	for (uint64_t i = 0; i < mDTIConnectionsDB.size(); i++)
	{
		//Requirements Check - BEGIN
		if ((inView != eViewState::unspecified && mDTIConnectionsDB[i]->getCurrentView() != inView)
			|| onlyActive && mDTIConnectionsDB[i]->getStatus() != eDTISessionStatus::alive)
			continue;
		//Requirements Check - END

		toRet.push_back(mDTIConnectionsDB[i]);
	}

	return toRet;
}

CDTIServer::~CDTIServer()
{
	destroySSH();
}

void CDTIServer::destroySSH()//the purpose is to signal possibly active ssh socket and kill any active connections.
{

	std::lock_guard<std::mutex> lock0(mSSHBindGuardian);
	std::lock_guard<std::recursive_mutex> lock(mSessionPoolGuardian);
	doCleaningUp(true);//force kill all DTI connections
	//free libssh internal resources

	if (mSSHBind != nullptr)
	{//ssh_
		ssh_bind_free(mSSHBind);
		ssh_finalize();
		mSSHBind = nullptr;
	}

}

std::vector<uint8_t> CDTIServer::registerQRIntentResponse(std::shared_ptr<CQRIntentResponse> response)
{
	std::vector<uint8_t> receiptID;
	if (response == nullptr)
		return receiptID;

	std::shared_ptr<CDTI> terminal = getDTIbyID(response->getDestinationID());
	std::shared_ptr<CConversation> conversation;
	std::shared_ptr<CWebSocketsServer> webSockServer = mNetworkManager->getWebSocketsServer();
	if (terminal != nullptr)
	{
		if (terminal->getStatus() != eDTISessionStatus::alive)
			return receiptID;

		if (terminal == nullptr)
			return receiptID;

		return	terminal->registerQRIntentResponse(response);
	}
	else if (webSockServer != nullptr)
	{
		conversation = webSockServer->getConversationByID(response->getDestinationID());
		if (conversation != nullptr)
		{
			return conversation->registerQRIntentResponse(response);
		}
		//search within Web-Socket conversations instead
	}

	return std::vector<uint8_t>();
}

/// <summary>
/// Registers a vector of byte-vectors in delivered in response to a data query made on behalf of the VM.
/// The response's authenticity should be verified before (CNetMsg's) signature. Not needed if data is a signature-only itself.
/// </summary>
/// <param name="dataFields"></param>
/// <returns></returns>
std::vector<uint8_t> CDTIServer::registerVMMetaDataResponse(std::shared_ptr<CNetMsg> msg, std::vector<std::shared_ptr<CVMMetaSection>> sections)
{
	std::vector<uint8_t> receiptID;
	if (sections.size() == 0)
		return receiptID;

	std::shared_ptr<CConversation> conversation;
	std::shared_ptr<CWebSocketsServer> webSockServer = mNetworkManager->getWebSocketsServer();

	if (msg->getDestinationType() == eEndpointType::TerminalID)
	{
		std::shared_ptr<CDTI> terminal = getDTIbyID(msg->getDestination());

		if (terminal != nullptr)
		{
			if (terminal->getStatus() != eDTISessionStatus::alive)
				return receiptID;

			if (terminal == nullptr)
				return receiptID;

			return	terminal->registerVMMetaDataResponse(sections);
		}
	}
	else if (msg->getDestinationType() == eEndpointType::WebSockConversation)
	{
		conversation = webSockServer->getConversationByID(msg->getDestination());
		if (conversation != nullptr)
		{
			return conversation->registerVMMetaDataResponse(sections);
		}
		//search within Web-Socket conversations instead
	}

	return std::vector<uint8_t>();
}


std::shared_ptr<CDTI> CDTIServer::getDTIbyID(std::vector<uint8_t> id)
{
	std::lock_guard<std::recursive_mutex> lock(mSessionPoolGuardian);

	for (uint64_t i = 0; i < mDTIConnections.size(); i++)
	{
		if (mTools->compareByteVectors(id, mDTIConnections[i]->getID()))
			return mDTIConnections[i];
	}
	return nullptr;
}
/// <summary>
/// Gets the number of maximum simultonious sessions.
/// </summary>
/// <returns></returns>
size_t CDTIServer::getMaxSessionsCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mMaxSessions;
}

/// <summary>
/// Retrieves client's IP address based on SSH session.
/// </summary>
/// <param name="session"></param>
/// <returns></returns>
std::string CDTIServer::getSSHClientIP(ssh_session session) {

	struct sockaddr_storage tmp;
	struct sockaddr_in* sock;
	int len = 100;
	char ip[100] = "\0";
	getpeername(ssh_get_fd(session), (struct sockaddr*)&tmp, &len);
	sock = (struct sockaddr_in*)&tmp;
	inet_ntop(AF_INET, &sock->sin_addr, ip, len);
	std::string ip_str = ip;

	return ip_str;
}

std::string CDTIServer::getSSHClientIP(ssh_bind bind) {

	struct sockaddr_storage tmp;
	struct sockaddr_in* sock;
	int len = 100;
	char ip[100] = "\0";
	getpeername(ssh_bind_get_fd(bind), (struct sockaddr*)&tmp, &len);
	sock = (struct sockaddr_in*)&tmp;
	inet_ntop(AF_INET, &sock->sin_addr, ip, len);
	std::string ip_str = ip;

	return ip_str;
}

std::string CDTIServer::getSSHClientIP(socket_t socket) {

	struct sockaddr_storage tmp;
	struct sockaddr_in* sock;
	int len = 100;
	char ip[100] = "\0";
	getpeername(socket, (struct sockaddr*)&tmp, &len);
	sock = (struct sockaddr_in*)&tmp;
	inet_ntop(AF_INET, &sock->sin_addr, ip, len);
	std::string ip_str = ip;

	return ip_str;
}


/// <summary>
/// Sets the number of max simultonious sessions.
/// </summary>
/// <param name="nr"></param>
void CDTIServer::setMaxSessionsCount(size_t nr)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mMaxSessions = nr;
}
size_t CDTIServer::getLastTimeCleanedUp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mLastTimeCleaned;
}
void  CDTIServer::setLastTimeCleanedUp(size_t timestamp)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastTimeCleaned = timestamp != 0 ? (timestamp) : (mTools->getTime());
}


uint64_t CDTIServer::getLastControllerLoopRun()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastControllerLoopRun;
}

void CDTIServer::pingtLastControllerLoopRun()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastControllerLoopRun = std::time(0);
}

/// <summary>
/// Constructs the DTI server.
/// </summary>
CDTIServer::CDTIServer(std::shared_ptr<CNetworkManager> nm)
{
	mStatusChange = eManagerStatus::initial;
	mLastControllerLoopRun = 0;
	mDTISessionsBufferTimestamp = 0;
	mNetworkManager = nm;
	mDBInProgress = false;
	mStatus = eManagerStatus::eManagerStatus::initial;
	mShutdown = false;
	mLastTimeCleaned = 0;
	mTools = nm->getTools();
	mMaxSessionInactivity = 180;//60 sec inactivity allowed.
	mMaxWarningsCount = 5;
	mMaxSessions = 3000; //allow for max of 3000 concurent DTI session per full-node
	//Note: initialization done outside
	mDBUpdateInterval = 5;//update the sessions double buffer at most every 5 seconds
}

/// <summary>
/// Initialize the DTI-Server.
/// Note: this CANNOT be done within a constructor. The initialization MIGHT NOT succeed.
/// </summary>
/// <returns></returns>
bool CDTIServer::initialize()
{
	return initializeServer();
}

/// <summary>
/// Stop the DTI server.
/// </summary>
void CDTIServer::stop()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (getStatus() == eManagerStatus::eManagerStatus::stopped)
		return;
	//firs ensure no new session comes-in
	mStatusChange = eManagerStatus::eManagerStatus::stopped;
	if (mControllerThread.get_id() != std::this_thread::get_id())
		if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::stopped)//controller is dead; we need first to thwart it for to enable for a state-transmission.
			mControllerThread = std::thread(&CDTIServer::mControllerThreadF, this);

	if (mControllerThread.get_id() != std::this_thread::get_id())
	{
		while (getStatus() != eManagerStatus::eManagerStatus::stopped && getStatus() != eManagerStatus::eManagerStatus::initial)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		if (mControllerThread.joinable())
			mControllerThread.join();
	}

	doCleaningUp(true);//kill all active sessions
	destroySSH();
	mTools->writeLine("DTI-Sessions Manager killed;");
}
/// <summary>
/// Pause the DTI server
/// </summary>
void CDTIServer::pause()
{
	if (getStatus() == eManagerStatus::eManagerStatus::paused)
		return;

	//enesure nonone comes in
	mStatusChange = eManagerStatus::eManagerStatus::paused;

	if (mControllerThread.get_id() != std::this_thread::get_id())
	{
		if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::paused)//controller is dead; we need first to thwart it for to enable for a state-transmission.
			mControllerThread = std::thread(&CDTIServer::mControllerThreadF, this);

		while (getStatus() != eManagerStatus::eManagerStatus::paused && getStatus() != eManagerStatus::eManagerStatus::initial)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	doCleaningUp(true);//disocnnect all active

	//halt all the intrisic sub-systems - END
}
/// <summary>
/// Resume the DTI Server
/// </summary>
void CDTIServer::resume()
{
	if (getStatus() == eManagerStatus::eManagerStatus::running)
		return;
	mStatusChange = eManagerStatus::eManagerStatus::running;

	if (mControllerThread.get_id() != std::this_thread::get_id())
		if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::running)//controller is dead; we need first to thwart it for to enable for a state-transmission.
			mControllerThread = std::thread(&CDTIServer::mControllerThreadF, this);


	while (getStatus() != eManagerStatus::eManagerStatus::running && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

/// <summary>
/// Initialize the controller.
/// The controller thread takes care of managing the Server's state and reacts to outside inqueries.(pause/resume etc).
/// </summary>
void CDTIServer::mControllerThreadF()
{
	std::string tName = "DTIServer Controller";
	mTools->SetThreadName(tName.data());
	setStatus(eManagerStatus::eManagerStatus::running);
	bool wasPaused = false;
	if (mStatus != eManagerStatus::eManagerStatus::running)
		setStatus(eManagerStatus::eManagerStatus::running);
	uint64_t justCommitedFromHeaviestChainProof = 0;

	mDTIServerThread = std::thread(&CDTIServer::DTIServerThreadF, this);

	while (mStatus != eManagerStatus::eManagerStatus::stopped)
	{
		pingtLastControllerLoopRun();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		wasPaused = false;
		doCleaningUp();
		updateSessionsDB();//double buffering of sessions
		if (mStatusChange == eManagerStatus::eManagerStatus::paused)
		{
			setStatus(eManagerStatus::eManagerStatus::paused);
			mStatusChange = eManagerStatus::eManagerStatus::initial;

			while (mStatusChange == eManagerStatus::eManagerStatus::initial)
			{

				if (!wasPaused)
				{
					mTools->writeLine("My thread operations were freezed. Halting..");
					if (mDTIServerThread.native_handle() != 0)
					{

						while (!mDTIServerThread.joinable())
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
						mDTIServerThread.join();
					}

					wasPaused = true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		if (wasPaused)
		{
			mTools->writeLine("My thread operations are now resumed. Commencing further..");
			mDTIServerThread = std::thread(&CDTIServer::DTIServerThreadF, this);
			mStatus = eManagerStatus::eManagerStatus::running;
		}


		if (mStatusChange == eManagerStatus::eManagerStatus::stopped)
		{
			mStatusChange = eManagerStatus::eManagerStatus::initial;
			mStatus = eManagerStatus::eManagerStatus::stopped;

		}

	}
	doCleaningUp(true);//kill all the connections; do not allow for Zombie-connections
	setStatus(eManagerStatus::eManagerStatus::stopped);
}

eManagerStatus::eManagerStatus CDTIServer::getStatus()
{
	std::lock_guard<std::recursive_mutex> lock(mStatusGuardian);
	return mStatus;
}

void CDTIServer::setStatus(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::recursive_mutex> lock(mStatusGuardian);
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
	case eManagerStatus::eManagerStatus::stopped:
		mTools->writeLine("I'm now stopped");
		break;
	default:
		mTools->writeLine("I'm now in an unknown state;/");
		break;
	}
}

void CDTIServer::requestStatusChange(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	mStatusChange = status;
}

eManagerStatus::eManagerStatus CDTIServer::getRequestedStatusChange()
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	return mStatusChange;
}




