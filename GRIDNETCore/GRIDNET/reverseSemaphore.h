#pragma once
#include <mutex>
#include <condition_variable>

class ReverseSemaphore {
private:
    unsigned long counter_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;

public:
    ReverseSemaphore() : counter_(0) {}
    //std::lock_guard_wrapper - BEGIN
    void lock() {

       // waitFree();// since we mostly use a lock guard inside of internals of a CTrieDB object and while waiting for a permission to modify its contents.
                   //in such a case we need to wait for the internal counter to be 0.
                   // 
                   
        acquire(); //However normally we would use acquire() instead.
    }

    void unlock() {
        release();
    }
    //std::lock_guard_wrapper - END

    unsigned long acquire() {
        std::unique_lock<std::mutex> lock(mtx_);
        ++counter_;
        cv_.notify_all();
        return counter_;
    }

    unsigned long release() {
        std::unique_lock<std::mutex> lock(mtx_);
        if (counter_ > 0) --counter_;
        cv_.notify_all();
        return counter_;
    }

    /// <summary>
    /// Waits for a resource to be free.
    /// IMPORTANT: it locks the resource afterawards. The resource is left in a LOCKED state  - after the function exits.
    /// It needs to be thus RELEASED through release() after client is done using the resource.
    /// </summary>
    /// <returns></returns>
    unsigned long waitFree() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return counter_ == 0; });
        ++counter_;
        return counter_;
    }

    bool tryWaitFree() {
        std::unique_lock<std::mutex> lock(mtx_, std::try_to_lock);
        if(!lock.owns_lock() || counter_ != 0) {
            return false;
        }
        ++counter_;
        return true;
    }

    unsigned long get_counter() const {
        std::unique_lock<std::mutex> lock(mtx_);
        return counter_;
    }
};

