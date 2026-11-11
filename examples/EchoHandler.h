#ifndef LIBEPOLL_ECHOHANDLER_H
#define LIBEPOLL_ECHOHANDLER_H

#include "MessageFrame.h"

namespace libepoll::example
{
// False for unknown message IDs, invalid payloads or serialization failure.
// On failure, response is unchanged.
bool buildEchoResponse(const MessageFrame& request, MessageFrame& response);
}

#endif
