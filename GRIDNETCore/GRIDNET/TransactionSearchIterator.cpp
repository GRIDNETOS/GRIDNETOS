#include "TransactionSearchIterator.h"
#include "TransactionDesc.hpp"
#include "Receipt.h"
#include "Block.h"
#include "SearchCursors.h"
#include "BlockchainManager.h"

TransactionSearchIterator::TransactionSearchIterator(
    const std::string& query,
    const std::shared_ptr<CSearchFilter>& filter,
    const std::shared_ptr<CBlockchainManager>& blockchainManager,
    const std::optional<TransactionCursor>& cursor
)
    : mQuery(query),
    mFilter(filter),
    mBlockchainManager(blockchainManager),
    mMemPoolIndex(0),
    mCacheIndex(0),
    mCacheChunkIndex(0),
    mCacheOnlyMode(false),
    mIncludeMemPool(true)
{

    mTools = CTools::getInstance();
    mTransactionManager = mBlockchainManager->getLiveTransactionsManager();

    if (cursor)
    {
        mCacheOnlyMode = cursor->cacheOnlyMode;

        if (mCacheOnlyMode)
        {
            // Initialize cache state from cursor
            mMemPoolIndex = cursor->memPoolIndex;
            mCacheIndex = cursor->cacheIndex;
            mCacheChunkIndex = cursor->cacheChunkIndex;
        }
        else
        {
            // Initialize blockchain traversal state from cursor
            mMemPoolIndex = cursor->memPoolIndex;
            mCurrentBlockDepth = cursor->blockDepth;
            mTransactionIndex = cursor->transactionIndex;
        }
    }

    initialize();
}

void TransactionSearchIterator::setCacheOnlyMode(bool enable)
{
    if (mCacheOnlyMode != enable)
    {
        mCacheOnlyMode = enable;
        initialize(); // Re-initialize state when mode changes
    }
}

bool TransactionSearchIterator::isCacheOnlyMode() const
{
    return mCacheOnlyMode;
}

void TransactionSearchIterator::initialize()
{
    // Mempool Initialization
    mMemPoolTransactions = mTransactionManager->getUnprocessedTransactions(eTransactionSortingAlgorithm::recentFirst);

    if (mMemPoolIndex >= mMemPoolTransactions.size())
    {
        mMemPoolIndex = mMemPoolTransactions.size();
    }

    if (mCacheOnlyMode)
    {
        // Cache Initialization
        mTotalCachedTransactions = mBlockchainManager->getTotalRecentTransactionsCount();
        if (mCacheIndex > mTotalCachedTransactions)
        {
            mCacheIndex = mTotalCachedTransactions;
        }
        mCacheChunkIndex = 0;
        mCacheChunk.clear();
    }
    else
    {
        // On-chain Initialization
        mMaxBlockDepth = mBlockchainManager->getHeight();
        if (mCurrentBlockDepth > mMaxBlockDepth)
        {
            mCurrentBlockDepth = mMaxBlockDepth;
        }
        mCurrentBlockTransactions.clear();
    }
}

bool TransactionSearchIterator::hasNext() const
{
    if (mMemPoolIndex < mMemPoolTransactions.size())
        return true;

    if (mCacheOnlyMode)
    {
        if (mCacheChunkIndex < mCacheChunk.size() || mCacheIndex < mTotalCachedTransactions)
            return true;
    }
    else
    {
        if (!mCurrentBlockTransactions.empty())
            return true;

        if (mCurrentBlockDepth <= mMaxBlockDepth)
            return true;
    }

    return false;
}

std::optional<CSearchResults::ResultData> TransactionSearchIterator::next()
{

    // Operational Logic - BEGIN
    while (hasNext())
    {
        // Process Mem-Pool Transactions - BEGIN
        // mem-pool transactions are agnostic to 'normal' and 'cache-only modes

        if (mIncludeMemPool)
        {

            if (mMemPoolIndex < mMemPoolTransactions.size())
            {
                while (mMemPoolIndex < mMemPoolTransactions.size())
                {
                    auto tx = mMemPoolTransactions[mMemPoolIndex++];
                    auto txDesc = mBlockchainManager->createTransactionDescription(tx, nullptr, 0, false);
                    CSearchResults::ResultData resultData = txDesc;

                    if (matchesTransactionFilter(resultData))
                    {
                        return resultData;
                    }
                }
            }
        }

        // Process Mem-Pool Transactions - END

        // Pre-Processed TX Cache Mode - BEGIN
        if (mCacheOnlyMode)
        {
            // Process Cached Transactions
            while (true)
            {
                if (mCacheChunkIndex >= mCacheChunk.size())
                {
                    // Load next chunk
                    if (mCacheIndex >= mTotalCachedTransactions)
                    {
                        // No more transactions in the cache
                        break;
                    }

                    uint64_t remaining = mTotalCachedTransactions - mCacheIndex;
                    uint64_t count = std::min(mCacheChunkSize, remaining);

                    mCacheChunk = mBlockchainManager->getRecentTransactionsInfo(mCacheIndex, count);
                    mCacheChunkIndex = 0;
                    mCacheIndex += count;

                    if (mCacheChunk.empty())
                    {
                        // No more transactions in the cache
                        break;
                    }
                }

                // Process transactions in the current chunk - BEGIN
                while (mCacheChunkIndex < mCacheChunk.size())
                { 
                    auto txInfo = mCacheChunk[mCacheChunkIndex++];
                    if (!txInfo)
                        continue;

                    auto txDesc = txInfo->getDescription();
                    CSearchResults::ResultData resultData = txDesc;

                    if (matchesTransactionFilter(resultData))
                    {
                        return resultData;
                    }
                }
                // Process transactions in the current chunk - END
            }
        }
        // Pre-Processed TX Cache Mode - END
        else
        {
            // Process On-chain Transactions - BEGIN

            // TX Retrieval - BEGIN
            if (mCurrentBlockTransactions.empty())
            {
                while (mCurrentBlockDepth <= mMaxBlockDepth)
                {
                    mCurrentBlock = mBlockchainManager->getBlockAtDepth(mCurrentBlockDepth, false, eChainProof::verifiedCached);
                    mCurrentBlockDepth++;

                    if (mCurrentBlock)
                    {
                        const auto& transactions = mCurrentBlock->getTransactions();
                        // Start from mTransactionIndex if resuming
                        for (size_t i = mTransactionIndex; i < transactions.size(); ++i)
                        {
                            mCurrentBlockTransactions.push_back(std::make_shared<CTransaction>(transactions[i]));
                        }
                        mCurrentBlockTimestamp = mCurrentBlock->getHeader()->getSolvedAtTime();
                        mTransactionIndex = 0; // Reset for next block
                        break;
                    }
                }
            }
            // TX Retrieval - END

            // TX Processing - BEGIN
            while (!mCurrentBlockTransactions.empty())
            {
                auto tx = mCurrentBlockTransactions.front();
                mCurrentBlockTransactions.pop_front();

                CReceipt rec;
                if (mCurrentBlock->getHeader()->getReceiptForTransaction(tx->getHash(), rec, false))
                {
                    auto receipt = std::make_shared<CReceipt>(rec);
                    auto txDesc = mBlockchainManager->createTransactionDescription(tx, receipt, mCurrentBlockTimestamp, false);
                    CSearchResults::ResultData resultData = txDesc;

                    if (matchesTransactionFilter(resultData))
                    {
                        return resultData;
                    }
                }
            }
            // TX Processing - END
        }
        // Process On-chain Transactions - END

        // If we reach here, no more transactions to process
        break;
    }

    return std::nullopt;
    // Operational Logic - END
}

void TransactionSearchIterator::skip(uint64_t count)
{
    uint64_t skipped = 0;

    // Skip over Mempool Transactions
    while (mMemPoolIndex < mMemPoolTransactions.size() && skipped < count)
    {
        auto tx = mMemPoolTransactions[mMemPoolIndex++];
        auto txDesc = mBlockchainManager->createTransactionDescription(tx, nullptr, 0, false);
        CSearchResults::ResultData resultData = txDesc;

        if (matchesTransactionFilter(resultData))
        {
            skipped++;
        }
    }

    if (mCacheOnlyMode)
    {
        // Skip over Cached Transactions
        while (skipped < count)
        {
            if (mCacheChunkIndex >= mCacheChunk.size())
            {
                // Load next chunk
                if (mCacheIndex >= mTotalCachedTransactions)
                {
                    // No more transactions in the cache
                    break;
                }

                uint64_t remaining = mTotalCachedTransactions - mCacheIndex;
                uint64_t chunkCount = std::min(mCacheChunkSize, remaining);

                mCacheChunk = mBlockchainManager->getRecentTransactionsInfo(mCacheIndex, chunkCount);
                mCacheChunkIndex = 0;
                mCacheIndex += chunkCount;

                if (mCacheChunk.empty())
                {
                    // No more transactions in the cache
                    break;
                }
            }

            while (mCacheChunkIndex < mCacheChunk.size() && skipped < count)
            {
                auto txInfo = mCacheChunk[mCacheChunkIndex++];
                if (!txInfo)
                    continue;

                auto txDesc = txInfo->getDescription();
                CSearchResults::ResultData resultData = txDesc;

                if (matchesTransactionFilter(resultData))
                {
                    skipped++;
                }
            }
        }
    }
    else
    {
        // Skip over On-chain Transactions
        while (skipped < count)
        {
            if (mCurrentBlockTransactions.empty())
            {
                while (mCurrentBlockDepth <= mMaxBlockDepth)
                {
                    mCurrentBlock = mBlockchainManager->getBlockAtDepth(mCurrentBlockDepth, false, eChainProof::verifiedCached);
                    mCurrentBlockDepth++;

                    if (!mCurrentBlock)
                    {
                        continue;
                    }

                    const auto& transactions = mCurrentBlock->getTransactions();
                    // Start from mTransactionIndex if resuming
                    for (size_t i = mTransactionIndex; i < transactions.size(); ++i)
                    {
                        mCurrentBlockTransactions.push_back(std::make_shared<CTransaction>(transactions[i]));
                    }
                    mCurrentBlockTimestamp = mCurrentBlock->getHeader()->getSolvedAtTime();
                    mTransactionIndex = 0; // Reset for next block
                    break;
                }
            }

            while (!mCurrentBlockTransactions.empty() && skipped < count)
            {
                auto tx = mCurrentBlockTransactions.front();
                mCurrentBlockTransactions.pop_front();

                CReceipt rec;
                if (mCurrentBlock->getHeader()->getReceiptForTransaction(tx->getHash(), rec, false))
                {
                    auto receipt = std::make_shared<CReceipt>(rec);
                    auto txDesc = mBlockchainManager->createTransactionDescription(tx, receipt, mCurrentBlockTimestamp, false);
                    CSearchResults::ResultData resultData = txDesc;

                    if (matchesTransactionFilter(resultData))
                    {
                        skipped++;
                    }
                }
            }

            if (mCurrentBlockTransactions.empty() && mCurrentBlockDepth > mMaxBlockDepth)
            {
                // No more blocks to process
                break;
            }
        }
    }
}

void TransactionSearchIterator::setIncludeMemPool(bool dotIt)
{
     mIncludeMemPool = dotIt;
}

bool TransactionSearchIterator::getIncludeMemPool()
{
    return mIncludeMemPool;
}

TransactionCursor TransactionSearchIterator::getCursor() const
{
    TransactionCursor cursor;

    cursor.memPoolIndex = mMemPoolIndex;

    if (mCacheOnlyMode)
    {
        cursor.cacheIndex = mCacheIndex;
        cursor.cacheChunkIndex = mCacheChunkIndex;
        cursor.cacheOnlyMode = true;
    }
    else
    {
        cursor.blockDepth = mCurrentBlockDepth;
        cursor.transactionIndex = mTransactionIndex;
        cursor.cacheOnlyMode = false;
    }

    return cursor;
}

bool TransactionSearchIterator::matchesTransactionFilter(CSearchResults::ResultData& resultData)
{
    return mBlockchainManager->matchesTransactionFilter(resultData, mQuery, *mFilter);
}
