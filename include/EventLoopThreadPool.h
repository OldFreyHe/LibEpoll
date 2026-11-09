//
// Created by root on 10/12/26.
//

#ifndef LIBEPOLL_EVENTLOOPTHREADPOOL_H
#define LIBEPOLL_EVENTLOOPTHREADPOOL_H
#include "EventLoop.h"

#endif //LIBEPOLL_EVENTLOOPTHREADPOOL_H

#include <memory>
#include <vector>

class LoopThread;

class EventLoopThreadPool
{
public:
    explicit EventLoopThreadPool(EventLoop *baseLoop);

    ~EventLoopThreadPool() = default;

    EventLoopThreadPool(const EventLoopThreadPool &) = delete;
    EventLoopThreadPool &operator=(const EventLoopThreadPool &) = delete;

    void start(std::size_t numThreads);

    EventLoop *getNextLoop();

private:
    EventLoop *baseLoop_;

    std::vector<std::unique_ptr<LoopThread>> threads_;
    std::vector<EventLoop*> loops_;

    std::size_t nextLoopIndex_ = 0;

    bool started_ = false;

};
