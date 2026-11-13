#ifndef LIBEPOLL_WORKER_THREAD_H
#define LIBEPOLL_WORKER_THREAD_H
#include "WorkerPool.h"
#include <memory>
#include <string>
#include <utility>

// One execution thread and one bounded FIFO, shared by multiple business workers.
class WorkerThread
{
public:
    explicit WorkerThread(std::string name, std::size_t capacity = 1024)
        : name_(std::move(name)), executor_(1, capacity) {}
    bool submit(std::function<void()> task) { return executor_.submit(std::move(task)); }
    void stopAccepting() { executor_.stopAccepting(); }
    bool idle() { return executor_.idle(); }
    const std::string& name() const { return name_; }
    std::shared_ptr<const int> identity() const { return identity_; }
private:
    std::string name_;
    std::shared_ptr<const int> identity_ = std::make_shared<const int>(0);
    WorkerPool executor_;
};
#endif
