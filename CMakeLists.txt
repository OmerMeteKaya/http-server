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
)

# To build with Test/main.c instead of test.c, uncomment below and comment out test.c above:
# add_executable(http_server_test
#         Server.c
#         HTTPRequest.c
#         HTTPServer.c
#         Route.c
#         ThreadPool.c
#         Client.c
#         Test/main.c
# )
