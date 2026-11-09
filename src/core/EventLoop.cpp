//
// Created by root on 9/17/26.
//

#include "EventLoop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdexcept>

#include "Connection.h"
#include "Channel.h"

EventLoop::EventLoop()
    : epoll_fd_(-1),
      running_(true),
      wakeUpChannel_(nullptr),
      event_fd_(-1) {

    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ == -1) {
        throw std::runtime_error("epoll_create1 failed");
    }

    event_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (event_fd_ == -1) {
        close(epoll_fd_);
        epoll_fd_ = -1;
        throw std::runtime_error("eventfd failed");
    }

    wakeUpChannel_ = std::make_unique<Channel>(this, event_fd_);
    wakeUpChannel_->setReadCallback([this] (uint32_t)
    {
        handleWakeup();
    });

    wakeUpChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeUpChannel_.reset();

    if (event_fd_ != -1)
    {
        close(event_fd_);
        event_fd_ = -1;
    }

    if (epoll_fd_ != -1)
    {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

void EventLoop::loop()
{
    constexpr int MAX_EVENTS = 64;

    epoll_event events[MAX_EVENTS];

    while (running_)
    {
        const int count = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

        if (count == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            throw std::runtime_error("epoll wait failed");
        }

        for (int i = 0; i < count; i++)
        {

            auto* channel = static_cast<Channel*>(events[i].data.ptr);
            if (channel == nullptr)
            {
                continue;;
            }

            channel->handleEvent(events[i].events);
        }

        doPendingTasks();
    }
}

void EventLoop::quit()
{
    running_ = false;

    const uint64_t value = 1;

    const ssize_t result = write(event_fd_, &value, sizeof(value));

    if (result != sizeof(value))
    {
        if (errno != EAGAIN)
        {
            throw std::runtime_error("eventfd write failed");
        }
    }
}


void EventLoop::add(int fd, uint32_t events, void* ptr)
{
    epoll_event event{};

    event.events = events;
    event.data.ptr = ptr;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == -1)
    {
        throw std::runtime_error("epollctl add failed");
    }
}

void EventLoop::modify(int fd, uint32_t events, void* ptr)
{
    epoll_event event{};

    event.events = events;
    event.data.ptr = ptr;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) == -1)
    {
        throw std::runtime_error("epollctl mod failed");
    }
}

void EventLoop::remove(int fd)
{
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1)
    {
        throw std::runtime_error("epollctl del failed");
    }
}

void EventLoop::queueInLoop(std::function<void()> callback)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingTasks_.push_back(std::move(callback));
    }

    const uint64_t value = 1;

    const ssize_t n = write(event_fd_, &value, sizeof(value));

    if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        throw std::runtime_error("QueueInLoop: write failed");
    }
}

void EventLoop::handleWakeup()
{
    uint64_t value = 0;

    while (true)
    {
        const ssize_t n = read(event_fd_, &value, sizeof(value));

        if (n == sizeof(value))
        {
            continue;
        }

        if (n == -1 && errno == EINTR)
        {
            continue;
        }

        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            break;
        }

        break;
    }

}

void EventLoop::doPendingTasks()
{
    std::vector<std::function<void()>> tasks;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.swap(pendingTasks_); //multithread useful
    }


    for (auto& task : pendingTasks_)
    {
        if (task)
        {
            task();
        }
    }
}

