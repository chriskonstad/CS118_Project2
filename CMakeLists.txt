cmake_minimum_required(VERSION 3.3)
project(CS118_Project2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    client_src/client.c
    lib/buffer.c
    lib/buffer.h
    lib/packet.h
    lib/rdtp.c
    lib/rdtp.h
    lib/test.c
    server_src/server.c)

add_executable(CS118_Project2 ${SOURCE_FILES})