//
// Created by root on 9/29/26.
//

#ifndef LIBEPOLL_EPOLLSERVER_H
#define LIBEPOLL_EPOLLSERVER_H

#include <memory>
#include <cstdint>
#include "Connection.h"
#include "WorkerThread.h"
#include <atomic>
#include <vector>

class EventLoop;
class ConnectionManager;
class Acceptor;

class EPollServer
{
public:
    explicit EPollServer(uint16_t port);
    ~EPollServer();

    EPollServer(EPollServer const&) = delete;
    EPollServer& operator=(EPollServer const&) = delete;

    void start();
    void stop();
    // Configure before start(). Server owns and drains these execution groups.
    WorkerThread& createWorkerThread(std::string name, std::size_t capacity = 1024);

    // Set before start(). Called on the IO thread; do not block or destroy the connection.
    void setMessageCallback(Connection::MessageCallback callback);

private:
    void beginStop();
    void finishStop();
    std::unique_ptr<EventLoop> loop_;
    std::unique_ptr<ConnectionManager> connectionManager_;
    std::unique_ptr<Acceptor> acceptor_;
    std::vector<std::unique_ptr<WorkerThread>> workers_;
    std::atomic<bool> stopRequested_{false};
    bool started_ = false;
};

#endif //
