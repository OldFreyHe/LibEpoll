//
// Created by root on 9/20/26.
//

#ifndef LIBEPOLL_CHANNEL_H
#define LIBEPOLL_CHANNEL_H

#include <cstdint>
#include <functional>

class EventLoop;

class Channel
{
public:
    using EventCallback = std::function<void (int32_t)>;

public:
    Channel(EventLoop* loop, int fd);
    ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    [[nodiscard]] int fd() const;

    void setReadCallback(EventCallback callback);
    void setWriteCallback(EventCallback callback);
    void setCloseCallback(EventCallback callback);
    void setErrorCallback(EventCallback callback);

    void handleEvent(uint32_t events);

    void enableReading();
    void enableWriting();
    void disableWriting();

    void disableAll();
    void remove();

private:
    void update();

private:
    EventLoop* loop_ = nullptr;
    int fd_ = -1;

    uint32_t events_ = 0;

    bool added_ = false;

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};

#endif //LIBEPOLL_CHANNEL_H