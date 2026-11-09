//
// Created by root on 10/12/26.
//

#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "LoopThread.h"

#include <stdexcept>
#include <utility>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop) : baseLoop_(baseLoop)
{
    if (baseLoop_ == nullptr)
    {
        throw std::invalid_argument("EventLoopThreadPool: baseLoop is nullptr");
    }
}

void EventLoopThreadPool::start(std::size_t numThreads)
{
    if (started_)
    {
        throw std::invalid_argument("EventLoopThreadPool has already been started");
    }

    started_ = true;

    threads_.reserve(numThreads);
    loops_.reserve(numThreads);

    for (std::size_t i = 0; i < numThreads; ++i)
    {
        auto thread = std::make_unique<LoopThread>();
        EventLoop* loop = thread->startLoop();

        loops_.push_back(loop);
        threads_.push_back(std::move(thread));
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    if (loops_.empty())
    {
        return baseLoop_;
    }

    EventLoop* loop = loops_[nextLoopIndex_];

    ++nextLoopIndex_;

    if (nextLoopIndex_ >= loops_.size())
    {
        nextLoopIndex_ = 0;
    }

    return loop;
}