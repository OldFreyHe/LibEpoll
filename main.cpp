#include "EPollServer.h"
#include "EchoHandler.h"
#include <iostream>

int main()
{
    EPollServer server(9008);
    // Decode a Protobuf request and reply using the original sequence.
    server.setMessageCallback([](Connection& connection, MessageFrame&& frame)
    {
        MessageFrame response;
        if (!libepoll::example::buildEchoResponse(frame, response))
        {
            std::cerr << "Unknown messageId or invalid Protobuf payload\n";
            return;
        }
        if (!connection.sendMessage(response))
        {
            std::cerr << "Failed to send echo response\n";
        }
    });
    server.start();

    return 0;
}
