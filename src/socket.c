#include "CHTTP/socket.h"

/*
Learned how to do and adapted from Beej's guide.
 */


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>


#define BACKLOG 10 //number of backlogged connections allowed

void
sigchld_handler(int s)
{
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


void*
get_in_addr(struct sockaddr* sa)
{
    if(sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }


    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


CHTTP_socket
CHTTP_socket_open(uint16_t port)
{
    CHTTP_socket sock;
    memset(&sock, 0, sizeof(CHTTP_socket));

    sock.portNum = port;

    {
        int x = snprintf(0,0, "%d", sock.portNum);
        x++; //include ending '\0'
        char* pbuf = calloc(x, sizeof(char));
        snprintf(pbuf, x, "%d", sock.portNum);
        sock.port = (const char*)pbuf;
    }

    struct addrinfo hints, *servinfo, *ptr;
    int yes = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    {
        int rv = 0;
        if((rv = getaddrinfo(NULL, sock.port, &hints, &servinfo)) != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            exit(1);
        }
    }


    for(ptr = servinfo; ptr != NULL; ptr = ptr->ai_next)
    {
        if((sock.fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if(setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if(bind(sock.fd, ptr->ai_addr, ptr->ai_addrlen) == -1)
        {
            close(sock.fd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if(ptr == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    return sock;
/*
    if(listen(socket.fd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }
*/

/*
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
*/

}

void
CHTTP_socket_listen(CHTTP_socket* socket)
{
    if(listen(socket->fd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }
}

CHTTP_socket
CHTTP_socket_accept(CHTTP_socket* socket)
{
    struct sockaddr_storage their_addr;
    CHTTP_socket newsock;
    memset(&newsock, 0, sizeof(CHTTP_socket));

    newsock.fd = -1;

    socklen_t sin_size = sizeof(their_addr);
    while(newsock.fd == -1)
    {
        newsock.fd = accept(socket->fd, (struct sockaddr*)&their_addr, &sin_size);
    }
    char* s = calloc(INET6_ADDRSTRLEN, sizeof(char));
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, INET6_ADDRSTRLEN);
    newsock.addrstr = (const char*)s;

    return newsock;
}

void
CHTTP_socket_close(CHTTP_socket* socket)
{
    close(socket->fd);
    free((void*)socket->port);
    free((void*)socket->addrstr);
}

int
CHTTP_socket_send(CHTTP_socket* socket, const void* msg, uint32_t len)
{
    return send(socket->fd, msg, len, 0);
}

void
CHTTP_socket_read(CHTTP_socket* socket, char** data, uint32_t* datalen)
{
    uint32_t chunk = 256;
    *data = calloc(chunk, sizeof(char));
    int n = 0;
    *datalen = 1; // '\0'

    while((n = recv(socket->fd, *data + *datalen - 1, chunk-1, 0)) == chunk-1)
    {
        *datalen += n;
        if(n == -1)
        {
            perror("recv");
            exit(1);
        }
        *data = realloc(*data, sizeof(char) * (*datalen + chunk));
    }

    *datalen += n;

    if(*datalen == 1)
    {
        free(*data);
        *data = NULL;
        *datalen = 0;
    }
    else
    {
        (*data)[*datalen] = '\0';
        *data = realloc(*data, *datalen + 1);
    }
}
