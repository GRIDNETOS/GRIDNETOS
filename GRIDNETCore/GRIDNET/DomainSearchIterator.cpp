#include "DomainSearchIterator.h"
#include "DomainDesc.hpp"
#include "SearchCursors.h"
#include "CStateDomainManager.h"
#include "Tools.h"
DomainSearchIterator::DomainSearchIterator(
    const std::string& query,
    const std::shared_ptr<CSearchFilter>& filter,
    const std::shared_ptr<CStateDomainManager>& domainManager,
    const std::optional<DomainCursor>& cursor,
    bool allowAdhocMetadataGeneration)
    : mQuery(query)
    , mFilter(filter)
    , mDomainManager(domainManager)
    , mAllowAdhocMetadataGeneration(allowAdhocMetadataGeneration)
{
    // Initialization - BEGIN
    mTools = CTools::getInstance();
    initialize();
    assertGN(mDomainManager, "No Domain Manager provided to DomainSearchIterator.");

    if (cursor) {
        mCurrentIndex = cursor->currentIndex;
    }
    else {
        mCurrentIndex = 0;
    }
    // Initialization - END
}


void DomainSearchIterator::initialize()
{
    // Initialization - BEGIN
    
    mDomainIDs = mDomainManager->getKnownDomainIDs(true);
    // mCurrentIndex is set in the constructor
    // Initialization - END
}

bool DomainSearchIterator::hasNext() const
{
    return mCurrentIndex < mDomainIDs.size();
}

void DomainSearchIterator::skip(uint64_t count)
{
    // Local Variables - BEGIN
    uint64_t skipped = 0;
    std::shared_ptr<CDomainDesc> domainDesc = nullptr;
    // Local Variables - END

    // Operational Logic - BEGIN
    while (mCurrentIndex < mDomainIDs.size() && skipped < count)
    {
        const auto& id = mDomainIDs[mCurrentIndex++];

        // Try to get cached metadata first
        domainDesc = mDomainManager->getCachedDomainMetadata(id);

        // If not cached and adhoc generation is allowed, generate it
        if (!domainDesc && mAllowAdhocMetadataGeneration)
        {
            CStateDomain* domain = mDomainManager->findByID(id);
            if (domain)
            {
                domainDesc = mDomainManager->createDomainDescription(domain);
            }
        }

        if (domainDesc && matchesDomainFilter(domainDesc))
        {
            skipped++;
        }
    }
    // Operational Logic - END
}
std::optional<CSearchResults::ResultData> DomainSearchIterator::next()
{
    // Local Variables - BEGIN
    std::shared_ptr<CDomainDesc> domainDesc = nullptr;
    // Local Variables - END

    // Fetching Results - BEGIN
    while (mCurrentIndex < mDomainIDs.size())
    {
        const auto& id = mDomainIDs[mCurrentIndex++];

        // Try to get cached metadata first
        domainDesc = mDomainManager->getCachedDomainMetadata(id);

        // If not cached and adhoc generation is allowed, generate it
        if (!domainDesc && mAllowAdhocMetadataGeneration)
        {
            CStateDomain* domain = mDomainManager->findByID(id, true, false);
            if (!domain)
            {
                mTools->logEvent("Domain not found for ID",
                    "DomainSearchIterator::next",
                    eLogEntryCategory::localSystem,
                    8,
                    eLogEntryType::warning);
                continue;
            }
            domainDesc = mDomainManager->createDomainDescription(domain);
        }

        // Process found domain description
        if (domainDesc && matchesDomainFilter(domainDesc))
        {
            return domainDesc;
        }
        else if (!domainDesc && !mAllowAdhocMetadataGeneration)
        {
            mTools->logEvent("Metadata not available for domain and adhoc generation disabled",
                "DomainSearchIterator::next",
                eLogEntryCategory::localSystem,
                3,
                eLogEntryType::notification);
        }
    }
    // Fetching Results - END

    return std::nullopt;
}
bool DomainSearchIterator::matchesDomainFilter(const std::shared_ptr<CDomainDesc>& desc)
{
    return mDomainManager->matchesDomainFilter(desc, mQuery, *mFilter);
}

DomainCursor DomainSearchIterator::getCursor() const
{
    return DomainCursor{ mCurrentIndex };
}