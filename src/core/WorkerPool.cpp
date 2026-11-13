#include "WorkerPool.h"
#include <stdexcept>
#include <iostream>

WorkerPool::WorkerPool(std::size_t threads, std::size_t capacity) : capacity_(capacity)
{
    if (!threads || !capacity) throw std::invalid_argument("WorkerPool requires nonzero sizes");
    try
    {
        for (std::size_t i = 0; i < threads; ++i) threads_.emplace_back([this] { run(); });
    }
    catch (...)
    {
        { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; }
        ready_.notify_all();
        for (auto& thread : threads_) thread.join();
        throw;
    }
}
WorkerPool::~WorkerPool()
{
    { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; }
    ready_.notify_all();
    for (auto& thread : threads_) thread.join();
}
bool WorkerPool::submit(std::function<void()> task)
{
    if (!task) return false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_ || tasks_.size() >= capacity_) return false;
        tasks_.push_back(std::move(task));
    }
    ready_.notify_one();
    return true;
}
void WorkerPool::stopAccepting()
{
    { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; }
    ready_.notify_all();
}
bool WorkerPool::idle()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.empty() && active_ == 0;
}
void WorkerPool::run()
{
    for (;;)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            ready_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop_front();
            ++active_;
        }
        try { task(); }
        catch (const std::exception& error) { std::cerr << "Worker task failed: " << error.what() << '\n'; }
        catch (...) { std::cerr << "Worker task failed with unknown exception\n"; }
        { std::lock_guard<std::mutex> lock(mutex_); --active_; }
    }
}
