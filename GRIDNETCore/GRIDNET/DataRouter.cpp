#include "DataRouter.h"
#include "RTEntry.h"
#include "EEndPoint.h"
#include "NetMsg.h"
#include "NetworkManager.h"
#include "DTIServer.h"
#include "DTI.h"
#include "conversation.h"
#include "UDTConversationsServer.h"
#include "WebSocketServer.h"
#include "TransmissionToken.h"
#include "conversationState.h"
#include "BlockchainManager.h" 
#include "webRTCSwarm.h"
#include "webRTCSwarmMember.h"
#include "QUICConversationsServer.h"

std::shared_ptr<CTools> CDataRouter::getTools()
{
	std::lock_guard<std::mutex> tools(mToolsGuardian);
	return mTools;
}

bool CDataRouter::addRTEntry(std::shared_ptr<CRTEntry> entry)
{
	std::lock_guard<std::mutex> lock(mRTGuardian);
	if (!entry)
	{
		return false;
	}
	if (getLogNetworkDebugEvents())
	{
		getTools()->logEvent(getTools()->getColoredString("Adding RT entry: ", eColor::lightCyan) + entry->getDescription(),eLogEntryCategory::network, 0);
	}

	mEntries.push_back(entry);

	return true;
}

bool CDataRouter::removeRTEntry(uint64_t id)
{
	std::lock_guard<std::mutex> lock(mRTGuardian);
	std::vector<std::shared_ptr<CRTEntry>>::iterator it = mEntries.begin();
	bool log =getLogNetworkDebugEvents();
	while (it != mEntries.end()) {

		if ((*it)->getID()==id) {
			if (log)
			{
				getTools()->logEvent("Removing RT entry: " + (*it)->getDescription(), eLogEntryCategory::network, 0);
			}
			it = mEntries.erase(it);
			return true;
		}
		else ++it;
	}
	return false;
}

void CDataRouter::initFields()
{
	mCleanUpEverySec = 0;
	mLastCleanUp = 0;
	mFreePropagationsCount = 1;
	mMinPropagationReward = 0;//be altruistic during tests
	mLogNetworkDebugEvents = false;
	mSettingsLoaded = false;//these are cached within the Router for performance. Determined once (on first routing request) before becoming operational.
}

void CDataRouter::loadSettings(bool forceRenewal)
{
	std::lock_guard<std::mutex> lock(mSettingsGuardian);

	if (mSettingsLoaded && !forceRenewal)
		return;

	std::shared_ptr<CNetworkManager> nm = mNetworkManager.lock();
	if (nm == nullptr)
		return;

	mLogNetworkDebugEvents = CBlockchainManager::getInstance(nm->getBlockchainMode())->getSettings()->getLogEvents(eLogEntryCategory::network, eLogEntryType::notification);
	mLocalID = nm->getLocalID();
	mSettingsLoaded = true;
}

bool CDataRouter::getLogNetworkDebugEvents()
{
	std::lock_guard<std::mutex> lock(mSettingsGuardian);
	return mLogNetworkDebugEvents;
}

uint64_t CDataRouter::getMinPropagationReward()
{
	std::lock_guard<std::mutex> lock(mSettingsGuardian);
	return mMinPropagationReward;
}

void CDataRouter::addCollectedTT(std::shared_ptr<CTransmissionToken> tt)
{
	std::lock_guard<std::mutex> lock(mCollectedTTsGuardian);
	
	mCollectedTTs.push_back(tt);
}

std::vector<std::shared_ptr<CTransmissionToken>> CDataRouter::getCollectedTTs()
{
	std::lock_guard<std::mutex> lock(mCollectedTTsGuardian);
	return mCollectedTTs;
}

/// <summary>
/// Attempts to collect reward (Transmission Token) from a routed Datagram (CNetMsg container).
/// Transmission Tokens deemed as not suitable (cashing out on-the-chain not possible) for local peer are neglected.
/// The function is NOT responsible for an in-depth on-the chain TT validation. The function is performance oriented to be invoked
/// on per-routing request basis. The function MAY perform additional checks (affecting performance, such as signature validation when one available) -
/// performed when 'doChecks' is set.
/// </summary>
/// <param name="msg"></param>
/// <param name="reportedValue"></param>
/// <returns></returns>
eTTCollectionResult::eTTCollectionResult CDataRouter::collectTT(std::shared_ptr<CNetMsg> msg, const uint64_t& reportedValue, bool doChecks)
{
	std::shared_ptr<CTools> tools = getTools();
	//Local Variables - BEGIN
	eTTCollectionResult::eTTCollectionResult resultStatus = eTTCollectionResult::noTT;
	std::shared_ptr<CTransmissionToken> tt = msg->getTT();
	bool isValidTT = false;
	std::vector<uint8_t> recipient;
	std::vector<uint8_t> localID = getLocalID();
	//Local Variables - END

	if (tt != nullptr)
	{	//Operational Logic - BEGIN
		recipient = tt->getRecipient();
			//Initial TT Validation - BEGIN
		if (recipient.size() > 0)
		{//dealing with a targeted transmission-token
			if (!tools->compareByteVectors(localID, recipient))
			{
				return eTTCollectionResult::notForMe;
			}
		}
		if (doChecks)
		{
			if (!tt->validate())
			{
				return eTTCollectionResult::invalidTT;
			}
		}
			//Initial TT Validation - END

		if (isValidTT)
		{
			addCollectedTT(tt);
			return eTTCollectionResult::collected;
		}
		//Operational Logic - END
	}

	return resultStatus;
}

std::vector<uint8_t> CDataRouter::getLocalID()
{
	std::lock_guard<std::mutex> lock(mSettingsGuardian);
	return mLocalID;
}

/// <summary>
/// Removes entries with a given EndPoint set as next-hop.
/// Rationalization: used when a given conversations ends.
/// </summary>
/// <param name="nextHop"></param>
/// <returns>The number of removed routing table entries.</returns>
uint64_t CDataRouter::removeEntriesWithNextHop(std::shared_ptr<CEndPoint> nextHop)
{
	
	if (nextHop == nullptr)
		return 0;

	//Local Variables - BEGIN
	std::lock_guard<std::mutex> lock(mRTGuardian);
	uint64_t removedCount = 0;
	std::vector<std::shared_ptr<CRTEntry>>::iterator it = mEntries.begin();
	//Local Variables - END

	//Operational Logic - BEGIN

	while (it != mEntries.end())
	{
		//does the next hop match?
		if ((*it) == nullptr)
		{
			it = mEntries.erase(it);
		}
		else if ((*(*it)->getNextHop()) == (*nextHop))
		{
			it = mEntries.erase(it);
			removedCount++;
			//it does
		}
		//if not, proceed further
		else
		{
			++it;
		}

	}

	//Operational Logic - END

	return removedCount;
}

CDataRouter::CDataRouter(std::weak_ptr<CNetworkManager> networkManager,uint64_t cleanupEverySec)
{
	initFields();
	
	mNetworkManager = networkManager;
	if (auto nm = networkManager.lock())
	{
		mTools = nm->getTools();
	}
	loadSettings();
}

/// <summary>
/// Updates routing table based on the provided CNetMsg and information describing from where it was received
/// (immediateSource and immediateSourceID) parameters.
/// Note: some source types may require additional positive verification to be accounted for.
/// </summary>
/// <param name="msg"></param>
/// <param name="immediateSource"></param>
/// <param name="immediateSourceID"></param>
/// <returns></returns>
bool CDataRouter::updateRT(std::shared_ptr<CNetMsg> msg, std::shared_ptr<CEndPoint> immediateSource)
{
	std::shared_ptr<CTools> tools = getTools();
	if (immediateSource == nullptr || msg == nullptr)
		return false;

	//Local variables - BEGIN
	std::vector<uint8_t> peerID;
	std::shared_ptr<CRTEntry> entry;
	std::shared_ptr<CEndPoint> target;
	eRTUpdateReason::eRTUpdateReason updateReason = eRTUpdateReason::newPeer;
	bool updated = false;
	//Local variables - END

	//Initial Validation - BEGIN
	if (msg == nullptr)
		return false;

	if (msg->getSource().size() == 0)
		return false;
	//Initial Validation - END

	//Operational Logic - BEGIN

	//Authorization - BEGIN (optional, based on source-type)
	if (msg->getSourceType() == eEndpointType::PeerID)
	{
		//signature verification required for this type of an entry
		/*
			Recall that Peer-ID routing table entries can be established in two ways:
			1) transport layer, strongly-authenticated CNetMsg containers (attempted over here), OR
			2) through session layer CSessionDescription containers (taking place during the initial handshake).
			   During Hello Request a IV vector is requested to be signed trough a private key corresponding to a public key
			   one present within of the Session Description. Only once delivered (pas part of Hello Notify) do we agree to create a corresponding Routing Table entry.

			   [ PREFERENCE ]: we perefer 2) since 1) creates too much overhead. Session Desription is delivered only once in contrast with enabling strong per-datagram authentication.
		*/

		/*
			This tipe of an entry requires an IDENTITY-public key to be present within the datagram's source field.
			Valid signature made with a coresponding private key should be present within mSig field.
			Currently this routing mechanism is triggered only through Hello-Requests from mobile token app.
		*/


		if (msg->getSource().size() != 32)
			return false;//a valid x25519 public key required

		if (!msg->verifySig(msg->getSource()))
		{
			return false;
		}

		//generate the actual user id based on a pub-key - that will go into the routing table.

		std::vector<uint8_t> source = msg->getSource(); //source needs to be a valid public key
		if (source.size()!=32 || !CCryptoFactory::getInstance()->genAddress(source, peerID))
			return false;
	}
	//Authorization - END

	entry = findEntryForDest(msg->getSource(), msg->getSourceType());
	//notify the Routing Table that source of CNetMsg is reachable through the 'immediateSource', additionally increasing the hop-count value within 
	//the coresponding entry.
    target = std::make_shared<CEndPoint>(peerID.size()>0 ? peerID: msg->getSource(), msg->getSourceType(), nullptr, 1);//target destination of a routing-table entry.

	if (entry == nullptr)
	{//no entry within RT as of yet
		entry = std::make_shared<CRTEntry>(immediateSource, target, msg->getHops()+1);
		entry->setDstSeq(msg->getSourceSeq());
		addRTEntry(entry);
		return true;
	}
	else
	{
		if (entry->getHops() > (msg->getHops() + 1))
		{

			if (getLogNetworkDebugEvents())
			{

				updateReason = eRTUpdateReason::shorterPath;
				tools->logEvent("[RT Update Reason]: " + tools->rtUpdateReasonToStr(updateReason) + ". Previous: " + std::to_string(entry->getHops()) + " Now: " +
					std::to_string(msg->getHops() + 1), eLogEntryCategory::network, 0);
			}


			updated = addRTEntry(entry);//insert new new neighboor only if better than the known one
			//consider updating the entry: no need, multiple paths may exist shortest always chosen; just insert new neighboor
		}
		else if ((*entry->getNextHop()) == *immediateSource)//we can only UPDATE if the found entry is regarding the immediateSource
		{//as a result no duplicate entries regarding next-hops for same destination should ever exist.
			//when a new datagram with newer source destination is encountered, the destination sequence number within routing table is updated.
			if (entry->getDstSeq() < (msg->getSourceSeq()))
			{
				//in that case we update the path instead of inserting a new one
				if (getLogNetworkDebugEvents())
				{

					updateReason = eRTUpdateReason::newerPath;
					tools->logEvent("[RT Update Reason]: " + tools->rtUpdateReasonToStr(updateReason) + ". Previous: " + std::to_string(entry->getDstSeq()) + " Now: " +
						std::to_string(msg->getSourceSeq()), eLogEntryCategory::network, 0);
				}

				//check it's a path through the same EndPoint. just update the destination seq number
				//entry->setNextHop(immediateSource);  <=redundant; we wouldn't be here otherwise.

				entry->setDstSeq(msg->getSourceSeq());//update the destination sequence within the routingt table.
			}

		}
		else
		{
			if (getLogNetworkDebugEvents())
			tools->logEvent("Routing table won't be updated datagram from a different immediate peer.", eLogEntryCategory::network, 0);
		}
	}


		if(!updated)
		{
			if(getLogNetworkDebugEvents())
			tools->logEvent("Routing table won't be updated - a path exists which is at least as good.", eLogEntryCategory::network, 0);
		}
		
	//Operational Logic - END

	return false;
}

/// <summary>
/// Updates routing table based on the provided destination,coresponding hop-count, and immediatePeer.
/// Note, endpoints might be virtual (like a WebSock, UDR or WebRTC Swarm conversation).
/// (immediateSource and immediateSourceID) parameters.
/// Note: This function compares timestamps instead of source sequence numbers available within data-packets when doing routing table updates.
/// </summary>
/// <param name="msg"></param>
/// <param name="immediateSource"></param>
/// <param name="immediateSourceID"></param>
/// <returns></returns>
bool CDataRouter::updateRT(std::shared_ptr<CEndPoint> destination, std::shared_ptr<CEndPoint> immediatePeer,uint64_t hopCount, eRouteKowledgeSource::eRouteKowledgeSource knowledgeSource)
{
	std::shared_ptr<CTools> tools = getTools();
	if (destination == nullptr || immediatePeer== nullptr)
		return false;

	//Local Variables - BEGIN
	uint64_t timestampNow = tools->getTime();
	std::shared_ptr<CRTEntry> entry = findEntryForDest(destination->getAddress(), destination->getType());
	eRTUpdateReason::eRTUpdateReason updateReason = eRTUpdateReason::newPeer;
	bool updated = false;
	//Local Variables - END

	//notify the Routing Table that source of CNetMsg is reachable through the 'immediateSource', additionally increasing the hop-count value within 
	//the coresponding entry.
	//std::shared_ptr<CEndPoint> target = std::make_shared<CEndPoint>(destination->getAddress(), msg->getSourceType(), nullptr, 1);//target destination of a routing-table entry.

	if (entry == nullptr)
	{//no entry within RT as of yet
		entry = std::make_shared<CRTEntry>(immediatePeer, destination, hopCount, knowledgeSource);
		entry->setDstPubKey(destination->getPubKey());
		addRTEntry(entry);
		return true;
	}
	else
	{
		
		if (entry->getHops() > (hopCount + 1))
		{

			if (getLogNetworkDebugEvents())
			{

				updateReason = eRTUpdateReason::shorterPath;
				tools->logEvent("[RT Update Reason]: " + tools->rtUpdateReasonToStr(updateReason) + ". Previous: " + std::to_string(entry->getHops()) + " Now: " +
					std::to_string(hopCount + 1), eLogEntryCategory::network, 0);
			}

			updated = addRTEntry(entry);//insert new new neighboor only if better than the known one
			//consider updating the entry: no need, multiple paths may exist shortest always chosen; just insert new neighboor
		}
		else if ((*entry->getNextHop()) == *immediatePeer)//we can only UPDATE if the found entry is regarding the immediateSource
		{//as a result no duplicate entries regarding next-hops for same destination should ever exist.
			//when a new datagram with newer source destination is encountered, the destination sequence number within routing table is updated.
			
			if (entry->getTimestamp() <= timestampNow)
			{
				/*
				Rationalization: this function gets exectued by system-intrinsic mechanics. Thus, here we can rely on the concept of time.
				When datagrams are received from network, there instead - we rely on sequence numbers.
				*/

				//in that case we update the path instead of inserting a new one
				if (getLogNetworkDebugEvents())
				{

					//updateReason = eRTUpdateReason::newerPath;
					//tools->logEvent("[RT Update Reason]: " + tools->rtUpdateReasonToStr(updateReason) + ". Previous: " + std::to_string(entry->getDstSeq()) + " Now: " +
					//	std::to_string(timestampNow), eLogEntryCategory::network, 0);
				}
				/*if (entry->getKnowledgeSource() == eRouteKowledgeSource::propagation &&
			knowledgeSource != eRouteKowledgeSource::propagation )
		{
			tools->logEvent("[RT Update Reason]: Updating RT knowledge source for "+ entry->getDescription() + " to a more specific: "+ tools->routeKowledgeSourceToString(knowledgeSource) + ". Previous: " + tools->routeKowledgeSourceToString(entry->getKnowledgeSource()), eLogEntryCategory::network, 0);
		}*/

				//check it's a path through the same EndPoint. just update the destination seq number
				//entry->setNextHop(immediateSource);  <=redundant; we wouldn't be here otherwise.

				//UPDATE: as of new we simply refresh the entry.
				entry->ping();
				//entry->setT(msg->getSourceSeq());//update the destination sequence within the routingt table.
			}

		}
		else
		{
			if (getLogNetworkDebugEvents())
				tools->logEvent("Routing table won't be updated datagram from a different immediate peer.", eLogEntryCategory::network, 0);
		}

	}

	return false;
}

/// <summary>
/// The function constructs Transit Pools, to be cashed out on-the-chain.
/// The function returns the amount of confirmed GNC received.
/// Todo: process invalid results, ban those peers.
/// </summary>
/// <returns></returns>
uint64_t CDataRouter::cashOutRewards()
{
	//Local Variables - BEGIN

	uint64_t receivedGNC = 0;
	//Local Variables - END

	//Operational Logic - BEGIN
	//Operational Logic - END
	return receivedGNC;
}

/// <summary>
/// Returns description of the current router's state.
/// </summary>
/// <param name="includeRT">Prints the routing table.</param>
/// <returns></returns>
std::string CDataRouter::getDescription(bool includeRT, bool extendedRT,bool checkAvailability, bool includeKadPeers, bool showKnowledgeSource, bool showPeerIDs, std::string newLine, eColumnFieldAlignment::eColumnFieldAlignment alignment, uint64_t maxWidth)
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mRTGuardian);
	//Local Variables - BEGIN
	std::string toRet;
	std::vector<uint8_t> pubKey, dstID;
	std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
	//Local Variables - END

	if (includeRT)
	{
		std::vector <std::string> descriptors;
		std::vector <std::vector<std::string>> rows;
		std::vector<std::string> row;

		row.push_back("Destination");
		row.push_back("Gateway");
		row.push_back("Distance");
		row.push_back("Timestamp");
		if(showKnowledgeSource)
	    row.push_back("Result");

		if (checkAvailability)
		{
			row.push_back("Alive");
		}
		if (extendedRT)
		{
			row.push_back("DstSeq");
			row.push_back("SrcSeq");
		}
		if (showPeerIDs)
		{
			row.push_back("ID");
		}
		rows.push_back(row);
		//std::string header = tools->genTableLine(descriptors);
		//toRet += header+"\n";
		std::lock_guard<std::mutex> lock(mRTGuardian);
		for (uint64_t i = 0; i < mEntries.size(); i++)
		{
			row.clear();
			row.push_back(mEntries[i]->getDst()->getDescription());
			row.push_back(mEntries[i]->getNextHop()->getDescription());
			row.push_back(std::to_string(mEntries[i]->getHops()));
			char mbstr[100];
			time_t ts = mEntries[i]->getTimestamp();

			if(std::strftime(mbstr, sizeof(mbstr), "%T", std::localtime(&ts))) {
				row.push_back(std::string(mbstr));
			}

			if (showKnowledgeSource)
				row.push_back(tools->routeKowledgeSourceToString(mEntries[i]->getKnowledgeSource()));

			if (checkAvailability)
			{
				row.push_back(isRTEntryReachable(mEntries[i]) ? "True" : "False");
			}

			if (extendedRT)
			{
				row.push_back(std::to_string(mEntries[i]->getDstSeq()));
				row.push_back(std::to_string(mEntries[i]->getSrcSeq()));
			}

			if (showPeerIDs)
			{
				pubKey = mEntries[i]->getDstPubKey();
				if (pubKey.size() == 32)
				{
					if (cf->genAddress(pubKey, dstID))
					{
						row.push_back(tools->bytesToString(dstID));
					}
					else
					{
						row.push_back("invalid");//would never happen anyway..
					}

				}
				else
				{//unknown
					row.push_back("");
				}
			}
			//add the actual row to the table
			rows.push_back(row);
			
		
		}
		toRet += tools->genTable(rows, alignment, true, "[Routing Table Entries]", newLine,maxWidth) + newLine;
	}

	return toRet;

}



/// <summary>
/// Takes care of extrinsic (networks) as well as of intrinsic (DTI Terminals, UDT/WebSocket conversations) CNetMsg containers' routing.
/// If, sourceEntity is nullptr the routing table will not be updated.
/// Conversely, sourceEntity might be, for example, an IPv4 node OR a websocket conversation.
/// Routing table entry update would take use of sourceEntity to learn/update a possibly new path.
/// The routine consits of two main statges:
/// 1) Internal Routing (no routing table entries are used) - the local sub-systems are queried for destination entity by the target desitnation's endpoint-type and ID 
/// During this stage data CAN be delivered to a locally hosted VM. The VM may be attached to a DTI or a Conversation (either UDT or websocket).
/// The benefit of DTI over conversation is that DTI builds upon conversation and provides additional facilities (asynchronous code execution through CCmdExecutor and the Virtual Terminal Multi-Threaded Interface users
/// can interact with).
/// 2) External routing (employs routing table) - employes routing table. During this stage data cannot be routed to a local VM attached to an endpoint.
/// </summary>
/// <param name="msg"></param>
/// <param name="source"></param>
/// <param name="sourceType"></param>
/// <returns></returns>
bool CDataRouter::route(std::shared_ptr<CNetMsg> msg, std::shared_ptr<CEndPoint> sourceEntity, std::shared_ptr<CConversation> sourceConversation)
{
	std::shared_ptr<CTools> tools = getTools();
	loadSettings();//extremely fast, happens once, until forced.
	/*
	IMPORTANT: any validations of the CNetMsg container should have had happened A priori.
	*/

	//Local Variables - BEGIN
	if(msg==nullptr)
	return false;
	bool delivered = false;
	std::shared_ptr<CWebRTCSwarm> swarm;
	std::vector <std::shared_ptr<CRTEntry>> entries;
	std::shared_ptr<CNetTask> task;
	std::shared_ptr<CDTI> dti;
	uint64_t supposedReward = 0;
	std::vector<uint8_t> destinationID = msg->getDestination();
	eEndpointType::eEndpointType destinationType = msg->getDestinationType();
	std::shared_ptr<CNetworkManager> nm = mNetworkManager.lock();//deleted once out of scope
	std::shared_ptr<CConversation> conversation;
	std::shared_ptr<CUDTConversationsServer> udtServer = nm->getUDTServer();
	std::shared_ptr<CQUICConversationsServer> quicServer = nm->getQUICServer();
	std::vector<std::shared_ptr<CConversation>> conversations;
	uint64_t spreadFactor = getSpreadFactor(msg);
	bool logNetworkDebugData = CBlockchainManager::getInstance(mNetworkManager.lock()->getBlockchainMode())->getSettings()->getLogEvents(eLogEntryCategory::network, eLogEntryType::notification);
	bool logDebugEvents = getLogNetworkDebugEvents();
	std::vector<uint8_t> receiptID;
	std::shared_ptr<CRTEntry> nextHopRTEntry = findEntryForDest(destinationID,destinationType);
	std::string temp;
	//Local Variables - END

	//Operational Logic - BEGIN

	eTTCollectionResult::eTTCollectionResult res = collectTT(msg, supposedReward);
	if (supposedReward < mMinPropagationReward)
	{
		if (logDebugEvents)
		{
			tools->logEvent("Won't route " + msg->getDescription() + " the reward of " + std::to_string(supposedReward) + " is not enough (" + std::to_string(mMinPropagationReward) + ") required.",eLogEntryCategory::network,0);
		}
		return false;
	}


	//Routing Table update - BEGIN
	if (sourceEntity != nullptr)
	{
		if (!updateRT(msg, sourceEntity))
		{
			if(logDebugEvents)
			tools->logEvent("Routing table was not updated for: [Source]: " + sourceEntity->getDescription() +"[NetMsg]: " + msg->getDescription(), eLogEntryCategory::network, 1, eLogEntryType::failure);
		}
		else if(logDebugEvents)
		{
			tools->logEvent("Routing table updated. Source: " + sourceEntity->getDescription() + " NetMsg: " + msg->getDescription(), eLogEntryCategory::network, 0, eLogEntryType::notification);
		}
	}
	//Routing Table update - END

	//[FIRST] - here an attempt is made to deliver datagram to one of the locally hosted entities (websockets, UDT, conversations, webRTC swarms etc.)
	// 	   No routing table is used. If the target entity (web-socket conversation/DTI) is found and the datagram is of type VM-Meta-Data, the data is handed over to the VM which MAY do further processing.
	//Attempt immediate peers - BEGIN
	if (nm == nullptr)
		return false;

	//Increase hop-count only as soon as we're sure that a delivery attempt will be made,
	//avoid memory copies decrease the counter instead.
	switch (msg->getDestinationType())
	{

	case eEndpointType::VM:
		///all datagrams targeting a particular VM are handled through the routing table.
		//in such a case a CConversation always acts as an intermediary.
		//i.e. for this to work, there needs to be a routing table entry with [Destination: vmID, DestinationType: VM, nextHop: conversation]
		//the conversation

		break;
	case eEndpointType::WebRTCSwarm://the message targets an entire WebRTC swarm (NOT its particular member)
		
		swarm = nm->getWebSocketsServer()->getSwarmByID(destinationID);
		
		if (swarm != nullptr)
		{
			tools->logEvent("Immediate routing to a registered WebRTC swarm is being attempted..", eLogEntryCategory::network, 0, eLogEntryType::notification);
			
			if (uint64_t peers = swarm->broadcastMsg(msg))
			{
				tools->logEvent("Datagram delivered to " + std::to_string(peers) + " Swarm members.", eLogEntryCategory::network, 0);
			}
			else
			{
				tools->logEvent("No peers within swarm for a datagram delivery.", eLogEntryCategory::network, 0);
			}
		}
		else
		{
			tools->logEvent("Immediate delivery of a datagram targeting WebRTC Swarm impossible. Routing will be attempted.", eLogEntryCategory::network, 0, eLogEntryType::notification);
		}

		break;

	case eEndpointType::TerminalID:
	
		//MetaData extraction attempt will take place and DataResponse-fields will be passed on to terminal-related VM.
		conversation = sourceConversation;
		if (deliverToTerminalEndpoint(conversation, dti, nm, destinationID, msg))
		{
			tools->logEvent("Datagram delivered to a locally spawned terminal (ID:" + tools->bytesToString(destinationID) + ").", eLogEntryCategory::network, 0, eLogEntryType::notification);
				return true;
		}
		else
		{
			tools->logEvent("Immediate delivery of datagram impossible. Routing will be attempted.", eLogEntryCategory::network, 0, eLogEntryType::notification);
		}

		break;

	case eEndpointType::QUICConversation:

		if (deliverToQUICConversation(logDebugEvents, msg, conversation, nm, destinationID, task, dti))
		{
			tools->logEvent("Datagram delivered through immediate, direct routing to QUIC Conversation (ID:" + tools->bytesToString(destinationID) + ").", eLogEntryCategory::network, 0, eLogEntryType::notification);
			return true;
		}
		else
		{
			tools->logEvent("Immediate delivery of datagram impossible. Routing will be attempted.", eLogEntryCategory::network, 0, eLogEntryType::notification);
		}

		break;

	case eEndpointType::UDTConversation:
	
		if (deliverToUDTConversation(logDebugEvents, msg, conversation, nm, destinationID, task, dti))
		{
			tools->logEvent("Datagram delivered through immediate, direct routing to UDT Conversation (ID:"+tools->bytesToString(destinationID) +").",eLogEntryCategory::network,0,eLogEntryType::notification);
			return true;
		}
		else
		{
			tools->logEvent("Immediate delivery of datagram impossible. Routing will be attempted.", eLogEntryCategory::network, 0, eLogEntryType::notification);
		}

		break;
	
	case eEndpointType::WebSockConversation://only immediate web-sock peers supported in this mode (may be router further by web-peer)
	//the CNetMsg will be routed through a web-socket and processed remotely by the browser.
	
		if (deliverToWebSocketEndpoint(msg, receiptID, sourceConversation, task, nm, logDebugEvents))
			return true;

		break;

	case eEndpointType::IPv4:
		case eEndpointType::IPv6://same handler indeed

			if (deliverToIPEndpoint(quicServer, conversation, nm, destinationID, conversations, task, msg, dti, spreadFactor))
				return true; // first try QUIC sub-system
			
			if(deliverToIPEndpoint(udtServer, conversation, nm, destinationID, conversations, task, msg, dti, spreadFactor))
			return true; // then fallback to UDT

			break;
		case eEndpointType::MAC:
			//todo: add support within IoT (mobile app)
			break;
		case eEndpointType::PeerID:
			
			//Update: for this, we are using the Routing Table.
			//Peer might be within any of the internal routing sub-systems (webrtc/websocket/UDT etc.)
			
			break;
		
	default:
		break;
	}
	//Attempt immediate peers - END

	//[SECOND] - now we'll try to deliver according to the routing table.
	/*
	* IMPORTANT: now, the type (data-layer) protocol used for delivery will be based on Next Hop - (which MAY NOT) be the type of destination.
	* Most often, this means looking up the coresponding CConversation within particular's subprocol's server and issuing an asynchronous data-delivery task.
	*/
	//Attempt routing as per the Routing Table Entries - BEGIN

	/*
	* ******IMPORTANT**** Destination entries are found right here in the line below
	*/
	

	entries = findEntriesForDest(destinationID, destinationType);//the function returns 'the best' nextHop neigboor from the pool of ones available/known.
	
	uint64_t routedThrough = 0;//the number of paths through which the CNetMsg has been sprayed through;
	//recall, the factor depands upon the value of the received Transmission Token
	std::shared_ptr<SE::CScriptEngine> targetVM, hostingVM;

	if (entries.size()>0)
	{
		if (logDebugEvents)
			tools->logEvent(std::to_string(entries.size())+ " routing table entries FOUND." , eLogEntryCategory::network, 0);

		msg->incHops();

		task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData,3);
		task->setData(msg->getPackedData());

		for (int i = 0; i < entries.size() && i < spreadFactor; i++)
		{
			targetVM = nullptr;
			//route data as per the next-hop's type (quic/udt/websock/swarm)

			if (logDebugEvents)
				tools->logEvent("[Routing Decision] for "+msg->getDescription()+": {\n"+ entries[i]->getDescription()+"\n}", eLogEntryCategory::network, 0);

			eEndpointType::eEndpointType eType = entries[i]->getNextHop()->getType();
			switch (eType)
			{

			case eEndpointType::VM:
				
				//data cannot be routed through a VM.
				//data can only be routed TO a VM/thread through a Conversation.
				//It means that say, the JavaScript context can freely dispatch VM-Meta-Data PDUs targetting a specific Thread,
				//and we rely on existance of an appropriate routing table entry (Conversation=>particular thread) for having the meta-data delivered this 
				//this very thread.
				
				break;
			case eEndpointType::WebRTCSwarm:
				
					swarm = nm->getWebSocketsServer()->getSwarmByID(destinationID);

					if (swarm)
					{
						if (destinationType == eEndpointType::WebRTCSwarm)
						{//we're targeting an entire WebRTC swarm. Broadcast will be used for data-delivery.
							if (logDebugEvents)
								tools->logEvent("Attempting brodcast delivery to WebRTC Swarm. ID: " + tools->base58CheckEncode(destinationID), eLogEntryCategory::network, 0);
							swarm->broadcastMsg(msg);
						}
						else
						{
							//delivery to an individual peer over WebRTC
							if (logDebugEvents)
								tools->logEvent("Attempting peer delivery to WebRTC Swarm. ID: " + tools->base58CheckEncode(destinationID), eLogEntryCategory::network, 0);

							if (std::shared_ptr<CWebRTCSwarmMember> member = swarm->findMemberByID(destinationID))
							{
								if (logDebugEvents)
									tools->logEvent("Peer found within Swarm. Delivering..", eLogEntryCategory::network, 0);

								//create an asynchronous data-delivery task
								conversation = member->getConversation();
								task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);//delivers the routed CNetMsg as a byte array(RAW data transfer)
								msg->incHops();//since the msg might be routed further by the web-peer
								task->setData(msg->getPackedData());
								conversation->addTask(task);
								return true;

							}
							else
							{
								if (logDebugEvents)
									tools->logEvent("Peer not found within the targeted swarm.", eLogEntryCategory::network, 0);
							}
							
						}
					}
					else
					{
						if (logDebugEvents)
							tools->logEvent("Routing failed. Swarm " + tools->base58CheckEncode(destinationID)+ " not available.", eLogEntryCategory::network, 0);
					}
				
				break;

			case eEndpointType::WebSockConversation:
				conversation = nm->getWebSocketsServer()->getConversationByID(entries[i]->getNextHop()->getAddress());
				if (conversation != nullptr && conversation->getState()->getCurrentState() == eConversationState::running)
				{
					if (logDebugEvents)
						tools->logEvent("[Routing Task Created]: Attempting internal routing to WebSocket Conversation. ID: " + tools->base58CheckEncode(destinationID) + " over " + conversation->getDescription(), eLogEntryCategory::network, 0);

					//Decentralized Threads Support - BEGIN
					if (destinationType == eEndpointType::VM)
					{
						tools->logEvent("[VM-Routing]: Attempting conversation-level routing of a VM-datagram. ID: " + tools->base58CheckEncode(destinationID) + " over " + conversation->getDescription(), eLogEntryCategory::network, 0);
						hostingVM = conversation->getSystemThread();

						if (hostingVM)
						{
							if (tools->compareByteVectors(hostingVM->getID(), destinationID))
							{
								if(logDebugEvents)
									tools->logEvent("[VM-Routing Decision]: Internal routing to WebSocket Conversation's Main VM. ID: " + tools->base58CheckEncode(destinationID) + " over " + conversation->getDescription(), eLogEntryCategory::network, 0);
								
								targetVM = conversation->getSystemThread();//it's the Main VM associated with the Conversation
							}
							else
							{
								//it still might be targeting one of its sub-threads
								targetVM = hostingVM->getSubThreadByVMID(destinationID);

								if (logDebugEvents)
									tools->logEvent("[VM-Routing Decision]: Internal routing to WebSocket Conversation's sub-Thread. ID: " + tools->base58CheckEncode(destinationID) + " over " + conversation->getDescription(), eLogEntryCategory::network, 0);
							}
						}
					}
					else
					{
						tools->logEvent("[VM-Routing]: Local conversation did not have main VM registered. ID: " + tools->base58CheckEncode(destinationID) + " over " + conversation->getDescription(), eLogEntryCategory::network, 0);
					}

					if (targetVM)
					{
						//the datagram is destined for processing on the locally registered VM's sub-thread.
						//i.e. the datagram shall not be propagated further.
						return conversation->processNetMsg(msg, std::vector<uint8_t>(), sourceConversation);
						
					}
					//Decentralized Threads Support - END
					else
					{
						
						//Datagram is to be routed further away
						//Note: we do NOT use eNetTaskType::sendNetMsg over here as we do NOT want ANY modifications to the routed datagram to take place.
						//Most importantly; we do NOT want msg to be re-encrypted with local-conversation's specific session key.
						//**i.e. - the datagram is simply ROUTED **
						task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);//delivers the routed CNetMsg as a byte array(RAW data transfer)
						//the web-browser supports the format and expects a CNetMsg anyway.
						msg->incHops();//since the msg might be routed further by the web-peer
						task->setData(msg->getPackedData());
						conversation->addTask(task);
					}
					delivered = true;
					return true;
				}
				else
				{
					if (logDebugEvents)
					{
						if (conversation == nullptr)
						{
							tools->logEvent("Routing to Web-Socket conversation (ID " + tools->base58CheckEncode(destinationID) + ") over " + entries[i]->getDescription()+"  - FAILED. Intermediate Conversation does not exist anymore.",
								eLogEntryCategory::network, 0, eLogEntryType::failure);
						}
						else
						{
							tools->logEvent("Routing to Web-Socket conversation (ID " + tools->base58CheckEncode(destinationID) + ") FAILED. Intermediate Conversation is not active anymore.",
								eLogEntryCategory::network, 0, eLogEntryType::failure);
						}
					}
				
			return false;
			}
				break;

			case eEndpointType::QUICConversation:
				return deliverToQUICConversation(logDebugEvents, msg, conversation, nm, entries[i]->getNextHop()->getAddress(), task, dti, false);
				break;
			case eEndpointType::UDTConversation:
				return deliverToUDTConversation(logDebugEvents, msg, conversation, nm, entries[i]->getNextHop()->getAddress(), task, dti,false);
				break;
			case eEndpointType::IPv4:
			case eEndpointType::IPv6:
				// first try the QUIC sub-system
				routedThrough = deliverToIPEndpoint(quicServer, conversation, nm, entries[i]->getNextHop()->getAddress(), conversations, task, msg, dti, spreadFactor);
				// then spray over UDT as well
				routedThrough += deliverToIPEndpoint(udtServer, conversation, nm, entries[i]->getNextHop()->getAddress(), conversations, task, msg, dti, spreadFactor);
				spreadFactor = min(0, spreadFactor - routedThrough);
				break;
			case eEndpointType::MAC:
				break;

			case eEndpointType::PeerID:
				break;
			case eEndpointType::TerminalID:
				conversation = sourceConversation;
				//if we're targeting Terminal, then just route to its ID, otherwise the terminal's ID is in the routing table's entry as a Next Hop
				//the terminal would then recognize the type of target as VM and attempt routing to it as well.
				if (deliverToTerminalEndpoint(conversation, dti, nm, msg->getDestinationType() == eEndpointType::TerminalID ? destinationID :
					entries[i]->getNextHop()->getAddress(), msg))
				{
					delivered = true;
					spreadFactor = min(0, spreadFactor - 1);
				}
				break;
			default:
				break;
			}

		}
		
		conversation->addTask(task);
	}
	//Attempt routing as per the Routing Table Entries - END

	//Operational Logic - END
	return delivered;
}

bool CDataRouter::deliverToWebSocketEndpoint(std::shared_ptr<CNetMsg>& msg, std::vector<uint8_t>& receiptID, std::shared_ptr<CConversation>& sourceConversation, std::shared_ptr<CNetTask>& task, std::shared_ptr<CNetworkManager>& nm, bool logDebugEvents)
{
	std::shared_ptr<CTools> tools = getTools();
	//Pre-Validation - BEGIN
	if (!msg || !nm || nm->getWebSocketsServer()==nullptr)
		return false;
	//Pre-Validation - END

	//Local Variables - BEGIN
	std::vector<uint8_t> destinationID = msg->getDestination();
	std::shared_ptr<CConversation> conversation  = nm->getWebSocketsServer()->getConversationByID(destinationID);
	std::string temp;

	//Local Variables - END

	//Initial Validation - BEGIN
	if (!conversation)
	{
		return false;
	}
	//Initial Validation - END


	//DTIs can only listen to data and process. DTIs can do no forwarding.
	// It's a different story with Conversations.
	//A conversation can either do local processing (if it has a VM attached and data-packet is of type VM-Meta-Data) OR it may forward the data further to the remote peer.
	//Here we first check the type of the datagram. If the above conditions are not met the data-gram is destined for routing to the remote peer.
	//[todo:TheOldWizard:medium]: reconsider explicit an WebSockConversationVM or even a 'VM' endpoint type.
	std::shared_ptr<SE::CScriptEngine> se = conversation->getSystemThread();

	//Attempt VM-processing - BEGIN
	
	if (conversation->processNetMsg(msg, receiptID, sourceConversation))
	{
		//Status Reporting - BEGIN
		//receipt would be delivered ONLY if available. Delivery status is always reported in a lower layer.

		if (receiptID.size() > 0)
		{
			task = std::make_shared<CNetTask>(eNetTaskType::notifyReceiptID);
			task->setData(receiptID);
			conversation->addTask(task);
		}
		//Status Reporting - END
		return true;
	}
	//Attempt VM-processing - END
	else
	{//Forwarding of Data OUT through a conversation - BEGIN

		temp = tools->base58CheckEncode(destinationID);
		

		if (conversation != nullptr && conversation->getState()->getCurrentState() == eConversationState::running)
		{
			if (logDebugEvents)
				tools->logEvent("[Routing Task Created]: Attempting internal routing to WebSocket Conversation. ID: " + tools->base58CheckEncode(destinationID), eLogEntryCategory::network, 0);

			task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);//delivers the routed CNetMsg as a byte array(RAW data transfer)
																		 //the web-browser supports the format and expects a CNetMsg anyway.
			msg->incHops();//since the msg might be routed further by the web-peer
			task->setData(msg->getPackedData());
			conversation->addTask(task);
			return true;
		}
		else
		{

			if (logDebugEvents)
				tools->logEvent(tools->endpointTypeToString(msg->getDestinationType()) + " Conversation (ID " + tools->base58CheckEncode(destinationID) + ") is not hosted. Will attempt routing...",
					eLogEntryCategory::network, 0, eLogEntryType::failure);
		}
	}//Forwarding of Data OUT through a conversation - END


	return false;
}

bool CDataRouter::isRTEntryReachable(std::shared_ptr<CRTEntry> entry)
{
	if (entry == nullptr)
		return false;

	//Validation - BEGIN (limited)
	if (entry->getNextHop() == nullptr)
		return false;
	//Validation - END

	//Local Variables - BEGIN
	eEndpointType::eEndpointType eType = entry->getNextHop()->getType();
	std::shared_ptr<CNetworkManager> networkManager = mNetworkManager.lock();
	if (networkManager == nullptr)
		return false;

	std::shared_ptr<CUDTConversationsServer> udtServer;
	std::shared_ptr<CQUICConversationsServer> quicServer;
	std::shared_ptr<CWebSocketsServer> websockServer;
	std::shared_ptr <CDTIServer> dtiServer;
	std::vector<uint8_t> nextHopAddr = entry->getNextHop()->getAddress();
	if (nextHopAddr.size() == 0)
		return false;

	std::shared_ptr<CConversation> conversation;
	//Local Variables - END

	switch (eType)
	{
	case eEndpointType::IPv4:
		// QUIC - BEGIN
		quicServer = networkManager->getQUICServer();

		if (quicServer == nullptr)
			return false;

		if (!(conversation = quicServer->getConversationByIP(nextHopAddr)))
			return false;

		if (conversation->getState()->getCurrentState() == eConversationState::running)
			return true;

		// QUIC - END

		// UDT - BEGIN
		udtServer = networkManager->getUDTServer();

		if (udtServer == nullptr)
			return false;

		if(!(conversation = udtServer->getConversationByIP(nextHopAddr)))
			return false;

		if (conversation->getState()->getCurrentState() == eConversationState::running)
			return true;

		// UDT - END

		break;
	case eEndpointType::IPv6:
		// QUIC - BEGIN
		quicServer = networkManager->getQUICServer();
		if (quicServer == nullptr)
			return false;

		if (!(conversation = quicServer->getConversationByIP(nextHopAddr)))
			return false;

		if (conversation->getState()->getCurrentState() == eConversationState::running)
			return true;
		// QUIC - END

		// UDT - BEGIN
		udtServer = networkManager->getUDTServer();
		if (udtServer == nullptr)
			return false;

		if (!(conversation = udtServer->getConversationByIP(nextHopAddr)))
			return false;

		if (conversation->getState()->getCurrentState() == eConversationState::running)
			return true;
		// UDT - END

		break;


		break;
	case eEndpointType::MAC:
		//todo: traverse all network sub-systems/types in search of the peer
		break;
	case eEndpointType::PeerID:

		//todo: traverse all network sub-systems/types in search of the peer
		break;
	case eEndpointType::TerminalID:
		dtiServer = networkManager->getDTIServer();

		if (dtiServer == nullptr)
			return false;

		if (dtiServer->getDTIbyID(nextHopAddr))
			return true;

		break;
	case eEndpointType::WebSockConversation:
		websockServer = networkManager->getWebSocketsServer();

		if (websockServer == nullptr)
			return false;

		if (!(conversation = websockServer->getConversationByID(nextHopAddr)))
			return false;

		if (conversation->getState()->getCurrentState() == eConversationState::running)
			return true;

		break;
	case eEndpointType::UDTConversation:

		udtServer = networkManager->getUDTServer();
		if (udtServer == nullptr)
			return false;

		if (!(conversation = udtServer->getConversationByIP(nextHopAddr)))
			return false;
		if (conversation->getState()->getCurrentState() == eConversationState::running)
			return true;

		break;

	case eEndpointType::QUICConversation:

		quicServer = networkManager->getQUICServer();
		if (quicServer == nullptr)
			return false;

		if (!(conversation = quicServer->getConversationByIP(nextHopAddr)))
			return false;
		if (conversation->getState()->getCurrentState() == eConversationState::running)
			return true;

		break;
	case eEndpointType::WebRTCSwarm:

		websockServer = networkManager->getWebSocketsServer();

		if (websockServer == nullptr)
			return false;
		if (websockServer->getSwarmByID(nextHopAddr))
			return true;
		break;

	default:
		return false;
		break;
	}
	return false;
}

bool CDataRouter::deliverToUDTConversation(bool logDebugEvents, std::shared_ptr<CNetMsg>& msg, std::shared_ptr<CConversation>& conversation, std::shared_ptr<CNetworkManager>& nm, std::vector<uint8_t>& destinationID, std::shared_ptr<CNetTask>& task, std::shared_ptr<CDTI>& dti, bool isStStage)
{
	std::shared_ptr<CTools> tools = getTools();
	if (logDebugEvents)//do not event attempt to prepare the message if debug-info not enabled
		tools->logEvent("[Pre-Routing]: (" + std::string(isStStage ? "1st" : "2nd") + " stage) to "+msg->getFormatedDestination()+ " from " + msg->getFormatedSource() + " via UDT ("+tools->base58CheckEncode(destinationID)+")" ,
			eLogEntryCategory::network, 0, eLogEntryType::notification);
	conversation = nm->getUDTServer()->getConversationByID(destinationID);


	if (conversation != nullptr && conversation->getState()->getCurrentState() == eConversationState::running)
	{
		if (logDebugEvents)//do not even attempt to prepare the message if debug-info not enabled
			tools->logEvent("[Routing-Task]: (" + std::string(isStStage ? "1st" : "2nd") + " stage) to " + msg->getFormatedDestination() + " from " + msg->getFormatedSource() + " via "+conversation->getDescription(),
				eLogEntryCategory::network, 0, eLogEntryType::notification);

		task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);
		msg->incHops();
		task->setData(msg->getPackedData());
		conversation->addTask(task);
		return true;
	}
	else if (conversation != nullptr && conversation->getState()->getCurrentState() == eConversationState::running)
	{
		if(logDebugEvents)
		tools->logEvent("Routing to " + msg->getFormatedDestination() + " over UDT(" + tools->base58CheckEncode(destinationID) + ") FAILED. The intermediary conversation " + " UDT(" + tools->base58CheckEncode(destinationID) + ")" + " is registered but is no longer Operational.", eLogEntryCategory::network, 0);
	}
	else
	{
		if(!isStStage && logDebugEvents)
		tools->logEvent("Routing to "+ msg->getFormatedDestination()+" over UDT(" + tools->base58CheckEncode(destinationID) + ") FAILED. The intermediary conversation "+" UDT("+tools->base58CheckEncode(destinationID)+")"+" does not exist.",eLogEntryCategory::network, 0);
	}

	return false;//by default
}
/**
 * @brief Deliver message to a QUIC conversation.
 *
 * This method routes a network message to the appropriate QUIC conversation if it exists and is operational.
 * It performs necessary logging based on the debug flag and prepares tasks for message delivery.
 *
 * @param logDebugEvents Flag indicating whether to log debug events.
 * @param msg Shared pointer to the network message to be delivered.
 * @param conversation Shared pointer to the conversation object.
 * @param nm Shared pointer to the network manager.
 * @param destinationID Vector containing the destination ID.
 * @param task Shared pointer to the network task.
 * @param dti Shared pointer to the data transfer interface.
 * @param isStStage Boolean indicating whether it is the first stage of delivery.
 *
 * @return True if the message is successfully delivered to an operational conversation; false otherwise.
 */
bool CDataRouter::deliverToQUICConversation(bool logDebugEvents, std::shared_ptr<CNetMsg>& msg, std::shared_ptr<CConversation>& conversation, std::shared_ptr<CNetworkManager>& nm, std::vector<uint8_t>& destinationID, std::shared_ptr<CNetTask>& task, std::shared_ptr<CDTI>& dti, bool isStStage)
{
	// Retrieve tools instance for logging and utility functions
	std::shared_ptr<CTools> tools = getTools();

	// Log pre-routing event if debug logging is enabled
	if (logDebugEvents)
	{
		tools->logEvent(
			"[Pre-Routing]: (" + std::string(isStStage ? "1st" : "2nd") + " stage) to " + msg->getFormatedDestination() +
			" from " + msg->getFormatedSource() + " via QUIC (" + tools->base58CheckEncode(destinationID) + ")",
			eLogEntryCategory::network, 0, eLogEntryType::notification
		);
	}

	// Attempt to retrieve the conversation by its destination ID
	conversation = nm->getQUICServer()->getConversationByID(destinationID);

	// Check if the conversation is valid and running
	if (conversation != nullptr && conversation->getState()->getCurrentState() == eConversationState::running)
	{
		// Log routing task event if debug logging is enabled
		if (logDebugEvents)
		{
			tools->logEvent(
				"[Routing-Task]: (" + std::string(isStStage ? "1st" : "2nd") + " stage) to " + msg->getFormatedDestination() +
				" from " + msg->getFormatedSource() + " via " + conversation->getDescription(),
				eLogEntryCategory::network, 0, eLogEntryType::notification
			);
		}

		// Prepare and add the network task for sending raw data
		task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);
		msg->incHops(); // Increment the hop count for the message
		task->setData(msg->getPackedData()); // Set the packed data to the task
		conversation->addTask(task); // Add the task to the conversation's task queue

		return true; // Return true indicating successful delivery
	}
	// Handle the case where the conversation is registered but no longer operational
	else if (conversation != nullptr && conversation->getState()->getCurrentState() == eConversationState::running)
	{
		if (logDebugEvents)
		{
			tools->logEvent(
				"Routing to " + msg->getFormatedDestination() + " over QUIC(" + tools->base58CheckEncode(destinationID) +
				") FAILED. The intermediary conversation QUIC(" + tools->base58CheckEncode(destinationID) +
				") is registered but is no longer operational.",
				eLogEntryCategory::network, 0
			);
		}
	}
	// Handle the case where the conversation does not exist
	else
	{
		if (!isStStage && logDebugEvents)
		{
			tools->logEvent(
				"Routing to " + msg->getFormatedDestination() + " over QUIC(" + tools->base58CheckEncode(destinationID) +
				") FAILED. The intermediary conversation QUIC(" + tools->base58CheckEncode(destinationID) +
				") does not exist.",
				eLogEntryCategory::network, 0
			);
		}
	}

	return false; // Return false indicating unsuccessful delivery
}



/// <summary>
/// The function attempts a CNetmsg delivery to one of locally spaawned Virtual Terminal (DTI) end-points.
/// </summary>
/// <param name="conversation"></param>
/// <param name="dti"></param>
/// <param name="nm"></param>
/// <param name="destAddr"></param>
/// <param name="msg"></param>
/// <param name="task"></param>
/// <returns></returns>
bool CDataRouter::deliverToTerminalEndpoint(std::shared_ptr<CConversation>& conversation, std::shared_ptr<CDTI>& dti, std::shared_ptr<CNetworkManager>& nm, std::vector<uint8_t>& destAddr, std::shared_ptr<CNetMsg>& msg)
{
	std::shared_ptr<CTools> tools = getTools();

	//IMPORTANT: here, the conversation is a Data-Delivery conversation.

	//Local Variables - BEGIN
	std::vector<uint8_t> receiptID;
	std::shared_ptr<CNetTask> task;
	bool result;
	std::vector<uint8_t> destinationID = destAddr;
	std::shared_ptr<SE::CScriptEngine> targetVM, hostingVM;
	bool logDebugEvents = getLogNetworkDebugEvents();
	if (msg == nullptr)
		return false;
	//Local Variables - END
	
	//Processing - BEGIN


	dti = nm->getDTIServer()->getDTIbyID(destinationID);//destAddr is either VM or Terminal ID (note invocations)

	if (dti == nullptr)
	{
		return false;
	}

	if (msg->getDestinationType() == eEndpointType::VM)
	{
		std::string targetDesc = "DTI " + tools->base58Encode(dti->getID());
		destinationID = msg->getDestination();//replace now with the real VM's address
		//destAddr = destinationID;

		if(logDebugEvents)
		tools->logEvent("[VM-Routing]: Attempting DTI-level routing of a VM-datagram. ID: " + tools->base58CheckEncode(destinationID) + " over  " + targetDesc, eLogEntryCategory::network, 0);
		hostingVM = dti->getScriptEngine();

		if (hostingVM)
		{
			if (tools->compareByteVectors(hostingVM->getID(), destinationID))
			{
				if (logDebugEvents)
					tools->logEvent("[VM-Routing Decision]: Internal routing to DTI's Main VM. ID: " + tools->base58CheckEncode(destinationID) + " over " + targetDesc, eLogEntryCategory::network, 0);

				targetVM = hostingVM;//it's the Main VM associated with the Conversation
			}
			else
			{
				//it still might be targetting one of its sub-threads
				targetVM = hostingVM->getSubThreadByVMID(destinationID);

				if (logDebugEvents)
					tools->logEvent("[VM-Routing Decision]: Internal routing to WebSocket Conversation's sub-Thread. ID: " + tools->base58CheckEncode(destinationID) + " over " + targetDesc, eLogEntryCategory::network, 0);
			}
		}
		else
		{
			tools->logEvent("[VM-Routing]: Local DTI did not have a main VM registered. ID: " + tools->base58CheckEncode(destinationID) + " over " + targetDesc, eLogEntryCategory::network, 0);
		}
		
	}
	else
	{
		if (getLogNetworkDebugEvents())
			tools->logEvent("[Routing Task Created]: Attempting DIRECT internal routing to Terminal. ID: " + tools->base58CheckEncode(destinationID), eLogEntryCategory::network, 0);
	}
		result= dti->processNetMsg(msg, receiptID, conversation);
		//Processing - END

		//Reporting oda data-delivery is taken care at a lower level.
		//Here we report on the receipt ID if any
		//Status Reporting - BEGIN
		//receipt would be delivered ONLY if availble. Delivery status is always reported in a lower layer.

		if (receiptID.size()>0 && conversation != nullptr)
		{
			task = std::make_shared<CNetTask>(eNetTaskType::notifyReceiptID);
			task->setData(receiptID);
			conversation->addTask(task);
		}
		//Status Reporting - END
		
		return result;
}

/// <summary>
/// Attempt CNetMsg delivery an an immediate peer.
/// </summary>
/// <param name="udtServer"></param>
/// <param name="conversation"></param>
/// <param name="nm"></param>
/// <param name="destAddr"></param>
/// <param name="conversations"></param>
/// <param name="task"></param>
/// <param name="msg"></param>
/// <param name="dti"></param>
/// <param name="spreadFactor"></param>
/// <returns></returns>
uint64_t CDataRouter::deliverToIPEndpoint(std::shared_ptr<CUDTConversationsServer>& udtServer, std::shared_ptr<CConversation>& conversation, std::shared_ptr<CNetworkManager>& nm, std::vector<uint8_t>& destAddr, std::vector<std::shared_ptr<CConversation>>& conversations, std::shared_ptr<CNetTask>& task, std::shared_ptr<CNetMsg>& msg, std::shared_ptr<CDTI>& dti, const uint64_t& spreadFactor)
{
	uint64_t routedThrough = 0;
	//first look within UDT peers
	if (udtServer != nullptr)
		conversation = nm->getUDTServer()->getConversationByIP(destAddr);
	//then try Web-Socket connections
	if (conversation == nullptr)
		conversations = nm->getWebSocketsServer()->getConversationsByIP(destAddr);

	if (conversation != nullptr || conversations.size()>0)
	{
		task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);
		msg->incHops();
		task->setData(msg->getPackedData());
		dti->getConversation()->addTask(task);

		if (conversation != nullptr && conversation->getState()->getCurrentState() == eConversationState::running)
		{
			routedThrough = 1;
			conversation->addTask(task);
		}
		else if (conversations.size() > 0)
		{
			for (int i = 0; i < conversations.size() && i < spreadFactor; i++)//spam up to X web-socket connections (might be behind single NAT/browser Tabs)
			{
				if (conversations[i]->getState()->getCurrentState() == eConversationState::running)
				{
					conversations[i]->addTask(task);
					routedThrough++;
				}
			}
		}
		return routedThrough;//right on, let us be positive shall we
	}
	return routedThrough;
}

/**
 * @brief Deliver message to an IP endpoint using QUIC or WebSocket connections.
 *
 * This method routes a network message to an appropriate QUIC conversation or WebSocket connection based on the destination IP address.
 * It performs necessary task preparation and message delivery while considering the spread factor for WebSocket connections.
 *
 * @param quicServer Shared pointer to the QUIC conversations server.
 * @param conversation Shared pointer to the conversation object.
 * @param nm Shared pointer to the network manager.
 * @param destAddr Vector containing the destination IP address.
 * @param conversations Vector of shared pointers to conversation objects.
 * @param task Shared pointer to the network task.
 * @param msg Shared pointer to the network message.
 * @param dti Shared pointer to the data transfer interface.
 * @param spreadFactor The factor indicating how many WebSocket connections to spread the task across.
 *
 * @return The number of routes the message was successfully delivered through.
 */
uint64_t CDataRouter::deliverToIPEndpoint(std::shared_ptr<CQUICConversationsServer>& quicServer, std::shared_ptr<CConversation>& conversation, std::shared_ptr<CNetworkManager>& nm, std::vector<uint8_t>& destAddr, std::vector<std::shared_ptr<CConversation>>& conversations, std::shared_ptr<CNetTask>& task, std::shared_ptr<CNetMsg>& msg, std::shared_ptr<CDTI>& dti, const uint64_t& spreadFactor)
{
	uint64_t routedThrough = 0;

	// First look within QUIC peers
	if (quicServer != nullptr)
		conversation = nm->getQUICServer()->getConversationByIP(destAddr);

	// Then try WebSocket connections
	if (conversation == nullptr)
		conversations = nm->getWebSocketsServer()->getConversationsByIP(destAddr);

	// If a valid conversation or WebSocket connection is found
	if (conversation != nullptr || conversations.size() > 0)
	{
		// Prepare the network task for sending raw data
		task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);
		msg->incHops(); // Increment the hop count for the message
		task->setData(msg->getPackedData()); // Set the packed data to the task
		dti->getConversation()->addTask(task); // Add the task to the DTI conversation's task queue

		// If a valid QUIC conversation is found and is running
		if (conversation != nullptr && conversation->getState()->getCurrentState() == eConversationState::running)
		{
			routedThrough = 1;
			conversation->addTask(task); // Add the task to the QUIC conversation's task queue
		}
		// If valid WebSocket connections are found
		else if (conversations.size() > 0)
		{
			for (size_t i = 0; i < conversations.size() && i < spreadFactor; i++) // Spread across WebSocket connections
			{
				if (conversations[i]->getState()->getCurrentState() == eConversationState::running)
				{
					conversations[i]->addTask(task); // Add the task to the WebSocket conversation's task queue
					routedThrough++;
				}
			}
		}
		return routedThrough; // Return the number of successful routes
	}

	return routedThrough; // Return the number of successful routes (default 0)
}


/// <summary>
/// Retrieves spread factor based on CNetMsg's Transmission Token (if available).
/// the higher Transmission Reward the higher spread factor i.e. the higher number of nodes 
/// through which delivery attempts will be made.
/// </summary>
/// <param name="msg"></param>
/// <returns></returns>
uint64_t  CDataRouter::getSpreadFactor(std::shared_ptr<CNetMsg> msg)
{
	std::shared_ptr<CTransmissionToken> tt =  msg->getTT();
	if (tt == nullptr)
		return mFreePropagationsCount;//no tt available; are we altruistic?

	return  min( (static_cast<uint64_t>(tt->getValue()) / mMinPropagationReward), 5);
}

/// <summary>
/// Returns a vector of routing table entries sorted by path lengths.
/// </summary>
/// <param name="id"></param>
/// <param name="eType"></param>
/// <returns></returns>
std::vector <std::shared_ptr<CRTEntry>> CDataRouter::findEntriesForDest(std::vector<uint8_t> id, eEndpointType::eEndpointType eType)
{
	std::vector <std::shared_ptr<CRTEntry>> toRet;

	std::lock_guard<std::mutex> lock(mRTGuardian);
	std::shared_ptr<CEndPoint> dst = nullptr;

	for (uint64_t i = 0; i < mEntries.size(); i++)
	{
		dst = mEntries[i]->getDst();

		if (dst->getType() == eType)
		{
			if (getTools()->compareByteVectors(id, dst->getAddress()))
			{
				if (toRet.size() == 0)
					toRet.push_back(mEntries[i]);
				else
				{
					//Ad-Hoc Sorting - BEGIN
					for (uint64_t a = 0; a < toRet.size(); a++)
					{
						if (toRet[a]->getHops() > mEntries[i]->getHops() || toRet[a]->getTimestamp()< mEntries[i]->getTimestamp())//shorter OR newer entries are prefered (same priority on these properties)
						{
							toRet.insert(toRet.begin() + a, mEntries[i]);
							break;
						}
						
					}
					toRet.push_back(mEntries[i]);
					//Ad-Hoc Sorting -END
				}
			}
		}
	}

	return toRet;
}


/// <summary>
/// Returns an entry for destination.
/// By default, if entry found, automatically updates the entrie's timestamp to prevent it from deletion (pingEntry param).
/// </summary>
/// <param name="id"></param>
/// <param name="eType"></param>
/// <param name="pingEntry"></param>
/// <returns></returns>
std::shared_ptr<CRTEntry> CDataRouter::findEntryForDest(std::vector<uint8_t> id, eEndpointType::eEndpointType eType, bool pingEntry)
{
	//avoid usage of recursive mutexes for performance reasons

	std::shared_ptr<CEndPoint> dst = nullptr;

	std::vector <std::shared_ptr<CRTEntry>> entries = findEntriesForDest(id, eType);
	if (entries.size() == 0)
		return nullptr;
	else
	{
		if (pingEntry)
			entries[0]->ping();

		return entries[0];// contains shortest path first
	}

	return nullptr;
}





/// <summary>
/// Performs Routing Table clean-up.
/// </summary>
void CDataRouter::cleanTable()
{
	std::shared_ptr<CTools> tools = getTools();
	std::lock_guard<std::mutex> lock(mRTGuardian);
	//Local Variables - BEGIN
	const uint64_t maxInactivity = 1800;//30 minutes
	uint64_t entriesTime = 0;
	uint64_t now = tools->getTime();//not in future checks below, one time check for peformance
	//Local Variables - END
	uint64_t removedEntries = 0;

	std::vector<std::shared_ptr<CRTEntry>>::iterator it = mEntries.begin();
	
	while (it != mEntries.end()) {

		 entriesTime = (*it)->getTimestamp();
		if (!(*it)->getTimestamp()>now && !(maxInactivity> entriesTime) &&((now- entriesTime)> maxInactivity)) {

			it = mEntries.erase(it);
			removedEntries++;
		}
		else ++it;
	}
}




