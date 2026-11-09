//
// Created by root on 10/20/26.
//

#include "MessageParser.h"
#include "Buffer.h"
#include <cstdint>

bool MessageParser::parse(
    Buffer& buffer,
    const MessageParser::MessageCallback& callback) const
{
    while (true)
    {
        const std::size_t readable =
            buffer.readableSize();

        // 1. 连固定长度的协议头都不完整。
        //    不消费数据，等待下一次接收。
        if (readable < MessageCodec::HEADER_LEN)
        {
            return true;
        }

        MessageFrame frame;

        const auto* data =
            reinterpret_cast<const uint8_t*>(
                buffer.peek());

        // 2. 先解析协议头，获取整帧长度。
        if (!MessageCodec::decodeHeader(
                data,
                readable,
                frame))
        {
            // 头部字段非法，例如长度超出允许范围。
            return false;
        }

        const std::size_t frameLength =
            frame.length;

        // 3. 头部完整，但消息体还没收齐。
        //    不消费任何数据，等下次 read 后继续解析。
        if (readable < frameLength)
        {
            return true;
        }

        // 4. 当前至少有一整帧。
        //    decodeFrame 只处理这一帧，不包含后面的粘包数据。
        if (!MessageCodec::decodeFrame(
                data,
                frameLength,
                frame))
        {
            return false;
        }

        // 5. 消费已解析的字节。
        //    后续数据仍留在 Buffer 中。
        buffer.retrieve(frameLength);

        // 6. 将完整消息交给上层。
        if (callback)
        {
            callback(std::move(frame));
        }

        // 7. 回到循环顶部。
        //    如果还有完整帧，继续解析；
        //    如果只剩半帧，就等待下一次接收。
    }
}
