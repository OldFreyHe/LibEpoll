#ifndef LIBEPOLL_PROTOBUF_CODEC_H
#define LIBEPOLL_PROTOBUF_CODEC_H

#include "MessageFrame.h"
#include "MessageCodec.h"
#include <google/protobuf/message_lite.h>
#include <string>
#include <type_traits>
#include <utility>

class ProtobufCodec
{
public:
    template <typename Message>
    static bool decode(const MessageFrame& frame, Message& message)
    {
        static_assert(std::is_base_of_v<google::protobuf::MessageLite, Message>);
        if (frame.payload.size() > MessageCodec::MAX_FRAME_LEN - MessageCodec::HEADER_LEN)
        {
            return false;
        }
        Message decoded;
        const void* data = frame.payload.empty() ? static_cast<const void*>("")
                                                : frame.payload.data();
        if (!decoded.ParseFromArray(data, static_cast<int>(frame.payload.size())))
        {
            return false;
        }
        message = std::move(decoded);
        return true;
    }

    template <typename Message>
    static bool encode(const Message& message, MessageFrame& frame)
    {
        static_assert(std::is_base_of_v<google::protobuf::MessageLite, Message>);
        if (message.ByteSizeLong() > MessageCodec::MAX_FRAME_LEN - MessageCodec::HEADER_LEN)
        {
            return false;
        }
        std::string bytes;
        if (!message.SerializeToString(&bytes) ||
            bytes.size() > MessageCodec::MAX_FRAME_LEN - MessageCodec::HEADER_LEN)
        {
            return false;
        }
        frame.payload.assign(bytes.begin(), bytes.end());
        return true;
    }
};

#endif
