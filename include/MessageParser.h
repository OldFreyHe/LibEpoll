//
// Created by root on 10/20/26.
//

#ifndef LIBEPOLL_MESSAGEPARSER_H
#define LIBEPOLL_MESSAGEPARSER_H

#include "MessageCodec.h"

#include <functional>
#include <utility>

class Buffer;

class MessageParser
{
public:
    using MessageCallback =
        std::function<void(MessageFrame&&)>;

    // 尽可能解析 Buffer 中的所有完整消息。
    //
    // true  : 解析正常，可能还有不完整数据等待后续接收。
    // false : 检测到非法协议帧。
    bool parse(
        Buffer& buffer,
        const MessageCallback& callback) const;
};


#endif //LIBEPOLL_MESSAGEPARSER_H
