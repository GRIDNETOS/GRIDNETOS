
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

#include "QUICConversationsServer.h"

#include "CGlobalSecSettings.h"
#include "BlockchainManager.h"
#include "conversation.h"
#include "conversationState.h"
#include "GRIDNET.h"
#include "ThreadPool.h"
#include "QUICConversationsServer.h"


std::string CQUICConversationsServer::getCertPath()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mCertPath;
}
std::string CQUICConversationsServer::getKeyPath()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mPrivKeyPath;
}

/**
 * @brief Initializes the QUIC configuration for the server or client.
 *
 * This function sets up the QUIC configuration with specific settings and
 * credentials. It defines the Application-Layer Protocol Negotiation (ALPN)
 * and sets the idle timeout for the QUIC configuration. For a server, it loads
 * the server's TLS certificate and private key from specified file paths. For
 * a client, it sets the flags to bypass certificate validation. This function
 * is crucial for establishing secure and correctly configured QUIC connections.
 *
 * @param isServer Indicates whether the configuration is for a server (true) or a client (false).
 * @param configuration Reference to the QUIC handle that will be initialized with the configuration.
 *
 * @return Returns true if the configuration was successful, false otherwise.
 *
 * Usage Example:
 *     HQUIC configuration = nullptr;
 *     if (!initializeQUICConfiguration(true, configuration)) {
 *         // Handle failure
 *     }
 *     // Proceed with using the configuration for QUIC operations
 *
 * @note Ensure the certificate and private key paths are valid and accessible.
 *       For client configuration, be aware that certificate validation is bypassed.
 */
bool CQUICConversationsServer::initializeQUICConfiguration(bool isServer, HQUIC& configuration, uint64_t allowedStreamCount) {

	// Local Variables - BEGIN
	QUIC_STATUS Status = QUIC_STATUS_SUCCESS;
	const QUIC_API_TABLE* MsQuic = getQUIC();
	std::string privKeyPath = getKeyPath();
	std::string certPath = getCertPath();
	// Local Variables - END

	// Operational Logic - BEGIN

	// Define the ALPN (Application-Layer Protocol Negotiation)
	const char* Alpn = "GRIDNET";
	QUIC_BUFFER AlpnBuffers = { static_cast<uint32_t>(strlen(Alpn)), (uint8_t*)Alpn };

	// Define QUIC settings
	QUIC_SETTINGS Settings = {};

	Settings.IdleTimeoutMs = 15000;  // 15 seconds idle timeout
	/*This setting specifies the general idle timeout for the entire lifetime of
	the QUIC connection, beyond just the handshake phase. It defines the maximum
	amount of time (in milliseconds) the connection can stay idle (with no packets
	sent or received) before it is considered dead and silently closed.
	*/
	Settings.IsSet.IdleTimeoutMs = TRUE;

	Settings.HandshakeIdleTimeoutMs = 10000;  // 10 seconds connection formation timeout.
	//TODO: consider lowering the handshake time-out.
	Settings.IsSet.HandshakeIdleTimeoutMs = TRUE;
	/*
	The HandshakeIdleTimeoutMs setting defines the maximum amount of time (in
	milliseconds) that the handshake phase can remain idle (without receiving
	any packets) before the connection attempt is aborted. This timeout is
	particularly important for quickly detecting and failing connections that
	cannot be established, possibly due to network issues, configuration errors,
	or malicious attempts that do not complete the handshake.
	*/

	Settings.KeepAliveIntervalMs = 2000; // 2 seconds interval between transport layer keep-alive datagrams.
	Settings.IsSet.KeepAliveIntervalMs = TRUE;

	Settings.PacingEnabled = TRUE;
	Settings.IsSet.PacingEnabled = TRUE;

	Settings.PeerBidiStreamCount = 0; // single stream per connection allowed (we need to sequence all the datagrams, as these are required to be arriving in order). 
	Settings.IsSet.PeerBidiStreamCount = TRUE; // Make sure to mark these settings as set


	Settings.PeerUnidiStreamCount = allowedStreamCount;  // read about how FIN flag is used to optimize delimiting of messages with 'streams'. The reason why we use uni-directional streams.
	Settings.IsSet.PeerUnidiStreamCount = TRUE;

	/*
	* This boolean setting controls whether send pacing is enabled for the
	* connection. Send pacing spreads out packet transmissions to avoid bursts
	* that could lead to congestion and packet loss in the network. Enabling
	* pacing can improve network utilization and reduce congestion-related
	* issues, especially in high-throughput scenarios.
	*/

	//Settings.MaxOperationsPerDrain = 4;

	/* TODO: consider applying the above ^
	*
	* This setting controls the maximum number of operations the library will
	* perform on a single call to the work-draining functions (like processing
	* incoming packets, sending data, etc.). Limiting this value can help ensure
	* that a single connection doesn't monopolize the CPU and allows for fairer
	* scheduling among multiple connections. However, too low a value might
	* impact the throughput and latency of the connection.
	*/


	//Settings.ServerResumptionLevel = QUIC_SERVER_RESUME_AND_ZERORTT; <= TODO: add support for resumption tickets
	// Open the QUIC configuration with the defined ALPN and settings
	Status = MsQuic->ConfigurationOpen(mRegistration, &AlpnBuffers, 1, &Settings, sizeof(Settings), NULL, &configuration);
	if (QUIC_FAILED(Status)) {
		// Handle failure in opening configuration
		return false;
	}

	// Load credentials (TLS configuration)
	// WARNING: by default we are using a self-signed certificate.
	QUIC_CREDENTIAL_CONFIG CredConfig;
	QUIC_CERTIFICATE_FILE CertFile;
	memset(&CredConfig, 0, sizeof(CredConfig));
	memset(&CertFile, 0, sizeof(CertFile));

	if (isServer) {
		// Server-specific settings
		CredConfig.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;

		CertFile.CertificateFile = certPath.c_str();
		CertFile.PrivateKeyFile = privKeyPath.c_str();
		CredConfig.CertificateFile = &CertFile;
	}
	else {
		// Client-specific settings
		CredConfig.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION; // do NOT verify the certificate (MIGHT be self-signed).
	}

	// Apply the credential configuration to the QUIC configuration
	Status = MsQuic->ConfigurationLoadCredential(configuration, &CredConfig);
	if (QUIC_FAILED(Status)) {
		// Handle failure in loading credentials
		MsQuic->ConfigurationClose(configuration);
		return false;
	}
	// Operational Logic - END

	return true;  // Configuration was successful
}


std::string CQUICConversationsServer::getLastConnectionsReport()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastConnectionsReport;
}

void CQUICConversationsServer::setLastConnectionsReport(const std::string& report)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastConnectionsReport = report;
}

const QUIC_API_TABLE* CQUICConversationsServer::getQUIC() const
{
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mFieldsGuardian));

	return mMsQuic;
}

/**
 * @brief Server Listener Callback function for QUIC server events.
 *
 * This function is the callback handler for the QUIC listener events. It handles
 * various events such as new incoming connections and listener stop events. For new
 * connections, it performs several checks like firewall rules, connection limits per IP,
 * and then initiates a conversation if allowed. When the listener is stopped, it handles
 * the necessary cleanup.
 *
 * @param Listener The QUIC listener handle that generated the event.
 * @param Event A pointer to the QUIC_LISTENER_EVENT structure containing details about
 *              the event, including the type of event and event-specific parameters.
 *
 * @return QUIC_STATUS_SUCCESS if the operation completes successfully, or an appropriate
 *         error status as needed.
 *
 * @note This function is registered as a callback in the QUIC API and should conform
 *       to the expected signature and return types.
 *
 * Usage Example:
 *     // Assuming 'Listener' is a valid HQUIC listener handle
 *     getQUIC()->ListenerOpen(Registration, ServerListenerCallback, nullptr, &Listener);
 *     // The callback will now be invoked for listener events
 */
QUIC_STATUS QUIC_API CQUICConversationsServer::ServerListenerCallback(
	_In_ HQUIC Listener,
	_Inout_ QUIC_LISTENER_EVENT* Event
) {

	//Local Variables - BEGIN
	char ClientHost[NI_MAXHOST];
	char ClientService[NI_MAXSERV];
	QUIC_ADDR ClientAddr = {};
	QUIC_STATUS Status = QUIC_STATUS_SUCCESS;
	std::vector<uint8_t> IPBytes;
	uint32_t AddrSize = 0;
	uint64_t alreadyConnected = 0;
	std::string clientIP;
	std::shared_ptr<CConversation> conversation;
	std::shared_ptr<CEndPoint> endPoint;
	std::shared_ptr<CTools>  tools = getTools();
	conversationFlags flags;
	const QUIC_API_TABLE* MsQuic = getQUIC();
	eIncomingConnectionResult::eIncomingConnectionResult connectionResult = eIncomingConnectionResult::allowed;
	//Local Variables - END

	//Operational Logic - BEGIN

	switch (Event->Type) {
	case QUIC_LISTENER_EVENT_NEW_CONNECTION:

		// A new connection is being attempted by a client.
		// Accept the connection and set the callback handler for connection events.
		// Extract the client's address

		AddrSize = sizeof(ClientAddr);
	
		MsQuic->GetParam(Event->NEW_CONNECTION.Connection, QUIC_PARAM_CONN_REMOTE_ADDRESS, &AddrSize, &ClientAddr);
		
		getnameinfo((sockaddr*)&ClientAddr, AddrSize, ClientHost, NI_MAXHOST, ClientService, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

		 clientIP = ClientHost;

		tools->logEvent("New QUIC connection: " + clientIP + ":" + std::string(ClientService), eLogEntryCategory::network, 1);

		connectionResult = getNetworkManager()->isConnectionAllowed(clientIP, true, eTransportType::QUIC, false);

		if (connectionResult != eIncomingConnectionResult::allowed) {
			tools->logEvent("QUIC connection not allowed from: " + clientIP + " Reason: " + describeConnectionResult(connectionResult), "Security", eLogEntryCategory::network, 1);
			MsQuic->ConnectionShutdown(Event->NEW_CONNECTION.Connection, QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT, 0);
			return QUIC_STATUS_ABORTED; // Indicating that the new connection was explicitly rejected.
		}

		mConversationsGuardian.lock();
	     IPBytes = tools->stringToBytes(clientIP);
		 alreadyConnected = std::count_if(mConversations.begin(), mConversations.end(), [&IPBytes, &tools](const auto& conv) {
			return tools->compareByteVectors(conv->getEndpoint()->getAddress(), IPBytes) && conv->getState()->getCurrentState() == eConversationState::running;
			});

		if (alreadyConnected > CGlobalSecSettings::getMaxQUICConnectionsPerIP()) {
			tools->logEvent("Max connections limit reached for IP: " + clientIP, eLogEntryCategory::network, 1);
			mConversationsGuardian.unlock();
			MsQuic->ConnectionShutdown(Event->NEW_CONNECTION.Connection, QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT, 0);
			return QUIC_STATUS_ABORTED;// Indicating that the new connection was explicitly rejected.
		}

		//TODO: consider using eEndpointType::QUICConversation below but ensure internal routing of data works as expected (QR codes)
		endPoint = std::make_shared<CEndPoint>(IPBytes, eEndpointType::IPv4, nullptr, CGlobalSecSettings::getDefaultPortNrDataExchange(mBlockchainMode));
		conversation = std::make_shared<CConversation>(Event->NEW_CONNECTION.Connection, getRegistration(), CBlockchainManager::getInstance(mBlockchainMode), getNetworkManager(), endPoint);
		conversation->initState(conversation);
		flags = conversation->getFlags();// IMPORTANT! Flags were altered in the contructor of CConversation
		flags.isIncoming = true;// so that we know that we are dealing an incoming connection.
		conversation->setFlags(flags);
		

		if (addConversation(conversation)) {

			// Set the connection callback handler.

			//IMPORTANT: the callback function is provided with a RAW pointer.
			/*			 Nonetheless, the object is instantiated through a smart-pointer.
			*			 It is thus of paramount importance to ensure proper life-time of the associated CConversation object.
			*			 The callbacks is thus assigned only after conversation has been added to a collection through addConversation() above.
			*/


	
			MsQuic->SetCallbackHandler(
				Event->NEW_CONNECTION.Connection,
				CConversation::ConnectionCallbackTrampoline, //  connection callback function here accessed through a static trampoline.
				static_cast<void*>(conversation.get())  // Context passed to the connection callback. Here, context a raw pointer to the conversation instantiated over here.
			);

			if (QUIC_FAILED(Status = MsQuic->ConnectionSetConfiguration(Event->NEW_CONNECTION.Connection, mConfiguration)))
			{
				getTools()->logEvent("Failed to set QUIC configuration for connection from: "+ conversation->getIPAddress(), eLogEntryCategory::network, 10);
			}

			conversation->setIsAuthenticationRequired(false);
			conversation->setIsEncryptionRequired(true);
			conversation->startQUICConversation(getThreadPool(), Event->NEW_CONNECTION.Connection, clientIP);
		}

		mConversationsGuardian.unlock();
		break;

	case QUIC_LISTENER_EVENT_STOP_COMPLETE:
		// Listener has been stopped and is now completely cleaned up.
		// Perform any necessary cleanup.
		HandleListenerStop();
		break;
	default:
		break;
	}
	//Operational Logic - END
	return QUIC_STATUS_SUCCESS;
}

void CQUICConversationsServer::HandleListenerStop() {
	// Perform any necessary cleanup after the listener has stopped.
	// This might include freeing resources, logging, or other shutdown procedures.
	getTools()->logEvent("QUIC listener stopped.", eLogEntryCategory::network, 10);

	// If there are additional specific cleanup actions, add them here.
}


std::string CQUICConversationsServer::describeConnectionResult(eIncomingConnectionResult::eIncomingConnectionResult result) {
	switch (result) {
	case eIncomingConnectionResult::allowed:
		return "Connection allowed";
	case eIncomingConnectionResult::DOS:
		return "Connection rejected due to suspected DOS attack";
	case eIncomingConnectionResult::limitReached:
		return "Connection limit for IP reached";
	case eIncomingConnectionResult::onlyBootstrapNodesAllowed:
		return "Only connections from Bootstrap nodes are currently allowed";
	case eIncomingConnectionResult::insufficientResources:
		return "Connection rejected due to insufficient resources";
	default:
		return "Unknown connection result";
	}
}

bool CQUICConversationsServer::initializeQUICRegistration(QUICExecutionProfile::QUICExecutionProfile profile) {
	QUIC_REGISTRATION_CONFIG regConfig = {};
	regConfig.AppName = "GRIDNET Core";

	// Set the execution profile based on the provided argument
	switch (profile) {
	case QUICExecutionProfile::RealTime:
		regConfig.ExecutionProfile = QUIC_EXECUTION_PROFILE_TYPE_REAL_TIME;
		break;
	case QUICExecutionProfile::LowLatency:
		regConfig.ExecutionProfile = QUIC_EXECUTION_PROFILE_LOW_LATENCY;
		break;
	case QUICExecutionProfile::Scavenger:
		regConfig.ExecutionProfile = QUIC_EXECUTION_PROFILE_TYPE_SCAVENGER;
		break;
	case QUICExecutionProfile::HighThroughput:
		regConfig.ExecutionProfile = QUIC_EXECUTION_PROFILE_TYPE_MAX_THROUGHPUT;
		break;
	default:
		// Handle invalid profile, set to default or return false
		regConfig.ExecutionProfile = QUIC_EXECUTION_PROFILE_LOW_LATENCY; // Default profile
		break;
	}

	QUIC_STATUS status = getQUIC()->RegistrationOpen(&regConfig, &mRegistration);
	return QUIC_SUCCEEDED(status);
}


/**
 * @brief Starts a QUIC server listener on a specified IP address.
 *
 * This function initiates a QUIC listener on the provided IP address and port number defined
 * by the `CGlobalSecSettings::getDefaultPortNrDataExchange` function for the specified blockchain mode.
 * It configures the listener with the Application-Layer Protocol Negotiation (ALPN) identifier "GRIDNET"
 * and sets up the necessary QUIC parameters for operation. The function handles both IPv4 and IPv6 addresses
 * and starts the listener to await incoming QUIC connections.
 *
 * @param ipAddress The IP address as a string on which the QUIC server will listen. This can be
 *                  an IPv4 or IPv6 address. The function determines the IP version automatically
 *                  and configures the listener accordingly.
 *
 * Usage Example:
 * @code
 *     CQUICConversationsServer quicServer;
 *     quicServer.StartQuicServer("192.168.1.5");
 * @endcode
 *
 * @note This function sets the QUIC listener to listen on the specified IP address and the default port number
 *       for data exchange as determined for the current blockchain mode. It also ensures that the listener
 *       is properly closed in case of initialization errors.
 *
 * @warning If the provided IP address is invalid or if there are issues starting the listener,
 *          the function logs the error and performs cleanup to avoid resource leaks.
 */
bool CQUICConversationsServer::StartQuicServer(const std::string& ipAddress) {

	// Local Variables - BEGIN
	const char* AlpnString = "GRIDNET";
	QUIC_BUFFER Alpn = { static_cast<uint32_t>(strlen(AlpnString)), reinterpret_cast<uint8_t*>(const_cast<char*>(AlpnString)) };
	QUIC_STATUS Status;
	HQUIC Listener = nullptr;
	QUIC_ADDR Addr = {};
	uint16_t port = CGlobalSecSettings::getDefaultPortNrDataExchange(eBlockchainMode::TestNet, true);
	const QUIC_API_TABLE* MsQuic = getQUIC();
	std::shared_ptr<CTools> tools = getTools();
	// Local Variables - END

	// Operational Logic - BEGIN
	if (QUIC_FAILED(Status = MsQuic->ListenerOpen(getRegistration(), ServerListenerTrampoline, this, &Listener))) {
		tools->logEvent("QUIC ListenerOpen failed.", "Networking (QUIC)", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
		goto ListenerSetupError;
	}

	// Determine IP address version and set address family
	if (ipAddress.find(':') != std::string::npos) {
		// IPv6 address
		QuicAddrSetFamily(&Addr, QUIC_ADDRESS_FAMILY_INET6);
		inet_pton(AF_INET6, ipAddress.c_str(), &Addr.Ipv6.sin6_addr);
	}
	else {
		// IPv4 address
		QuicAddrSetFamily(&Addr, QUIC_ADDRESS_FAMILY_INET);
		inet_pton(AF_INET, ipAddress.c_str(), &Addr.Ipv4.sin_addr);
	}

	QuicAddrSetPort(&Addr, port);

	if (QUIC_FAILED(Status = MsQuic->ListenerStart(Listener, &Alpn, 1, &Addr))) {
		tools->logEvent("QUIC ListenerStart failed.", "Networking (QUIC)", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
		goto ListenerSetupError;
	}

	return true; // Successful start of the listener

ListenerSetupError:
	if (Listener != nullptr) {
		MsQuic->ListenerClose(Listener);
	}
	// Operational Logic - END
	return false;
}



/// <summary>
/// Accepts incoming QUIC connections. Auto-firewall functionality included.
/// </summary>
void CQUICConversationsServer::QUICConversationServerThreadF()
{
	//Local Variables - BEGIN
	sockaddr_storage clientaddr;
	int addrlen = sizeof(clientaddr);
	char clienthost[NI_MAXHOST];
	char clientservice[NI_MAXSERV];
	std::shared_ptr<CNetworkManager> nm = getNetworkManager();
	uint64_t alreadyConnected = 0;
	std::string ip;
	std::shared_ptr<CTools> tools = getTools();

	mTools->SetThreadName("QUIC Conversation Server");

	//accept an incoming connection
	uint64_t now = std::time(0);
	bool DOSattack = false;
	uint64_t lastDOSNotification = 0;
	//Local Variables - END

	//Operational Logic - BEGIN
	while (getStatus() == eManagerStatus::eManagerStatus::running)
	{
		//Refresh Variables - BEGIN
		now = std::time(0);
		DOSattack = false;
		//Refresh Variables - END

		//WARNING: *DO NOT* perform ANY additional logic besides accepting incoming connections over here.
		//		   use mControllerThread instead.
		//START to accept QUIC connections

		Sleep(100);
	}
	//Operational Logic - END
}

bool CQUICConversationsServer::bootUpQUICServerSocket()
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
//Kademlia socket to 0.0.0.0 and QUIC to concrete IP address instead.
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
			ss << "QUIC sub-system needs to use a concrete IPv4 address, not a wild-card address.";
			ss << tools->getColoredString(" OVERRIDING ", eColor::lightPink);
			ss << " IPv4 address assigned to Eth1 (" + tools->getColoredString(IPv4ToUse, eColor::lightCyan) + ") with ";
			ss << tools->getColoredString(localIPs[0], eColor::lightCyan) << ".";

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
			tools->logEvent(std::string("Looks like" + tools->getColoredString(" you are behind a NAT ", eColor::lightPink) + "(" + tools->getColoredString(publicIP, eColor::lightCyan) + ") ! Make sure to route the appropriate ports to your local Virtual Interface Eth1 (" + tools->getColoredString(IPv4ToUse, eColor::lightCyan) + ")."), "Networking (QUIC)", eLogEntryCategory::localSystem, 10);
		}
		else
		{
			tools->logEvent(std::string("Looks like" + tools->getColoredString(" you are not behind a NAT.", eColor::lightGreen) + " The public Virtual Interface Eth0 (" + tools->getColoredString(IPv4ToUse, eColor::lightCyan) + ") is to be assigned to the QUIC subsystem."), "Networking (QUIC)", eLogEntryCategory::localSystem, 10);
		}


		std::string portStr = std::to_string(CGlobalSecSettings::getDefaultPortNrDataExchange(mBlockchainMode, true));

		tools->logEvent("Bootstrapping QUIC Service at " + tools->getColoredString(IPv4ToUse + ":" + portStr, eColor::lightCyan), "Networking (QUIC)", eLogEntryCategory::localSystem, 10);

		Sleep(2000);

		if (0 != getaddrinfo(IPv4ToUse.c_str(), portStr.c_str(), &hints, &res))
		{
			tools->logEvent("Error: Seems like GRIDNET Core Server is already running. I'll quit.", "Networking (QUIC)", eLogEntryCategory::localSystem, 10);
			Sleep(2000);
			return false;
		}

		struct AddrInfoGuard {
			addrinfo* ptr;
			~AddrInfoGuard() {
				if (ptr) freeaddrinfo(ptr);
			}
		} guard{ res };  // This will free res when it goes out of scope

		//mQUICServerSocket = QUIC::socket(AF_INET, res->ai_socktype, 0);


		int timeout = 5000;// static_cast<int>(CGlobalSecSettings::getMaxServerSocketTimeout());
		bool reuseAddr = true;
		bool block = false;
		std::string errorMsg;
		int optRes = 0;

		// here set QUIC server socket timeouts

		// Prepare Buffer Properties - BEGIN

		//	IMPORTANT: these options NEED to be set BEFORE the socket is BOUND (connected / bound).
		//	These settings would be inherited down onto client sockets.
		//  Note: RX buffer size is STATIC. The TX buffer size is allocated dynamically on demand.



		// Prepare Buffer Properties - END

		//Initialize QUIC Registration
		if (!initializeQUICRegistration(QUICExecutionProfile::HighThroughput)) {
			tools->logEvent("Error: QUIC registration failed.", "Networking (QUIC)", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
			return false;
		}

		initializeQUICConfiguration(true, mConfiguration,  CGlobalSecSettings::getAllowedQUICStreamCount());

		//here bind the quick server socket retrieve port by calling uint32_t CGlobalSecSettings::getDefaultPortNrPeerDiscovery(eBlockchainMode::testNet , true)

		tools->writeLine("GRIDNET Core QUIC DataExchange sub-system "+ tools->getColoredString( "open at port: " + std::to_string(CGlobalSecSettings::getDefaultPortNrDataExchange(mBlockchainMode,true)), eColor::lightCyan));

		StartQuicServer(IPv4ToUse);

		// here listen for incoming connections
		return true;
	}
	catch (const std::exception& e) {
		tools->logEvent("Error during QUIC bootstrapping: " + std::string(e.what()), "Networking (QUIC)", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
		Sleep(4000);
		return false;
	}
	catch (...) {
		tools->logEvent("Unknown error during QUIC bootstrapping.", "Networking (QUIC)", eLogEntryCategory::localSystem, 10, eLogEntryType::failure);
		Sleep(4000);
		return false;
	}
	//Operational Logic - END
}
std::shared_ptr<CNetworkManager> CQUICConversationsServer::getNetworkManager()
{
	std::lock_guard<std::mutex> lock(mNetworkManagerGuardian);
	return mNetworkManager;
}
bool CQUICConversationsServer::addConversation(std::shared_ptr<CConversation> convesation)
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

	if (mConversations.size() > CGlobalSecSettings::getMaxSimultaniousQUIConversationCount())
		return false;

	mConversations.push_back(convesation);

	return true;
}

/// <summary>
/// Kills the server-socket.
/// </summary>
void CQUICConversationsServer::killServerSocket()
{
	mTools->writeLine("Killing the QUIC server socket..");
}

bool CQUICConversationsServer::initializeServer() {
	std::shared_ptr<CTools> tools = getTools();
	if (!tools->prepareCertificate(getNetworkManager())) {
		return false;
	}

	mCertPath = (mSS->getMainDataDir() + "\\" + CGlobalSecSettings::getWebCertPath());
	mPrivKeyPath = (mSS->getMainDataDir() + "\\" + CGlobalSecSettings::getWebKeyPath());

	// Temporary non-const pointer for MsQuicOpenVersion
	QUIC_API_TABLE* TempMsQuic = nullptr;

	// Initialize MS QUIC with Version 2
	const uint32_t DesiredVersion = QUIC_API_VERSION_2; // Specify the desired version
	QUIC_STATUS Status = MsQuicOpenVersion(DesiredVersion, (const void**)&TempMsQuic);
	if (QUIC_FAILED(Status)) {
		mTools->writeLine("ERROR: Failed to open MS QUIC with version 2.");
		return false;
	}

	// Assign to the member variable
	mMsQuic = TempMsQuic;

	if (!bootUpQUICServerSocket()) {
		mTools->writeLine("ERROR: I was unable to start the QUIC Server socket.");
		if (mMsQuic) {
			MsQuicClose(mMsQuic);
		}
		return false;
	}

	mControllerThread = std::thread(&CQUICConversationsServer::mControllerThreadF, this);
	return true;
}


HQUIC CQUICConversationsServer::getRegistration()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);

	return mRegistration;
}

std::shared_ptr<ThreadPool> CQUICConversationsServer::getThreadPool()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mThreadPool;
}

uint64_t CQUICConversationsServer::doCleaningUp(bool forceKillAllSessions)
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

uint64_t CQUICConversationsServer::getLastAliveListing()
{
	std::lock_guard<std::mutex> lock(mLastAliveListingGuardian);
	return mLastAliveListingTimestamp;
}

std::shared_ptr<CTools> CQUICConversationsServer::getTools()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mTools;
}


/// <summary>
/// Does cleaning up of QUIC Conversations. 
/// Returns the number of freed communication slots.
/// </summary>
/// <returns></returns>
uint64_t CQUICConversationsServer::cleanConversations()
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

	//Local Variables - BEGIN
	size_t time = mTools->getTime();
	uint64_t removedCount = 0;
	uint64_t eliminatedCount = 0;
	std::string report = tools->getColoredString(std::string("\n-- Registered QUIC Conversations -- \n"), eColor::blue);
	uint64_t lastConvActivity = 0;
	uint64_t timeConvStarted = 0;
	std::string timeStr;
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



	for (std::vector <std::shared_ptr<CConversation>>::iterator it = mConversations.begin(); it != mConversations.end(); ) {

		if (genReport) {
			// Fetch blockchain height from the conversation's lead block info
			auto [blockID, blockHeader] = (*it)->getLeadBlockInfo();
			size_t blockchainHeight = blockHeader ? blockHeader->getHeight() : 0;

			// Fetch notification statistics
			uint64_t totalNotifications = (*it)->getTotalBlockNotifications();
			double avgNotificationInterval = (*it)->getAverageNotificationInterval();

			// Prepare time and status strings
			timeStr = "Started: " + tools->timeToString((*it)->getState()->getTimeStarted()) +
				" Duration: " + tools->secondsToFormattedString((*it)->getState()->getDuration());

			// Build the report string for this conversation
			report += ((*it)->getFlags().isIncoming ? inConn : outConn);
			report += " " + tools->base58CheckEncode((*it)->getID()) + "@" + (*it)->getIPAddress() +
				":" + tools->conversationStateToString((*it)->getState()->getCurrentState()) +
				((*it)->getIsEncryptionAvailable() ? tools->getColoredString(" Sec", eColor::orange) : "") +
				((*it)->getIsSyncEnabled() ? tools->getColoredString(" Sync", eColor::lightCyan) : "") +
				((*it)->getFlags().isMobileApp ? tools->getColoredString(" (mobile)", eColor::blue) : "") +
				" BH: " + std::to_string(blockchainHeight) +
				" Blocks: " + std::to_string(totalNotifications) +
				" Interval: " + tools->doubleToString(avgNotificationInterval,1)+ " sec" +
				" " + timeStr + "\n";
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
				getTools()->logEvent((*it)->getIPAddress() + " exceeded warnings' count. Terminating.. ", "Security", eLogEntryCategory::network, 3, eLogEntryType::warning);
				(*it)->end(false);
			}
			//Security - END
		
			else  if (((*it)->getIsEncryptionAvailable() == false && (*it)->getIsEncryptionRequired()) &&
				((*it)->getDuration()>20))
			{
				tools->logEvent("[FORCED Shutdown] QUIC Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to a secure session timeout.", eLogEntryCategory::network, 1);
				(*it)->end(false);
			}
			else  if (((*it)->getIsSyncEnabled() == false && (*it)->getSyncToBeActive()) &&
				((*it)->getDuration() > 30))// each conversation decides on its own by seeing if dealing with a mobile device
			{
				tools->logEvent("[FORCED Shutdown] QUIC Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to Sync protocol initialisation timeout.", eLogEntryCategory::network, 1);
				(*it)->end(false);
			}
			else  if ((*it)->getState()->getCurrentState() == eConversationState::initializing &&
				(time > lastConvActivity) && ((time - lastConvActivity) > CGlobalSecSettings::getQUICConvInitTimeout()))
			{
				tools->logEvent("[FORCED Shutdown] QUIC Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to a timed-out initialization.", eLogEntryCategory::network, 1);
				(*it)->end(false);
			}
			else if ((*it)->getState()->getCurrentState() != eConversationState::ended &&
				(*it)->getState()->getCurrentState() != eConversationState::running &&
				(time > lastConvActivity) && ((time - lastConvActivity) > CGlobalSecSettings::getOverridedConvShutdownAfter()))
			{
				tools->logEvent("[FORCED Shutdown] QUIC Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to a timed-out idle state.", eLogEntryCategory::network, 1);
				(*it)->end(false);
			}
			else if ((*it)->getState()->getCurrentState() != eConversationState::ended && (time > lastConvActivity) && ((time - lastConvActivity) > CGlobalSecSettings::getOverridedConvShutdownAfter()))
			{
				tools->logEvent("[FORCED Shutdown] QUIC Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to inactivity.", eLogEntryCategory::network, 1);
				(*it)->end(false);
			}

			//Security - BEGIN
			// Anti-excessive connectivity suffocation mechanics. [Part 1] - connection local. 
			// Notice: [Part 2] keeps track of global connection times based on IP addresses.
			// These mechanics thwart overly long connections with same peers.
			// Max Connection Duration  - BEGIN
			uint64_t defferedTimeoutSince = (*it)->getDeferredTimeoutRequestedAt();
			bool isDeferred = (defferedTimeoutSince && (time > defferedTimeoutSince) && ((time - defferedTimeoutSince) < 60 * 10));

			if ((*it)->getDuration() >= (CGlobalSecSettings::getMaxBootstrapConnDuration() * (isDeferred ? 10 : 5)))
			{
				tools->logEvent(tools->getColoredString("[Security]:", eColor::blue) + " Max connection time with " + (*it)->getIPAddress() + " reached. Killing it.", eLogEntryCategory::network, 2);
				((*it))->end(false);
			}
			// Bootstrap Node - END

			//Security - BEGIN
			//Termination Conditions - END
		}

		// Execution - BEGIN

		if (((*it)->getState()->getCurrentState() == eConversationState::ended && ((time > (*it)->getState()->getTimeEnded() && (time - (*it)->getState()->getTimeEnded()) > CGlobalSecSettings::getFreeConversationAfter()))
			|| (*it)->getState()->getCurrentState() == eConversationState::unableToConnect) && (time > lastConvActivity) && ((time - lastConvActivity) > (std::max(CGlobalSecSettings::getQUICConversationInactivityTimeout(), CGlobalSecSettings::getFreeConversationAfter()))))
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

			tools->logEvent("[Freeing] QUIC Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ").", eLogEntryCategory::network, 0);

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

void CQUICConversationsServer::lastAlivePrinted()
{
	std::lock_guard<std::mutex> lock(mLastAliveListingGuardian);
	mLastAliveListingTimestamp = mTools->getTime();
}

size_t CQUICConversationsServer::getActiveSessionsCount()
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
	return mConversations.size();
}

CQUICConversationsServer::~CQUICConversationsServer()
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);


	if (mQUICServerThread.joinable())
		mQUICServerThread.join();


	if (mControllerThread.joinable())
		mControllerThread.join();
}

std::vector<std::shared_ptr<CConversation>> CQUICConversationsServer::getAllConversations()
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
	return mConversations;
}

std::shared_ptr<CConversation> CQUICConversationsServer::getConversationByID(std::vector<uint8_t> id)
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

	for (uint64_t i = 0; i < mConversations.size(); i++)
	{
		if (mTools->compareByteVectors(mConversations[i]->getID(), id))
			return mConversations[i];
	}
	return nullptr;
}

std::shared_ptr<CConversation> CQUICConversationsServer::getConversationByIP(std::vector<uint8_t> IP)
{
	std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
	for (uint64_t i = 0; i < mConversations.size(); i++)
	{
		if (mTools->compareByteVectors(mConversations[i]->getEndpoint()->getAddress(), IP))
			return mConversations[i];
	}
	return nullptr;
}

size_t CQUICConversationsServer::getConversationsCountByIP(std::vector<uint8_t> IP)
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

size_t CQUICConversationsServer::getMaxSessionsCount()
{
	return CGlobalSecSettings::getMaxSimultaniousQUIConversationCount();
}

void  CQUICConversationsServer::setLastTimeCleanedUp(size_t timestamp)
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	mLastTimeCleaned = timestamp != 0 ? (timestamp) : (mTools->getTime());
}

size_t CQUICConversationsServer::getLastTimeCleanedUp()
{
	std::lock_guard<std::mutex> lock(mFieldsGuardian);
	return mLastTimeCleaned;
}


CQUICConversationsServer::CQUICConversationsServer(std::shared_ptr<ThreadPool> pool, std::shared_ptr<CBlockchainManager> bm, std::shared_ptr<CNetworkManager> nm)
{
	mSS = nullptr;
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
	mSS = bm->getSolidStorage();
}

bool CQUICConversationsServer::initialize()
{
	return initializeServer();
}

void CQUICConversationsServer::stop()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);

	if (getStatus() == eManagerStatus::eManagerStatus::stopped)
		return;

	mStatusChange = eManagerStatus::eManagerStatus::stopped;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::stopped)
		mControllerThread = std::thread(&CQUICConversationsServer::mControllerThreadF, this);

	while (getStatus() != eManagerStatus::eManagerStatus::stopped && getStatus() != eManagerStatus::eManagerStatus::initial) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	if (mControllerThread.joinable())
		mControllerThread.join();

	doCleaningUp(true);
	killServerSocket();


	mTools->writeLine("QUIC Conversations Manager killed;");
}

/**
 * @brief Shuts down the QUIC subsystem and releases all associated resources.
 *
 * This function is responsible for cleanly shutting down the QUIC subsystem within the server.
 * It involves closing the QUIC registration handle and releasing all associated resources.
 * This is a crucial step in ensuring that resources are properly deallocated,
 * preventing memory leaks and ensuring that the server can be shut down or restarted without issues.
 *
 * @note This method should be called as part of the server's shutdown sequence,
 * usually when the server is being stopped or when the QUIC functionality is no longer needed.
 *
 * Usage Example:
 *     // Assuming CQUICConversationsServer instance 'quicServer'
 *     quicServer.shutdownQUIC();
 *     // At this point, the QUIC subsystem is cleanly shut down
 */
void CQUICConversationsServer::shutdownQUIC() {

	// Local Variables - BEGIN
	const QUIC_API_TABLE* MsQuic = getQUIC();
	HQUIC registration = getRegistration();
	// Local Variables - END

	// Operational Logic - BEGIN
	if (MsQuic)
	{
		if (registration != nullptr) {
			MsQuic->RegistrationClose(registration);
			registration = nullptr;
		}

		// Clean up the main QUIC sub-system

		MsQuicClose(MsQuic);
		mFieldsGuardian.lock();
		mMsQuic = nullptr;
		mFieldsGuardian.unlock();
	}
	// Operational Logic - END
}

void CQUICConversationsServer::pause()
{
	if (getStatus() == eManagerStatus::eManagerStatus::paused)
		return;

	mStatusChange = eManagerStatus::eManagerStatus::paused;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::paused)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CQUICConversationsServer::mControllerThreadF, this);

	while (getStatus() != eManagerStatus::eManagerStatus::paused && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	doCleaningUp(true);
	mTools->writeLine("QUIC Server paused;");
}

void CQUICConversationsServer::resume()
{
	if (getStatus() == eManagerStatus::eManagerStatus::running)
		return;
	mStatusChange = eManagerStatus::eManagerStatus::running;
	if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::running)//controller is dead; we need first to thwart it for to enable for a state-transmission.
		mControllerThread = std::thread(&CQUICConversationsServer::mControllerThreadF, this);


	while (getStatus() != eManagerStatus::eManagerStatus::running && getStatus() != eManagerStatus::eManagerStatus::initial)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	mTools->writeLine("QUIC Server resumed;");
}

/// <summary>
/// Controls the QUIC Server Manager
/// </summary>
void CQUICConversationsServer::mControllerThreadF()
{
	//Local Variables - BEGIN
	std::string tName = "QUIC-Conversations Controller";
	std::shared_ptr<CTools> tools = getTools();
	uint64_t now = std::time(0);
	tools->SetThreadName(tName.data());
	setStatus(eManagerStatus::eManagerStatus::running);
	bool wasPaused = false;
	if (mStatus != eManagerStatus::eManagerStatus::running)
		setStatus(eManagerStatus::eManagerStatus::running);
	uint64_t justCommitedFromHeaviestChainProof = 0;

	mQUICServerThread = std::thread(&CQUICConversationsServer::QUICConversationServerThreadF, this);
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
					tools->writeLine("Freed " + std::to_string(cleanedCount) + " QUIC Conversations");
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
					tools->writeLine("My thread operations were friezed. Halting..");
					if (mQUICServerThread.native_handle() != 0)
					{
						while (!mQUICServerThread.joinable())
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
						mQUICServerThread.join();
					}

					wasPaused = true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		if (wasPaused)
		{
			tools->writeLine("My thread operations are now resumed. Commencing further..");
			mQUICServerThread = std::thread(&CQUICConversationsServer::QUICConversationServerThreadF, this);
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

eManagerStatus::eManagerStatus CQUICConversationsServer::getStatus()
{
	std::lock_guard<std::recursive_mutex> lock(mStatusGuardian);
	return mStatus;
}


void CQUICConversationsServer::setStatus(eManagerStatus::eManagerStatus status)
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

void CQUICConversationsServer::requestStatusChange(eManagerStatus::eManagerStatus status)
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	mStatusChange = status;
}

eManagerStatus::eManagerStatus CQUICConversationsServer::getRequestedStatusChange()
{
	std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
	return mStatusChange;
}

