#include "BlockSearchIterator.h"
#include "BlockDesc.hpp"
#include "SearchCursors.h"
#include "BlockchainManager.h"
#include "Tools.h"

BlockSearchIterator::BlockSearchIterator(
    const std::string& query,
    const std::shared_ptr<CSearchFilter>& filter,
    const std::shared_ptr<CBlockchainManager>& blockchainManager,
    const std::optional<BlockCursor>& cursor
)
    : mQuery(query), mFilter(filter), mBlockchainManager(blockchainManager)
{
    // Initialization - BEGIN
    mTools = CTools::getInstance();
    initialize();

    if (cursor)
    {
        mCurrentDepth = cursor->currentDepth;
    }
    else
    {
        mCurrentDepth = 0; // Start from the latest block
    }
    // Initialization - END
}

void BlockSearchIterator::skip(uint64_t count)
{
    // Operational Logic - BEGIN
    uint64_t skipped = 0;

    while (mCurrentDepth <= mMaxDepth && skipped < count)
    {
        auto block = mBlockchainManager->getBlockAtDepth(mCurrentDepth, false, eChainProof::verifiedCached);
        mCurrentDepth++;

        if (!block)
        {
            continue;
        }

        // Block Meta Data - BEGIN
        auto blockDesc = block->getDescription();// attempt to re-use existing block meta-data

        if (!blockDesc) {  // only if not available generate block meta-data
            std::string errorMessage;
            blockDesc = mBlockchainManager->createBlockDescription(block, true, true, errorMessage);

            if (!blockDesc) {
                mTools->logEvent(
                    "Failed to create block description for block: " +
                    mTools->base58CheckEncode(block->getID()) +
                    " [Reason]: " + (errorMessage.empty() ? "Unknown error" : errorMessage),
                    "GetBlockDetails",
                    eLogEntryCategory::localSystem,
                    5,
                    eLogEntryType::failure
                );
            }
            else {
                block->setDescription(blockDesc);
            }
        }
        // Block Meta Data - END

        if (matchesBlockFilter(blockDesc))
        {
            skipped++;
        }
    }
    // Operational Logic - END
}

void BlockSearchIterator::initialize()
{
    // Initialization - BEGIN
    mMaxDepth = mBlockchainManager->getHeight();
    // Initialization - END
}

bool BlockSearchIterator::hasNext() const
{
    return mCurrentDepth <= mMaxDepth;
}

std::optional<CSearchResults::ResultData> BlockSearchIterator::next()
{
    // Fetching Results - BEGIN
    while (mCurrentDepth <= mMaxDepth)
    {
        auto block = mBlockchainManager->getBlockAtDepth(mCurrentDepth, false, eChainProof::verifiedCached);
        mCurrentDepth++;

        if (block)
        {
            // Block Meta Data - BEGIN
            auto blockDesc = block->getDescription();  // attempt to re-use existing block meta-data
            if (!blockDesc)  // only if not available generate block meta-data
            {
                std::string errorMessage;
                blockDesc = mBlockchainManager->createBlockDescription(block, true, true, errorMessage);

                if (!blockDesc) {
                    mTools->logEvent(
                        "Failed to create block description for block: " +
                        mTools->base58CheckEncode(block->getID()) +
                        " [Reason]: " + (errorMessage.empty() ? "Unknown error" : errorMessage),
                        "GetBlockDetails",
                        eLogEntryCategory::localSystem,
                        5,
                        eLogEntryType::failure
                    );
                }
                else {
                    block->setDescription(blockDesc);
                }
            }
            // Block Meta Data - END

            if (matchesBlockFilter(blockDesc))
            {
                return blockDesc;
            }
        }
    }
    // Fetching Results - END

    // No more results
    return std::nullopt;
}

bool BlockSearchIterator::matchesBlockFilter(const std::shared_ptr<CBlockDesc>& desc)
{
    return mBlockchainManager->matchesBlockFilter(desc, mQuery, *mFilter);
}

BlockCursor BlockSearchIterator::getCursor() const
{
    return BlockCursor{ mCurrentDepth };
}