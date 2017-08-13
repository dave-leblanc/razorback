#ifndef RAZORBACK_LOCAL_CACHE_H
#define RAZORBACK_LOCAL_CACHE_H

#include <razorback/types.h>
#include <razorback/lock.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum
{
    LT_T1,
    LT_T2,
    LT_B1,
    LT_B2,
    LT_NONE
} LISTTYPE;

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

typedef struct _ENTRY
{
    struct _ENTRY *next;
    struct _ENTRY *prev;
    LISTTYPE listtype;
    unsigned size;
	uint8_t *key;
	uint32_t sfflags;
	uint32_t entflags;
} ENTRY;

typedef struct _CACHE
{
    unsigned size;
    unsigned entries;
    unsigned listSize[LT_NONE];
    double target;
    struct Mutex * cachemutex;
    ENTRY *LRU[LT_NONE];
    ENTRY *MRU[LT_NONE];
    ENTRY *entrylist;
} CACHE;

Lookup_Result checkLocalEntry(uint8_t *key, uint32_t size, uint32_t *sfflags, uint32_t *entflags, CacheType type);
Lookup_Result addLocalEntry(uint8_t *key, uint32_t size, uint32_t sfflags, uint32_t entflags, CacheType type);
Lookup_Result updateLocalEntry(uint8_t *key, uint32_t size, uint32_t sfflags, uint32_t entflags, CacheType type);
Lookup_Result removeLocalEntry(uint8_t *key, uint32_t size, CacheType type);
Lookup_Result clearLocalEntry(CacheType type, ClearMethod method);
#ifdef __cplusplus
}
#endif
#endif
