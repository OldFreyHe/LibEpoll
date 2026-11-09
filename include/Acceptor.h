//
// Created by root on 9/20/26.
//

#ifndef LIBEPOLL_ACCEPTOR_H
#define LIBEPOLL_ACCEPTOR_H
#include <cstdint>
#include <memory>

class EventLoop;
class Channel;
class ConnectionManager;

class Acceptor
{
public:
    Acceptor(EventLoop *loop, ConnectionManager* manager, uint16_t port);
    ~Acceptor();

    Acceptor(const Acceptor &) = delete;
    Acceptor &operator=(const Acceptor &) = delete;

    void start();

private:
    void handleRead();
    void handleError();

private:
    EventLoop *loop_;
    ConnectionManager *manager_;

    int listenFd_;

    std::unique_ptr<Channel> listenChannel_;
};

#endif //LIBEPOLL_ACCEPTOR_H
