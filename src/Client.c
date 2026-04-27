//
// Created by mete on 15.04.2026.
//

#include "../include/Client.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

void request(struct Client* client,char* server_ip,char* request);

struct Client client_constructor(int domain,int service,int protocol,int port,u_long interface)
{
    struct Client client;
    client.domain = domain;
    client.port = port;
    client.interface = interface;

    client.sockdf = socket(domain,service,protocol);
    client.request = request;
    return client;
}

void request(struct Client* client,char* server_ip,char* request)
{
    struct sockaddr_in server_adress;

    server_adress.sin_family = client->domain;
    server_adress.sin_port = htons(client->port);
    server_adress.sin_addr.s_addr = client->interface;

    inet_pton(client->domain,server_ip,&server_adress.sin_addr);
    connect(client->sockdf,(struct sockaddr*)&server_adress,sizeof(server_adress));
    send(client->sockdf,request,strlen(request),0);


}
