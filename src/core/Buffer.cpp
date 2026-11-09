//
// Created by root on 9/30/26.
//

#include "Buffer.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

Buffer::Buffer(std::size_t initialSize) :
    buffer_(initialSize),
    readIndex_(0),
    writeIndex_(0)
{

}

std::size_t Buffer::readableSize() const
{
    return writeIndex_ - readIndex_;
}

std::size_t Buffer::writeableSize() const
{
    return buffer_.size() - writeIndex_;
}

const char* Buffer::peek() const
{
    return buffer_.data() + readIndex_;
}

void Buffer::append(const char* data, std::size_t len)
{
    if (data == nullptr && len != 0)
    {
        throw std::invalid_argument("Buffer::append: data is null");
    }

    ensureWriteableBytes(len);

    std::memcpy(buffer_.data() + writeIndex_, data, len);

    writeIndex_ += len;
}

void Buffer::retrieve(std::size_t len)
{
    if (len >= readableSize())
    {
        retrieveAll();
        return;
    }

    readIndex_ += len;
}

void Buffer::retrieveAll()
{
    readIndex_ = 0;
    writeIndex_ = 0;
}

std::string Buffer::retrieveAsString()
{
    std::string result(buffer_.data() + readIndex_, readableSize());

    retrieveAll();

    return result;
}

void Buffer::ensureWriteableBytes(std::size_t len)
{
    if (writeableSize() >= len)
    {
        return;
    }

    makeSpace(len);
}

void Buffer::makeSpace(std::size_t len)
{
    const std::size_t readable = readableSize();

    if (readIndex_ + writeableSize() < len)
    {
        buffer_.resize(writeIndex_ + len);
    } else
    {
        std::copy(buffer_.begin() + readIndex_, buffer_.begin() + writeIndex_, buffer_.begin());
    }

    readIndex_ = 0;
    writeIndex_ = readable;
}

char* Buffer::beginWrite()
{
    return buffer_.data() + writeIndex_;
}

void Buffer::hasWritten(std::size_t len)
{
    if (len > writeableSize())
    {
        throw std::out_of_range("Buffer::hasWritten: length exceeds writeableSize");
    }

    writeIndex_ += len;
}