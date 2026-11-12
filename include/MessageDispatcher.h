#ifndef LIBEPOLL_MESSAGE_DISPATCHER_H
#define LIBEPOLL_MESSAGE_DISPATCHER_H

#include "Connection.h"
#include "ProtobufCodec.h"
#include "WorkerPool.h"
#include "WorkerThread.h"
#include "MessageWorker.h"
#include <functional>
#include <stdexcept>
#include <utility>
#include <unordered_map>

class MessageDispatcher
{
public:
    enum class Result { Sent, Queued, Busy, UnknownMessage, InvalidPayload, EncodeFailed, SendFailed };
    using UnknownCallback = std::function<void(Connection&, MessageFrame&&)>;

    // Multiple worker objects may bind to the same thread. Configure before IO starts.
    // thread must outlive this dispatcher and finish before the EventLoop is destroyed.
    template <typename Worker>
    void registerWorker(WorkerThread& thread, std::shared_ptr<Worker> worker)
    {
        using Request = typename Worker::RequestType;
        using Response = typename Worker::ResponseType;
        if (!worker) throw std::invalid_argument("MessageDispatcher: null worker");
        if (worker->boundThread_ && worker->boundThread_ != thread.identity())
            throw std::invalid_argument("MessageDispatcher: worker already bound to another thread");
        registerWorkerHandler<Request, Response>(MessageTraits<Request>::id,
            MessageTraits<Response>::id, thread,
            [worker](const Request& request, Response& response)
            { worker->processMessage(request, response); });
        worker->boundThread_ = thread.identity();
    }

    // pool must outlive the dispatcher. Handler captures must be thread-safe.
    template <typename Request, typename Response, typename Executor>
    void registerWorkerHandler(uint32_t requestId, uint32_t responseId, Executor& pool,
                              std::function<void(const Request&, Response&)> handler)
    {
        if (!handler || handlers_.find(requestId) != handlers_.end())
            throw std::invalid_argument("MessageDispatcher: empty or duplicate worker handler");
        handlers_.emplace(requestId, [&pool, responseId, handler = std::move(handler)]
            (Connection& connection, MessageFrame&& frame)
        {
            Request request;
            if (!ProtobufCodec::decode(frame, request)) return Result::InvalidPayload;
            auto send = connection.replySender();
            MessageFrame reply;
            reply.head = frame.head;
            reply.sequence = frame.sequence;
            reply.messageId = responseId;
            bool accepted = pool.submit([request = std::move(request), reply = std::move(reply),
                                         send = std::move(send), handler]() mutable
            {
                Response response;
                handler(request, response);
                if (!ProtobufCodec::encode(response, reply))
                    throw std::runtime_error("Worker response encoding failed");
                send(std::move(reply));
            });
            return accepted ? Result::Queued : Result::Busy;
        });
    }

    // Register before starting the IO loop. Dispatch and handlers run on that loop.
    template <typename Request, typename Response>
    void registerHandler(uint32_t requestId, uint32_t responseId,
                         std::function<void(Connection&, const Request&, Response&)> handler)
    {
        if (!handler)
        {
            throw std::invalid_argument("MessageDispatcher: empty handler");
        }
        if (handlers_.find(requestId) != handlers_.end())
        {
            throw std::invalid_argument("MessageDispatcher: duplicate request ID");
        }
        handlers_.emplace(requestId, [responseId, handler = std::move(handler)]
            (Connection& connection, MessageFrame&& frame)
        {
            Request request;
            if (!ProtobufCodec::decode(frame, request))
            {
                return Result::InvalidPayload;
            }
            Response response;
            handler(connection, request, response);
            MessageFrame output;
            output.head = frame.head;
            output.sequence = frame.sequence;
            output.messageId = responseId;
            if (!ProtobufCodec::encode(response, output))
            {
                return Result::EncodeFailed;
            }
            return connection.sendMessage(output) ? Result::Sent : Result::SendFailed;
        });
    }

    Result dispatch(Connection& connection, MessageFrame&& frame) const
    {
        const auto it = handlers_.find(frame.messageId);
        if (it == handlers_.end())
        {
            if (unknownCallback_)
            {
                unknownCallback_(connection, std::move(frame));
            }
            return Result::UnknownMessage;
        }
        return it->second(connection, std::move(frame));
    }

    void setUnknownCallback(UnknownCallback callback)
    {
        unknownCallback_ = std::move(callback);
    }

private:
    using Handler = std::function<Result(Connection&, MessageFrame&&)>;
    std::unordered_map<uint32_t, Handler> handlers_;
    UnknownCallback unknownCallback_;
};

#endif
