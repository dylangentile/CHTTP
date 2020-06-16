#include "CHTTP/hash_map.h"
#include <stdlib.h>
#include <assert.h>

#define CHTTP_HASHMAP_PRIME 15485863

uint64_t
CHTTP_hash(const void* str, uint32_t len)
{
    uint64_t result = 1;

    assert(sizeof(uint8_t) == 1 && "uint8_t isn't 1 byte!");

    for(const uint8_t* ptr = str; ptr != (const uint8_t*)str + len; ptr++)
    {
        result *= *ptr; //supposed to roll over
    }

    return result%CHTTP_HASHMAP_PRIME;
}



bool byteCompare(const void* a, const void* b, uint32_t len)
{
    for(const uint8_t* it1 = a, *it2 = b; it1 != (const uint8_t*)a + len; it1++, it2++)
    {
        if(*it1 != *it2)
            return false;
    }
    return true;
}


CHTTP_Bucket*
CHTTP_hash_map_find(CHTTP_hash_map* map, const void* str, uint32_t len)
{
    uint64_t offset = CHTTP_hash(str, len) % map->arraySize;
    CHTTP_Bucket* bucket = map->array + offset;

    if(bucket->data == NULL)
    {
        return NULL;
    }

    while(bucket->keySize != len)
    {
        if(bucket->other == NULL)
        {
            return NULL;
        }

        bucket = bucket->other;
    }

    if(!byteCompare(str, bucket->key, len))
    {
       return NULL;
    }
    return bucket;
}

CHTTP_Bucket*
CHTTP_hash_map_insert(CHTTP_hash_map* map, const void* key, uint32_t keylen, const void* value, uint32_t valuelen)
{
    uint64_t offset = CHTTP_hash(key, keylen) % map->arraySize;

    CHTTP_Bucket* bucket = map->array + offset;
    while(bucket->data != NULL)
    {
        if(bucket->keySize == keylen)
        {
            if(byteCompare(bucket->key, key, keylen))
            {
               return bucket;
            }
        }

        if(bucket->other != NULL)
        {
            return bucket;
        }

        bucket->other = calloc(1, sizeof(CHTTP_Bucket));
        bucket = bucket->other;

    }

    bucket->data = value;
    bucket->dataSize = valuelen;
    bucket->key = key;
    bucket->keySize = keylen;
    map->itemCount++;
    return bucket;
}


CHTTP_hash_map*
CHTTP_hash_map_allocate(uint32_t size)
{

    CHTTP_hash_map* map = calloc(1, sizeof(CHTTP_hash_map));
    map->array = calloc(size, sizeof(CHTTP_Bucket));
    map->arraySize = size;

    return map;
}

void
CHTTP_hash_map_delete(CHTTP_hash_map* map)
{
    for(CHTTP_Bucket* bucket = map->array; bucket != map->array + map->arraySize; bucket++)
    {
        CHTTP_Bucket* other = bucket->other;
        while(other != NULL)
        {
            CHTTP_Bucket* freeme = other;
            other = freeme->other;
            free(freeme);
        }
    }

    if(map != NULL)
    {
        free(map->array);
        free(map);
    }

}
