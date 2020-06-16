#ifndef CHTTP_hash_map_h
#define CHTTP_hash_map_h

#include <stdint.h>
#include <stdbool.h>


uint64_t CHTTP_hash(const void*, uint32_t);

typedef struct CHTTP_Bucket
{
    struct CHTTP_Bucket* other;
    const void* key;
    uint32_t keySize;

    const void* data;
    uint32_t dataSize;
}CHTTP_Bucket;


typedef struct CHTTP_hash_map
{
    CHTTP_Bucket* array;
    uint32_t arraySize;
    uint32_t itemCount;
}CHTTP_hash_map;

CHTTP_Bucket*   CHTTP_hash_map_find  (CHTTP_hash_map*, const void*, uint32_t);
CHTTP_Bucket*   CHTTP_hash_map_insert(CHTTP_hash_map*, const void*, uint32_t, const void*, uint32_t);
CHTTP_hash_map* CHTTP_hash_map_allocate(uint32_t size);
void CHTTP_hash_map_delete(CHTTP_hash_map*);



#endif //CHTTP_hash_map_h
