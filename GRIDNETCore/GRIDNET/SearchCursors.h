#pragma once
// SearchCursors.h

#ifndef SEARCHCURSORS_H
#define SEARCHCURSORS_H

#include <cstdint>
#include <cstddef>

/**
 * @brief Cursor for BlockSearchIterator
 *
 * Represents the current position in a block search.
 */
struct BlockCursor {
    uint64_t currentDepth; ///< The current depth in the blockchain being searched

    /**
     * @brief Construct a new Block Cursor
     *
     * @param depth The initial depth in the blockchain (default is 0, representing the latest block)
     */
    BlockCursor(uint64_t depth = 0)
        : currentDepth(depth) {}
};

/**
 * @brief Cursor structure to maintain the state of TransactionSearchIterator.
 *
 * This structure holds the necessary indices to resume iteration in both
 * cache-only mode and normal mode (blockchain traversal). It does not include
 * transient state variables that could become stale between sessions.
 */
struct TransactionCursor {
    // Common state
    size_t memPoolIndex;      // Index in the mempool transactions vector

    // Cache-only mode state
    uint64_t cacheIndex;      // Global index in the cache (total transactions processed)
    size_t cacheChunkIndex;   // Index within the current cache chunk
    bool cacheOnlyMode;       // Indicates if the iterator is in cache-only mode

    // Blockchain traversal mode state
    uint64_t blockDepth;      // Current block depth being processed
    size_t transactionIndex;  // Index within the current block's transactions

    /**
     * @brief Default constructor initializing all indices to zero and cacheOnlyMode to false.
     */
    TransactionCursor()
        : memPoolIndex(0),
        cacheIndex(0),
        cacheChunkIndex(0),
        cacheOnlyMode(false),
        blockDepth(0),
        transactionIndex(0)
    {}
};


/**
 * @brief Cursor for DomainSearchIterator
 *
 * Represents the current position in a domain search.
 */
struct DomainCursor {
    size_t currentIndex; ///< The current index in the list of domain IDs being searched

    /**
     * @brief Construct a new Domain Cursor
     *
     * @param index The initial index in the domain ID list (default is 0)
     */
    DomainCursor(size_t index = 0)
        : currentIndex(index) {}
};

#endif // SEARCHCURSORS_H