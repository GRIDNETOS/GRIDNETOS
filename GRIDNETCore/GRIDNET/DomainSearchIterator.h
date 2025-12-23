#pragma once
// DomainSearchIterator.h

#ifndef DOMAINSEARCHITERATOR_H
#define DOMAINSEARCHITERATOR_H

#include "SearchIterator.h"
#include "SearchFilter.hpp"
#include "SearchCursors.h"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <optional>

class CStateDomainManager;
class CTools;
class CDomainDesc;

/**
 * @brief Iterator for searching domains incrementally.
 *
 * This class provides functionality to iterate over domains,
 * applying search filters and returning matching domains.
 */
class DomainSearchIterator : public SearchIterator {
public:
    /**
   * @brief Construct a new Domain Search Iterator.
   *
   * @param query The search query string.
   * @param filter A shared pointer to the search filter.
   * @param domainManager A shared pointer to the state domain manager.
   * @param cursor An optional cursor to resume search from a specific position.
   * @param allowAdhocMetadataGeneration If true, allows generation of metadata when not found in cache.
   */
    DomainSearchIterator(
        const std::string& query,
        const std::shared_ptr<CSearchFilter>& filter,
        const std::shared_ptr<CStateDomainManager>& domainManager,
        const std::optional<DomainCursor>& cursor = std::nullopt,
        bool allowAdhocMetadataGeneration = false
    );

    /**
     * @brief Get the next matching domain in the search.
     *
     * @return std::optional<CSearchResults::ResultData> The next matching domain, or nullopt if no more matches.
     */
    std::optional<CSearchResults::ResultData> next() override;

    /**
     * @brief Check if there are more domains to search.
     *
     * @return true If there are more domains to search.
     * @return false If the search has reached the end of the domain list.
     */
    bool hasNext() const override;

    /**
     * @brief Skip a number of matching domains in the search.
     *
     * @param count The number of matching domains to skip.
     */
    void skip(uint64_t count) override;

    /**
     * @brief Retrieve the current cursor position.
     *
     * @return DomainCursor The current position in the search.
     */
    DomainCursor getCursor() const;

private:
    void initialize();
    bool matchesDomainFilter(const std::shared_ptr<CDomainDesc>& desc);
    bool mAllowAdhocMetadataGeneration;
    std::string mQuery;
    std::shared_ptr<CSearchFilter> mFilter;
    std::shared_ptr<CStateDomainManager> mDomainManager;
    std::shared_ptr<CTools> mTools;

    std::vector<std::vector<uint8_t>> mDomainIDs;
    size_t mCurrentIndex;
};

#endif // DOMAINSEARCHITERATOR_H