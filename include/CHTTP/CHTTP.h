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



typedef struct CHTTP_http_server
{
//public:
    uint16_t port;

//private: //these are pretend since it is 'C'
    void* sock;
    CHTTP_hash_map* handle_map;
}CHTTP_http_server;


typedef struct CHTTP_request
{
    CHTTP_header_map* header_map;
    HTTP_Method method;
    const char* location;
}CHTTP_request;

typedef struct CHTTP_response
{
    CHTTP_header_map* header_map;
}CHTTP_response;




void CHTTP_addHandle(CHTTP_http_server*, const char*, void (*)(CHTTP_http_server*, CHTTP_request*));
void CHTTP_runServer(CHTTP_http_server*);
void CHTTP_respond(CHTTP_http_server*, CHTTP_request*, CHTTP_response*);

CHTTP_http_server* CHTTP_http_server_allocate(uint16_t);
void CHTTP_http_server_delete(CHTTP_http_server*);





#endif //CHTTP_CHTTP_H
