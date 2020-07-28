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
void poll_loop(CHTTP_server*);
#endif


//typedef void (*)(CHTTP_server*, CHTTP_request*) locationHandleType;


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

CHTTP_server*
CHTTP_server_allocate(uint16_t port)
{
    CHTTP_server* server = calloc(1, sizeof(CHTTP_server));
    server->port = port;
    server->handle_map = CHTTP_hash_map_allocate(1024);

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
CHTTP_server_delete(CHTTP_server* server)
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
CHTTP_addHandle(CHTTP_server* server, const char* location, void (*handle)(CHTTP_server*, CHTTP_request*, CHTTP_response*))
{
    CHTTP_hash_map_insert(server->handle_map, location, strlen(location),
                          handle, sizeof(void (*)(CHTTP_server*, CHTTP_request*, CHTTP_response*)));
}
#pragma clang diagnostic pop


void
CHTTP_runServer(CHTTP_server* server)
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
    const char response[] = "HTTP/1.1 400 Bad Request\r\nServer: CHTTP\r\nContent-Encoding: identity\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 12\r\nConnection: close\r\n\r\nBad Request!";
    CHTTP_socket_send(sock, response, sizeof(response));
}

bool fetch_block(char* buf, uint32_t bufsize, const char** ptr, char terminator)
{
    uint32_t bufloc = 0;
    while(**ptr != '\0' && **ptr != terminator)
    {
        buf[bufloc++] = **ptr;
        (*ptr)++;

        if(bufloc == bufsize)
        {
            return false;
        }
    }

    if(**ptr == '\0')
    {
        return false;
    }

    buf[bufloc++] = '\0';
    return true;
}

void
handle_404_not_found(CHTTP_socket* sock, CHTTP_request* req)
{
    const char format[] = "HTTP/1.1 404 Not Found\r\nServer: CHTTP\r\nContent-Encoding: identity\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %lu\r\nConnection: close\r\n\r\nCould Not find location: %s";

    uint32_t len = snprintf(NULL, 0, format, strlen(req->location)+25, req->location);
    char* response = calloc(len + 1, sizeof(char));

    snprintf(response, len + 1, format, strlen(req->location)+25, req->location);
    CHTTP_socket_send(sock, response, len + 1);

}





void
handle_request(CHTTP_server* server, CHTTP_socket* sock, const char* msg, uint32_t msglen)
{
    const int bufsize = 512;
    char buf[bufsize];
    uint32_t bufloc = 0;

    const char* ptr = msg;
    if(!fetch_block(buf, bufsize, &ptr, ' '))
    {
        bad_request_handler(sock);
        return;
    }
    ptr++;

    CHTTP_Bucket* bucket = CHTTP_hash_map_find(methodMap, (const char*)buf, strlen((const char*)buf)); //don't include null terminator!

    if(bucket == NULL)
    {
        fprintf(stderr, "invalid header: %s\n", (const char*)buf);
        //exit(1);
    }

    CHTTP_request req;
    memset(&req, 0, sizeof(CHTTP_request));
    req.method = *(HTTP_Method*)bucket->data;
    req.header_map = CHTTP_header_map_allocate();
    req.GET = CHTTP_hash_map_allocate(128);

    memset(buf, 0, sizeof(char) * bufsize);

    if(!fetch_block(buf, bufsize, &ptr, ' '))
    {
        bad_request_handler(sock);
        return;
    }
    ptr++;

    char* location = calloc(strlen(buf) + 1, sizeof(char));
    strcpy(location, buf);

    req.location = (const char*)location;

    while(!fetch_block(buf, bufsize, &ptr, '\n'))
    {
        bad_request_handler(sock);
        goto defer;
    }
    ptr++;

    if(strcmp((const char*) buf, "HTTP/1.1\r") != 0)
    {
        bad_request_handler(sock);
        goto defer;
    }

    //printf("good!\n");
    {
        char buf2[bufsize];
        memset(buf2, 0, bufsize);

        while(*ptr != '\r' && *(ptr + 1) != '\n')
        {
            while(!fetch_block(buf, bufsize, &ptr, ':'))
            {
                bad_request_handler(sock);
                goto defer;
            }
            ptr++;

            while(*ptr == ' ') ptr++;

            while(!fetch_block(buf2, bufsize, &ptr, '\n'))
            {
                bad_request_handler(sock);
                goto defer;
            }
            ptr++;

            char* ending = &buf2[strlen((const char*)buf2) - 1];
            if(*ending == '\r') *ending = '\0'; //overwrite \r
            CHTTP_header_map_insert(req.header_map, (const char*)buf, (const char*)buf2);
        }
    }
    ptr += 2;

    CHTTP_response resp;
    memset(&resp, 0, sizeof(CHTTP_response));
    resp.header_map = CHTTP_header_map_allocate();
    resp.freeContent = true;

    void (*handle)(CHTTP_server*, CHTTP_request*, CHTTP_response*);

    bucket = CHTTP_hash_map_find(server->handle_map, req.location, strlen(req.location));
    if(bucket == NULL)
    {
        handle_404_not_found(sock, &req);
    }
    else
    {
        handle = bucket->data;
        (*handle)(server, &req, &resp);

        int len = snprintf(NULL, 0, "%lu", resp.contentLength);
        char* theLength = calloc(len + 1, sizeof(char));
        snprintf(theLength, len + 1, "%lu", resp.contentLength);

        const char* code = "HTTP/1.1 200 OK\r\n";

        CHTTP_header_map_insert(resp.header_map, "Server", "CHTTP");
        CHTTP_header_map_insert(resp.header_map, "Content-Encoding", "identity");
        CHTTP_header_map_insert(resp.header_map, "Content-Type", "text/html; charset=utf-8");
        CHTTP_header_map_insert(resp.header_map, "Content-Length", (const char*)theLength);
        CHTTP_header_map_insert(resp.header_map, "Connection", "close");
        char* header_str = CHTTP_header_map_generate(resp.header_map);

        uint32_t n = snprintf(NULL, 0, "%s%s\r\n%s", code, (const char*)header_str, (const char*)resp.content);
        char* responseBuffer = calloc(n + 1, sizeof(char));
        snprintf(responseBuffer, n+1, "%s%s\r\n%s", code, (const char*)header_str, (const char*)resp.content);

        CHTTP_socket_send(sock, responseBuffer, n+1);

        free(header_str);
        free(theLength);

    }







defer:
    free(location);
    CHTTP_header_map_delete(req.header_map);
    CHTTP_hash_map_delete(req.GET);
    CHTTP_header_map_delete(resp.header_map);
    if(resp.freeContent) free(resp.content);

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

void poll_loop(CHTTP_server* server)
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
                        handle_request(server, &(socketArray[i]), (const char*)req, reqlen);
                    }
                }
            }
        }


    }
}
#endif
