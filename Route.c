//
// Created by mete on 14.04.2026.
//

#include "Route.h"
#include <stdlib.h>

void free_router(struct Router *router)
{
    for (int i = 0; i < router->count; i++)
    {
        if (router->routes[i].uri)
        {
            free(router->routes[i].uri);
            router->routes[i].uri = NULL;
        }
    }
    router->count = 0;
}
