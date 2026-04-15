//
// Created by mete on 14.04.2026.
//

#include "../../http-server/HTTPServer.h"
#include <stdio.h>

char *render_template();

char *example(struct HTTPServer *server, struct HTTPRequest *request)
{
    (void)server;
    (void)request;
    return render_template();
}

char *render_template()
{
    return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Hello!</h1></body></html>";
}

int main()
{
    struct HTTPServer server = HTTPServer_constructor();
    server.register_routes(&server, example, "/example", 2, 0, 1);
    server.launch(&server);
    return 0;
}
