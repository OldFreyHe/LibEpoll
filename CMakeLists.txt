cmake_minimum_required(VERSION 4.3)
project(LibEPoll)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp)
message(${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp)
find_package(Threads REQUIRED)
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

target_link_libraries("EPollServer" PRIVATE epoll_core)


#target_link_libraries(epoll_server PUBLIC epoll_core)
#target_link_libraries(epoll_client PUBLIC epoll_core)