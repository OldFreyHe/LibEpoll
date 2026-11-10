#include "EPollServer.h"

int main()
{
    EPollServer server(9008);
    // Minimal echo example: preserve messageId, sequence and the opaque payload.
    server.setMessageCallback([](Connection& connection, MessageFrame&& frame)
    {
        connection.sendMessage(frame);
    });
    server.start();

    return 0;
}
