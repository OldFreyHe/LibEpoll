#ifndef LIBEPOLL_REQUEST_MANAGER_H
#define LIBEPOLL_REQUEST_MANAGER_H

#include "MessageFrame.h"
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <unordered_map>

class Connection;
class EventLoop;

// Shared handle may outlive Connection. EventLoop must outlive all calls to it.
class RequestManager : public std::enable_shared_from_this<RequestManager>
{
public:
    enum class Status { Success, Timeout, Disconnected, SendFailed };
    struct Result { Status status; MessageFrame response; };
    using Callback = std::function<void(Result)>;

    RequestManager(EventLoop* loop, Connection* connection);
    // Thread-safe submission; callbacks run on IO thread. They must not block or throw.
    void requestAsync(MessageFrame request, uint32_t responseId,
                      std::chrono::milliseconds timeout, Callback callback);
    // Throws logic_error on the IO thread. Loop must be running.
    Result request(MessageFrame frame, uint32_t responseId, std::chrono::milliseconds timeout);

    // Internal IO-thread operations.
    bool accept(MessageFrame& response);
    void disconnect();

private:
    struct Pending
    {
        uint32_t responseId;
        std::chrono::steady_clock::time_point deadline;
        Callback callback;
    };
    void finish(uint64_t sequence, Status status, MessageFrame response = {});
    EventLoop* loop_;
    Connection* connection_;
    uint64_t nextSequence_ = 0;
    std::unordered_map<uint64_t, Pending> pending_;
};
#endif
