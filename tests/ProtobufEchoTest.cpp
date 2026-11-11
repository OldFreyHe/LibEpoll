#include "EchoHandler.h"
#include "MessageCodec.h"
#include "echo.pb.h"
#include "MessageId.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
void check(bool value)
{
    if (!value)
    {
        std::cerr << "Protobuf echo test failed\n";
        std::abort();
    }
}
}

int main()
{
    using namespace libepoll::example;
    EchoRequest body;
    // Include embedded NUL and UTF-8 text to verify binary-safe payload handling.
    const std::string text = std::string("hello\0", 6) + "\xe4\xbd\xa0\xe5\xa5\xbd";
    body.set_text(text);
    std::string payload;
    check(body.SerializeToString(&payload));

    MessageFrame request;
    request.sequence = 0x123456789ABCDEF0ULL;
    request.messageId = ECHO_CATEGORY_REQUEST;
    request.head = {'L', 'I', 'B', 'E', 'P', 'O', 'L', 'L'};
    request.payload.assign(payload.begin(), payload.end());

    std::vector<uint8_t> wire;
    check(MessageCodec::encode(request, wire));
    MessageFrame decoded;
    check(MessageCodec::decodeFrame(wire.data(), wire.size(), decoded));
    MessageFrame response;
    check(buildEchoResponse(decoded, response));
    check(response.sequence == request.sequence);
    check(response.messageId == ECHO_CATEGORY_RESPONSE);
    check(response.head == request.head);
    check(MessageCodec::encode(response, wire));
    check(MessageCodec::decodeFrame(wire.data(), wire.size(), decoded));
    EchoResponse reply;
    check(reply.ParseFromString(std::string(decoded.payload.begin(), decoded.payload.end())));
    check(reply.text() == text);

    // An empty proto3 message is valid and represents default field values.
    request.payload.clear();
    check(buildEchoResponse(request, response));
    check(reply.ParseFromString(std::string(response.payload.begin(), response.payload.end())));
    check(reply.text().empty());

    request.payload = {0x0a, 0x05, 'x'}; // Truncated length-delimited text field.
    response.sequence = 99;
    check(!buildEchoResponse(request, response));
    check(response.sequence == 99);
    request.payload.clear();
    request.messageId = ECHO_CATEGORY_RESPONSE;
    check(!buildEchoResponse(request, response));
    request.messageId = 999;
    check(!buildEchoResponse(request, response));
}
