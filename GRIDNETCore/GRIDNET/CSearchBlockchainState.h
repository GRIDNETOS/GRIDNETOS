#pragma once
// CSearchBlockchainState.h
#include "CKeyWordState.h"
#include <optional>

class BlockCursor;
class TransactionCursor;
class DomainCursor;

class CSearchBlockchainState : public CKeyWordState {
public:
    CSearchBlockchainState(const std::vector<uint8_t>& keywordStateID)
        : mKeywordStateID(keywordStateID) {}

    // Explicitly declare virtual destructor
    virtual ~CSearchBlockchainState() = default;

    const std::vector<uint8_t>& getKeywordStateID() const override {
        return mKeywordStateID;
    }

    // State for iterators
    std::optional<BlockCursor> blockCursor;
    std::optional<TransactionCursor> transactionCursor;
    std::optional<DomainCursor> domainCursor;

private:
    std::vector<uint8_t> mKeywordStateID;
};