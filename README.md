# LibEpoll
A Network Communication Dynamic Library Implemented Using EPoll

## Run tests on Linux

Requires CMake 4.3 or later, a C++20 compiler, and a native build tool
(such as Make or Ninja).

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
