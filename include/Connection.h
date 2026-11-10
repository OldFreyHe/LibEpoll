//
// Created by root on 9/20/26.
//

#ifndef LIBEPOLL_CONNECTION_H
#define LIBEPOLL_CONNECTION_H

#include <cstdint>
#include <memory>
#include "Buffer.h"
#include "MessageParser.h"

class EventLoop;
class Channel;
class ConnectionManager;

class Connection
{
public:
    using MessageCallback = std::function<void(Connection&, MessageFrame&&)>;

    Connection(EventLoop *loop, ConnectionManager* manager, int fd);
    ~Connection();

    // Configure and invoke on the connection's EventLoop thread.
    void setMessageCallback(MessageCallback callback);

    // EventLoop thread only. True means accepted for sending, not delivered or acknowledged.
    // The caller supplies sequence; responses must copy it from the request.
    bool sendMessage(const MessageFrame& frame);

private:
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    bool closeConnection();

private:
    ConnectionManager *manager_ = nullptr;

    int fd_ = -1;

    std::unique_ptr<Channel> channel_;

    bool closed_ = false;

    Buffer inputBuffer_;

    MessageParser parser_;
    MessageCallback messageCallback_;

    Buffer outputBuffer_;
};

#endif //LIBEPOLL_CONNECTION_H
