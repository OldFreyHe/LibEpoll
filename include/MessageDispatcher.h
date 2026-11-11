#ifndef LIBEPOLL_MESSAGE_DISPATCHER_H
#define LIBEPOLL_MESSAGE_DISPATCHER_H

#include "Connection.h"
#include "ProtobufCodec.h"
#include <functional>
#include <stdexcept>
#include <utility>
#include <unordered_map>

class MessageDispatcher
{
public:
    enum class Result { Sent, UnknownMessage, InvalidPayload, EncodeFailed, SendFailed };
    using UnknownCallback = std::function<void(Connection&, MessageFrame&&)>;

    // Register before starting the IO loop. Dispatch and handlers run on that loop.
    template <typename Request, typename Response>
    void registerHandler(uint32_t requestId, uint32_t responseId,
                         std::function<void(Connection&, const Request&, Response&)> handler)
    {
        if (!handler)
        {
            throw std::invalid_argument("MessageDispatcher: empty handler");
        }
        if (handlers_.contains(requestId))
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
