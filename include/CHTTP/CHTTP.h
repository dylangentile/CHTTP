#ifndef CHTTP_CHTTP_H
#define CHTTP_CHTTP_H

#include <stdint.h>
#include "CHTTP/header_map.h"

typedef enum
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
}HTTP_Method;



typedef struct CHTTP_server
{
//public:
    uint16_t port;

//private: //these are pretend since it is 'C'
    void* sock;
    CHTTP_hash_map* handle_map;
}CHTTP_server;


typedef struct CHTTP_request
{
    CHTTP_header_map* header_map;
    CHTTP_hash_map* GET;

    HTTP_Method method;
    const char* location;
    const char* content;
}CHTTP_request;

typedef struct CHTTP_response
{
    CHTTP_header_map* header_map;
    void* content;
    uint32_t contentLength;
    bool freeContent;
}CHTTP_response;




void CHTTP_addHandle(CHTTP_server*, const char*, void (*)(CHTTP_server*, CHTTP_request*, CHTTP_response*));
void CHTTP_runServer(CHTTP_server*);

CHTTP_server* CHTTP_server_allocate(uint16_t);
void CHTTP_server_delete(CHTTP_server*);





#endif //CHTTP_CHTTP_H
