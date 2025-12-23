#ifndef THREADPOOL_H
#define THREADPOOL_H
#include "Windows.h"
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <mutex>
#include <functional>
#include <stdexcept>

enum class TaskType { Normal, Stop };
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

using Task = std::tuple<std::function<void()>, TaskType, ThreadPriority>;

#ifdef _WIN32  // Windows platform

inline int MapToOSPriority(ThreadPriority priority) {
    switch (priority) {
    case ThreadPriority::LOWEST:  return THREAD_PRIORITY_LOWEST;
    case ThreadPriority::LOW:     return THREAD_PRIORITY_BELOW_NORMAL;
    case ThreadPriority::NORMAL:  return THREAD_PRIORITY_NORMAL;
    case ThreadPriority::HIGH:    return THREAD_PRIORITY_ABOVE_NORMAL;
    case ThreadPriority::HIGHEST: return THREAD_PRIORITY_HIGHEST;
    default:                      return THREAD_PRIORITY_NORMAL;
    }
}

#else  // POSIX (Linux/UNIX)

inline int MapToOSPriority(ThreadPriority priority) {
    // This is just a sample mapping. Actual values might differ based on specific requirements.
    switch (priority) {
    case ThreadPriority::LOWEST:  return 1;
    case ThreadPriority::LOW:     return 25;
    case ThreadPriority::NORMAL:  return 50;
    case ThreadPriority::HIGH:    return 75;
    case ThreadPriority::HIGHEST: return 99;
    default:                      return 50;
    }
}

#endif


/**
 * @class ThreadPool
 *
 * @brief A dynamic thread pool class which can grow and shrink based on the
 *        workload while maintaining a buffer of threads to minimize the thread
 *        bootstrapping time.
 *
 * The ThreadPool can be initialized with a minimum and maximum number of threads.
 * If the number of active tasks exceeds min_threads, it ensures a buffer of threads
 * are ready. The ThreadPool then further grows as needed up to the maximum allowed
 * threads. When tasks are empty, the pool shrinks back to the base minimum or
 * the buffered amount.
 */
class ThreadPool {
public:


    // Singleton access method
    static std::shared_ptr<ThreadPool> getInstance(size_t minThreads = 10, size_t maxThreads = 300, size_t bufferThreads = 5, ThreadPriority priority = ThreadPriority::NORMAL) {
        static std::mutex instanceMutex;
        std::lock_guard<std::mutex> lock(instanceMutex); // Ensure thread-safe access to the instance
        if (!sInstance) {
            // Use 'new' and wrap it with a shared_ptr using a private struct deleter to access the private constructor
            struct MakeSharedEnabler : public ThreadPool {
                MakeSharedEnabler(size_t minThreads, size_t maxThreads, size_t bufferThreads, ThreadPriority priority)
                    : ThreadPool(minThreads, maxThreads, bufferThreads, priority) {}
            };
            sInstance = std::make_shared<MakeSharedEnabler>(minThreads, maxThreads, bufferThreads, priority);
        }
        return sInstance;
    }
    size_t getActiveThreadCount() ;

    void incActiveThreadCount() ;

    void decActiveThreadCount() ;

    /**
     * @brief Constructor that initializes and starts the thread pool.
     *
     * @param minThreads The minimum number of threads to keep in the pool.
     * @param maxThreads The maximum number of threads that can be active in the pool.
     * @param bufferThreads once the number of active threads exceeds minThreads we ensure addition buffer threads are ready to be used.
     */
    ThreadPool(size_t minThreads=10, size_t maxThreads=300, size_t bufferThreads=5, ThreadPriority priority= ThreadPriority::NORMAL);


    ThreadPool(ThreadPool&& other) noexcept;

    ThreadPool& operator=(ThreadPool&& other) noexcept;
       

        template<class F, class... Args>
    auto enqueue(F&& f, ThreadPriority priority = ThreadPriority::NORMAL, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
        
    /*template<class F, class... Args>
    auto enqueue(F&& f, ThreadPriority priority = ThreadPriority::NORMAL, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;*/
    ~ThreadPool();
    void waitAll();
private:
    std::mutex monitor_mutex;
    std::condition_variable monitor_cv;
    static std::mutex sInstanceMutex;
    static std::shared_ptr<ThreadPool> sInstance;
    std::mutex mFieldsGuardian;
    std::thread monitor;
    std::vector< std::thread > workers;
    std::queue<Task> tasks;
    std::function<void()> stop_task = []() {}; // Special task to signal thread termination

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable all_tasks_done_condition;
    std::condition_variable idle_condition;  // To signal when a thread goes idle
    ThreadPriority mPriority;
    void clearResources();
    void createThread(ThreadPriority priority = ThreadPriority::NORMAL);

    void signalWorkerToStop();
    int buffer_threads = 0;
    int running_tasks = 0;
    int idle_threads = 0;  // Track idle threads
    size_t max_threads;
    size_t min_threads;
    bool stop;


    void monitoringFunction(); // This function will check and resize the thread pool
};

inline size_t ThreadPool::getActiveThreadCount()  {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return running_tasks;
}

inline void  ThreadPool::incActiveThreadCount()  {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    ++running_tasks;
}

inline void  ThreadPool::decActiveThreadCount()  {
    std::lock_guard<std::mutex> ock(mFieldsGuardian);
    --running_tasks;
}




// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t minThreads, size_t maxThreads, size_t bufferThreads, ThreadPriority priority)
    : min_threads(minThreads), max_threads(maxThreads), buffer_threads(bufferThreads), stop(false), mPriority(priority){
    // ... [Unchanged]
    for (size_t i = 0; i < min_threads; ++i)
        createThread();

    monitor = std::thread(&ThreadPool::monitoringFunction, this);
}


inline void ThreadPool::clearResources() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;

        // Signal all threads to stop
        for (size_t i = 0; i < workers.size(); ++i) {
            tasks.emplace([]() {}, TaskType::Stop, ThreadPriority::NORMAL);
        }

        // Notify all waiting worker threads about the new tasks
        condition.notify_all();
    }

    // Notify the monitor thread to wake up and exit
    monitor_cv.notify_one();

    // Wait for the monitoring thread to finish
    if (monitor.joinable()) {
        monitor.join();
    }

    // Join all worker threads
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    // Clear the worker threads and reset the tasks queue
    workers.clear();
    std::queue<Task> emptyQueue;
    std::swap(tasks, emptyQueue);
}



inline void ThreadPool::createThread(ThreadPriority priority) {
    workers.emplace_back(
        [this, priority]  // Include the priority in the lambda capture
        {

            bool  setPrioritySuccess = true;
            // Set thread priority based on the platform
#ifdef _WIN32
            if (!SetThreadPriority(GetCurrentThread(), MapToOSPriority(priority))) {
                setPrioritySuccess = false;
            }
#else
            sched_param sch_params;
            sch_params.sched_priority = MapToOSPriority(priority);
            if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch_params) != 0) {
                setPrioritySuccess = false;
            }
#endif

            if (!setPrioritySuccess) {
                CTools::getTools()->logEvent("Unable to adjust thread's priority. Ensure proper permissions.", eLogEntryCategory::localSystem, 4, eLogEntryType::failure, eColor::lightPink);

                /*REDUNDANT
                // Fallback to setting the thread to Normal priority if setting priority failed
                priority = ThreadPriority::NORMAL;
                #ifdef _WIN32
                SetThreadPriority(GetCurrentThread(), MapToOSPriority(priority));
                 #else
                sch_params.sched_priority = MapToOSPriority(priority);
                pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch_params);
                 #endif*/
            }

            for (;;)
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

                    if (this->stop && this->tasks.empty())
                        return;

                    if (std::get<1>(this->tasks.front()) == TaskType::Stop) {
                        tasks.pop();
                        return;
                    }

                    task = std::move(std::get<0>(this->tasks.front()));
                    this->tasks.pop();
                    incActiveThreadCount();
                    idle_threads--;
                }

                // Execute the task outside the lock
                task();

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    decActiveThreadCount();
                    idle_threads++;
                    idle_condition.notify_one();
                    if (running_tasks == 0) {
                        all_tasks_done_condition.notify_all();
                    }
                }
            }
        }
    );
    idle_threads++;
}




/**
 * @brief Signals a worker thread to terminate gracefully.
 *
 * This function is designed to encapsulate the logic of signaling a worker thread
 * in the pool to stop its execution. It is used in scenarios where the thread pool
 * needs to dynamically adjust the number of active worker threads based on the
 * current workload.
 *
 * The function does the following:
 * 1. It pushes a special task of type `TaskType::Stop` into the task queue.
 *    Worker threads are programmed to recognize tasks of this type as an
 *    instruction to terminate.
 * 2. It then sends a notification to potentially wake up one of the worker
 *    threads waiting on the condition variable. This ensures that if a worker
 *    thread is waiting for a task, it can quickly pick up the stop task and
 *    exit.
 * 3. Finally, the function decrements the count of `idle_threads`. This is
 *    important because when a worker thread picks up a stop task, it will
 *    eventually terminate and won't be available to process any more tasks,
 *    effectively reducing the number of idle threads in the pool.
 */
inline void ThreadPool::signalWorkerToStop() {
    tasks.emplace([]() {}, TaskType::Stop, ThreadPriority::NORMAL);  // Add a special stop task to the queue
    condition.notify_one();  // Notify one worker about the new task
    idle_threads--;  // Decrement the count of idle threads
}

/**
 * @brief Monitors and dynamically adjusts the size of the thread pool.
 *
 * This method periodically checks the state of the thread pool and adjusts
 * the number of worker threads based on current demand. The function operates
 * in two main phases:
 *
 * 1. Expansion Phase: If the current number of tasks surpasses the minimum threshold,
 * the thread pool size is increased, first by adding a buffer amount of threads and
 * then potentially growing up to the maximum allowable threads.
 *
 * 2. Shrinking Phase: When tasks are scarce and there are more idle threads than required,
 * the excess idle threads are terminated to conserve resources, ensuring that a base minimum
 * remains active.
 *
 * The function continues to run as long as the thread pool is active, with checks
 * approximately every second to minimize CPU overhead.
 */
inline void ThreadPool::monitoringFunction() {
    while (true) {
        std::unique_lock<std::mutex> lock(monitor_mutex);

        // Wait until stop is true or 1 second has passed
        monitor_cv.wait_for(lock, std::chrono::seconds(1), [this] { return this->stop; });

        if (stop)
            break; // Exit loop if stop is true

        lock.unlock(); // Release monitor_mutex before acquiring queue_mutex

        // Now, acquire queue_mutex to access shared data
        {
            std::unique_lock<std::mutex> qlock(queue_mutex);

            // Calculate demand for threads based on tasks waiting to be processed
            size_t demand = tasks.size();

            // ------------------- EXPANSION PHASE ---------------------
            if (demand + running_tasks > workers.size() && workers.size() < max_threads) {
                size_t threadsNeeded = min(demand + running_tasks - workers.size(), max_threads - workers.size());
                for (size_t i = 0; i < threadsNeeded; ++i) {
                    createThread(mPriority);
                }
            }

            // ------------------- SHRINKING PHASE ---------------------
            while (tasks.empty() && idle_threads > min_threads && workers.size() > min_threads + buffer_threads) {
                signalWorkerToStop();
                if (!idle_condition.wait_for(qlock, std::chrono::seconds(1), [this] { return idle_threads <= min_threads + buffer_threads || stop; })) {
                    if (stop)
                        break;
                }
            }
        }

        if (stop)
            break;
    }
}







/**
 * @brief Enqueues a new task into the thread pool.
 *
 * This method adds a new task to the thread pool's task queue. The task is wrapped
 * in a std::future, allowing the caller to obtain the result of the task once it's completed.
 * It's designed to handle tasks that are associated with a std::shared_ptr, ensuring
 * that the shared_ptr's reference count is properly managed.
 *
 * Problem Addressed:
 * When a std::shared_ptr is passed to a lambda function, it increases the reference
 * count of the shared object. This can lead to an issue where the shared_ptr is not
 * released after the task completion if the lambda captures the shared_ptr and holds it
 * indefinitely. This behavior can cause resource leaks or prevent objects from being
 * destroyed when expected.
 *
 * Solution:
 * To manage the life-cycle of the shared_ptr correctly, the lambda function within this
 * method is marked as mutable. This allows the lambda to modify its captured variables.
 * After the task is executed, the shared_ptr is explicitly reset by calling task.reset().
 * This action decreases the reference count of the shared object, ensuring that the
 * resources are released as soon as the task is completed.
 *
 * @tparam F Function type of the task.
 * @tparam Args Variadic template for arguments to the task function.
 * @param f Function to be executed in the task.
 * @param priority Priority of the task in the thread pool.
 * @param args Arguments to be passed to the task function.
 * @return std::future that can be used to retrieve the result of the task.
 * @throws std::runtime_error if the thread pool is stopped before enqueueing the task.
 */
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, ThreadPriority priority, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace(
            [task]() {
                (*task)();
            },
            TaskType::Normal, priority
        );
    }

    condition.notify_one();
    return res;
}




inline void ThreadPool::waitAll()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    all_tasks_done_condition.wait(lock, [this]() { return tasks.empty() && running_tasks == 0; });
}


inline ThreadPool::ThreadPool(ThreadPool&& other) noexcept
{
    std::lock_guard<std::mutex> lock(other.queue_mutex);
    monitor = std::move(other.monitor);
    workers = std::move(other.workers);
    tasks = std::move(other.tasks);
    running_tasks = other.running_tasks;
    idle_threads = other.idle_threads;
    max_threads = other.max_threads;
    min_threads = other.min_threads;
    buffer_threads = other.buffer_threads;
    stop = other.stop;

    // Reset other's state
    other.running_tasks = 0;
    other.idle_threads = 0;
    other.stop = true; // Prevent other from doing any further processing
}

inline ThreadPool& ThreadPool::operator=(ThreadPool&& other) noexcept
{
    if (this != &other) {
        std::lock(queue_mutex, other.queue_mutex);  // Lock both mutexes without deadlock
        std::lock_guard<std::mutex> self_lock(queue_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> other_lock(other.queue_mutex, std::adopt_lock);

        clearResources();

        monitor = std::move(other.monitor);
        workers = std::move(other.workers);
        tasks = std::move(other.tasks);
        running_tasks = other.running_tasks;
        idle_threads = other.idle_threads;
        max_threads = other.max_threads;
        min_threads = other.min_threads;
        buffer_threads = other.buffer_threads;
        stop = other.stop;

        // Reset other's state
        other.running_tasks = 0;
        other.idle_threads = 0;
        other.stop = true;
    }
    return *this;
}


// the destructor joins all threads
inline ThreadPool::~ThreadPool() {
    waitAll();  // Ensure all tasks have been processed.
    clearResources();
}
inline std::shared_ptr<ThreadPool> ThreadPool::sInstance = nullptr;

#endif
