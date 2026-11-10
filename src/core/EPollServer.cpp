//
// Created by root on 9/29/26.
//

#include "EPollServer.h"

#include "Acceptor.h"
#include "ConnectionManager.h"
#include "EventLoop.h"
#include <utility>

EPollServer::EPollServer(uint16_t port)
{
    loop_ = std::make_unique<EventLoop>();
    connectionManager_ = std::make_unique<ConnectionManager>(loop_.get());
    acceptor_ = std::make_unique<Acceptor>(loop_.get(), connectionManager_.get(), port);
}

EPollServer::~EPollServer()
{
    stop();
}

void EPollServer::stop()
{
    if (loop_)
    {
        loop_->quit();
    }
}

void EPollServer::start()
{
    loop_->loop();
}

void EPollServer::setMessageCallback(Connection::MessageCallback callback)
{
    connectionManager_->setMessageCallback(std::move(callback));
}
