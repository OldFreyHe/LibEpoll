# LibEpoll
A Network Communication Dynamic Library Implemented Using EPoll

## Run tests on Linux

## Synchronous and asynchronous requests

On the IO thread, obtain `auto requests = connection.requests()` and retain
that shared handle for calls from other threads. `requestAsync(frame,
expectedResponseId, timeout, callback)` queues submission without blocking;
`request(frame, expectedResponseId, timeout)` waits for the same completion
mechanism. The manager assigns sequence numbers, overriding the outgoing value.
Responses must match both sequence and expected ID. Deadlines include queueing
time and use a monotonic clock. Completion removes the pending entry first,
so duplicate responses cannot complete it twice.

Synchronous calls on the owning IO thread throw `logic_error`. The EventLoop
must be running and must outlive every request call and retained handle.
Close connections and complete pending requests before stopping their loop;
`quit()` alone does not cancel pending requests. Callbacks run on the IO thread
and must not block or throw. Unmatched/late frames continue to the application
dispatcher, which must only register request IDs as business handlers.
Protobuf request payloads can be prepared with `ProtobufCodec::encode()` and
successful response payloads decoded with `ProtobufCodec::decode()`.

## Build and execute tests

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

The server registers `EchoRequest -> EchoResponse` with `MessageDispatcher`.
The dispatcher selects the handler by request ID, uses `ProtobufCodec` to
parse the payload, invokes the typed handler, then serializes and sends the
response with the original head and sequence. Business handlers only fill the
response fields. Registration rejects duplicate request IDs and empty handlers.
Register handlers before `start()`; handlers run synchronously on the IO thread.
`dispatch()` returns `Sent` (accepted for sending), `UnknownMessage`,
`InvalidPayload`, `EncodeFailed` or `SendFailed`. Handler exceptions propagate
to the caller. This initial API automatically sends one response per request;
it does not yet support asynchronous worker replies or request timeouts.

`message_dispatcher` tests routing, automatic responses, sequence preservation,
duplicate registration, malformed payload rejection and empty proto3 requests.

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
