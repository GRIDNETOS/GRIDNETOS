// CBERCacheManager.h

#ifndef CBER_CACHE_MANAGER_H
#define CBER_CACHE_MANAGER_H

#include "robin_hood.h"
#include <vector>
class CBERCacheManager
{
public:
    // Constructor remains the same for backwards compatibility

    CBERCacheManager(bool optimizedMode = true, uint64_t cacheTimeout = 180);

    // Accessors remain the same
    bool getIsOptimizedMode();
    uint64_t getCacheLifeTime();

    // Public methods remain the same
    bool checkAndRetrieveFromCache(const std::vector<std::vector<uint8_t>>& byteVectors, std::vector<uint8_t>& output);
    bool checkAndRetrieveFromCache(const std::vector<uint8_t>& hashKey, std::vector<uint8_t>& output);

    bool insertIntoCache(const std::vector<uint8_t>& hashKey, const std::vector<uint8_t>& data);

    void maintainCacheExt();
    std::vector<uint8_t> computeCacheKey(const std::vector<std::vector<uint8_t>>& byteVectors);
protected:
    // Internal logic remains similar but now also deals with size limitation
    void maintainCache();
  

private:
    // Private members
    bool mOptimizedMode;
    uint64_t mCacheLifetime;

    // The cache:
    //   key:   SHA-256 hash (32 bytes)
    //   value: pair of (cached data, last accessed time)
    std::unordered_map<
        std::vector<uint8_t>,
        std::pair<std::vector<uint8_t>, std::time_t>
    > mBERCache;

    // Protects mBERCache and mCurrentCacheSize
    std::mutex mBERCacheMutex;

    // Protects mOptimizedMode and mCacheLifetime
    std::mutex mFieldsGuardian;

    // Keep track of current cache size and the max allowed
    uint64_t mCurrentCacheSize = 0;
    uint64_t mMaxCacheSize = 100ULL * 1024ULL * 1024ULL; // 100 MB by default

    // Helper function to enforce max size
    void enforceMaxSize();
};

#endif // CBER_CACHE_MANAGER_H