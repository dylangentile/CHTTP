#ifndef CHTTP_socket_h
#define CHTTP_socket_h

#include <stdint.h>


typedef struct CHTTP_socket
{
    int fd;
    const char* port;
    uint16_t portNum;

    const char* addrstr;

}CHTTP_socket;




CHTTP_socket CHTTP_socket_open(uint16_t);
void CHTTP_socket_close(CHTTP_socket*);

void CHTTP_socket_listen(CHTTP_socket*);
CHTTP_socket CHTTP_socket_accept(CHTTP_socket*);

int CHTTP_socket_send(CHTTP_socket*, const void*, uint32_t);
void CHTTP_socket_read(CHTTP_socket*, char**, uint32_t*);



#endif //CHTTP_socket_h
