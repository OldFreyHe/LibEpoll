#include "Connector.h"
#include "Channel.h"
#include "EventLoop.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <stdexcept>
#include <utility>

struct Connector::State : std::enable_shared_from_this<Connector::State>
{
    EventLoop* loop;
    int fd = -1;
    bool started = false;
    bool done = false;
    Callback callback;
    std::unique_ptr<Channel> channel;
    explicit State(EventLoop* value) : loop(value) {}
    void finish(Status status, int error, bool notify = true)
    {
        if (done) return;
        done = true;
        if (channel) channel->remove();
        int connectedFd = -1;
        if (status == Status::Connected) connectedFd = std::exchange(fd, -1);
        else if (fd != -1) { close(fd); fd = -1; }
        // Channel may still be executing handleEvent, or occur later in this
        // epoll batch. Keep the state alive until the entire batch completes.
        auto keepAlive = shared_from_this();
        loop->queueInLoop([keepAlive] {});
        auto completion = std::move(callback);
        if (notify && completion) completion(status, connectedFd, error);
    }
    void ready()
    {
        if (done) return;
        int error = 0;
        socklen_t size = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size) < 0) error = errno;
        finish(error ? Status::Failed : Status::Connected, error);
    }
};

Connector::Connector(EventLoop* loop)
{
    if (!loop) throw std::invalid_argument("Connector requires EventLoop");
    state_ = std::make_shared<State>(loop);
}

Connector::~Connector()
{
    if (state_->started && !state_->done) state_->finish(Status::Cancelled, ECANCELED, false);
}

void Connector::cancel()
{
    if (state_->started) state_->finish(Status::Cancelled, ECANCELED);
}

void Connector::start(const std::string& ipv4, unsigned short port,
                      std::chrono::milliseconds timeout, Callback callback)
{
    auto state = state_;
    if (state->started) throw std::logic_error("Connector is one-shot");
    if (!callback || timeout.count() <= 0)
        throw std::invalid_argument("Connector requires callback and positive timeout");
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ipv4.c_str(), &address.sin_addr) != 1)
        throw std::invalid_argument("Connector requires numeric IPv4 address");
    state->started = true;
    state->callback = std::move(callback);
    state->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (state->fd < 0) { state->finish(Status::Failed, errno); return; }
    const int result = connect(state->fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    if (result == 0) { state->finish(Status::Connected, 0); return; }
    const int error = errno;
    if (error != EINPROGRESS && error != EINTR)
    {
        state->finish(Status::Failed, error);
        return;
    }
    state->channel = std::make_unique<Channel>(state->loop, state->fd);
    std::weak_ptr<State> weak = state;
    auto ready = [weak](int32_t)
    {
        if (auto value = weak.lock()) value->ready();
    };
    state->channel->setWriteCallback(ready);
    state->channel->setErrorCallback(ready);
    state->channel->setCloseCallback(ready);
    state->channel->enableWriting();
    state->loop->runAfter(timeout, [weak]
    {
        if (auto value = weak.lock()) value->finish(Status::Timeout, ETIMEDOUT);
    });
}
