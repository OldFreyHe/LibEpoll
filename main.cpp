#include "EPollServer.h"
#include "MessageDispatcher.h"
#include "MessageId.h"
#include "echo.pb.h"
#include "EchoWorker.h"
#include <iostream>

int main()
{
    EPollServer server(9008);
    WorkerThread accountThread("account");
    MessageDispatcher dispatcher;
    dispatcher.registerWorker(accountThread, std::make_shared<EchoWorker>());
    server.setMessageCallback([&dispatcher](Connection& connection, MessageFrame&& frame)
    {
        const auto result = dispatcher.dispatch(connection, std::move(frame));
        if (result != MessageDispatcher::Result::Sent && result != MessageDispatcher::Result::Queued)
        {
            std::cerr << "Message dispatch failed, result=" << static_cast<int>(result) << '\n';
        }
    });
    server.start();

    return 0;
}
