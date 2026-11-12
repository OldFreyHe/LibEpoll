#ifndef LIBEPOLL_ECHO_WORKER_H
#define LIBEPOLL_ECHO_WORKER_H
#include "MessageWorker.h"
#include "MessageId.h"
#include "echo.pb.h"

template <> struct MessageTraits<libepoll::example::EchoRequest>
{ static constexpr uint32_t id = libepoll::example::ECHO_CATEGORY_REQUEST; };
template <> struct MessageTraits<libepoll::example::EchoResponse>
{ static constexpr uint32_t id = libepoll::example::ECHO_CATEGORY_RESPONSE; };

class EchoWorker : public MessageWorker<libepoll::example::EchoRequest,
                                       libepoll::example::EchoResponse>
{
public:
    void processMessage(const RequestType& request, ResponseType& response) override
    {
        response.set_text(request.text());
    }
};
#endif
