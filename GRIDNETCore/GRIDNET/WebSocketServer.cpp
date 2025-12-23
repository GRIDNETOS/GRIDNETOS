#include "WebSocketServer.h"

#include "CGlobalSecSettings.h"
#include "WebSocketResponse.h"
#include "conversation.h"
#include "BlockchainManager.h"
#include "NetworkManager.h"
#include "NetMsg.h"
#include "EEndPoint.h"
#include "DTIServer.h"
#include "conversationState.h"
#include "CGlobalSecSettings.h"
#include "webRTCSwarm.h"
#include "DataRouter.h"
#include "websocket.h"
#include "webRTCSwarmMember.h"
#include "ThreadPool.h"

bool CWebSocketsServer::initialize()
{
    return initializeServer();
}

void CWebSocketsServer::stop()
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
 
    //firs ensure no new session come-in
    if (getStatus() == eManagerStatus::eManagerStatus::stopped)
        return;

    mStatusChange = eManagerStatus::eManagerStatus::stopped;
    if (!mControllerThread.joinable() && getStatus() != eManagerStatus::eManagerStatus::stopped)//controller is dead; we need first to thwart it for to enable for a state-transmission.
        mControllerThread = std::thread(&CWebSocketsServer::mControllerThreadF, this);

    while (getStatus() != eManagerStatus::eManagerStatus::stopped && getStatus() != eManagerStatus::eManagerStatus::initial)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (mControllerThread.joinable())
        mControllerThread.join();

    //then clean-up data structures / free memory


    doCleaningUp(true);//kill all active ones

    tools->writeLine("Web-Sockets Conversations Manager killed;");

    //ix::uninitNetSystem();
}

void CWebSocketsServer::pause()
{
}

void CWebSocketsServer::resume()
{
}

void CWebSocketsServer::mControllerThreadF()
{

    //Local Variables - BEGIN
    std::shared_ptr<CTools> tools = getTools();
    std::string tName = "WebSockets-Conversations Controller";
    tools->SetThreadName(tName.data());
    setStatus(eManagerStatus::eManagerStatus::running);
    bool wasPaused = false;
    if (mStatus != eManagerStatus::eManagerStatus::running)
        setStatus(eManagerStatus::eManagerStatus::running);
    uint64_t justCommitedFromHeaviestChainProof = 0;
    //Local Variables - END

    // Operational Logic - BEGIN
    mWebSocketServerThread = std::thread(&CWebSocketsServer::WebSocketServerThreadF, this);

    while (mStatus != eManagerStatus::eManagerStatus::stopped)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
                    if (mWebSocketServerThread.native_handle() != 0)
                    {
                        while (!mWebSocketServerThread.joinable())
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        mWebSocketServerThread.join();
                    }

                    wasPaused = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        if (wasPaused)
        {
            tools->writeLine("My thread operations are now resumed. Commencing further..");
            mWebSocketServerThread = std::thread(&CWebSocketsServer::WebSocketServerThreadF, this);
            mStatus = eManagerStatus::eManagerStatus::running;
        }


        if (mStatusChange == eManagerStatus::eManagerStatus::stopped)
        {
            mStatusChange = eManagerStatus::eManagerStatus::initial;
            mStatus = eManagerStatus::eManagerStatus::stopped;
        }

    }
    // Operational Logic - END

    doCleaningUp(true);//kill all the connections; do not allow for Zombie-connections
    setStatus(eManagerStatus::eManagerStatus::stopped);

   
}

eManagerStatus::eManagerStatus CWebSocketsServer::getStatus()
{
    std::lock_guard<std::recursive_mutex> lock(mStatusGuardian);
    return mStatus;
}

void CWebSocketsServer::setStatus(eManagerStatus::eManagerStatus status)
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::recursive_mutex> lock(mStatusGuardian);
    if (mStatus == status)
        return;
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
        tools->writeLine("I'm now in an unknown state;/");
        break;
    }
}

void CWebSocketsServer::requestStatusChange(eManagerStatus::eManagerStatus status)
{
    std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
    mStatusChange = status;
}

eManagerStatus::eManagerStatus CWebSocketsServer::getRequestedStatusChange()
{
    std::lock_guard<std::mutex> lock(mStatusChangeGuardian);
    return mStatusChange;
}

size_t CWebSocketsServer::getLastTimeCleanedUp()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mLastTimeCleaned;
}

size_t CWebSocketsServer::getActiveSessionsCount()
{
    std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
    return mConversations.size();
}

size_t CWebSocketsServer::getMaxSessionsCount()
{
    return CGlobalSecSettings::getMaxWebSocketConversationsCount();
}

void CWebSocketsServer::setLastTimeCleanedUp(size_t timestamp)
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    mLastTimeCleaned = timestamp != 0 ? (timestamp) : (tools->getTime());
}


CWebSocketsServer::CWebSocketsServer(std::shared_ptr<ThreadPool> pool, std::shared_ptr<CBlockchainManager> bm, bool initSwarmMechanics)
{
    mThreadPool = pool;
    mMaxWarningsCount = 10;
    mLastAliveListingTimestamp = 0;
    mMaxSwarmInactivityThreshold = CGlobalSecSettings::getCloseWebRTCSwarmAfter();
    mLastSwarmsTick = 0;
    mMaxActiveSwarmsCounts = 10000;
    mLastTimeCleaned = 0;
    mTools = bm->getTools();
    mBlockchainManager = bm;
    mSwarmMechanicsEnabled = initSwarmMechanics;
    mBlockchainMode = bm->getMode();
    mNetworkManager = bm->getNetworkManager();
    mStatus = eManagerStatus::eManagerStatus::initial;
    std::string ip = "0.0.0.0";// getNetworkManager()->getPublicIP()
    mRootDir = ".\\..\\..\\WebUI";
    mDebugLevel = "1";
    mEnableHexdump = "no";
    mSsPattern = "#.shtml";
    mListeningAddress = "https://" + ip + ":8001";

}
/*
std::shared_ptr<ix::WebSocketServer> CWebSocketsServer::getWebSockServer()
{
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    return mServer;
}*/
std::vector<std::shared_ptr<CConversation>> CWebSocketsServer::getConversationsByIP(std::vector<uint8_t> IP)
{
    std::shared_ptr<CTools> tools = getTools();
    std::vector<std::shared_ptr<CConversation>> toRet;
    if (IP.size() == 0)
        return toRet;

    std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

    for (uint64_t i = 0; i < mConversations.size(); i++)
    {
        if (tools->compareByteVectors(mConversations[i]->getEndpoint()->getAddress(), IP))
            toRet.push_back(mConversations[i]);
    }

    return toRet;
}

std::vector<std::shared_ptr<CConversation>> CWebSocketsServer::getAllConversations()
{
    std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);
    return mConversations;
}
bool CWebSocketsServer::registerConversation(std::shared_ptr<CConversation> session)
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

    if (mConversations.size() < getMaxSessionsCount())
    {
        mConversations.push_back(session);
        tools->writeLine("Allowed for a new WebSock connection from "
            + tools->bytesToString(session->getEndpoint()->getAddress()) + ". ConversationID:" + tools->base58CheckEncode(session->getID()));//+ std::string(clienthost) + ":" + std::string(clientservice));
        return true;
    }
    else
        return false;
}
//Warning: we do not provide a function to retrieve an explicit SINGLE web-socket connection for a SINGLE ip since there might be multiple conversations per IP.
//Use getConversationsByIP() or getConversationByID() instead.
std::shared_ptr<CConversation> CWebSocketsServer::getConversationByID(std::vector<uint8_t> id)
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

    for (uint64_t i = 0; i < mConversations.size(); i++)
    {
        if (tools->compareByteVectors(mConversations[i]->getID(), id))
            return mConversations[i];
    }
    return nullptr;
}



std::shared_ptr<CWebRTCSwarm> CWebSocketsServer::createSwarmWithID(std::vector<uint8_t> id)
{
    std::shared_ptr<CTools> tools = getTools();
    if (id.size() == 0)
        return nullptr;

    std::lock_guard<std::recursive_mutex> lock(mSwarmsGuardian);
    if (mSwarms.size() >= mMaxActiveSwarmsCounts)
        return nullptr;

    //the node will exchange/route data for this Swarm through inter-full-node Kademlia communication sub-system
    std::shared_ptr<CWebRTCSwarm> swarm = getSwarmByID(id);

    if (swarm == nullptr)
    {
        swarm = std::make_shared<CWebRTCSwarm>(getNetworkManager(), id, getNetworkManager()->getRouter());
        //the swarm gets automatically registered within the routing table upon construction
        mSwarms.push_back(swarm);
        tools->logEvent(tools->getColoredString("registering Swarm ", eColor::lightCyan) +"'"+ tools->base58CheckEncode(id)+ "'", "Swarms", eLogEntryCategory::network, 1);
        return swarm;
    }

    return nullptr;
}
std::shared_ptr<CWebRTCSwarm> CWebSocketsServer::getRandomSwarm()
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::recursive_mutex> lock(mSwarmsGuardian);
    if (mSwarms.size() == 0)
        return nullptr;
    if (mSwarms.size() == 1)
        return mSwarms[0];

    return mSwarms[tools->genRandomNumber(0, mSwarms.size() - 1)];
}

void CWebSocketsServer::lockSwarms(bool doIt=true)
{
    if (doIt)
        mSwarmsGuardian.lock();
    else
        mSwarmsGuardian.unlock();
}
std::shared_ptr<CWebRTCSwarm> CWebSocketsServer::getSwarmByID(std::vector<uint8_t> id)
{
    std::shared_ptr<CTools> tools = getTools();
    if (id.size() == 0)
        return nullptr;

    std::lock_guard<std::recursive_mutex> lock(mSwarmsGuardian);

    std::shared_ptr<CWebRTCSwarm> toRet;

    for (uint64_t i = 0; i < mSwarms.size(); i++)
    {
        if (tools->compareByteVectors(mSwarms[i]->getID(), id))
        {
            return mSwarms[i];
        }
    }

    return nullptr;
}

bool CWebSocketsServer::getSwarmMechanicsEnabled()
{
    std::lock_guard<std::recursive_mutex> lock(mSwarmsGuardian);
    return mSwarmMechanicsEnabled;
}

void CWebSocketsServer::setSwarmMechanicsEnabled(bool setIt)
{
    std::lock_guard<std::recursive_mutex> lock(mSwarmsGuardian);
    mSwarmMechanicsEnabled = setIt;
}




std::shared_ptr<CTools> CWebSocketsServer::getTools()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mTools;
}
std::shared_ptr<CNetworkManager> CWebSocketsServer::getNetworkManager()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mNetworkManager.lock();
}

uint64_t CWebSocketsServer::getLastAliveListing()
{
    std::lock_guard<std::mutex> lock(mLastAliveListingGuardian);
    return mLastAliveListingTimestamp;
}

void CWebSocketsServer::lastAlivePrinted()
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::mutex> lock(mLastAliveListingGuardian);
    mLastAliveListingTimestamp = tools->getTime();
}

/// <summary>
/// Does cleaning up of WebSocket Conversations. 
/// Returns the number of freed communication slots.
/// </summary>
/// <returns></returns>
uint64_t CWebSocketsServer::cleanConversations()
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

    //Local Variables - BEGIN
    size_t time = tools->getTime();
    uint64_t removedCount = 0;

    std::string report = std::string(tools->getColoredString("\n-- Registered WebSock Conversations -- \n", eColor::blue));
    //Local Variables - END
    uint64_t maxInvalidMsgs = CGlobalSecSettings::getMaxInvalidWebMsgsCount();

    report += mConversations.size() > 0 ? "\n" : tools->getColoredString("none", eColor::cyberWine);
    bool genReport = (tools->getTime() - getLastAliveListing()) > 15 ? true : false;

    for (std::vector <std::shared_ptr<CConversation>>::iterator it = mConversations.begin(); it != mConversations.end(); ) {
        if (genReport)
            report += tools->base58CheckEncode((*it)->getID()) + ":" + tools->conversationStateToString((*it)->getState()->getCurrentState()) + "\n";
        time = tools->getTime();
        if ((*it)->getState()->getLastActivity() > time)
        {//it's in the future
            ++it;
            continue;
        }

        if ((*it)->isSlowDataAttack()) {
            tools->logEvent("[WebSocket Slowloris] Ending conversation due to slow data attack",
                eLogEntryCategory::network, 5, eLogEntryType::warning);

            // Ban the IP
            std::string ip = (*it)->getIPAddress();
            getNetworkManager()->banIP(ip, 3600); // 1 hour ban

            (*it)->end();
        }

        if ((*it)->getState()->getCurrentState() == eConversationState::ended)
        {
            it = mConversations.erase(it);
            continue;
        }
        if ((*it)->getState()->getCurrentState() != eConversationState::ended && (time - (*it)->getState()->getLastActivity()) > CGlobalSecSettings::getOverridedConvShutdownAfter())
        {
            //do *NOT* delete conversation just yet. let all the callbacks to execute during the grace period (30sec)
            tools->logEvent("[FORCED shutdown] WebSock (ID:" + tools->base58CheckEncode((*it)->getID()) + ") due to inactivity.", eLogEntryCategory::network, 1);
            (*it)->end(false);
        }

        if ((*it)->getState()->getCurrentState() == eConversationState::ended && ((time - (*it)->getState()->getLastActivity()) > CGlobalSecSettings::getFreeConversationAfter()))
        {
            tools->logEvent("[Freeing] WebSock (ID:" + tools->base58CheckEncode((*it)->getID()) + ").", eLogEntryCategory::network, 0);
            it = mConversations.erase(it);
            removedCount++;
        }
        else if (maxInvalidMsgs > 1 && ((*it)->getInvalidMsgsCount() > maxInvalidMsgs))
        {
         
            (*it)->end(false);
        }
 
        else {//let it be
            ++it;
        }
    }

    //Report - BEGIN
    if (genReport)
    {
        report += tools->getColoredString("\n-------\n", eColor::blue);
        tools->logEvent(report, eLogEntryCategory::network, 1);
        lastAlivePrinted();
    }
    //Report - END

    return removedCount;
}

uint64_t CWebSocketsServer::cleanSwarms()
{//[todo]: Discuss on 07.02.21
    std::lock_guard<std::recursive_mutex> lock(mSwarmsGuardian);
    //Local Variables - BEGIN
    uint64_t time = 0;
    uint64_t removedCount = 0;
    uint64_t rtEntriesRemoved = 0;
    std::shared_ptr<CTools> tools = getTools();
    //Local Variables - END

    //Operational Logic - BEGIN

    for (std::vector <std::shared_ptr<CWebRTCSwarm>>::iterator it = mSwarms.begin(); it != mSwarms.end(); )
    {
        time = tools->getTime();//keep the timestamp fresh enough shall we

        if (time < (*it)->getLastActivityTimestamp())
        {
            ++it;
            continue;//it might happen so due to long processing within the above; the local timestamp is absolete
        }
        if (((time - (*it)->getLastActivityTimestamp())) > mMaxSwarmInactivityThreshold)
        {

            (*it)->close();
            rtEntriesRemoved = getNetworkManager()->getRouter()->removeEntriesWithNextHop((*it)->getAbstractEndpoint());//remove routing table entries which had this conversation set on path as an immediate hop.
            tools->logEvent("[WebRTC Swarm Closed] (ID:" + tools->base58CheckEncode((*it)->getID()) + "). Reason: due to inactivity. As a result removed " + std::to_string(rtEntriesRemoved)
                + " routing table entries.", eLogEntryCategory::network, 1);
        }
        if ((time - (*it)->getLastActivityTimestamp()) > (mMaxSwarmInactivityThreshold + 10))//give a 10 a sec grace period until memory is freed
        {
            tools->logEvent("[Freeing] UDT Conversation (ID:" + tools->base58CheckEncode((*it)->getID()) + ").", eLogEntryCategory::network, 0);
            it = mSwarms.erase(it);
            removedCount++;
        }
        else {//let it be
            ++it;
        }

    }

    //Operational Logic - END

    return removedCount;
}

/// <summary>
/// Facilitates Swarm-support mechanics.
/// Routes local-full node SDP-messages across local/immediate peers.
/// Routes SDP-messages across full-node boundaries i.e. across full-node network through Kademlia.
/// Takes care of incentivization and authentication of peers.
/// Facilitates WebRTC SDP-signaling functionality.
/// </summary>
void CWebSocketsServer::makeSwarmsTick()
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::recursive_mutex> lock(mSwarmsGuardian);

    //Local Variables - BEGIN
    uint64_t time = 0;
    //Local Variables - END

    for (std::vector <std::shared_ptr<CWebRTCSwarm>>::iterator it = mSwarms.begin(); it != mSwarms.end(); )
    {
        tools->getTime();//keep the timestamp fresh enough shall we
        (*it)->routeSDPEntities();
        (*it)->doMaintenance();
        ++it;

    }
    // mLastSwarmsTick = tools->getTime();//[todo] add mutex protection? disabled=>performance=>needless

}

uint64_t CWebSocketsServer::doCleaningUp(bool forceKillAllSessions)
{
    std::lock_guard<std::recursive_mutex> lock(mConversationsGuardian);

    //Local Variables - BEGIN
    uint64_t totalObjectsCleaned = 0; std::shared_ptr<CTools> tools = getTools();
    //Local Variables - END

    //Operational Logic - BEGIN
    if (forceKillAllSessions)
    {
        //WebSocket conversations - BEGIN
        for (int i = 0; i < mConversations.size(); i++)
        {
            mConversations[i]->end();
        }
        //WebSocket conversations - END

        //WebRTCSwarms  - BEGIN
        for (int i = 0; i < mSwarms.size(); i++)
        {
            mSwarms[i]->close();
        }
        //WebRTCSwarms  - END

        setLastTimeCleanedUp(tools->getTime());

        return mConversations.size();
    }
    else
    {
        totalObjectsCleaned = cleanConversations();//WebSockets
        totalObjectsCleaned += cleanSwarms();//WebRTC
        setLastTimeCleanedUp(tools->getTime());
    }

    //Operational Logic - END

    return totalObjectsCleaned;
}

bool CWebSocketsServer::addSwarm(std::shared_ptr<CWebRTCSwarm> swarm)
{
    if (swarm == nullptr)
        return false;

    std::lock_guard<std::recursive_mutex> lock(mSwarmsGuardian);
    mSwarms.push_back(swarm);
    return true;
}

void CWebSocketsServer::WebSocketServerThreadF()
{
    //Local Variables - BEGIN
    uint64_t cleanedCount = 0;
    uint64_t alreadyConnected = 0;
    std::string ip;
    std::shared_ptr<CTools> tools = getTools();
    tools->SetThreadName("WebSock-Conversations Server");
    std::stringstream reportBuilder;
    uint64_t now = 0;
    uint64_t lastReportTimestamp = 0;
    uint64_t lastCleanUp = 0;
    const uint64_t cleanUpIntervalSec = 5;
    //Local Variables - END
   
    //*[IMPORTANT]*:  WWWServer takes care of web-sockets.

    //Operational Logic - BEGIN
    
    // Run the server in the background. Server can be stopped by calling server.stop()
    while (getStatus() == eManagerStatus::eManagerStatus::running)
    {
        now = tools->getTime();
        cleanedCount = 0;
        //Let the CPU breathe - BEGIN
        Sleep(5);
        //Let the CPU breathe - END

        lastCleanUp = getLastTimeCleanedUp();

        if ((now - lastCleanUp) > cleanUpIntervalSec)
        {
            cleanedCount = doCleaningUp();
        }

        makeSwarmsTick();

        if (cleanedCount > 0)
            tools->writeLine("Freed " + std::to_string(cleanedCount) + " WebSock Conversations");

        alreadyConnected = 0;

        //Reporting - BEGIN

        //Swarms - BEGIN
        if ((now - lastReportTimestamp) > 20)
        { //done by CWebSocketServer
            mSwarmsGuardian.lock();
            uint64_t localMembers = 0;
            uint64_t remoteMembers = 0;
            std::shared_ptr<CConversation> conv;
            reportBuilder << tools->getColoredString(" -- Registered Swarms: " + tools->getColoredString(std::to_string(mSwarms.size()), eColor::lightCyan) + " --\r\n", eColor::blue);
            std::vector<std::shared_ptr<CWebRTCSwarmMember>> members;
            for (uint64_t i = 0; i < mSwarms.size(); i++)
            {
                //reset state - BEGIN
                localMembers = 0;
                remoteMembers = 0;
                //reset state - END

                members = mSwarms[i]->getMembers();

                for (uint64_t i = 0; i < members.size(); i++)
                {
                    conv = members[i]->getConversation();

                    if (conv && conv->getEndpoint()->getType() == eEndpointType::WebSockConversation)
                    {
                        localMembers++;
                    }
                    else
                    {
                        remoteMembers++;
                    }
                }

                reportBuilder << tools->getColoredString(std::to_string(i+1) + ") ", eColor::greyWhiteBox) << "[Local Peers]: " + tools->getColoredString(std::to_string(localMembers), eColor::lightCyan)
                    << " [Remote Peers]: " + tools->getColoredString(std::to_string(remoteMembers), eColor::blue) <<"\r\n";
            }

            reportBuilder << tools->getColoredString(" -----------------------\r\n", eColor::blue);
            mSwarmsGuardian.unlock();
            tools->logEvent(reportBuilder.str(), eLogEntryCategory::network, 1);
            reportBuilder.str("");
            lastReportTimestamp = std::time(0);
        }

        //Swarms - END
        //Reporting - END
         //Operational Logic - END
        
        //allow only one UDT conversation per IP address  - END

    }
}

bool CWebSocketsServer::sendNetMsgRAW(std::shared_ptr<CWebSocket> socket, eNetEntType::eNetEntType eNetType, eNetReqType::eNetReqType eNetReqType, std::vector<uint8_t> RAWbytes)
{
    return sendNetMsg(socket, eNetType, eNetReqType, eDFSCmdType::unknown, std::vector<uint8_t>(), RAWbytes);
}

bool CWebSocketsServer::sendNetMsg(std::shared_ptr<CWebSocket> socket, eNetEntType::eNetEntType eNetType, eNetReqType::eNetReqType eNetReqType, eDFSCmdType::eDFSCmdType dfsCMDType,
    std::vector<uint8_t> dfsData1, std::vector<uint8_t> NetMSGRAWbytes)
{
    //Local variables - BEGIN
    std::shared_ptr<CDFSMsg>  dfsMsg;
    std::shared_ptr<CNetTask> task;
    std::shared_ptr<CNetMsg> netMsg = std::make_shared<CNetMsg>(eNetType, eNetReqType);
    std::vector<uint8_t> bytes;
    //Local variables - END

    //Operational Login - BEGIN
    if (dfsCMDType != eDFSCmdType::unknown)
    {
        dfsMsg = std::make_shared<CDFSMsg>(dfsCMDType);
        dfsMsg->setData1(dfsData1);
        netMsg->setData(dfsMsg->getPackedData());
    }
    else
    {
        netMsg->setData(NetMSGRAWbytes);
    }
    bytes = netMsg->getPackedData();

   // mg_ws_send(socket, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
    socket->sendBytes(bytes);
    return true;
    //Operational Login - END
}

bool CWebSocketsServer::initSwarmMechanics()
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::recursive_mutex> lock(mGuardian);
    if (mSwarms.size() == 0)
    {
        //create the initial Swarm
        //the swarm gets automatically registered within routing table upon construction
        std::shared_ptr<CWebRTCSwarm> swarm = std::make_shared<CWebRTCSwarm>(getNetworkManager(), CCryptoFactory::getInstance()->getSHA2_256Vec(tools->stringToBytes("main")), getNetworkManager()->getRouter());
        addSwarm(swarm);
    }

    return true;
}
void log_via_log_file(const void* buf, size_t len, void* userdata)
{
    CTools::getInstance()->writeToFile("websockLog.txt", std::string(reinterpret_cast<const char*>(buf), len));
}
std::shared_ptr<ThreadPool> CWebSocketsServer::getThreadPool()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mThreadPool;
}



bool CWebSocketsServer::initializeServer()
{
    std::shared_ptr<CTools> tools = getTools();
    std::lock_guard<std::recursive_mutex> lock(mGuardian);

    if (!tools->prepareCertificate(getNetworkManager()))
        return false;

    if (mSwarmMechanicsEnabled)
    {
        initSwarmMechanics();
    }

    //Rationale behind the "Trampoline below".
    //The Mongoose's API requires the event handler to be in the form defined by its typedef mg_event_handler_t. - which is a pointer
    // to a regular function. I.e. not a member function that is. We are object-oriented and for Good Design, employ objects, together with member functions.
    // We want events to be handled within member functions. Single instance of GRIDNET Core may support multiple networks (Live, Test-Net etc.)
    // As such, there may be multiple web-servers active, from he very same executable at various ports.
    //The trouble is, - one cannot convert a member-function-pointer to a regular pointer. For that, one needs a "trampoline".
    //The "trampoline" is a static (as required) stub, a wrapper around the local member function pointer.
    //This allows for usage of local member functions as event handler to the Mongoose's API.

    std::string connectionId;
    //start the controller thread which in turn would spawn the server thread.
    mControllerThread = std::thread(&CWebSocketsServer::mControllerThreadF, this);
    return true;
}
