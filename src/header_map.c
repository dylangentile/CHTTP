#include "CHTTP/header_map.h"

#include <stdlib.h>
#include <string.h>

CHTTP_header_map*
CHTTP_header_map_allocate()
{
    CHTTP_header_map* headers = calloc(1, sizeof(CHTTP_header_map));
    headers->theHashMap = CHTTP_hash_map_allocate(128);
    return headers;
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

    //calloc will give us the null terminator with the zero out
    char* keycopy = calloc(keylen + 1, sizeof(char));
    char* valcopy = calloc(vallen + 1, sizeof(char));

    strcpy(keycopy, key);
    strcpy(valcopy, val);

    CHTTP_Bucket* bucket = CHTTP_hash_map_find(header_map->theHashMap, keycopy, keylen);
    if(bucket != NULL)
    {
        //we can safely cast away const since the header must have been allocated in here
        char* originalData = (char*)bucket->data;
        uint32_t offset = bucket->dataSize;
        bucket->dataSize = bucket->dataSize + vallen + 3;  //+3 == ',' ' ' '\0'
        originalData = realloc(originalData, sizeof(char)*(bucket->dataSize));
        originalData[offset] = ',';
        originalData[offset + 1] = ' ';
        strcpy(&originalData[offset + 2], (const char*)valcopy);

        free(keycopy);
        free(valcopy);
    }
    else
    {
        CHTTP_hash_map_insert(header_map->theHashMap, keycopy, keylen, valcopy, vallen);
    }
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

char*
CHTTP_header_map_generate(CHTTP_header_map* header_map)
{
    uint32_t str_size = 2048;
    uint32_t used_bytes = 0;
    char* str = calloc(str_size, sizeof(char));

    int str_count = 0;
    CHTTP_Bucket* array = header_map->theHashMap->array;
    for(; array != header_map->theHashMap->array + header_map->theHashMap->arraySize; array++)
    {
        CHTTP_Bucket* theBucket = array;
        while(theBucket != NULL && theBucket->key != NULL)
        {
            int n = snprintf(NULL, 0, "%s: %s\r\n", (const char*)theBucket->key, (const char*)theBucket->data);
            while(used_bytes + n + 1 >= str_size)
            {
                str_size *= 2;
                str = realloc(str, sizeof(char)*str_size);
            }

            snprintf(&str[used_bytes], str_size - used_bytes + 1, "%s: %s\r\n", (const char*)theBucket->key, (const char*)theBucket->data);
            used_bytes += n;

            theBucket = theBucket->other;
        }
    }

    return str;

}
