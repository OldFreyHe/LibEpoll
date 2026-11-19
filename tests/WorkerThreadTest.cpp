#include "WorkerThread.h"
#include <cstdlib>
#include <future>
#include <vector>

void check(bool ok) { if (!ok) std::abort(); }

// A small business worker used to demonstrate WorkerThread assignment.
// Real protobuf workers derive from MessageWorker<Request, Response>; the
// thread grouping rules are the same.
class ExampleWorker
{
public:
    explicit ExampleWorker(const char* name) : name_(name) {}

    void processMessage(int message)
    {
        lastMessage_ = message;
        thread_ = std::this_thread::get_id();
    }

    const char* name() const { return name_; }
    int lastMessage() const { return lastMessage_; }
    std::thread::id thread() const { return thread_; }

private:
    const char* name_;
    int lastMessage_ = 0;
    std::thread::id thread_;
};

int main()
{
    // loginWorker and sessionWorker execute serially on the same business
    // thread. orderWorker belongs to an independent business thread.
    ExampleWorker loginWorker("login");
    ExampleWorker sessionWorker("session");
    ExampleWorker orderWorker("order");
    std::vector<int> sequence;
    std::promise<void> accountDone, orderDone;
    {
        WorkerThread accountThread("account");
        WorkerThread orderThread("orders");

        // Multiple workers can submit work to the same WorkerThread. Its FIFO
        // guarantees that login runs before session.
        check(accountThread.submit([&]
        {
            loginWorker.processMessage(1001);
            sequence.push_back(1);
        }));
        check(accountThread.submit([&]
        {
            sessionWorker.processMessage(1002);
            sequence.push_back(2);
            accountDone.set_value();
        }));

        // This worker runs independently on the second WorkerThread.
        check(orderThread.submit([&]
        {
            orderWorker.processMessage(2001);
            orderDone.set_value();
        }));

        accountDone.get_future().wait();
        orderDone.get_future().wait();

        check(loginWorker.thread() == sessionWorker.thread());
        check(loginWorker.thread() != orderWorker.thread());
        check(loginWorker.thread() != std::this_thread::get_id());
        check(sequence == std::vector<int>({1, 2}));
        check(loginWorker.lastMessage() == 1001);
        check(sessionWorker.lastMessage() == 1002);
        check(orderWorker.lastMessage() == 2001);
    }
}
