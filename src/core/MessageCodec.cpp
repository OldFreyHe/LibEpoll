//
// Created by root on 10/20/26.
//
#include "MessageCodec.h"

#include <algorithm>
#include <limits>

namespace
{

void appendUint32(
    std::vector<uint8_t>& output,
    uint32_t value)
{
    output.push_back(
        static_cast<uint8_t>((value >> 24) & 0xFF));

    output.push_back(
        static_cast<uint8_t>((value >> 16) & 0xFF));

    output.push_back(
        static_cast<uint8_t>((value >> 8) & 0xFF));

    output.push_back(
        static_cast<uint8_t>(value & 0xFF));
}

void appendUint64(
    std::vector<uint8_t>& output,
    uint64_t value)
{
    for (int shift = 56; shift >= 0; shift -= 8)
    {
        output.push_back(
            static_cast<uint8_t>((value >> shift) & 0xFF));
    }
}

uint32_t readUint32(const uint8_t* input)
{
    return
        (static_cast<uint32_t>(input[0]) << 24) |
        (static_cast<uint32_t>(input[1]) << 16) |
        (static_cast<uint32_t>(input[2]) << 8) |
        static_cast<uint32_t>(input[3]);
}

uint64_t readUint64(const uint8_t* input)
{
    uint64_t value = 0;

    for (int i = 0; i < 8; ++i)
    {
        value = (value << 8) | input[i];
    }

    return value;
}

} // namespace

bool MessageCodec::encode(
    const MessageFrame& frame,
    std::vector<uint8_t>& output)
{
    if (frame.payload.size() >
        MAX_FRAME_LEN - HEADER_LEN)
    {
        return false;
    }

    const auto totalLength =
        HEADER_LEN + frame.payload.size();

    if (totalLength >
        std::numeric_limits<uint32_t>::max())
    {
        return false;
    }

    // 每次编码先清空输出，避免重复追加旧数据。
    output.clear();
    output.reserve(totalLength);

    // 1. 写入固定长度的协议标识。
    for (char ch : frame.head)
    {
        output.push_back(
            static_cast<uint8_t>(ch));
    }

    // 2. 写入整帧长度，包含头部。
    appendUint32(
        output,
        static_cast<uint32_t>(totalLength));

    // 3. 写入请求关联编号。
    appendUint64(output, frame.sequence);

    // 4. 写入消息类型编号。
    appendUint32(output, frame.messageId);

    // 5. 写入业务 payload。
    output.insert(
        output.end(),
        frame.payload.begin(),
        frame.payload.end());

    return true;
}

bool MessageCodec::decodeHeader(
    const uint8_t* input,
    std::size_t inputLength,
    MessageFrame& frame)
{
    if (input == nullptr ||
        inputLength < HEADER_LEN)
    {
        return false;
    }

    // 8 字节 head。
    std::copy(
        input,
        input + HEAD_LEN,
        frame.head.begin());

    // 4 字节 length。
    frame.length = readUint32(input + 8);

    // 长度必须至少包含完整头部，且不能超过上限。
    if (frame.length < HEADER_LEN ||
        frame.length > MAX_FRAME_LEN)
    {
        return false;
    }

    // 8 字节 sequence。
    frame.sequence = readUint64(input + 12);

    // 4 字节 messageId。
    frame.messageId = readUint32(input + 20);

    return true;
}

bool MessageCodec::decodeFrame(
    const uint8_t* input,
    std::size_t inputLength,
    MessageFrame& frame)
{
    if (!decodeHeader(input, inputLength, frame))
    {
        return false;
    }

    // 只有拿到整帧后，才能解码完整消息。
    if (inputLength != frame.length)
    {
        return false;
    }

    const auto payloadLength =
        inputLength - HEADER_LEN;

    frame.payload.assign(
        input + HEADER_LEN,
        input + HEADER_LEN + payloadLength);

    return true;
}