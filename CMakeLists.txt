cmake_minimum_required(VERSION 4.2)
project(http_server C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(http_server
        src/Server.c
        include/Server.h
        test/test.c
        src/HTTPRequest.c
        include/HTTPRequest.h
        src/HTTPResponse.c
        include/HTTPResponse.h
        src/HTTPServer.c
        include/HTTPServer.h
        src/Route.c
        include/Route.h
        src/ThreadPool.c
        include/ThreadPool.h
        src/Client.c
        include/Client.h
        src/Logger.c
        include/Logger.h
)
