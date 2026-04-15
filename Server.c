//
// Created by mete on 11.04.2026.
//

#include "Server.h"
#include "Logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

struct Server server_constructor(int domain, int type, int protocol,
                                 u_long interface, int port, int backlog)
{
    struct Server server;
    memset(&server, 0, sizeof(server));

    server.domain = domain;
    server.type = type;
    server.protocol = protocol;
    server.backlog = backlog;
    server.port = port;
    server.interface = interface;

    memset(&server.address, 0, sizeof(server.address));
    server.address.sin_family = domain;
    server.address.sin_port = htons((uint16_t)port);
    server.address.sin_addr.s_addr = htonl((uint32_t)interface);

    server.sockdf = socket(domain, type, protocol);
    if (server.sockdf < 0) {
        LOG_ERROR("Failed to create socket: %s", strerror(errno));
        return server;
    }

    int opt = 1;
    if (setsockopt(server.sockdf, SOL_SOCKET, SO_REUSEADDR, &opt, (socklen_t)sizeof(opt)) < 0) {
        LOG_WARN("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
        // Non-fatal — continue anyway
    }

    if (bind(server.sockdf, (struct sockaddr *)&server.address,
             (socklen_t)sizeof(server.address)) < 0) {
        LOG_ERROR("Failed to bind socket on port %d: %s", port, strerror(errno));
        close(server.sockdf);
        server.sockdf = -1;
        return server;
    }

    if (listen(server.sockdf, backlog) < 0) {
        LOG_ERROR("Failed to listen on socket (backlog=%d): %s", backlog, strerror(errno));
        close(server.sockdf);
        server.sockdf = -1;
        return server;
    }

    LOG_INFO("Server socket created: port=%d, backlog=%d, fd=%d", port, backlog, server.sockdf);
    return server;
}
