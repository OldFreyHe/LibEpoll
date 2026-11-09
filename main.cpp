#include "EPollServer.h"

int main()
{
    EPollServer server(9008);
    server.start();

    return 0;
}