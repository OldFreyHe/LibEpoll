#ifndef LIBEPOLL_MESSAGE_WORKER_H
#define LIBEPOLL_MESSAGE_WORKER_H
#include <memory>
#include <cstdint>

// Specialize once per request type in the application protocol header:
// template<> struct MessageTraits<MyRequest> { static constexpr uint32_t id = ...; };
template <typename Message> struct MessageTraits;
class MessageDispatcher;

template <typename Request, typename Response>
class MessageWorker
{
public:
    using RequestType = Request;
    using ResponseType = Response;
    virtual ~MessageWorker() = default;
    MessageWorker() = default;
    MessageWorker(const MessageWorker&) = delete;
    MessageWorker& operator=(const MessageWorker&) = delete;
    virtual void processMessage(const Request& request, Response& response) = 0;
private:
    friend class MessageDispatcher;
    // Keep identity alive to prevent rebinding this instance to another thread.
    std::shared_ptr<const int> boundThread_;
};
#endif
