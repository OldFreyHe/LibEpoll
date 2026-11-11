#include "RequestManager.h"
#include "Connection.h"
#include "EventLoop.h"
#include <stdexcept>
#include <limits>

RequestManager::RequestManager(EventLoop* loop, Connection* connection)
    : loop_(loop), connection_(connection) {}

void RequestManager::requestAsync(MessageFrame frame, uint32_t responseId,
                                  std::chrono::milliseconds timeout, Callback callback)
{
    if (!callback || timeout.count() <= 0)
        throw std::invalid_argument("Request requires callback and positive timeout");
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    auto self = shared_from_this();
    loop_->queueInLoop([self, frame = std::move(frame), responseId, deadline,
                        callback = std::move(callback)]() mutable
    {
        if (!self->connection_)
        {
            callback({Status::Disconnected, {}});
            return;
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            callback({Status::Timeout, {}});
            return;
        }
        if (self->nextSequence_ == std::numeric_limits<uint64_t>::max())
        {
            callback({Status::SendFailed, {}});
            return;
        }
        frame.sequence = ++self->nextSequence_;
        const auto sequence = frame.sequence;
        self->pending_.emplace(sequence, Pending{responseId, deadline, std::move(callback)});
        std::weak_ptr<RequestManager> weak = self;
        self->loop_->runAfter(std::chrono::ceil<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()), [weak, sequence]
        {
            if (auto manager = weak.lock()) manager->finish(sequence, Status::Timeout);
        });
        if (!self->connection_->sendMessage(frame)) self->finish(sequence, Status::SendFailed);
    });
}

RequestManager::Result RequestManager::request(MessageFrame frame, uint32_t responseId,
                                              std::chrono::milliseconds timeout)
{
    if (loop_->isInLoopThread()) throw std::logic_error("Cannot block the IO thread");
    auto promise = std::make_shared<std::promise<Result>>();
    auto future = promise->get_future();
    requestAsync(std::move(frame), responseId, timeout,
        [promise](Result result) { promise->set_value(std::move(result)); });
    return future.get();
}

bool RequestManager::accept(MessageFrame& response)
{
    const auto it = pending_.find(response.sequence);
    if (it == pending_.end() || it->second.responseId != response.messageId) return false;
    const auto sequence = response.sequence;
    if (std::chrono::steady_clock::now() >= it->second.deadline)
        finish(sequence, Status::Timeout);
    else
        finish(sequence, Status::Success, std::move(response));
    return true;
}

void RequestManager::finish(uint64_t sequence, Status status, MessageFrame response)
{
    const auto it = pending_.find(sequence);
    if (it == pending_.end()) return;
    auto callback = std::move(it->second.callback);
    pending_.erase(it);
    callback({status, std::move(response)});
}

void RequestManager::disconnect()
{
    connection_ = nullptr;
    auto pending = std::move(pending_);
    pending_.clear();
    for (auto& [sequence, entry] : pending) entry.callback({Status::Disconnected, {}});
}
