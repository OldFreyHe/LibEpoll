#include "Connection.h"
#include "EventLoop.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
void check(bool value)
{
    if (!value)
    {
        std::cerr << "Connection send test failed\n";
        std::abort();
    }
}
}

int main()
{
    // Growth after partial consumption must preserve the unsent bytes.
    Buffer buffer(8);
    buffer.append("abcdefgh", 8);
    buffer.retrieve(5);
    buffer.append("ijklmnop", 8);
    check(buffer.retrieveAsString() == "fghijklmnop");

    int sockets[2];
    check(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sockets) == 0);
    int sendBuffer = 4096;
    check(setsockopt(sockets[0], SOL_SOCKET, SO_SNDBUF, &sendBuffer, sizeof(sendBuffer)) == 0);

    EventLoop loop;
    Connection connection(&loop, nullptr, sockets[0]);
    MessageFrame first;
    first.sequence = 123456789;
    first.messageId = 1;
    first.payload.assign(1024 * 1024, 42);
    MessageFrame second;
    second.sequence = 987654321;
    second.messageId = 2;
    second.payload.assign(1024 * 1024, 99);

    std::vector<uint8_t> firstBytes, secondBytes;
    check(MessageCodec::encode(first, firstBytes));
    check(MessageCodec::encode(second, secondBytes));
    // No reader yet: these sends must leave pending output and preserve its order.
    check(connection.sendMessage(first));
    check(connection.sendMessage(second));

    std::thread reader([&]
    {
        std::vector<uint8_t> received(firstBytes.size() + secondBytes.size());
        std::size_t offset = 0;
        while (offset < received.size())
        {
            const ssize_t n = recv(sockets[1], received.data() + offset,
                                   received.size() - offset, 0);
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
            {
                std::this_thread::yield();
                continue;
            }
            check(n > 0);
            offset += static_cast<std::size_t>(n);
        }
        std::vector<uint8_t> expected = firstBytes;
        expected.insert(expected.end(), secondBytes.begin(), secondBytes.end());
        check(received == expected);
        loop.quit();
    });
    loop.loop();
    reader.join();

    close(sockets[1]);
    // MSG_NOSIGNAL must turn a disconnected peer into failure, not process death.
    check(!connection.sendMessage(first));
    check(!connection.sendMessage(second));
}
