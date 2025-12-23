#pragma once

#include "enums.h"
#include <vector>
#include <mutex>

class CQRIntentResponse
{
private:
    std::mutex mGuardian;
    std::vector<uint8_t> mTerminalID;// so that full-node can recognize terminal session
    std::vector<uint8_t> mSignature;
    std::vector<uint8_t> mPubKey;//optional
    std::vector<uint8_t> mData;

public:
    CQRIntentResponse();
    CQRIntentResponse(std::vector<uint8_t> terminalID,std::vector<uint8_t> sig,std::vector<uint8_t> pubKey=std::vector<uint8_t>(), std::vector<uint8_t> data=
        std::vector<uint8_t>());
    std::vector<uint8_t> getSig();
    std::vector<uint8_t> getPubKey();

    std::vector<uint8_t> getDestinationID();
    std::vector<uint8_t> getData();
    void setData(std::vector<uint8_t> data);
    //deserialization functionality
    static std::shared_ptr<CQRIntentResponse> instantiate(std::vector<uint8_t> bytes);
    static std::shared_ptr<CQRIntentResponse> instantiate(std::string &base58Encoded);
    std::vector<uint8_t> getPackedData();
};

