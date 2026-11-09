//
// Created by root on 9/20/26.
//

#include "Acceptor.h"
#include "Connection.h"
#include "EventLoop.h"
#include "ConnectionManager.h"
#include "Channel.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <list>
#include <stdexcept>
#include <sys/epoll.h>


namespace
{
    int createListenSocket(uint16_t port)
    {
        const int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

        if (listenfd == -1)
        {
            throw std::runtime_error("socket failed");
        }

        int reuse = 1;

        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) == -1)
        {
            close(listenfd);
            throw std::runtime_error("setsockopt failed");
        }

        sockaddr_in address{};

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);

        if (bind(listenfd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1)
        {
            close(listenfd);

            throw std::runtime_error("bind failed");
        }

        if (listen(listenfd, SOMAXCONN) == -1)
        {
            close(listenfd);

            throw std::runtime_error("listen failed");
        }

        return listenfd;
    }
}

Acceptor::Acceptor(EventLoop* loop, ConnectionManager* manager, uint16_t port) :
    loop_(loop),
    manager_(manager),
    listenFd_(-1)
{
    if (loop_ == nullptr)
    {
        throw std::runtime_error("Acceptor requires a valid EventLoop");
    }

    if (manager_ == nullptr)
    {
        throw std::runtime_error("Acceptor requires a valid ConnectionManager");
    }

    listenFd_ = createListenSocket(port);

    listenChannel_ = std::make_unique<Channel>(loop_, listenFd_);

    listenChannel_->setReadCallback([this](uint32_t)
    {
        handleRead();
    });

    listenChannel_->setErrorCallback([this](uint32_t)
    {
        handleError();
    });

    listenChannel_->enableReading();
}

Acceptor::~Acceptor()
{
    if (listenChannel_)
    {
        listenChannel_->remove();
        listenChannel_.reset();
    }

    if (listenFd_ != -1)
    {
        close(listenFd_);
        listenFd_ = -1;
    }
}

void Acceptor::handleRead()
{
    while (true)
    {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        const int clientFd = accept4(listenFd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);

        if (clientFd >= 0)
        {
            manager_->add(clientFd);

            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            break;
        }

        throw std::runtime_error("Accept failed");
    }
}

void Acceptor::handleError()
{
    throw std::runtime_error("Listen sock error");
}
