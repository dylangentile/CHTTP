#include "CHTTP/CHTTP.h"

#include <stdlib.h>
#include <stdio.h>

const char hello[] = "Hello World!";

void root(CHTTP_server* server, CHTTP_request* req, CHTTP_response* resp)
{
    resp->content = hello;
    resp->contentLength = sizeof(hello);
    resp->freeContent = false;
}


int main(void)
{
    //  CHTTP_http_server myServer;
    //myServer.port = 8080;
    /*  CHTTP_socket mySocket = CHTTP_socket_open(8080);
    CHTTP_socket_listen(&mySocket);

    while(1)
    {
        CHTTP_socket connection = CHTTP_socket_accept(&mySocket);

        char* msg = NULL;
        uint32_t msglen = 0;

        CHTTP_socket_read(&connection, &msg, &msglen);
        fprintf(stderr, "%s\n", (const char*)msg);
        CHTTP_socket_close(&connection);
        free(msg);
        }*/

    CHTTP_server* server = CHTTP_server_allocate(8080);
    CHTTP_addHandle(server, "/", &root);


    CHTTP_runServer(server);


    printf("Exiting...");
    CHTTP_server_delete(server);

    return 0;
}
