#pragma once

#include "enums.h"
#include <vector>
#include <mutex>
#include "netmsg.h"

struct qrFlags
{
	bool encrypted : 1;//whether mData is encrypted to mPubKey
	bool session : 1;//whether session is to be established and consecutive msgs's bodies encrypted with plain Chacha20 secret /nonce (based on sourceSeq in each
	//consecutive NetMsg) and no AEAD. 
	bool reserved1 : 1;
	bool reserved2 : 1;
	bool reserved3 : 1;
	bool reserved4 : 1;
	bool reserved5 : 1;
	bool reserved6 : 1;
};

//We might have as well used CNetMsg. Still it might be better to leave a specialized container.
class CQRIntent
{
private:
	std::vector<uint8_t> mChallange;//to avoid reply-attacks. data here is signed. signature MAY be delivered within mSig field of CNetMsg IF 'signed' field not set. (if
	   //set  then the mSig value of CNetMsg would be attempted to be treated as signature of the CNetMsg container itself.
	qrFlags mFlags;
	nmFlags mNFlags;
	std::vector<uint8_t> mDestinationID; //destination ID
	eEndpointType::eEndpointType mDestinationType;
	eOperationScope::eOperationScope mOperationScope;
	std::vector<uint8_t> mRouteThrough; //IP address of a full-node / first proposed hop for a response
	eQRIntentType::eQRIntentType mType; //indicates intent-type i.e. its purpose
	
	std::vector<uint8_t> mData; // information/data contained within an intent
	/*
	* In case of the LogMeIn intent, the mobile-token app provides ECC signature of mData within mChallange field of CSessionDescription.
	* The LogMeIn QRIntent contains pseudo-random data within the mData field.
	*/
	std::recursive_mutex mGuardian;
	std::vector<uint8_t> mInfo;
	uint64_t mVersion;
	uint64_t mSourceSeq; //source sequence number, used for generation of a (CNetMsg)response and/or (CNetMsg-Operation Status)
	//the same number is then used to update the status of any asynchronous tasks

	std::vector<uint8_t> mPubKey;// if pubkey != 0 , mData can be decrypted only by holder of the coresponding private key
	//NOTE: full AEAD/ECIES needed here, with consistancy validation.
	void initFields();
public:
	std::vector<uint8_t> getChallange();
	void  setChallange(std::vector<uint8_t> data);
	qrFlags getFlags();
	void setFlags(qrFlags flags);
	void setNetworkFlags(nmFlags flags);
	nmFlags getNetworkFlags();
	
	void setSourceSeq(uint64_t seq);
	void setOperationScope(eOperationScope::eOperationScope scope);
	eOperationScope::eOperationScope getOperationScope();
	uint64_t getSourceSeq();
	uint64_t getVersion();
	//initialization
	CQRIntent(eQRIntentType::eQRIntentType QRType);
	CQRIntent(eQRIntentType::eQRIntentType  QRType, std::vector<uint8_t> destinationID, std::vector<uint8_t> routeThrough, eEndpointType::eEndpointType = eEndpointType::WebSockConversation);
	//intent type
	eQRIntentType::eQRIntentType getType();
	void setType(eQRIntentType::eQRIntentType QRType);
	//pub-key
	void setPubKey(std::vector<uint8_t> pubKey);
	std::vector<uint8_t> getPubKey();
	//destination type
	void setDestinationType(eEndpointType::eEndpointType eType);
	eEndpointType::eEndpointType getDestinationType();

	void setRouteThrough(std::vector<uint8_t> ip);
	std::vector<uint8_t> getRouteThrough();
	//data
	bool setData(std::vector<uint8_t> data);
	std::vector<uint8_t> getData();

	//sender info
	bool setInfo(std::vector<uint8_t> info);
	std::vector<uint8_t> getInfo();

	//terminal info
	void setDestinationID(std::vector<uint8_t> terminalID);
	std::vector<uint8_t> getDestinationID();

	//serialization / deserialization functionality
	std::vector<uint8_t> getPackedData();
	std::vector<std::vector<bool>> getQRMatrix(uint64_t marginLength=0, bool doBottomMargin=true, bool doBottomFrame=true,uint64_t bottomMarginWidth=1);
	std::string getASCIIQRCode(uint64_t& width,std::string newLineCode="\n", const uint64_t& height=0, bool addSafeMargin=true, uint64_t terminalWidth=0);
};