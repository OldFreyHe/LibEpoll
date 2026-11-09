//
// Created by root on 10/20/26.
//

#ifndef LIBEPOLL_MESSAGEFRAME_H
#define LIBEPOLL_MESSAGEFRAME_H

#include <array>
#include <cstdint>
#include <vector>

struct MessageFrame
{
    std::array<char, 8> head{};

    uint32_t length = 0;

    uint64_t sequence = 0;

    uint32_t messageId = 0;

    std::vector<uint8_t> payload;
};


#endif //LIBEPOLL_MESSAGEFRAME_H
