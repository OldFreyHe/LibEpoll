cmake_minimum_required(VERSION 3.20)
project(LibEPoll)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp)
message(${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED MODULE)

# Keep business payload schemas separate from the transport library.
add_library(epoll_example_proto STATIC proto/echo.proto)
protobuf_generate(TARGET epoll_example_proto LANGUAGE cpp APPEND_PATH)
target_include_directories(epoll_example_proto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(epoll_example_proto PUBLIC protobuf::libprotobuf)

add_library(epoll_echo_example STATIC examples/EchoHandler.cpp)
target_include_directories(epoll_echo_example PUBLIC examples include)
target_link_libraries(epoll_echo_example PUBLIC epoll_example_proto)
#file(GLOB_RECURSE SERVER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp)
#file(GLOB_RECURSE CLIENT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp)

add_library(epoll_core SHARED ${CORE_SOURCES}
        include/Connection.h
        src/core/Connection.cpp
        include/Acceptor.h
        src/core/Acceptor.cpp
        include/Channel.h
        src/core/Channel.cpp
        include/ConnectionManager.h
        include/EPollServer.h
        src/core/EPollServer.cpp
        include/MessageParser.h
        src/core/MessageParser.cpp)
#add_library(epoll_server SHARED ${SERVER_SOURCES})
#add_library(epoll_client SHARED ${CLIENT_SOURCES})

add_executable("EPollServer" main.cpp)

target_include_directories("EPollServer" PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(epoll_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(epoll_core PUBLIC Threads::Threads)

target_link_libraries("EPollServer" PRIVATE epoll_core epoll_echo_example)

add_executable(EPollClient examples/EchoClient.cpp)
target_link_libraries(EPollClient PRIVATE epoll_core epoll_example_proto)

include(CTest)
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(server_shutdown_test tests/ServerShutdownTest.cpp)
    target_link_libraries(server_shutdown_test PRIVATE epoll_core)
    add_test(NAME server_shutdown COMMAND server_shutdown_test)
    set_tests_properties(server_shutdown PROPERTIES TIMEOUT 15)
    add_executable(worker_thread_test tests/WorkerThreadTest.cpp)
    target_link_libraries(worker_thread_test PRIVATE epoll_core)
    add_test(NAME worker_thread COMMAND worker_thread_test)
    set_tests_properties(worker_thread PROPERTIES TIMEOUT 15)
    add_executable(worker_pool_test tests/WorkerPoolTest.cpp)
    target_link_libraries(worker_pool_test PRIVATE epoll_core)
    add_test(NAME worker_pool COMMAND worker_pool_test)
    set_tests_properties(worker_pool PROPERTIES TIMEOUT 15)
    add_executable(connector_test tests/ConnectorTest.cpp)
    target_include_directories(connector_test PRIVATE examples)
    target_link_libraries(connector_test PRIVATE epoll_core epoll_example_proto)
    add_test(NAME connector COMMAND connector_test)
    set_tests_properties(connector PROPERTIES TIMEOUT 15)
    add_executable(request_manager_test tests/RequestManagerTest.cpp)
    target_link_libraries(request_manager_test PRIVATE epoll_core)
    add_test(NAME request_manager COMMAND request_manager_test)
    set_tests_properties(request_manager PROPERTIES TIMEOUT 15)
    add_executable(message_dispatcher_test tests/MessageDispatcherTest.cpp)
    target_link_libraries(message_dispatcher_test PRIVATE epoll_core epoll_example_proto)
    add_test(NAME message_dispatcher COMMAND message_dispatcher_test)
    set_tests_properties(message_dispatcher PROPERTIES TIMEOUT 15)
    add_executable(protobuf_echo_test tests/ProtobufEchoTest.cpp)
    target_link_libraries(protobuf_echo_test PRIVATE epoll_core epoll_echo_example)
    add_test(NAME protobuf_echo COMMAND protobuf_echo_test)
    set_tests_properties(protobuf_echo PROPERTIES TIMEOUT 15)
    add_executable(connection_send_test tests/ConnectionSendTest.cpp)
    target_link_libraries(connection_send_test PRIVATE epoll_core)
    add_test(NAME connection_send COMMAND connection_send_test)
    set_tests_properties(connection_send PROPERTIES TIMEOUT 15)
endif()


#target_link_libraries(epoll_server PUBLIC epoll_core)
#target_link_libraries(epoll_client PUBLIC epoll_core)
