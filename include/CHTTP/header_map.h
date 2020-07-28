
#ifndef CHTTP_header_map_h
#define CHTTP_header_map_h

#include "CHTTP/hash_map.h"

/*
  header map will insert a copy into the hashmap
  which it will delete upon _delete(*)
 */

typedef struct
{
    CHTTP_hash_map* theHashMap;
}CHTTP_header_map;

CHTTP_header_map* CHTTP_header_map_allocate();
void CHTTP_header_map_delete(CHTTP_header_map*);

//inserts by copy
void CHTTP_header_map_insert(CHTTP_header_map*, const char* key, const char* val);

//returns pointer to Bucket data, deleting the header_map will delete this.
const char* CHTTP_header_map_find(CHTTP_header_map*, const char* key);
char* CHTTP_header_map_generate(CHTTP_header_map*);



#endif //CHTTP_header_map_h
