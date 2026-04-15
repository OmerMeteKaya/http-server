//
// Created by mete on 14.04.2026.
//

#include "HTTPRequest.h"

struct HTTPRequest
{
    int x;
};
struct HTTPServer
{
    int y;
    void (*register_routes)(struct HTTPServer *server,char *(*route_function)(struct HTTPServer *server,struct HTTPRequest *request),
        char *uri,int method_num,...);
};
char *render_template();

char *example(struct HTTPServer *server,struct HTTPRequest *request)
{
    return render_template();
};

int main()
{
    struct HTTPServer server;
    server.register_routes(&server,example,"/example",2,0,1);
    server.launch();
    return 0;
}