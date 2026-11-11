#include "EPollServer.h"
#include "MessageDispatcher.h"
#include "MessageId.h"
#include "echo.pb.h"
#include <iostream>

int main()
{
    EPollServer server(9008);
    MessageDispatcher dispatcher;
    dispatcher.registerHandler<libepoll::example::EchoRequest,
                                libepoll::example::EchoResponse>(
        libepoll::example::ECHO_CATEGORY_REQUEST,
        libepoll::example::ECHO_CATEGORY_RESPONSE,
        [](Connection&, const auto& request, auto& response)
        {
            response.set_text(request.text());
        });
    server.setMessageCallback([&dispatcher](Connection& connection, MessageFrame&& frame)
    {
        const auto result = dispatcher.dispatch(connection, std::move(frame));
        if (result != MessageDispatcher::Result::Sent)
        {
            std::cerr << "Message dispatch failed, result=" << static_cast<int>(result) << '\n';
        }
    });
    server.start();

    return 0;
}
