//
// Created by root on 9/29/26.
//

#include "EPollServer.h"

#include "Acceptor.h"
#include "ConnectionManager.h"
#include "EventLoop.h"
#include <utility>
#include <stdexcept>

EPollServer::EPollServer(uint16_t port)
{
    loop_ = std::make_unique<EventLoop>();
    connectionManager_ = std::make_unique<ConnectionManager>(loop_.get());
    acceptor_ = std::make_unique<Acceptor>(loop_.get(), connectionManager_.get(), port);
}

EPollServer::~EPollServer()
{
    // Caller must join the thread executing start() before destroying the server.
    for (auto& worker : workers_) worker->stopAccepting();
    workers_.clear();
}

void EPollServer::stop()
{
    if (stopRequested_.exchange(true)) return;
    loop_->queueInLoop([this] { beginStop(); });
}

WorkerThread& EPollServer::createWorkerThread(std::string name, std::size_t capacity)
{
    if (started_ || stopRequested_) throw std::logic_error("Configure workers before start");
    workers_.push_back(std::make_unique<WorkerThread>(std::move(name), capacity));
    return *workers_.back();
}

void EPollServer::beginStop()
{
    acceptor_->stop();
    // Close first: synchronous requests made by workers must be unblocked before draining.
    connectionManager_->shutdown();
    for (auto& worker : workers_) worker->stopAccepting();
    finishStop();
}

void EPollServer::finishStop()
{
    for (auto& worker : workers_)
    {
        if (!worker->idle())
        {
            loop_->runAfter(std::chrono::milliseconds(1), [this] { finishStop(); });
            return;
        }
    }
    // Drain replies and deferred removals already queued before leaving the loop.
    loop_->queueInLoop([this] { loop_->quit(); });
}

void EPollServer::start()
{
    if (started_) throw std::logic_error("Server start is one-shot");
    started_ = true;
    loop_->loop();
}

void EPollServer::setMessageCallback(Connection::MessageCallback callback)
{
    connectionManager_->setMessageCallback(std::move(callback));
}
