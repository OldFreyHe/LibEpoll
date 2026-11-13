#include "Connector.h"
#include "Channel.h"
#include "ConnectionManager.h"
#include "EventLoop.h"
#include "MessageDispatcher.h"
#include "MessageId.h"
#include "echo.pb.h"
#include "EchoWorker.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>

void check(bool condition) { if (!condition) std::abort(); }

int main()
{
    using namespace std::chrono_literals;
    using namespace libepoll::example;
    int listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    check(listener >= 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
    socklen_t length = sizeof(address);
    check(getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) == 0);
    const auto port = ntohs(address.sin_port);
    check(listen(listener, 8) == 0);

    // Cancel before processing the writable event; completion must occur once.
    {
        EventLoop loop;
        Connector connector(&loop);
        int callbacks = 0;
        connector.start("127.0.0.1", port, 1s,
            [&](Connector::Status status, int fd, int error)
            {
                ++callbacks;
                if (status == Connector::Status::Connected)
                {
                    // Immediate connect is also legal; ownership still transfers.
                    close(fd);
                }
                else check(status == Connector::Status::Cancelled && error == ECANCELED);
            });
        connector.cancel();
        connector.cancel();
        check(callbacks == 1);
        loop.queueInLoop([&] { loop.quit(); });
        loop.loop();
        check(callbacks == 1);
        int accepted = accept4(listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (accepted >= 0) close(accepted);
    }

    {
        EventLoop loop;
        ConnectionManager server(&loop), client(&loop);
        WorkerThread workers("echo");
        MessageDispatcher dispatcher;
        auto echoWorker = std::make_shared<EchoWorker>();
        dispatcher.registerWorker(workers, echoWorker);
        WorkerThread anotherThread("other");
        MessageDispatcher otherDispatcher;
        bool rebindRejected = false;
        try { otherDispatcher.registerWorker(anotherThread, echoWorker); }
        catch (const std::invalid_argument&) { rebindRejected = true; }
        check(rebindRejected);
        server.setMessageCallback([&](Connection& connection, MessageFrame&& frame)
        {
            check(dispatcher.dispatch(connection, std::move(frame)) == MessageDispatcher::Result::Queued);
        });
        Channel acceptChannel(&loop, listener);
        acceptChannel.setReadCallback([&](int32_t)
        {
            int fd = accept4(listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            check(fd >= 0);
            check(server.add(fd) != nullptr);
        });
        acceptChannel.enableReading();
        bool complete = false;
        std::thread synchronousCaller;
        Connector connector(&loop);
        connector.start("127.0.0.1", port, 1s,
            [&](Connector::Status status, int fd, int error)
            {
                check(status == Connector::Status::Connected && error == 0);
                auto* connection = client.add(fd);
                check(connection != nullptr);
                EchoRequest request;
                request.set_text("real TCP protobuf request");
                MessageFrame frame;
                frame.messageId = ECHO_CATEGORY_REQUEST;
                check(ProtobufCodec::encode(request, frame));
                connection->requests()->requestAsync(frame, ECHO_CATEGORY_RESPONSE, 1s,
                    [&](RequestManager::Result result)
                    {
                        check(result.status == RequestManager::Status::Success);
                        EchoResponse response;
                        check(ProtobufCodec::decode(result.response, response));
                        check(response.text() == "real TCP protobuf request");
                        complete = true;
                    });
                auto requests = connection->requests();
                synchronousCaller = std::thread([&, requests, frame]
                {
                    auto result = requests->request(frame, ECHO_CATEGORY_RESPONSE, 1s);
                    check(result.status == RequestManager::Status::Success);
                    EchoResponse response;
                    check(ProtobufCodec::decode(result.response, response));
                    check(response.text() == "real TCP protobuf request");
                    loop.queueInLoop([&] { check(complete); loop.quit(); });
                });
            });
        loop.runAfter(2s, [] { check(false); });
        loop.loop();
        synchronousCaller.join();
        check(complete);
        acceptChannel.remove();
    }
    // Keep the port bound but no longer listening: refusal without a port reuse race.
    close(listener);
    int reserved = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    check(reserved >= 0);
    address.sin_port = 0;
    check(bind(reserved, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
    check(getsockname(reserved, reinterpret_cast<sockaddr*>(&address), &length) == 0);
    {
        EventLoop loop;
        Connector connector(&loop);
        int calls = 0;
        connector.start("127.0.0.1", ntohs(address.sin_port), 1s,
            [&](Connector::Status status, int fd, int error)
            {
                check(status == Connector::Status::Failed && fd == -1 && error != 0);
                ++calls;
                loop.quit();
            });
        loop.loop();
        check(calls == 1);
    }
    close(reserved);
}
