#ifndef LIBEPOLL_CONNECTOR_H
#define LIBEPOLL_CONNECTOR_H

#include <chrono>
#include <functional>
#include <memory>
#include <string>

class EventLoop;

// One-shot nonblocking IPv4 connection. Configure/start/cancel/destroy on IO
// thread (or before loop()). EventLoop must outlive Connector.
class Connector
{
public:
    enum class Status { Connected, Failed, Timeout, Cancelled };
    // On success fd ownership passes to callback; otherwise fd=-1, error is errno.
    using Callback = std::function<void(Status, int fd, int error)>;
    explicit Connector(EventLoop* loop);
    ~Connector();
    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;
    void start(const std::string& ipv4, unsigned short port,
               std::chrono::milliseconds timeout, Callback callback);
    void cancel();
private:
    struct State;
    std::shared_ptr<State> state_;
};
#endif
