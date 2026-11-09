//
// Created by root on 10/20/26.
//

#ifndef LIBEPOLL_GENMESSAGESEQUENCE_H
#define LIBEPOLL_GENMESSAGESEQUENCE_H

#include <atomic>
#include <cstdint>

class GenMessageSequence {
public:
    static uint64_t getSequence() {
        return sequence.fetch_add(1, std::memory_order_relaxed);
    }

private:
    static std::atomic<uint64_t> sequence;
};


#endif //LIBEPOLL_GENMESSAGESEQUENCE_H
