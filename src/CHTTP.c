#include "CHTTP/CHTTP.h"
#include "CHTTP/socket.h"

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(BSD)

#elif defined(LINUX)

#else
#include <poll.h>
void poll_loop(CHTTP_http_server*);
#endif



bool gotInterrupt = false;
CHTTP_hash_map* methodMap = NULL;
const char* methodStrList[] =
{
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "CONNECT",
    "OPTIONS",
    "TRACE",
    "PATCH"
};

const HTTP_Method methodEnumList[] =
{
    kMethod_GET,
    kMethod_HEAD,
    kMethod_POST,
    kMethod_PUT,
    kMethod_DELETE,
    kMethod_CONNECT,
    kMethod_OPTIONS,
    kMethod_TRACE,
    kMethod_PATCH
};



void sigint_handler(int s)
{
    gotInterrupt = true;
}

CHTTP_http_server*
CHTTP_http_server_allocate(uint16_t port)
{
    CHTTP_http_server* server = calloc(1, sizeof(CHTTP_http_server));
    server->port = port;
    server->handle_map = calloc(1, sizeof(CHTTP_hash_map));

    if(methodMap == NULL)
    {
        methodMap = CHTTP_hash_map_allocate(9);
        for(int i = 0; i < 9; i++)
        {
            CHTTP_Bucket* bucket = CHTTP_hash_map_insert(methodMap,
                                                         methodStrList[i], strlen(methodStrList[i]) * sizeof(char),
                                                         &methodEnumList[i], sizeof(HTTP_Method));
            if(bucket == NULL)
            {
                perror("hash_map failed to insert");
                exit(1);
            }
        }
    }


    return server;

}

void
CHTTP_http_server_delete(CHTTP_http_server* server)
{
    if(server != NULL)
    {
        CHTTP_hash_map_delete(server->handle_map);
        free(server);
    }
}



#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
void
CHTTP_addHandle(CHTTP_http_server* server, const char* location, void (*handle)(CHTTP_http_server*, CHTTP_request*))
{
    CHTTP_hash_map_insert(server->handle_map, location, strlen(location),
                          handle, sizeof(void (*)(CHTTP_http_server*, CHTTP_request*)));
}
#pragma clang diagnostic pop


void
CHTTP_runServer(CHTTP_http_server* server)
{
    CHTTP_socket thesock = CHTTP_socket_open(server->port);
    server->sock = (void*)&thesock;

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        fprintf(stderr, "failed to create sigint handler!\n");
        exit(1);
    }

#if defined(BSD) //includes macOs
//uses kqueue
#error "kqueue not implemented!"
#elif defined(LINUX)
//uses epoll
#error "epoll not implemented!"
#else

    poll_loop(server);
#endif


    CHTTP_socket_close(&thesock);
    server->sock = NULL;
}

void
bad_request_handler(CHTTP_socket* sock)
{
    const char response[] = "HTTP/1.1 400 Bad Request\r\nServer: CHTTP\r\n";
    CHTTP_socket_send(sock, response, sizeof(response));
}

void
handle_request(CHTTP_socket* sock, const char* msg, uint32_t msglen)
{
    const int bufsize = 512;
    char buf[bufsize];
    uint32_t bufloc = 0;
    uint32_t count = 0;

    const char* ptr = msg;
    while(*ptr != '\0' && *ptr == ' ')
    {
        ptr++;
    }


    while(*ptr != '\0' && *ptr != ' ')
    {
        buf[bufloc++] = *ptr;
        ptr++;
        count++;
    }

    buf[bufloc++] = '\0';

    if(*ptr == '\0')
    {
        bad_request_handler(sock);
        return;
    }

    CHTTP_Bucket* bucket = CHTTP_hash_map_find(methodMap, (const char*)buf, count);

    if(bucket == NULL)
    {
        fprintf(stderr, "invalid header: %s\n", (const char*)buf);
        //exit(1);
    }

    CHTTP_request req;
    memset(&req, 0, sizeof(CHTTP_request));
    req.method = *(HTTP_Method*)bucket->data;
    req.header_map = CHTTP_header_map_allocate();

    memset(buf, 0, sizeof(char) * bufsize);
    bufloc = 0;

    bad_request_handler(sock);



}




#if defined(BSD)

#elif defined(LINUX)

#else

void removeFromPolling(int index, CHTTP_socket* socket_array, struct pollfd* pfds, int* fd_count)
{
    printf("disconnection on %d\n", socket_array[index].fd);

    CHTTP_socket_close(&socket_array[index]);
    memset(&socket_array[index], 0, sizeof(CHTTP_socket));

    pfds[index].fd = pfds[*fd_count - 1].fd; //overwrite
    pfds[index].events = POLLIN;

    socket_array[index] = socket_array[*fd_count - 1];


    (*fd_count)--;
}

void addSocketToPoll(CHTTP_socket* addme, CHTTP_socket** socket_array, struct pollfd** pfds, int* array_size, int* fd_count)
{
    printf("connection on %d\n", addme->fd);

    if(*array_size == *fd_count)
    {
        *array_size *= 2;

        *socket_array = realloc(*socket_array, sizeof(CHTTP_socket) * (*array_size));
        *pfds = realloc(*pfds, sizeof(struct pollfd) * (*array_size));
        for(int i = *fd_count; i < *array_size; i++)
        {
            (*pfds)[i].fd = -1;
        }
    }

    (*socket_array)[*fd_count] = *addme;
    (*pfds)[*fd_count].fd = addme->fd;
    (*pfds)[*fd_count].events = POLLIN;

    (*fd_count)++;
}

void poll_loop(CHTTP_http_server* server)
{
    CHTTP_socket* listener = server->sock;
    CHTTP_socket_listen(listener);


    int fd_array_size = 4;
    int fd_count = 1; //main listener

    struct pollfd* pfds = calloc(fd_array_size, sizeof(struct pollfd));
    CHTTP_socket* socketArray = calloc(fd_array_size, sizeof(CHTTP_socket));

    for(int i = 0; i < fd_array_size; i++)
    {
        pfds[i].fd = -1;
    }

    socketArray[0] = *listener;
    pfds[0].fd = listener->fd;
    pfds[0].events = POLLIN;


    while(!gotInterrupt)
    {
        int ready_count = poll(pfds, fd_count, -1);

        if(ready_count == -1)
        {
            perror("poll");
            exit(1);
        }

        for(int i = 0; i < fd_count; i++)
        {
            if(pfds[i].revents & POLLIN)
            {
                if(pfds[i].fd == listener->fd)
                {
                    CHTTP_socket sock = CHTTP_socket_accept(listener);
                    addSocketToPoll(&sock, &socketArray, &pfds, &fd_array_size, &fd_count);
                }
                else
                {
                    char* req = NULL;
                    uint32_t reqlen = 0;
                    CHTTP_socket_read(&(socketArray[i]), &req, &reqlen);
                    if(reqlen == 0)
                    {
                        //closes the socket for us
                        removeFromPolling(i, socketArray, pfds, &fd_count);
                    }
                    else
                    {
                        handle_request(&(socketArray[i]), (const char*)req, reqlen);
                    }
                }
            }
        }


    }
}
#endif
