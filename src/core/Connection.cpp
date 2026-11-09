//
// Created by root on 9/20/26.
//

#include "Connection.h"
#include "EventLoop.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Channel.h"

#include <iostream>

#include <sys/uio.h>

Connection::Connection(EventLoop *loop, ConnectionManager* manager, int fd)
    : fd_(fd),
      manager_(manager)
{
    channel_ = std::make_unique<Channel>(loop, fd_);

    channel_->setReadCallback([this](uint32_t)
    {
        handleRead();
    });

    channel_->setCloseCallback([this](uint32_t)
     {
         handleClose();
     });

    channel_->setErrorCallback([this](uint32_t)
    {
        handleError();
    });

    channel_->enableReading();
}

Connection::~Connection()
{
    if (channel_)
    {
        channel_->remove();
    }

    if (fd_ != -1)
    {
        close(fd_);
        fd_ = -1;
    }
}

void Connection::handleRead()
{
    char extraBuffer[65536];

    while (true)
    {
        const std::size_t writeable = inputBuffer_.writeableSize();

        struct iovec vec[2];

        vec[0].iov_base = inputBuffer_.beginWrite();
        vec[0].iov_len = inputBuffer_.writeableSize();

        vec[1].iov_base = extraBuffer;
        vec[1].iov_len = sizeof(extraBuffer);

        const ssize_t n = readv(fd_, vec, 2);

        if (n > 0)
        {
            if (static_cast<std::size_t>(n) <= writeable)
            {
                inputBuffer_.hasWritten(n);
            } else
            {
                inputBuffer_.hasWritten(writeable);

                inputBuffer_.append(extraBuffer, static_cast<std::size_t>(n) - writeable);
            }

            std::cout << "received " << n << " bytes" << std::endl;
            std::cout << "input buffer readable = " << inputBuffer_.readableSize() << std::endl;

            continue;
        }

        if (n == 0)
        {
            handleClose();
            return;
        }

        if (errno == EINTR)
        {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }

        handleError();
        return;
    }
}

void Connection::handleClose()
{
    std::cout << "Connection closed, fd = " << fd_ << std::endl;

    channel_->remove();

    close(fd_);
    fd_ = -1;
}

void Connection::handleError()
{
    std::cerr << "Connection error, fd = " << fd_ << std::endl;
    channel_->remove();

    close(fd_);
    fd_ = -1;
}


bool Connection::closeConnection()
{
    if (closed_)
    {
        return false;
    }

    closed_ = true;

    if (channel_)
    {
        channel_->remove();
    }

    if (fd_ != -1)
    {
        close(fd_);
        fd_ = -1;
    }

    return true;
}