// BlockSearchIterator.h

#ifndef BLOCKSEARCHITERATOR_H
#define BLOCKSEARCHITERATOR_H

#include "SearchIterator.h"
#include "SearchFilter.hpp"
#include "SearchCursors.h"
#include <memory>
#include <optional>
#include <string>
#include <cstdint>

class CBlockchainManager;
class CTools;
class CBlockDesc;

/**
 * @brief Iterator for searching blocks incrementally in the blockchain.
 *
 * This class provides functionality to iterate over blocks in the blockchain,
 * applying search filters and returning matching blocks.
 */
class BlockSearchIterator : public SearchIterator {
public:
    /**
     * @brief Construct a new Block Search Iterator.
     *
     * @param query The search query string.
     * @param filter A shared pointer to the search filter.
     * @param blockchainManager A shared pointer to the blockchain manager.
     * @param cursor An optional cursor to resume search from a specific position.
     */
    BlockSearchIterator(
        const std::string& query,
        const std::shared_ptr<CSearchFilter>& filter,
        const std::shared_ptr<CBlockchainManager>& blockchainManager,
        const std::optional<BlockCursor>& cursor = std::nullopt
    );

    /**
     * @brief Get the next matching block in the search.
     *
     * @return std::optional<CSearchResults::ResultData> The next matching block, or nullopt if no more matches.
     */
    std::optional<CSearchResults::ResultData> next() override;

    /**
     * @brief Skip a number of matching blocks in the search.
     *
     * @param count The number of matching blocks to skip.
     */
    void skip(uint64_t count) override;

    /**
     * @brief Check if there are more blocks to search.
     *
     * @return true If there are more blocks to search.
     * @return false If the search has reached the end of the blockchain.
     */
    bool hasNext() const override;

    /**
     * @brief Retrieve the current cursor position.
     *
     * @return BlockCursor The current position in the search.
     */
    BlockCursor getCursor() const;

private:
    void initialize();
    bool matchesBlockFilter(const std::shared_ptr<CBlockDesc>& desc);

    std::string mQuery;
    std::shared_ptr<CSearchFilter> mFilter;
    std::shared_ptr<CBlockchainManager> mBlockchainManager;
    std::shared_ptr<CTools> mTools;

    uint64_t mCurrentDepth;
    uint64_t mMaxDepth;
};

#endif // BLOCKSEARCHITERATOR_H