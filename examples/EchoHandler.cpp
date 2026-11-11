#include "EchoHandler.h"
#include "echo.pb.h"
#include "MessageCodec.h"
#include "MessageId.h"
#include "ProtobufCodec.h"

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
    if (!ProtobufCodec::decode(request, body))
    {
        return false;
    }

    EchoResponse reply;
    reply.set_text(body.text());
    MessageFrame result;
    result.head = request.head;
    result.sequence = request.sequence;
    result.messageId = ECHO_CATEGORY_RESPONSE;
    if (!ProtobufCodec::encode(reply, result))
    {
        return false;
    }
    response = std::move(result);
    return true;
}
}
