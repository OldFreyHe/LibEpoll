//
// Created by root on 9/22/26.
//

#include "ConnectionManager.h"
#include "Connection.h"
#include "EventLoop.h"
#include <stdexcept>
#include <utility>

ConnectionManager::ConnectionManager(EventLoop *loop) : loop_(loop)
{
    if (loop_ == nullptr)
    {
        throw std::runtime_error("ConnectionManager requirs a valid EventLoop");
    }
}

ConnectionManager::~ConnectionManager()
{
    connections_.clear();
}

Connection* ConnectionManager::add(int fd)
{
    if (fd == -1)
    {
        return nullptr;
    }

    auto connection = std::make_unique<Connection>(loop_, this, fd);
    connection->setMessageCallback(messageCallback_);

    Connection* ptr = connection.get();

    const auto [it, inserted] = connections_.emplace(fd, std::move(connection));

    if (!inserted)
    {
        return nullptr;
    }

    return ptr;
}

void ConnectionManager::remove(int fd)
{
    connections_.erase(fd);
}

Connection* ConnectionManager::find(int fd)
{
    const auto it = connections_.find(fd);

    if (it == connections_.end())
    {
        return nullptr;
    }

    return it->second.get();
}

void ConnectionManager::removeDeferred(int fd)
{
    loop_->queueInLoop([this, fd]
    {
        remove(fd);
    });
}

void ConnectionManager::setMessageCallback(Connection::MessageCallback callback)
{
    messageCallback_ = std::move(callback);
    for (auto& entry : connections_)
    {
        entry.second->setMessageCallback(messageCallback_);
    }
}
