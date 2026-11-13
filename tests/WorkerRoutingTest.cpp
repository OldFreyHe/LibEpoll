#include "MessageDispatcher.h"
#include "EventLoop.h"
#include "EchoWorker.h"
#include <cstdlib>
#include <future>
#include <sys/socket.h>
#include <unistd.h>

void check(bool value) { if (!value) std::abort(); }
template<typename Request, typename Response>
class ProbeWorker : public MessageWorker<Request, Response>
{
public:
    std::promise<std::thread::id> processed;
    void processMessage(const Request& request, Response& response) override
    {
        response.set_text(request.text());
        processed.set_value(std::this_thread::get_id());
    }
};

int main()
{
    using namespace libepoll::example;
    EventLoop loop;
    int sockets[2];
    check(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sockets) == 0);
    Connection connection(&loop, nullptr, sockets[0]);
    WorkerThread shared("shared");
    MessageDispatcher dispatcher;
    auto first = std::make_shared<ProbeWorker<EchoRequest, EchoResponse>>();
    // Test-only reverse route uses the second ID without adding example schemas.
    auto second = std::make_shared<ProbeWorker<EchoResponse, EchoRequest>>();
    auto firstDone = first->processed.get_future();
    auto secondDone = second->processed.get_future();
    dispatcher.registerWorker(shared, first);
    dispatcher.registerWorker(shared, second);
    EchoRequest request;
    request.set_text("route A");
    MessageFrame a;
    a.messageId = ECHO_CATEGORY_REQUEST;
    a.sequence = 11;
    check(ProtobufCodec::encode(request, a));
    check(dispatcher.dispatch(connection, std::move(a)) == MessageDispatcher::Result::Queued);
    EchoResponse other;
    other.set_text("route B");
    MessageFrame b;
    b.messageId = ECHO_CATEGORY_RESPONSE;
    b.sequence = 22;
    check(ProtobufCodec::encode(other, b));
    check(dispatcher.dispatch(connection, std::move(b)) == MessageDispatcher::Result::Queued);
    const auto thread = firstDone.get();
    check(thread == secondDone.get());
    check(thread != std::this_thread::get_id());
    // Drop queued replies after connection closes; worker captures must remain safe.
    connection.shutdown();
    loop.queueInLoop([&] { loop.quit(); });
    loop.loop();
    close(sockets[1]);
}
