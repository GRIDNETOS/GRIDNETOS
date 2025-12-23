#pragma once
// SearchIterator.h

#ifndef SEARCHITERATOR_H
#define SEARCHITERATOR_H

#include "SearchResults.hpp"
#include <optional>
#include <cstdint>

/**
 * @brief Interface for search iterators used to fetch results incrementally.
 *
 * This abstract class defines the interface for all search iterators in the system.
 * It provides methods for retrieving results one at a time, checking for more results,
 * and skipping a number of results. Specific implementations (e.g., BlockSearchIterator,
 * TransactionSearchIterator, DomainSearchIterator) should inherit from this class.
 */
class SearchIterator {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~SearchIterator() = default;

    /**
     * @brief Retrieves the next result in the search sequence.
     *
     * @return std::optional<CSearchResults::ResultData> An optional containing the next result,
     *         or std::nullopt if no more results are available.
     */
    virtual std::optional<CSearchResults::ResultData> next() = 0;

    /**
     * @brief Checks if there are more results to fetch.
     *
     * @return bool true if there are more results available, false otherwise.
     */
    virtual bool hasNext() const = 0;

    /**
     * @brief Skips the specified number of matching results.
     *
     * This method advances the iterator by the specified number of matching results
     * without returning them. If there are fewer matching results than the count,
     * the iterator will advance to the end.
     *
     * @param count The number of matching results to skip.
     */
    virtual void skip(uint64_t count) = 0;
};

#endif // SEARCHITERATOR_H