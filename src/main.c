#include "CHTTP/CHTTP.h"
#include <stdlib.h>
//#include "CHTTP/socket.h"

/*
void genericReply(CHTTP_http_server* server, CHTTP_request* req)
{
    CHTTP_response myResponse;

    CHTTP_respond(server, req, &myResponse);
}
*/
#include <stdio.h>

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

    CHTTP_http_server* server = CHTTP_http_server_allocate(8080);
    CHTTP_runServer(server);

    printf("Exiting...");
    CHTTP_http_server_delete(server);

    return 0;
}
