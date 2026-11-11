# LibEpoll
A Network Communication Dynamic Library Implemented Using EPoll

## Run tests on Linux

Requires CMake 4.3 or later, a C++20 compiler, and a native build tool
(such as Make or Ninja), plus Protobuf's C++ library and `protoc` compiler.
Use matching Protobuf compiler and library versions.

```bash
bash run_tests.sh
```

The script configures a Debug build in `build/tests`, compiles the project,
and runs all registered CTest tests. It stops on configuration, build, or
test failure and returns a nonzero exit code. An empty test suite also fails.
It can be invoked from any working directory using its path.

Additional arguments are passed to CTest:

```bash
bash run_tests.sh -R connection_send  # Run matching tests
bash run_tests.sh -V                  # Show full test output
```

## Protobuf echo example

`proto/echo.proto` defines `EchoRequest` (messageId 1) and `EchoResponse`
(messageId 2). CMake runs `protoc` during the build; generated `.pb.h` and
`.pb.cc` files stay in the build directory. Do not generate them into `include`
or commit them to the repository.

The server on port 9008 now expects a framed Protobuf request instead of
arbitrary payload echo. The existing 24-byte transport header is unchanged:
8 bytes of head, 4 bytes of total frame length, 8 bytes of sequence and
4 bytes of messageId. Integers use network byte order. The payload is the
serialized `EchoRequest`. Its `text` is returned in an `EchoResponse` with
the same head and sequence and message ID `0x0102`. Unknown IDs and malformed payloads
are logged and ignored. Empty requests are valid proto3 messages.

Transport code remains independent of Protobuf; the example handler owns the
business serialization. Sequence matching, request timeouts and workers are
not implemented yet.

`MessageId.h` combines an 8-bit category and 8-bit message value and registers\r\neach ID during startup. Duplicate category or message IDs trigger an assertion,\r\nso protocol conflicts are detected early.\r\n\r\n`protobuf_echo` tests framing, Protobuf serialization, sequence preservation,
empty messages, unknown IDs and truncated payload rejection.
