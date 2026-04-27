//
// Created by mete on 14.04.2026.
//

#include "../include/Route.h"
#include "../include/Logger.h"
#include <stdlib.h>

void free_router(struct Router *router)
{
    if (!router) return;

    for (int i = 0; i < router->count; i++)
    {
        if (router->routes[i].uri)
        {
            free(router->routes[i].uri);
            router->routes[i].uri = NULL;
        }
    }
    LOG_INFO("Router cleaned up: %d routes freed", router->count);
    router->count = 0;
}
