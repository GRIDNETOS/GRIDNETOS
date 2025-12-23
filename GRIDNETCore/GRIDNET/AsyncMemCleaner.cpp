#include "AsyncMemCleaner.h"
#include "ThreadPool.h"
#include "TrieDB.h"

std::mutex CAsyncMemCleaner::mGuardian;
std::shared_ptr<ThreadPool> CAsyncMemCleaner::mThreadPool;
std::atomic<bool> CAsyncMemCleaner::mShutdown(false);

namespace {
    struct ThreadPoolInitializer {
        ThreadPoolInitializer() { CAsyncMemCleaner::initialize(); }
        ~ThreadPoolInitializer() { CAsyncMemCleaner::shutdown(); }
    };
    static ThreadPoolInitializer initializer;
}

void CAsyncMemCleaner::initialize() {
    if (!mThreadPool) {
        mThreadPool = std::make_shared<ThreadPool>(THREAD_COUNT, THREAD_COUNT, THREAD_COUNT);
        mShutdown = false;
    }
}

void CAsyncMemCleaner::shutdown() {
    if (mThreadPool) {
        mShutdown = true;
        mThreadPool->waitAll();
        mThreadPool.reset();
    }
}

void CAsyncMemCleaner::cleanIt(void** region) {
    if (region == nullptr || (*region) == nullptr)
        return;

    auto* vec = new std::vector<void*>();
    vec->push_back(*region);
    *region = nullptr;

    if (mThreadPool) {
        mThreadPool->enqueue([vec]() { cleanMemRegionsAsync(vec); });
    }
}

void CAsyncMemCleaner::cleanMemRegionsAsync(std::vector<void*>* regions) {
    if (regions == nullptr)
        return;

    for (int i = 0; i < regions->size(); i++) {
        if ((*regions)[i] == nullptr)
            continue;
        delete static_cast<CTrieDB*>((*regions)[i]);
       // std::this_thread::sleep_for(std::chrono::milliseconds(10)); <- no need to sleep
    }
    delete regions;
}

// Primary template for rvalue references
template <typename From>
void CAsyncMemCleaner::cleanItVec(From&& from) {
    using std::begin; using std::end;
    if constexpr (std::is_rvalue_reference_v<From&&>) {
        auto* dbs = new std::vector<void*>(begin(from), end(from));
        from.clear();
        mThreadPool->enqueue([dbs]() { cleanMemRegionsAsync(dbs); });
    }
    else {
        // For lvalue references and const references
        auto* dbs = new std::vector<void*>(begin(from), end(from));
        mThreadPool->enqueue([dbs]() { cleanMemRegionsAsync(dbs); });
    }
}

template <>
void CAsyncMemCleaner::cleanItVec(std::vector<CTrieDB*>& from) {
    using std::begin; using std::end;
    auto* dbs = new std::vector<void*>(begin(from), end(from));
    from.clear();
    if (mThreadPool && !mShutdown) {
        mThreadPool->enqueue([dbs]() { cleanMemRegionsAsync(dbs); });
    }
    else {
        // If no thread pool or shutdown, cleanup immediately
        for (auto ptr : *dbs) {
            delete static_cast<CTrieDB*>(ptr);
        }
        delete dbs;
    }
}


// Explicit template instantiations for common types
template void CAsyncMemCleaner::cleanItVec<std::vector<void*>>(std::vector<void*>&& from);
template void CAsyncMemCleaner::cleanItVec<const std::vector<void*>&>(const std::vector<void*>&);