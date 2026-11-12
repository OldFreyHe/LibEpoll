# LibEpoll

## Business workers

The preferred business API uses fixed `WorkerThread` groups instead of random
pool scheduling. Define a worker derived from `MessageWorker<Request, Response>`
and override `processMessage(const Request&, Response&)`. Specialize
`MessageTraits<Request>::id` and `MessageTraits<Response>::id` once in the
application protocol header (see `examples/EchoWorker.h`). Then register:

```cpp
WorkerThread accountThread("account");
MessageDispatcher dispatcher;
dispatcher.registerWorker(accountThread, std::make_shared<EchoWorker>());
// Register other worker types on accountThread to share the same execution thread.
```

Each thread owns a bounded FIFO. All workers bound to it run serially on that
same OS thread; different groups run concurrently. A worker instance cannot
be rebound to another thread, and duplicate request IDs are rejected per
dispatcher. Register before dispatch starts. Dispatcher/task captures keep the
worker alive. Captured external dependencies must also outlive its tasks.
The thread name is a logical label, not an OS thread name. A slow task delays
all workers in its group. Worker construction/destruction are not guaranteed
to run on that thread; only dispatched processing is.

Declare EventLoop/server first, WorkerThreads second and dispatcher last so
threads drain before connections and the loop are destroyed. Do not access
worker state from outside its execution thread without synchronization.
`worker_thread` tests shared thread identity, distinct group threads and FIFO;
`connector` covers inherited worker dispatch and rejects cross-thread rebinding.

`WorkerPool(threadCount, queueCapacity)` provides a bounded FIFO queue; `submit`
returns false for a full queue or an empty task. Destruction drains accepted
tasks and joins workers. Destroy the pool on its owner thread, not inside a task.
Task exceptions are logged and workers continue running.

`MessageDispatcher::registerWorkerHandler<Request, Response>(requestId,
responseId, pool, handler)` parses requests on the IO thread and runs
`handler(const Request&, Response&)` on a worker. Responses retain the original
sequence and are posted back to the owning EventLoop. Closed connections drop
replies safely; replies never locate a connection by a potentially reused fd.
Handlers may run concurrently and finish out of order. Captured state must be
thread-safe. The existing `registerHandler` remains available for short IO-thread
handlers. Worker submission returns `Queued` or `Busy`, not delivery confirmation.
Busy queues and failed tasks currently produce no protocol error response;
callers can time out. Define application error messages before adding retries.

The pool must finish all tasks before the EventLoop is destroyed; keep it alive
through connection cleanup too. Stop accepting work before draining the pool.
This version does not implement coordinated graceful server shutdown.
`worker_pool` checks queue capacity and draining; `connector` now exercises
real TCP requests through worker handling and back to the original IO thread.

## Nonblocking Connector

### Run the client/server examples

After `bash run_tests.sh` builds the project, start the server in one terminal:

```bash
./build/tests/EPollServer
```

In a second terminal, run either client mode (port 9008):

```bash
./build/tests/EPollClient async 127.0.0.1 "hello async"
./build/tests/EPollClient sync 127.0.0.1 "hello sync"
```

Both print the matching response sequence and echoed text, then exit. Async
mode completes in an IO-thread callback. Sync mode waits on a business thread
while the main thread continues polling the EventLoop. Connection and request
timeouts are each three seconds; failure returns a nonzero process exit code.
The client obtains a shared RequestManager handle on the IO thread, so the
worker never dereferences a Connection pointer. It joins the worker before
destroying the connections and loop. Stop the example server with Ctrl+C.

`Connector` performs a one-shot connection to a numeric IPv4 address. Call
`start(address, port, timeout, callback)` on the owning IO thread or before
`loop()`. Completion reports `Connected`, `Failed`, `Timeout`, or `Cancelled`.
On success the callback owns the nonblocking socket and should pass it to
`ConnectionManager::add(fd)` (or close it). On failure fd is -1 and the error
argument carries an errno value. Callbacks must not throw.

The loop must outlive the connector. Start, cancel and destruction belong to
the IO thread; destruction cancels silently, while `cancel()` notifies the
callback. Immediate success/failure may invoke the callback inside `start()`.
Writable events are verified using `SO_ERROR`; timers use a weak reference and
completed connections ignore later events. DNS, reconnect and cross-thread
connector control are not provided yet.

`connector` tests a real loopback TCP Protobuf request/response through
Connector, Connection, RequestManager and MessageDispatcher, plus refusal on a
bound non-listening port. Timeout timing depends on network behavior and is
not covered by this local integration test.
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

Requires CMake 3.20 or later, a C++20 compiler, and a native build tool
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
