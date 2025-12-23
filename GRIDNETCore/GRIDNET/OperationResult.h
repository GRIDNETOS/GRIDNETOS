#pragma once
#include <vector>
#include "enums.h"
#include <mutex>
#include "Receipt.h"

/// <summary>
/// COperationResult provides information about the result of an Operation.
/// That is on lower-level than VMMetaData results and MIGHT be used to indicate result of particular datagram deliveries but can go as high as describing 
/// accomplishment status of Decentralized State Mechine operations as well.
/// It MIGHT encapsulate a CReceipt IF the Operation was supposed to affect the Decentralized State Machine.
/// </summary>
class COperationStatus
{
private:
    std::mutex mGuardian;
    eOperationScope::eOperationScope mScope;
    eOperationStatus::eOperationStatus mStatus;
    uint64_t mVersion;
    uint64_t mReqID;//in response to requestID
    std::vector<uint8_t> mID;//equal to receipt if CReceipt present
    void initFields();
    std::shared_ptr<CReceipt> mReceipt;
    std::string mNotes;

public:
    std::string  getNotes();
    void setNotes(std::string notes);
    uint64_t getReqID();
    std::string getDescription();
    void setReqID(uint64_t reqID);
    std::vector<uint8_t> getID();
    void setID(std::vector<uint8_t> id);
    eOperationStatus::eOperationStatus getStatus();
    eOperationScope::eOperationScope getScope();
    COperationStatus();

    void setReceipt(std::shared_ptr<CReceipt> receipt);
    std::shared_ptr<CReceipt> getTransaction();
    uint64_t getVersion();

    COperationStatus(eOperationStatus::eOperationStatus status, eOperationScope::eOperationScope scope = eOperationScope::eOperationScope::dataTransit);

    std::vector<uint8_t> getPackedData();
    static std::shared_ptr<COperationStatus> instantiate(std::vector<uint8_t> bytes);
};
