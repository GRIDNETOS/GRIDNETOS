#pragma once
#include "enums.h"
#include <mutex>
class CChatMsg
{
public:
	CChatMsg(eChatMsgType::eChatMsgType type = eChatMsgType::text, std::vector<uint8_t> from = std::vector<uint8_t>(),
		std::vector<uint8_t> to = std::vector<uint8_t>(), std::vector<uint8_t> data = std::vector<uint8_t>(), uint64_t timestamp = 0, bool external = false);
    eChatMsgType::eChatMsgType getType();
    std::vector<uint8_t> getSourceID();
    std::vector<uint8_t> getDestinationID();
    uint64_t getTimestamp();
    std::string getRendering(bool web = false);
    void initialize();
    uint64_t getVersion();
    std::vector<uint8_t> getSig();
    void sign(Botan::secure_vector<uint8_t> privKey);
    bool verifySignature(std::vector<uint8_t> pubKey);
    friend bool operator== (const CChatMsg& c1, const CChatMsg& c2);
    friend bool operator!= (const CChatMsg& c1, const CChatMsg& c2);
    std::vector<uint8_t>  getPackedData(bool includeSig = true);
    std::vector<uint8_t> getData();
    static std::shared_ptr<CChatMsg> instantiate(std::vector<uint8_t> packedData);
private:
    void initFields();
    std::mutex mFieldsGuardian;
    std::vector<uint8_t> mFromID;
    std::vector<uint8_t> mToID;
    uint64_t mVersion;
    std::vector<uint8_t> mData;
    uint64_t mTimestamp;
    bool mExternal; //NOT serialized. optimization only
    std::vector<uint8_t> mSig;
    bool mInitialized;
    eChatMsgType::eChatMsgType  mType;

};