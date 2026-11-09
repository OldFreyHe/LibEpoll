//
// Created by root on 10/20/26.
//

#ifndef LIBEPOLL_MESSAGECODEC_H
#define LIBEPOLL_MESSAGECODEC_H

#pragma once

#include "MessageFrame.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class MessageCodec
{
public:
    static constexpr std::size_t HEAD_LEN = 8;
    static constexpr std::size_t HEADER_LEN = 24;

    static constexpr uint32_t MAX_FRAME_LEN = 10 * 1024 * 1024;

    // 将逻辑消息帧编码成可通过 TCP 发送的字节。
    static bool encode(
        const MessageFrame& frame,
        std::vector<uint8_t>& output);

    // 解析固定长度的协议头。
    // input 至少需要包含 HEADER_LEN 个字节。
    static bool decodeHeader(
        const uint8_t* input,
        std::size_t inputLength,
        MessageFrame& frame);

    // 解析一条已经完整接收的消息帧。
    static bool decodeFrame(
        const uint8_t* input,
        std::size_t inputLength,
        MessageFrame& frame);
};

#endif //LIBEPOLL_MESSAGECODEC_H
