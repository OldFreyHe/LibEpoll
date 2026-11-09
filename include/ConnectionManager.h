//
// Created by root on 9/22/26.
//

#ifndef LIBEPOLL_CONNECTIONMANAGER_H
#define LIBEPOLL_CONNECTIONMANAGER_H
#include <memory>
#include <unordered_map>

class Connection;
class EventLoop;

class ConnectionManager
{
public:
    explicit ConnectionManager(EventLoop *loop);
    ~ConnectionManager();

    ConnectionManager(const ConnectionManager &) = delete;
    ConnectionManager &operator=(const ConnectionManager &) = delete;

    Connection* add(int fd);
    void remove(int fd);

    Connection* find(int fd);

    void removeDeferred(int fd);

private:
    EventLoop *loop_;

    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
};

#endif //LIBEPOLL_CONNECTIONMANAGER_H
