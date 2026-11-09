//
// Created by root on 10/9/26.
//

#ifndef LIBEPOLL_LOOPTHREAD_H
#define LIBEPOLL_LOOPTHREAD_H

#include <condition_variable>
#include <mutex>
#include <thread>
#include <regex>

class EventLoop;

class LoopThread
{
public:
    LoopThread();
    ~LoopThread();

    LoopThread(const LoopThread&) = delete;
    LoopThread& operator=(const LoopThread&) = delete;

    EventLoop* startLoop();

private:
    void threadFunc();

private:
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;

    EventLoop* loop_;
    bool started_;
};

#endif //LIBEPOLL_LOOPTHREAD_H
