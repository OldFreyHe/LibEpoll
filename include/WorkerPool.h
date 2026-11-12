#ifndef LIBEPOLL_WORKER_POOL_H
#define LIBEPOLL_WORKER_POOL_H
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

// Bounded queue. Destruction drains accepted work and joins all threads.
// Destroy on the owner thread, never from a worker task.
class WorkerPool
{
public:
    explicit WorkerPool(std::size_t threads, std::size_t capacity = 1024);
    ~WorkerPool();
    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;
    bool submit(std::function<void()> task);
private:
    void run();
    std::mutex mutex_;
    std::condition_variable ready_;
    std::deque<std::function<void()>> tasks_;
    std::vector<std::thread> threads_;
    std::size_t capacity_;
    bool stopping_ = false;
};
#endif
