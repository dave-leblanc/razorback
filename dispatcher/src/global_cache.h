#ifndef RAZORBACK_GLOBAL_CACHE_H
#define RAZORBACK_GLOBAL_CACHE_H

#include <razorback/types.h>
#include <razorback/log.h>

typedef enum
{
    GOODHASH,
    BADHASH,
    URL,
    ALL
} CacheType;

typedef enum
{
    FULL
} ClearMethod;

Lookup_Result checkGlobalEntry (uint8_t *key, uint32_t size, uint32_t *sfflags, uint32_t *entflags, CacheType type);
Lookup_Result addGlobalEntry (uint8_t *key, uint32_t size, uint32_t sfflags, uint32_t entflags, CacheType type);
Lookup_Result updateGlobalEntry (uint8_t *key, uint32_t size, uint32_t sfflags, uint32_t entflags, CacheType type);
Lookup_Result removeGlobalEntry (uint8_t *key, uint32_t size, CacheType type);
Lookup_Result clearGlobalEntry (CacheType type, ClearMethod method);
uint32_t Global_Cache_Initialize ();

void GlobalCache_Lock (void);
void GlobalCache_Unlock (void);

#endif
