//
// Created by root on 10/9/26.
//

#include "LoopThread.h"
#include "EventLoop.h"
#include <stdexcept>

LoopThread::LoopThread() = default;

LoopThread::~LoopThread()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (loop_ != nullptr)
        {
            loop_->quit();
        }

        if (thread_.joinable())
        {
            thread_.join();
        }
    }
}

EventLoop* LoopThread::startLoop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (started_)
        {
            throw std::logic_error("LoopThread has already been started");
        }

        started_ = true;
    }

    thread_ = std::thread(&LoopThread::threadFunc, this);

    std::unique_lock<std::mutex> lock(mutex_);

    cv_.wait(lock, [this]
    {
        return loop_ != nullptr;
    });

    return loop_;
}

void LoopThread::threadFunc()
{
    EventLoop loop;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
    }

    cv_.notify_one();

    loop.loop();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
}
