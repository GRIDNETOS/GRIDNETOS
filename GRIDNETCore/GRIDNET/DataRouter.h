#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include "enums.h"
#include <stdafx.h>

//forward declarations - BEGIN
class CRTEntry;
class CNetMsg;
class CConversation;
class CNetworkManager;
class CUDTConversationsServer;
class CNetTask;
class CQUICConversationsServer;
class CTransmissionToken;
class CEndPoint;
//forward declarations - END

class CDataRouter
{
private:
	std::mutex mGuardian;
	std::mutex mSettingsGuardian;
	std::mutex mRTGuardian;
	std::mutex mCollectedTTsGuardian;
	std::vector<std::shared_ptr<CRTEntry>> mEntries;
	size_t mLastCleanUp;
	uint64_t mCleanUpEverySec;
	std::shared_ptr<CTools> mTools;
	std::mutex mToolsGuardian;
	std::shared_ptr<CTools> getTools();
	bool addRTEntry(std::shared_ptr<CRTEntry> entry);
	bool removeRTEntry(uint64_t id);
	std::weak_ptr<CNetworkManager> mNetworkManager;
	void initFields();
	uint64_t mMinPropagationReward;
	uint64_t mFreePropagationsCount;
	bool mLogNetworkDebugEvents;
	bool mSettingsLoaded;
	std::vector<uint8_t> mLocalID;//used to check whether a targeted TT is spendable by local peer
	void loadSettings(bool forceRenewal = false);
	bool getLogNetworkDebugEvents();
	std::vector<std::shared_ptr<CTransmissionToken>> mCollectedTTs;//collected  Transmission Tokens recognized as spendable.
	//These are off-the-chain rewards collected by local router, the be cashed out on-the chain when accomulated.

	uint64_t getMinPropagationReward();
	void addCollectedTT(std::shared_ptr<CTransmissionToken> tt);
	std::vector<std::shared_ptr<CTransmissionToken>> getCollectedTTs();

	eTTCollectionResult::eTTCollectionResult collectTT(std::shared_ptr<CNetMsg> msg, const uint64_t& reportedValue = 0,bool doChecks=false);
	std::vector<uint8_t> getLocalID();
public: 

	uint64_t removeEntriesWithNextHop(std::shared_ptr<CEndPoint> nextHop);
	CDataRouter(std::weak_ptr<CNetworkManager> networkManager, uint64_t cleanupEverySec=15);
	bool updateRT(std::shared_ptr<CNetMsg> msg, std::shared_ptr<CEndPoint> immediateSource);
	bool updateRT(std::shared_ptr<CEndPoint> destination, std::shared_ptr<CEndPoint> immediatePeer, uint64_t hopCount = 0, eRouteKowledgeSource::eRouteKowledgeSource knowledgeSource = eRouteKowledgeSource::propagation);
	uint64_t cashOutRewards();
	std::string getDescription(bool includeRT = true, bool extendedRT = false, bool checkAvailability = false, bool includeKadPeers = true, bool showKnowledgeSource = false, bool showPeerIDs=false, std::string newLine = "\n", eColumnFieldAlignment::eColumnFieldAlignment alignment = eColumnFieldAlignment::center, uint64_t maxWidth =120);
	bool route(std::shared_ptr<CNetMsg> msg, std::shared_ptr<CEndPoint> sourceEntity, std::shared_ptr<CConversation> sourceConversation=nullptr);
	bool deliverToWebSocketEndpoint(std::shared_ptr<CNetMsg>& msg, std::vector<uint8_t>& receiptID, std::shared_ptr<CConversation>& sourceConversation, std::shared_ptr<CNetTask>& task, std::shared_ptr<CNetworkManager>& nm, bool logDebugEvents);
	bool isRTEntryReachable(std::shared_ptr<CRTEntry> entry);
	bool deliverToUDTConversation(bool logDebugEvents, std::shared_ptr<CNetMsg>& msg, std::shared_ptr<CConversation>& conversation, std::shared_ptr<CNetworkManager>& nm, std::vector<uint8_t>& destinationID, std::shared_ptr<CNetTask>& task, std::shared_ptr<CDTI>& dti,bool isStStage =true);
	bool deliverToQUICConversation(bool logDebugEvents, std::shared_ptr<CNetMsg>& msg, std::shared_ptr<CConversation>& conversation, std::shared_ptr<CNetworkManager>& nm, std::vector<uint8_t>& destinationID, std::shared_ptr<CNetTask>& task, std::shared_ptr<CDTI>& dti, bool isStStage = true);
	bool deliverToTerminalEndpoint(std::shared_ptr<CConversation>& conversation, std::shared_ptr<CDTI>& dti, std::shared_ptr<CNetworkManager>& nm, std::vector<uint8_t>& destAddr, std::shared_ptr<CNetMsg>& msg);
	uint64_t deliverToIPEndpoint(std::shared_ptr<CUDTConversationsServer>& udtServer, std::shared_ptr<CConversation>& conversation, std::shared_ptr<CNetworkManager>& nm, std::vector<uint8_t>& destAddr, std::vector<std::shared_ptr<CConversation>>& conversations, std::shared_ptr<CNetTask>& task, std::shared_ptr<CNetMsg>& msg, std::shared_ptr<CDTI>& dti, const uint64_t& spreadFactor);
	uint64_t getSpreadFactor(std::shared_ptr<CNetMsg> msg);
	uint64_t deliverToIPEndpoint(std::shared_ptr<CQUICConversationsServer>& quicServer,
		std::shared_ptr<CConversation>& conversation,
		std::shared_ptr<CNetworkManager>& nm,
		std::vector<uint8_t>& destAddr,
		std::vector<std::shared_ptr<CConversation>>& conversations,
		std::shared_ptr<CNetTask>& task,
		std::shared_ptr<CNetMsg>& msg,
		std::shared_ptr<CDTI>& dti,
		const uint64_t& spreadFactor);

	std::vector<std::shared_ptr<CRTEntry>> findEntriesForDest(std::vector<uint8_t> id, eEndpointType::eEndpointType eType);
	std::shared_ptr<CRTEntry> findEntryForDest(std::vector<uint8_t> id, eEndpointType::eEndpointType eType, bool pingEntry=true);
	void cleanTable();
};