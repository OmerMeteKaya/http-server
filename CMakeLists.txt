cmake_minimum_required(VERSION 4.2)
project(http_server C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(http_server
        Server.c
        Server.h
        test.c)
