
#include "NetworkManager.h"
#include <vector>
#ifndef WIN32
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#endif

#include "UDTConversationsServer.h"

#include "CGlobalSecSettings.h"
#include "BlockchainManager.h"
#include "conversation.h"
#include "conversationState.h"
#include "GRIDNET.h"
#include "udt/src/udt.h"
#include "ThreadPool.h"

/// <summary>
/// Accepts incoming UDT connections. Auto-firewall functionality included.
/// </summary>
void CUDTConversationsServer::UDTConversationServerThreadF()
{

	//Local Variables - BEGIN
	UDTSOCKET clientSocket;
	sockaddr_storage clientaddr;
	int addrlen = sizeof(clientaddr);
	char clienthost[NI_MAXHOST];
	char clientservice[NI_MAXSERV];
	std::shared_ptr<CNetworkManager> nm = getNetworkManager();
	uint64_t alreadyConnected = 0;
	std::string ip;
	std::shared_ptr<CTools> tools = getTools();
	//uint64_t lastCleaning = 0;
	mTools->SetThreadName("UDT Conversation Server");

	//accept an incoming connection
	uint64_t now = std::time(0);
	bool DOSattack = false;
	bool canLogThisIP = true;
	uint64_t lastDOSNotification = 0;
	//Local Variables - END


	CTools::getInstance()->setThreadPriority(ThreadPriority::HIGHEST);

	//Operational Logic - BEGIN
	while (getStatus() == eManagerStatus::eManagerStatus::running)
	{
		//Refresh Variables - BEGIN
		now = std::time(0);
		DOSattack = false;
		//Refresh Variables - END
		
		//WARNING: *DO NOT* perform ANY additional logic besides accepting incoming connections over here.
		//		   use mControllerThread instead.
		//START to accept UDT connections
		clientSocket = UDT::accept(mUDTServerSocket, (sockaddr*)&clientaddr, &addrlen);
		if (6002== clientSocket || UDT::INVALID_SOCK == clientSocket)
		{
			continue;
			/*
			int errorCode = UDT::getlasterror().getErrorCode();
			// Check if the error is simply because there's no connection to accept.
			if (errorCode == CUDTException::EASYNCRCV)
			{
				// No connection to accept in async mode, this is not necessarily an error.
				continue;
			}
			else
			{
				tools->logEvent("UDT accept error: " + std::string(UDT::getlasterror().getErrorMessage()), eLogEntryCategory::network, 3, eLogEntryType::failure);
				continue;
			}*/
		}


		getnameinfo((sockaddr*)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST | NI_NUMERICSERV);

		canLogThisIP = nm->canEventLogAboutIP(clienthost);
		if (canLogThisIP)
		{
			tools->logEvent("New UDT connection: " + std::string(clienthost) + ":" + std::string(clientservice), eLogEntryCategory::network, 0);
		}
		//check if node is on the list of known nodes; if not => add it.

		ip = clienthost;
		alreadyConnected = 0;
		bool abortConnection = false;
		//[Security] (Part 1) - BEGIN
		/*
		* Are we even to instantiate a Conversation with the subject?
		* Rationale: that would be resource intensive. Supposedly lots of RAM required plus for maintenance overhead, threads etc.
		*/

		eIncomingConnectionResult::eIncomingConnectionResult result = nm->isConnectionAllowed(ip, true, eTransportType::UDT, false);

		if (result != eIncomingConnectionResult::allowed)
			abortConnection = true;
		//[Security] (Part 2) - BEGIN
		/*
		* If a decision to Abort was made - do *NOT* even enter the critical section below.
		*/
		if (!abortConnection)
		{
			//allow up to 5 conversations per IP address - BEGIN
			{
				std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
				std::vector<uint8_t> ipV = tools->stringToBytes(ip);

				for (uint64_t i = 0; i < mConversations.size(); i++)
				{
					if (tools->compareByteVectors(mConversations[i]->getEndpoint()->getAddress(), ipV) && mConversations[i]->getState()->getCurrentState() == eConversationState::running)
					{
						alreadyConnected++;
					}
				}

				if (alreadyConnected > CGlobalSecSettings::getMaxUDTConnectionsPerIP())
				{
					abortConnection = true;
					if (canLogThisIP)
					{
						tools->logEvent(tools->getColoredString("[Security]:", eColor::blue) + " Won't allow for a new UDT conversation from " + std::string(clienthost) + ":" + std::string(clientservice) + " - " + std::to_string(CGlobalSecSettings::getMaxUDTConnectionsPerIP()) + " conn. per IP limit  reached.", eLogEntryCategory::network, 1);
					
						// Aggressive Peer / Sub-net - BEGIN
						if (nm->getConnectionAttemptsFrom(ip, eTransportType::UDT, 180) > CGlobalSecSettings::getAggressiveAlreadyConnectedThreshold())
						{
							nm->banIP(ip, 60 * 45);
						}
					    // Aggressive Peer / Sub-net - END
					
					}
				}
				//allow only one UDT conversation per IP address  - END
			}
		}
		//[Security] (Part 2) - END

		//Execution - BEGIN
		if (!abortConnection)
		{
			std::shared_ptr<CEndPoint> ep = std::make_shared<CEndPoint>(tools->stringToBytes(ip), eEndpointType::IPv4, nullptr, CGlobalSecSettings::getDefaultPortNrDataExchange(mBlockchainMode));
			std::shared_ptr< CConversation> conversation = std::make_shared<CConversation>(clientSocket, CBlockchainManager::getInstance(mBlockchainMode), mNetworkManager, ep);
			conversationFlags flags = conversation->getFlags();

			flags.isIncoming = true;

			conversation->setFlags(flags);
			conversation->initState(conversation);

			if (addConversation(conversation))
			{
				//initialize a conversation only if added to the underlying container.
				
				//initiate and add an UDT conversation
				conversation->setIsAuthenticationRequired(false);
				conversation->setIsEncryptionRequired(true);
				conversation->startUDTConversation(getThreadPool(), clientSocket, clienthost);
			}
			else
			{
				if (canLogThisIP)
				{
					tools->logEvent(tools->getColoredString("[Security]:", eColor::blue) + " Won't allow for a new UDT conversation. Global limits reached..", eLogEntryCategory::network, 10, eLogEntryType::failure);
				}
			}
			
		}
		else
		{
			if (nm->canEventLogAboutIP(clienthost))
			{
				if (result == eIncomingConnectionResult::DOS)
				{
					tools->logEvent("mitigating DOS attack at the UDT layer from " + std::string(clienthost), "Security", eLogEntryCategory::network,
						10, eLogEntryType::notification, eColor::cyborgBlood, true);
				}
				else
					if (result == eIncomingConnectionResult::limitReached)
					{
						tools->logEvent("Won't accept connection at the UDT layer from " + std::string(clienthost) + " Limits reached.", "Security", eLogEntryCategory::network,
							10, eLogEntryType::notification, eColor::cyborgBlood, true);
					}

					else
						if (result == eIncomingConnectionResult::onlyBootstrapNodesAllowed)
						{
							tools->logEvent("Won't accept connection at the UDT layer from " + std::string(clienthost) + " Now allowing only Bootstrap nodes.", "Security", eLogEntryCategory::network,
								10, eLogEntryType::notification, eColor::cyborgBlood, true);
						}
						else
							if (result == eIncomingConnectionResult::insufficientResources)
							{
								tools->logEvent("Won't accept connection at the UDT layer from " + std::string(clienthost) + " Insufficient Resources..", "Security", eLogEntryCategory::network,
									10, eLogEntryType::notification, eColor::cyborgBlood, true);
							}
			}
			UDT::close(clientSocket);
			incUDTSocketsClosedCount();
			if (DOSattack && (now - lastDOSNotification) > 5)
			{
				lastDOSNotification = now;
				if (canLogThisIP)
				{
					tools->logEvent("mitigating DOS attack at the UDT layer from " + ip, "Security", eLogEntryCategory::network,
						10, eLogEntryType::notification, eColor::cyborgBlood, true);
				}
			}
		}
		//Execution - END

		Sleep(75);
	}
	//Operational Logic - END
}
bool CUDTConversationsServer::bootUpUDTServerSocket()
{

		//Local Variables - BEGIN
		std::shared_ptr<CTools> tools = getNetworkManager()->getTools();
		addrinfo hints;
		addrinfo* res;
		std::once_flag flag;
		memset(&hints, 0, sizeof(struct addrinfo));

		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;//instead of SOCK_STREAM -> we get msg boundaries for free. not really needed as all MSGs are BER encoded already.
									   //hints.ai_socktype = SOCK_DGRAM;
		

	//on windows there's no  SO_REUSEPORT so we're stuck with SO_REUSEADDRESS.
	//With SO_REUSEADDRESS there can't be same 5x tuple, so we need to bind 
	//Kademlia socket to 0.0.0.0 and UDT to concrete IP address instead.
		std::shared_ptr<CNetworkManager> nm = getNetworkManager();
		std::string publicIP = nm->getPublicIP();
		//Public IP - IP of a NAT, if present - the Eth0 virtual interface
		//Local IP - IP of the local computer - the Eth1 virtual interface
	
		std::vector<std::string> localIPs = nm->getLocalIPAdresses();//get the main local IPv4
		std::string IPv4ToUse = nm->getLocalIP();//pick the one assigned by Operator to Eth1
		std::stringstream ss;

		//Local Variables - END

		//Operational Logic - BEGIN
		try {
		if (tools->doStringsMatch(IPv4ToUse, "0.0.0.0"))
		{
			ss << "UDT sub-system needs to use a concrete IPv4 address, not a wild-card address.";
			ss << tools->getColoredString(" OVERRIDING ", eColor::lightPink);
			ss << " IPv4 address assigned to Eth1 (" + tools->getColoredString(IPv4ToUse, eColor::lightCyan) + ") with ";
			ss << tools->getColoredString(localIPs[0], eColor::lightCyan) <<  ".";

			tools->logEvent(ss.str(), "Network Configuration", eLogEntryCategory::localSystem, 10);
			IPv4ToUse = localIPs[0];
		}
		bool behindANAT = true;
		//first check if we're behind a NAT
		for (uint64_t i = 0; i < localIPs.size(); i++)
		{
			if (tools->doStringsMatch(localIPs[i], publicIP))
			{
				behindANAT = false;//prefer the public interface if available directly, of course.
				IPv4ToUse = localIPs[i];
				break;
			}
		}
		if (behindANAT)
		{
			tools->logEvent(std::string("Looks like"+tools->getColoredString(" you are behind a NAT ", eColor::lightPink) +"("+ tools->getColoredString(publicIP, eColor::lightCyan) + ") ! Make sure to route the appropriate ports to your local Virtual Interface Eth1 (" + tools->getColoredString(IPv4ToUse, eColor::lightCyan) + ")."), "Networking (UDT)", eLogEntryCategory::localSystem, 10);
		}
		else
		{
			tools->logEvent(std::string("Looks like" + tools->getColoredString(" you are not behind a NAT.", eColor::lightGreen)+" The public Virtual Interface Eth0 (" + tools->getColoredString(IPv4ToUse, eColor::lightCyan) + ") is to be assigned to the UDT subsystem."), "Networking (UDT)", eLogEntryCategory::localSystem, 10);
		}

		
		std::string portStr = std::to_string(CGlobalSecSettings::getDefaultPortNrDataExchange(mBlockchainMode));

		tools->logEvent("Bootstrapping UDT Service at " +tools->getColoredString(IPv4ToUse + ":" + portStr, eColor::lightCyan),"Networking (UDT)", eLogEntryCategory::localSystem,10);

		Sleep(2000);

		if (0 != getaddrinfo(IPv4ToUse.c_str(), portStr.c_str(), &hints, &res))
		{
			tools->logEvent("Error: Seems like GRIDNET Core Server is already running. I'll quit.", "Networking (UDT)", eLogEntryCategory::localSystem, 10);
			Sleep(2000);
			return false;
		}

		struct AddrInfoGuard {
			addrinfo* ptr;
			~AddrInfoGuard() {
				if (ptr) freeaddrinfo(ptr);
			}
		} guard{ res };  // This will free res when it goes out of scope
		
		mUDTServerSocket = UDT::socket(AF_INET, res->ai_socktype, 0);
		
		UDT::setUnsolicitedDatagramReceivedCallback( reinterpret_cast<UDT::unsolicitedDataCallback>(&unsolicitedDatagramCallback), this);
		
		if (nm->getIsNetworkTestMode())
		{//only in Analysis Mode; due to the performance hit.
			UDT::setValidDatagramReceivedCallback(reinterpret_cast<UDT::validDataCallback>(&validDatagramCallback), this);
		}

		int timeout = 7000;// static_cast<int>(CGlobalSecSettings::getMaxServerSocketTimeout());
		bool reuseAddr = true;
		bool block = false;
		std::string errorMsg;
		int optRes = 0;

		optRes = UDT::setsockopt(mUDTServerSocket, 0, UDT_RCVTIMEO, &timeout, sizeof(timeout));
		if ((errorMsg = CConversation::checkUDTError(optRes)) != "")
			assertGN(false);
		int maxUDTSendTimeout = 180000;
		optRes = UDT::setsockopt(mUDTServerSocket, 0, UDT_SNDTIMEO, &maxUDTSendTimeout, sizeof(maxUDTSendTimeout));
		if ((errorMsg = CConversation::checkUDTError(optRes)) != "")
			assertGN(false);

		optRes = UDT::setsockopt(mUDTServerSocket, 0, UDT_REUSEADDR, &reuseAddr, sizeof(reuseAddr));
		if ((errorMsg = CConversation::checkUDTError(optRes)) != "") 
			assertGN(false);

		optRes = UDT::setsockopt(mUDTServerSocket, 0, UDT_RCVSYN, &block, sizeof(block));
		if ((errorMsg = CConversation::checkUDTError(optRes)) != "")
			assertGN(false);

		optRes = UDT::setsockopt(mUDTServerSocket, 0, UDT_SNDSYN, &block, sizeof(block));
		if ((errorMsg = CConversation::checkUDTError(optRes)) != "")
			assertGN(false);

		// Prepare Buffer Properties - BEGIN
		
		//	IMPORTANT: these options NEED to be set BEFORE the socket is BOUND (connected / bound).
		//	These settings would be inherited down onto client sockets.
		//  Note: RX buffer size is STATIC. The TX buffer size is allocated dynamically on demand.

		int maxRecvBuffer = CGlobalSecSettings::getMaxUDTNetworkPackageSize() +10000;//UDT uses some required padding.
		int maxSendBuffer = 2 * CGlobalSecSettings::getMaxUDTNetworkPackageSize();

		optRes = UDT::setsockopt(mUDTServerSocket, 0, UDT_RCVBUF, &maxRecvBuffer, sizeof(maxRecvBuffer));
		if ((errorMsg = CConversation::checkUDTError(optRes)) != "")
			assertGN(false);

		optRes = UDT::setsockopt(mUDTServerSocket, 0, UDT_SNDBUF, &maxSendBuffer, sizeof(maxSendBuffer));
		if ((errorMsg = CConversation::checkUDTError(optRes)) != "")
			assertGN(false);

		// Prepare Buffer Properties - END

		if (UDT::ERROR == UDT::bind(mUDTServerSocket, res->ai_addr, res->ai_addrlen))
		{
			tools->logEvent("Error: Couldn't bind the UDT socket.", "Networking (UDT)", eLogEntryCategory::localSystem, 10);
			Sleep(4000);
			return false;
		}

		tools->writeLine("GRIDNET Core DataExchange sub-system " + tools->getColoredString("open at port: " + std::to_string(CGlobalSecSettings::getDefaultPortNrDataExchange(mBlockchainMode, false)), eColor::lightCyan));
		
	

		if (UDT::ERROR == UDT::listen(mUDTServerSocket, 100))
		{
			tools->logEvent("Error: Couldn't initiate listening on UDT socket.", "Networking (UDT)", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
			Sleep(4000);
			return false;
		}
		return true;
	}
	 catch (const std::exception& e) {
		 tools->logEvent("Error during UDT bootstrapping: " + std::string(e.what()), "Networking (UDT)", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
		 Sleep(4000);
		 return false;
	 }
	 catch (...) {
		 tools->logEvent("Unknown error during UDT bootstrapping.", "Networking (UDT)", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
		 Sleep(4000);
		 return false;
	 }
	//Operational Logic - END
}
std::shared_ptr<CNetworkManager> CUDTConversationsServer::getNetworkManager()
{
	std::lock_guard<std::mutex> lock(mNetworkManagerGuardian);
	return mNetworkManager;
}
bool CUDTConversationsServer::addConversation(std::shared_ptr<CConversation> convesation)
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

	if (mConversations.size() > CGlobalSecSettings::getMaxSimultaniousUDTConversationCount())
		return false;

	mConversations.push_back(convesation);

	return true;
}

/// <summary>
/// Kills the server-socket.
/// </summary>
void CUDTConversationsServer::killServerSocket()
{
	mTools->writeLine("Killing the UDT server socket..");
	UDT::close(mUDTServerSocket);
}

bool CUDTConversationsServer::initializeServer()
{
	
	UDT::startup();
	bool retflag;
	if (!bootUpUDTServerSocket())
	{
		mTools->writeLine("ERROR: I was unable to start the UDT Server socket.");
		return false;
	}
	//let us not forget about an overwatch, shall we..
	mControllerThread= std::thread(&CUDTConversationsServer::mControllerThreadF, this);
	return true;
}
std::shared_ptr<ThreadPool> CUDTConversationsServer::getThreadPool()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mThreadPool;
}

uint64_t CUDTConversationsServer::doCleaningUp(bool forceKillAllSessions)
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
	setLastTimeCleanedUp(mTools->getTime());

	if (forceKillAllSessions)
	{
		for (int i = 0; i < mConversations.size(); i++)
		{
			mConversations[i]->end();
		}
		setLastTimeCleanedUp(mTools->getTime());
		return mConversations.size();
	}
	else
	{
		setLastTimeCleanedUp(mTools->getTime());
		return cleanConversations();
	
	}
}

uint64_t CUDTConversationsServer::getLastAliveListing()
{
	std::lock_guard<std::mutex> lock(mLastAliveListingGuardian);
	return mLastAliveListingTimestamp;
}

std::shared_ptr<CTools> CUDTConversationsServer::getTools()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mTools;
}

std::string CUDTConversationsServer::getLastConnectionsReport()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastConnectionsReport;
}

void CUDTConversationsServer::setLastConnectionsReport(const std::string& report)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastConnectionsReport = report;
}
/// <summary>
/// Does cleaning up of UDT Conversations. 
/// Returns the number of freed communication slots.
/// </summary>
/// <returns></returns>
uint64_t CUDTConversationsServer::cleanConversations()
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
	
	//Local Variables - BEGIN
	size_t time = mTools->getTime();
	uint64_t removedCount = 0;
	uint64_t eliminatedCount = 0;
	std::string report = tools->getColoredString(std::string("\n-- Registered UDT Conversations -- \n"), eColor::blue);
	uint64_t lastConvActivity = 0;
	uint64_t timeConvStarted = 0;
	std::string inConn = tools->getColoredString("IN", eColor::orange);
	std::string outConn = tools->getColoredString("OUT", eColor::blue);
	//Local Variables - END
	std::shared_ptr<CNetworkManager> nm = getNetworkManager();

	

	report += mConversations.size() > 0 ? "\n" : tools->getColoredString("none", eColor::cyberWine);
	bool genReport = (tools->getTime() - getLastAliveListing()) > 15 ? true : false;
	if (nm)
	{
		/*UPDATE: the duplicates' elimination logic was moved to CNetworkManager since we need to cross compare across both UDT and QUIC.
		eliminatedCount =nm->eliminateDuplicates(mConversations);
		if (eliminatedCount)
		{
			tools->logEvent("Ending " + std::to_string(eliminatedCount) + " duplicate UDT connections..", eLogEntryCategory::network, 1,eLogEntryType::notification,eColor::none);
		}
		*/
	}
	std::string timeStr;
	bool canLogThisIP = true;
	for (std::vector <std::shared_ptr<CConversation>>::iterator it = mConversations.begin(); it != mConversations.end(); ) {

		canLogThisIP = nm->canEventLogAboutIP((*it)->getIPAddress());
		if (genReport)
		{
			timeStr = "Started: " + tools->timeToString((*it)->getState()->getTimeStarted()) + " Duration: " + tools->secondsToFormattedString((*it)->getState()->getDuration());
			report += (((*it)->getFlags().isIncoming ? inConn : outConn));
			report += " ";
			report += tools->base58CheckEncode((*it)->getID()) + "@" + (*it)->getIPAddress() + ":" + tools->conversationStateToString((*it)->getState()->getCurrentState()) + ((*it)->getIsEncryptionAvailable() ? tools->getColoredString(u8" Sec", eColor::orange) : "") + ((*it)->getIsSyncEnabled() ? tools->getColoredString(u8" Sync", eColor::lightCyan) : "") + ((*it)->getFlags().isMobileApp ? tools->getColoredString(u8" (mobile)", eColor::blue) : "") + " " + timeStr + "\n";
		}
		lastConvActivity = (*it)->getState()->getLastActivity();

		time = tools->getTime();
		if ((*it)->getState()->getLastActivity() > time)
		{//it's in the future
			++it;
			continue;
		}

	
		
		// Termination Conditions - BEGIN
		// [ WARNING ]: the EXECUTION section - it NEEDS to be reached. While the conditions MIGHT be mutually exclusive.
			//Security - BEGIN
		
		if (((*it)->getState()->getCurrentState() != eConversationState::ended))
		{


			if ((*it)->getWarningsCount() > CGlobalSecSettings::getMaxConversationWarningsAllowed())
			{
				if (canLogThisIP)
				{
					getTools()->logEvent((*it)->getIPAddress() + " exceeded warnings' count. Terminating.. ", "Security", eLogEntryCategory::network, 3, eLogEntryType::warning);
				}
				(*it)->end(false);
			}
			else  if (((*it)->getIsEncryptionAvailable() == false && (*it)->getIsEncryptionRequired()) &&
				(time > (*it)->getDuration() > 20))
			{
				tools->logEvent("[FORCED Shutdown] UDT Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to a secure session timeout.", eLogEntryCategory::network, 1);
				(*it)->end(false);
			}
			else  if (((*it)->getIsSyncEnabled() == false && (*it)->getSyncToBeActive()) &&
				(time > (*it)->getDuration() > 30))// each conversation decides on its own by seeing if dealing with a mobile device
			{
				tools->logEvent("[FORCED Shutdown] UDT Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to Sync protocol initialisation timeout.", eLogEntryCategory::network, 1);
				(*it)->end(false);
			}
			//Security -END
			else  if ((*it)->getState()->getCurrentState() == eConversationState::initializing &&
				(time > lastConvActivity) && ((time - lastConvActivity) > CGlobalSecSettings::getUDTConvInitTimeout()))
			{
				if (canLogThisIP)
				{
					tools->logEvent("[FORCED Shutdown] UDT Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to a timed-out initialization.", eLogEntryCategory::network, 1);
				}
				(*it)->end(false);
			}
			else if ((*it)->getState()->getCurrentState() != eConversationState::ended &&
				(*it)->getState()->getCurrentState() != eConversationState::running &&
				(time > lastConvActivity) && ((time - lastConvActivity) > CGlobalSecSettings::getOverridedConvShutdownAfter()))
			{
				if (canLogThisIP)
				{
					tools->logEvent("[FORCED Shutdown] UDT Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to a timed-out idle state.", eLogEntryCategory::network, 1);
				}
				(*it)->end(false);
			}
			else if ((*it)->getState()->getCurrentState() != eConversationState::ended && (time > lastConvActivity) && ((time - lastConvActivity) > CGlobalSecSettings::getOverridedConvShutdownAfter()))
			{
				if (canLogThisIP)
				{
					tools->logEvent("[FORCED Shutdown] UDT Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to inactivity.", eLogEntryCategory::network, 1);
				}
				(*it)->end(false);
			}

			//Security - BEGIN
			// Anti-excessive connectivity suffocation mechanics. [Part 1] - connection local. 
			// Notice: [Part 2] keeps track of global connection times based on IP addresses.
			// These mechanics thwart overly long connections with same peers.
			// Max Connection Duration  - BEGIN
			uint64_t defferedTimeoutSince = (*it)->getDeferredTimeoutRequestedAt();
			bool isDeferred = (defferedTimeoutSince && (time > defferedTimeoutSince) && ((time - defferedTimeoutSince) < 60 * 10));
			
			if ((*it)->getState()->getCurrentState() == eConversationState::running)
			{
				if ((*it)->getDuration() >= (CGlobalSecSettings::getMaxBootstrapConnDuration() * (isDeferred ? 10 : 5)))
				{
					if (canLogThisIP)
					{
						tools->logEvent(tools->getColoredString("[Security]:", eColor::blue) + " Max connection time with " + (*it)->getIPAddress() + " reached. Killing it.", eLogEntryCategory::network, 2);
					}
					((*it))->end(false);
				}
			}
			// Bootstrap Node - END
			
			//Security - BEGIN
			//Termination Conditions - END
		}

		// Execution - BEGIN
		
		if (((*it)->getState()->getCurrentState() == eConversationState::ended && ((time > (*it)->getState()->getTimeEnded() && (time-(*it)->getState()->getTimeEnded()) > CGlobalSecSettings::getFreeConversationAfter()))
			|| (*it)->getState()->getCurrentState() == eConversationState::unableToConnect) && (time > lastConvActivity) && ((time - lastConvActivity) > (std::max((*it)->getUDTTimeout(), CGlobalSecSettings::getFreeConversationAfter()))))
		{
			//std::lock_guard<std::recursive_mutex> lock((*it)->mDestructionGuardian);
			if ((*it)->getIsScheduledToStart())
			{
				++it;
				continue;
			}

			if ((*it)->isSocketFreed() == false)
			{
				++it;
				//tools->logEvent("Waiting for conversation" + tools->base58CheckEncode((*it)->getID()) + " to free the underlying socket).", eLogEntryCategory::network, 0);
				continue;
			}
			if ((*it)->getIsThreadAlive())
			{
				++it;
				//tools->logEvent("Waiting for conversation" + tools->base58CheckEncode((*it)->getID()) + " to free the underlying socket).", eLogEntryCategory::network, 0);
				continue;
			}

			tools->logEvent("[Freeing] UDT Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ").", eLogEntryCategory::network, 0);
			
			it = mConversations.erase(it);
			removedCount++;
		}
		else {//let it be
			++it;
		}

		// Execution - END
	}

	//Report - BEGIN
	if (genReport)
	{
		report += tools->getColoredString("\n-------\n", eColor::blue);
		setLastConnectionsReport(report);
		tools->logEvent(report, eLogEntryCategory::network, 1);
		lastAlivePrinted();
	}
	//Report - END

	return removedCount;
}

void CUDTConversationsServer::lastAlivePrinted()
{
	std::lock_guard<std::mutex> lock(mLastAliveListingGuardian);
	mLastAliveListingTimestamp = mTools->getTime();
}

size_t CUDTConversationsServer::getActiveSessionsCount()
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
	return mConversations.size();
}

CUDTConversationsServer::~CUDTConversationsServer()
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);


	if (mUDTServerThread.joinable())
		mUDTServerThread.join();


	if (mControllerThread.joinable())
		mControllerThread.join();
}

std::vector<std::shared_ptr<CConversation>> CUDTConversationsServer::getAllConversations()
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
	return mConversations;
}

std::shared_ptr<CConversation> CUDTConversationsServer::getConversationByID(std::vector<uint8_t> id)
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

	for (uint64_t i = 0; i < mConversations.size(); i++)
	{
		if (mTools->compareByteVectors(mConversations[i]->getID(),id))
			return mConversations[i];
   }
	return nullptr;
}

std::shared_ptr<CConversation> CUDTConversationsServer::getConversationByIP(std::vector<uint8_t> IP)
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
	for (uint64_t i = 0; i < mConversations.size(); i++)
	{
		if (mTools->compareByteVectors(mConversations[i]->getEndpoint()->getAddress(), IP))
			return mConversations[i];
	}
	return nullptr;
}

size_t CUDTConversationsServer::getConversationsCountByIP(std::vector<uint8_t> IP)
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
	size_t count = 0;

	for (uint64_t i = 0; i < mConversations.size(); i++)
	{
		if (mTools->compareByteVectors(mConversations[i]->getEndpoint()->getAddress(), IP))
			count++;
	}
	return count;
}

size_t CUDTConversationsServer::getMaxSessionsCount()
{
	return CGlobalSecSettings::getMaxSimultaniousUDTConversationCount();
}

void  CUDTConversationsServer::setLastTimeCleanedUp(size_t timestamp)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastTimeCleaned = timestamp != 0 ? (timestamp) : (mTools->getTime());
}

size_t CUDTConversationsServer::getLastTimeCleanedUp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mLastTimeCleaned;
}

uint64_t CUDTConversationsServer::getUDTSocketsClosedCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mUDTSocketsClosedCount;
}

void CUDTConversationsServer::incUDTSocketsClosedCount()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 ++mUDTSocketsClosedCount;
}

CUDTConversationsServer::CUDTConversationsServer(std::shared_ptr<ThreadPool> pool, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm)
{
	mUDTSocketsClosedCount = 0;
	mLastConversationCleanup = 0;
	mThreadPool = pool;
	mLastAliveListingTimestamp = 0;
	mBlockchainManager = bm;
	mBlockchainMode = bm->getMode();
	mNetworkManager = nm;
	mTools = bm->getTools();
	mShutdown = false;
	mStatusChange = eManagerStatus::initial;
	mStatus = eManagerStatus::initial;
	mMaxWarningsCount = 3;

}

bool CUDTConversationsServer::initialize()
{
	return initializeServer();
}

void CUDTConversationsServer::stop()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	//firs ensure no new session come-in
	if (getStatus() == eManagerStatus::eManagerStatus::stopped)
		return;

	mStatusChange = eManagerStatus::eManagerStatus::stopped;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::stopped)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CUDTConversationsServer::mControllerThreadF, this);

	while (getStatus() != eManagerStatus::eManagerStatus::stopped && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	if (mControllerThread.joinable())
		mControllerThread.join();

	//then clean-up data structures / free memory
	
	doCleaningUp(true);//kill all active ones

	killServerSocket();
	UDT::cleanup();
	mTools->writeLine("UDT Conversations Manager killed;");
}

void CUDTConversationsServer::pause()
{
	if (getStatus() == eManagerStatus::eManagerStatus::paused)
		return;

	mStatusChange = eManagerStatus::eManagerStatus::paused;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::paused)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CUDTConversationsServer::mControllerThreadF, this);

	while (getStatus() != eManagerStatus::eManagerStatus::paused && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	doCleaningUp(true);
	mTools->writeLine("UDT Server paused;");
}

void CUDTConversationsServer::resume()
{
	if (getStatus() == eManagerStatus::eManagerStatus::running)
		return;
	mStatusChange = eManagerStatus::eManagerStatus::running;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::running)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CUDTConversationsServer::mControllerThreadF, this);


	while (getStatus() != eManagerStatus::eManagerStatus::running && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	mTools->writeLine("UDT Server resumed;");
}

/// <summary>
/// Controls the UDT Server Manager
/// </summary>
void CUDTConversationsServer::mControllerThreadF()
{
	//Local Variables - BEGIN
	std::string tName = "UDT-Conversations Controller";
	std::shared_ptr<CTools> tools = getTools();
	uint64_t now = std::time(0);
	tools->SetThreadName(tName.data());
	setStatus(eManagerStatus::eManagerStatus::running);
	bool wasPaused = false;
	if (mStatus != eManagerStatus::eManagerStatus::running)
		setStatus(eManagerStatus::eManagerStatus::running);
	uint64_t justCommitedFromHeaviestChainProof = 0;

	mUDTServerThread = std::thread(&CUDTConversationsServer::UDTConversationServerThreadF, this);
	uint64_t cleanedCount = 0;
	//Local Variables - END

	//Operational Logic - BEGIN
	while (mStatus != eManagerStatus::eManagerStatus::stopped)
	{
		now = std::time(0);
		cleanedCount = 0;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		wasPaused = false;

		if (!CGRIDNET::getInstance()->getIsShuttingDown())
		{

			//cleaning - BEGIN
			if (now - getLastTimeCleanedUp() > 15)
			{
				cleanedCount = doCleaningUp();//updates last action timestamp all by itself.

				if (cleanedCount)
				{
					tools->writeLine("Freed " + std::to_string(cleanedCount) + " UDT Conversations");
				}
			}
			//cleaning - END
		
		}
		if (mStatusChange == eManagerStatus::eManagerStatus::paused)
		{
			setStatus(eManagerStatus::eManagerStatus::paused);
			mStatusChange = eManagerStatus::eManagerStatus::initial;

			while (mStatusChange == eManagerStatus::eManagerStatus::initial)
			{

				if (!wasPaused)
				{
					tools->writeLine("My thread operations were freezed. Halting..");
					if (mUDTServerThread.native_handle() != 0)
					{
						while (!mUDTServerThread.joinable())
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
						mUDTServerThread.join();
					}

					wasPaused = true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		if (wasPaused)
		{
			tools->writeLine("My thread operations are now resumed. Commencing further..");
			mUDTServerThread = std::thread(&CUDTConversationsServer::UDTConversationServerThreadF, this);
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
	//Operational Logic - END
}

eManagerStatus::eManagerStatus CUDTConversationsServer::getStatus()
{
	std::lock_guard<std::recursive_mutex> lock(mStatusGuardian);
	return mStatus;
}

void CUDTConversationsServer::setStatus(eManagerStatus::eManagerStatus status)
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

void CUDTConversationsServer::requestStatusChange(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	mStatusChange = status;
}

eManagerStatus::eManagerStatus CUDTConversationsServer::getRequestedStatusChange()
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	return mStatusChange;
}
void validDatagramCallback(uint64_t protocolType, std::vector<uint8_t>& senderID, uint64_t srcPort, void* fn_data)
{
	CUDTConversationsServer* udtServer = reinterpret_cast<CUDTConversationsServer*> (fn_data);

	if (udtServer)
	{
		std::shared_ptr<CNetworkManager> nm = udtServer->getNetworkManager();

		if (nm)
		{
			if (!srcPort)
			{

				return;
			}
			////CEndPoint endpoint(senderID, eEndpointType::IPv4,
			//	nullptr, srcPort);

			nm->pingDatagramReceived(eNetworkSystem::UDT, senderID);
		}
	}
}
void unsolicitedDatagramCallback(uint64_t protocolType, std::vector<uint8_t> &data, std::vector<uint8_t>& senderID , uint64_t srcPort, void* fn_data)
{
		CUDTConversationsServer* udtServer = reinterpret_cast<CUDTConversationsServer*> (fn_data);

		if (udtServer)
		{
			std::shared_ptr<CNetworkManager> nm = udtServer->getNetworkManager();

			if (nm)
			{
				if (!srcPort)
				{
					
					return;
				}
				CEndPoint endpoint(senderID, eEndpointType::IPv4, 
					nullptr, srcPort);
			
				nm->processUnsolicitedDatagram(protocolType, data, endpoint);
			}
		}
}
