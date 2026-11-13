#include "EPollServer.h"
#include <cstdlib>
#include <future>
#include <thread>

void check(bool value) { if (!value) std::abort(); }
int main()
{
    EPollServer server(0);
    auto& worker = server.createWorkerThread("business");
    std::promise<void> entered, release, finished;
    auto gate = release.get_future().share();
    check(worker.submit([&]
    {
        entered.set_value();
        gate.wait();
        finished.set_value();
    }));
    entered.get_future().wait();
    std::thread io([&] { server.start(); });
    server.stop();
    server.stop(); // Idempotent, including cross-thread calls.
    release.set_value();
    io.join();
    finished.get_future().get();
    check(worker.idle());
    check(!worker.submit([] {}));

    EPollServer stoppedBeforeStart(0);
    stoppedBeforeStart.stop();
    stoppedBeforeStart.start();
}
