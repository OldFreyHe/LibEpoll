//
// Created by root on 9/17/26.
//

#ifndef LIBEPOLL_EVENTLOOP_H
#define LIBEPOLL_EVENTLOOP_H
#include <cstdint>
#include <memory>
#include <atomic>
#include <functional>
#include <mutex>
#include <vector>
#include <chrono>
#include <map>

class Channel;

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop &) = delete;
    EventLoop &operator=(const EventLoop &) = delete;

    void loop();
    void quit();

    void add(int fd, uint32_t events, void* ptr);
    void modify(int fd, uint32_t events, void* ptr);
    void remove(int fd);

    void queueInLoop(std::function<void()> callback);
    bool isInLoopThread() const;
    // Call on the IO thread. Timers use a monotonic clock.
    void runAfter(std::chrono::milliseconds delay, std::function<void()> callback);

private:
    void handleWakeup();
    void doPendingTasks();

private:
    int epoll_fd_;
    int event_fd_;
    std::atomic<bool> running_;

    std::unique_ptr<Channel> wakeUpChannel_;

    std::mutex mutex_;
    std::vector<std::function<void()>> pendingTasks_;
    std::multimap<std::chrono::steady_clock::time_point, std::function<void()>> timers_;
    inline static thread_local EventLoop* currentLoop_ = nullptr;
};

#endif //LIBEPOLL_EVENTLOOP_H
