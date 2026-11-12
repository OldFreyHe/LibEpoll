#include "WorkerPool.h"
#include <atomic>
#include <cstdlib>
#include <future>

void check(bool ok) { if (!ok) std::abort(); }
int main()
{
    std::promise<void> started, release;
    auto gate = release.get_future().share();
    std::atomic<int> completed{0};
    {
        WorkerPool pool(1, 1);
        check(pool.submit([&] { started.set_value(); gate.wait(); ++completed; }));
        started.get_future().wait();
        check(pool.submit([&] { ++completed; }));
        check(!pool.submit([] {}));
        release.set_value();
    }
    check(completed == 2);
}
