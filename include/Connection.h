//
// Created by root on 9/20/26.
//

#ifndef LIBEPOLL_CONNECTION_H
#define LIBEPOLL_CONNECTION_H

#include <cstdint>
#include <memory>
#include "ConnectionManager.h"
#include "Buffer.h"

class EventLoop;
class Channel;

class Connection
{
public:
    Connection(EventLoop *loop, ConnectionManager* manager, int fd);
    ~Connection();

private:
    void handleRead();
    void handleClose();
    void handleError();

    bool closeConnection();

private:
    ConnectionManager *manager_ = nullptr;

    int fd_ = -1;

    std::unique_ptr<Channel> channel_;

    bool closed_ = false;

    Buffer inputBuffer_;

    Buffer outputBuffer_;
};

#endif //LIBEPOLL_CONNECTION_H
