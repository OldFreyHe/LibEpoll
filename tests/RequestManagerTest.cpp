#include "Connection.h"
#include "EventLoop.h"
#include <cstdlib>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>

void check(bool ok) { if (!ok) std::abort(); }

int main()
{
    using namespace std::chrono_literals;
    int sockets[2];
    check(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sockets) == 0);
    EventLoop loop;
    Connection connection(&loop, nullptr, sockets[0]);
    auto manager = connection.requests();
    bool success = false;
    bool timeout = false;
    MessageFrame request;
    request.messageId = 1;
    manager->requestAsync(request, 2, 100ms, [&](RequestManager::Result result)
    {
        check(result.status == RequestManager::Status::Success);
        success = true;
    });
    loop.queueInLoop([&]
    {
        MessageFrame response;
        response.sequence = 1;
        response.messageId = 3;
        check(!manager->accept(response));
        response.messageId = 2;
        check(manager->accept(response));
        check(!manager->accept(response));
        bool rejected = false;
        try { manager->request(request, 2, 10ms); }
        catch (const std::logic_error&) { rejected = true; }
        check(rejected);
    });
    manager->requestAsync(request, 2, 20ms, [&](RequestManager::Result result)
    {
        check(result.status == RequestManager::Status::Timeout);
        timeout = true;
    });
    std::thread worker([&]
    {
        const auto result = manager->request(request, 2, 40ms);
        check(result.status == RequestManager::Status::Timeout);
        loop.quit();
    });
    loop.loop();
    worker.join();
    check(success && timeout);
    close(sockets[1]);

    EventLoop closingLoop;
    auto closed = std::make_shared<RequestManager>(&closingLoop, nullptr);
    bool disconnected = false;
    closed->requestAsync(request, 2, 100ms, [&](RequestManager::Result result)
    {
        check(result.status == RequestManager::Status::Disconnected);
        disconnected = true;
        closingLoop.quit();
    });
    closingLoop.loop();
    check(disconnected);

    // Both already-pending async and synchronous requests finish on shutdown.
    EventLoop shutdownLoop;
    int pair[2];
    check(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, pair) == 0);
    Connection closingConnection(&shutdownLoop, nullptr, pair[0]);
    auto pending = closingConnection.requests();
    int completions = 0;
    pending->requestAsync(request, 2, 1s, [&](RequestManager::Result result)
    {
        check(result.status == RequestManager::Status::Disconnected);
        ++completions;
    });
    std::thread waiting([&]
    {
        auto result = pending->request(request, 2, 1s);
        check(result.status == RequestManager::Status::Disconnected);
        shutdownLoop.quit();
    });
    shutdownLoop.runAfter(20ms, [&] { closingConnection.shutdown(); });
    shutdownLoop.loop();
    waiting.join();
    check(completions == 1);
    close(pair[1]);
}
