#include "WorkerThread.h"
#include <cstdlib>
#include <future>
#include <vector>

void check(bool ok) { if (!ok) std::abort(); }

// Two independent business objects share thread A, a third runs on thread B.
struct Probe
{
    std::thread::id thread;
    void processMessage() { thread = std::this_thread::get_id(); }
};

int main()
{
    Probe login, session, order;
    std::vector<int> sequence;
    std::promise<void> firstDone, secondDone;
    {
        WorkerThread account("account"), orders("orders");
        check(account.submit([&] { login.processMessage(); sequence.push_back(1); }));
        check(account.submit([&]
        {
            session.processMessage();
            sequence.push_back(2);
            firstDone.set_value();
        }));
        check(orders.submit([&] { order.processMessage(); secondDone.set_value(); }));
        firstDone.get_future().wait();
        secondDone.get_future().wait();
        check(login.thread == session.thread);
        check(login.thread != order.thread);
        check(login.thread != std::this_thread::get_id());
        check(sequence == std::vector<int>({1, 2}));
    }
}
