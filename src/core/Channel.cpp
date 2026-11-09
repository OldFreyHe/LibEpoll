//
// Created by root on 9/20/26.
//

#include "Channel.h"

#include <sys/epoll.h>
#include <utility>

#include "EventLoop.h"

Channel::Channel(EventLoop* loop, int fd) :
    loop_(loop),
    fd_(fd)
{

}

Channel::~Channel()
{

}

int Channel::fd() const
{
    return fd_;
}

void Channel::setReadCallback(EventCallback callback)
{
    readCallback_ = std::move(callback);
}

void Channel::setWriteCallback(EventCallback callback)
{
    writeCallback_ = std::move(callback);
}

void Channel::setCloseCallback(EventCallback callback)
{
    closeCallback_ = std::move(callback);
}

void Channel::setErrorCallback(EventCallback callback)
{
    errorCallback_ = std::move(callback);
}

void Channel::handleEvent(uint32_t events)
{
    if (events & EPOLLIN)
    {
        if (readCallback_ != nullptr)
        {
            readCallback_(events);
        }
    }

    if (events & EPOLLOUT)
    {
        if (writeCallback_ != nullptr)
        {
            writeCallback_(events);
        }
    }

    if (events & EPOLLHUP)
    {
        if (closeCallback_ != nullptr)
        {
            closeCallback_(events);
        }
    }

    if (events & EPOLLERR)
    {
        if (errorCallback_ != nullptr)
        {
            errorCallback_(events);
        }
    }
}

void Channel::enableReading()
{
    events_ |= EPOLLIN;

    update();
}

void Channel::enableWriting()
{
    events_ |= EPOLLOUT;

    update();
}

void Channel::disableWriting()
{
    events_ &= ~EPOLLOUT;
}

void Channel::update()
{
    if(events_ == 0)
    {
        if (added_)
        {
            remove();
        }

        return;
    }
    if (!added_)
    {
        loop_->add(fd_, events_, this);
        added_ = true;
    } else
    {
        loop_->modify(fd_, events_, this);
    }
}

void Channel::disableAll()
{
    events_ = 0;

    if (added_)
    {
        remove();
    }
}

void Channel::remove()
{
    if (!added_)
    {
        return;
    }

    loop_->remove(fd_);
    added_ = false;
    events_ = 0;
}