//
// Created by root on 9/30/26.
//

#ifndef LIBEPOLL_BUFFER_H
#define LIBEPOLL_BUFFER_H

#include <cstddef>
#include <string>
#include <vector>

class Buffer
{
public:
    explicit Buffer(std::size_t initialSize = 1024);

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&&) = default;
    Buffer& operator=(Buffer&&) = default;

    ~Buffer() = default;

    std::size_t readableSize() const;

    std::size_t writeableSize() const;

    const char* peek() const;

    void append(const char* data, std::size_t size);

    void retrieve(std::size_t len);

    void retrieveAll();

    std::string retrieveAsString();

    char* beginWrite();

    void hasWritten(std::size_t len);

private:
    void ensureWriteableBytes(std::size_t len);

    void makeSpace(std::size_t len);

private:
    std::vector<char> buffer_;

    std::size_t readIndex_;

    std::size_t writeIndex_;

};


#endif //LIBEPOLL_BUFFER_H
