#include "MessageDispatcher.h"
#include "EventLoop.h"
#include "MessageId.h"
#include "echo.pb.h"
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
void check(bool condition)
{
    if (!condition)
    {
        std::cerr << "MessageDispatcher test failed\n";
        std::abort();
    }
}
}

int main()
{
    using namespace libepoll::example;
    int sockets[2];
    check(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sockets) == 0);
    EventLoop loop;
    Connection connection(&loop, nullptr, sockets[0]);
    MessageDispatcher dispatcher;
    int calls = 0;
    std::function<void(Connection&, const EchoRequest&, EchoResponse&)> handler =
        [&](Connection&, const EchoRequest& request, EchoResponse& response)
        {
            ++calls;
            response.set_text(request.text());
        };
    dispatcher.registerHandler<EchoRequest, EchoResponse>(
        ECHO_CATEGORY_REQUEST, ECHO_CATEGORY_RESPONSE, handler);
    bool duplicateRejected = false;
    try
    {
        dispatcher.registerHandler<EchoRequest, EchoResponse>(
            ECHO_CATEGORY_REQUEST, ECHO_CATEGORY_RESPONSE, handler);
    }
    catch (const std::invalid_argument&) { duplicateRejected = true; }
    check(duplicateRejected);

    EchoRequest body;
    body.set_text("dispatcher test");
    MessageFrame frame;
    frame.messageId = ECHO_CATEGORY_REQUEST;
    frame.sequence = 123456789;
    check(ProtobufCodec::encode(body, frame));
    check(dispatcher.dispatch(connection, std::move(frame)) == MessageDispatcher::Result::Sent);
    uint8_t bytes[1024];
    const auto count = recv(sockets[1], bytes, sizeof(bytes), 0);
    check(count > 0);
    MessageFrame response;
    check(MessageCodec::decodeFrame(bytes, static_cast<std::size_t>(count), response));
    check(response.sequence == 123456789);
    check(response.messageId == ECHO_CATEGORY_RESPONSE);
    EchoResponse reply;
    check(ProtobufCodec::decode(response, reply));
    check(reply.text() == body.text());
    check(calls == 1);

    frame.messageId = ECHO_CATEGORY_REQUEST;
    frame.payload = {0x0a, 0x05, 'x'};
    EchoRequest unchanged;
    unchanged.set_text("unchanged");
    check(!ProtobufCodec::decode(frame, unchanged));
    check(unchanged.text() == "unchanged");
    check(dispatcher.dispatch(connection, std::move(frame)) == MessageDispatcher::Result::InvalidPayload);
    check(calls == 1);
    frame.messageId = 9999;
    check(dispatcher.dispatch(connection, std::move(frame)) == MessageDispatcher::Result::UnknownMessage);
    frame.messageId = ECHO_CATEGORY_REQUEST;
    frame.payload.clear();
    check(dispatcher.dispatch(connection, std::move(frame)) == MessageDispatcher::Result::Sent);
    check(calls == 2);
    close(sockets[1]);
}
