#include "Connector.h"
#include "ConnectionManager.h"
#include "EventLoop.h"
#include "MessageId.h"
#include "ProtobufCodec.h"
#include "echo.pb.h"

#include <iostream>
#include <string>
#include <thread>

namespace
{
bool printResult(const RequestManager::Result& result)
{
    if (result.status != RequestManager::Status::Success)
    {
        std::cerr << "Request failed, status=" << static_cast<int>(result.status) << '\n';
        return false;
    }
    libepoll::example::EchoResponse response;
    if (!ProtobufCodec::decode(result.response, response))
    {
        std::cerr << "Invalid Protobuf response\n";
        return false;
    }
    std::cout << "sequence=" << result.response.sequence
              << ", response=" << response.text() << '\n';
    return true;
}
}

int main(int argc, char** argv)
{
    using namespace std::chrono_literals;
    using namespace libepoll::example;
    const std::string mode = argc > 1 ? argv[1] : "async";
    const std::string host = argc > 2 ? argv[2] : "127.0.0.1";
    const std::string text = argc > 3 ? argv[3] : "Hello from LibEpoll";
    if (argc > 4 || (mode != "async" && mode != "sync"))
    {
        std::cerr << "Usage: EPollClient [async|sync] [IPv4] [text]\n";
        return 1;
    }

    EventLoop loop;
    ConnectionManager connections(&loop);
    Connector connector(&loop);
    std::thread worker;
    int exitCode = 1;
    try
    {
        connector.start(host, 9008, 3s,
            [&](Connector::Status status, int fd, int error)
            {
                if (status != Connector::Status::Connected)
                {
                    std::cerr << "Connect failed, status=" << static_cast<int>(status)
                              << ", errno=" << error << '\n';
                    loop.quit();
                    return;
                }
                auto* connection = connections.add(fd);
                if (!connection)
                {
                    std::cerr << "Cannot register connection\n";
                    loop.quit();
                    return;
                }
                EchoRequest request;
                request.set_text(text);
                MessageFrame frame;
                frame.messageId = ECHO_CATEGORY_REQUEST;
                if (!ProtobufCodec::encode(request, frame))
                {
                    std::cerr << "Cannot encode request\n";
                    loop.quit();
                    return;
                }
                auto requests = connection->requests();
                if (mode == "async")
                {
                    requests->requestAsync(std::move(frame), ECHO_CATEGORY_RESPONSE, 3s,
                        [&](RequestManager::Result result)
                        {
                            exitCode = printResult(result) ? 0 : 1;
                            loop.quit();
                        });
                }
                else
                {
                    // Only this business thread blocks; the main thread keeps polling IO.
                    worker = std::thread([&, requests, frame = std::move(frame)]() mutable
                    {
                        try
                        {
                            auto result = requests->request(std::move(frame), ECHO_CATEGORY_RESPONSE, 3s);
                            exitCode = printResult(result) ? 0 : 1;
                        }
                        catch (const std::exception& error)
                        {
                            std::cerr << "Request exception: " << error.what() << '\n';
                        }
                        loop.quit();
                    });
                }
            });
        loop.loop();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        // An unexpected IO-loop exception cannot safely leave a blocking worker waiting.
        if (worker.joinable()) std::terminate();
        return 1;
    }
    if (worker.joinable()) worker.join();
    return exitCode;
}
