#include "CBERCacheManager.h"
#include "DataConcatenator.h"  
#include <utility>
#include <ctime>
#include <mutex>
#include "CryptoFactory.h"  


// Constructor: same signature to maintain backward compatibility
CBERCacheManager::CBERCacheManager(bool optimizedMode, uint64_t cacheTimeout)
    : mOptimizedMode(optimizedMode)
    , mCacheLifetime(cacheTimeout)
    , mCurrentCacheSize(0)
    , mMaxCacheSize(100ULL * 1024ULL * 1024ULL) // default 100MB
{
}

bool CBERCacheManager::getIsOptimizedMode()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mOptimizedMode;
}

uint64_t CBERCacheManager::getCacheLifeTime()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mCacheLifetime;
}

// Computes a cache key for a list of byte vectors.
// Unchanged, except that it calls getIsOptimizedMode() for logic.
std::vector<uint8_t> CBERCacheManager::computeCacheKey(const std::vector<std::vector<uint8_t>>& byteVectors)
{
    if (byteVectors.empty()) {
        return {};
    }

    CDataConcatenator concat;

    if (getIsOptimizedMode()) {
        for (const auto& vec : byteVectors) {
            concat.add(vec);
        }
    }
    else {
        // Original logic: only front + back
        concat.add(byteVectors.front());
        if (byteVectors.size() > 1) {
            concat.add(byteVectors.back());
        }
    }

    return CCryptoFactory::getInstance()->getSHA2_256Vec(concat.getData());
}

// Overload: accepts multiple byte vectors, computes a single hash, then tries to retrieve.
bool CBERCacheManager::checkAndRetrieveFromCache(const std::vector<std::vector<uint8_t>>& byteVectors,
    std::vector<uint8_t>& output)
{
    auto hashKey = computeCacheKey(byteVectors);
    return checkAndRetrieveFromCache(hashKey, output);
}

// Actual retrieval from cache by hash key
bool CBERCacheManager::checkAndRetrieveFromCache(const std::vector<uint8_t>& hashKey,
    std::vector<uint8_t>& output)
{
    // Key must be 32 bytes (SHA-256)
    if (hashKey.size() != 32) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mBERCacheMutex);

    // Clean old entries first
    maintainCache();

    auto it = mBERCache.find(hashKey);
    if (it != mBERCache.end()) {
        // Update last accessed time
        it->second.second = std::time(nullptr);
        // Return the cached data
        output = it->second.first;
        return true;
    }
    return false;
}

// Insert data into the cache
bool CBERCacheManager::insertIntoCache(const std::vector<uint8_t>& hashKey,
    const std::vector<uint8_t>& data)
{
    if (hashKey.size() != 32) {
        return false; // Must be 32 bytes (SHA-256)
    }

    std::lock_guard<std::mutex> lock(mBERCacheMutex);

    // If already exists, replace data and update times
    auto it = mBERCache.find(hashKey);
    if (it == mBERCache.end()) {
        // Insert as a new entry
        mBERCache[hashKey] = { data, std::time(nullptr) };
        // Increase current cache size
        mCurrentCacheSize += data.size();
    }
    else {
        // Adjust the current size: remove old size, add new
        mCurrentCacheSize -= it->second.first.size();
        it->second.first = data;
        it->second.second = std::time(nullptr);
        mCurrentCacheSize += data.size();
    }

    // Enforce the size limit if exceeded
    enforceMaxSize();

    return true;
}

// External method to maintain the cache (thread-safe)
void CBERCacheManager::maintainCacheExt()
{
    std::lock_guard<std::mutex> lock(mBERCacheMutex);
    maintainCache();
}

// Removes entries that exceed their lifetime
void CBERCacheManager::maintainCache()
{
    auto now = std::time(nullptr);
    uint64_t lifeTime = getCacheLifeTime();

    for (auto it = mBERCache.begin(); it != mBERCache.end();) {
        if (static_cast<uint64_t>(now - it->second.second) > lifeTime) {
            // Subtract the size of the erased entry
            mCurrentCacheSize -= it->second.first.size();
            it = mBERCache.erase(it);
        }
        else {
            ++it;
        }
    }

    // After removing expired entries, also ensure we haven't exceeded max size
    enforceMaxSize();
}

// If total cache size exceeds mMaxCacheSize, remove oldest entries
void CBERCacheManager::enforceMaxSize()
{
    // While we exceed the max size, remove the oldest entry
    while (mCurrentCacheSize > mMaxCacheSize && !mBERCache.empty()) {
        // Find the oldest entry by comparing last accessed time
        auto oldestIt = std::min_element(
            mBERCache.begin(), mBERCache.end(),
            [](const auto& lhs, const auto& rhs) {
                return lhs.second.second < rhs.second.second;
            }
        );

        // Remove its size from the total
        mCurrentCacheSize -= oldestIt->second.first.size();
        // Erase from the map
        mBERCache.erase(oldestIt);
    }
}
