#pragma once
#include <vector>
#include "enums.h"
#include <stdafx.h>
#include <vector>
#include <memory>
#include <mutex>

/// <summary>
/// Wrapper around SIP-data, provides aditional Swarm-descriptors.
/// </summary>

class CTransmissionToken;
class CConversation;
class CSDPEntity
{
private:
	static std::mutex sRecentSourceSeqGuardian;
	static uint64_t sRecentSourceSeq;
	std::recursive_mutex mGuardian;
	eSDPEntityType::eSDPEntityType mType;
	eSDPControlStatus::eSDPControlStatus mStatus;
	size_t mTimeCreated;
	//NOT SERIALIZED - BEGIN
	bool mPending;//not serialized
	uint64_t mHopCount;//upper CNetMsg layer is concerned with hop-count; here we copy over from it.
	//NOT SERIALIZED - END
	void initFields();
	uint64_t getNewSourceSeq();
	std::vector<uint8_t> mSwarmID;//used to target swarm-members through a WebSocket-protocol
	std::vector<uint8_t> mSourceID;
	std::vector<uint8_t> mDestinationID;
	std::vector<uint8_t> mSDPData;//RAW HTML5-compatible SIP (RTCSessionDescription) data-bundle
	std::vector<uint8_t> mSDPSessionID;//timestamp 
	std::vector<uint8_t> mSig, mExtraData;
	size_t mSeqNr;//so we can distinguish datagrams with same data and detect redundant/past ones.
	size_t mVersion;
	std::shared_ptr<CConversation> mConversation;//not serialized - conversation which DELIVERED the entity.
	eConnCapabilities::eConnCapabilities mCapabilities;
	std::shared_ptr<CTransmissionToken> mTT;
	bool mMarkedForDeletion;
public:
	void setHopCount(uint64_t value);
	uint64_t getHopCount();
	eConnCapabilities::eConnCapabilities getCapabilities();
	void markForDeletion(bool doIt);
	bool getIsMarkedForDeletion();
	void setCapabilities(eConnCapabilities::eConnCapabilities capabilities);
	std::shared_ptr<CTransmissionToken> getTT();
	void setTT(std::shared_ptr<CTransmissionToken> tt);
	void setStatus(eSDPControlStatus::eSDPControlStatus status);
	eSDPControlStatus::eSDPControlStatus getStatus();
	friend bool operator== (const CSDPEntity& sdp1, const CSDPEntity& sdp2);
	friend bool operator!= (const CSDPEntity& sdp1, const CSDPEntity& sdp2);
	bool validateSignature(std::vector<uint8_t> pubKey);
	bool sign(Botan::secure_vector<uint8_t> privKey);
	std::vector<uint8_t> getSourceID();
	std::vector<uint8_t> getDestinationID();
	void setSeqNr(uint64_t seqNr);
	uint64_t getSeqNr();
	CSDPEntity(std::shared_ptr<CConversation> conversation=nullptr, eSDPEntityType::eSDPEntityType type = eSDPEntityType::eSDPEntityType::joining, std::vector<uint8_t>swarmID = std::vector<uint8_t>(), std::vector<uint8_t> source= std::vector<uint8_t>(), std::vector<uint8_t> destination= std::vector<uint8_t>(), std::vector<uint8_t> SIPData = std::vector<uint8_t>());
	std::shared_ptr<CConversation> getConversation();
	void setConversation(std::shared_ptr<CConversation> conversation);
	eSDPEntityType::eSDPEntityType getType();
	std::vector<uint8_t> getPackedData(bool includeSig=true);
	static std::shared_ptr<CSDPEntity> instantiate(std::vector<uint8_t> packedData);
	bool getIsPending();
	void setSDPSessionID(std::vector<uint8_t> id);
	uint64_t getSDPSessionIDAsUInt();
	std::vector<uint8_t> getSDPSessionID();
	size_t getTimeCreated();
	void setIsPending(bool isIt = true);

	std::vector<uint8_t> getSwarmID();

};