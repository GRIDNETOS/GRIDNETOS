// SynchronizedLocker.cpp

#include "SynchronizedLocker.hpp"

namespace sync {

    // Exception implementations
    locker_error::locker_error(const char* message)
        : std::runtime_error(message) {}

    locker_error::locker_error(const std::string& message)
        : std::runtime_error(message) {}

    // SynchronizedLocker implementations
    SynchronizedLocker::SynchronizedLocker(LockingPolicy policy)
        : locked_(false)
        , next_sequence_id_(0)
        , policy_(policy) {}

    SynchronizedLocker::SynchronizedLocker(SynchronizedLocker&& other) noexcept
        : lockables_(std::move(other.lockables_))
        , locked_(other.locked_)
        , next_sequence_id_(other.next_sequence_id_)
        , policy_(other.policy_) {
        other.locked_ = false;
    }

    SynchronizedLocker::~SynchronizedLocker() {
        if (locked_) {
            try {
                unlock();
            }
            catch (...) {
                // Suppress exceptions in destructor
            }
        }
    }

    void SynchronizedLocker::lock() {
        if (locked_) {
            throw locker_error("Already locked");
        }
        if (lockables_.empty()) {
            return;
        }

        order_lockables_();
        size_t total_locked = 0;  // Successfully held locks across all attempts

        // Thread-local backoff mechanism
        static thread_local std::mutex backoff_mutex;
        static thread_local std::condition_variable backoff_cv;
        static thread_local std::mt19937 rng(std::random_device{}());

        // Backoff parameters - tuned for performance
        const std::chrono::milliseconds initial_backoff(1);
        const std::chrono::milliseconds max_backoff(100);  // Increased from 50ms
        std::chrono::milliseconds current_backoff(initial_backoff);

        // CRITICAL WARNING: The backoff mechanism will NOT work correctly if any of the
        // lockables support recursive locking (std::recursive_mutex, ExclusiveWorkerMutex)
        // AND are already locked by the current thread before calling this function.
        // In such cases, try_lock() will succeed (recursive lock), but unlock() during
        // backoff will only decrement the lock count without fully releasing the mutex,
        // preventing other threads from acquiring it and causing deadlock.
        //
        // SOLUTION: Ensure that NO lockables are already held by the calling thread
        // before constructing SynchronizedLocker or calling lock().

        while (true) {
            try {
                // Try to acquire all locks from the beginning
                for (size_t i = 0; i < lockables_.size(); ++i) {
                    if (!lockables_[i]->try_lock()) {
                        // Release ALL locks acquired so far
                        for (size_t j = 0; j < i; ++j) {
                            lockables_[j]->unlock();
                        }

                        // Add randomized jitter to prevent synchronized retry patterns
                        std::uniform_int_distribution<> jitter_dist(0, static_cast<int>(current_backoff.count() / 4));
                        auto jitter = std::chrono::milliseconds(jitter_dist(rng));
                        auto backoff_time = current_backoff + jitter;

                        // Smart backoff: sleep to avoid CPU spinning
                        {
                            std::unique_lock<std::mutex> backoff_lock(backoff_mutex);
                            backoff_cv.wait_for(backoff_lock, backoff_time);
                        }
                        current_backoff = std::min(current_backoff * 2, max_backoff);
                        goto retry;  // Retry from the beginning
                    }
                }

                // All locks acquired successfully
                locked_ = true;
                return;

            retry:
                continue;  // Jump target for retry
            }
            catch (...) {
                // Release all locks on exception
                for (size_t i = 0; i < lockables_.size(); ++i) {
                    try {
                        lockables_[i]->unlock();
                    }
                    catch (...) {
                        // Continue unlocking even if one fails
                    }
                }
                throw locker_error("Lock acquisition failed");
            }
        }
    }

    void SynchronizedLocker::unlock() {
        if (!locked_) {
            throw locker_error("Not locked");
        }

        unlock_n_(lockables_.size());
        locked_ = false;
    }

    void SynchronizedLocker::set_policy(LockingPolicy policy) {
        if (locked_) {
            throw locker_error("Cannot change policy while locked");
        }
        policy_ = policy;
    }

    void SynchronizedLocker::order_lockables_() {
        switch (policy_) {
        case LockingPolicy::MemoryOrdered:
            // Sort by memory address
            std::sort(lockables_.begin(), lockables_.end(),
                [](const std::unique_ptr<detail::lockable_wrapper_base>& a,
                    const std::unique_ptr<detail::lockable_wrapper_base>& b) {
                        return a->get_address() < b->get_address();
                });
            break;

        case LockingPolicy::SequenceOrdered:
            // Sort by sequence ID (order of addition)
            std::sort(lockables_.begin(), lockables_.end(),
                [](const std::unique_ptr<detail::lockable_wrapper_base>& a,
                    const std::unique_ptr<detail::lockable_wrapper_base>& b) {
                        return a->get_sequence_id() < b->get_sequence_id();
                });
            break;

        default:
            assert(false && "Unknown locking policy");
            break;
        }
    }

    void SynchronizedLocker::unlock_n_(std::size_t count) {
        // Unlock in reverse order for proper LIFO semantics
        auto it = lockables_.rbegin();
        auto end = lockables_.rend();

        for (std::size_t i = 0; i < count && it != end; ++i, ++it) {
            try {
                (*it)->unlock();
            }
            catch (...) {
                // Continue unlocking remaining locks even if one fails
            }
        }
    }

} // namespace sync
