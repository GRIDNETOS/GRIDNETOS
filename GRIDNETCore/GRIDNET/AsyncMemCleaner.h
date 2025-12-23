#pragma once
#include "stdafx.h"
//#include <mutex>
//#include <memory>
//#include <functional>
//#include <atomic>
//#include <vector>

// Forward declarations
class ThreadPool;
class ThreadPool;
class CTrieDB;
/**
 * @brief Provides asynchronous memory clean-up facilities using a thread pool.
 *
 * This class manages asynchronous deletion of memory regions using a thread pool.
 * It provides thread-safe operations for queuing memory cleanup tasks that are processed
 * in the background by worker threads.
 */
class CAsyncMemCleaner
{
private:
    /** @brief Mutex for thread synchronization of general operations */
    static std::mutex mGuardian;

    /** @brief Number of worker threads in the thread pool */
    static constexpr size_t THREAD_COUNT = 3;

    /** @brief Shared pointer to the thread pool instance */
    static std::shared_ptr<ThreadPool> mThreadPool;

    /** @brief Flag indicating whether the cleaner is shutting down */
    static std::atomic<bool> mShutdown;

public:
    /**
     * @brief Asynchronously cleans up a single memory region.
     */
    static void cleanIt(void** region);

    /**
     * @brief Processes the asynchronous cleanup of multiple memory regions.
     */
    static void cleanMemRegionsAsync(std::vector<void*>* dbs);

    /**
     * @brief Initializes the thread pool.
     */
    static void initialize();

    /**
     * @brief Asynchronously cleans up a vector of objects.
     *
     * @tparam From Type of the input vector or container
     * @param from Vector or container of objects to be cleaned up
     */
    template<typename From>
    static void cleanItVec(From&& from);

    /**
     * @brief Shuts down the thread pool and cleans up resources.
     */
    static void shutdown();
};

// Template declarations for common types
extern template void CAsyncMemCleaner::cleanItVec<std::vector<void*>>(std::vector<void*>&& from);
extern template void CAsyncMemCleaner::cleanItVec<const std::vector<void*>&>(const std::vector<void*>&);