#pragma once
#include "NetworkManager.h"
#include "BlockchainManager.h"

#include "EEndPoint.h"
#include "Settings.h"
#include "Verifiable.h"
#include "conversationState.h"
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
#include "StatusBarHub.h"
#include "conversation.h"
#include "udt/src/udt.h"
#include "CGlobalSecSettings.h"
#include "keyChain.h"
#include "transaction.h"
#include <algorithm>  // for std::clamp
#include "NetTask.h"
#include "Receipt.h"
#include "DTI.h"
#include "DTIServer.h"
#include "GRIDNET.h"
#include "UDTConversationsServer.h"
#include "KademliaServer.h"
#include <httplib.h>
#include "CFileSystemServer.h"
#include "WebSocketServer.h"
#include "DataRouter.h"
#include "WWWServer.h"
#include "VMMetaGenerator.h"
#include "CORSproxy.h"
#include "ChatMsg.h"
#include "ThreadPool.h"
#include "ConnTracker.h"
#include "QUICConversationsServer.h"
/**
 * Retrieves all IP addresses associated with a given domain name.
 *
 * This method uses the Winsock2 API to query the DNS records for the specified domain. It supports both IPv4 and IPv6 addresses.
 * In case of failure during the DNS query or if the Winsock initialization fails, the method logs an error and returns an empty vector.
 *
 * @param domain The domain name for which to retrieve the IP addresses.
 * @return std::vector<std::string> A vector containing the string representations of all IP addresses associated with the domain.
 * If the domain is invalid, non-existent, or in case of an error, returns an empty vector.
 *
 * @note The function initializes Winsock with WSAStartup and cleans it up with WSACleanup after execution.
 * It logs errors with a severity level of 10 and a failure type in the 'DNS' log category.
 *
 * Example Usage:
 * CNetworkManager networkManager;
 * std::vector<std::string> ips = networkManager.getDomainIPs("example.com");
 * for (const auto& ip : ips) {
 *     std::cout << ip << std::endl;
 * }
 */

std::vector<std::string> CNetworkManager::getDomainIPs(const std::string& domain) {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed: " << result << std::endl;
		return std::vector<std::string>();
	}

	struct addrinfo hints, * res, * p;
	std::vector<std::string> ips;
	char ipstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;

	if ((result = getaddrinfo(domain.c_str(), NULL, &hints, &res)) != 0) {
		std::stringstream ss;
		ss<< "getaddrinfo: " << gai_strerror(result);
		getTools()->logEvent("error retrieving A DNS entries for '" + domain + "': " + ss.str(), "DNS", eLogEntryCategory::network, 10, eLogEntryType::failure);
		WSACleanup();
		return ips;
	}

	for (p = res; p != NULL; p = p->ai_next) {
		void* addr;
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
			addr = &(ipv4->sin_addr);
		}
		else { // IPv6
			struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
			addr = &(ipv6->sin6_addr);
		}

		// Convert the IP to a string and add to the ips vector
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		ips.push_back(std::string(ipstr));
	}

	freeaddrinfo(res); // Free the linked list
	WSACleanup();
	return ips;
}
/**
 * @brief Retrieves a filtered list of conversations from both UDT and QUIC servers based on the provided criteria.
 *
 * This method consolidates conversations from UDT and QUIC servers, if requested, and applies filtering based on the
 * specified flags and state conditions. It allows for a flexible retrieval of conversations that match specific requirements,
 * such as being active, in the process of connecting, or matching certain conversation flags.
 *
 * @param flags A struct of type conversationFlags containing specific flags to filter conversations. Only conversations
 *              with matching flags will be included in the returned list.
 * @param fetchUDTConversations If true, conversations from the UDT server will be included in the search.
 * @param onlyActive If true, only conversations in the 'running' state will be included in the returned list.
 * @param connecting If false, conversations in the 'connecting' state will be excluded from the returned list.
 * @param fetchQUICConversations If true, conversations from the QUIC server will be included in the search.
 *
 * @return std::vector<std::shared_ptr<CConversation>> A vector of shared pointers to CConversation objects that match
 *         the specified criteria. The list may include conversations from both UDT and QUIC servers, depending on
 *         the input parameters.
 *
 * @note The method combines and filters conversations based on the specified criteria, which can be adjusted based
 *       on the requirements of the caller. This allows for a high degree of flexibility in managing and interacting
 *       with multiple conversation servers and their respective conversations.
 */
std::vector<std::shared_ptr<CConversation>> CNetworkManager::getConversations(conversationFlags flags, bool fetchUDTConversations, bool onlyActive, bool connecting, bool fetchQUICConversations)
{
	// Local Variables - BEGIN
	std::vector<std::shared_ptr<CConversation>> toRet;
	std::shared_ptr<CUDTConversationsServer> udtServer;
	std::shared_ptr<CQUICConversationsServer> quicServer;
	std::vector<std::shared_ptr<CConversation>> convs;
	conversationFlags cFlags;
	// Local Variables - END

	// Pre-Flight - BEGIN
	quicServer = getQUICServer();
	udtServer = getUDTServer();

	if (fetchQUICConversations && !quicServer)
	{
		fetchQUICConversations = false;
	}

	if (fetchUDTConversations && !udtServer)
	{
		fetchUDTConversations = false;
	}
	
	// Pre-Flight - END

	// Operational Logic - BEGIN

	// Fetch and combine QUIC and UDT conversations as needed
	if (fetchQUICConversations)
	{
		
		auto quicConvs = quicServer->getAllConversations();
		convs.insert(convs.end(), quicConvs.begin(), quicConvs.end());
	}

	if (fetchUDTConversations)
	{
		auto udtConvs = udtServer->getAllConversations();
		convs.insert(convs.end(), udtConvs.begin(), udtConvs.end());
	}

	// Filtering Logic - BEGIN
	for (const auto& conv : convs)
	{
		if (onlyActive && conv->getState()->getCurrentState() != eConversationState::running)
		{
			continue;
		}
		

		if (!connecting && conv->getState()->getCurrentState() == eConversationState::connecting)
		{
			continue;
		}

		cFlags = conv->getFlags();


		if (flags.exchangeBlocks && !cFlags.exchangeBlocks)
			continue;

		if (flags.isMobileApp && !cFlags.isMobileApp)
			continue;

		toRet.push_back(conv);
	}
	// Filtering Logic - END

	// Operational Logic - END

	return toRet;
}


/**
 * Marks a chain proof for a specific block as processed by storing its ID.
 * Used to prevent duplicate processing of the same chain proof.
 *
 * @param blockIID The ID of the block whose chain proof has been processed
 * @return void
 */
void CNetworkManager::markChainProofForLeadBlockProcessed(std::vector<uint8_t> blockIID) {
	// Validation - BEGIN
	if (blockIID.size() == 0)
		return;
	// Validation - END

	// Operational Logic - BEGIN
	std::lock_guard <std::mutex> lock(mFieldsGuardian);
	mReceivedChainproofsFor.insert(blockIID);
	// Operational Logic - END
}

/**
 * Checks if a chain proof for a specific block has already been processed.
 * Helps prevent redundant processing of chain proofs for the same block.
 *
 * @param blockIID The ID of the block to check for processed chain proof
 * @return bool True if chain proof was already processed, false otherwise
 */
bool CNetworkManager::wasChainProofForBlockProcessed(std::vector<uint8_t> blockIID) {
	// Validation - BEGIN
	if (blockIID.size() == 0)
		return false;
	// Validation - END

	// Operational Logic - BEGIN
	std::lock_guard <std::mutex> lock(mFieldsGuardian);
	if (mReceivedChainproofsFor.find(blockIID) != mReceivedChainproofsFor.end()) {
		return true;
	}
	return false;
	// Operational Logic - END
}

/**
 * Clears all recorded chain proof processing history.
 * Useful for resetting the state or clearing memory when needed.
 * Thread-safe operation protected by mFieldsGuardian.
 *
 * @return bool Always returns true to indicate successful operation
 */
bool CNetworkManager::forgetReceivedChainProofs() {
	// Operational Logic - BEGIN
	std::lock_guard <std::mutex> lock(mFieldsGuardian);
	mReceivedChainproofsFor.clear();
	return true;
	// Operational Logic - END
}


/// <summary>
/// Eliminates duplicate connections in a vector of DSM (Distributed Synchronization Mechanism) conversations.
/// This function ensures that only a specified number of duplicate connections per IP address are allowed, 
/// while also giving preference to certain protocols (QUIC over UDT) to maintain optimal performance.
/// </summary>
/// <param name="dsmConvs">Vector of shared pointers to CConversation objects representing active DSM conversations.</param>
/// <param name="allowedDuplicates">The maximum number of allowed duplicate connections per IP address.</param>
/// <param name="bePolite">Boolean flag indicating whether to wait for the peer with a larger IP address to end the connection. 
/// If set to true, the function ends a connection only if the local IP address is 'smaller' than the remote IP, 
/// or if bePolite is false, indicating an aggressive approach to eliminating duplicates.</param>
/// <returns>The total number of eliminated duplicate connections.</returns>
/// <remarks>
/// Connections are first filtered to include only those in a running state. They are then grouped by their remote IP addresses.
/// Within each group, connections are sorted to prioritize QUIC over UDT. 
/// The function respects the allowed number of duplicates, and excess connections are terminated based on the comparison 
/// of BigInt representations of local and remote IP addresses, and the bePolite flag.
/// </remarks>
uint64_t CNetworkManager::eliminateDuplicates(std::vector<std::shared_ptr<CConversation>>& dsmConvs, uint64_t allowedDuplicates, bool bePolite) {
	
	//Local Variables - BEGIN
	std::unordered_map<std::string, std::vector<std::shared_ptr<CConversation>>> connectionsPerIP;
	uint64_t eliminated = 0;
	std::shared_ptr<CTools> tools = getTools();
	std::string localIP = getPublicIP();
	BigInt localIPBI = tools->BytesToBigInt(std::vector<uint8_t>(localIP.begin(), localIP.end()));
	//Local Variables - END

	// Operational Logic
	for (auto& conversation : dsmConvs) {
		if (conversation->getState()->getCurrentState() != eConversationState::running) {
			continue; // Consider only running conversations
		}

		auto endPoint = conversation->getEndpoint();
		std::string remoteIP = tools->bytesToString(endPoint->getAddress());
		connectionsPerIP[remoteIP].push_back(conversation);
	}

	for (auto& [ip, connections] : connectionsPerIP) {
		BigInt remoteIPBI = tools->BytesToBigInt(std::vector<uint8_t>(ip.begin(), ip.end()));

		// Sort connections by protocol priority (QUIC over UDT)
		std::sort(connections.begin(), connections.end(), [](const std::shared_ptr<CConversation>& a, const std::shared_ptr<CConversation>& b) {
			return a->getProtocol() == eTransportType::QUIC && b->getProtocol() != eTransportType::QUIC;
			});

		// Keep allowed number of duplicates, eliminate the rest
		for (size_t i = allowedDuplicates; i < connections.size(); ++i) {
		/* end the conversation ONLY if we 'dominate' the other peer.
	   otherwise, wait for the other peer to end the conversation.
	   that mitigates situation in which both peers would end (not necessary same)
	   conversations leading to a starvation and no connectivity between these.*/
			if ((remoteIPBI < localIPBI) || !bePolite) {
				connections[i]->end(false, true, eConvEndReason::duplicateStream);
				eliminated++;
			}
		}
	}

	return eliminated;
	//Operational Logic - END

}




uint64_t CNetworkManager::getLastDSMQualityReportTimestamp()
{
	std::lock_guard <std::mutex> lock(mFieldsGuardian);
	return mLastDSMQualityReport;
}

void CNetworkManager::pingDSMQualityReport()
{
	std::lock_guard <std::mutex> lock(mFieldsGuardian);
	mLastDSMQualityReport = std::time(0);
}
void CNetworkManager::handlePortInUse(unsigned short port, eFirewallProtocol::eFirewallProtocol protocol)
{
	//Local Variables - BEGIN
	DWORD processId;
	eColor::eColor logColor = eColor::none;
	std::string processOrServiceName;
	bool portInUse = false;
	std::shared_ptr<CTools> tools = getTools();
	std::stringstream ss;
	std::vector<std::string> serviceNames;

	//Local Variables - END

	DWORD pid = tools->GetRunningProcessPID();
	if (tools->GetProcessInfoByPort(port, protocol, processId, processOrServiceName, serviceNames) && (pid != processId)) {
		std::stringstream portMessage;
		portMessage << "Port " << port << " is used by process ID: " << processId << ", Name/Service: " << processOrServiceName;

		if (processOrServiceName == "svchost.exe" && !serviceNames.empty()) {
			for (const auto& serviceName : serviceNames) {
				std::string question = portMessage.str() + ". Do you want to stop the service '" + serviceName + "'?";
				if (tools->askYesNo(question, true, "Networking Assistant", true, false, true)) {
					tools->logEvent("Stopping service " + serviceName, "Maintenance", eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::lightPink);
					Sleep(4000);
					tools->stopServiceByName(serviceName, true);
				}
			}
		}
		else {
			std::string question = portMessage.str() + ". Do you want to kill the process?";

			if (tools->askYesNo(question, true, "Networking Assistant")) {
				tools->logEvent("Terminating " + processOrServiceName, "Maintenance", eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::lightPink);
				Sleep(4000);
				tools->terminateProcessByID(processId);
			}
		}
	}
	else {
		tools->logEvent("No process found using port " + std::to_string(port), "Networking Assistant", eLogEntryCategory::localSystem, 10, eLogEntryType::notification, eColor::orange);
	}
};

/**
 * @brief Initializes various subsystems for network operations.
 *
 * This function performs a series of checks and initializations for
 * different networking modules and services. It also ensures that
 * required ports are not being used by other processes or services.
 * If conflicts are detected (e.g., port usage by other processes or services),
 * the user is prompted for corrective actions such as stopping a service
 * or terminating a process.
 *
 * @return Returns `true` if all subsystems were initialized successfully.
 *         Returns `false` if an error occurred or if the user chooses
 *         to abort due to a detected conflict.
 *
 * @note This function relies on CTools for several utility operations
 *       like checking port usage, logging events, and prompting the user.
 *       Ensure that CTools is properly initialized and available before
 *       invoking this function.
 *
 * @warning Any errors or conflicts detected during initialization could
 *          lead to a system shutdown or service stop based on user's
 *          response to prompts.
 */
bool CNetworkManager::initializeSubSystems()
{
	//Local Variables - BEGIN
	DWORD processId;
	eColor::eColor logColor = eColor::none;
	std::string processOrServiceName;
	bool portInUse = false;
	unsigned short port = 443;
	std::stringstream ss;
	//Local Variables - END

	std::shared_ptr<CTools> tools = getTools();


	//System checks - BEGIN
	if (!(CGlobalSecSettings::getWarnPeerAboutPendingShutdownAfter() <
		CGlobalSecSettings::getGentleConversationShutdownAfter()
		< CGlobalSecSettings::getForcedConversationShutdownAfter()))
	{
		tools->logEvent("[CRITICAL Error]: Invalid system-wide conversation-timeouts configured.", eLogEntryCategory::localSystem, 3, eLogEntryType::failure);
		return false;
	}

	//System checks - END

	//Take care of the DTI Server - BEGIN

	//Preliminary Self-Diagnostics - BEGIN
	handlePortInUse(22, eFirewallProtocol::TCP);

	//Preliminary Self-Diagnostics - END


	if (getInitializeDTIServer())
	{
		if (mDTIServer == nullptr)
			mDTIServer = std::make_shared<CDTIServer>(shared_from_this());
		if (mRouter == nullptr)
			mRouter = std::make_shared<CDataRouter>(shared_from_this());
		CSettings::setGlobalAutoConfigStepDescription("Initializing DTI system for " + getTools()->blockchainmodeToString(getBlockchainMode()));
		if (!mDTIServer->initialize())
			if (tools->askYesNo("Error: Unable to initialize DTI Server. Do you want to abort?", true, "", false, false, true))
			{
				CTools::clearScreen();
				CTools::writeLineS("Shutting down..");
				CGRIDNET::getInstance()->shutdown();
				return false;
			}
	}
	else
		tools->writeLine("DTI Server will NOT be enabled.");



	

	//Take care of the QUIC Server - START
	port = 444;
	//Preliminary Self-Diagnostics - BEGIN
	handlePortInUse(port, eFirewallProtocol::UDP);

	//Preliminary Self-Diagnostics - END


	if (getInitializeQUICServer())
	{
		std::shared_ptr<CQUICConversationsServer> server = getQUICServer();

		if (server == nullptr)
		{
			{
				std::lock_guard<std::mutex> lock(mFieldsGuardian);
				mQUICServer = std::make_shared<CQUICConversationsServer>(mThreadPool, mBlockchainManager, shared_from_this());
			}

			server = getQUICServer();
		}

		CSettings::setGlobalAutoConfigStepDescription("Initializing QUIC sub-system for " + getTools()->blockchainmodeToString(getBlockchainMode()));
		if (!server->initialize())
			if (tools->askYesNo("Error: Unable to initialize QUIC-Conversations Server. Do you want to abort?", true, "", false, false, true))
			{
				CTools::clearScreen();
				CTools::writeLineS("Shutting down..");
				CGRIDNET::getInstance()->shutdown();
				return false;
			}

		mQUICNetworkingEnabled = true;

	}
	else
		tools->writeLine("QUIC Server will NOT be enabled.");
	//Take care of the QUIC  Server - END


	//Take care of the UDT Server - START

	port = 443;
	//Preliminary Self-Diagnostics - BEGIN
	handlePortInUse(port, eFirewallProtocol::UDP);

	//Preliminary Self-Diagnostics - END
	if (getInitializeUDTServer())
	{
		if (mUDTServer == nullptr)
			mUDTServer = std::make_shared<CUDTConversationsServer>(mThreadPool, mBlockchainManager, shared_from_this());
		CSettings::setGlobalAutoConfigStepDescription("Initializing UDT sub-system for " + getTools()->blockchainmodeToString(getBlockchainMode()));
		if (!mUDTServer->initialize())
			if (tools->askYesNo("Error: Unable to initialize UDT-Conversations Server. Do you want to abort?", true, "", false, false, true))
			{
				CTools::clearScreen();
				CTools::writeLineS("Shutting down..");
				CGRIDNET::getInstance()->shutdown();
				return false;
			}

		mUDTNetworkingEnabled = true;

	}
	else
		tools->writeLine("UDT Server will NOT be enabled.");
	//Take care of the UDT  Server - END


	//Take care of Web-Server - START

	//Preliminary Self-Diagnostics - BEGIN
	//check 443 TCP
	handlePortInUse(443, eFirewallProtocol::TCP);
	handlePortInUse(80, eFirewallProtocol::TCP);

	//Preliminary Self-Diagnostics - END

	initWebServer();
	//Take care of Web-Server - END

	//Take care of CORS-Proxy - START
	if (getInitializeCORSProxy())
	{
		if (mCORSProxy == nullptr)
			mCORSProxy = std::make_shared<CCORSProxy>(mBlockchainManager);
		CSettings::setGlobalAutoConfigStepDescription("Initializing  CORS proxy for " + getTools()->blockchainmodeToString(getBlockchainMode()));
		if (!mCORSProxy->initialize())
			if (tools->askYesNo("Error: Unable to initialize CORS Proxy. Do you want to abort?", true, "", false, false, true))
			{
				CTools::clearScreen();
				CTools::writeLineS("Shutting down..");
				CGRIDNET::getInstance()->shutdown();
				return false;
			}


		mWebNetworkingEnabled = true;

	}
	else
		tools->writeLine("CORS proxy will NOT be enabled.");
	//Take care of CORS-Proxy - END


	//Take care of WebSockets Server - START
	if (mWebNetworkingEnabled && getInitializeWebSocketsServer())
	{
		if (mWebSocketsServer == nullptr)
			mWebSocketsServer = std::make_shared<CWebSocketsServer>(mThreadPool, mBlockchainManager);
		CSettings::setGlobalAutoConfigStepDescription("Initializing WebSockets for " + getTools()->blockchainmodeToString(getBlockchainMode()));
		if (!mWebSocketsServer->initialize())
			if (tools->askYesNo("Error: Unable to initialize Web-Sockets Server. Do you want to abort?",  true, "", false, false, true))
			{
				CTools::clearScreen();
				CTools::writeLineS("Shutting down..");
				CGRIDNET::getInstance()->shutdown();
				return false;
			}

		mWebSocketsNetworkingEnabled = true;

	}
	else
		tools->writeLine("WebSockets Server will NOT be enabled.");
	//Take care of WebSockets Server - END


		//Take care of File Server - START
	//the functionality is taken care of directly through CConversations through either WebSockets or UDT
		/*if (mWebSocketsNetworkingEnabled && getInitializeFileServer())
		{
			if (mFileSystemServer == nullptr)
				mFileSystemServer = std::make_shared<CFileSystemServer>(mBlockchainManager);

			if (!mFileSystemServer->initialize())
				if (tools->askYesNo("Error: Unable to initialize File Server. Do you want to abort?", true))
				{
					CGRIDNET::getInstance()->shutdown();
					return false;
				}

			mFileSystemNetworkingEnabled = true;

		}
		else
			tools->writeLine("UDT Server will NOT be enabled.");*/
			//Take care of File Server - END



		//Take care of Kademlia - BEGIN

	if (getInitializeKademlia()) {

		mKademliaServer = std::make_shared<CKademliaServer>(mIsBootstrapNode, mBlockchainManager, shared_from_this());
		// --- BEGIN FIX 1 ---
		// FIX: Join the Kademlia initialization thread to prevent use-after-free.
		//      Do not detach if the thread might access CNetworkManager members after CNetworkManager
		//      could be potentially destroyed or if its lifetime isn't strictly shorter.
		//      Here, we make it a member and join it in Stop() or destructor.
		mTools->logEvent("Starting Kademlia initialization thread...", "NetworkManager", eLogEntryCategory::localSystem, 1);
		mKademliaInitThread = std::thread(&CNetworkManager::initializeKademlia, this, std::ref(mKademliaServer), mIsBootstrapNode, shared_from_this());
		// kademliaThread.detach(); // REMOVED DETACH
		// --- END FIX 1 ---

	}
	else {
		tools->writeLine("P2P node-discovery (Kademlia) will NOT be enabled.");
	}

	//Take care of Kademlia - END
	return true;
}

void CNetworkManager::initializeKademlia(std::shared_ptr<CKademliaServer>& mKademliaServer, bool mIsBootstrapNode, std::shared_ptr<CNetworkManager> sharedThis) {
	if (mKademliaServer == nullptr)
		
	CSettings::setGlobalAutoConfigStepDescription("Initializing Kademlia for " + sharedThis->getTools()->blockchainmodeToString(sharedThis->getBlockchainMode()));
	if (!mKademliaServer->initialize()) {
		if (sharedThis->getTools()->askYesNo("Error: Unable to initialize Kademlia. Do you want to abort?", true, "", false, false, true)) {
			CTools::clearScreen();
			CTools::writeLineS("Shutting down..");
			CGRIDNET::getInstance()->shutdown();
			// Handle failure appropriately, possibly setting a flag to indicate initialization failure
		}
	}
	else {
		sharedThis->setIsKademliaOperational(true);
	}
}

bool CNetworkManager::initWebServer()
{
	std::shared_ptr<CTools> tools = getTools();
	if (getInitializeWebServer())
	{
		if (mWWWServer == nullptr)
		{

			tools->writeLine("Pre-flight checks for the internal HTTP/WWW sub-system..");

			handlePortInUse(443, eFirewallProtocol::TCP);
			handlePortInUse(80, eFirewallProtocol::TCP);

			mWWWServer = std::make_shared<CWWWServer>(mThreadPool, mBlockchainManager);
			CSettings::setGlobalAutoConfigStepDescription("Initializing  WWW server for " + getTools()->blockchainmodeToString(getBlockchainMode()));
			if (!mWWWServer->initialize())
				tools->askYesNo(tools->getColoredString("Error:", eColor::cyborgBlood) + " Unable to initialize the internal Web Server. Operator! Make sure ports 80 and 443 are not being used by other apps. Do you acknowledge ? ", true, "", false, false, true);

			mWebNetworkingEnabled = true;
			return true;
		}
		else
		{
			tools->writeLine("Web-Server already initialized");
			return true;
		}

	}
	else
	{
		tools->writeLine("Web-Server will NOT be enabled.");
		return false;
	}
	return false;
}

/// <summary>
/// The purpose of this function is to determine whether an IP address (IPv4/IPv6)
/// LIKELY comprises one of the local interfaces.
/// Warning: the function is ALLOWED to return invalid results.
/// </summary>
/// <param name="ip"></param>
/// <returns></returns>
bool CNetworkManager::isIPAddressLocal(std::vector<uint8_t> ip)
{

	std::vector<std::string> locals = getLocalIPAdresses();
	std::shared_ptr<CTools> tools = getTools();
	std::string publicIP = getPublicIP();
	std::string wildcard = "0.0.0.0";

	locals.push_back("127.0.0.1");
	locals.push_back(publicIP);
	locals.push_back(wildcard);

	for (uint64_t i = 0; i < locals.size(); i++)
	{
		if (tools->compareByteVectors(locals[i], ip))
		{
			return true;
		}
	}
	return false;
}

void CNetworkManager::pingDatagramReceived(eNetworkSystem::eNetworkSystem netSystem, std::vector<uint8_t> address)
{
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::lock_guard<std::mutex> lock(mStatisticsGuardian);
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> ::iterator sIt;
	std::vector<uint8_t> addrImage = cf->getSHA2_256Vec(address);
	uint64_t datagramCount = 0;
	switch (netSystem)
	{
	case eNetworkSystem::unknown:
		sIt = mUnsolicitedDatagramsReceived.find(addrImage);
		if (sIt != mUnsolicitedDatagramsReceived.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mUnsolicitedDatagramsReceived[addrImage] = std::make_tuple(address,datagramCount);

		break;
	case eNetworkSystem::Kademlia:
		sIt = mValidKademliaDatagramsReceived.find(addrImage);
		if (sIt != mValidKademliaDatagramsReceived.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mValidKademliaDatagramsReceived[addrImage] = std::make_tuple(address, datagramCount);

		break;
	case eNetworkSystem::UDT:
		sIt = mValidUDTDatagramsReceived.find(addrImage);
		if (sIt != mValidUDTDatagramsReceived.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mValidUDTDatagramsReceived[addrImage] = std::make_tuple(address, datagramCount);
		break;

	case eNetworkSystem::QUIC:
		sIt = mValidQUICDatagramsReceived.find(addrImage);
		if (sIt != mValidQUICDatagramsReceived.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mValidQUICDatagramsReceived[addrImage] = std::make_tuple(address, datagramCount);
		break;

	case eNetworkSystem::HTTPProxy:
		break;
	case eNetworkSystem::WebServer:
		break;
	case eNetworkSystem::WebRTC:
		break;
	case eNetworkSystem::WebSocket:
		break;
	default:
		break;
	}
}

void CNetworkManager::pingInvalidDatagramReceived(eNetworkSystem::eNetworkSystem netSystem, std::vector<uint8_t> address)
{
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::vector<uint8_t> addrImage = cf->getSHA2_256Vec(address);
	uint64_t datagramCount = 0;
	std::lock_guard<std::mutex> lock(mStatisticsGuardian);
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> ::iterator sIt;

	switch (netSystem)
	{
	case eNetworkSystem::unknown:
		break;
	case eNetworkSystem::Kademlia:
		sIt = mInvalidDatagramsReceived.find(addrImage);
		if (sIt != mInvalidDatagramsReceived.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mInvalidDatagramsReceived[addrImage] = std::make_tuple(address, datagramCount);
		break;

	case eNetworkSystem::UDT:
		sIt = mInvalidDatagramsReceived.find(addrImage);
		if (sIt != mInvalidDatagramsReceived.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mInvalidDatagramsReceived[addrImage] = std::make_tuple(address, datagramCount);
		break;

	case eNetworkSystem::HTTPProxy:
		break;
	case eNetworkSystem::WebServer:
		break;
	case eNetworkSystem::WebRTC:
		break;
	case eNetworkSystem::WebSocket:
		break;
	case eNetworkSystem::QUIC:
		sIt = mInvalidDatagramsReceived.find(addrImage);
		if (sIt != mInvalidDatagramsReceived.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mInvalidDatagramsReceived[addrImage] = std::make_tuple(address, datagramCount);
		break;
	default:
		break;
	}
}


void CNetworkManager::pingDatagramSent(eNetworkSystem::eNetworkSystem netSystem, std::vector<uint8_t> address)
{
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::vector<uint8_t> addrImage = cf->getSHA2_256Vec(address);
	/*
	std::lock_guard<std::mutex> lock(mStatisticsGuardian);


	uint64_t datagramCount = 0;
	switch (netSystem)
	{
	case eNetworkSystem::unknown:
		sIt = mUnsolicitedDatagramsReceived.find(address);
		if (sIt != mUnsolicitedDatagramsReceived.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;*/
	uint64_t datagramCount = 0;
	std::lock_guard<std::mutex> lock(mStatisticsGuardian);
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> ::iterator sIt;

	switch (netSystem)
	{
	case eNetworkSystem::unknown:
		break;
	case eNetworkSystem::Kademlia:
		sIt = mKademliaDatagramsSent.find(addrImage);
		if (sIt != mKademliaDatagramsSent.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mKademliaDatagramsSent[addrImage] = std::make_tuple(address, datagramCount);
		break;

	case eNetworkSystem::UDT:
		sIt = mUDTDatagramsSent.find(addrImage);
		if (sIt != mUDTDatagramsSent.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mUDTDatagramsSent[addrImage] = std::make_tuple(address, datagramCount);;
		break;

	case eNetworkSystem::QUIC:
		sIt = mQUICDatagramsSent.find(addrImage);
		if (sIt != mQUICDatagramsSent.end())
			datagramCount = std::get<1>(sIt->second);
		datagramCount++;
		mQUICDatagramsSent[addrImage] = std::make_tuple(address, datagramCount);;
		break;

	case eNetworkSystem::HTTPProxy:
		break;
	case eNetworkSystem::WebServer:
		break;
	case eNetworkSystem::WebRTC:
		break;
	case eNetworkSystem::WebSocket:
		break;
	default:
		break;
	}
}

void CNetworkManager::clearCounters(uint64_t& unsolicitedRX, uint64_t& validKADRX, uint64_t& validUDTRX, uint64_t& invalidRX, uint64_t& KADTX, uint64_t& UDTTX, uint64_t& validQUICRX, uint64_t& QUICTX) {
	unsolicitedRX = 0;
	validKADRX = 0;
	validUDTRX = 0;
	invalidRX = 0;
	KADTX = 0;
	UDTTX = 0;
	validQUICRX = 0;
	QUICTX = 0;
}

/// <summary>
/// Removes all rules from the underlying operating system's kernel mode module.
/// </summary>
/// <param name="wipeAll"></param>
void CNetworkManager::cleanKernelModeRules(bool wipeAll)
{
	std::shared_ptr<CTools> tools = getTools();
	tools->cleanKernelFirewallRules();
}

/// <summary>
/// The Network's Manager controller thread is responsible for management of all the underlying Sybil-proof data-exchange sub-systems.
//Namely:
// + Decentralized Terminal Interface (DTI) Server
// + UDT Server (for exchange of larger data packets and synchronization with the network) - also communication with lite/mobile clients
// + Kademlia peer-discovery sub-system - provides fully decentralized discovery of GRIDNET-Core full-nodes throughout the Internet.
/// </summary>
void CNetworkManager::mControllerThreadF()
{
	std::string tName = "Network Manager";
	
	//mKademlialNetworkingEnabled = true;
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	mDTINetworkingEnabled = true;
	std::shared_ptr<CTools> tools = getTools();
	std::stringstream reportBuilder;
	uint64_t lastReport = 0;
	tools->SetThreadName(tName.data());

	if (!getIsReady())
	{
		tools->writeLine("Waiting for myself to become ready..");

		while (!getIsReady())
		{
			if (getRequestedStatusChange() == eManagerStatus::eManagerStatus::stopped)
				return;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		tools->writeLine("I'm ready, commencing further..");
	}

	if (!getBlockchainManager()->getIsReady())
	{
		tools->writeLine("Waiting for Blockchain Manager to become ready..");
		while (!mBlockchainManager->getIsReady())
		{
			if (getRequestedStatusChange() == eManagerStatus::eManagerStatus::stopped)
				return;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		tools->writeLine("Blockchain Manager ready, commencing further..");
	}

	setStatus(eManagerStatus::eManagerStatus::initial);

	bool initOk = initializeSubSystems();
	if (!initOk)
	{
		if (!tools->askYesNo("Some of the networking sub. Do you want to continue?", false))
		{
			getBlockchainManager()->exit(true);
		}
		else
		{
			if (!tools->askYesNo("Do you want Network Manager to remain active and continue with available sub-systems?", true))
			{
				stop();
				return;
			}

		}
	}
	else
		tools->writeLine("All the enabled communication sub-system INITIALIZED. Commencing further..");

	bool wasPaused = false;
	bool analysisMode = false;
	uint64_t lastStatusReport = 0;
	uint64_t eliminatedCount = 0;
	uint64_t lastReportAboutDisabledSync = 0;
	uint64_t lastAnalysisReport = 0;
	uint64_t now = 0;
	uint64_t unsolicitedRX, validKADRX, validUDTRX, validQUICRX, invalidRX, KADTX, UDTTX, QUICTX;
	uint64_t lastThreadCountReport = 0;
	std::shared_ptr<CWWWServer> wwwServer;
	std::vector <std::shared_ptr<CConversation>> conversations;
	if (mStatus != eManagerStatus::eManagerStatus::running)
		setStatus(eManagerStatus::eManagerStatus::running);
	std::stringstream report;
	std::shared_ptr<CStatusBarHub> barHub = CStatusBarHub::getInstance();
	eBlockchainMode::eBlockchainMode bMode = getBlockchainMode();
	conversationFlags flags;
	while (mStatus != eManagerStatus::eManagerStatus::stopped)
	{


		std::this_thread::sleep_for(std::chrono::milliseconds(50));


		pingLastTimeControllerThreadRun();



		if ((now - lastThreadCountReport) > 5)
		{
			tools->logEvent(tools->getColoredString("Network Threads:", eColor::blue) + tools->getColoredString(std::to_string(getThreadPool()->getActiveThreadCount()), eColor::lightCyan), "Network Manager", eLogEntryCategory::localSystem);
			lastThreadCountReport = now;
		}
		//Maintenance Mode - BEGIN
		
		//Maintenance Mode - END
		now = tools->getTime();

		//Refresh Variables - BEGIN
		conversations = getConversations(flags, true, true, true, true);
		eliminatedCount = 0;
		report.str("");
		analysisMode = getIsNetworkTestMode();
		if (analysisMode)
		{
			//clearCounters(unsolicitedRX, validKADRX, validUDTRX, invalidRX, KADTX, UDTTX);

			getCounters(unsolicitedRX, validKADRX, validUDTRX, invalidRX, KADTX, UDTTX, validQUICRX, QUICTX);

			if ((now - mLastAnalysisStatusBarModeSwitched) > 2)
			{
				mLastAnalysisStatusBarMode = (mLastAnalysisStatusBarMode + 1) % 4;
				mLastAnalysisStatusBarModeSwitched = now;
			}

			//Main Report - BEGIN
			if (!getIsFocused() && ((now - lastAnalysisReport) > 20))
			{
				std::string report = getAnalysisReport();
				tools->writeLine(CTools::getFTNoRendering()+ report, true, false);
				lastAnalysisReport = now;
			}
			//Main Report - END

			// Status Bar Report - BEGIN
			switch (mLastAnalysisStatusBarMode)
			{
			case 0:
				report << tools->getColoredString("Total RX: ", eColor::lightCyan) << std::to_string(validKADRX + validUDTRX) << " [KAD]: " << std::to_string(validKADRX) << " [QUIC]: " << std::to_string(validQUICRX) << " [UDT]: " << std::to_string(validUDTRX);

				break;
			case 1:
				report << tools->getColoredString("Total TX: ", eColor::lightCyan) << std::to_string(KADTX + UDTTX) << " [KAD]: " << std::to_string(KADTX) << " [QUIC]: " << std::to_string(QUICTX) << " [UDT]: " << std::to_string(UDTTX);
				break;

			case 2:
				report << tools->getColoredString("Invalid RX: ", eColor::lightCyan)  << std::to_string(invalidRX);
				break;
			case 3:
				report << tools->getColoredString("Grid Analysis Active ", eColor::lightCyan);
				break;
			default:
				break;
			}
			// Status Bar Report - END
			std::string reportS = report.str();
			barHub->setCustomStatusBarText(bMode, getMyCustomStatusBarID(), reportS, 20);
		}
		//Refresh Variables - END

		if (mStatusChange == eManagerStatus::eManagerStatus::stopped)
		{
			tools->logEvent(tools->getColoredString("I'm shutting down.", eColor::cyborgBlood), "Network Manager", eLogEntryCategory::localSystem);
			mStatusChange = eManagerStatus::eManagerStatus::initial;
			mDTIServer->stop();
			mUDTServer->stop();
			if (mWebSocketsServer != nullptr);
			mWebSocketsServer->stop();

			mStatus = eManagerStatus::eManagerStatus::stopped;
			break;

		}
		try
		{
			//Extremely important: keep the pause-loop BEFORE a call to accept().
			//When the Network Manager is paused; the socket gets killed.
			wasPaused = false;
			if (mStatusChange == eManagerStatus::eManagerStatus::paused)
			{
				setStatus(eManagerStatus::eManagerStatus::paused);
				mStatusChange = eManagerStatus::eManagerStatus::initial;
				while (mStatusChange == eManagerStatus::eManagerStatus::initial)
				{
					if (!wasPaused)
					{
						tools->writeLine("My thread operations were friezed. Halting..");
						if (mDTIServer != nullptr)
							mDTIServer->pause();

						if (mUDTServer != nullptr)
							mUDTServer->pause();

						if (mWebSocketsServer != nullptr);
						mWebSocketsServer->pause();

						wasPaused = true;
					}

				}
			}

			if (wasPaused)
			{
				tools->writeLine("My thread operations are now resumed. Commencing further..");

				if (mQUICNetworkingEnabled)
				{
					tools->writeLine("Rebooting UDT-server..");
					std::shared_ptr<CQUICConversationsServer> server = getQUICServer();
					if (server != nullptr)
						server->resume();
					tools->writeLine("QUIC Server up and running..");
				}

				if (mUDTNetworkingEnabled)
				{
					tools->writeLine("Rebooting UDT-server..");
					if (mUDTServer != nullptr)
						mUDTServer->resume();
					tools->writeLine("UDT Server up and running..");
				}

				if (mDTINetworkingEnabled)
				{
					tools->writeLine("Rebooting  DTI-server..");
					if (mDTIServer != nullptr)
						mDTIServer->resume();
				}

				if (mWebSocketsNetworkingEnabled);
				{
					tools->writeLine("Rebooting  WebSockets-server..");
					if (mWebSocketsServer != nullptr)
						mWebSocketsServer->resume();
				}

				tools->writeLine("DTI Server up and running..");

				mStatus = eManagerStatus::eManagerStatus::running;
			}

			//main network-tasks processing - BEGIN
			//here tasks are distributed across  particular Conversations (of multiple type e.x. UDT vs websockets)

			processNetTasks();
			uint64_t now = tools->getTime();

			// Clean Up - BEGIN

			// Clean Router - BEGIN
			uint64_t rtCleanup = getLastRouterCleanup();
			if (!(rtCleanup > now) && ((now - rtCleanup) > getRouterCleanupInterval()))
			{
				mRouter->cleanTable();
				now = tools->getTime();
				setLastRouterCleanup(now);
			}
			// Clean Router - END

			// Clean Synchronization Streams - BEGIN
			// Below, take care of both QUIC and UDT connections.
			// If at least two or more connections are found between this node and any other node
			// all are to be eliminated (ended) but for one which is to remain.
			// In case both UDT and QUIC connections are found to same IP address, only the QUIC connection is to remain.
			if ((now - getLastConversationsCleanUp()) > 5)
			{
				eliminatedCount = eliminateDuplicates(conversations);
				if (eliminatedCount)
				{
					tools->logEvent("Ending " + std::to_string(eliminatedCount) + " duplicate UDT connections..", eLogEntryCategory::network, 1, eLogEntryType::notification, eColor::none);
				}
				pingLastConversationsCleanUp();
			}
			// Clean Synchronization Streams - END
				
			// Kernel Firewall Clean-Up - BEGIN
			
			if (getIsFirewallKernelModeIntegrationEnabled() &&
				((now - getLastKernelModeFirewallCleanUpTimestamp()) > CGlobalSecSettings::getKernelModeFirewallCleanupInterval()))
			{
				tools->logEvent("Clearing Kernel Mode Firewall Tables..", eLogEntryCategory::network, 3, eLogEntryType::notification, eColor::orange);

				if (tools->cleanKernelFirewallRules()) // clear kernel mode table of the native operating system
				{
					tools->logEvent("Clearing Kernel Mode Firewall Tables succeded.", eLogEntryCategory::network, 3, eLogEntryType::notification, eColor::neonGreen);
				}
				else
				{
					tools->logEvent("Clearing Kernel Mode Firewall Tables failed.", eLogEntryCategory::network, 3, eLogEntryType::notification, eColor::lightPink);
				}

				pingLastKernelModeFirewallCleanUpTimestamp();
			 }

			// Kernel Firewall Clean-Up - END

			// Clean Up - END

			//Network Analysis Mode - BEGIN
			if (analysisMode)
			{

			}
			//Network Analysis Mode - END
			
			//main network-tasks processing - BEGIN
			//Regular-routine tasks - BEGIN

			// DNS Support - BEGIN
		
			if ((now - getDNSBooststrapUpdateTimestamp()) > (60 * 5))
			{
				getBootstrapNodes(true);
				pingDNSBooststrapUpdate();
			}
			// DNS Support - END


			if (mKademlialNetworkingEnabled)
			{

			}
			if (mUDTNetworkingEnabled)
			{

			}

			if (mDTINetworkingEnabled)
			{

			}
			//Regular-routine tasks - END

			//Reporting - BEGIN
				//Synchronization - BEGIN
				//Synchronization - END
				

				//Swarms - BEGIN
				//wwwServer = getWWWServer();
				if (wwwServer)
				{ //done by CWebSocketServer
					//wwwServer->swarm
				}
				//reportBuilder << "There are "<< 
				//Swarms - END
			//Reporting - END
			
			//DSM synchronization - BEGIN
			if (!CSettings::getIsGlobalAutoConfigInProgress())
			{
				if (bm->getSyncMachine())
				{
					DSMSyncThreadF();
				}
				else
				{
					if ((now - lastReportAboutDisabledSync) > 60)
					{
						lastReportAboutDisabledSync = std::time(0);
						tools->logEvent("Warning: State-Synchronization is DISABLED.", "Network Manager", eLogEntryCategory::localSystem, 1, eLogEntryType::notification, eColor::orange);
					}
					
				}
			}
			else
			{
				if ((now - lastStatusReport) > 60)
				{
					lastStatusReport = std::time(0);
					tools->logEvent("Postponing synchronization, node is still bootstrapping..", "Network Manager",eLogEntryCategory::localSystem, 1, eLogEntryType::notification, eColor::orange);
				
				}
			}
			//DSM synchronization - END

		}
		catch (const std::invalid_argument& ex)
		{
		 assertGN(false);

		}
	}
	setStatus(eManagerStatus::eManagerStatus::stopped);
}

std::vector<uint8_t> CNetworkManager::getCachedAutoDetectedPublicIP()
{
	std::lock_guard<std::mutex> lock(mCachedAutoDetectedPublicIPGuardian);
	return mCachedAutoDetectedPublicIP;
}

bool CNetworkManager::notifyAboutNewBlock(std::shared_ptr<CBlockHeader> header)
{
	return enqueHeaderNotification(header);
}

void CNetworkManager::exchangeChatMessagesThreadF()

{
	//Local Variables - BEGIN
	conversationFlags flags = conversationFlags();
	flags.exchangeBlocks = true;
	std::vector<std::shared_ptr<CConversation>>  dsmConvs = getConversations(flags);
	std::shared_ptr<CNetTask> task;
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::vector<uint8_t> temp;
	std::shared_ptr<CChatMsg> chatMsg;
	//Local Variables - END
	setIsChatExchangeAlive(true);
	//process chat messages - BEGIN
	while (getKeepChatExchangeAlive())
	{
		do
		{
			chatMsg = dequeChatMsgNotification();// getHeadChatMsgNotification();
			bool delivered = false;
			if (chatMsg)
			{
				temp = chatMsg->getPackedData();
				if (temp.size())
				{
					for (uint64_t i = 0; i < dsmConvs.size(); i++)
					{
						task = std::make_shared<CNetTask>(eNetTaskType::notifyChatMsg);
						task->setPriority(3);
						task->setData(temp);

						tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
							"notifying peer " + dsmConvs[i]->getIPAddress() + " about a chat msg. ", eLogEntryCategory::network, 0);

						dsmConvs[i]->addTask(task);
						delivered = true;
					}
				}
			}
		} while (chatMsg != nullptr);
		pingLastTimeChatMsgsRouted();
		Sleep(100);
	}

	
	setIsChatExchangeAlive(false);
	//process chat messages - END
}

void CNetworkManager::enableSyncTransport(eTransportType::eTransportType transport) {
	std::lock_guard <std::mutex> lock(mFieldsGuardian);

	if (std::find(mSyncTransports.begin(), mSyncTransports.end(), transport) == mSyncTransports.end()) {
		mSyncTransports.push_back(transport);
	}
}
void CNetworkManager::disableSyncTransport(eTransportType::eTransportType transport) {
	std::lock_guard <std::mutex> lock(mFieldsGuardian);

	auto it = std::find(mSyncTransports.begin(), mSyncTransports.end(), transport);
	if (it != mSyncTransports.end()) {
		mSyncTransports.erase(it);
	}
}
bool CNetworkManager::isTransportEnabledForSync(eTransportType::eTransportType transport) {
	std::lock_guard <std::mutex> lock(mFieldsGuardian);

	return std::find(mSyncTransports.begin(), mSyncTransports.end(), transport) != mSyncTransports.end();
}


void CNetworkManager::DSMSyncThreadF()
{
	pingLastTimeDSMControllerThreadRun();
	//Local Variables - BEGIN
	uint64_t lastScheduled = 0;;
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		lastScheduled = mLastTimeBlocksScheduled;
	}

	conversationFlags flags;
	flags.exchangeBlocks = true;

	//fetch currently active DSM connections
	std::vector<std::shared_ptr<CConversation>> conversations = getConversations(flags, true, false, true, true);// notice: this would fetch both QUIC and UDT connections.

	//now fetch what has been discovered through Kademlia; so that we know with WHOM we could connect with..
	std::shared_ptr<CKademliaServer> kademlia = getKademliaServer();
	std::vector<std::shared_ptr<CEndPoint>>  peers;

	bool lastSpecificReport = getLastNetworkSpecifcsReported();
	bool canReportSpecifics = (std::time(0) - lastSpecificReport) > 30 ? true : false;

	if (canReportSpecifics)
	{
		pingLastNetworkSpecifcsReported();
	}

	if(kademlia)
	 peers = getKademliaServer()->getPeers();
	bool logDebug = getBlockchainManager()->getSettings()->getMinEventPriorityForConsole() == 0;
	bool connectedToBootstrapNode = false;
	bool canLogAboutIP = true;
	bool isInFocusedMode = getIsFocused();
	static uint64_t mLastOnlyBootstrapConnectionWarning = 0;
	bool isTestMode = getIsNetworkTestMode();
	uint64_t activeSyncConnections = 0;
	static uint64_t desiredCountOfConns = CGlobalSecSettings::getTargetUDTConnectionsCount();
	std::vector <std::vector<uint8_t>> alreadyConnectedWithIPs, alreadyConnectingWithIPs, triedRecently;
	std::shared_ptr<CConversation> conv;
	std::string addr;
	uint64_t advertiseMemPoolEvery = 10;
	uint64_t now = std::time(0);
	bool reportConnectivity = false;
	bool usingForcedBootstrap = getHasForcedBootstrapNodesAvailable();
	std::shared_ptr<CEndPoint> bNode =  getBootstrapNode(!usingForcedBootstrap);// do not toss a new Tier 0 node if Operator forced an explicit node of his choosing.
	std::vector<uint8_t> bNodeAddress;

	if (bNode)
	{
		bNodeAddress = bNode->getAddress();
	}
	else
	{
		mTools->writeLine(mTools->getColoredString("Empty bootstrap node.", eColor::alertError)+
			" Falling back to default:  " + mTools->getColoredString(CGlobalSecSettings::getDefaultBootstrapNode(), eColor::lightCyan));

		bNodeAddress = mTools->stringToBytes(CGlobalSecSettings::getDefaultBootstrapNode());

	}
	static std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	std::vector<std::tuple<std::vector<uint8_t>, std::shared_ptr<CBlockHeader>>> huntedBlocks;

	bm->getHuntedBlocks(huntedBlocks);
	std::vector<std::shared_ptr<CConversation>> dsmConvs;
	uint64_t fetchFrom = 0;
	static std::shared_ptr<CTools> tools = getTools();
	std::shared_ptr<CNetTask> task;
	static  std::string myself = getPublicIP();
	std::shared_ptr<CBlockHeader> header;
	std::shared_ptr<CChatMsg> chatMsg;
	static std::vector<uint8_t> myself2 = getCachedAutoDetectedPublicIP();//just in case
	std::string headerID;
	uint64_t lastTimeNotifiedAboutPendingBlocks = 0;
	//uint64_t lastConnectivityReport = 0;
	//Local Variables - END

	uint64_t lastDSMQualityReport = getLastDSMQualityReportTimestamp();
	uint64_t DSMQualityReportInterval = 20;
	std::vector<std::shared_ptr<CConversation>>::iterator peer = conversations.begin();
	bool doingDSMQualityReport = false;

	if ((now - lastDSMQualityReport) > DSMQualityReportInterval)
	{
		doingDSMQualityReport = true;
		pingDSMQualityReport();
	}

	uint64_t lastConnectivityReport = getLastConnectivityReportTimestamp();
	if ((now > lastConnectivityReport) && ((now - lastConnectivityReport) > 10))
	{
		pingLastConnectivityReportTimestamp();
		reportConnectivity = true;
	}

	while (peer != conversations.end()) {

		if ((*peer)->getState()->getCurrentState() == eConversationState::ended
			|| (*peer)->getState()->getCurrentState() == eConversationState::ending)
		{
			peer = conversations.erase(peer);
		}
		else {
			++peer;
		}
	}
	eConversationState::eConversationState cState = eConversationState::ended;
	//Ensure the desired count of DSM-sync connections - BEGIN
	//Disclaimer: we need to try to maintain the desired numerosity of DSM-synchronization links.



	for (uint64_t i = 0; i < conversations.size(); i++)
	{
		if (!connectedToBootstrapNode && getIsBootstrapNode(conversations[i]->getIPAddress()))
		{
			connectedToBootstrapNode = true;
		}

		cState = conversations[i]->getState()->getCurrentState();
		flags = conversations[i]->getFlags();
		if (cState == eConversationState::connecting
			|| cState == eConversationState::initial
			|| cState == eConversationState::initializing)
		{
			alreadyConnectingWithIPs.push_back(tools->stringToBytes(conversations[i]->getIPAddress()));
			continue;
		}

		if (cState == eConversationState::unableToConnect || cState == eConversationState::ended || cState == eConversationState::ending
			|| cState == eConversationState::bye)
		{
			triedRecently.push_back(tools->stringToBytes(conversations[i]->getIPAddress()));
			continue;
		}

		if (cState == eConversationState::running)
		{
			if (flags.exchangeBlocks)
			{
				alreadyConnectedWithIPs.push_back(tools->stringToBytes(conversations[i]->getIPAddress()));
				activeSyncConnections++;
			}
		}
	}

	// DSM Sync Connectivity Reporting - BEGIN
	if (activeSyncConnections >= desiredCountOfConns)
	{
		setIsConnectivityOptimal(true);

		if (reportConnectivity && canReportSpecifics)
		{
			tools->logEvent("Target sync connectivity reached.", "Synchronization", eLogEntryCategory::network, 1, eLogEntryType::notification, eColor::lightGreen);
		}
	}
	else
	{
		setIsConnectivityOptimal(false);
		if (reportConnectivity && canReportSpecifics)
		{
			lastConnectivityReport = now;
			tools->logEvent("Target sync connectivity not yet reached.", "Synchronization", eLogEntryCategory::network, 1, eLogEntryType::warning, eColor::lightPink);
		}
	}
	// DSM Sync Connectivity Reporting  - END

	std::vector<uint8_t> peerAddr;
	bool bootstrapNodeTried = false;
	std::shared_ptr<CEndPoint> alternativePeer;
	// Initial Infant-Nodes Anti-Eclipsing Measure - BEGIN

	bool connectOnlyWithBootstrapNodes = getMaintainConnectivityOnlyWithBootstrapNodes();


	if (connectOnlyWithBootstrapNodes)
	{
		peers = getBootstrapNodes();//override peers discovered through Kademlia
		if (!isInFocusedMode && getHasForcedBootstrapNodesAvailable())
		{
			if (!getIsFocused() && canReportSpecifics)
			{
				std::stringstream ss;
				if (!peers.empty())
				{
					for (uint64_t i = 0; i < peers.size(); i++)
					{
						ss << mTools->getColoredString(mTools->bytesToString(peers[i]->getAddress()), eColor::neonGreen);
						if (i != peers.size() - 1)
						{
							ss << ",";
						}
					}
				}
				else
				{
					ss << mTools->getColoredString("none", eColor::lightPink);
				}

				if (now - mLastOnlyBootstrapConnectionWarning > 10)
				{
					mLastOnlyBootstrapConnectionWarning = now;
					tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
						"Attempting to maintain connectivity only with following nodes: " + ss.str(),

						tools->getColoredString(tools->bytesToString(peers[0]->getAddress()), eColor::lightCyan), eLogEntryCategory::network, 1);
				}
			}
		}
		else {
			if (!getIsFocused() && canReportSpecifics)
			{
				if (now - mLastOnlyBootstrapConnectionWarning > 10)
				{
					mLastOnlyBootstrapConnectionWarning = now;
					tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
						"For now, attempting to maintain connectivity only with Bootstrap nodes..", eLogEntryCategory::network, 5);
				}
			}

		}

	}

	if (peers.empty())
	{
		if (canReportSpecifics)
		{
			tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
				"No peers known yet. Falling back to bootstrap nodes.", eLogEntryCategory::network, 5);
		}
		peers = getBootstrapNodes();
	}


	// Initial Infant-Nodes Anti-Eclipsing Measure - END



	// Maintain Desired Connectivity Ratio - BEGIN
	if ((now - getLastConnectivityImprovement()) > 10)
	{
		pingLastConnectivityImprovement();
		bool hasSyncProtocolAvailable = false;
		hasSyncProtocolAvailable = (isTransportEnabledForSync(eTransportType::UDT) || isTransportEnabledForSync(eTransportType::QUIC));

		if (!hasSyncProtocolAvailable)//do we have any synchronization protocol enabled?
		{
			tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) + "No synchronization protocol available.",
				eLogEntryCategory::localSystem, 1, eLogEntryType::warning);

		}
		else
		{

			for (uint64_t i = 0; i < peers.size() && //index cannot exceed container's dimensions.
				((activeSyncConnections < desiredCountOfConns) || // connections' arity is not yet optimal.
					(bNodeAddress.empty() == false && !connectedToBootstrapNode && !bootstrapNodeTried)); i++)//we are not connected to any Tier 0 node yet.
			{

				if (bootstrapNodeTried)
				{
					alternativePeer = nullptr;
				}
				if ((!((i < peers.size() && activeSyncConnections < desiredCountOfConns))) &&
					(!connectedToBootstrapNode && !bootstrapNodeTried))
				{
					peerAddr = bNodeAddress;
					bootstrapNodeTried = true;
					alternativePeer = std::make_shared<CEndPoint>(peerAddr, eEndpointType::IPv4, nullptr, CGlobalSecSettings::getDefaultPortNrDataExchange(getBlockchainMode()));
				}
				else
				{//every other case in which we're not trying the bootstrap node
					peerAddr = peers[i]->getAddress();
				}

				if (isTestMode)
				{
					canLogAboutIP = canEventLogAboutIP(tools->bytesToString(peerAddr));
				}
				if (isIPAddressLocal(peerAddr))
				{
					// it likely is the localhost.
					continue;
				}

				// looks like we can try to improve the current situation..
				if (tools->checkVectorContained(peerAddr, alreadyConnectedWithIPs))
				{//not already connected with
					if (canLogAboutIP)
						tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
							" I won't try " + tools->getColoredString(addr, eColor::lightGreen) + " - already connected...", eLogEntryCategory::network, 0);
					continue;
				}

				if (tools->checkVectorContained(peerAddr, triedRecently))
				{
					if (canLogAboutIP)
						tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
							" I won't try " + tools->getColoredString(addr, eColor::lightGreen) + " since I've tried recently.", eLogEntryCategory::network, 0);
					continue;
				}

				if (tools->checkVectorContained(peerAddr, alreadyConnectingWithIPs))
				{
					if (canLogAboutIP)
						tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
							" I won't try " + tools->getColoredString(addr, eColor::lightGreen) + " since I'm already trying..", eLogEntryCategory::network, 0);
					continue;
				}
				addr = tools->bytesToString(peerAddr);

				if (tools->compareByteVectors(addr, myself2))
				{
					continue;//do not try connect with myself.
				}

				// prevent node overuse - begin


				eTransportType::eTransportType targetProtocol = eTransportType::QUIC;
				bool targetProtocolFound = false;

				if (isTransportEnabledForSync(targetProtocol))
				{
					if (!canAttemptConnection(addr, targetProtocol)) {
						if (logDebug && canLogAboutIP)
							tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
								"Tried recently. Won't try " + tools->transportTypeToString(targetProtocol) + " with " + tools->getColoredString(addr, eColor::blue) + ".", eLogEntryCategory::network, 1);
					}
					else
					{
						targetProtocolFound = true;
					}
				}

				if (!targetProtocolFound && isTransportEnabledForSync(eTransportType::UDT))
				{	// now try UDT
					targetProtocol = eTransportType::UDT;

					if (!canAttemptConnection(addr, targetProtocol)) {

						targetProtocolFound = false;
						if (logDebug && canLogAboutIP)
							tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
								"Tried recently. Won't try " + tools->transportTypeToString(targetProtocol) + " with " + tools->getColoredString(addr, eColor::blue) + ".", eLogEntryCategory::network, 1);

					}
					else
					{
						targetProtocolFound = true;
					}
				}

				if (!targetProtocolFound)
				{
					if (logDebug && canLogAboutIP)
						tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
							"Tried recently. Won't try any protocol with " + tools->getColoredString(addr, eColor::lightGreen) + ".", eLogEntryCategory::network, 1);
				}

				if (targetProtocolFound)
				{
					connectNode(alternativePeer ? alternativePeer : peers[i], targetProtocol);
						activeSyncConnections++;
				}
			}
		}
	}
	// Maintain Desired Connectivity Ratio - END
	
	//in any case - always try to maintain connection with at least ones of the bootstrap nodes  - BEGIN

	//in any case - always try to maintain connection with at least ones of the bootstrap nodes  - END
	//Ensure the desired count of DSM-sync connections - END

	//Process Block-Header Outgress notifications - BEGIN
	//these notifications are enqueued whenever Blockchain Manager decided to append a block which was enqueued to it for processing.
	std::vector<uint8_t> temp;
	do
	{
		header = dequeHeaderNotification();
	
		if (header && header->getPackedData(temp))
		{
			headerID = tools->getColoredString(tools->base58CheckEncode(header->getHash()), eColor::orange);

			for (uint64_t i = 0; i < dsmConvs.size(); i++)
			{
				if (dsmConvs[i]->getIsEncryptionAvailable() == false)
				{
					continue;
				}

				if (isTestMode)
				{
					canLogAboutIP = canEventLogAboutIP(dsmConvs[i]->getIPAddress());
				}
				task = std::make_shared<CNetTask>(eNetTaskType::notifyBlock);
				task->setData(temp);
				if(canLogAboutIP)
				tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
					"notifying peer " + dsmConvs[i]->getIPAddress() + " about block " + headerID, eLogEntryCategory::network, 1);

				dsmConvs[i]->addTask(task);
			}
		}
	} while (header != nullptr);


	//Process Block-Header Outgress notifications - END
	
	//Distribute Block Download Tasks - BEGIN
	//these can be created only as a result of the Heaviest-Chain-Proof's validation.

	
	bool scheduleDownloads = true;
	if ((now - lastScheduled) < 5)
	{
		scheduleDownloads = false;
	}
	bool notifyAboutPendingBlocks = true;
	bool favorableNodeFound = false;
	if ((std::time(0) - lastTimeNotifiedAboutPendingBlocks)<15)
	{
		tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
			" active connections:" + std::to_string(dsmConvs.size()), eLogEntryCategory::network, 1, eLogEntryType::notification, eColor::orange);

		notifyAboutPendingBlocks = false;
	}
	uint64_t bql = 0;
	bm->getBlockQueueLength(bql, false);
	if (scheduleDownloads == false || bql >= CGlobalSecSettings::getSceduleDownloadsTillBlockQueueLength())
	{
		if (notifyAboutPendingBlocks)
		{
			tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
				" holding on with further downloads..", eLogEntryCategory::network, 1, eLogEntryType::notification, eColor::orange);
		}
	}
	else if(huntedBlocks.size()){
		//if (notifyAboutPendingBlocks)
		//{

			tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
				"scheduling further downloads now..", eLogEntryCategory::network, 0, eLogEntryType::notification, eColor::orange);
		//}
			{
				std::lock_guard<std::mutex> lock(mFieldsGuardian);
				mLastTimeBlocksScheduled = std::time(0);
			}
			uint64_t convsBeforeEliminationCount = dsmConvs.size();
		//eliminate conversations that haven't notified about leading block -  BEGIN
			std::vector<std::shared_ptr<CConversation>>::iterator it = dsmConvs.begin();
			while (it != dsmConvs.end())
			{
				if ((*it)->getLeadBlockID().size() != 32)
				{
					it = dsmConvs.erase(it);
				}
				if (it != dsmConvs.end())
				{
					++it;
				}
			}
		//eliminate conversations that haven't notified about leading block -  END

			if (doingDSMQualityReport)
			{
				
				tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
					std::to_string(dsmConvs.size()) + " nodes out of active " + std::to_string(convsBeforeEliminationCount) +
					" notified about leading blocks.", eLogEntryCategory::network, 1);
			}

		flags = conversationFlags();
		flags.exchangeBlocks = true;
		dsmConvs = getConversations(flags);
			
		if (!dsmConvs.empty())
		{
		
			for (uint64_t i = 0; i < huntedBlocks.size(); i++)
			{
				if (isBlockCurrentlyBeingFetched(std::get<0>(huntedBlocks[i])))
				{
					if (notifyAboutPendingBlocks)
					{
						tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
							" block " + tools->base58CheckEncode(std::get<0>(huntedBlocks[i])) + " is already being fetched. Waiting..", eLogEntryCategory::network, 0);
					}
					continue;
				}


				///first go through all the nodes and see which nodes SHOULD have the block available
				favorableNodeFound = false;
				uint64_t startPoint = tools->genRandomNumber(0, dsmConvs.size() - 1);
				bool firstRun = true;
				eBlockAvailabilityConfidence::eBlockAvailabilityConfidence requiredAvailabilityConfidence = eBlockAvailabilityConfidence::available;

			// Loop - BEGIN
			tryToFindNode:

				firstRun = true;
				for (uint64_t x = startPoint; !(firstRun==false && x== startPoint);x=((x+1) % dsmConvs.size()))// the main logic here is to check if the loop wrapped around
				{//while starting at a random point.
					
					//that is to eliminate round trips between nodes, it is first checked if the other node shalt have the block available,
					//based on data available locally.
					if (dsmConvs[x]->hasBlockAvailable(huntedBlocks[i]) >= requiredAvailabilityConfidence)
					{
						tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
							" node " + dsmConvs[fetchFrom]->getIPAddress()+" is expected to have block " + tools->base58CheckEncode(std::get<0>(huntedBlocks[i])) + " available..", eLogEntryCategory::network, 1, eLogEntryType::notification);
						favorableNodeFound = true;
						fetchFrom = x;
						break;
					}
					firstRun = false;
				}

				if (!favorableNodeFound && requiredAvailabilityConfidence == eBlockAvailabilityConfidence::available)
				{
					// lower required confidence level
					requiredAvailabilityConfidence = eBlockAvailabilityConfidence::possiblyAvailable;
					goto tryToFindNode;
				}
			// Loop - END

				//if no node supposedly holds the desired block, pick a random node.
				//[UPDATE]: do NOT pick a random node. Now, each conversation keeps a dedicated copy of a full chain-proof reconstructed based on data delivered from a remote peer.
				// we schedule downloads *ONLY* from nodes that broadcast a valid current Verified Chain Proof.
				/*
				* 1) the advertised chain-proof needs to be compatible with the latest obligatory Checkpoint.
				* 2) the looked for block - it needs to be presented within the history of events proclaimed by the remote peer.
				*							We know the history of events proclaimed by a remote peer based on the advertised (partial) chain-proofs.
				* 
				* If any of the above is not met - we would NOT schedule downloads from the particular node.
				*/
				if (!favorableNodeFound)
				{

					tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
						" no active neighboor has block " + tools->base58CheckEncode(std::get<0>(huntedBlocks[i]))+":"+ tools->getColoredString(std::to_string(std::get<1>(huntedBlocks[i])->getHeight()), eColor::blue) + ". Postponing download..", eLogEntryCategory::network, 1, eLogEntryType::warning, eColor::lightPink);

					/*[DEPRECATED - SECURITY - network protection]tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
						" no known node is expected to have block " + tools->base58CheckEncode(std::get<0>(huntedBlocks[i])) + " available, picking a random node..", eLogEntryCategory::network, 1, eLogEntryType::warning, eColor::lightPink);
					fetchFrom = tools->genRandomNumber(0, dsmConvs.size() - 1);*/
				}
				else
				{

					tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
						" Requesting block " + tools->base58CheckEncode(std::get<0>(huntedBlocks[i])) +"@" + std::to_string(std::get<1>(huntedBlocks[i])->getHeight()) +tools->getColoredString("@", eColor::lightCyan) + tools->getColoredString(dsmConvs[fetchFrom]->getIPAddress(), eColor::orange), eLogEntryCategory::network, 1);

					if (!dsmConvs[fetchFrom]->requestBlock(std::get<0>(huntedBlocks[i])))
					{
						tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
							" failure when trying to issue a block request to " + dsmConvs[fetchFrom]->getIPAddress(), eLogEntryCategory::network, 10, eLogEntryType::failure, eColor::cyborgBlood);
					}
				}

			}
		}
	}
	//Distribute Block Download Tasks - END


	//Advertise Contents of Mem-Pool to other nodes - BEGIN
	std::shared_ptr<CTransactionManager> tm = bm->getFormationFlowManager();//received TXs make (or have made) their way into the Formation Flow Manager.
	uint64_t lastTimeMemPoolAdvertised = tm->getLastTimeMemPoolAdvertised();
	now = std::time(0);
	
	if ((now - lastTimeMemPoolAdvertised) > advertiseMemPoolEvery)
	{
		tm->advertiseMemPoolContents();
	}
	//Advertise Contents of Mem-Pool to other nodes - END
	

	//the assumption is that each connection will be taking care of all the 'synchronization-talk'.
	/* all by itself. Yet still, the Network Manager:
	* 1) chooses which node to ask for a particular Block - by dispatching an appropriate task to it.
	* We do not want to query multiple nodes for the same block at the same time.
	* 2) dispatches a global task of notifying peers about the current leader which has been produced locally. To all connected peers.
	* 3) decides upon the current checkpoints that are to be used for formulation of Chain-Proof requests.
	* 
	* [Connection phases]:
	* 1) Once a connection is established node sends the current leader (block header).
	* 2) Once that header is received, node MAY decide to ask for a chain-proof.
	* 3) The received chain-proof is analyzed and it MAY replace the locally best known.
	*/


}

bool CNetworkManager::isBlockCurrentlyBeingFetched(std::vector<uint8_t>& blockID)
{
	std::shared_ptr<CUDTConversationsServer> udtServer = getUDTServer();
	if (!udtServer)
		return false;
	std::vector<std::shared_ptr<CConversation>> convs = udtServer->getAllConversations();

	for (uint64_t i = 0; i < convs.size(); i++)
	{
		if (convs[i]->getState()->getCurrentState() == eConversationState::running)
		{
			if (convs[i]->isRequestForBlockPending(blockID))
				return true;
		}
	}
	return false;
}


void CNetworkManager::addLocalIPAddr(std::string ipAddr)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mLocalIPPoolGuardian);

	tools->writeLine("Added " + tools->getColoredString(ipAddr, eColor::lightCyan) + " to the pool of local IP addresses.");
	mLocalIPAddresses.push_back(ipAddr);
}

std::shared_ptr<CChatMsg> CNetworkManager::dequeChatMsgNotification()
{

	std::lock_guard<std::mutex> lock(mChatNotificationQueueGuardian);

	std::shared_ptr<CChatMsg>toRet;

	if (mChatNotificationQueue.empty())
		return nullptr;

	toRet = mChatNotificationQueue.front();
	mChatNotificationQueue.pop();
	return toRet;

}
bool CNetworkManager::connectNode(const std::string &IP, eTransportType::eTransportType transportType)
{
	std::shared_ptr<CEndPoint> endpoint = std::make_shared<CEndPoint>(getTools()->stringToBytes(IP), eEndpointType::IPv4, nullptr,
		CGlobalSecSettings::getDefaultPortNrDataExchange(getBlockchainMode()));
	return connectNode(endpoint, transportType);
}

bool CNetworkManager::connectNode(std::shared_ptr<CEndPoint> endpoint, eTransportType::eTransportType transportType)
{

	if (!endpoint)
		return false;
	std::shared_ptr<CTools> tools = getTools();
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	std::string ipStr = tools->bytesToString(endpoint->getAddress());

	bool logDebug = getBlockchainManager()->getSettings()->getMinEventPriorityForConsole() == 0;
	bool canLogAboutIP = canEventLogAboutIP(tools->bytesToString(endpoint->getAddress()));
	//prevent node overuse - end
	if (logDebug && canLogAboutIP)
		tools->logEvent(tools->getColoredString("[DSM-sync]: ", eColor::lightCyan) +
			"establishing " + tools->transportTypeToString(transportType) + " connection with: " + tools->getColoredString(ipStr, eColor::lightGreen), eLogEntryCategory::network, 3);

	std::shared_ptr<CConversation> conv = std::make_shared<CConversation>(0, mBlockchainManager, shared_from_this(), endpoint);//alternativePeer only in the case of a bootstrap node
	conv->initState(conv);
	conv->setIsAuthenticationRequired(false);
	conv->setIsEncryptionRequired();

	//conversationFlags flags = conversationFlags();

	//flags.exchangeBlocks = bm->getSyncMachine();

	storeConnectionAttempt(ipStr, transportType);// protocol invariant


	//initialize the conversation only if accepted by the UDT sub-system.
	//conv->setFlags(flags);

	if (isTransportEnabledForSync(eTransportType::UDT) && transportType == eTransportType::UDT)
	{
		if (mUDTServer->addConversation(conv))
		{
			if (!conv->startUDTConversation(getThreadPool(), 0, ipStr))
			{
				conv->end(false, true);
			}
			else
			{
				return true;
			}
			
		}
	}
	else if (isTransportEnabledForSync(eTransportType::QUIC) && transportType == eTransportType::QUIC)
	{
		std::shared_ptr<CQUICConversationsServer> server = getQUICServer();

		if (server && server->addConversation(conv))
		{
			if (!conv->startQUICConversation(getThreadPool(), 0, ipStr))
			{
				conv->end(false, true);
				
			}
			else
			{
				return true;
			}
			
		}
	}

	return false;
}

std::shared_ptr<CChatMsg> CNetworkManager::getHeadChatMsgNotification()
{

	std::lock_guard<std::mutex> lock(mChatNotificationQueueGuardian);

	std::shared_ptr<CChatMsg>toRet;

	if (mChatNotificationQueue.empty())
		return nullptr;

	toRet = mChatNotificationQueue.front();
	return toRet;

}

bool CNetworkManager::enqueChatMsgNotification(std::shared_ptr<CChatMsg> msg)
{
	if (!msg)
		return false;

	std::shared_ptr<CTools> tools = getTools();

	std::lock_guard<std::mutex> lock(mChatNotificationQueueGuardian);

	tools->logEvent(tools->getColoredString("[Chat]:", eColor::lightCyan) + " enqueing network notification about a chat msg.", eLogEntryCategory::network, 0);

	mChatNotificationQueue.push(msg);

	return true;
}


std::shared_ptr<CBlockHeader> CNetworkManager::dequeHeaderNotification()
{

	std::lock_guard<std::mutex> lock(mHeaderNotificationsQueueGuardian);

	std::shared_ptr<CBlockHeader>toRet;

	if (mHeaderNotificationsQueue.empty())
		return nullptr;
	
	toRet = mHeaderNotificationsQueue.front();
	mHeaderNotificationsQueue.pop();
	return toRet;

}

bool CNetworkManager::enqueHeaderNotification(std::shared_ptr<CBlockHeader> header)
{
	if (!header)
		return false;

	std::shared_ptr<CTools> tools = getTools();

	std::lock_guard<std::mutex> lock(mHeaderNotificationsQueueGuardian);

	tools->logEvent(tools->getColoredString("[DSM Sync]:", eColor::lightCyan)+ " enqueueing network notification about block: " + tools->base58CheckEncode(header->getHash()), eLogEntryCategory::network, 1);

		mHeaderNotificationsQueue.push(header);

	return true;
}
void CNetworkManager::forceSyncCheckpointsReferesh()
{
	std::shared_ptr<CTools> tools = getTools();
	tools->logEvent(tools->getColoredString("[DSM Sync]:", eColor::lightCyan) + " invalidating local Synchronisation Points.", eLogEntryCategory::localSystem, 1);

	std::lock_guard<std::mutex> lock(mCheckpointsGuardian);
	mCheckpointsRefreshedTimestamp = 0;
}
/**
 * @brief Generates and returns the set of synchronization checkpoints for chain‐proof exchange.
 *
 * This method constructs a list of checkpoint block IDs that are used during DSM synchronization
 * to allow remote peers to generate sub‐chain proofs starting from one of these known synchronization points.
 * Checkpoints are derived from the local node's current heaviest chain (using key block heights) and are
 * generated in two phases:
 *
 * 1. Cached Checkpoints:
 *    - If a previously generated set of checkpoints (cached in mCheckpoints) exists and is still fresh (i.e.,
 *      refreshed within the last 180 seconds), the method uses that cached set.
 *    - Before returning the cached list, it ensures that the latest key block (i.e., the block at depth 0)
 *      is included as the first checkpoint. If the cached list’s first element does not match the latest key block,
 *      it inserts the latest key block at the beginning.
 *
 * 2. Checkpoint Refresh:
 *    - If no cached checkpoints exist or they have expired, the method refreshes the cache by clearing mCheckpoints.
 *    - It always includes fixed checkpoints at depths 0, 10, and 100 (when available, based on the current heaviest key height)
 *      and then computes additional intermediate checkpoints using a fragmentation scheme (with a fragmentation factor of 200)
 *      over the range from 0 to the current heaviest key height.
 *    - If any computed checkpoint is empty or duplicates the previous checkpoint, the generation fails, the cache is cleared,
 *      and the method returns false.
 *
 * The generated checkpoints are stored in mCheckpoints (protected by mCheckpointsGuardian) and then copied
 * into the provided output parameter. The resulting list is ordered from the newest checkpoint (index 0) downward.
 *
 * @param[out] checkpoints A reference to a vector that will be populated with the synchronization checkpoint block IDs.
 *                         Each checkpoint is represented as a vector of uint8_t (typically a block hash).
 *
 * @return true if the synchronization checkpoints were successfully generated and at least one checkpoint is available;
 *         false if generation fails (e.g., due to missing key leader information, an empty blockchain, or failure
 *         in generating a valid intermediate checkpoint).
 *
 * @note This method logs events at various stages (including debug messages if enabled) and uses the cached
 *       checkpoint list (mCheckpoints) when possible to improve performance. The “latest key block” adjustment
 *       (inserting the checkpoint at depth 0 if not already present) ensures that even when using cached checkpoints,
 *       the most recent key block is always offered as the optimal synchronization point.
 *
 * @see CBlockchainManager::getBlockIDAtKeyDepth, mCheckpoints, mCheckpointsRefreshedTimestamp,
 *      CBlockchainManager::getHeaviestChainProofKeyLeader, CBlockchainManager::getCachedLeader
 */
bool CNetworkManager::getCPCheckpoints(std::vector<std::vector<uint8_t>>& checkpoints)
{
	// --------------------------------------------------------------
	// Local Variables - BEGIN
	// --------------------------------------------------------------
	bool logDebug = getBlockchainManager()->getSettings()->getMinEventPriorityForConsole() == 0;
	bool refreshNeeded = false;
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();

	// Heaviest key leader in the chain (used for depth-based queries)
	std::shared_ptr<CBlockHeader> heaviestKeyLeader = bm->getHeaviestChainProofKeyLeader();
	// Possibly the latest “cached” leader in memory
	std::shared_ptr<CBlock> kl = bm->getCachedLeader(true);
	std::shared_ptr<CBlockHeader> keyLeader = kl ? kl->getHeader() : nullptr;

	std::shared_ptr<CTools> tools = getTools();
	std::vector<uint8_t> id;
	std::vector<uint8_t> previousBlockID;
	// --------------------------------------------------------------
	// Local Variables - END
	// --------------------------------------------------------------

	// --------------------------------------------------------------
	// Pre-Flight Checks - BEGIN
	// --------------------------------------------------------------
	if (!heaviestKeyLeader || !keyLeader)
	{
		tools->logEvent(
			tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
			+ "Sync-Point Generation failed - missing heaviest Key Leader.",
			eLogEntryCategory::network, 1, eLogEntryType::notification
		);
		return false;
	}

	uint64_t currentHeaviestKeyHeight = heaviestKeyLeader->getKeyHeight();
	uint64_t currentKeyHeight = keyLeader->getKeyHeight();

	if (currentHeaviestKeyHeight == 0)
	{
		tools->logEvent(
			tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
			+ "Sync-Point Generation failed - currentHeaviestKeyHeight is 0 (blockchain is empty).",
			eLogEntryCategory::network, 1, eLogEntryType::warning
		);
		return false;
	}
	// --------------------------------------------------------------
	// Pre-Flight Checks - END
	// --------------------------------------------------------------

	// --------------------------------------------------------------
	// Start Lock Scope
	// --------------------------------------------------------------
	{
		std::lock_guard<std::mutex> lock(mCheckpointsGuardian);

		// Decide if we can use cached checkpoints or if we need to refresh
		if (mCheckpoints.empty() ||
			(!mCheckpoints.empty() && (std::time(0) - mCheckpointsRefreshedTimestamp) > 180))
		{
			refreshNeeded = true;
		}
		else
		{
			// We have non-empty, fresh checkpoints
			checkpoints = mCheckpoints;

			// --------------------------------------------------------------
			// Ensure the latest key block is always at index 0
			// --------------------------------------------------------------
			std::vector<uint8_t> latest = bm->getBlockIDAtKeyDepth(0, eChainProof::heaviest);
			if (!latest.empty() && !checkpoints.empty() && !tools->compareByteVectors(checkpoints[0], latest))
			{
				checkpoints.insert(checkpoints.begin(), latest);
			}

			tools->logEvent(
				tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
				+ "Sync-Point Generation: using cached checkpoints.",
				eLogEntryCategory::network, 1, eLogEntryType::notification
			);
			return true;
		}

		// --------------------------------------------------------------
		// If we do need to refresh, clear out old checkpoints
		// --------------------------------------------------------------
		if (refreshNeeded)
		{
			tools->logEvent(
				tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
				+ "Sync-Point Generation: refreshing checkpoints (clearing cache).",
				eLogEntryCategory::network, 1, eLogEntryType::notification
			);
			mCheckpoints.clear();
		}

		// --------------------------------------------------------------
		// Generate Fresh Checkpoints
		// --------------------------------------------------------------
		uint64_t fragmentation = 200;

		// Always include fixed checkpoints (0, 10, 100) if available
		std::vector<uint64_t> fixedDepths;
		if (currentHeaviestKeyHeight >= 100)
		{
			fixedDepths = { 0, 10, 100 };
		}
		else if (currentHeaviestKeyHeight >= 10)
		{
			fixedDepths = { 0, 10 };
		}
		else
		{
			fixedDepths = { 0 };
		}

		// --------------------------------------------------------------
		// Add the fixed checkpoints
		// --------------------------------------------------------------
		for (uint64_t depth : fixedDepths)
		{
			//std::shared_ptr<CBlock> block = bm->getBlockAtDepth(depth, eChainProof::heaviestCached);
			//assertGN(block->getHeader()->getKeyHeight() ==  bm->getHeaviestChainProofLeader()->getKeyHeight() - depth);

			id = bm->getBlockIDAtKeyDepth(depth, eChainProof::heaviest);

			if (id.empty())
			{
				// If the “fixed” depth is empty, we can choose to fail or just log a warning.
				// Here, we only log a warning and continue (so that at least some checkpoints might succeed).
				if (logDebug)
				{
					tools->logEvent(
						tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
						+ "Warning: fixed checkpoint at depth " + std::to_string(depth) + " is empty.",
						eLogEntryCategory::network, 0, eLogEntryType::warning
					);
				}
				continue;
			}

			// Also avoid duplicates among fixed checkpoints
			if (!previousBlockID.empty() && tools->compareByteVectors(id, previousBlockID))
			{
				if (logDebug)
				{
					tools->logEvent(
						tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
						+ "Warning: fixed checkpoint at depth " + std::to_string(depth) + " duplicates previous checkpoint. Skipping.",
						eLogEntryCategory::network, 0, eLogEntryType::warning
					);
				}
				continue;
			}

			if (logDebug)
			{
				tools->logEvent(
					tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
					+ "Adding fixed checkpoint at depth " + std::to_string(depth) + ".",
					eLogEntryCategory::network, 0, eLogEntryType::notification
				);
			}

			mCheckpoints.push_back(id);
			previousBlockID = id;
		}

		// --------------------------------------------------------------
		// Generate additional intermediate checkpoints
		// --------------------------------------------------------------
		//
		// NOTE: If you want the loop to include or nearly include the top-most key block,
		// you might run `i` to `fragmentation` instead of `(fragmentation - 2)`,
		// or otherwise adjust accordingly. It's mostly design preference.
		//
		for (uint64_t i = 1; i <= fragmentation; i++)
		{
			// The formula below linearly interpolates from 0..currentHeaviestKeyHeight,
			// ignoring the top-most (depth=0) if i=0 is skipped. If we want a slightly
			// more "exponential" distribution, we could do something like:
			//    computedDepth = (uint64_t)(std::pow( (double)i / (double)fragmentation, 2.0 ) * currentHeaviestKeyHeight );
			//
			uint64_t computedDepth = static_cast<uint64_t>(
				((double)i / (double)fragmentation) * (double)currentHeaviestKeyHeight
				);

			// If i == fragmentation, computedDepth might == currentHeaviestKeyHeight
			// which is actually an out-of-range block, since the maximum key depth is (height - 1).
			// So we guard below:
			if (computedDepth >= currentHeaviestKeyHeight)
			{
				break;
			}

			id = bm->getBlockIDAtKeyDepth(computedDepth, eChainProof::heaviest);
			if (id.empty())
			{
				// Clear partial data to avoid leaving the cache in half-refreshed state.
				mCheckpoints.clear();
				tools->logEvent(
					tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
					+ "Sync-Point Generation failed - checkpoint at depth "
					+ std::to_string(computedDepth) + " is empty. Clearing partial cache.",
					eLogEntryCategory::network, 1, eLogEntryType::warning
				);
				return false;
			}

			if (tools->compareByteVectors(id, previousBlockID))
			{
				// Duplicate of the previous checkpoint. Consider it a failure if you do not want duplicates.
				mCheckpoints.clear();
				tools->logEvent(
					tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
					+ "Sync-Point Generation failed - checkpoint at depth "
					+ std::to_string(computedDepth) + " duplicates previous checkpoint. Clearing partial cache.",
					eLogEntryCategory::network, 1, eLogEntryType::warning
				);
				return false;
			}

			if (logDebug)
			{
				tools->logEvent(
					tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
					+ "Adding fragmented checkpoint at depth " + std::to_string(computedDepth) + ".",
					eLogEntryCategory::network, 0, eLogEntryType::notification
				);
			}

			mCheckpoints.push_back(id);
			previousBlockID = id;
		}

		mCheckpointsRefreshedTimestamp = std::time(0);

		tools->logEvent(
			tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
			+ "Sync-Point Generation: successfully refreshed checkpoints. Total checkpoints: "
			+ std::to_string(mCheckpoints.size()) + ".",
			eLogEntryCategory::network, 1, eLogEntryType::notification
		);
	}
	// --------------------------------------------------------------
	// Lock is released automatically here by lock_guard destructor
	// --------------------------------------------------------------

	// --------------------------------------------------------------
	// Final Check and Return
	// --------------------------------------------------------------
	if (mCheckpoints.empty())
	{
		tools->logEvent(
			tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
			+ "Sync-Point Generation failed - mCheckpoints is empty after processing.",
			eLogEntryCategory::network, 1, eLogEntryType::warning
		);
		return false;
	}

	checkpoints = mCheckpoints;

	tools->logEvent(
		tools->getColoredString("[DSM Sync]: ", eColor::lightCyan)
		+ "Sync-Point Generation: returning " + std::to_string(checkpoints.size()) + " checkpoints.",
		eLogEntryCategory::network, 1, eLogEntryType::notification
	);

	if (checkpoints.empty())
	{
		// Should be unreachable given the prior check,
		// but we keep it for defensive programming.
		return false;
	}

	return true;
}



std::shared_ptr<CKademliaServer> CNetworkManager::getKademliaServer()
{
	std::lock_guard<std::mutex> lock(mKademliaGuardian);
	return mKademliaServer;
}

std::vector<std::shared_ptr<CConversation>> CNetworkManager::getAllConversations(bool includeWeb, bool includeUDT, bool includeQUIC, bool onlyAlive)
{
	std::vector<std::shared_ptr<CConversation>> toRet;
	std::vector<std::shared_ptr<CConversation>>  temp;
	std::shared_ptr<CUDTConversationsServer>  udtServer = getUDTServer();
	std::shared_ptr<CWebSocketsServer> websocketServer = getWebSocketsServer();
	std::shared_ptr<CQUICConversationsServer> quicServer = getQUICServer();

	if (includeWeb && websocketServer)
	{
		temp = websocketServer->getAllConversations();

		for (uint64_t i = 0; i < temp.size(); i++)
		{
			if (onlyAlive && temp[i]->getState()->getCurrentState() != eConversationState::running)
				continue;

			toRet.push_back(temp[i]);
		}
	}

	if (includeUDT && udtServer)
	{
		temp = udtServer->getAllConversations();//that includes conversations with other full-nodes and mobile apps.
		
		for (uint64_t i = 0; i < temp.size(); i++)
		{
			if (onlyAlive && temp[i]->getState()->getCurrentState() != eConversationState::running)
				continue;

			toRet.push_back(temp[i]);
		}
	}

	if (includeQUIC && quicServer)
	{
		temp = quicServer->getAllConversations();//that includes conversations with other full-nodes and mobile apps.

		for (uint64_t i = 0; i < temp.size(); i++)
		{
			if (onlyAlive && temp[i]->getState()->getCurrentState() != eConversationState::running)
				continue;

			toRet.push_back(temp[i]);
		}
	}

	return toRet;//mConversations;
}

bool CNetworkManager::getPublicLinkStatus()
{	
	std::lock_guard<std::mutex> lock(mLinkStatusGuardian);
	return mPublicLinkEnabled;
}

void CNetworkManager::setPublicLinkStatus(bool value)
{
	std::lock_guard<std::mutex> lock(mLinkStatusGuardian);
	 mPublicLinkEnabled = value;
}

std::shared_ptr<CDataRouter> CNetworkManager::getRouter()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mRouter;
}

/**
 * @brief Get the description of network interfaces.
 *
 * This method provides detailed information about the network interfaces,
 * including loopback, public, and local interfaces. It uses colored strings
 * to indicate the status of the IP addresses.
 *
 * @param newLine The newline string to be used in the output.
 * @return A string containing the description of the network interfaces.
 */
std::string CNetworkManager::getInterfacesDescription(std::string newLine)
{
	std::shared_ptr<CTools> tools = getTools();
	std::string toRet;
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	std::shared_ptr<CSettings> settings = bm->getSettings();
	bool wasFromCS = false;

	std::string desiredPublicIP = settings->getPublicIPv4(wasFromCS);
	std::string desiredLocalIP = settings->getLocalIPv4(wasFromCS);
	std::string effectivePublicIP = getPublicIP();
	std::string effectiveLocalIP = getLocalIP();

	bool isPublicIPPending = (tools->iequals(desiredPublicIP, effectivePublicIP) == false);
	bool isLocalIPPending = (tools->iequals(desiredLocalIP, effectiveLocalIP) == false);

	// Loopback - BEGIN
	toRet += tools->getColoredString("lo:", eColor::lightCyan) + newLine +
		"flags = 73<UP, LOOPBACK, RUNNING>  mtu 65536" + newLine +
		"inet 127.0.0.1  netmask 255.0.0.0" + newLine +
		"inet6 ::1  prefixlen 128  scopeid 0x10<host>" + newLine +
		"loop  txqueuelen 1000  (Local Loopback)" + newLine +
		"RX bytes " + tools->getColoredString(std::to_string(getLoopbackRX()), eColor::blue) +
		" RX errors 0  dropped 0  overruns 0  frame 0" + newLine +
		"TX bytes " + tools->getColoredString(std::to_string(getLoopbackTX()), eColor::blue) +
		" TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0" + newLine;
	// Loopback - END

	toRet += newLine + newLine;

	// Eth0 - BEGIN - Public Interface
	toRet += tools->getColoredString("eth0:", eColor::lightCyan) + newLine +
		"flags = 4163<UP, BROADCAST, RUNNING, MULTICAST>  mtu 1500" + newLine +
		"inet " + tools->getColoredString(getPublicIP(), isPublicIPPending == false ? eColor::lightCyan : eColor::lightPink) +
		(isPublicIPPending ? (" (" + tools->getColoredString(desiredPublicIP + " pending", eColor::orange) + ")") : (" " + tools->getColoredString("active", eColor::lightGreen))) +
		" netmask 255.255.255.0  broadcast [Kademlia]" + newLine +
		"RX bytes " + tools->getColoredString(std::to_string(getEth0RX()), eColor::blue) +
		" RX errors 0  dropped 0  overruns 0  frame 0" + newLine +
		"TX bytes " + tools->getColoredString(std::to_string(getEth0TX()), eColor::blue) +
		" TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0" + newLine;
	// Eth0 - END

	toRet += newLine + newLine;

	// Eth1 - BEGIN - Local Network Interface
	toRet += tools->getColoredString("eth1:", eColor::lightCyan) + newLine +
		"flags = 4163<UP, BROADCAST, RUNNING, MULTICAST>  mtu 1500" + newLine +
		"inet " + tools->getColoredString(getLocalIP(), isLocalIPPending == false ? eColor::lightCyan : eColor::lightPink) +
		(isLocalIPPending ? (" (" + tools->getColoredString(desiredLocalIP + " pending", eColor::orange) + ")") : (" " + tools->getColoredString("active", eColor::lightGreen))) +
		" netmask 255.255.255.0  broadcast [disabled]" + newLine +
		"RX bytes " + tools->getColoredString(std::to_string(getEth0RX()), eColor::blue) +
		" RX errors 0  dropped 0  overruns 0  frame 0" + newLine +
		"TX bytes " + tools->getColoredString(std::to_string(getEth0TX()), eColor::blue) +
		" TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0" + newLine;
	// Eth1 - END

	return toRet;
}

uint64_t CNetworkManager::getRouterCleanupInterval()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mRouterCleanUpInterval;
}

void CNetworkManager::setRouterCleanupInterval(uint64_t interval)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mRouterCleanUpInterval = interval;
}

uint64_t CNetworkManager::getLastRouterCleanup()
{
	std::lock_guard<std::mutex> lock(mLocalIDGuardian);
	return mLastRouterCleanup;
}

void CNetworkManager::setLastRouterCleanup(uint64_t timestamp)
{
	std::lock_guard<std::mutex> lock(mLocalIDGuardian);
	mLastRouterCleanup = timestamp;
}

std::vector<uint8_t> CNetworkManager::getLocalID()
{
	std::lock_guard<std::mutex> lock(mLocalIDGuardian);
	return mLocalID;
}

std::shared_ptr<CBlockchainManager> CNetworkManager::getBlockchainManager()
{
	std::lock_guard<std::mutex> lock(mBMGuardian);
	return mBlockchainManager;
}

eBlockchainMode::eBlockchainMode CNetworkManager::getBlockchainMode()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mBlockchainMode;
}

std::shared_ptr<CUDTConversationsServer> CNetworkManager::getUDTServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mUDTServer;
}

std::shared_ptr<CQUICConversationsServer> CNetworkManager::getQUICServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mQUICServer;
}

std::shared_ptr<CCORSProxy> CNetworkManager::getWebProxy()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mCORSProxy;
}


std::shared_ptr<CWebSocketsServer> CNetworkManager::getWebSocketsServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mWebSocketsServer;
}

std::shared_ptr<CWWWServer> CNetworkManager::getWWWServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mWWWServer;
}

std::shared_ptr<CFileSystemServer> CNetworkManager::getFileSystemServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mFileSystemServer;
}

/// <summary>
/// IP address assigned to Eth0 Virtual Interface.
/// </summary>
/// <returns></returns>
std::string CNetworkManager::getPublicIP()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mPublicIP;
}

/// <summary>
/// IP address assigned to Eth1 Virtual Interface.
/// </summary>
/// <returns></returns>
std::string CNetworkManager::getLocalIP()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLocalIP;
}
/// <summary>
/// Generates a comprehensive report of all network connections, including both incoming and outgoing attempts.
/// This method provides detailed information about connection durations, statuses, and limits.
/// </summary>
/// <param name="rows">Vector to store the report data as rows of strings.</param>
/// <param name="useColors">Boolean indicating whether to use colored output.</param>
/**
 * @brief Validates whether a connection from a given IP address is allowed.
 *
 * This method implements comprehensive security checks including rate limiting,
 * connection counting, and automatic banning for suspected attacks.
 *
 * @param ip The IP address attempting to connect
 * @param autoBan Whether to automatically ban IPs that exceed limits
 * @param transport The type of transport/connection being attempted
 * @param sendNotifications Whether to send notifications to the client about blocks
 * @param incrementCounters Whether to increment connection counters (default: true)
 * @return eIncomingConnectionResult indicating if connection is allowed and why
 */
eIncomingConnectionResult::eIncomingConnectionResult CNetworkManager::isConnectionAllowed(
	std::string ip,
	bool autoBan,
	eTransportType::eTransportType transport,
	bool sendNotifications,
	bool incrementCounters) // New parameter with default = true in header
{

	static const bool excuseLocalIPs = true;
	// ============================================
	// PHASE 1: Early Exit Checks (No State Modification)
	// ============================================

	std::shared_ptr<CTools> tools = getTools();
	if (!tools || ip.empty()) {
		return eIncomingConnectionResult::insufficientResources;
	}

	// Local addresses are always allowed
	if (excuseLocalIPs && isIPAddressLocal(tools->stringToBytes(ip))) {
		return eIncomingConnectionResult::allowed; 
	}

	// Handle private IP addresses with auto-whitelisting
	if (excuseLocalIPs &&  tools->isPrivateIpAddress(ip)) {
		if (whitelist(tools->stringToBytes(ip))) {
			tools->logEvent(tools->getColoredString("Implicitly whitelisted private address: ", eColor::orange) +
				tools->getColoredString(ip, eColor::neonGreen),
				"Security", eLogEntryCategory::network, 10, eLogEntryType::notification);
		}
		else {
			tools->logEvent(tools->getColoredString("Accepting connection from a whitelisted private address: ", eColor::orange) +
				tools->getColoredString(ip, eColor::neonGreen),
				"Security", eLogEntryCategory::network, 1, eLogEntryType::notification);
		}
		return eIncomingConnectionResult::allowed;
	}

	// Whitelisted IPs bypass all checks
	if (excuseLocalIPs && getIsWhitelisted(tools->stringToBytes(ip))) {
		return eIncomingConnectionResult::allowed;
	}

	// ============================================
	// PHASE 2: Connection Type Classification
	// ============================================

	bool isHttp = false;
	bool isHTTPNewConnection = false;
	bool isHTTPAPIRequest = false;
	bool isWebSocket = false;
	bool isDataExchange = false;  // UDT/QUIC for blockchain sync

	switch (transport) {
	case eTransportType::WebSocket:
		isWebSocket = true;
		isHttp = true;  // WebSocket starts as HTTP
		break;
	case eTransportType::HTTPRequest:
		isHttp = true;
		break;
	case eTransportType::HTTPAPIRequest:
		isHTTPAPIRequest = true;
		isHttp = true;
		break;
	case eTransportType::HTTPConnection:
		isHTTPNewConnection = true;
		isHttp = true;
		break;
	case eTransportType::UDT:
	case eTransportType::QUIC:
		isDataExchange = true;
		break;
	default:
		break;
	}

	// ============================================
	// PHASE 3: Resource Availability Check
	// ============================================

	if (!hasResourcesForConnection(transport)) {
		std::stringstream ss;
		ss << "Could not serve " << tools->transportTypeToString(transport)
			<< " to " << ip << " due to "
			<< tools->getColoredString("insufficient resources", eColor::cyborgBlood) << ".";
		tools->logEvent(ss.str(), "Liveness Assurance", eLogEntryCategory::network, 3, eLogEntryType::warning);
		return eIncomingConnectionResult::insufficientResources;
	}

	// ============================================
	// PHASE 4: Special Mode Checks
	// ============================================

	// Debug mode restrictions for HTTP
	if (isHttp && getWWWServer()) {
		auto wwwServer = getWWWServer();
		if (wwwServer->getUIDebugModeActive()) {
			std::shared_ptr<CSettings> settings = getBlockchainManager()->getSettings();
			std::string allowedUIIP = settings->getUIDebuggingIPv4();
			if (!tools->compareByteVectors(allowedUIIP, ip) && !tools->isPrivateIpAddress(ip)) {
				return eIncomingConnectionResult::limitReached;
			}
		}
	}

	// Bootstrap-only mode for data exchange connections
	if (isDataExchange) {
		if (getMaintainConnectivityOnlyWithBootstrapNodes() && !getIsBootstrapNode(ip)) {
			if (canEventLogAboutIP(ip)) {
				tools->logEvent(tools->getColoredString("Rejecting connection from " + ip, eColor::orange) +
					". Now accepting only Bootstrap nodes.",
					"Security", eLogEntryCategory::network, 1, eLogEntryType::notification);
			}
			return eIncomingConnectionResult::onlyBootstrapNodesAllowed;
		}
	}

	// ============================================
	// PHASE 5: Ban List Check
	// ============================================

	uint64_t currentTime = tools->getTime();
	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(tools->stringToBytes(ip));

	{
		std::lock_guard<std::mutex> lock(mBannedIPsGuardian);
		auto itBanned = mBannedIPs.find(ipHash);
		if (itBanned != mBannedIPs.end()) {
			uint64_t bannedTill = std::get<0>(itBanned->second);
			if (bannedTill != 0 && bannedTill > currentTime) {
				// Send notification if requested
				if (sendNotifications && isWebSocket) {
					// Send ban notification through existing WebSocket connections
					auto wsServer = getWebSocketsServer();
					if (wsServer) {
						auto convs = wsServer->getConversationsByIP(tools->stringToBytes(ip));
						for (auto& conv : convs) {
							CVMMetaGenerator meta;
							meta.reset();
							meta.beginSection(eVMMetaSectionType::notifications);
							uint64_t reqID = meta.addNotification(eNotificationType::error, "Temporal Ban",
								"Due to excessive requests the full-node has autonomously blocked your IP for around 30 minutes.");
							meta.endSection();

							auto task = std::make_shared<CNetTask>(eNetTaskType::sendVMMetaData, 1, reqID, 1);
							task->setData(meta.getData());
							conv->addTask(task);
							conv->processTask(task, 0);
						}
					}
				}
				return eIncomingConnectionResult::DOS;
			}
		}
	}

	// ============================================
	// PHASE 6: Connection Limit Checks (HTTP/WebSocket)
	// ============================================

	// Check concurrent connection limits (Slowloris protection)
	if (isHTTPNewConnection || isWebSocket) {
		uint64_t currentConcurrentCount = getHTTPConnectionsCount(ip);
		uint64_t maxAllowed = CGlobalSecSettings::getMaxHTTPConnectionsPerIP();

		// FIX: Corrected logic (was inverted)
		if (currentConcurrentCount >= maxAllowed) {
			tools->logEvent("Too many concurrent " + std::string(isWebSocket ? "WebSocket" : "HTTP") +
				" connections from " + ip + " (" + std::to_string(currentConcurrentCount) +
				"/" + std::to_string(maxAllowed) + ")",
				"Security", eLogEntryCategory::network, 5, eLogEntryType::warning);

			if (autoBan) {
				banIP(ip, 1800, true); // 30 minute ban
			}
			return eIncomingConnectionResult::DOS;
		}
	}

	// ============================================
	// PHASE 7: Rate Limiting Checks
	// ============================================

	uint64_t timeWindow = CGlobalSecSettings::getFirewallTimeWindowSize();
	uint64_t connsInTimeWindow = 0;

	{
		std::lock_guard<std::mutex> lock(mConnectionsPerWindowIPsGuardian);
		auto itConns = mConnectionsPerWindowIPs.find(ipHash);

		if (itConns != mConnectionsPerWindowIPs.end()) {
			// Update last seen time
			std::get<4>(itConns->second) = currentTime;

			// Check if time window expired and reset if needed
			if ((currentTime - std::get<1>(itConns->second)) > timeWindow) {
				std::get<0>(itConns->second) = 0;  // General counter
				std::get<2>(itConns->second) = 0;  // HTTP counter
				std::get<5>(itConns->second) = 0;  // API counter
				std::get<1>(itConns->second) = currentTime;  // Reset time
			}

			// Get current counter value based on connection type
			if (isHTTPAPIRequest) {
				connsInTimeWindow = std::get<5>(itConns->second);
			}
			else if (isHttp && !isWebSocket && !isHTTPNewConnection) {
				// Pure HTTP requests (not new connections or upgrades)
				connsInTimeWindow = std::get<2>(itConns->second);
			}
			else {
				// WebSocket, UDT, QUIC, or new HTTP connections
				connsInTimeWindow = std::get<0>(itConns->second);
			}

			// Only increment if requested (fixes multiple increment issue)
			if (incrementCounters) {
				if (isHTTPAPIRequest) {
					++(std::get<5>(itConns->second));
					connsInTimeWindow = std::get<5>(itConns->second);
				}
				else if (isHttp && !isWebSocket && !isHTTPNewConnection) {
					++(std::get<2>(itConns->second));
					connsInTimeWindow = std::get<2>(itConns->second);
				}
				else {
					++(std::get<0>(itConns->second));
					connsInTimeWindow = std::get<0>(itConns->second);
				}
			}
		}
		else if (incrementCounters) {
			// New IP - create entry
			// tuple: <general_counter, last_reset, http_counter, ip_bytes, last_seen, api_counter>
			uint64_t generalCount = 0, httpCount = 0, apiCount = 0;

			if (isHTTPAPIRequest) {
				apiCount = 1;
			}
			else if (isHttp && !isWebSocket && !isHTTPNewConnection) {
				httpCount = 1;
			}
			else {
				generalCount = 1;
			}

			mConnectionsPerWindowIPs[ipHash] = std::make_tuple(
				generalCount, currentTime, httpCount,
				tools->stringToBytes(ip), currentTime, apiCount
			);
			connsInTimeWindow = 1;
		}
	}

	// ============================================
	// PHASE 8: Threshold Checking and Banning
	// ============================================

	if (connsInTimeWindow > 0) {
		bool shouldBan = false;
		uint64_t banDurationSeconds = CGlobalSecSettings::getBanDTIPeersForSecs();
		uint64_t threshold = 0;
		std::string violationType;

		// Determine threshold based on connection type
		if (isHTTPAPIRequest) {
			threshold = CGlobalSecSettings::getMaxHttpAPIReqInTimeWindow();
			if (connsInTimeWindow > threshold) {
				shouldBan = true;
				banDurationSeconds = CGlobalSecSettings::getBanHTTPAPIPeersForSecs();
				violationType = "HTTP API request";
			}
		}
		else if (isHttp && !isWebSocket && !isHTTPNewConnection) { 
			threshold = CGlobalSecSettings::getMaxHttpReqInTimeWindow();
			if (connsInTimeWindow > threshold) {
				shouldBan = true;
				violationType = "HTTP request";
			}
		}
		else {
			threshold = CGlobalSecSettings::getMaxConnPerTimeWindow();
			if (connsInTimeWindow > threshold) {
				shouldBan = true;
				violationType = tools->transportTypeToString(transport) + " connection";
			}
		}

		if (shouldBan) {
			// Log the violation
			tools->logEvent("Rate limit exceeded for " + violationType + " from " + ip +
				" (" + std::to_string(connsInTimeWindow) + "/" +
				std::to_string(threshold) + " in " +
				std::to_string(timeWindow) + "s window)",
				"Security", eLogEntryCategory::network, 5, eLogEntryType::warning);

			// Apply ban
			{
				std::lock_guard<std::mutex> lock(mBannedIPsGuardian);
				uint64_t banUntilTime = currentTime + banDurationSeconds;
				mBannedIPs[ipHash] = std::make_tuple(banUntilTime, 0);
			}

			// Kernel mode firewall integration
			if (getIsFirewallKernelModeIntegrationEnabled()) {
				tools->addKernelFirewallRule(ip);
			}

			return eIncomingConnectionResult::DOS;
		}
	}

	// ============================================
	// PHASE 9: Connection Time Tracking (for UDT/QUIC)
	// ============================================

	if (isDataExchange) {
		// Check cumulative connection time to prevent excessive resource usage
		uint64_t connectionDuration = getConnTracker()->getTime(ip, 3 * 60 * 60); // 3 hour window
		uint64_t maxDuration = 30 * 60; // 30 minutes max

		if (connectionDuration > maxDuration) {
			// Check if we have low connectivity (allow exception)
			conversationFlags flags;
			flags.exchangeBlocks = true;
			auto convs = getConversations(flags);

			if (convs.size() < 3) {
				tools->logEvent("Allowing excessive connectivity from " + ip +
					" due to low peer count",
					"Liveness Assurance", eLogEntryCategory::network, 1,
					eLogEntryType::notification);
			}
			else {
				tools->logEvent("Connection time limit reached for " + ip,
					"Liveness Assurance", eLogEntryCategory::network, 1,
					eLogEntryType::notification);
				return eIncomingConnectionResult::limitReached;
			}
		}
	}

	// All checks passed
	return eIncomingConnectionResult::allowed;
}
/// <summary>
/// Converts an eIncomingConnectionResult to a human-readable string.
/// </summary>
/// <param name="status">The connection result status to convert.</param>
/// <returns>A string representation of the connection status.</returns>
std::string CNetworkManager::getConnectionStatusString(eIncomingConnectionResult::eIncomingConnectionResult status)
{
	switch (status) {
	case eIncomingConnectionResult::allowed:
		return "Allowed";
	case eIncomingConnectionResult::limitReached:
		return "Limit Reached";
	case eIncomingConnectionResult::DOS:
		return "DOS";
	case eIncomingConnectionResult::insufficientResources:
		return "Insufficient Resources";
	case eIncomingConnectionResult::onlyBootstrapNodesAllowed:
		return "Only Bootstrap Nodes";
	default:
		return "Unknown";
	}
}
void CNetworkManager::clearConnectionAttempts()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastOutgoingConnectionAttempts.clear();
}

void CNetworkManager::registerConversation(std::shared_ptr<CConversation> conversation)
{
	if (!conversation)
		return;

	// Local Variables - BEGIN
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::string ipS = conversation->getIPAddress();
	
	if (!tools->validateIPv4(ipS))
	{
		return;
	}

	// Hash-Table Index Preparation - BEGIN
	std::vector<uint8_t> ipB = tools->stringToBytes(ipS);
	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(ipB); // use hash as index
	// Hash-Table Index Preparation - END

	uint64_t time = std::time(0);
	// Local Variables - END

	std::lock_guard<std::mutex> lock(mConnectionsPerWindowIPsGuardian);

	// Operational Logic - BEGIN
	robin_hood::unordered_map < std::vector<uint8_t>, std::tuple<uint64_t, uint64_t, uint64_t, std::vector<uint8_t>, uint64_t, uint64_t>>::iterator itConns = 
		mConnectionsPerWindowIPs.find(ipHash);

	if (itConns == mConnectionsPerWindowIPs.end())
	{//peer is not already known

		// [ Warning ] : index = hash of IP. Real IP at index 3.
		mConnectionsPerWindowIPs[ipHash] = std::tuple<uint64_t, uint64_t, uint64_t, std::vector<uint8_t>, uint64_t, uint64_t>(1, time, 1, ipB, time, 0);
	}
	// Operational Logic - END
}



/**
 * @brief Determines if a new connection attempt with a specified node is permissible.
 *
 * This function checks if a new connection attempt to a specific node is allowed based on the
 * last attempt timestamps for both QUIC and UDT protocols. It adheres to certain rules to
 * prioritize QUIC connections over UDT and ensures that connection attempts are not made
 * too frequently to the same node with the same protocol or switch to a different protocol.
 *
 * @param ip The IP address of the node to which the connection attempt is considered.
 * @param protocol The protocol (QUIC or UDT) for the current connection attempt.
 *
 * @return bool True if a new connection attempt is allowed, False otherwise.
 *
 * The function works as follows:
 * - If no previous attempt was recorded for the given IP, it allows a new connection.
 * - Resets future timestamps to the current time.
 * - Promotes QUIC attempts over UDT, but does not block UDT if it's valid to attempt.
 * - Allows a new attempt if enough time has passed since the last attempt of any protocol,
 *   using the interval defined by `getRetryUnavailableNodeAfterSec()`.
 * - Disallows a new attempt if not enough time has passed since the last attempt of the
 *   other protocol, using the interval defined by `getTryAnotherProtocolAfterSec()`.
 * - Allows a new attempt if enough time has passed since the last attempt of the same protocol,
 *   using the interval defined by `getCanAttemptSameNodeSameProtocolAfterSec()`.
 * - Defaults to allowing a new attempt unless one of the conditions for dis-allowance is met.
 */
bool CNetworkManager::canAttemptConnection(const std::string& ip, eTransportType::eTransportType protocol, bool markConnectionAttempt) {
	
	// Local Variables - BEGIN
	std::shared_ptr<CQUICConversationsServer> quicServer = getQUICServer();
	std::shared_ptr<CUDTConversationsServer> udtServer = getUDTServer();
	bool connectOnlyWithBootstrapNodes = getMaintainConnectivityOnlyWithBootstrapNodes();
	std::shared_ptr<CTools> tools = getTools();
	uint64_t currentTime = std::time(0);
	bool quicEnabled = isTransportEnabledForSync(eTransportType::QUIC);
	// Local Variables - END

	// Local addresses - BEGIN (localhost etc.)
	if (isIPAddressLocal(getTools()->stringToBytes(ip)))
	{
		return true;
	}
	// Local addresses - END

	// Private addresses - BEGIN
	if (tools->isPrivateIpAddress(ip))
	{
		if (whitelist(tools->stringToBytes(ip)))
		{
			tools->logEvent(tools->getColoredString("Implicitly whitelisted egress private address: ", eColor::orange) + tools->getColoredString(ip, eColor::neonGreen),
				"Security", eLogEntryCategory::network, 10, eLogEntryType::notification);
		}
		else
		{
			tools->logEvent(tools->getColoredString("Allowing egress connection to a whitelisted private address: ", eColor::orange) + tools->getColoredString(ip, eColor::neonGreen),
				"Security", eLogEntryCategory::network, 1, eLogEntryType::notification);
		}
	}
	// Private addresses - END

	if (getIsWhitelisted(mTools->stringToBytes(ip)))
	{
		return true;
	}

	decltype(mLastOutgoingConnectionAttempts)::iterator it;
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		it = mLastOutgoingConnectionAttempts.find(ip);
	}


	
	// Operational Logic - BEGIN

	if (connectOnlyWithBootstrapNodes && !getIsBootstrapNode(ip))
	{
		return false;
	}
	
	if (protocol == eTransportType::UDT && !udtServer)
	{
		return false;
	}
	else if (protocol == eTransportType::QUIC && !quicServer)
	{
		return false;
	}

	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	// If no previous attempt, allow a new connection
	if (it == mLastOutgoingConnectionAttempts.end()) {

		if (markConnectionAttempt)
		{
			mLastOutgoingConnectionAttempts[ip] = std::make_shared< ConnectionAttemptTimestamps>(protocol, std::time(0));
		}

		return true;
	}


	auto& attempts = it->second;

	uint64_t lastUDTAttempt = attempts->getLastUDTAttempt();
	uint64_t lastQUICAttempt = attempts->getLastQUICAttempt();

	// Reset future timestamps to current time
	if (lastQUICAttempt > currentTime || lastUDTAttempt > currentTime) {
		lastQUICAttempt = currentTime;
		lastUDTAttempt = currentTime;
	}


	// Promote QUIC attempts over UDT. Prevent UDT connection if the most recent attempt wasn't QUIC.
	// Block UDT attempts even when no QUIC attempt has been made
	if (quicEnabled &&  protocol == eTransportType::UDT && (lastQUICAttempt <= lastUDTAttempt || lastQUICAttempt == 0)) {
		return false;
	}

	//  Do not allow new attempt if not enough time has passed since the last attempt of any protocol
	if ((currentTime - lastQUICAttempt) < CGlobalSecSettings::getRetryUnavailableNodeAfterSec() ||
		(currentTime - lastUDTAttempt) < CGlobalSecSettings::getRetryUnavailableNodeAfterSec()) {
		return false;
	}

	// Disallow a new attempt if not enough time has passed since the last attempt of the other protocol
	uint64_t otherProtocolLastAttempt = (protocol == eTransportType::QUIC) ? lastUDTAttempt : lastQUICAttempt;

	if ((currentTime - otherProtocolLastAttempt) < CGlobalSecSettings::getTryAnotherProtocolAfterSec()) {
		return false;
	}

	// Disallow a new attempt if not enough time has passed since the last attempt of the same protocol
	uint64_t lastAttemptTime = (protocol == eTransportType::QUIC) ? lastQUICAttempt : lastUDTAttempt;
	if ((currentTime - lastAttemptTime) < CGlobalSecSettings::getCanAttemptSameNodeSameProtocolAfterSec()) {
		return false;
	}

	// Default to allowing a new attempt
	return true;
	// Operational Logic - END
}


void CNetworkManager::storeConnectionAttempt(const std::string& ip, eTransportType::eTransportType protocol) {
	
	std::shared_ptr<ConnectionAttemptTimestamps> attempts;
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		attempts = mLastOutgoingConnectionAttempts[ip];
		if (!attempts)
		{
			mLastOutgoingConnectionAttempts[ip] = std::make_shared<ConnectionAttemptTimestamps>(protocol, std::time(0));
			return;
		}
	}

	if (protocol == eTransportType::QUIC) {
		attempts->pingLastQUICAttempt();
	}
	else {
		attempts->pingLastUDTAttempt();
	}
}

bool CNetworkManager::isBonusSlotAllowedForIP(std::string ip)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mBonusSlotsGuardian);

	//Local Variables - BEGIN
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t>::iterator itSlotsPerIP;
	uint64_t slotsTaken = 0;
	std::vector<uint8_t> ipB = CCryptoFactory::getInstance()->getSHA2_256Vec(tools->stringToBytes(ip));
	//Local Variables - END

	//Operational Logic - BEGIN
	itSlotsPerIP = mBonusSlotsPerIP.find(ipB);

	if (itSlotsPerIP != mBonusSlotsPerIP.end())
	{
		slotsTaken = itSlotsPerIP->second;

		if (slotsTaken < CGlobalSecSettings::getAllowedBonusSlotsPerIP())
		{
			++(itSlotsPerIP->second);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		mBonusSlotsPerIP[ipB] = 1;
	}

	return true;

	//Operational Logic - END
}

/// <summary>
/// Returns a detailed report regarding each remote endpoint the node happened to get in contact with.
/// </summary>
/// <param name="newLine"></param>
/// <returns></returns>
std::string CNetworkManager::getAnalysisReport(std::string newLine)
{
	std::string toRet;
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mStatisticsGuardian);
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> ::iterator sIt;
	uint64_t counter = 0;


	//Unsolicited Datagrams - BEGIN
	toRet += newLine;
	toRet += "[Datagrams Put On Stack]:";
	toRet += newLine;
	sIt = mUnsolicitedDatagramsReceived.begin();

	while (sIt != mUnsolicitedDatagramsReceived.end())
	{
		toRet += tools->bytesToString(std::get<0>(sIt->second));
		toRet += ": ";
		toRet += std::to_string(std::get<1>(sIt->second));
		toRet += newLine;
		++sIt; counter++;
	}
	counter = 0;
	//Unsolicited Datagrams - END

	//Kademlia Datagrams - BEGIN
	toRet += newLine;
	toRet += "[Kademlia Datagrams]:";
	toRet += newLine;
	sIt = mValidKademliaDatagramsReceived.begin();

	while (sIt != mValidKademliaDatagramsReceived.end())
	{
		toRet += tools->bytesToString(std::get<0>(sIt->second));
		toRet += ": ";
		toRet += std::to_string(std::get<1>(sIt->second));
		toRet += newLine;
		++sIt; counter++;
	}
	counter = 0;
	//Kademlia Datagrams - END

	//UDT Datagrams - BEGIN
	toRet += newLine;
	toRet += "[UDT Datagrams]:";
	toRet += newLine;
	sIt = mValidUDTDatagramsReceived.begin();

	while (sIt != mValidUDTDatagramsReceived.end())
	{
		toRet += tools->bytesToString(std::get<0>(sIt->second));
		toRet += ": ";
		toRet += std::to_string(std::get<1>(sIt->second));
		toRet += newLine;
		++sIt; counter++;
	}
	counter = 0;
	//UDT Datagrams - END


	//QUIC Datagrams - BEGIN
	toRet += newLine;
	toRet += "[QUIC Datagrams]:";
	toRet += newLine;
	sIt = mValidQUICDatagramsReceived.begin();

	while (sIt != mValidQUICDatagramsReceived.end())
	{
		toRet += tools->bytesToString(std::get<0>(sIt->second));
		toRet += ": ";
		toRet += std::to_string(std::get<1>(sIt->second));
		toRet += newLine;
		++sIt; counter++;
	}
	counter = 0;
	//QUIC Datagrams - END



	//Invalid Datagrams - BEGIN
	toRet += newLine;
	toRet += "[Invalid Datagrams]:";
	toRet += newLine;
	sIt = mInvalidDatagramsReceived.begin();

	while (sIt != mInvalidDatagramsReceived.end())
	{
		toRet += tools->bytesToString(std::get<0>(sIt->second));
		toRet += ": ";
		toRet += std::to_string(std::get<1>(sIt->second));
		toRet += newLine;
		++sIt; counter++;
	}
	counter = 0;
	//Invalid Datagrams - END
	return toRet;

}

void CNetworkManager::setIsKademliaOperational(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mKademlialNetworkingEnabled = isIt;
}

/*
	[ Notice ]: this method is used ONLY when the UDT sub-system is enabled.
	[ Rationale ]: in this mode of operation, the underlying UDP socket of UDP - it would be REUSED for retrival of
				   incoming Kademlia datagrams (see implementation of CScriptEngine::net() for more explanation).

				   In this mode, when an unsolicited non-UDT compliant UDP datagram arrives, it would be passed on
				   to this very method for further processing.
*/
bool CNetworkManager::processUnsolicitedDatagram(uint64_t protocolType, std::vector<uint8_t> bytes, const CEndPoint& endpoint)
{
	pingDatagramReceived(eNetworkSystem::putOnStack, endpoint.getAddress());

	//Local Variables - BEGIN
	std::shared_ptr<CTools> tools = getTools();
	uint64_t startTime = tools->getTime(true);
	uint64_t endTime = 0;
	uint64_t diff = 0;
	bool getIsAnalysisMode = getIsNetworkTestMode();
	std::shared_ptr<CKademliaServer> kad = getKademliaServer();
	bool res = false;
	//Local Variables - END

	//Operational Logic - BEGIN
	
		if (protocolType == eNetworkSystem::Kademlia)
		{
			if (getIsAnalysisMode)
			{
				tools->logEvent("Received a Kademlia datagram from " + endpoint.getDescription());//simply it was not yet processed by any sub-system.
				//we then go ahead and calculate a ratio between unsolicited and recognized datagrams. Invalid datagrams are kept track of in test-mode as well.
			}
			if (kad)
			{
				//Kademlia Datagram Support - BEGIN
				res = kad->processKademliaDatagram(bytes, endpoint);
				endTime = tools->getTime(true);
				diff = (endTime - startTime);
				//Kademlia Datagram Support - END

				if (diff > 2000)
				{
					tools->logEvent("Warning: processing of an incoming Kademlia datagram took: " + std::to_string(diff) + "ms", eLogEntryCategory::network, 1, eLogEntryType::warning, eColor::cyberWine);
				}
			}
			else
			{
				tools->logEvent("Error processing Kademlia datagram. The Kademlia sub-system is unavailable.", eLogEntryCategory::network, 1, eLogEntryType::failure, eColor::cyberWine);
			}
		}
		else
		{
			if (getIsAnalysisMode)
			{
				tools->logEvent("Received an unsolicited datagram from " + endpoint.getDescription());//simply it was not yet processed by any sub-system.
				//we then go ahead and calculate a ratio between unsolicited and recognized datagrams. Invalid datagrams are kept track of in test-mode as well.
			}
		}

		return res;
	
	//Operational Logic - END
	return false;
}
bool CNetworkManager::getIsKademliaOperational()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mKademlialNetworkingEnabled;
}
bool CNetworkManager::clearDataSet()
{
	std::lock_guard<std::mutex> lock(mBannedIPsGuardian);
	std::lock_guard<std::mutex> loc2k(mConnectionsPerWindowIPsGuardian);
	mConnectionsPerWindowIPs.clear();
	mBannedIPs.clear();
	return true;
}

void CNetworkManager::clearSeenTXs()
{
	std::lock_guard<std::mutex> lock(mSeenTXsGuardian);
	mSeenTXs.clear();
}

bool CNetworkManager::whitelist(std::vector<uint8_t> address)
{
	std::shared_ptr<CTools> tools = getTools();
	if (!address.size())
		return false;

	std::lock_guard<std::mutex> lock(mBannedIPsGuardian);
	std::lock_guard<std::mutex> loc2k(mConnectionsPerWindowIPsGuardian);


	for (uint64_t i = 0; i < mWhitelist.size(); i++)
	{
		if (tools->compareByteVectors(mWhitelist[i], address))
		{
			return false;// already whitelisted
		}
	}
	tools->logEvent(tools->getColoredString("Whitelisting IP Address: ", eColor::lightGreen) + tools->getColoredString(tools->bytesToString(address), eColor::blue), eLogEntryCategory::network, 1);
	mWhitelist.push_back(address);
	
	  std::shared_ptr<CSettings> settings = getBlockchainManager()->getSettings();

	  if (!settings->setWhitelistedIPs(mWhitelist))
	  {
		  return false;
	  }
	return true;
}

bool CNetworkManager::unWhitelist(std::vector<uint8_t> address)
{
	std::lock_guard<std::mutex> lock(mBannedIPsGuardian);
	std::lock_guard<std::mutex> loc2k(mConnectionsPerWindowIPsGuardian);
	std::shared_ptr<CTools> tools = CTools::getInstance();
	std::shared_ptr<CSettings> settings = getBlockchainManager()->getSettings();

	std::vector<std::vector<uint8_t>>::iterator peer = mWhitelist.begin();
	while (peer != mWhitelist.end()) {

		if (tools->compareByteVectors(*peer, address))
		{
			peer = mWhitelist.erase(peer);

			if (!settings->setWhitelistedIPs(mWhitelist))
			{
				return false;
			}

			return true;
		}
		else
		{
			++peer;
		}
	}

	return false;
}

/// <summary>
/// Checks if a particular TX-data structure was already seen in the wild.
/// The function can also be used to check whether a data-package was already transmitted to a remote peer.
/// That is to spare on unnecessary processing.
/// Important: hash is a hash of the received data-array.
/// NOT an identifier of a TX. That is for optimization purposes so to spare on instantiation of TX data-structures.
/// 
/// </summary>
/// <param name="address"></param>
/// <returns></returns>
uint64_t CNetworkManager::getWasDataSeen(const std::vector<uint8_t> &hash, const std::string & receivedFrom, const bool& autoBan , const std::string& target, bool allowSingleOutgressLoop)
{

	if (hash.empty())
	{
		return 0;
	}

	std::shared_ptr< CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::lock_guard<std::mutex> lock(mSeenTXsGuardian);

	//Local Variables - BEGIN
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t>::iterator seenTXIt;
	robin_hood::unordered_map<std::vector<uint8_t>, uint64_t>::iterator redundantTxIpsIt;
	std::vector<uint8_t> accessKey;
	bool wasSeen = false;
	bool isIngressData = target.size() == 0 ? true : false;
	//Local Variables - END


	//Operational Logic - BEGIN

	//Look-Up - BEGIN
	//take into account the recipient (if any), while constructing the look-up key.
	if (isIngressData==false)
	{
		if (allowSingleOutgressLoop)//
		{
			accessKey.insert(accessKey.begin(),1);
		}
		accessKey.insert(accessKey.begin(), hash.begin(), hash.end());
		accessKey.insert(accessKey.end(), target.begin(), target.end());
		accessKey = cf->getSHA2_256Vec(accessKey);
	}
	else
	{
		accessKey = hash;
	}

	//perform the actual look-up
	seenTXIt = mSeenTXs.find(isIngressData ==false? accessKey: hash);//optimization
	//Look-Up - END
	
	wasSeen = (seenTXIt != mSeenTXs.end());
	

	//Firewall support for the Incoming Requests - BEGIN
	if (autoBan && isIngressData && !receivedFrom.empty())
	{
		std::shared_ptr<CTools> tools = getTools();
		std::lock_guard<std::mutex> lock(mIPsRedundantObjectsReceivedGuardian);
		std::vector<uint8_t> peerID = cf->getSHA2_256Vec(tools->stringToBytes(receivedFrom));
		redundantTxIpsIt = mIPsRedundantObjectsReceived.find(peerID);

		uint64_t IPRedundancyCount = 0;

			if (redundantTxIpsIt != mIPsRedundantObjectsReceived.end())
			{
				mIPsRedundantObjectsReceived[peerID]++;
				IPRedundancyCount = redundantTxIpsIt->second;
			}
			else
			{
				mIPsRedundantObjectsReceived[peerID] = 1;
				IPRedundancyCount = 0;
			}
	

			if (IPRedundancyCount > CGlobalSecSettings::getMaxRedundantObjectsPerDSMSyncSession())
			{

				uint64_t banTime = tools->genRandomNumber(900, 7200);
				tools->logEvent("[DSM Sync]: temporarily banning " + receivedFrom + " (for " + tools->secondsToString(banTime) + ") due to excessive redundant objects received.",
					eLogEntryCategory::network, 2, eLogEntryType::warning, eColor::lightPink);
				const_cast<bool&>(autoBan) = true;//let the caller know that the IP was banned so that the connection can be terminated
				banIP(receivedFrom, banTime);
			}

	}
	//Firewall support for the Incoming Requests - END

	return wasSeen? seenTXIt->second:0;
	//Operational Logic - END
}

/// <summary>
/// Important: hash is a hash of the received data-array.
/// NOT an identifier of a TX. That is for optimization purposes so to spare on instantiation of TX data-structures.
/// 
/// NOTE: the function can be used for both ingress and outgress data-grams (to check if a particular byte sequence was already sent to a remote particular node).
/// </summary>
/// <param name="address"></param>
/// <returns></returns>
void CNetworkManager::sawData(const std::vector<uint8_t>& hash, const std::string & target)
{
	if (hash.size() != 32)
	{
		return;
	}

	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	std::vector<uint8_t> accessKey;
	bool isIngressData = target.size() == 0 ? true : false;
	//Local Variables - END

	//Operational Logic - BEGIN
	
	//take into account the recipient (if any), while constructing the look-up key.
	if (isIngressData == false)
	{
		accessKey.insert(accessKey.begin(), hash.begin(), hash.end());
		accessKey.insert(accessKey.end(), target.begin(), target.end());
		accessKey = cf->getSHA2_256Vec(accessKey);
	}
	else
	{
		accessKey = hash;
	}

	{
		std::lock_guard<std::mutex> lock(mSeenTXsGuardian);
		mSeenTXs[isIngressData == false ? accessKey : hash] = std::time(0);//optimization to save on memory-copies
	}
	//Operational Logic - END
}

bool CNetworkManager::getIsWhitelisted(std::vector<uint8_t> address)
{
	std::lock_guard<std::mutex> lock(mBannedIPsGuardian);
	std::lock_guard<std::mutex> loc2k(mConnectionsPerWindowIPsGuardian);
	std::shared_ptr<CTools> tools = CTools::getInstance();


	std::vector<std::vector<uint8_t>>::iterator peer = mWhitelist.begin();
	while (peer != mWhitelist.end()) {
		if (tools->compareByteVectors(*peer, address))
		{
			return true;
		}
		++peer;
	}

	return false;
}



bool CNetworkManager::pardonPeer(std::vector<uint8_t> address) {
    if (address.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mBannedIPsGuardian);
    std::lock_guard<std::mutex> lock2(mConnectionsPerWindowIPsGuardian);

    // Hash the address for consistent key representation
    std::vector<uint8_t> tag = CCryptoFactory::getInstance()->getSHA2_256Vec(address);

    // Remove the entry from the banned IPs map
    auto itBanned = mBannedIPs.find(tag);
    if (itBanned != mBannedIPs.end()) {
        mBannedIPs.erase(itBanned);
    }

    // Reset the connection attempt counters for this IP address
    auto itConns = mConnectionsPerWindowIPs.find(tag);
    if (itConns != mConnectionsPerWindowIPs.end()) {
        std::get<0>(itConns->second) = 0; // Reset the main connection attempt counter
        std::get<2>(itConns->second) = 0; // Reset the HTTP requests counter
        return true;
    } else {
        return false;
    }
}

void CNetworkManager::getStateTable(std::vector<std::vector<std::string>>& rows, bool onlyBlocked, bool hideAddresses)
{
	std::lock_guard<std::mutex> lock(mBannedIPsGuardian);

	//Local Variables - BEGIN
	std::vector<uint8_t> id;
	std::vector<std::string> row;
	std::shared_ptr<CWebSocketsServer> websockServ = getWebSocketsServer();
	std::shared_ptr<CUDTConversationsServer> udtServ = getUDTServer();
	std::shared_ptr<CTools> tools = CTools::getInstance();
	uint64_t httpWarningThreshold = 2000;
	uint64_t generalBucketWarningThreshold = 2;
	size_t convsCount = 0;
	uint64_t bucketSize = 0;
	uint64_t currentTime = std::time(0);

	// Get kernel firewall rules if needed
	std::vector<std::string> kernelBannedIPs;
	if (onlyBlocked && getIsFirewallKernelModeIntegrationEnabled()) {
		kernelBannedIPs = tools->getKernelFirewallRules();
	}
	//Local Variables - END

	//Operational Logic - BEGIN
	rows.clear();

	// Create header row
	row.push_back(tools->getColoredString("Address", eColor::blue));
	row.push_back(tools->getColoredString("Bucket", eColor::blue));
	row.push_back(tools->getColoredString("HTTP Bucket", eColor::blue));
	row.push_back(tools->getColoredString("WebSockets", eColor::blue));
	row.push_back(tools->getColoredString("UDTs", eColor::blue));
	row.push_back(tools->getColoredString("Timestamp", eColor::blue));
	if (onlyBlocked) {
		row.push_back(tools->getColoredString("Ban Expires", eColor::blue));
		row.push_back(tools->getColoredString("Kernel Mode", eColor::blue));
	}
	rows.push_back(row);

	for (const auto& entry : mConnectionsPerWindowIPs)
	{
		id = std::get<3>(entry.second);
		std::string ipStr = tools->bytesToString(id);

		// Skip non-banned IPs if onlyBlocked is true
		if (onlyBlocked) {
			auto bannedIt = mBannedIPs.find(id);
			if (bannedIt == mBannedIPs.end()) {
				continue; // Skip this IP as it's not banned
			}
			// Skip if ban has expired
			uint64_t banExpiration = std::get<0>(bannedIt->second);
			if (banExpiration < currentTime) {
				continue;
			}
		}

		row.clear();
		convsCount = 0;

		// Add IP address
		row.push_back(tools->getColoredString(
			hideAddresses ? "*" : ipStr,
			eColor::lightCyan
		));

		// Add bucket sizes
		auto sec = entry.second;
		bucketSize = std::get<0>(entry.second);
		row.push_back(bucketSize > generalBucketWarningThreshold ?
			tools->getColoredString(std::to_string(bucketSize), eColor::lightPink) :
			std::to_string(bucketSize));

		bucketSize = std::get<2>(entry.second);
		row.push_back(bucketSize > httpWarningThreshold ?
			tools->getColoredString(std::to_string(bucketSize), eColor::lightPink) :
			std::to_string(bucketSize));

		// Add WebSocket connections
		if (websockServ) {
			convsCount = websockServ->getConversationsByIP(id).size();
		}
		row.push_back(std::to_string(convsCount));

		// Add UDT connections
		if (udtServ) {
			convsCount = udtServ->getConversationsCountByIP(id);
		}
		else {
			convsCount = 0;
		}
		row.push_back(std::to_string(convsCount));

		// Add timestamp
		row.push_back(tools->timeToString(std::get<4>(entry.second)));

		// Add ban information for blocked IPs
		if (onlyBlocked) {
			auto bannedIt = mBannedIPs.find(id);
			uint64_t banExpiration = std::get<0>(bannedIt->second);
			row.push_back(tools->timeToString(banExpiration));

			// Add kernel mode status
			bool isKernelBanned = std::find(kernelBannedIPs.begin(),
				kernelBannedIPs.end(),
				ipStr) != kernelBannedIPs.end();
			row.push_back(tools->getColoredString(
				isKernelBanned ? "Yes" : "No",
				isKernelBanned ? eColor::lightGreen : eColor::lightPink
			));
		}

		rows.push_back(row);
	}
	//Operational Logic - END
}


/**
 * @brief Bans an IP address for a specified duration, optionally leveraging kernel-mode firewall integration for improved performance.
 *
 * This method bans the specified IP address for the given duration. It involves converting the IP address to bytes, hashing it for secure storage,
 * and adding or updating the entry in the banned IPs list. When kernel-mode firewall integration is enabled, the method also instructs the kernel to
 * handle the IP ban, leading to improved performance in rule processing on the native operating system.
 *
 * @param ip The IP address to ban, given as a string.
 * @param time The duration of the ban in seconds. If zero, the default duration is used.
 * @param useKernelMode Uses kernel mode intergation with native operating system whenever available.
 * @return Returns true on successful operation.
 *
 * @details
 * The method performs the following operations:
 * - Converts the IP address string to a byte vector and hashes it using SHA2-256 for secure and efficient storage.
 * - Calculates the ban duration based on the current time and the specified or default ban duration.
 * - Acquires a lock on the banned IPs list to ensure thread-safe operations.
 * - Updates the banned IPs list with the hashed IP and its ban duration.
 * - If kernel-mode firewall integration is enabled, adds a rule to the kernel's firewall to enforce the ban, significantly improving performance by
 *   offloading the rule processing to the kernel of the native operating system.
 *
 * @note
 * - The use of hashing for IP storage enhances security and privacy.
 * - Kernel-mode firewall integration offers a performance advantage by reducing the overhead of rule processing in user mode.
 * - This method should be used with caution as banning IP addresses can impact network connectivity and access.
 */
bool CNetworkManager::banIP(std::string ip, size_t time, bool useKernelMode) {

	if (mTools->iequals("127.0.0.1", ip))
	{
		return false;
	}
	std::shared_ptr<CTools> tools = getTools();

	// Convert the IP string to bytes and hash it
	std::vector<uint8_t> ipB = getTools()->stringToBytes(ip);
	ipB = CCryptoFactory::getInstance()->getSHA2_256Vec(ipB);

	// Calculate the ban duration
	uint64_t banDuration = std::time(0) + (time == 0 ? CGlobalSecSettings::getBanDTIPeersForSecs() : time);

	// Lock the guardian mutex before accessing mBannedIPs
	std::lock_guard<std::mutex> lock(mBannedIPsGuardian);

	// Insert or update the entry in mBannedIPs with the ban duration and reset the reject screens shown count
	mBannedIPs[ipB] = std::make_tuple(banDuration, 0);

	clearHTTPConnectionCounter(ip);

	// Kernel Mode Firewall Integration - BEGIN
	if (useKernelMode && getIsFirewallKernelModeIntegrationEnabled())
	{
		return tools->addKernelFirewallRule(ip); // let Windows Kernel handle the 'ban'
	}
	// Kernel Mode Firewall Integration - END

	return true;
}

/// <summary>
/// The aim of this function is to assess whether local node can serve a yet another new connection, described by its transport layer protocol.
/// </summary>
/// <param name="transport"></param>
/// <returns></returns>
bool CNetworkManager::hasResourcesForConnection(eTransportType::eTransportType transport)
{
	//Local Variables - BEGIN
	bool isGo = true;
	std::shared_ptr<CTools> tools = getTools();
	uint64_t requiredRAM =  CGlobalSecSettings::getRAMOperationalMargin();
	uint64_t availableRAM = tools->getFreeHotStorage();
	//Local Variables - END
	
	//Operational Logic - BEGIN
	//Assess Requirements - BEGIN
	switch (transport)
	{
	case eTransportType::SSH:
		break;
	case eTransportType::WebSocket:
		requiredRAM += 4000000;
		break;
	case eTransportType::UDT:
		requiredRAM += CGlobalSecSettings::getCeilingUDTConnectionRAMUsage();
		break;

	case eTransportType::QUIC:
		requiredRAM += CGlobalSecSettings::getCeilingUDTConnectionRAMUsage();
		break;
	case eTransportType::local:
		break;
	default:
		break;
	}
	//Assess Requirements - END

	// Execution - BEGIN
	if (availableRAM < requiredRAM)
	{
		isGo = false;
	}
	
	// Execution - END
	return isGo;

	//Operational Logic - END
}


void CNetworkManager::pingBlockedAccessNotified(const std::string& ip) {
	std::lock_guard<std::mutex> lock(mBannedIPsGuardian);

	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(std::vector<uint8_t>(ip.begin(), ip.end()));

	auto it = mBannedIPs.find(ipHash);
	if (it != mBannedIPs.end()) {
		std::get<1>(it->second)++; // Increment the count of screens shown
	}
}


bool CNetworkManager::canShowBlockedAccessScreen(const std::string& ip) {
	std::lock_guard<std::mutex> lock(mBannedIPsGuardian);

	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(std::vector<uint8_t>(ip.begin(), ip.end()));

	auto it = mBannedIPs.find(ipHash);
	if (it != mBannedIPs.end()) {
		uint64_t showCount = std::get<1>(it->second); // Retrieve the count of screens shown
		return showCount < CGlobalSecSettings::getBlockedScreenShowThreshold();
	}
	return true; // If the IP is not banned, allow showing the screen
}



std::shared_ptr<CDTIServer> CNetworkManager::getDTIServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mDTIServer;
}





bool CNetworkManager::incrementWebSocketConnection(const std::string& ip) {
	// Similar to incrementHTTPConnection but for WebSockets
	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(
		std::vector<uint8_t>(ip.begin(), ip.end()));

	std::lock_guard<std::mutex> lock(mConcurrentHTTPGuardian); // Reuse same guardian

	// Use a different counter or add a flag
	// Count both HTTP and WebSocket connections together
	uint64_t currentCount = 0;
	auto it = mConcurrentHTTPConnections.find(ipHash);
	if (it != mConcurrentHTTPConnections.end()) {
		currentCount = it->second;
	}

	// Lower limit for combined HTTP+WebSocket
	if (currentCount >= CGlobalSecSettings::getMaxHTTPConnectionsPerIP()) { // Total connections limit
		return false;
	}

	mConcurrentHTTPConnections[ipHash] = currentCount + 1;
	return true;
}

/**
 * @brief Clears the connection counter for a specific IP address
 *
 * This method removes the connection tracking entry for the specified IP address,
 * effectively resetting its HTTP and WebSocket connection count to zero.
 *
 * @param ip The IP address to clear from connection tracking
 */
void CNetworkManager::clearHTTPConnectionCounter(const std::string& ip) {
	if (ip.empty() || !mTools->validateIPv4(ip)) {
		mTools->logEvent("Invalid IP address provided to clearHTTPConnectionCounter: " + ip,
			"Network Manager", eLogEntryCategory::network, 5, eLogEntryType::warning);
		return;
	}

	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(
		std::vector<uint8_t>(ip.begin(), ip.end()));

	std::lock_guard<std::mutex> lock(mConcurrentHTTPGuardian);

	auto it = mConcurrentHTTPConnections.find(ipHash);
	if (it != mConcurrentHTTPConnections.end()) {
		uint64_t previousCount = it->second;
		mConcurrentHTTPConnections.erase(it);

		mTools->logEvent("Cleared HTTP connection counter for IP " + ip +
			" (previous count: " + std::to_string(previousCount) + ")",
			"Network Manager", eLogEntryCategory::network, 3, eLogEntryType::notification);
	}
}



void CNetworkManager::decrementWebSocketConnection(const std::string& ip) {
	// This is the same as decrementHTTPConnection since we're using 
	// the same counter for both HTTP and WebSocket connections
	decrementHTTPConnection(ip);
}



uint64_t CNetworkManager::getHTTPConnectionsCount(const std::string& ip) {
	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(
		std::vector<uint8_t>(ip.begin(), ip.end()));

	std::lock_guard<std::mutex> lock(mConcurrentHTTPGuardian);

	// Check current count first
	uint64_t currentCount = 0;
	auto it = mConcurrentHTTPConnections.find(ipHash);
	if (it != mConcurrentHTTPConnections.end()) {
		currentCount = it->second;
	}

	// Enforce limit
	return currentCount;

	
}


bool CNetworkManager::incrementHTTPConnection(const std::string& ip) {
	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(
		std::vector<uint8_t>(ip.begin(), ip.end()));

	std::lock_guard<std::mutex> lock(mConcurrentHTTPGuardian);

	// Check current count first
	uint64_t currentCount = 0;
	auto it = mConcurrentHTTPConnections.find(ipHash);
	if (it != mConcurrentHTTPConnections.end()) {
		currentCount = it->second;
	}

	// Enforce limit
	if (currentCount >= CGlobalSecSettings::getMaxHTTPConnectionsPerIP()) {
		return false; // Limit reached
	}

	// Increment
	mConcurrentHTTPConnections[ipHash] = currentCount + 1;
	return true;
}

void CNetworkManager::decrementHTTPConnection(const std::string& ip) {
	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(
		std::vector<uint8_t>(ip.begin(), ip.end()));

	std::lock_guard<std::mutex> lock(mConcurrentHTTPGuardian);
	auto it = mConcurrentHTTPConnections.find(ipHash);
	if (it != mConcurrentHTTPConnections.end()) {
		if (it->second > 0) {
			it->second--;
		}
		if (it->second == 0) {
			mConcurrentHTTPConnections.erase(it);
		}
	}
}




/**
 * @brief Retrieves the number of connection attempts from a given IP address within a specified time window, filtered by transport type.
 *
 * This function queries the internal data structures tracking connection attempts to return the count of attempts
 * made from a specific IP address, using a specified transport protocol, within the given time window. It is
 * designed to support security and monitoring functionalities by enabling the tracking of connection frequencies
 * and patterns. The function ensures thread-safe access to the underlying data structures and performs a time-bound
 * query based on the provided parameters.
 *
 * @param IP A std::string reference representing the IP address for which connection attempts are queried.
 * @param transport The transport type for which the connection attempts are to be counted. This parameter is of the
 *                  enum type eTransportType::eTransportType, which includes various transport protocols such as SSH,
 *                  WebSocket, UDT, etc.
 * @param seconds The size of the time window in seconds, looking back from the current time, within which the
 *                connection attempts are to be counted.
 * @return uint64_t The number of connection attempts made from the specified IP address, using the specified transport
 *         type, within the specified time window. If the IP address is not found in the tracking data structures, or
 *         if no attempts were made within the time window, the function returns 0.
 *
 * @note This function assumes that the connection attempt data is being accurately and timely updated by other components
 *       of the system. It is important that the tracking data structures are maintained with up-to-date information to
 *       ensure the accuracy of the query results.
 * @warning The function uses a locking mechanism to ensure thread-safe access to shared data structures. It is
 *          important to avoid calling this function from within the same critical section that might hold locks on
 *          the involved data structures to prevent deadlocks.
 */
uint64_t CNetworkManager::getConnectionAttemptsFrom(std::string &IP, eTransportType::eTransportType transport, uint64_t seconds)
{
	//Local Variables - BEGIN
	std::shared_ptr<CTools> tools = getTools();
	uint64_t currentTime = tools->getTime();
	uint64_t startTime = currentTime - seconds;
	uint64_t connectionAttempts = 0;
	//Local Variables - END
	

	// Operational Logic - BEGIN
	
	// Convert IP to the same hashed format used in your tracking maps.
	std::vector<uint8_t> ipBytes = tools->stringToBytes(IP);
	ipBytes = CCryptoFactory::getInstance()->getSHA2_256Vec(ipBytes);


	// Lock the mutex to safely access the shared map resource
	std::lock_guard<std::mutex> lock(mConnectionsPerWindowIPsGuardian);

	auto it = mConnectionsPerWindowIPs.find(ipBytes);
	if (it != mConnectionsPerWindowIPs.end())
	{
		// Extract the timestamp of the last reset and the connection attempt counts.
		uint64_t lastReset = std::get<1>(it->second);
		uint64_t connsInTimeWindow = 0;

		// Determine which counter to use based on the transport type
		switch (transport)
		{
		case eTransportType::SSH:
		case eTransportType::WebSocket:
		case eTransportType::UDT:
		case eTransportType::local:
		case eTransportType::Proxy:
		case eTransportType::QUIC:
			connsInTimeWindow = std::get<0>(it->second);
			break;
		case eTransportType::HTTPRequest:
		case eTransportType::HTTPConnection:
			connsInTimeWindow = std::get<2>(it->second);
			break;
		case eTransportType::HTTPAPIRequest:
			connsInTimeWindow = std::get<5>(it->second);
			break;
		}

		// If the last reset timestamp is within the specified window, use the count.
		if (lastReset >= startTime)
		{
			connectionAttempts = connsInTimeWindow;
		}
	}
	// Operational Logic - END
	return connectionAttempts;
}

std::vector <std::string>  CNetworkManager::getLocalIPAdresses()
{
	std::lock_guard<std::mutex> lock(mLocalIPPoolGuardian);
	if (mLocalIPAddresses.size() > 0)
		return mLocalIPAddresses;
	else return std::vector <std::string>{"127.0.0.1"};
}

/**
 * Retrieves the last reported attack timestamp for a given IP address and optionally reports a new attack.
 *
 * This function looks up the last time an attack was reported from the specified IP address.
 * If 'reportIt' is true, it updates the record for this IP address with the current time.
 *
 * @param ip The IP address in string format.
 * @param reportIt A boolean flag to indicate whether to report an attack at the current time.
 * @return The timestamp of the last reported attack from the given IP address.
 *         If no previous attack is recorded, or if 'reportIt' is true, returns the current time.
 *         Returns 0 if 'reportIt' is false and no attack is recorded from the IP.
 */
uint64_t CNetworkManager::getLastAttacksReportedFrom(const std::string& ip, bool reportIt) {

	if (!ip.size())
		return 0;

	std::lock_guard<std::mutex> lock(mIPAttackingReportedAtGuardian);
	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(
		std::vector<uint8_t>(ip.begin(), ip.end()));

	auto it = mAttackReportedFromWhen.find(ipHash);

	if (reportIt) {
		uint64_t currentTime = static_cast<uint64_t>(std::time(0));
		if (it != mAttackReportedFromWhen.end()) {
			// Update the timestamp for the existing entry
			it->second = currentTime;
		}
		else {
			// Create a new entry with the current timestamp
			mAttackReportedFromWhen[ipHash] = currentTime;
		}
		return currentTime;
	}
	else {
		// If the IP is found, return the timestamp, otherwise return 0
		return (it != mAttackReportedFromWhen.end()) ? it->second : 0;
	}
}

/// <summary>
/// We distinguish between node-intrinsic and extrinsic routing.
/// If a datagram targets a virtual terminal or websocket conversation then we employ the routing terminology as well.
/// Routing is needed also for IPv4/6 network inrfaces not available wihtin the local node.
/// Datagram that do not need routing include datagrams without target destination specified at all and those that target locally available 
/// network interfaces. Such datagrams might include ad-hoc data queries which require spawning of a new GridScript VM etc.
/// </summary>
/// <param name="msg"></param>
/// <returns></returns>
bool CNetworkManager::needsRouting(std::shared_ptr<CNetMsg> msg, std::shared_ptr<CEndPoint> receivedby)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mLocalIPPoolGuardian);

	if (msg == nullptr)
		return false;

	if (msg->getDestination().size() == 0)
		return false;//if destination not set; then treated as destined for current conversation (received by)

	std::string destinationID = tools->bytesToString(msg->getDestination());

	if (msg->getDestinationType() == eEndpointType::TerminalID || msg->getDestinationType() == eEndpointType::WebSockConversation
		|| msg->getDestinationType() == eEndpointType::UDTConversation || msg->getDestinationType() == eEndpointType::QUICConversation || msg->getDestinationType() == eEndpointType::WebRTCSwarm)
	{
		if (receivedby != nullptr && msg->getDestinationType() == receivedby->getType() && tools->compareByteVectors(receivedby->getAddress(), msg->getDestination()))
			return false;//it might be a tunneled datagram (wrapper) for another destination.Even so, we need to to pre-precessing
		//i.e. unwrapping within the particular conversation FIRST. The particular conversation object has access to particular ephemeral private keys
		//and/or session keys.

		return true; // <= if targeting these types then that's certain that we need to do INTERNAL routing IF the id is not the same as the current ednpoint
	}

	if (msg->getDestinationType() == eEndpointType::IPv4)
	{

		for (uint64_t i = 0; i < mLocalIPAddresses.size(); i++)
		{
			if (mLocalIPAddresses[i].compare(destinationID) == 0)
				return false;
		}
	}

	return true;
}


// Initialize WFP Firewall
bool CNetworkManager::initializeWFPFirewall() {
	std::lock_guard<std::mutex> lock(mWFPFirewallGuardian);
	
		if (!getIsFirewallKernelModeIntegrationEnabled()) {
		mTools->logEvent("WFP Firewall initialization skipped - kernel mode integration disabled",
			"Network Manager", eLogEntryCategory::network, 5, eLogEntryType::notification);
		return false;
		
	}
	
			// Create WFP firewall module
		mWFPFirewall = std::make_shared<CWFPFirewallModule>(shared_from_this());
	
			// Initialize the module
		if (!mWFPFirewall->initialize()) {
		mTools->logEvent("Failed to initialize WFP Firewall module",
			"Network Manager", eLogEntryCategory::network, 10, eLogEntryType::failure);
		mWFPFirewall.reset();
		return false;
		
	}
	
			// Configure initial thresholds based on global settings
		DosThresholds thresholds;
	thresholds.synFloodThreshold = 100;
	thresholds.udpFloodThreshold = 1000;
	thresholds.connectionRateThreshold = CGlobalSecSettings::getMaxConnPerTimeWindow();
	thresholds.packetRateThreshold = 10000;
	mWFPFirewall->setThresholds(thresholds);
	
			// Start maintenance thread
		//mWFPMaintenanceThread = std::thread(&CNetworkManager::WFPMaintenanceThreadF, this);
	
		mTools->logEvent("WFP Firewall module initialized successfully",
			"Network Manager", eLogEntryCategory::network, 5, eLogEntryType::notification, eColor::lightGreen);
	
		return true;
	
}

// Shutdown WFP Firewall
void CNetworkManager::shutdownWFPFirewall() {
	std::lock_guard<std::mutex> lock(mWFPFirewallGuardian);
	
		if (mWFPFirewall) {
		mWFPFirewall->shutdown();
		mWFPFirewall.reset();
		
			mTools->logEvent("WFP Firewall module shut down",
				"Network Manager", eLogEntryCategory::network, 5, eLogEntryType::notification);
		
	}
	
}

// Get WFP Firewall
std::shared_ptr<CWFPFirewallModule> CNetworkManager::getWFPFirewall() {
	std::lock_guard<std::mutex> lock(mWFPFirewallGuardian);
	return mWFPFirewall;
	
}

// Set WFP Firewall enabled state
void CNetworkManager::setWFPFirewallEnabled(bool enabled) {
	mWFPFirewallEnabled = enabled;
	
		auto firewall = getWFPFirewall();
	if (firewall) {
		firewall->setEnabled(enabled);
		
	}
	
}

// Check if WFP Firewall is enabled
bool CNetworkManager::isWFPFirewallEnabled() const {
	return mWFPFirewallEnabled;
	
}

// Get WFP statistics
void CNetworkManager::getWFPStatistics(std::vector<std::vector<std::string>>&rows) {
	auto firewall = getWFPFirewall();
	if (firewall) {
		firewall->getAttackStatistics(rows);
		
	}
	
}

// Configure WFP thresholds
void CNetworkManager::configureWFPThresholds(const DosThresholds & thresholds) {
	auto firewall = getWFPFirewall();
	if (firewall) {
		firewall->setThresholds(thresholds);
		
	}
	
}


// Report WFP statistics
void CNetworkManager::reportWFPStatistics() {
	auto firewall = getWFPFirewall();
	if (!firewall) {
		return;
		
	}
	
		uint64_t blockedCount = firewall->getBlockedIPCount();
	auto activeAttacks = firewall->getActiveAttacks();
	
		if (blockedCount > 0 || !activeAttacks.empty()) {
		std::stringstream msg;
		msg << "WFP Firewall Status - Blocked IPs: " << blockedCount;
		
			if (!activeAttacks.empty()) {
			msg << ", Active attacks from: ";
			for (size_t i = 0; i < activeAttacks.size(); i) {
				msg << activeAttacks[i];
				if (i < activeAttacks.size() - 1) {
					msg << ", ";
					
				}
				
			}
			
		}
		
			mTools->logEvent(msg.str(), "WFP Firewall", eLogEntryCategory::network,
				5, eLogEntryType::notification);
		
	}
	
}



CNetworkManager::~CNetworkManager()
{
	std::shared_ptr<CTools> tools = getTools();
	mKeepRunning = false; // Ensure controller loop condition can break


	mKeepWFPMaintenanceAlive = false;
	if (mWFPMaintenanceThread.joinable()) {
		mWFPMaintenanceThread.join();
	}
	shutdownWFPFirewall();

	// --- BEGIN FIX 1 (Destructor Safeguard) ---
	if (mKademliaInitThread.joinable()) {
		mTools->logEvent("Destructor: Waiting for Kademlia initialization thread...", "NetworkManager", eLogEntryCategory::localSystem, 1, eLogEntryType::warning);
		mKademliaInitThread.join();
	}
	// --- END FIX 1 (Destructor Safeguard) ---

	if (mChatThread.joinable()) { // Ensure chat thread is joined
		mChatThread.join();
	}
	if (mController.joinable())
		mController.join();

	tools->writeLine("Network Manager killed (destructor);");

}

void CNetworkManager::focusOnIP(const std::string& ip)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mFocusOnIP = ip;
}

bool CNetworkManager::getIsFocusingOnIP(const std::string& ip)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	if (mFocusOnIP.empty())
		return false;

	const_cast<std::string&>(ip) = mFocusOnIP;
	return true;
}

bool CNetworkManager::getIsFocused()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mFocusOnIP.empty() == false;
}

bool  CNetworkManager::canEventLogAboutIP(const std::string& ip)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	if (mFocusOnIP.empty())
		return true;


	if (ip.compare(mFocusOnIP) != 0)
	{
		return false;
	}
	return true;
}


uint64_t CNetworkManager::getEth0RX()
{
	std::lock_guard<std::mutex> lock(mNetworkStatsGuardian);
	return mEth0RX;
}
uint64_t CNetworkManager::getEth0TX()
{
	std::lock_guard<std::mutex> lock(mNetworkStatsGuardian);
	return mEth0TX;
}
uint64_t  CNetworkManager::getLoopbackRX()
{
	std::lock_guard<std::mutex> lock(mNetworkStatsGuardian);
	return mLoopbackRX;
}

uint64_t  CNetworkManager::getLoopbackTX()
{
	std::lock_guard<std::mutex> lock(mNetworkStatsGuardian);
	return mLoopbackTX;
}

void CNetworkManager::incEth0RX(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mNetworkStatsGuardian);
	mEth0RX += value;
}

void CNetworkManager::incEth0TX(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mNetworkStatsGuardian);
	mEth0TX += value;
}
void CNetworkManager::incLoopbackRX(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mNetworkStatsGuardian);
	mLoopbackRX += value;
}

void CNetworkManager::incLoopbackTX(uint64_t value)
{
	std::lock_guard<std::mutex> lock(mNetworkStatsGuardian);
	mLoopbackRX += value;
}

size_t CNetworkManager::getLastTimeChatMsgsRouted()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastTimeChatMsgsRouted;

}
void CNetworkManager::pingLastTimeChatMsgsRouted()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastTimeChatMsgsRouted = std::time(0);
}


uint64_t CNetworkManager::getLastTimeControllerThreadRun()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastControllerLoopRun;

}

void CNetworkManager::pingLastTimeControllerThreadRun()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mLastControllerLoopRun = std::time(0);

}

bool CNetworkManager::getMaintainConnectivityOnlyWithBootstrapNodes()
{
	//Local Variables - BEGIN
	bool connectOnlyWithBootstrapNodes = false;
	uint64_t operatingSince = getTimeSinceOperational();
	uint64_t now = std::time(0);
	std::shared_ptr<CBlockchainManager> bm = getBlockchainManager();
	std::shared_ptr<CBlockHeader> heaviestLeader = bm->getHeaviestChainProofLeader();
	std::shared_ptr<CSettings> settings = bm->getSettings();
	std::shared_ptr<CBlock> verifiedLeader = bm->getCachedLeader();
	//Local Variables - END
	
	// Operational Logic - BEGIN

	if (settings->getForceUsageOfOnlyBootstrapNodes())
		return true;

	//ensure the initial chain-proof is delivered from a Tir 0 node or that a timeout expires.
	if ( getIsNetworkTestMode()==false &&( !heaviestLeader || (heaviestLeader->getHeight() < 3000) // || !verifiedLeader || (verifiedLeader && verifiedLeader->getHeader()->getHeight() < 100)
		&& (operatingSince &&(now- operatingSince) < (60*60*1)))) //after an hour with no heaviest chain proof from bootstrap nodes, connect with anyone
	{
		connectOnlyWithBootstrapNodes = true;
	}

	return connectOnlyWithBootstrapNodes;
	// Operational Logic - END
}

uint64_t CNetworkManager::getLastTimeDSMControllerThreadRun()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastDSMLoopRun;
}

void CNetworkManager::pingLastTimeDSMControllerThreadRun()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastDSMLoopRun = std::time(0);
}

bool CNetworkManager::getKeepChatExchangeAlive()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mKeepChatExchangeAlive;
}

bool CNetworkManager::getIsChatExchangeAlive()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mChatExchangeAlive;
}

void CNetworkManager::setKeepChatExchangeAlive(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mKeepChatExchangeAlive = isIt;
}

void CNetworkManager::setIsChatExchangeAlive(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	 mChatExchangeAlive = isIt;
}

std::shared_ptr<ThreadPool> CNetworkManager::getThreadPool()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mThreadPool;
}

bool CNetworkManager::canProcessBlockNotification(const std::string& ip, bool markAnAttemptIfOK) {
	std::lock_guard<std::mutex> lock(mBlockNotificationsGuardian);

	// Compute the SHA-256 hash of the IP address
	std::vector<uint8_t> ipHash = CCryptoFactory::getInstance()->getSHA2_256Vec(
		std::vector<uint8_t>(ip.begin(), ip.end()));

	// Get the current time
	uint64_t now = static_cast<uint64_t>(std::time(0));

	// Check if the IP address is in the map
	auto it = mLastAllowedBlockNotificationPerIP.find(ipHash);
	if (it != mLastAllowedBlockNotificationPerIP.end()) {
		// Calculate the time elapsed since the last notification for this IP
		uint64_t elapsedTime = now - it->second;

		if (elapsedTime < CGlobalSecSettings::getMaxBlockNotificationsPerIPInterval()) {
			// Not enough time has elapsed
			return false;
		}
	}

	// Enough time has elapsed, or this is the first notification for this IP
	if (markAnAttemptIfOK) {
		// Update the timestamp for this IP address
		mLastAllowedBlockNotificationPerIP[ipHash] = now;
	}

	return true;
}

bool CNetworkManager::getIsBootstrapNode(std::shared_ptr<CEndPoint> ep)
{
	if (!ep)
		return false;

	std::vector<std::string> nodes = CGlobalSecSettings::getBootStrapNodes();
	std::vector<uint8_t> ip = ep->getAddress();
	std::shared_ptr<CTools> tools = getTools();

	for (uint64_t i = 0; i <= nodes.size(); i++)
	{
		if (tools->compareByteVectors(ip, nodes[i]))
		{
			return true;
		}
	}
	return false;
}

bool CNetworkManager::getIsBootstrapNode(const std::string & IP)
{

	std::vector<std::shared_ptr<CEndPoint>>  nodes = getBootstrapNodes();
	std::shared_ptr<CTools> tools = getTools();

	if (tools->validateIPv4(IP) == false)
		return false;

	for (uint64_t i = 0; i < nodes.size(); i++)
	{
		if (tools->compareByteVectors(nodes[i]->getAddress(), const_cast<std::string&>(IP)))
		{
			return true;
		}
	}
	return false;
}

uint64_t CNetworkManager::updateLocalDNSBootstrapNodes(const std::vector<std::string>& newNodes) {
	
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	uint64_t countInserted = 0; // Counter for the number of inserted entries

	// Iterate through the new nodes
	for (const auto& node : newNodes) {
		// Check if the node is already in mDNSBootstrapNodes
		if (std::find(mDNSBootstrapNodes.begin(), mDNSBootstrapNodes.end(), node) == mDNSBootstrapNodes.end()) {
			// If the node is not found, add it to mDNSBootstrapNodes
			mDNSBootstrapNodes.push_back(node);
			++countInserted; // Increment the counter
		}
	}

	return countInserted; // Return the number of inserted entries
}

 std::vector<std::string> CNetworkManager::getDNSBootstrapNodes() {
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mDNSBootstrapNodes;
}

 /**
 * Estimates the total network hash-rate based on the current difficulty.
 *
 * @param difficulty The current mining difficulty of the network.
 * @return Estimated network hash-rate in hashes per second.
 *
 * Note: This function uses the global network constant A factor
 *       and the targeted block interval defined in CGlobalSecSettings.
 */
 double CNetworkManager::estimateNetworkHashrate(double difficulty) {
	 return (difficulty * CGlobalSecSettings::getNetworkAFactor()) / CGlobalSecSettings::getTargetedBlockInterval();
 }

 /**
	 *Estimates the time it will take for a local mining node to successfully mine a block,
	 * given the node's hashrate and the current network difficulty.
	 *
	 *@param localHashrate The hashrate of the local mining node in hashes per second.
	 * @param difficulty The current mining difficulty of the network.
	 * @return Estimated time in seconds for the local node to mine a block.
	 * Returns UINT64_MAX in case of a division by zero error, indicating an error or undefined state.
	 *
	 *This function calculates the estimated mining time based on the following steps :
	 * 1. Calculate the total network hashrate using the `estimateNetworkHashrate` function,
	 * which takes into account the current network difficulty.
	 * 2. Determine the miner's relative power as a ratio of the local hashrate to the network hashrate.
	 * This ratio represents the percentage of the total network mining power contributed by the local node.
	 * 3. Use the blockchain's configured target block interval (obtained from `CGlobalSecSettings::getTargetedBlockInterval`)
	 * to understand the intended average time between blocks.
	 * 4. Estimate the mining time by dividing the average block time by the miner's relative power.
	 * This approach assumes that the likelihood of mining a block is proportional to the miner's share 
	 * of the total network hashrate.
	 *
	 *Note: The function assumes that the network hashrate is non - zero.If the calculated network hashrate is zero,
	 * which could occur in certain edge cases or during initial network setup, the function returns UINT64_MAX
	 * to indicate an error or undefined state.
	 * /
 */

 uint64_t CNetworkManager::estimateMiningTime(double localHashrate, double difficulty) {
	 double networkHashrate = estimateNetworkHashrate(difficulty);
	 if (networkHashrate == 0) return 0; // Avoid division by zero
	 double minerRelativePower = localHashrate / networkHashrate;
	 double averageBlockTime = CGlobalSecSettings::getTargetedBlockInterval(); // 10 minutes in seconds
	 return static_cast<uint64_t>((double)averageBlockTime / (double)minerRelativePower);
 }

 std::vector<std::shared_ptr<CEndPoint>> CNetworkManager::getBootstrapNodes(bool fetchFromDNS, bool onlyEffectiv)
 {
	 std::vector<std::shared_ptr<CEndPoint>> nodes;
	 std::shared_ptr<CTools> tools = getTools();
	 std::vector<std::string> nodeIPs = CGlobalSecSettings::getBootStrapNodes();

	 // DNS Support - BEGIN
	 if (fetchFromDNS)
	 {
		 std::string domain = "bootstrap.gridnet.org";
		 tools->logEvent("Refreshing bootstrap nodes from " + tools->getColoredString(domain, eColor::lightCyan),
			 "DNS", eLogEntryCategory::network, 2);

		 std::vector<std::string> dnsIPs = getDomainIPs(domain);

		 if (dnsIPs.empty())
		 {
			 tools->logEvent("Could not refresh bootstrap nodes from " + tools->getColoredString(domain, eColor::lightCyan), "DNS",
				 eLogEntryCategory::network, 10, eLogEntryType::failure);
		 }
		 else
		 {
			 tools->logEvent("Retrieved " + tools->getColoredString(std::to_string(dnsIPs.size()), eColor::lightGreen) + " bootstrap nodes from " +
				 tools->getColoredString(domain, eColor::lightCyan), "DNS",
				 eLogEntryCategory::network, 10, eLogEntryType::notification);
		 }

		 updateLocalDNSBootstrapNodes(dnsIPs);
	 }

	 // Get a superset and ensure there are no duplicates
	 nodeIPs = tools->getSupersetNoDuplicates(getDNSBootstrapNodes(), nodeIPs);
	 // DNS Support - END

	 // Enforced Single Bootstrap Node Mode - BEGIN
	 if (onlyEffectiv && getHasForcedBootstrapNodesAvailable())
	 {
		 std::lock_guard lock(mBootstrapPeersGuardian);
		 return mForcedBootstrapPeers;
	 }
	 // Enforced Single Bootstrap Node Mode - END

	 // Multi-Bootstrap Nodes Mode (default) - BEGIN

	 std::vector<std::string> forcedIPs;
	 {
		 std::lock_guard lock(mBootstrapPeersGuardian);
		 for (const auto& peer : mForcedBootstrapPeers)
		 {
			 forcedIPs.push_back(tools->bytesToString(peer->getAddress()));
		 }
	 }

	 // Add mForcedBootstrapPeers if not only effective
	 if (!onlyEffectiv)
	 {
		 std::lock_guard lock(mBootstrapPeersGuardian);
		 nodeIPs = tools->getSupersetNoDuplicates(forcedIPs, nodeIPs);
	 }

	 // Add nodes from nodeIPs
	 for (const auto& ip : nodeIPs)
	 {
		 nodes.push_back(std::make_shared<CEndPoint>(
			 tools->stringToBytes(ip), eEndpointType::IPv4, nullptr,
			 CGlobalSecSettings::getDefaultPortNrDataExchange(getBlockchainMode())));
	 }

	 // Multi-Bootstrap Nodes Mode - END

	 return nodes;
 }



void CNetworkManager::pingDNSBooststrapUpdate()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastTimeBootstrapDNSUpdate = std::time(0);
}

uint64_t CNetworkManager::getDNSBooststrapUpdateTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastTimeBootstrapDNSUpdate;
}

uint64_t CNetworkManager::getLastConversationsCleanUp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastConversationsCleanUp;
}

void CNetworkManager::pingLastConversationsCleanUp()
{
	 std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mLastConversationsCleanUp = std::time(0);
}

/**
 * @brief Retrieves the timestamp of the last kernel-mode firewall cleanup operation.
 *
 * This method returns the timestamp (in Unix time format) of when the kernel-mode firewall rules were last cleaned up.
 * The cleanup timestamp helps in tracking and managing the frequency of cleanup operations on firewall rules
 * that are handled in kernel mode for enhanced performance.
 *
 * @return Returns a uint64_t value representing the Unix timestamp of the last cleanup operation.
 *
 * @details
 * The method ensures thread-safe access to the internal timestamp variable through mutex locking.
 * It is typically used for monitoring and administrative purposes to determine the last time the kernel-mode firewall rules were cleaned.
 *
 * @note
 * - This method is part of the network management functionalities focusing on maintaining optimal performance and security of the firewall.
 */
uint64_t CNetworkManager::getLastKernelModeFirewallCleanUpTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastKernelModeFirewallCleanUp;
}

/**
 * @brief Updates the timestamp of the last kernel-mode firewall cleanup to the current time.
 *
 * This method sets the internal record of the last kernel-mode firewall cleanup operation to the current timestamp.
 * This is typically invoked following a cleanup operation to keep track of when the last cleanup occurred.
 *
 * @details
 * The method locks a mutex to ensure thread-safe modification of the internal timestamp variable.
 * It records the time of the invocation as the new timestamp for the last kernel-mode firewall cleanup.
 *
 * @note
 * - Regular updates of this timestamp are crucial for maintaining an accurate schedule and history of cleanup operations.
 * - This method is part of the network management process, ensuring the efficiency and security of kernel-mode firewall rule management.
 */
void CNetworkManager::pingLastKernelModeFirewallCleanUpTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastKernelModeFirewallCleanUp = std::time(0);
}


void CNetworkManager::pingLastConnectivityImprovement()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastConnectivityImprovementAttempt = std::time(0);
}

uint64_t CNetworkManager::getLastConnectivityImprovement()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastConnectivityImprovementAttempt;
}

uint64_t CNetworkManager::getLastNetworkSpecifcsReported()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastTimeNetworkSpecificReported;
}
void CNetworkManager::pingLastNetworkSpecifcsReported()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastTimeNetworkSpecificReported = std::time(0);
}

bool CNetworkManager::getAllowConnectivityOnlyWithBootstrapNodes()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mAllowOnlyBootstrapNodes;
}

void CNetworkManager::setAllowConnectivityOnlyWithBootstrapNodes(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mAllowOnlyBootstrapNodes = doIt;
}


/// <summary>
/// Generates a comprehensive report of all network connections, including both incoming and outgoing attempts.
/// This method provides detailed information about connection durations, statuses, and limits.
/// </summary>
/// <param name="rows">Vector to store the report data as rows of strings.</param>
/// <param name="useColors">Boolean indicating whether to use colored output.</param>
void CNetworkManager::generateComprehensiveConnectionReport(std::vector<std::vector<std::string>>& rows, bool useColors)
{
	//Local Variables - BEGIN
	std::shared_ptr<CTools> tools = getTools();
	uint64_t currentTime = tools->getTime();
	uint64_t timeWindow = 3 * 60 * 60; // 3 hour time-window
	uint64_t maxDurationPerWindow = 30 * 60; // 30 minutes max duration
	std::shared_ptr<CWebSocketsServer> websockServ = getWebSocketsServer();
	std::shared_ptr<CUDTConversationsServer> udtServ = getUDTServer();
	//Local Variables - END

	//Prepare Headers - BEGIN
	std::vector<std::string> headers = {
		"IP Address", "Last Attempt", "Total Duration", "Time Left", "Limit Renews At",
		"Ingress Status", "Egress Status", "General Bucket", "HTTP Bucket",
		"WebSockets", "UDTs", "Last Seen"
	};
	rows.push_back(headers);
	//Prepare Headers - END

	//Gather Connection Data - BEGIN
	std::vector<std::tuple<std::string, std::tuple<uint64_t, uint64_t, uint64_t, std::vector<uint8_t>, uint64_t, uint64_t>>> connectionData;
	{
		std::lock_guard<std::mutex> lock(mFieldsGuardian);
		for (const auto& entry : mConnectionsPerWindowIPs) {
			connectionData.push_back(std::make_tuple(tools->bytesToString(std::get<3>(entry.second)), entry.second));
		}
	}
	//Gather Connection Data - END

	//Process Each Connection - BEGIN
	for (const auto& entry : connectionData) {
		std::vector<std::string> row;
		const std::string& ip = std::get<0>(entry);
		const auto& connectionInfo = std::get<1>(entry);
		std::shared_ptr<CConnTracker> tracker = getConnTracker();
		// IP Address
		row.push_back(ip);

		//Last Attempt - BEGIN
		uint64_t lastAttempt = 0;
		{
			std::lock_guard<std::mutex> lock(mFieldsGuardian);
			auto attemptIt = mLastOutgoingConnectionAttempts.find(ip);
			if (attemptIt != mLastOutgoingConnectionAttempts.end()) {
				lastAttempt = std::max(attemptIt->second->getLastQUICAttempt(), attemptIt->second->getLastUDTAttempt());
			}
		}
		row.push_back(lastAttempt > 0 ? tools->timeToString(lastAttempt) : "N/A");
		//Last Attempt - END

		//Cumulative Connection Duration - BEGIN
		uint64_t connDuration = tracker->getTime(ip, timeWindow);
		row.push_back(tools->secondsToFormattedString(connDuration));
		//Cumulative Connection Duration - END
		bool isWhitelisted = getIsWhitelisted(mTools->stringToBytes(ip));
		//Time Left - BEGIN
		uint64_t timeLeft = (maxDurationPerWindow > connDuration) ? (maxDurationPerWindow - connDuration) : 0;
		std::string timeLeftStr = tools->secondsToFormattedString(timeLeft);
		if (useColors) {

			if (isWhitelisted == false)
			{
				if (timeLeft == 0) {
					timeLeftStr = tools->getColoredString(timeLeftStr, eColor::lightPink);
				}
				else if (timeLeft < 5 * 60) { // Less than 5 minutes
					timeLeftStr = tools->getColoredString(timeLeftStr, eColor::orange);
				}
				else {
					timeLeftStr = tools->getColoredString(timeLeftStr, eColor::lightGreen);
				}
			}
			else
			{
				timeLeftStr = tools->getColoredString("unlimited", eColor::lightGreen);

			}
		}
		row.push_back(timeLeftStr);
		//Time Left - END

		//Limit Renews At - BEGIN
		uint64_t renewTime = tracker->getRenewalTime(ip, timeWindow, maxDurationPerWindow);
		row.push_back(tools->timeToString(renewTime));
		//Limit Renews At - END

		//Connection Statuses - BEGIN

		eIncomingConnectionResult::eIncomingConnectionResult ingressStatus = isConnectionAllowed(ip, false, eTransportType::QUIC, false);
		std::string ingressStatusStr;


		if (!isWhitelisted)
		{
			ingressStatusStr = getConnectionStatusString(ingressStatus);
		}
		else
		{
			ingressStatusStr = "unlimited";
		}
		if (useColors) {

			if (isWhitelisted)
			{
				ingressStatusStr = tools->getColoredString(ingressStatusStr, eColor::neonGreen);
			}
			else if (ingressStatus == eIncomingConnectionResult::allowed) {
				ingressStatusStr = tools->getColoredString(ingressStatusStr, eColor::lightGreen);
			}
			else {
				ingressStatusStr = tools->getColoredString(ingressStatusStr, eColor::lightPink);
			}
		}

		row.push_back(ingressStatusStr);


		bool egressStatus = canAttemptConnection(ip, eTransportType::QUIC, false);
		std::string egressStatusStr;
		if (isWhitelisted == false)
		{
			egressStatusStr = egressStatus ? "Allowed" : "Not Allowed";
		}
		else
		{
			egressStatusStr = "Unlimited";
		}

		if (useColors) {

			if (isWhitelisted)
			{
				egressStatusStr = tools->getColoredString(egressStatusStr, eColor::neonGreen);

			}
			else
			{
				egressStatusStr = tools->getColoredString(egressStatusStr, egressStatus ? eColor::lightGreen : eColor::lightPink);
			}

		}

		row.push_back(egressStatusStr);
		//Connection Statuses - END

		//Connection Buckets - BEGIN
		row.push_back(std::to_string(std::get<0>(connectionInfo))); // General Bucket
		row.push_back(std::to_string(std::get<2>(connectionInfo))); // HTTP Bucket
		//Connection Buckets - END

		//Active Connections - BEGIN
		size_t webSocketCount = websockServ ? websockServ->getConversationsByIP(std::get<3>(connectionInfo)).size() : 0;
		row.push_back(std::to_string(webSocketCount));

		size_t udtCount = udtServ ? udtServ->getConversationsCountByIP(std::get<3>(connectionInfo)) : 0;
		row.push_back(std::to_string(udtCount));
		//Active Connections - END

		//Last Seen - BEGIN
		row.push_back(tools->timeToString(std::get<4>(connectionInfo)));
		//Last Seen - END

		rows.push_back(row);
	}
	//Process Each Connection - END
}

/// <summary>
/// Constructs the Network Manager
/// </summary>
/// <param name="blockchainManager"></param>
/// <param name="isBootStrapNode"></param>
/// <returns></returns>
CNetworkManager::CNetworkManager(std::shared_ptr<CBlockchainManager> blockchainManager, bool isBootStrapNode, bool initializeDTIServer, bool initializeUDTServer, bool initializeKademlia, bool initializeFileServer,
	bool initializeWebServer, bool initializeWebSocketsServer, bool initCORSproxy, bool initializeQUICServer, bool useQUICForSync, bool useUDTForSync)
{


	mLastTimeNetworkSpecificReported = 0;
	if (useQUICForSync)
	{
		enableSyncTransport(eTransportType::QUIC);
	}

	if (useUDTForSync)
	{
		enableSyncTransport(eTransportType::UDT);
	}


	mLastConnectivityImprovementAttempt = 0;
	mFirewallKernelModeIntergation = true;
	mLastConversationsCleanUp = 0;
	mLastKernelModeFirewallCleanUp = 0;
	mUDTNetworkingEnabled = false;
	mQUICNetworkingEnabled = false;
	mDTINetworkingEnabled = false;
	mWebNetworkingEnabled = false;
	mFileSystemNetworkingEnabled = false;
	mWebSocketsNetworkingEnabled = false;
	mLastTimeBootstrapDNSUpdate = 0;
	mConnTracker = std::make_shared<CConnTracker>();
	mMyCustomStatusBarID = CStatusBarHub::getInstance()->getNextCustomStatusBarID(blockchainManager->getMode());
	mLastControllerLoopRun = 0;
	mKeepChatExchangeAlive = true;
	mChatExchangeAlive = false;
	mLastDSMLoopRun = 0;
	mLastTimeBlocksScheduled = 0;
	mNetworkTestMode = false;
	mLastAnalysisStatusBarMode = 0;
	mLastAnalysisStatusBarModeSwitched = 0;
	mLastTimeChatMsgsRouted = 0;
	mCheckpointsRefreshedTimestamp = 0;
	mCurrentBootstrapNodeIndex = 0;//should point to the user-provided bootstrap node.
	mPublicLinkEnabled = true;
	mLoopbackRX = 0;
	mLoopbackTX = 0;
	mEth0RX = 0;
	mEth0TX = 0;
	//Local Variables - BEGIN
	mInitializeCORSProxy = false;
	mMaxNrOfIncConnecinftions = 200;
	mMaxNrOfOutConnections = 200;
	mLastRouterCleanup = 0;
	mFileSystemNetworkingEnabled = false;
	mWebNetworkingEnabled = false;
	mWebSocketsNetworkingEnabled = false;
	mRouterCleanUpInterval = 900;//15 minutes
	assertGN(blockchainManager != nullptr);
	mInitializeFileServer = initializeFileServer;
	mInitializeWebServer = initializeWebServer;
	mInitializeCORSProxy = initCORSproxy;
	mInitializeWebSocketsServer = initializeWebSocketsServer;
	mBlockchainMode = blockchainManager->getMode();
	mTools = std::make_shared<CTools>("Network Manager", mBlockchainMode);
	lastGeneratedTaskID = 0;
	mIsBootstrapNode = isBootStrapNode;
	mTools->writeLine("Initializing the GRIDNET Networking Subsystem..");

	bool retrievedIP = false;
	bool ipFromStorage = false;
	std::shared_ptr<CSettings> settings = blockchainManager->getSettings();
	mAllowOnlyBootstrapNodes = settings->getForceUsageOfOnlyBootstrapNodes();
	//Local Variables - END

	//Local IPv4 - BEGIN
	//Important: that's the IP address local server sockets are bound to.
	//[todo:All]: reconsider the system to suggest 0.0.0.0 as default to avoid potential problems with connectivity.
	//Update: This has been agreed upon on a phone conversation with TheOldWizard [16.04.21], proceeding [Mike].
	//Results: the question whether the Operator wills to be using a custom, specific local interface will be presented
	// IF
	// 	   1) no auto-configuration in progress
	// 	   2) it was not chosen to load settings from storage
	//If an Operator wants to limit the number of interfaces the system listen on he can always do that manually.



	// do we want to enable native operating system firewall integration?
	setIsFirewallKernelModeIntegrationEnabled(settings->getEnableKernelModeFirewallIntegration(true));

	//Auto-Detection - BEGIN
	mTools->writeLine("Auto-Detecting Local IPv4..");
	mLocalIP = autoDetectLocalIP();
	if (mTools->doStringsMatch(mLocalIP, "0.0.0.0"))
	{
		mTools->writeLine(mTools->getColoredString("WARNING:", eColor::cyborgBlood) + " automatic network detection indicates no network connectivity (even local-network)!");
	}
	else
	{
		mTools->writeLine("Auto-Detected Local IPv4:" + mTools->getColoredString(mLocalIP, eColor::lightCyan));
		addLocalIPAddr(mLocalIP);
	}


	//Auto-Detection - END

	// White-listed IPs Support - BEGIN
	bool fromCS = false;
	mWhitelist = settings->getWhitelistedIPs(fromCS);

	std::stringstream message;
	if (fromCS && !mWhitelist.empty())
	{
		message << "White-listed IPs loaded from Cold Storage: ";
		for (const auto& ip : mWhitelist)
		{
			message << "\n" << mTools->getColoredString(mTools->bytesToString(ip), eColor::lightGreen) << "\n";
		}
		mTools->writeLine(mTools->getColoredString(message.str(), eColor::lightCyan));
	}
	else
	{
		mTools->writeLine(mTools->getColoredString("No whitelisted IPs known.", eColor::orange));
	}
	// White-listed IPs Support - END


		//Local-IP MIGHT be overridden by the call below. 
		//also it WOULD if the "Limit interfaces the node listens on" option is not set.
		//the mLocalIP address thus represents an address assigned to the  Eth1 Virtual Interface.
	std::string wildcardIP = "0.0.0.0";


	if (mTools->askYesNo("Limit interfaces the node listens on ?", false))
	{ // during auto-configuration it would answer no anyway
		std::string ipStr;
		uint64_t tries = 0;

		ipStr.clear();
		while (!mTools->validateIPv4(ipStr) && tries < 3)
		{// and it would keep proposing to use the wildcar IPv4 (listen on all local interfaces)
			ipStr = mTools->askString("Which local (true/bound to socket) IPv4 am I to use ?", wildcardIP, "Settings", true);
			tries++;
		}
		settings->setLocalIPv4(ipStr);
		mLocalIP = ipStr;
	}
	else
	{//we are either in auto-configration (in which case getLocalIPv4() would return a wildcard address or an explciit address if present in cold storage settings )

		mLocalIP = settings->getLocalIPv4(ipFromStorage);
	}

	if (!mTools->doStringsMatch("0.0.0.0", mLocalIP))
	{
		mTools->writeLine("I'll be using Local IPv4:" + mTools->getColoredString(mLocalIP, eColor::lightCyan));
		addLocalIPAddr(mLocalIP);
	}
	else
	{
		mTools->writeLine(mTools->getColoredString("I'll be listening on all network interfaces.", eColor::blue));
	}


	//Local IPv4 - END

	//Public IPv4 - BEGIN
	//Important: that's the IP address that is presented to the outside ex. within QR-Intents.
	ipFromStorage = false;


	//Auto-Detection - BEGIN
	// Eth0 Virtual Interface (the one used for QR Intents).
	mTools->writeLine("Auto-Detecting Public IPv4..");
	mPublicIP = autoDetectPublicIP();


	if (mTools->doStringsMatch(mPublicIP, "0.0.0.0"))
	{
		mTools->writeLine(mTools->getColoredString("WARNING:", eColor::cyborgBlood) + " automatic network detection indicates no Internet connectivity!");
	}
	else
	{
		mTools->writeLine("Auto-Detected Public IPv4:" + mTools->getColoredString(mPublicIP, eColor::lightCyan));
		//addLocalIPAddr(mPublicIP);//DO NOT add public IP address to the pool of 'local' addresses as that would prevent valid behind-NAT assessment during bootstrapping
								  // of the UDT sub-system. isIPAddressLocal() check for public IP anyway when we need to check if an IP address belongs to local computer.
	}

	mPublicIP = settings->getPublicIPv4(ipFromStorage, mPublicIP, mLocalIP);//[Mike]:the function takes care of all the validations etc. all by itels no need to introduce redundancy

	mTools->writeLine("I'll be using Public IPv4:" + mTools->getColoredString(mPublicIP, eColor::lightCyan));
	//Public IPv4 - END



	mStatusChange = eManagerStatus::eManagerStatus::initial;
	mStatus = eManagerStatus::eManagerStatus::initial;
	mBlockchainManager = blockchainManager;


	mInitializeDTIServer = initializeDTIServer;
	mInitializeUDTServer = initializeUDTServer;
	mInitializeQUICServer = initializeQUICServer;
	mInitializeKademlia = initializeKademlia;

	mWFPFirewallEnabled = true;
	mLastWFPCleanup = 0;
	mLastWFPStatsReport = 0;
	mKeepWFPMaintenanceAlive = true;

	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	if (mBlockchainMode == eBlockchainMode::TestNet)
	{

		//Bootstrap nodes - BEGIN
		reloadBootstrapNodes();
		//Bootstrap nodes - END
		mTools->writeLine("Whitelisting IPv4 addressess associated with local node..", true, true, eViewState::eventView, "Firewall");

		whitelist(mTools->stringToBytes("127.0.0.1"));
		whitelist(mTools->stringToBytes(mLocalIP));
		whitelist(mTools->stringToBytes(mPublicIP));

	}
}


std::vector<std::string> CNetworkManager::getWhitelistedIPs()
{
	std::vector<std::string> result;
	for (const auto& ipBytes : mWhitelist)
	{
		std::string ipStr = mTools->bytesToString(ipBytes);
		if (!ipStr.empty())
		{
			result.push_back(ipStr);
		}
	}
	return result;
}

/*
   ----------------------------------------------------------------------------
   CNetworkManager::reloadBootstrapNodes(bool interactive)
   ----------------------------------------------------------------------------
   This method re-initializes the internal bootstrap peers list(s). Depending
   on operator-provided IPs and forced mode, it populates either
   mForcedBootstrapPeers or mBootstrapPeers.
   ----------------------------------------------------------------------------
*/
void CNetworkManager::reloadBootstrapNodes()
{
	std::lock_guard lock(mBootstrapPeersGuardian);
	std::shared_ptr<CSettings> settings = mBlockchainManager->getSettings();
	
	// Account For Changed DMZ Eclipsing - BEGIN
	bool currentConnectivityOnlyWithBootstraps = getAllowConnectivityOnlyWithBootstrapNodes();

	if (settings->getForceUsageOfOnlyBootstrapNodes())
	{
		if (!currentConnectivityOnlyWithBootstraps)
		{
			mTools->logEvent("Connectivity allowance policy changed: " +
				mTools->getColoredString("Allowing only bootstrap nodes", eColor::lightGreen),
				eLogEntryCategory::localSystem, 10, eLogEntryType::notification);
			setAllowConnectivityOnlyWithBootstrapNodes(true);
		}
		else
		{
			mTools->logEvent("No allowance policy change: " +
				mTools->getColoredString("Already restricted to bootstrap nodes", eColor::neonPurple),
				eLogEntryCategory::localSystem, 10, eLogEntryType::notification);
		}
	}
	else
	{
		if (currentConnectivityOnlyWithBootstraps)
		{
			mTools->logEvent("Connectivity  allowance policy changed: " +
				mTools->getColoredString("Allowing general connectivity", eColor::lightCyan),
				eLogEntryCategory::localSystem, 10, eLogEntryType::notification);
			setAllowConnectivityOnlyWithBootstrapNodes(false);
		}
		else
		{
			mTools->logEvent("No  allowance policy change: " +
				mTools->getColoredString("Already allowing general connectivity", eColor::greyWhiteBox),
				eLogEntryCategory::localSystem, 10, eLogEntryType::notification);
		}
	}
	// Account For Changed DMZ Eclipsing - END

		
	//--- [1] Clear old data
	mForcedBootstrapPeers.clear();
	mBootstrapPeers.clear();

	//--- [2] Retrieve operator-provided bootstrap nodes from settings.
	//         ipFromStorage tells if these IPs were truly operator-provided
	bool ipFromStorage = false;
	std::vector<std::string> userProvidedBootstrapIpv4 =
		settings->getBootstrapIPv4s(
			ipFromStorage,
			/*interactive=*/false,      // no interactive mode here
			/*forceAskAdditional=*/false,
			{ CGlobalSecSettings::getDefaultBootstrapNode() }
		);

	//--- [3] Determine flags & conditions
	bool hasUserNodes = !userProvidedBootstrapIpv4.empty() && ipFromStorage;
	bool forcedMode = settings->getForceUsageOfOnlyBootstrapNodes();

	// We'll build a final list of IPs to log, then decide which container
	// (mForcedBootstrapPeers or mBootstrapPeers) we fill.
	std::vector<std::string> bootstrapIPs;

	//--- [4] Decide which IP set to use
	if (forcedMode)
	{
		// Exclusive operator-provided IP usage
		bootstrapIPs = userProvidedBootstrapIpv4;
	}
	else if (hasUserNodes)
	{
		// Mixed: operator-provided + official defaults
		bootstrapIPs = CGlobalSecSettings::getBootStrapNodes();
		bootstrapIPs.insert(
			bootstrapIPs.begin(),
			userProvidedBootstrapIpv4.begin(),
			userProvidedBootstrapIpv4.end()
		);
	}
	else
	{
		// Standard default set
		bootstrapIPs = CGlobalSecSettings::getBootStrapNodes();
		// Insert single fallback at front
		bootstrapIPs.insert(
			bootstrapIPs.begin(),
			CGlobalSecSettings::getDefaultBootstrapNode()
		);
	}

	//--- [5] Handle the case of zero IPs
	if (bootstrapIPs.empty())
	{
		std::stringstream msg;
		msg << "No bootstrap nodes known. Networking functionality will be disabled.";
		mTools->writeLine(msg.str());
		pause(); // Optional, depending on environment
		return;
	}

	//--- [6] Logging
	std::stringstream message;
	if (forcedMode)
	{
		// e.g. "Using exclusively operator-provided nodes..."
		if (userProvidedBootstrapIpv4.size() > 1)
		{
			message << "Using exclusively operator-provided bootstrap nodes: ";
		}
		else
		{
			message << "Using exclusively single operator-provided bootstrap node: ";
		}
	}
	else if (hasUserNodes)
	{
		message << "Using mixed bootstrap nodes (operator + default): ";
	}
	else
	{
		message << "Using default bootstrap configuration: ";
	}
	message << "\n\n";

	if (hasUserNodes)
	{
		message << "Operator-provided nodes: ";
		for (size_t i = 0; i < userProvidedBootstrapIpv4.size(); ++i)
		{
			message << mTools->getColoredString(userProvidedBootstrapIpv4[i], eColor::lightCyan);
			if (i < userProvidedBootstrapIpv4.size() - 1)
				message << ", ";
		}
		message << "\n";
	}

	message << "Active bootstrap nodes: ";
	for (size_t i = 0; i < bootstrapIPs.size(); ++i)
	{
		message << mTools->getColoredString(bootstrapIPs[i], eColor::lightCyan);
		if (i < bootstrapIPs.size() - 1)
			message << ", ";
	}
	mTools->writeLine(message.str());

	//--- [7] Populate the correct container
	if (forcedMode)
	{
		// Forced mode => only user-provided IPs
		for (const auto& ip : bootstrapIPs)
		{
			auto ep = std::make_shared<CEndPoint>(
				mTools->stringToBytes(ip),
				eEndpointType::IPv4,
				nullptr,
				CGlobalSecSettings::getDefaultPortNrPeerDiscovery(mBlockchainMode)
			);
			mForcedBootstrapPeers.push_back(ep);
		}
	}
	else
	{
		// Normal mode => use mBootstrapPeers
		for (const auto& ip : bootstrapIPs)
		{
			auto ep = std::make_shared<CEndPoint>(
				mTools->stringToBytes(ip),
				eEndpointType::IPv4,
				nullptr,
				CGlobalSecSettings::getDefaultPortNrPeerDiscovery(mBlockchainMode)
			);
			mBootstrapPeers.push_back(ep);
		}
	}
}


bool  CNetworkManager::getIsBootstrapNode()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mIsBootstrapNode;
}

bool CNetworkManager::getIsFirewallKernelModeIntegrationEnabled()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mFirewallKernelModeIntergation;
}

void CNetworkManager::setIsFirewallKernelModeIntegrationEnabled(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	 mFirewallKernelModeIntergation = isIt;
}

std::shared_ptr<CConnTracker> CNetworkManager::getConnTracker()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mConnTracker;
}

uint64_t CNetworkManager::getLastConnectivityReportTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastConnectivityReportTimestamp;
}

void CNetworkManager::pingLastConnectivityReportTimestamp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mLastConnectivityReportTimestamp = std::time(0);
}

bool CNetworkManager::getIsConnectivityOptimal()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mConnectivityOptimal;
}

void CNetworkManager::setIsConnectivityOptimal(bool isIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mConnectivityOptimal = isIt;
}

void CNetworkManager::pingRetrievingCPFromBootstrapNodesSince()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mTryingToGetGenesisCPFromBootstrapNodesSince = std::time(0);
}

uint64_t CNetworkManager::getRetrievingCPFromBootstrapNodesSince()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mTryingToGetGenesisCPFromBootstrapNodesSince;
}

uint64_t CNetworkManager::getMyCustomStatusBarID()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mMyCustomStatusBarID;
}

/**
 * @brief Initializes the network manager, setting up kernel-mode firewall integration, thread pools, and essential threads.
 *
 * This method performs the initial setup required for the network manager to function correctly. It includes:
 * - Checking and managing kernel-mode firewall integration based on the application's execution context and permissions.
 * - Logging the status of the kernel-mode firewall integration.
 * - Cleaning up kernel-mode firewall rules if the integration is active.
 * - Initializing a thread pool for handling network-related tasks.
 * - Starting controller and chat message exchange threads.
 *
 * @details
 * The method first acquires a lock to ensure thread-safe access to shared resources. It then checks if kernel-mode
 * firewall integration is enabled and whether the application is running with administrator privileges. If kernel-mode
 * integration is enabled but the application is not running as an administrator, it disables the integration for the
 * current session and logs an event. Regardless of the integration status, appropriate log events are recorded.
 *
 * After handling the kernel-mode firewall integration, the method initializes a thread pool with specific parameters
 * for managing network tasks and spawns essential threads for controller operations and chat message exchanges.
 *
 * @note
 * - Kernel-mode firewall integration is an ephemeral setting that does not affect cold storage or persistent configurations.
 * - The method ensures that all operations are thread-safe by using a mutex lock throughout its execution.
 * - This method should be called at the start of the application to ensure that the network manager is correctly set up before use.
 */
void CNetworkManager::initialize()
{
	// Local Variables - BEGIN
	std::shared_ptr<CTools> tools = getTools();
	// Local Variables - END

	// Operational Logic - BEGIN


	
	// Kernel Mode Integration - BEGIN
	if (getIsFirewallKernelModeIntegrationEnabled())
	{
		if (tools->isRunningAsAdmin() == false && tools->isElevatedAndInNetworkConfigurationOperatorsGroup()==false)
		{
			setIsFirewallKernelModeIntegrationEnabled(false); // ephemeral setting not affecting Cold Storage
			tools->logEvent("Disabling Kernel-Mode Firewall Integration - not running as administrator on the native operating system",
				eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
		}

		tools->logEvent("Kernel-Mode Firewall Integration is " +
			tools->getColoredString("ACTIVE", eColor::lightGreen), eLogEntryCategory::localSystem, 10, eLogEntryType::notification);

		tools->logEvent("Cleaning Kernel mode firewall rules..",eLogEntryCategory::localSystem, 10);

		cleanKernelModeRules();
	}
	else
	{
		tools->logEvent("Kernel-Mode Firewall Integration is "+
			tools->getColoredString("DISABLED", eColor::lightPink), eLogEntryCategory::localSystem, 10, eLogEntryType::warning);
	}
	// Kernel Mode Integration - END

	tools->logEvent("Spawning a Thread Pool for Network Manager..");
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	mThreadPool = std::make_shared<ThreadPool>(100,1000,20,ThreadPriority::NORMAL);
	mController = std::thread(&CNetworkManager::mControllerThreadF, this);
	mChatThread = std::thread(&CNetworkManager::exchangeChatMessagesThreadF, this);

	// Operational Logic - END
}
void CNetworkManager::setIsNetworkTestMode(bool isIt)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mNetworkModeGuardian);

	if (mNetworkTestMode != isIt)
	{
		if (isIt)
		{
			tools->writeLine(tools->getColoredString("[Entering the Test/Analysis Mode]", eColor::lightCyan));
		}
		else
		{
			tools->writeLine(tools->getColoredString("[Quitting the Test/Analysis Mode]", eColor::lightCyan));
			clearStatistics();
		}
		mNetworkTestMode = isIt;
		
	}

}
void  CNetworkManager::getCounters(uint64_t& unsolicitedRX, uint64_t& validKADRX, uint64_t& validUDTRX, uint64_t& invalidRX, uint64_t& KADTX, uint64_t& UDTTX,  uint64_t& validQUICRX, uint64_t& QUICTX)
{
	std::lock_guard<std::mutex> lock(mStatisticsGuardian);
	robin_hood::unordered_map<std::vector<uint8_t>, std::tuple<std::vector<uint8_t>, uint64_t>> ::iterator sIt;
	uint64_t datagramCount = 0;

	//Unsolicited RX - BEGIN
	sIt = mUnsolicitedDatagramsReceived.begin();
	while (sIt != mUnsolicitedDatagramsReceived.end())
	{
		datagramCount += std::get<1>(sIt->second);
		++sIt;
	}
	unsolicitedRX = datagramCount;
	//Unsolicited RX - END

	//Valid Kademlia RX - BEGIN
	datagramCount = 0;
	sIt = mValidKademliaDatagramsReceived.begin();
	while (sIt != mValidKademliaDatagramsReceived.end())
	{
		datagramCount += std::get<1>(sIt->second);
		++sIt;
	}
	validKADRX = datagramCount;
	//Valid Kademlia RX - END

	//Valid UDT RX - BEGIN
	datagramCount = 0;
	sIt = mValidUDTDatagramsReceived.begin();
	while (sIt != mValidUDTDatagramsReceived.end())
	{
		datagramCount += std::get<1>(sIt->second);
		++sIt;
	}
	validUDTRX = datagramCount;
	//Valid UDT RX - END

	//Valid QUIC RX - BEGIN
	datagramCount = 0;
	sIt = mValidQUICDatagramsReceived.begin();
	while (sIt != mValidQUICDatagramsReceived.end())
	{
		datagramCount += std::get<1>(sIt->second);
		++sIt;
	}
	validQUICRX = datagramCount;
	//Valid QUIC RX - END

	//General Invalid RX - BEGIN
	datagramCount = 0;
	sIt = mInvalidDatagramsReceived.begin();
	while (sIt != mInvalidDatagramsReceived.end())
	{
		datagramCount += std::get<1>(sIt->second);
		++sIt;
	}
	invalidRX = datagramCount;
	//General Invalid RX- END


	//KAD TX - BEGIN
	datagramCount = 0;
	sIt = mKademliaDatagramsSent.begin();
	while (sIt != mKademliaDatagramsSent.end())
	{
		datagramCount += std::get<1>(sIt->second);
		++sIt;
	}
	KADTX = datagramCount;
	//KAD TX - END

	//UDT TX - BEGIN
	datagramCount = 0;
	sIt = mUDTDatagramsSent.begin();
	while (sIt != mUDTDatagramsSent.end())
	{
		datagramCount += std::get<1>(sIt->second);
		++sIt;
	}
	UDTTX = datagramCount;
	//UDT TX - END

	//QUIC TX - BEGIN
	datagramCount = 0;
	sIt = mQUICDatagramsSent.begin();
	while (sIt != mQUICDatagramsSent.end())
	{
		datagramCount += std::get<1>(sIt->second);
		++sIt;
	}
	QUICTX = datagramCount;
	//QUIC TX - END


	

}
void  CNetworkManager::clearStatistics()
{
	std::lock_guard<std::mutex> lock(mStatisticsGuardian);
	mUnsolicitedDatagramsReceived.clear();
	mValidKademliaDatagramsReceived.clear();
	mValidUDTDatagramsReceived.clear();
	mUDTDatagramsSent.clear();
	mInvalidDatagramsReceived.clear();
	mKademliaDatagramsSent.clear();

}
bool CNetworkManager::getIsNetworkTestMode()
{
	std::lock_guard<std::mutex> lock(mNetworkModeGuardian);
	return mNetworkTestMode;
}


bool CNetworkManager::getHasForcedBootstrapNodesAvailable()
{
	std::lock_guard lock(mBootstrapPeersGuardian);	
	return mForcedBootstrapPeers.empty() == false;
}


bool CNetworkManager::addAndSetBootstrapNode(std::string IP)
{

	std::lock_guard lock(mBootstrapPeersGuardian);
	std::shared_ptr<CTools> tools = getTools();

	//validate IP
	if (!tools->validateIPv4(IP))
	{
		return false;
	}

	// Local Variables - BEGIN
	std::vector<uint8_t> ipB = tools->stringToBytes(IP);

	bool alreadyPresent = false;
	// Local Variables - END

	//Operational Logic - BEGIN

	//check if bootstrap node already known.
	for (uint64_t i = 0; i < mBootstrapPeers.size(); i++)
	{
		if (tools->compareByteVectors(mBootstrapPeers[i]->getAddress(), ipB))
		{
			mCurrentBootstrapNodeIndex = i;
			return true;
		}
	}

	//if not known - add it and set as active.

	std::shared_ptr<CEndPoint> proposal = std::make_shared<CEndPoint>(ipB, eEndpointType::IPv4,nullptr,CGlobalSecSettings::getDefaultPortNrDataExchange(eBlockchainMode::TestNet,false));
	mBootstrapPeers.push_back(proposal);
	mCurrentBootstrapNodeIndex = (mBootstrapPeers.size() - 1);

	return true;
	//Operational Logic - END

}
std::shared_ptr<CEndPoint> CNetworkManager::getBootstrapNode(bool tossNewOne)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard lock(mBootstrapPeersGuardian);
	// Determine which peer list to use
	std::vector<std::shared_ptr<CEndPoint>>& activePeers =
		getHasForcedBootstrapNodesAvailable() ? mForcedBootstrapPeers : mBootstrapPeers;

	// Early return if no peers available
	if (activePeers.empty()) {
		return nullptr;
	}

	// Generate new random index if requested
	if (tossNewOne) {
		tools->logEvent(
			tools->getColoredString("Tossing a new Bootstrap node..", eColor::lightCyan),
			eLogEntryCategory::network,
			0
		);
		mCurrentBootstrapNodeIndex = tools->genRandomNumber(0, activePeers.size() - 1);
	}

	// Clamp mCurrentBootstrapNodeIndex so it is within [0, activePeers.size()-1]
	uint64_t lowerBound = 0ULL;
	uint64_t upperBound = static_cast<uint64_t>(activePeers.size()) - 1ULL;

	mCurrentBootstrapNodeIndex = std::max(mCurrentBootstrapNodeIndex, lowerBound);
	mCurrentBootstrapNodeIndex = min(mCurrentBootstrapNodeIndex, upperBound);

	// Get local addresses for comparison
	std::string publicIP = getPublicIP();
	std::vector<std::string> localAddresses = getLocalIPAdresses();

	// Find suitable bootstrap node
	size_t checkedNodes = 0;
	std::shared_ptr<CEndPoint> proposal = activePeers[mCurrentBootstrapNodeIndex];

	while (checkedNodes < activePeers.size()) {
		bool isInvalidNode = false;

		// Check against local addresses
		for (const auto& localIP : localAddresses) {
			if (tools->compareByteVectors(proposal->getAddress(), tools->stringToBytes(localIP))) {
				isInvalidNode = true;
				break;
			}
		}

		// Check against public IP
		if (!isInvalidNode && tools->compareByteVectors(proposal->getAddress(), publicIP)) {
			isInvalidNode = true;
		}

		// If node is valid, return it
		if (!isInvalidNode) {
			return proposal;
		}

		// Try next node
		mCurrentBootstrapNodeIndex = (mCurrentBootstrapNodeIndex + 1) % activePeers.size();
		proposal = activePeers[mCurrentBootstrapNodeIndex];
		checkedNodes++;
	}

	// If no valid node found after checking all nodes, return the last proposal
	return proposal;
}

std::shared_ptr<CNetTask> CNetworkManager::createTask(eNetTaskType::eNetTaskType type, uint64_t priority, bool registerTaskk, std::vector<uint8_t> data)
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);
	lastGeneratedTaskID++;
	std::shared_ptr<CNetTask> task = std::make_shared<CNetTask>(type, priority);
	if (data.size() > 0)
		task->setData(data);
	if (registerTaskk)
		registerTask(task);
	return task;
}

/// <summary>
/// Retrieves public IP address.
//TODO: make this portable + eliminate reliance on the server
/// </summary>
/// <returns></returns>
std::string CNetworkManager::autoDetectPublicIP()
{
	std::shared_ptr<CTools> tools = getTools();
	httplib::Client cli("api.ipify.org", 80);
	bool gotIt = false;
	cli.set_timeout_sec(10);
	std::string ip;
	cli.set_follow_location(true);

	for (uint64_t i = 0; i < 3 && !gotIt; i++)
	{
		auto res = cli.Get("/");

		if (res && res->status == 200) {
			ip = res->body;
		}

		if (tools->validateIPv4(ip))
		{
			gotIt = true;
			{
				std::lock_guard<std::mutex> lock(mCachedAutoDetectedPublicIPGuardian);
				mCachedAutoDetectedPublicIP = std::vector<uint8_t>(ip.begin(), ip.end());
			}
			return ip;
		}
	}
	return "";
}
std::string CNetworkManager::autoDetectLocalIP()
{
	const char* google_dns_server = "8.8.8.8";
	int dns_port = 53;
	std::string addr;
	struct sockaddr_in serv;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	//Socket could not be created
	if (sock < 0)
	{
		std::cout << "Socket error" << std::endl;
	}

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(google_dns_server);
	serv.sin_port = htons(dns_port);

	int err = connect(sock, (const struct sockaddr*)&serv, sizeof(serv));
	if (err < 0)
	{
		std::cout << "Error number: " << errno
			<< ". Error message: " << strerror(errno) << std::endl;
	}

	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	err = getsockname(sock, (struct sockaddr*)&name, &namelen);

	char buffer[80];
	const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);
	if (p != NULL)
	{
		addr = std::string(p);
		//std::cout << "Local IP address is: " << buffer << std::endl;
	}
	else
	{
		return "unknown";
		//std::cout << "Error number: " << errno
		//	<< ". Error message: " << strerror(errno) << std::endl;
	}

	closesocket(sock);
	return addr;
}

bool CNetworkManager::registerTask(std::shared_ptr<CNetTask> task)
{
	if (!task)
		return false;
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);
	mTasks.push(task);
	return true;
}

std::shared_ptr<CNetTask> CNetworkManager::getCurrentTask(bool deqeue)
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);
	std::shared_ptr<CNetTask> toRet = nullptr;
	if (!mTasks.empty())
	{
		toRet = mTasks.top();
		if (deqeue)
			mTasks.pop();
	}
	return toRet;
}




//It's the high-level brain. 
/*
The function is responsible for distributing tasks among underlying connectivity sub-systems based on the type and nature of a task.
The tasks are distibuted accross active Converstion which might mean:
1) assignment eiter to UDT or websockets conversation
2) routing (also assigned to an active conversation). If a task embedding a CNetMsg ends-up over here (delegated from a Conversation)
// THAT means that it was not destined for the local node and thus an attempt to route it will be made.

Note that we do achieve multi-threaded task processing it's just that the controlling thread is oeprating on one core, distributing tasks
among multiple. Each Conversation might have a core of its own assigned.
*/
eNetTaskProcessingResult::eNetTaskProcessingResult CNetworkManager::processTask(std::shared_ptr<CNetTask> task)
{
	std::shared_ptr<CTools> tools = getTools();
	bool logDebug = getBlockchainManager()->getSettings()->getMinEventPriorityForConsole() == 0;
	if (task == nullptr)
		return eNetTaskProcessingResult::error;

	tools->logEvent("Processing " + task->getDescription() + " NetTask", eLogEntryCategory::network, 0);

	eNetTaskType::eNetTaskType tType = task->getType();
	switch (tType)
	{
		//taken care of at the level of particular Conversations - BEGIN
	case eNetTaskType::startConversation:
		break;
	case eNetTaskType::endConversation:
		break;
	case eNetTaskType::requestBlock:
		break;
	case eNetTaskType::notifyBlock:
		break;
	case eNetTaskType::downloadUpdate:
		break;
	case eNetTaskType::checkForUpdate:
		break;
	case eNetTaskType::sync:
		break;
	case eNetTaskType::sendQRIntentResponse:
		break;
	case eNetTaskType::notifyReceiptID:
		break;
	case eNetTaskType::notifyReceiptBody:
		break;
	case eNetTaskType::awaitReceiptID:
		break;
	case eNetTaskType::sendRAWData:
		break;
	case eNetTaskType::DFSTask:
		break;
	case eNetTaskType::sendVMMetaData:
		break;
	case eNetTaskType::requestData:
		break;

		//taken care of at the level of particular Conversations - End

		//Tasks of Global Scope - BEGIN
		//Network tasks that are outside the scope of specific Conversation's boundaries.
	case eNetTaskType::route:

		if (logDebug)
			tools->logEvent("Routing task in progress..", eLogEntryCategory::eLogEntryCategory::network, 0);

		if (mRouter->route(task->getNetMsg(), task->getOrigin(), task->getConversation()))
		{
			if (logDebug)
				tools->logEvent("Routing task SUCCEEDed", eLogEntryCategory::eLogEntryCategory::network, 0);
			//no explicit notification (efficiency); after some thought... let us notify the other peer on status of the data transit...
			task->getConversation()->notifyOperationStatus(eOperationStatus::success, eOperationScope::dataTransit);
			return eNetTaskProcessingResult::succeeded;
		}
		else
		{
			if (logDebug)
				tools->logEvent("Routing task FAILED. Notifying peer..", eLogEntryCategory::eLogEntryCategory::network, eLogEntryType::failure);

			task->getConversation()->notifyOperationStatus(eOperationStatus::failure, eOperationScope::dataTransit);
			return eNetTaskProcessingResult::aborted;//immediatedly abort failed routing tasks
		}

		break;

		//Tasks of Global Scope - END
	default:
		break;
	}
	return eNetTaskProcessingResult::error;
}
void CNetworkManager::dequeTask()
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);
	if (mTasks.empty())
		return;
	mTasks.pop();
}

/// <summary>
/// If a task ended up within queue it was either
/// 1) issued locally by an upper logic
/// 2)it was dispatched for global, Conversations-wide processing FROM a particular Conversation (e.x. a CNetMsg is to be routed further
/// through another Conversation using DataRouter)
/// </summary>
void CNetworkManager::processNetTasks()
{

	std::shared_ptr<CNetTask> currentTask = getCurrentTask(false);//assess the current Task

	if (currentTask != nullptr)
	{
		eNetTaskProcessingResult::eNetTaskProcessingResult  result = processTask(currentTask);


		if (result == eNetTaskProcessingResult::aborted || result == eNetTaskProcessingResult::succeeded)
			dequeTask();//precessing of the network task already finished
	}
}

std::shared_ptr<CNetTask> CNetworkManager::getTaskByMetaReqID(uint64_t id)
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);

	std::priority_queue<std::shared_ptr<CNetTask>, std::vector<std::shared_ptr<CNetTask>>, CCmpNetTasks> tasks = mTasks;
	std::shared_ptr<CNetTask> toRet = nullptr;
	while (!tasks.empty())
	{
		toRet = tasks.top();
		if (toRet->getMetaRequestID() == id)
			return toRet;

		tasks.pop();
	}
	return toRet;
}

std::shared_ptr<CNetTask> CNetworkManager::getTaskByID(uint64_t id)
{
	std::lock_guard<std::mutex> lock(mTasksQueueGuardian);

	std::priority_queue<std::shared_ptr<CNetTask>, std::vector<std::shared_ptr<CNetTask>>, CCmpNetTasks> tasks = mTasks;
	std::shared_ptr<CNetTask> toRet = nullptr;
	while (!tasks.empty())
	{
		toRet = tasks.top();
		if (toRet->getID() == id)
			return toRet;
		tasks.pop();
	}
	return toRet;
}

std::shared_ptr<CTools> CNetworkManager::getTools()
{
	return mTools;
}

void CNetworkManager::setInitializeDTIServer(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mInitializeDTIServer = doIt;
}



void CNetworkManager::setInitializeQUICServer(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mInitializeQUICServer = doIt;
}

void CNetworkManager::setInitializeFileServer(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mInitializeFileServer = doIt;
}

void CNetworkManager::setInitializeWebSocketsServer(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mInitializeWebSocketsServer = doIt;
}
void CNetworkManager::setInitializeWebServer(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mInitializeWebServer = doIt;
}
void CNetworkManager::setInitializeCORSProxy(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mInitializeCORSProxy = doIt;
}


bool CNetworkManager::getInitializeDTIServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mInitializeDTIServer;
}

bool CNetworkManager::getInitializeUDTServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mInitializeUDTServer;
}
void CNetworkManager::setInitializeUDTServer(bool doIt)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mInitializeUDTServer = doIt;
}

bool CNetworkManager::getInitializeQUICServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mInitializeQUICServer;
}
bool CNetworkManager::getInitializeFileServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mInitializeFileServer;
}

bool CNetworkManager::getInitializeWebServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mInitializeWebServer;
}

bool CNetworkManager::getInitializeCORSProxy()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mInitializeCORSProxy;
}

bool CNetworkManager::getInitializeWebSocketsServer()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mInitializeWebSocketsServer;
}


bool CNetworkManager::getInitializeKademlia()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mInitializeKademlia;
}





/// <summary>
/// Dispatches notification about a new block block to the nearby nodes.
/// The block might be mined by us or we just propagate the information further.
/// Includes a block header; does NOT include the body of a block.
/// Nodes look at the information within the header and decide whether they want to download the block or not.
/// If so, then they send us a download request.
/// </summary>
/// <param name="block"></param>
/// <returns></returns>
CDataPubResult CNetworkManager::publishBlock(std::shared_ptr<CBlock> block)
{
	return CDataPubResult();
}

/// <summary>
/// Publishes a transaction into the GRIDNET Core netowork.
/// </summary>
/// <param name="transaction"></param>
/// <returns></returns>
CDataPubResult CNetworkManager::publishTransaction(CTransaction transaction)
{
	std::shared_ptr<CTools> tools = getTools();
	CDataPubResult res;
	bool publish = false;
	if (!tools->validateTransactionSemantics(transaction))
	{
		res.setResult(eDataPubResult::failure);
		return res;
	}
	// tools->writeLine("Attempting to publish transaction within the Network..");
	 //todo: publish the transaction
	// tools->writeLine("Transaction published!");

	res.setResult(eDataPubResult::success);
	CReceipt rec(transaction, mBlockchainMode);
	res.addReceiptToHotCache(rec);
	return res;
}

/// <summary>
/// Publishes a Verifiable into the Network.
/// </summary>
/// <param name="transaction"></param>
/// <returns></returns>
CDataPubResult CNetworkManager::publishVerifiable(CVerifiable transaction)
{
	return CDataPubResult();
}

CDataPubResult CNetworkManager::publishLeaderInfo(CVerifiable transaction)
{
	return CDataPubResult();
}

CDataPubResult CNetworkManager::publishUpdateAvailable(CVerifiable transaction)
{
	return CDataPubResult();
}

CDataPubResult CNetworkManager::revertToBlock(std::vector<uint8_t> blockID)
{
	return CDataPubResult();
}

CDataPubResult CNetworkManager::banIPs(std::vector<uint8_t> blockID)
{
	return CDataPubResult();
}

/// <summary>
/// Checks for an update the GRIDNET Core software.  The download happens in a P2P mannear.
/// </summary>
/// <returns></returns>
bool CNetworkManager::checkForUpdate()
{
	return false;
}
/// <summary>
/// Download latest version for peer, verify signature.
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CNetworkManager::fetchLatestVersion()
{
	return std::vector<uint8_t>();
}

/// <summary>
/// Stops
/// </summary>
void CNetworkManager::stop()
{
	std::shared_ptr<CTools> tools = getTools();
	tools->logEvent(tools->getColoredString("Received a request to shut down.", eColor::cyborgBlood), "Network Manager", eLogEntryCategory::localSystem);
	
	setKeepChatExchangeAlive(false);
	if (mChatThread.joinable())//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mChatThread.join();

	// --- BEGIN FIX 1 ---
	// FIX: Join the Kademlia initialization thread if it was started.
	if (mKademliaInitThread.joinable()) {
		mTools->logEvent("Waiting for Kademlia initialization thread to complete...", "NetworkManager", eLogEntryCategory::localSystem, 1);
		mKademliaInitThread.join();
		mTools->logEvent("Kademlia initialization thread completed.", "NetworkManager", eLogEntryCategory::localSystem, 1);
	}
	// --- END FIX 1 ---

	mStatusChange = eManagerStatus::eManagerStatus::stopped;
	if (!mController.joinable() && getStatus() != eManagerStatus::eManagerStatus::stopped)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mController = std::thread(&CNetworkManager::mControllerThreadF, this);

	if (mController.get_id() != std::this_thread::get_id())
	{
		while (getStatus() != eManagerStatus::eManagerStatus::stopped)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}	if (mController.joinable())
			mController.join();
	}
	tools->writeLine("Network Manager killed;");
}

/// <summary>
/// Pause the Network Manager.
/// </summary>
void CNetworkManager::pause()
{

	mStatusChange = eManagerStatus::eManagerStatus::paused;

	//socket might be in a listening state. (blocked)
	//for now let's assume it has been paused. improve later on.
	mStatus = eManagerStatus::eManagerStatus::paused;

}
/// <summary>
/// Resume the Network Manager.
/// </summary>
void CNetworkManager::resume()
{
	mStatusChange = eManagerStatus::eManagerStatus::running;
	if (!mController.joinable() && getStatus() != eManagerStatus::eManagerStatus::running)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mController = std::thread(&CNetworkManager::mControllerThreadF, this);
	while (getStatus() != eManagerStatus::eManagerStatus::running && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

}

/// <summary>
/// Gets the status of the Network Manager.
/// </summary>
/// <returns></returns>
eManagerStatus::eManagerStatus CNetworkManager::getStatus()
{
	return mStatus;
}

/// <summary>
/// Sets the status of Network Manager.
/// </summary>
/// <param name="status"></param>
void CNetworkManager::setStatus(eManagerStatus::eManagerStatus status)
{
	std::shared_ptr<CTools> tools = getTools();
	mStatus = status;
	switch (status)
	{
	case eManagerStatus::eManagerStatus::running:

		tools->writeLine("I'm now running");
		break;
	case eManagerStatus::eManagerStatus::paused:
		tools->writeLine(" is now paused");
		break;
	case eManagerStatus::eManagerStatus::stopped:
		tools->writeLine("I'm now stopped");
		break;
	default:
		tools->writeLine("I'm nowin an unknown state;/");
		break;
	}
}

void CNetworkManager::requestStatusChange(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	mStatusChange = status;
}

eManagerStatus::eManagerStatus CNetworkManager::getRequestedStatusChange()
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	return mStatusChange;
}

ConnectionAttemptTimestamps::ConnectionAttemptTimestamps(uint64_t QUICTimestamp, uint64_t UDTTimestamp)
{
		mLastQUICAttempt = QUICTimestamp;
		mLastUDTAttempt = UDTTimestamp;
}

ConnectionAttemptTimestamps::ConnectionAttemptTimestamps(eTransportType::eTransportType transport, uint64_t timestamp)
{
	switch (transport)
	{
	case eTransportType::SSH:
		mLastUDTAttempt = 0;
		mLastQUICAttempt = 0;
		break;
	case eTransportType::WebSocket:
		mLastUDTAttempt = 0;
		mLastQUICAttempt = 0;
		break;
	case eTransportType::UDT:
		mLastUDTAttempt = timestamp;
		mLastQUICAttempt = 0;

		break;
	case eTransportType::local:
		mLastUDTAttempt = 0;
		mLastQUICAttempt = 0;
		break;
	case eTransportType::HTTPRequest:
		mLastUDTAttempt = 0;
		mLastQUICAttempt = 0;
		break;
	case eTransportType::HTTPConnection:
		mLastUDTAttempt = 0;
		mLastQUICAttempt = 0;
		break;
	case eTransportType::Proxy:
		mLastUDTAttempt = 0;
		mLastQUICAttempt = 0;
		break;
	case eTransportType::HTTPAPIRequest:
		mLastUDTAttempt = 0;
		mLastQUICAttempt = 0;
		break;
	case eTransportType::QUIC:
		mLastQUICAttempt = timestamp;
		mLastUDTAttempt = 0;

		break;
	default:
		break;
	}
}
 uint64_t ConnectionAttemptTimestamps::getLastQUICAttempt()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastQUICAttempt;
}

 uint64_t ConnectionAttemptTimestamps::getLastUDTAttempt()
 {
	 std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 return mLastUDTAttempt;
 }

 void ConnectionAttemptTimestamps::pingLastQUICAttempt()
 {
	 std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mLastQUICAttempt = std::time(0);
 }

 void ConnectionAttemptTimestamps::pingLastUDTAttempt()
 {
	 std::lock_guard<std::mutex> lock(mFieldsGuardian);
	 mLastUDTAttempt = std::time(0);
 }

