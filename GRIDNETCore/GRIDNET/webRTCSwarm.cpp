#include "webRTCSwarmMember.h"
#include "webRTCSwarm.h"
#include "conversation.h"
#include "SDPEntity.h"
#include "NetworkManager.h"
#include <memory>
#include <vector>
#include <mutex>
#include "enums.h"
#include "conversationState.h"
#include "NetTask.h"
#include "NetMsg.h"
#include "EEndPoint.h"
#include "DataRouter.h"

#define HOP_COUNT_LIMIT 100
void CWebRTCSwarm::initFields()
{
	mInactivityRemovalThreshold = 3600;//1hour
	mSizeLimit = 1000;
    mSDPEntityProcessingTimeout = 60;
    mID = CTools::getInstance()->genRandomVector(8);
    mLastActivity = CTools::getInstance()->getTime();
    mTools = CTools::getInstance();
}


std::shared_ptr<CTools> CWebRTCSwarm::getTools()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mTools;
}


std::shared_ptr<CNetworkManager> CWebRTCSwarm::getNetworkManager()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mNetworkManager;
}

std::shared_ptr<CEndPoint> CWebRTCSwarm::getAbstractEndpoint()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mAbstractEndpoint;
}

CWebRTCSwarm::CWebRTCSwarm(std::shared_ptr<CNetworkManager> nm, std::vector<uint8_t> id, std::shared_ptr<CDataRouter> router, uint64_t sizeLimit, uint64_t inactivityRemovalThreshold)
{

    initFields();
    mRouter = router;
    mNetworkManager = nm;
    if (id.size() > 0)
        mID = id;
    if (sizeLimit > 0)
        mSizeLimit = sizeLimit;
    if (inactivityRemovalThreshold > 0)
        mInactivityRemovalThreshold = inactivityRemovalThreshold;

    refreshAbstractEndpoint();
}

uint64_t CWebRTCSwarm::getLastActivityTimestamp()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mLastActivity;
}


/// <summary>
/// Routes SDP entities to designated recipients and transfers over to em' through WebSocket.
//  Returns number of successfully routed SDP entities.
/// </summary>
/// <returns></returns>
uint64_t CWebRTCSwarm::routeSDPEntities()
{
    //Preliminaries - BEGIN
    std::shared_ptr<CNetworkManager> nm = getNetworkManager();
    if (!nm)
    {
        return 0;
    }
    //Preliminaries - END

    std::lock_guard<std::mutex> lock(mGuardian);

    //Local Variables - BEGIN
    std::vector<uint8_t> sdpDeliveryIP;
    std::string destIPStr;
    std::shared_ptr<CCryptoFactory> cf = CCryptoFactory::getInstance();
    //copy members' pointers - BEGIN
    std::vector< std::shared_ptr<CWebRTCSwarmMember>> members;
    mMembersGuardian.lock();
    for (uint64_t i = 0; i < mMembers.size(); i++)
        members.push_back(mMembers[i]);
    mMembersGuardian.unlock();
    //copy members' pointers - END
    std::vector<uint8_t> packedReceivedSDP;
    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::shared_ptr<CWebRTCSwarmMember> bestKnownPeerIdentity, sameConversationPresentIdentity;
    std::shared_ptr<CConversation> peerConversation;
    std::shared_ptr<CConversation> dataDeliveryConversation;
    bool agentFound = false;
    bool delivered = false;
    std::string localSTR = tools->getColoredString("local", eColor::lightGreen);
    std::string remoteSTR = tools->getColoredString("remote", eColor::lightCyan);
    std::string routedAway = tools->getColoredString("routed away'", eColor::orange);
    std::string processedLocally = tools->getColoredString("processed locally'", eColor::lightGreen);
    std::string sourceTag;
    std::shared_ptr<CNetMsg> netMsg;
    std::shared_ptr <CNetTask> task;
    std::string sdpTypeStr = "";
    conversationFlags flags;
    bool loggingEnabled = true;
    flags.exchangeBlocks = false;
    flags.isMobileApp = false;
    eSDPEntityType::eSDPEntityType sdpType;
    bool wasRouted = false;
    bool ignore = false;
    uint64_t spreadCount = 0;
    uint64_t sdpHopCount = 0;
    std::vector<uint8_t> receivedSDPHash;
    //std::shared_ptr<CConversation> conv;
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> publicIPVec = tools->stringToBytes(getNetworkManager()->getPublicIP());
    std::vector<uint8_t> destID;
    std::string sdpDescription;
    std::vector<std::shared_ptr<CConversation>>  conversations = nm->getConversations(flags, true, true, false);
    bool isConversationAuthed = false;
    uint64_t resultedInLocalInvitationsCount = 0;
    //Local Variables - END
    std::vector<uint8_t> outgressSDPHash;
    /*
    * Notice: local node MIGHT not be directly connected with a member of a Swarm.
    * Further, local node MIGHT not be aware of all members of any particular Swarm.
    * Thus, after an SDP Entity (SDP/ICE) has been processed locally - it is propagated across the network.
    */
    //by the tend of the loop each entity gets Erased (NO further caching takes place)
    std::shared_ptr<CSDPEntity> sdp;
    for (std::vector <std::shared_ptr<CSDPEntity>>::iterator sdpEntitiesIterator = mPendingSDPEntities.begin(); sdpEntitiesIterator != mPendingSDPEntities.end(); ) {
        
           sdp = (*sdpEntitiesIterator);
        //  ^---- current datagram.
        sdpEntitiesIterator = mPendingSDPEntities.erase(sdpEntitiesIterator);//get rid of it already.

        //Clear State - BEGIN

        packedReceivedSDP = sdp->getPackedData();
        //^ needs to be the first thing, because:
        receivedSDPHash = cf->getSHA2_256Vec(packedReceivedSDP);
        outgressSDPHash.clear();////////////////////////
        resultedInLocalInvitationsCount = 0;
        sdpHopCount = sdp->getHopCount();
        isConversationAuthed = false;
        destID = sdp->getDestinationID();
        agentFound = false;
        ignore = false;
     
     
        wasRouted = false;
        delivered = false;
        netMsg = nullptr;
        task = nullptr;
        sdpType = sdp->getType();
        sdpTypeStr = tools->SDPTypeToString(sdpType);
        dataDeliveryConversation = sdp->getConversation();
        sdpDeliveryIP = sdp->getConversation()->getEndpoint()->getAddress();
        sourceTag = tools->getColoredString(("@'" + tools->bytesToString(sdp->getSourceID())+"'"), eColor::blue);
        
       
        bestKnownPeerIdentity = findMemberByID(sdp->getSourceID(), std::vector<uint8_t>(), true);
        sameConversationPresentIdentity = findMemberByID(sdp->getSourceID(), dataDeliveryConversation->getID(), true);//both the conversation ID and member's ID need to match (security measure)
        //Clear State - END

        nm->sawData(receivedSDPHash, tools->bytesToString(sdpDeliveryIP));//rememeber that we already saw this datagram.

        if (sdpDeliveryIP.size() == 0)
            break;

        if (dataDeliveryConversation->getEndpoint()->getType() == eEndpointType::IPv4)
            wasRouted = true;

        //^--- needs to be above.
        //due to the below which follows ---v

        sdpDescription = (wasRouted ? remoteSTR : localSTR) + " " + tools->getColoredString(sdpTypeStr, eColor::lightCyan) + sourceTag +
            " from " + tools->bytesToString(sdpDeliveryIP);

        //Logging - BEGIN
        if (loggingEnabled)
        {
            if (wasRouted)
            {
                tools->logEvent(remoteSTR + tools->getColoredString(sdpTypeStr, eColor::lightCyan) + " from " + "'" + tools->bytesToString(sdp->getSourceID()) + "'" + " to "+ (destID.size()?"'" + tools->bytesToString(destID) + "'":tools->getColoredString("vicinity",eColor::lightGreen)), "Swarms", eLogEntryCategory::network, 1);
            }
            else
            {
                tools->logEvent(localSTR + " " + tools->getColoredString(sdpTypeStr, eColor::lightCyan) + " from " + "'" + tools->bytesToString(sdp->getSourceID()) + "'"+" to "  +(destID.size() ? "'" + tools->bytesToString(destID) + "'" : tools->getColoredString("vicinity", eColor::lightGreen)), "Swarms", eLogEntryCategory::network, 1);
            }
        }
        //Logging - END

        //Operational Logic - BEGIN
        
        //Switch on SDP Type - BEGIN
        switch (sdpType)
        {

        case eSDPEntityType::eSDPEntityType::processICE: //pass it on - unicast

            //all ICEs need to be handed over to the destination as is.
            //[Security]: authentication is to rely on cross-browser Zero-Knowledge Proof. BUT we cannot rely on it unless the cross-browser connection has been already established.
            //[Routing]: 1) we rely the datagram to local endpoints no matter what.
            //           2) we rely to remote Core nodes only if peer was not found locally. (improve - do so only if local connection was authenticated in the name of the source).
            //              otherwise we open-up to spoofing.

             //Notify Local Clients - BEGIN
            for (std::vector <std::shared_ptr<CWebRTCSwarmMember>>::iterator it2 = members.begin(); it2 != members.end(); it2++) {
                if (tools->compareByteVectors((*it2)->getID(), sdp->getDestinationID()))
                {
                    agentFound = true;
                    peerConversation = (*it2)->getConversation();

                    //Targeted delivery - BEGIN
                    //first, check if target is directly reachable. Only then assume as delivered.
                    //todo: add authentication so that an attacker cannot eclipse legitimate deliveries.
                    if (peerConversation && peerConversation->getEndpoint()->getType() == eEndpointType::WebSockConversation
                        && peerConversation->getState()->getCurrentState() == eConversationState::running)
                    {
             
                        if (!nm->getWasDataSeen(receivedSDPHash, "localhostOut", false, peerConversation->getIPAddress(), true))//this relies on sequence numbers.
                        {
                            //^ should be fine. Even if multiple clients are behind the same NAT. ICE candidates are unicast and user-specific thus an image would differ.
                            //Notice that the above check ALLOWS for a SINGLE round-trip.

                                nm->sawData(receivedSDPHash, peerConversation->getIPAddress());

                                tools->logEvent(sdpDescription + " -  delivered to target immediate peer '" + tools->bytesToString(destID) + "'.", eLogEntryCategory::network, 1);
                                netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::Swarm, eNetReqType::process);
                                netMsg->setSource(publicIPVec);
                                netMsg->setSourceType(eEndpointType::IPv4);
                                netMsg->setHopCount(wasRouted?(sdpHopCount+1):0);
                                netMsg->setData(packedReceivedSDP);//repack received sdp

                                task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
                                task->setNetMsg(netMsg);
                                peerConversation->addTask(task);
                                delivered = true;//assume  data to be delivered; todo: require authentication to better protect against spoofing / eclipsing attacks.
                                // It would not be routed further.
                            
                        }
                        else
                        {
                            tools->logEvent(sdpDescription + " - " + tools->getColoredString("already seen by", eColor::lightCyberWine)+" '" + tools->bytesToString(destID) + "'.", eLogEntryCategory::network, 1);
                        }
                    }

                    //Targeted delivery - END

                }

            }
            //Notify Local Clients - END
            break;

        case eSDPEntityType::eSDPEntityType::processOffer://pass it on - unicast
            //[Security]: authentication is to rely on cross-browser Zero-Knowledge Proof.
            //all ICEs need to be handed over to the destination as is

            //[Routing]: 1) we rely a datagram to local endpoint if the target destination matches the local client's identifier, the one on file.
            //           2) we rely to remote Core nodes only if peer was not found locally. (improve - do so only if local connection was authenticated in the name of the source).
            //              otherwise we open-up to spoofing.

            //Notify Local Clients - BEGIN
            for (std::vector <std::shared_ptr<CWebRTCSwarmMember>>::iterator it2 = members.begin(); it2 != members.end(); it2++) {
                if (tools->compareByteVectors((*it2)->getID(), sdp->getDestinationID()))
                {
                    agentFound = true;
                    peerConversation = (*it2)->getConversation();

                    //Targeted delivery - BEGIN
                   //first, check if target is directly reachable. Only then assume as delivered.
                   //todo: add authentication so that an attacker cannot eclipse legitimate deliveries.
                    if (peerConversation && peerConversation->getEndpoint()->getType() == eEndpointType::WebSockConversation
                        && peerConversation->getState()->getCurrentState() == eConversationState::running)
                    {
                  
    

                        if (!nm->getWasDataSeen(receivedSDPHash, "localhostOut", false, peerConversation->getIPAddress()), true)//this relies on sequence numbers.
                        {//^ should be fine. Even if multiple clients are behind same NAT. SDP offers  are unicast and user-specific.
                            //Notice that the above check ALLOWS for a SINGLE round-trip.

                                nm->sawData(receivedSDPHash, peerConversation->getIPAddress());

                                tools->logEvent(sdpDescription + " -  delivered to target immediate peer '" + tools->bytesToString(destID) + "'.", eLogEntryCategory::network,1);
                                netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::Swarm, eNetReqType::process);
                                netMsg->setSource(publicIPVec);
                                netMsg->setSourceType(eEndpointType::IPv4);
                                netMsg->setHopCount(wasRouted?(sdpHopCount+1):0);
                                netMsg->setData(packedReceivedSDP);//repack received sdp

                                task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
                                task->setNetMsg(netMsg);
                                peerConversation->addTask(task);
                                delivered = true;//assume data delivered. It would not be routed further.
                            
                        }
                        else
                        {
                            tools->logEvent(sdpDescription + " - " + tools->getColoredString("already seen by", eColor::lightCyberWine) + " '" + tools->bytesToString(destID) + "'.", eLogEntryCategory::network, 1);
                        }
                    }
                    //Targeted delivery - END
                }

            }
            //Notify Local Clients - END
            break;

        case eSDPEntityType::eSDPEntityType::processOfferResponse://pass it on - unicast

            //[Security]: authentication is to rely on cross-browser Zero-Knowledge Proof.
            //[Routing]: 1) we rely a datagram to local endpoint if the target destination matches the local client's identifier, the one on file.
            //           2) we rely to remote Core nodes only if peer was not found locally. (improve - do so only if local connection was authenticated in the name of the source).
            //              otherwise we open-up to spoofing.
            // 
           
            //Notify Local Clients - BEGIN
   
            for (std::vector <std::shared_ptr<CWebRTCSwarmMember>>::iterator it2 = members.begin(); it2 != members.end(); it2++) {
                if (tools->compareByteVectors((*it2)->getID(), sdp->getDestinationID()))
                {
                    agentFound = true;
                    peerConversation = (*it2)->getConversation();
                    //Targeted delivery - BEGIN
                    //first, check if target is directly reachable. Only then assume as delivered.
                    //todo: add authentication so that an attacker cannot eclipse legitimate deliveries.
                    if (peerConversation && peerConversation->getEndpoint()->getType() == eEndpointType::WebSockConversation 
                        && peerConversation->getState()->getCurrentState() == eConversationState::running)
                    {
                      

                        if (!nm->getWasDataSeen(receivedSDPHash, "localhostOut", false, peerConversation->getIPAddress()), true)//this relies on sequence numbers.
                        {//^ should be fine. Even if multiple clients are behind same NAT. SDP offer responses are unicast and user-specific.
                            //Notice that the above check ALLOWS for a SINGLE round-trip.
                                nm->sawData(receivedSDPHash, peerConversation->getIPAddress());
                                tools->logEvent(sdpDescription + " -  delivered to target immediate peer '" + tools->bytesToString(destID) + "'.", eLogEntryCategory::network, 1);
                                netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::Swarm, eNetReqType::process);
                                netMsg->setSource(publicIPVec);
                                netMsg->setSourceType(eEndpointType::IPv4);
                                netMsg->setHopCount(wasRouted?(sdpHopCount+1):0);
                                netMsg->setData(packedReceivedSDP);//repack the received sdp
                                
                                task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
                                task->setNetMsg(netMsg);

                                peerConversation->addTask(task);
                                delivered = true;//assume data delivered. It would not be routed further.
                            
                        }
                        else
                        {
                            tools->logEvent(sdpDescription + " - " + tools->getColoredString("already seen by", eColor::lightCyberWine) + " '" + tools->bytesToString(destID) + "'.", eLogEntryCategory::network, 1);
                        }
                    
                    }
                    //Targeted delivery - END
                    //route
                }

            }
           
            //Notify Local Clients - END
            break;

        case eSDPEntityType::eSDPEntityType::joining://local processing + offer request to Swarm participants

            //Notice: this type of SDP is processed mainly by Core Nodes.

            //Routing: Always - since the Entity is of concern to each and every Swarm participant.

            //[IMPORTANT 1]: we MUST allow for multiple instances of peers pointing to same identity IF none of these are authenticated.
            //one may be attacker, while the other may not. There is no way of knowing. We need to make SDP datagrams flow.
            //In such a case protection relies on Zero-Knowledge-Proof authorization mechanics.

            //[IMPORANT 2]: notice that we might end-up with multiple entities, in the name of the same identity.
            //that is because join-request R1 may be coming from locally connected client over a web-socket, whereas we might be then seeing 
            //lots of requests being ping-pong'ed from other nodes as we spread the request throughout the network.

            //[IMPORTANT 3]: Local Authorized > Local Non-Authorized > Remote/Routed identities  

            //[IMPORTANT 4]: notice that eSDPEntityType::getOffer is dispatched only to locally connected clients as a result of 'joining' SDP request
            //               which might had been routed to the local Core node. 

            //[IMPORTANT 5]:  eSDPEntityType::joining is always broadcast.

            //Algorithm - BEGIN
            //[Security] that is thus a paramount importance *NOT to remove authenticated members of a Swarm*.
            //We need to iterate through all the entities and only eliminate entries that have not been authenticated.
            //IF, there is no authenticated entity, in the name of a given identity - then we MUST let all these entities be.
            // The reason is to allow for Web-RTC negotiations even in the presence of spoofed identities. In such a case
            // swarm members would be protected at the level of a Zero-Knowledge Proof authorization algorithm.
            // We would be letting all the SDP datagrams through, while a the actual data-exchange (between web-browsers) would proceed 
            // only among peers that have completed mutual Zero-Knowledge proof authentication.
            //Algorithm - END

            //notice that thus due to the below, peers would have different identifiers on various nodes.
            
            //on a node which has direct connectivity the IP above would be the real IP of a peer, on a node which acts as a hub, however - the
            //identifier would be based on the IP of a node which delivered the SDP Entity.

            //[Security]: notice that due to the above, routed SDP Entities cannot affect presence of a peer in the local representation of a Swarm,
            //had it been instantiated as a result of an SDP Entity delivered from another endpoint (another IP address) - which COULD be
            //a direct neighbor. 
       
            // Notice: local peer instance might be an immediate local neighboor* OR* a delegated instance(not immediatedly reachable).


            //DOS protection - BEGIN
            if (sameConversationPresentIdentity)
            {//peer already known from same conversation. 

                uint64_t recentJoinRequstTimestamp = sameConversationPresentIdentity->getJoinRequestTimestamp();
                sameConversationPresentIdentity->pingJoinRequest();
                int64_t now = tools->getTime();

                if ((now - recentJoinRequstTimestamp) < 5)
                {
                    tools->logEvent(tools->getColoredString("Won't process", eColor::lightPink) + " a Join-Request" + sourceTag + " - peer joined recently (seq: " + std::to_string(sdp->getSeqNr()) + ").", "Swarms", eLogEntryCategory::network, 1);
                    break; // <= here we *DO* break to avoid cross-wide DOS attacks.
                }
            }
            //DOS protection - END

            if (wasRouted && (bestKnownPeerIdentity || sameConversationPresentIdentity))
            {
                tools->logEvent(tools->getColoredString("Neglecting", eColor::lightPink) + " a routed Join-Request"+ sourceTag+" - peer already known.", "Swarms", eLogEntryCategory::network, 1);
                //we know about the peer and the current SDP datagram was routed from another node. do nothing. let it be. we know about it already.
               // break; <= no need to break. Let invitations be asked for - from the other local members.
            }
               
               
                //Members' Elimination - BEGIN
                //The Gist: we want local or even local authenticated members to be prioritized.
                // Local Authorized > Local Non-Authorized > Remote/Routed identities  <  !!!!!

                //i.e. we cannot let the received routed SDP datagrams affect what has been learned from the immediate web-socket connections.
       

                //A local web-socket connection has the possibility of actually overriding the world-view. 
               //BUT only if these are authenticated in the name of an identity reported within the SDP entity.

               //IMPORTANT: do not simply if (removeMemberByID(member->getID())) - as that would be prone to spoofing attacks.
                

                    if (dataDeliveryConversation)//check if conversation is authenticated.
                    {
                        if (tools->compareByteVectors(dataDeliveryConversation->getPeerAuthenticatedAsPubKey(), sdp->getSourceID()))
                        {
                            isConversationAuthed = true;
                        }
                    }
                    
                    /// Local Peers - BEGIN
                    if (dataDeliveryConversation->getEndpoint()->getType() == eEndpointType::WebSockConversation && !wasRouted)
                    {
                        if (isConversationAuthed)//current data delivery conversation was found to be authenticated in the name of the joined identity.
                        {//Authenticated Conversation - BEGIN
                            //remove ALL currently present entities regarding this identity.
                            //local immediate neighbor which has been authenticated has ultimate priority. We do NOT need other instances of same identity.
                            //i.e. remove both local and delegated instanced of this identity.
                            removeMemberByID(sdp->getSourceID(), false, false);// invalidate and eliminate current CANDIDATE only if not-auth'ed or connection dead.
                        
                            tools->logEvent(tools->getColoredString("Honoring", eColor::lightGreen) + " local Authenticated Join-Request from '" + tools->bytesToString(sameConversationPresentIdentity->getID()) + "'.", "Swarms", eLogEntryCategory::network, 1);
                            
                            //add peer instance
                            bestKnownPeerIdentity = std::make_shared<CWebRTCSwarmMember>(dataDeliveryConversation, sdp->getSourceID(), getID());
                            bestKnownPeerIdentity->pingJoinRequest();
                            
                            if (!addMember(bestKnownPeerIdentity, true, dataDeliveryConversation->getEndpoint(), sdp->getHopCount()))
                            {
                                tools->logEvent(tools->getColoredString("Won't add Swarm member", eColor::lightPink) + "'" + tools->bytesToString(sdp->getSourceID()) + "' - limits reached (seq: " + std::to_string(sdp->getSeqNr()) + ").", "Swarms", eLogEntryCategory::network, 1);
                                //break; <= no need to break. Let invitations be asked for - from the other local members
                            }
                            bestKnownPeerIdentity->ping();
                            members.push_back(bestKnownPeerIdentity);
                            sameConversationPresentIdentity = bestKnownPeerIdentity;
                        
                        }//Authenticated Conversation - END
                        else {
                            //Non-Authenticated Web-Sock Conversation - BEGIN
                            //we MUST allow for duplicates if none identity is authenticated; just remove instances with inactive connections (performed during swarm maintenance).
                            if (bestKnownPeerIdentity && bestKnownPeerIdentity->getConversation()->getEndpoint()->getType() == eEndpointType::WebSockConversation
                                && (tools->compareByteVectors(dataDeliveryConversation->getPeerAuthenticatedAsPubKey(), sdp->getSourceID())))
                            {//looks like there was an entity established through other local Authenticated conversation thiugh.
                                tools->logEvent(tools->getColoredString("Neglecting", eColor::lightPink) + " local Join-Request from '" + tools->bytesToString(sameConversationPresentIdentity->getID()) + "' - an authenticated conversation already present.", "Swarms", eLogEntryCategory::network, 1);

                                break;//we already are aware of an authenticated conversation in the name of this identity. a possible spoofing attack.
                                // Here we do break. We are already aware of an authenticated identity, and it is not the one performing this very join request.
                            }

                            if (!sameConversationPresentIdentity)
                            {//an identity was not established through this conversation, yet.
                               
                                tools->logEvent(tools->getColoredString("Honoring", eColor::lightGreen) + " local non-authenticated Join-Request from '" + tools->bytesToString(dataDeliveryConversation->getID()) + "'.", "Swarms", eLogEntryCategory::network, 1);
                                
                                //add peer instance
                                sameConversationPresentIdentity = std::make_shared<CWebRTCSwarmMember>(dataDeliveryConversation, sdp->getSourceID(), getID());
                                sameConversationPresentIdentity->pingJoinRequest();
                                if (!addMember(sameConversationPresentIdentity, true, dataDeliveryConversation->getEndpoint(), sdp->getHopCount()))
                                {
                                    tools->logEvent(tools->getColoredString("Won't add Swarm member", eColor::lightPink) + "'" + tools->bytesToString(sdp->getSourceID()) + "' - limits reached (seq: " + std::to_string(sdp->getSeqNr()) + ").", "Swarms", eLogEntryCategory::network, 1);
                                   // break;<= no need to break. Let invitations be asked for - from the other local members
                                }
                                sameConversationPresentIdentity->ping();

                                members.push_back(sameConversationPresentIdentity);
                            }
                            else
                            {
                                tools->logEvent(tools->getColoredString("Won't add Swarm member", eColor::lightPink) + "'" + tools->bytesToString(sdp->getSourceID()) + "' - identity from same conversation already present (seq: " + std::to_string(sdp->getSeqNr()) + ").", "Swarms", eLogEntryCategory::network, 1);
                            }
                        }//Non-Authenticated Web-Sock Conversation - END
                    }//Local Peers - END
                    else if (wasRouted) //Delegated Peers - BEGIN
                    {// if join-reques was routed, ADD only of no entity present at all.
                        if (!bestKnownPeerIdentity)
                        {
                            //add peer instance
                            bestKnownPeerIdentity  = std::make_shared<CWebRTCSwarmMember>(dataDeliveryConversation, sdp->getSourceID(), getID());
                            bestKnownPeerIdentity->pingJoinRequest();
                            if (!addMember(bestKnownPeerIdentity, true, dataDeliveryConversation->getEndpoint(), sdp->getHopCount()))
                            {
                                tools->logEvent(tools->getColoredString("Won't add Swarm member", eColor::lightPink) + "'" + tools->bytesToString(sdp->getSourceID()) + "' - limits reached (seq: " + std::to_string(sdp->getSeqNr()) + ").", "Swarms", eLogEntryCategory::network, 1);
                                //break; <= no need to break. Let invitations be asked for - from the other local members.
                            }
                            bestKnownPeerIdentity->ping();
                            sameConversationPresentIdentity = bestKnownPeerIdentity;
                            members.push_back(bestKnownPeerIdentity);
                           // break; <= no need to break. Let invitations be asked for - from the other local members.
                        }
                    }//Delegated Peers - END
                    

                    //todo: there would still be a copy of it within the 'members' local variable.

                

                //Members' Elimination - END
         
       

            //confirm registration - begin 
            // here confirmation is dispatched directly to peer who issued the join-request.
            if (wasRouted == false && dataDeliveryConversation->getEndpoint()->getType() == eEndpointType::WebSockConversation)
            {//confirmation delivered only by the immediate ⋮⋮⋮ Node.
                CSDPEntity e(nullptr, eSDPEntityType::control, getID(), std::vector<uint8_t>(), sdp->getSourceID());
                e.setStatus(eSDPControlStatus::joined);

                netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::Swarm, eNetReqType::notify);
                netMsg->setSource(publicIPVec);
                netMsg->setSourceType(eEndpointType::IPv4);
                netMsg->setHopCount(wasRouted ? (sdpHopCount + 1) : 0);
                netMsg->setData(e.getPackedData());
                task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
                task->setNetMsg(netMsg);
                bytes = netMsg->getPackedData();
                //v--- no need to keep track of deliveried from join-confirmations issued to immediate peeers.

                    // if (!nm->getWasDataSeen(CCryptoFactory::getInstance()->getSHA2_256Vec(bytes), "localhostOut", false, tools->bytesToString(sdpDeliveryIP)))//this relies on sequence numbers.
                    // {////^ should be fine. Even relies on a sequence number.
                    //nm->sawData(receivedSDPHash, dataDeliveryConversation->getIPAddress());
                   // nm->sawData(receivedSDPHash, conversations[i]->getIPAddress());
                dataDeliveryConversation->addTask(task); //notice that if we're a hub the datagram might be flowing to yet another hub, to finally arrive at the
                //hub which has the peer as an immediate neighboor.
                //confirm registration - end
            }
            

            //request SDP Offers from all the immediate neighbors.
            /*Notice: the 'join' request MIGHT had not originated from peer P1 connected to this very local node.
            * In such a case, nonetheless - SDP responses include ICE candidates, these need to be forwarded from peers PX
            * - connected to this very node - once these are received.
            */

      
            //Notify Local Clients - BEGIN
            // (..) and request SDP Offers from each (..).
            //IMPORTANT: here what we do is actually translating a 'join'-request (broadcast) into 'getOffer'-requests (unicast)
            //           which are delivered to particular swarm members - registered with the local Core node.
            for (std::vector <std::shared_ptr<CWebRTCSwarmMember>>::iterator it2 = members.begin(); it2 != members.end(); it2++) {
                if (tools->compareByteVectors((*it2)->getID(), sdp->getSourceID()))
                    continue;
                peerConversation = (*it2)->getConversation();

                if (peerConversation && peerConversation->getState()->getCurrentState() == eConversationState::running)
                {
                    resultedInLocalInvitationsCount++;
                  
                    CSDPEntity e(nullptr, eSDPEntityType::getOffer, getID(), sdp->getSourceID(), (*it2)->getID());
                    e.setSeqNr(sdp->getSeqNr());
                    //Logging - BEGIN
                    if (loggingEnabled)
                    {
                      
                          tools->logEvent(tools->getColoredString("Requesting SDP Offer", eColor::lightCyan)+ " from '" +tools->bytesToString((*it2)->getID()) + "'" + + " for " + "'" + tools->bytesToString(sdp->getSourceID()) + "'" , "Swarms", eLogEntryCategory::network, 1);
                       
                    }
                    //Logging - END

                    //IMPORTANT: copy over the exact requested capabilities 
                    e.setCapabilities(sdp->getCapabilities());
                    netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::Swarm, eNetReqType::request);
                    netMsg->setSource(publicIPVec);
                    netMsg->setSourceType(eEndpointType::IPv4);
                    netMsg->setHopCount(wasRouted?(sdpHopCount+1):0);
                    bytes = e.getPackedData();

                   // if (!nm->getWasDataSeen(CCryptoFactory::getInstance()->getSHA2_256Vec(packed), "localhostOut", false, tools->bytesToString(sdpDeliveryIP)))//this relies on sequence numbers.
                   // { ^ DO NOT. Same offer may be broadcast to many client behind same NAT.
                        netMsg->setData(bytes);

                        task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
                        task->setNetMsg(netMsg);
                        peerConversation->addTask(task);
                   // }
                }
                else
                {
                    tools->logEvent("Won't notify '" +tools->bytesToString((*it2)->getID())+"' about new peer. Swarm member is not a direct neighbor.", "Swarms", eLogEntryCategory::network, 1, eLogEntryType::warning);
                }
              
            }

            if (loggingEnabled)
            {
                tools->logEvent("Join-request"+sourceTag +" resulted in "+ tools->getColoredString(std::to_string(resultedInLocalInvitationsCount), eColor::lightCyan)+" local offer - requests.", "Swarms", eLogEntryCategory::network, 1, eLogEntryType::notification);
            }
            

            //Notify Local Clients - END

           
            break;
        case eSDPEntityType::control:
            //Purpose: indicates that a Core node is no longer to maintain this Swarm.
            //[Security]: from the perspective of web-browsers: authentication is to rely on cross-browser Zero-Knowledge Proof.
            delivered = true;// we never route this type of an SDP datagram. Remote peers have no control over whether the Swarm remains operational on this node.

            if (wasRouted)
            {
                break;//IGNORE. We now know that a neighbor Core node terminated the swarm.
                //Still, that is not to affect our local node, in any way.
                //Especially - we do not want this to affect our local clients.
                //Thus, we ignore the datagram.
            }

            //Notify Local Clients - BEGIN
     
            if (sdp->getStatus() == eSDPControlStatus::swarmClosing)
            {
                for (std::vector <std::shared_ptr<CWebRTCSwarmMember>>::iterator it2 = members.begin(); it2 != members.end(); it2++) {//NOTE: the underlying list is not modified or iterated here

                    peerConversation = (*it2)->getConversation();

                    if (peerConversation && peerConversation->getState()->getCurrentState() == eConversationState::running)
                    {
                       // if (mTools->compareByteVectors(ip, (*it2)->getConversation()->getEndpoint()->getAddress()) && mTools->compareByteVectors((*it2)->getID(), sdp->getSourceID())
                       //     && peerConversation->getState()->getCurrentState() == eConversationState::running)
                        //{
                        
                            CSDPEntity e(nullptr, eSDPEntityType::bye, getID(), sdp->getSourceID(), (*it2)->getID());
                            e.setStatus(eSDPControlStatus::swarmClosing);
                            netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::Swarm, eNetReqType::request);
                            netMsg->setSource(publicIPVec);
                            netMsg->setSourceType(eEndpointType::IPv4);
                          
                            bytes = e.getPackedData();
                           // if (!nm->getWasDataSeen(CCryptoFactory::getInstance()->getSHA2_256Vec(packed), "localhostOut", false, tools->bytesToString(sdpDeliveryIP)))//this relies on sequence numbers.
                            //{^ DO NOT. may be broadcast to many clients behind same NAT.
                                netMsg->setData(bytes);

                                task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
                                task->setNetMsg(netMsg);
                                peerConversation->addTask(task);

                                (*it2)->markForDeletion();// the peer will be marked for deletion within the Swarm's queue. It MIGHT get deleted BEFORE the notification is dispatched
                                //- from these data structures which IS ok. The WebSocket conversation would remain within the WebSocket's Controller until the latter deems it be closed.
                                continue;
                           // }
                       // }
                    }


                }
            }
            //Notify Local Clients - END
            break;
        case eSDPEntityType::eSDPEntityType::bye://always routed - its nature is about being broadcast to all swarm members.
            //DO NOT lock specialized mutexes over here; mark the object for deletion instead and let the maintenance procedure take over

    

            if (wasRouted)
            {
                ////[Security]: authenticate through signatures would be prone to reply attacks. We could AUTH through Transmission Tokens though.
                // At cross-browsers level we could AUTH based  on ZKP.

                //we want local clients to know when peer quit the Swarm.
                //gentle / polite quits could rely on cross-browse signaling.
                //abrupt quits can be handled through cross-browser ping datagrams.
                //non-authenticated, relied quit datagrams are most risky thus we ignore.
                ignore = true;
            }

            if (ignore)
            {
                //Logging - BEGIN
                if (loggingEnabled)
                {
                    tools->logEvent(remoteSTR + tools->getColoredString(sdpTypeStr, eColor::lightCyan) + " from " + "'" + tools->bytesToString(sdp->getSourceID()) + "' was ignored", "Swarms", eLogEntryCategory::network, 1, eLogEntryType::warning);
                }
                //Logging - END

            }

            //Notify Local Clients - BEGIN

            for (std::vector <std::shared_ptr<CWebRTCSwarmMember>>::iterator it2 = members.begin(); it2 != members.end(); it2++) {//NOTE: the underlying list is not modified or iterated here

                peerConversation = (*it2)->getConversation();

                if (peerConversation && peerConversation->getState()->getCurrentState() == eConversationState::running)// is the client reachable from local node? Notice that it might not be - it might had been created solely based on a 'joining'-request
                {//delivered from another Core node.
                  

                    CSDPEntity e(nullptr, eSDPEntityType::bye, getID(), sdp->getSourceID(), (*it2)->getID());//generates a new source sequence number.
                    netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::Swarm, eNetReqType::request);
                    netMsg->setSource(publicIPVec);
                    netMsg->setSourceType(eEndpointType::IPv4);
                   // netMsg->setHopCount(wasRouted?(sdpHopCount+1):0);
                    bytes = e.getPackedData();
                    //if (!nm->getWasDataSeen(CCryptoFactory::getInstance()->getSHA2_256Vec(packed), "localhostOut", false, tools->bytesToString(sdpDeliveryIP)))//this relies on sequence numbers.
                    //{^ DO NOT. may be broadcast to many clients behind same NAT.s
                        netMsg->setData(e.getPackedData());
                        task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
                        task->setNetMsg(netMsg);
                        peerConversation->addTask(task);
                    //}

                    //first dispatch notifications THEN mark for deletion ^(would accomplish anyway but..)
                        if (!wasRouted)
                        {
                            if (mTools->compareByteVectors(sdpDeliveryIP, peerConversation->getEndpoint()->getAddress()) && tools->compareByteVectors((*it2)->getID(), sdp->getSourceID()))
                            {
                                (*it2)->markForDeletion();
                                continue;
                            }
                        }
                }
            }
            //Notify Local Clients - END

            break;
        }


        //Outgress (inter-core) Routing of SDP Entities - BEGIN

        //Here it turns out that we need to further route the SDP entry.
        //We do NOT increase the sequence number and let it flow.
        //We could increment the hop-count though. Still, as of now we route at the SDP-layer, and the SDP layer, as it is - it does not
        //contain a hop-count.
        //things to consider: 1) employ routing at the level of CNetMsg containers
        //                    2) make SDP Entities actually 'route-able'.
        if (!delivered)//<- assumed as delivered only if explicit local neighbor found to be available and data delivered.
        {//the SDP datagram needs to be routed further away..
     
            //Logging - BEGIN
            if (loggingEnabled)
            {

                if (sdpHopCount > HOP_COUNT_LIMIT)
                {
                    tools->logEvent(sdpDescription + tools->getColoredString(" is being dropped", eColor::cyborgBlood)+". Hop-count limit reached.", "Swarms", eLogEntryCategory::network, 1, eLogEntryType::warning);
                    continue;
                }

                if (wasRouted)
                {
                    tools->logEvent(sdpDescription + " is being routed away..'", "Swarms", eLogEntryCategory::network, 1);
                }
                else
                {
                    tools->logEvent(sdpDescription + " is being routed away..'", "Swarms", eLogEntryCategory::network, 1);
                }
            }
            //Logging - END

            //Important: we need authentication of identities registered at local instances of a Swarm (signaling nodes)
            /* otherwise, we are opening up to identity spoofing attacks - Eve could register at a local node using a fake identity
            * and hijack SDP datagrams destined to Bob.
           [@CodesInChaos]: Thus either change this to always undelivered or use authentication.
            */
            spreadCount = 0;
      

            //now iterate over conversations and deliver data
            for (uint64_t i = 0; i < conversations.size(); i++) //this only includes Core nodes (mobile apps and web-clients excluded).
            {

                destIPStr = tools->bytesToString(conversations[i]->getEndpoint()->getAddress());

                if (nm->isIPAddressLocal(conversations[i]->getEndpoint()->getAddress()))
                    continue;//do not deliver to myself.

                if (nm->getWasDataSeen(receivedSDPHash, std::string(),false,conversations[i]->getIPAddress()))
                {
                    continue;//that node delivered the SDP.
                 }
     
                //todo: make it use the internal routing table.
                if (!nm->getWasDataSeen(receivedSDPHash, "localhost", false, destIPStr))//this indirectly relies on sequence numbers.
                {
                    netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::Swarm, eNetReqType::route);
                    netMsg->setSource(publicIPVec);
                    netMsg->setSourceType(eEndpointType::IPv4);
                    netMsg->setHopCount(wasRouted?(sdpHopCount+1):0);
                    netMsg->setData(packedReceivedSDP);//repack the received sdp

                    task = std::make_shared<CNetTask>(eNetTaskType::sendNetMsg);
                    task->setNetMsg(netMsg);
                    conversations[i]->addTask(task);
                    nm->sawData(receivedSDPHash, conversations[i]->getIPAddress());
                    spreadCount++;

                    //Logging - BEGIN
                    if (loggingEnabled)
                    {
                        tools->logEvent("Routing " + sdpDescription + " to " + destIPStr, "Swarms", eLogEntryCategory::network, 1);
                    }
                    //Logging - END


                }
                else
                {
                    tools->logEvent("[SDP ROUTING}: "+ sdpDescription + " - " + tools->getColoredString("already seen by", eColor::lightCyberWine) + " '" + tools->bytesToString(destID) + "'.", eLogEntryCategory::network, 1);
                }
            }
            //Logging - BEGIN
            if (loggingEnabled)
            {
                
                if (spreadCount == 0)
                {
                    tools->logEvent("[SDP ROUTING}: Nobody to share the SDP datagram with..", "Swarms", eLogEntryCategory::network, 1, eLogEntryType::warning);
                }
                else
                {
                    tools->logEvent("[SDP Spread Factor]: " + tools->getColoredString(std::to_string(spreadCount), eColor::blue), "Swarms", eLogEntryCategory::network, 1, eLogEntryType::warning);
                }
            }
            //Logging - END
        }
        else
        {
            if (loggingEnabled)
            {
                tools->logEvent(sdpDescription + tools->getColoredString(" won't be routed. Already delivered to immediate peer.", eColor::lightPink), "Swarms", eLogEntryCategory::network, 1, eLogEntryType::warning);
            }
            //nothing more to do for this datagram.
        }

        //Switch on SDP Type - END
        // 
        //Operational Logic - END
        
        //Outgress (inter-core) Routing of SDP Entities - END
    }


    return true;
}

bool  CWebRTCSwarm::genAndRouteSDP(eSDPEntityType::eSDPEntityType type,  std::shared_ptr<CConversation> conversation, eSDPControlStatus::eSDPControlStatus controlStatus,
    std::vector<uint8_t> sourceID, eNetReqType::eNetReqType reqType)
{
    if (conversation == nullptr)
        return false;
    std::vector<uint8_t> publicIPVec = getTools()->stringToBytes(getNetworkManager()->getPublicIP());
    CSDPEntity e(nullptr,eSDPEntityType::bye, getID(), sourceID);
    e.setStatus(controlStatus);
    std::shared_ptr <CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetEntType::eNetEntType::Swarm, eNetReqType::request);
    netMsg->setSource(publicIPVec);
    netMsg->setSourceType(eEndpointType::IPv4);
    netMsg->setData(e.getPackedData());

    std::shared_ptr <CNetTask>  task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);
    task->setData(netMsg->getPackedData());
    conversation->addTask(task);
    return true;
}

bool CWebRTCSwarm::addSDPEntity(std::shared_ptr<CSDPEntity> entity)
{
    std::lock_guard<std::mutex> lock(mGuardian);
	if (entity == nullptr)
		return false;
	 mPendingSDPEntities.push_back(entity);
     pingActvity();
	 return true;
}

/// <summary>
/// Constructs and/or refreshes the abstract endpoint describing this very conversation.
/// Optionally updates the Routing Table (default).
/// </summary>
/// <param name="updateRT"></param>
void CWebRTCSwarm::refreshAbstractEndpoint(bool updateRT)
{
    mAbstractEndpoint = std::make_shared<CEndPoint>(mID, eEndpointType::WebRTCSwarm);

    if (updateRT)
    {
        if (std::shared_ptr<CDataRouter> router = mRouter.lock())
        {
            //update the routing table so that the router can route to this conversation based on the remote conversation ID.
            std::shared_ptr<CEndPoint> target = std::make_shared<CEndPoint>(mID, mAbstractEndpoint->getType(), nullptr, 0);

            //additional entries targeting particular Swarm members(Peers) and pointing to this very WebRTC Swarm are created on peers' additions.
            router->updateRT(target, mAbstractEndpoint,0,eRouteKowledgeSource::WebRTCSwarm);
        }
    }
}

//Registers a Swarm client (may be either immediate or delegated).
//May update the routing table.
bool CWebRTCSwarm::addMember(std::shared_ptr<CWebRTCSwarmMember> member, bool updateRT, std::shared_ptr<CEndPoint> deliveredFrom, uint64_t distance)
{
    std::lock_guard<std::mutex> lock(mMembersGuardian);

	if (mMembers.size() >= mSizeLimit)
		return false;

    mMembers.push_back(member);
   
    if (updateRT)
    {
        if (std::shared_ptr<CDataRouter> router = mRouter.lock())
        {
            //create an entry within the routing table targeting particular Peer and pointing to this very WebRTC Swarm.
            //An additional entry targeting whole Swarm as such had been created during Swarm's initialization.
            std::shared_ptr<CEndPoint> ep = std::make_shared<CEndPoint>(mID, eEndpointType::PeerID, nullptr, 0);
            std::shared_ptr<CEndPoint> ep2 = std::make_shared<CEndPoint>(mID, eEndpointType::SwarmMember, nullptr, 0);
            router->updateRT(ep, deliveredFrom==nullptr?getAbstractEndpoint(): deliveredFrom, distance);
        }
    }
    
    pingActvity();
    return true;
}

bool CWebRTCSwarm::removeMemberByID(std::vector<uint8_t> ID, bool needsToBeNonAuthed , bool connNeedsToBeDead)
{
    std::lock_guard<std::mutex> lock(mMembersGuardian);
    bool removed = false;
    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::vector< std::shared_ptr<CWebRTCSwarmMember> >::iterator m = mMembers.begin();
    std::shared_ptr<CConversation> conv;
    while (m != mMembers.end()) {
        if (mTools->compareByteVectors((*m)->getID(), ID)) {
            // if track is empty, remove it

            if (connNeedsToBeDead || needsToBeNonAuthed)
            {
                conv = (*m)->getConversation();

                if (!conv)
                {
                    m = mMembers.erase(m);
                    removed = true;

                    continue;
                }
                else
                {   
                    if (connNeedsToBeDead)
                    {
                        if (conv->getState()->getCurrentState() == eConversationState::ended)
                        {
                            m = mMembers.erase(m);
                            removed = true;
                            continue;
                        }
                    }
                    if (needsToBeNonAuthed)
                    {
                        if (!tools->compareByteVectors(conv->getPeerAuthenticatedAsPubKey(),ID))
                        {
                            m = mMembers.erase(m);
                            removed = true;
                            continue;
                        }
                    }

                    if (!removed)
                    {
                        ++m;
                    }

                }

            }
            else {
                m = mMembers.erase(m);
                removed = true;
            }
        }
        else {
            ++m;
        }
    }
    return removed;
}
void  CWebRTCSwarm::pingActvity()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    this->mLastActivity = CTools::getInstance()->getTime();
}

std::vector<std::shared_ptr<CWebRTCSwarmMember>> CWebRTCSwarm::getMembers()
{
    std::lock_guard<std::mutex> lock(mMembersGuardian);
    std::vector<std::shared_ptr<CWebRTCSwarmMember>> toRet;

    for (int i = 0; i < mMembers.size(); i++)
        toRet.push_back(mMembers[i]);

    return toRet;
}



std::shared_ptr<CWebRTCSwarmMember> CWebRTCSwarm::findMemberByID(std::vector<uint8_t> ID, std::vector<uint8_t> conversationID, bool preferAuthenticated)
{
    std::lock_guard<std::mutex> lock(mMembersGuardian);
    if (ID.size() == 0)
        return nullptr;
    std::shared_ptr< CWebRTCSwarmMember> candidate;
    std::shared_ptr<CTools> tools = CTools::getInstance();
    std::shared_ptr<CConversation> candidateConv, conv;

    for (uint64_t i = 0; i < mMembers.size(); i++)
    {
        conv = mMembers[i]->getConversation();
        if (tools->compareByteVectors(mMembers[i]->getID(), ID))
        {
            if (conversationID.size() > 0)//if we require the Conversation IDs  to match as well (and we do if provided)
            {
                if (mMembers[i]->getConversation() == nullptr)
                    break;
                if (!(mTools->compareByteVectors(conversationID, mMembers[i]->getConversation()->getID())))
                    break;

            }


            if (candidate)
            {//there's already a candidate.
                candidateConv = candidate->getConversation();

                //always prefer active WebSocket connections.

                if (candidateConv == nullptr || (candidateConv->getEndpoint()->getType() != eEndpointType::WebSockConversation) 
                    || (candidateConv->getEndpoint()->getType() == eEndpointType::WebSockConversation && candidateConv->getState()->getCurrentState() != eConversationState::running))
                {
                    candidate = mMembers[i];
                }


                if (preferAuthenticated)
                {


                    if (candidateConv)
                    {
                        if (conv && conv->getPeerAuthenticated() && tools->compareByteVectors(conv->getPeerAuthenticatedAsPubKey(), ID))
                        {
                            candidate = mMembers[i];
                        }
                        else
                        {
                            //do nothing.
                        }
                    }
                }
            }
            else
            {
                candidate = mMembers[i];
            }

        }

    }

    return candidate;
}



std::shared_ptr<CWebRTCSwarmMember> CWebRTCSwarm::findMemberByIP(std::vector<uint8_t> IPAddress)
{
    std::lock_guard<std::mutex> lock(mMembersGuardian);
    if (IPAddress.size() == 0)
        return nullptr;

    std::shared_ptr<CTools> tools = CTools::getInstance();
    for (uint64_t i = 0; i < mMembers.size(); i++)
    {
                if (mMembers[i]->getConversation() == nullptr)
                    break;
                if (!(mTools->compareByteVectors(IPAddress, mMembers[i]->getConversation()->getEndpoint()->getAddress())))
                    break;

            return mMembers[i];

    }
    return nullptr;
}


void CWebRTCSwarm::close()
{
    std::lock_guard<std::mutex> lock1(mMembersGuardian);//Lock more specialized critical section first!
    std::lock_guard<std::mutex> lock2(mGuardian);
 
    for (std::vector <std::shared_ptr<CWebRTCSwarmMember>>::iterator it = mMembers.begin(); it != mMembers.end();++it ) {
        if ((*it)->getConversation() != nullptr)
        {
            genAndRouteSDP(eSDPEntityType::control, (*it)->getConversation(), eSDPControlStatus::swarmClosing);
            //the peer will be marked for deletion AFTER msg has been routed
        }
    }
}
/// <summary>
/// Broadcasts datagrams across all swarm-members.
/// The function returns the number of peers the datagram delivery had been attempted to. 
/// Warning: the function does NOT return the number of successful, confirmed deliveries.
/// Under the hood an async data-delivery task is created for each known swarm member.
/// Each web-peer MIGHT forward the message even further (to another web-browser or full-node)
/// </summary>
/// <param name="msg"></param>
/// <returns></returns>
uint64_t CWebRTCSwarm::broadcastMsg(std::shared_ptr<CNetMsg> msg)
{
    if (msg == nullptr)
        return 0;

    //Local Variables - BEGIN
    uint64_t deliveredTo = 0;
    std::shared_ptr<CNetTask> task;
    std::vector<uint8_t> bytes = msg->getPackedData();//IMPORTANT: performance. Do this only ONCE.
    //Local Variables - END

    //Operational Logic- BEGIN
     if (bytes.size() == 0)
         return 0; //should not happen
    std::lock_guard<std::mutex> lock(mMembersGuardian);
    for (uint64_t i = 0; i < mMembers.size(); i++)
    {
      //Note: we do NOT use eNetTaskType::sendNetMsg over here as we do NOT want ANY modifications to the routed datagram to take place.
      //Most importantly; we do NOT want msg to be re-encrypted with local-conversation's specific session key.
      //**i.e. - the datagram is simply ROUTED **
        task = std::make_shared<CNetTask>(eNetTaskType::sendRAWData);//delivers the routed CNetMsg as a byte array(RAW data transfer)
      //the web-browser supports the format and expects a CNetMsg anyway.

        msg->incHops();//since the msg might be routed further by the web-peer this particular field is mutable i.e. not protected by an optional signature
        task->setData(bytes);
        mMembers[i]->getConversation()->addTask(task);
        deliveredTo++;
    }
    //Operational Logic- END

    return deliveredTo;
}
/// <summary>
/// Does maintenance of the SDP processing queue.
/// Does maintenance of the reported Swarm-members.
/// </summary>
void CWebRTCSwarm::doMaintenance()
{
        //***************WARNING: mutex armed over here********************
         mMembersGuardian.lock();//lock more specialized/inner-most critical section first
        //^***************WARNING: mutex armed over here********************^ REMEMBER to unlock do NOT exit the flow

        std::lock_guard<std::mutex> lock2(mGuardian);

        size_t time = CTools::getInstance()->getTime();
        uint64_t removedSDPEntries = 0;
        uint64_t removedAgents= 0;
        std::shared_ptr<CConversation> conv;
        uint64_t lastTimePeerActive = 0;
        //Agents' Maintenance - BEGIN

      
        for (std::vector <std::shared_ptr<CWebRTCSwarmMember>>::iterator it = mMembers.begin(); it != mMembers.end(); ) {
            time = CTools::getInstance()->getTime();
            conv = (*it)->getConversation();
            lastTimePeerActive = (*it)->getLastTimeActive();
            if (conv && conv->getEndpoint()->getType() == eEndpointType::WebSockConversation)
            {//Local Peers - BEGIN

                //time-outs are to be handled by the conversations sub-system, not around here.

                if (//time - (*it)->getLastTimeActive() > mInactivityRemovalThreshold ||//remove if not pinged recently by client
                    ((*it)->getIsMarkedForDeletion()) ||//it was decided the peer needs to be removed, somewhere else.
                    //decisions based on web-sock conversation follow
                    ((*it)->getConversation() == nullptr || (*it)->getConversation()->getState()->getCurrentState() == eConversationState::ended ))
                 ///       ((time - (*it)->getConversation()->getState()->getLastActivity()) > mInactivityRemovalThreshold))
                    //)
                {  
                    //Warning: mutex armed above
                    it = mMembers.erase(it);
                    removedAgents++;
                }
                else {//warning: make sure iterator is not incremented twice (after deletion above).
                    ++it;
                }
            }//Local Peers - END
            else {
                //Delegated Peers - BEGIN
                
                if (lastTimePeerActive != 0 && (time - lastTimePeerActive) > 10800)//remove after 3 hours.
                {
                    it = mMembers.erase(it);
                    removedAgents++;
                }
                else//warning: make sure iterator is not incremented twice (after deletion above).
                {
                    ++it;
                }
                //Delegated Peers - END
               
            }
        }
        mMembersGuardian.unlock();//release ASAP

        //Agents' Maintenance - END

        //maintenance of WebSocket conversation happens through the WebSocket's server
        //the WebRTC conversation might be going long after the matchmaking has completed
        //peers are supposed to inform full-node at least at mMinKeepAlive interval, otherwise they will be removed
        //IF the web sockets connection to them is not active anymore.

        //all SDPEntities are removed after mSDPEntityProcessingTimeout expires

        //SDP entries Maintenance - BEGIN
        for (std::vector <std::shared_ptr<CSDPEntity>>::iterator it = mPendingSDPEntities.begin(); it != mPendingSDPEntities.end(); ) {
            time = CTools::getInstance()->getTime();
            if ((*it)->getTimeCreated() > time)
            {
                ++it;
                continue;//should not happen
            }

            if (!(*it)->getIsPending() || ((time - (*it)->getTimeCreated()) > mSDPEntityProcessingTimeout))
            {

                it = mPendingSDPEntities.erase(it);
                removedSDPEntries++;
            }
            else {
                ++it;
            }
        }
        //SDP entries Maintenance - END

    
}

std::vector<uint8_t> CWebRTCSwarm::getID()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mID;
}
