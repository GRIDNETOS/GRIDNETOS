// TransactionSearchIterator.h

#ifndef TRANSACTIONSEARCHITERATOR_H
#define TRANSACTIONSEARCHITERATOR_H

#include "SearchIterator.h"
#include "SearchFilter.hpp"
#include "SearchCursors.h"
#include <memory>
#include <deque>
#include <vector>
#include <string>

class CBlockchainManager;
class CTransactionManager;
class CTools;
class CTransaction;
class CBlock;
class CTXInfo;

/**
 * @brief Iterator for searching transactions incrementally in both mempool and blockchain.
 *
 * This class provides functionality to iterate over transactions in mempool,
 * transaction cache, and blockchain, applying search filters and returning matching transactions.
 */
class TransactionSearchIterator : public SearchIterator {
public:
    /**
     * @brief Construct a new Transaction Search Iterator.
     *
     * @param query The search query string.
     * @param filter A shared pointer to the search filter.
     * @param blockchainManager A shared pointer to the blockchain manager.
     * @param cursor An optional cursor to resume search from a specific position.
     */
    TransactionSearchIterator(
        const std::string& query,
        const std::shared_ptr<CSearchFilter>& filter,
        const std::shared_ptr<CBlockchainManager>& blockchainManager,
        const std::optional<TransactionCursor>& cursor = std::nullopt
    );

    /**
     * @brief Enable or disable cache-only mode.
     *
     * @param enable Set to true to enable cache-only mode, false to disable.
     */
    void setCacheOnlyMode(bool enable);

    /**
     * @brief Check if cache-only mode is enabled.
     *
     * @return true If cache-only mode is enabled.
     * @return false If cache-only mode is disabled.
     */
    bool isCacheOnlyMode() const;

    /**
     * @brief Get the next matching transaction in the search.
     *
     * @return std::optional<CSearchResults::ResultData> The next matching transaction, or nullopt if no more matches.
     */
    std::optional<CSearchResults::ResultData> next() override;

    /**
     * @brief Check if there are more transactions to search.
     *
     * @return true If there are more transactions to search.
     * @return false If the search has reached the end.
     */
    bool hasNext() const override;

    /**
     * @brief Retrieve the current cursor position.
     *
     * @return TransactionCursor The current position in the search.
     */
    TransactionCursor getCursor() const;

    /**
     * @brief Skip a number of matching transactions in the search.
     *
     * @param count The number of matching transactions to skip.
     */
    void skip(uint64_t count) override;
    void setIncludeMemPool(bool dotIt = true);
    bool getIncludeMemPool();
private:
    void initialize();
    bool matchesTransactionFilter(CSearchResults::ResultData& resultData);

    std::string mQuery;
    std::shared_ptr<CSearchFilter> mFilter;
    std::shared_ptr<CBlockchainManager> mBlockchainManager;
    std::shared_ptr<CTransactionManager> mTransactionManager;
    std::shared_ptr<CTools> mTools;

    // Cache-only mode flag
    bool mCacheOnlyMode;

    // Mempool state
    std::vector<std::shared_ptr<CTransaction>> mMemPoolTransactions;
    size_t mMemPoolIndex;
    bool mIncludeMemPool;
    // Cache state
    uint64_t mTotalCachedTransactions;
    uint64_t mCacheIndex;
    size_t mCacheChunkIndex;
    std::vector<std::shared_ptr<CTXInfo>> mCacheChunk;
    const size_t mCacheChunkSize = 100; // Adjust chunk size as needed

    // On-chain state
    uint64_t mCurrentBlockDepth;
    uint64_t mMaxBlockDepth;
    std::deque<std::shared_ptr<CTransaction>> mCurrentBlockTransactions;
    std::shared_ptr<CBlock> mCurrentBlock;
    uint64_t mCurrentBlockTimestamp;
    size_t mTransactionIndex;
};

#endif // TRANSACTIONSEARCHITERATOR_H
