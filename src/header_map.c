#include "CHTTP/header_map.h"

#include <stdlib.h>
#include <string.h>

CHTTP_header_map*
CHTTP_header_map_allocate()
{
    CHTTP_header_map* headers = calloc(1, sizeof(CHTTP_header_map));
    headers->theHashMap = CHTTP_hash_map_allocate(128);
}

void
CHTTP_header_map_delete(CHTTP_header_map* headers)
{
    if(headers != NULL)
    {

        CHTTP_Bucket* array = headers->theHashMap->array;
        for(; array != headers->theHashMap->array + headers->theHashMap->arraySize; array++)
        {
            if(array->key != NULL)
            {
                free(array->key);
                free(array->data);
                CHTTP_Bucket* other = array->other;
                while(other != NULL)
                {
                    free(other->key);
                    free(other->data);
                    other = other->other;
                    //other buckets get free-d by hash_map_destroy
                }
            }

        }


        CHTTP_hash_map_delete(headers->theHashMap);
        free(headers);
    }
}

void
CHTTP_header_map_insert(CHTTP_header_map* header_map, const char* key, const char* val)
{
    uint32_t keylen = strlen(key);
    uint32_t vallen = strlen(val);

    char* keycopy = calloc(keylen + 1, sizeof(char));
    char* valcopy = calloc(vallen + 1, sizeof(char));

    strcpy(keycopy, key);
    strcpy(valcopy, val);

    CHTTP_hash_map_insert(header_map->theHashMap, keycopy, keylen, valcopy, vallen);

}

const char*
CHTTP_header_map_find(CHTTP_header_map* header_map, const char* key)
{
    CHTTP_Bucket* bucket = CHTTP_hash_map_find(header_map->theHashMap, key, strlen(key));

    if(bucket == NULL)
    {
        return NULL;
    }

    return (const char*)bucket->data;
}
