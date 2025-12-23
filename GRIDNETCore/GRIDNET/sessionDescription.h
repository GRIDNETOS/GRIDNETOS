#pragma once
#include <stdafx.h>
#include <vector>
#include <memory>
#include <mutex>
#include "enums.h"


//1byte in total
struct sdFlags
{
    bool sessionEncrypted : 1;
    bool reserved1 : 1;
    bool reserved2 : 1;
    bool reserved3 : 1;
    bool reserved4 : 1;
    bool reserved5 : 1;
    bool reserved6 : 1;
    bool reserved7 : 1;

    sdFlags()
    {
        std::memset(this, 0, sizeof(sdFlags));
    }
    sdFlags(const sdFlags& sibling) {
        std::memcpy(this, &sibling, sizeof(sdFlags));
    }

};

/*
* 
*    [ IMPORTANT ]  - Dual Nature of CSessionDescription - BEGIN
* 
*                       CSessionDescription is used at two layers:
*                       1)  (OBLIGATORY PHASE) Network Session Layer / Routing Layer - while participating in the initial handshake
*                          
* 
*                          [ How ]: an appropriate flag indicating that payload of CNetMsg contains Session Description - it needs to be set.

*                         { Use Case 1 - Authentication }:  In a strongly authenticated mode (assumed implicitly once authenticated encryption enabled and authentication on conversation set), the public key present within CSessionDescription is used 
*                          to confirm signature of a challenge response. CSessionDescription carries a IV vector comprising a to-be-signed challange - delivered as part of a Hello Request.
* 
                           { Use Case 2 - Peer-ID routing table entry } Only once confirmed would a Core node include an apprriate routing entry of type Peer-ID . (see also two ways such entries can be included:
*                                                                                                                                                                   - per message strong auth
*                                                                                                                                                                   - one-time Session Description auth).
* 
*                       2)  (OPTIONAL PHASE) User Session Layer - { Use Case 2 - application layer data } to notify an instance of a Decentralized Terminal Interface (either DUI or SSH) about the active session details (user detail).  
*                        [ How ]: there's a seperate CNetMsg container type.
* 
* 
*                       [ Notice ]:  Network Session Layer can operate only on direct link connections. Opetaions over the User Session Layer can be onion-routed.
*   
* 
*                   - Dual Nature of CSessionDescription - BEGIN
* 
* 
* 
* 
    CSessionDescription facilitates session description.
    Provides all that is needed to perform basic configuration of a web-client.
    The isResponse flag indicates communication stage (request/response).
    IMPORTANT: EPHEMERAL EC mComPubKey are to be used. ONE-TIME only.

    CSessionDescription is delivered encapsulated (encrypted to an ephemeral punblic key ephPubKey) within CNetMsg in response to a LogMeIn Qr-Intent
    (which contains the ephPubKey).

 

    EC-DH is carried out thanks to it.
*/

class CSessionDescription
{
private:
    std::mutex mGuardian;
    uint64_t mVersion;
    // std::vector<uint8_t> mNonce;//nonce AND symatric key to be used by ChaCha20 during consecutive communication is established based on
    //EC-DH secret, which is based on EPHEMERAL public-keys.
    //Thus, exchange of EPHEMERAL public-keys is all we need.
    std::vector<uint8_t> mAddress;
    std::vector<uint8_t> mPubKey;//coresponds to mAddress, has nothing to do with communication/encryption
    std::vector<uint8_t> mComPubKey;
    std::vector<uint8_t> mConversationID;
    void initFields();
    sdFlags mFlags;
    std::vector<uint8_t> mChallange;//used for both the challange request and response
public:

    std::string getDescription();
    void setChallangeData(std::vector<uint8_t> challangeData);
    std::vector<uint8_t> getChallangeData();
    void setConversationID(std::vector<uint8_t> id);
    std::vector<uint8_t> getConversationID();
    sdFlags getFlags();
    void setFlags(sdFlags flags);
    uint64_t  getVersion();
    CSessionDescription();
    CSessionDescription(const CSessionDescription& sibling);
    std::vector<uint8_t> getAddress();
    std::vector<uint8_t> getPubKey();
    void setPubKey(std::vector<uint8_t> pubKey);

    std::vector<uint8_t> getComPubKey();
    void setComPubKey(std::vector<uint8_t> pubKey);

    void setAddress(std::vector<uint8_t> address);
    
    std::vector<uint8_t> getPackedData();
    static std::shared_ptr<CSessionDescription> instantiate(std::vector<uint8_t> bytes);
};
