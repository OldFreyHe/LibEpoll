#include "EchoHandler.h"
#include "echo.pb.h"
#include "MessageCodec.h"
#include "MessageId.h"

#include <string>
#include <utility>

namespace libepoll::example
{
bool buildEchoResponse(const MessageFrame& request, MessageFrame& response)
{
    if (request.messageId != ECHO_CATEGORY_REQUEST ||
        request.payload.size() > MessageCodec::MAX_FRAME_LEN - MessageCodec::HEADER_LEN)
    {
        return false;
    }

    EchoRequest body;
    const std::string payload(request.payload.begin(), request.payload.end());
    if (!body.ParseFromString(payload))
    {
        return false;
    }

    EchoResponse reply;
    reply.set_text(body.text());
    std::string encoded;
    if (!reply.SerializeToString(&encoded) ||
        encoded.size() > MessageCodec::MAX_FRAME_LEN - MessageCodec::HEADER_LEN)
    {
        return false;
    }

    MessageFrame result;
    result.head = request.head;
    result.sequence = request.sequence;
    result.messageId = ECHO_CATEGORY_RESPONSE;
    result.payload.assign(encoded.begin(), encoded.end());
    response = std::move(result);
    return true;
}
}
