cmake_minimum_required(VERSION 4.2)
project(http_server C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(http_server
        Server.c
        Server.h
        test.c
        HTTPRequest.c
        HTTPRequest.h
        HTTPServer.c
        HTTPServer.h
        Route.c
        Route.h
        ThreadPool.c
        ThreadPool.h
        Client.c
        Client.h
        Test/main.c
)
