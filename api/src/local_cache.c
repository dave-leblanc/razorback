#include <string.h>
#include <stdio.h>
#ifdef _MSC_VER
#else //_MSC_VER
#include <pthread.h>
#endif //_MSC_VER
#include "local_cache.h"
#include "config.h"
#include "init.h"
#include "runtime_config.h"
#include <razorback/debug.h>
#include <razorback/log.h>

static CACHE Cache[ALL];

static ENTRY *getNewEntry (CACHE *cache);
static ENTRY *PurgeLRU(LISTTYPE, CACHE *);
static void AddMRU(ENTRY *, LISTTYPE, CACHE *);
static ENTRY *FindMRU(LISTTYPE, CACHE *);
static ENTRY *FindLRU(LISTTYPE, CACHE *);
static ENTRY *FindEntry(ENTRY *, CACHE *);
static void removeEntry(ENTRY *, CACHE *);
static ENTRY *replace(CACHE *);
static void destroyEntry(ENTRY *);

#ifdef _MSC_VER
#undef max
#undef min
#endif
static double max(double, unsigned);
static double min(double, unsigned);



Lookup_Result checkLocalEntry(uint8_t *key, uint32_t size, uint32_t *sfflags, uint32_t *entflags, CacheType type) {
    
	CACHE *cache;
	ENTRY entry;
	ENTRY *cachehit;

    //Set Cache pointer based on CACHETYPE value    
    if (type >= ALL) {
        rzb_log(LOG_ERR, "%s: Invalid CacheType passed as argument", __func__); 
        return R_ERROR;
	}

	cache = &Cache[type];

    if (sfflags == NULL || entflags == NULL || key == NULL) {
        rzb_log(LOG_ERR, "%s: NULL pointer passed as argument", __func__); 
		return R_ERROR;
	}
 
    Mutex_Lock(cache->cachemutex);

	entry.next = NULL;
	entry.prev = NULL;
	entry.size = size;
	entry.key = key;
	entry.listtype = LT_NONE;
	entry.sfflags = *sfflags;
	entry.entflags = *entflags;

    cachehit = FindEntry(&entry, cache);

    if (cachehit != NULL)
    {
        switch (cachehit->listtype)
        {
            //Cache hits
            case LT_T1:
                //In T1? Cache is now frequent; grows T2, shrinks T1
                cache->listSize[LT_T1]--;
                cache->listSize[LT_T2]++;
            case LT_T2:
                //In T2? Reinsert entry as MRU, caches stay the same size
                //*****Check to make sure hit isn't already T2 MRU

                //Remove entry and insert it at top of T2
                removeEntry(cachehit, cache);
                AddMRU(cachehit, LT_T2, cache);
                break;

                //Ghost cache hits
            case LT_B1:
            case LT_B2:
                //If Cache hit in B1, trend target size towards Recency
                if (cachehit->listtype == LT_B1)
                {
                    cache->target = min(cache->target + max(cache->listSize[LT_B2]/cache->listSize[LT_B1], 1), cache->size);
                    cache->listSize[LT_B1]--;
                }
                //If Cache hit in B2, trend target size towards Frequency
                else
                {
                    cache->target = max(cache->target - max(cache->listSize[LT_B1]/cache->listSize[LT_B2], 1), 0);
                    cache->listSize[LT_B2]--;
                }

                //Remove from respective ghost cache
                removeEntry(cachehit, cache);
                //Shift whole list down
                replace(cache);
                //Reenter into frequency cache
                cache->listSize[LT_T2]++; //Think this needs to be here
                AddMRU(cachehit, LT_T2, cache);
                break;

            case LT_NONE:
                rzb_log(LOG_ERR, "%s: Unexpected listtype value, possible memory corruption", __func__);
				Mutex_Unlock(cache->cachemutex);
				return R_ERROR;
        }

        Mutex_Unlock(cache->cachemutex);

        *entflags = cachehit->entflags;
		*sfflags = cachehit->sfflags;

		if (type == BADHASH)
			rzb_log(LOG_DEBUG, "%s: --Local Cache Hit-- BADHASH SF: 0x%08x, ENT: 0x%08x", __func__, *sfflags, *entflags);
		else if (type == GOODHASH)
			rzb_log(LOG_DEBUG, "%s: --Local Cache Hit-- GOODHASH SF: 0x%08x, ENT: 0x%08x", __func__, *sfflags, *entflags);

        return R_FOUND;
    }

    Mutex_Unlock(cache->cachemutex);

    return R_NOT_FOUND;
}

Lookup_Result addLocalEntry(uint8_t *key, uint32_t size, uint32_t sfflags, uint32_t entflags, CacheType type)
{
    CACHE *cache;
    ENTRY *newentry;

    if (type >= ALL) {
		rzb_log(LOG_ERR, "%s: Invalid CacheType passed as argument", __func__);
        return R_ERROR;
    }

    cache = &Cache[type];

    Mutex_Lock(cache->cachemutex);
    
	//Cache Miss (aka: Addentry)
    //B1 + T1 full?
    if (cache->listSize[LT_T1] + cache->listSize[LT_B1] == cache->size)
    {
        if (cache->listSize[LT_T1] < cache->size)
        {
            newentry = PurgeLRU(LT_B1, cache); //remove LRU from B1, and give its old memory to newentry
            cache->listSize[LT_B1]--;
            replace(cache); //Shift values down list
        }
        else
        {
            newentry = PurgeLRU(LT_T1, cache); //remove LRU from T1, give its old memory to newentry
            cache->listSize[LT_T1]--;
        }
    }
    else
    {
        if (cache->entries >= cache->size)
        {
            if (cache->entries >= 2*cache->size)
            {
                newentry = PurgeLRU(LT_B2, cache);
                cache->listSize[LT_B2]--;
            }
            else
                newentry = getNewEntry(cache); //initialize new chksum
            replace(cache); //Make room for page
        }
        else
            newentry = getNewEntry(cache);
    }
    
	AddMRU(newentry, LT_T1, cache);
    cache->listSize[LT_T1]++;
    //size and protocol are a union and size is bigger
    //So, referencing the entire dword will copy either
    newentry->size = size;
	newentry->sfflags = sfflags;
	newentry->entflags = entflags;

    if (newentry->key != NULL)
		free(newentry->key);
	if ((newentry->key = (uint8_t *)malloc(size)) == 0) {
	    rzb_log(LOG_ERR, "%s: malloctile dysfunction", __func__);
		Mutex_Unlock(cache->cachemutex);
		return R_ERROR;
	}
    memcpy(newentry->key, key, size);

    Mutex_Unlock(cache->cachemutex);
    return R_SUCCESS;
}

Lookup_Result updateLocalEntry(uint8_t *key, uint32_t size, uint32_t sfflags, uint32_t entflags, CacheType type) {

    CACHE *cache;
    ENTRY *entry;
    ENTRY temp;

	if (type >= ALL) {
        rzb_log(LOG_ERR, "%s: Invalid CacheType passed as argument", __func__); 
        return R_ERROR;
    }

    cache = &Cache[type];

    if (key == NULL) {
        rzb_log(LOG_ERR, "%s: NULL key passed as argument", __func__); 
        return R_ERROR;
    }

    Mutex_Lock(cache->cachemutex);
    
	temp.next = NULL;
    temp.prev = NULL;
    temp.size = size;
    temp.key = key;
    temp.sfflags = sfflags;
    temp.entflags = entflags;
	
	entry = FindEntry(&temp, cache);

    if (entry == NULL) { 
        //Couldn't find entry to update, it's possible.
		Mutex_Unlock(cache->cachemutex);
        return R_NOT_FOUND;
    }
 
    entry->sfflags = sfflags;
    entry->entflags = entflags;
    Mutex_Unlock(cache->cachemutex);

    return R_SUCCESS;
}

Lookup_Result removeLocalEntry(uint8_t *key, uint32_t size, CacheType type) {
    CACHE *cache;
	ENTRY *entry;
	ENTRY temp;

	if (type >= ALL) {
		rzb_log(LOG_ERR, "%s: Invalid CacheType passed as argument", __func__);
		return R_ERROR;
	}

	cache = &Cache[type];

	if (key == NULL) {
		rzb_log(LOG_ERR, "%s: NULL key passed as argument", __func__);
		return R_ERROR;
	}

	Mutex_Lock(cache->cachemutex);

	temp.next = NULL;
	temp.prev = NULL;
	temp.size = size;
	temp.key = key;

	entry = FindEntry(&temp, cache);

	if (entry == NULL) {
		Mutex_Unlock(cache->cachemutex);
		return R_NOT_FOUND;
	}

	removeEntry(entry, cache);

	destroyEntry(entry);

	entry->size = 0;

	Mutex_Unlock(cache->cachemutex);

    return R_SUCCESS;
}

Lookup_Result
clearLocalEntry(CacheType type, ClearMethod method) 
{
    
	unsigned i,j;
	CACHE *cache;

    if (type > ALL) {
		rzb_log(LOG_ERR, "%s: Unrecognized cache type.", __func__);
		return R_ERROR;
	}

	
	if (type == ALL) {
          
        for (j = 0; j < ALL; j++) {     

            cache = &Cache[j];
        
	        Mutex_Lock(cache->cachemutex);
            
			switch (method) {

                case FULL:
                    for (i = 0; i < cache->entries; i++)
                    {
                        destroyEntry(&cache->entrylist[i]);
                    }

                    cache->entries = 0;
                    cache->target = cache->size;

                    for (i = 0; i < LT_NONE; i++) {
                        Cache->listSize[i] = 0;
                        Cache->LRU[i] = NULL;
                        Cache->MRU[i] = NULL;
                    }
                    break;

                default:
                    rzb_log(LOG_EMERG, "%s: Unsupported method", __func__);
                    Mutex_Unlock(cache->cachemutex);
                    return R_ERROR;
            }
            Mutex_Unlock(cache->cachemutex);
		}
	}
	else {
	
	    cache = &Cache[type];
	    
	    Mutex_Lock(cache->cachemutex);
		
		switch (method) {

			case FULL:
		    	for (i = 0; i < cache->entries; i++)
				{
			      	destroyEntry(&cache->entrylist[i]);
		    	}

		    	cache->entries = 0;
	        	cache->target = cache->size;

	        	for (i = 0; i < LT_NONE; i++) {
		            Cache->listSize[i] = 0;
		            Cache->LRU[i] = NULL;
			        Cache->MRU[i] = NULL;
		        }
				break;

			default:
				rzb_log(LOG_EMERG, "%s: Unsupported method", __func__);
                Mutex_Unlock(cache->cachemutex);
				return R_ERROR;
		}

        Mutex_Unlock(cache->cachemutex);
	
	}


    return R_SUCCESS;
}

static double max(double a, unsigned b)
{
    return(a > (double)b) ? a : b;
}

static double min(double a, unsigned b)
{
    return(a > (double)b) ? b : a;
}

static ENTRY *getNewEntry(CACHE *cache)
{
    ENTRY *newentry;

    if (cache->entries < (cache->size*2))
    {
        newentry = &cache->entrylist[cache->entries++];
        memset(newentry, 0, sizeof(*newentry));
        newentry->listtype = LT_NONE;
        return newentry;
    }
    
	rzb_log(LOG_ERR, "%s: returning NULL, the math is wrong somewhere", __func__);
    return NULL;
}

//Find entry in the cache
static ENTRY *FindEntry(ENTRY *entry, CACHE *cache)
{
    unsigned i;
    for (i = 0; i < cache->entries; i++)
    {
        if (!memcmp(cache->entrylist[i].key, entry->key, entry->size))
            return &cache->entrylist[i];
    }

    return NULL;
}

//Find MRU in a given list
static ENTRY *FindMRU(LISTTYPE listtype, CACHE *cache)
{
    if (cache->MRU[listtype] == NULL)
    {
        unsigned i;
        for (i = 0; i < cache->entries; i++)
        {
            if (cache->entrylist[i].next == NULL && cache->entrylist[i].listtype == listtype)
                cache->MRU[listtype] = &cache->entrylist[i];
        }
    }

    return cache->MRU[listtype];
}

//Find LRU in a given list
static ENTRY *FindLRU(LISTTYPE listtype, CACHE *cache)
{
    if (cache->LRU[listtype] == NULL)
    {
        unsigned i;
        for (i = 0; i < cache->entries; i++)
        {
            if (cache->entrylist[i].prev == NULL && cache->entrylist[i].listtype == listtype)
                cache->LRU[listtype] = &cache->entrylist[i];
        }
    }

    return cache->LRU[listtype];
}

//Removes the last entry in a given list
static ENTRY *PurgeLRU(LISTTYPE listtype, CACHE *cache)
{

    ENTRY *LRU = FindLRU(listtype, cache);
    //Check that LRU isn't also MRU
    if (LRU)
    {
        if (LRU->next)
        {
            LRU->next->prev = NULL;
            cache->LRU[listtype] = LRU->next;
        }
        else
        {
            //LRU is only entry
            cache->LRU[listtype] = NULL;
            cache->MRU[listtype] = NULL;
        }
    }
    else
    {
        //either you're trying to purge from an empty list
        //or LRU isn't registered or listtype isn't set properly
        rzb_log(LOG_EMERG, "%s: Could not find LRU, This shouldn't happen", __func__);
        return NULL;
    }

    LRU->prev = NULL;
    LRU->next = NULL;
    LRU->listtype = LT_NONE;

    return LRU;
}

//Adds an entry to the top of a given list
static void AddMRU(ENTRY *newentry, LISTTYPE listtype, CACHE *cache)
{
    ENTRY *MRU;

    //If MRU exists, shift it down for the new King
    if ((MRU = FindMRU(listtype, cache)) != NULL)
    {
        MRU->next = newentry;
        newentry->prev = MRU;
        newentry->next = NULL;
    }
    //Otherwise MRU doesn't exist and sum is now both
    //the new MRU and LRU.
    else
    {
        //Make note that sum is also the LRU
        cache->LRU[listtype] = newentry;
        newentry->prev = NULL;
        newentry->next = NULL;
    }

    //In both cases, set sum to new MRU
    //and copy size and MD5 info
    cache->MRU[listtype] = newentry;
    newentry->listtype = listtype;

    return;
}

//removes an a cache hit for reinsertion
static void removeEntry(ENTRY *cachehit, CACHE *cache)
{
    if (cachehit->next)
    {
        if (cachehit->prev)
        {
            //Cachehit is neither LRU nor MRU
            cachehit->next->prev = cachehit->prev;
            cachehit->prev->next = cachehit->next;
        }
        else
        {
            //prev is NULL and cachehit is LRU
            PurgeLRU(cachehit->listtype, cache);
        }
    }
    else
    {
        if (cachehit->prev != NULL)
        {
            //Cachehit is MRU
            cachehit->prev->next = NULL;
            cache->MRU[cachehit->listtype] = cachehit->prev;
        }
        else
        {
            //Cachehit is only member of list
            cache->MRU[cachehit->listtype] = NULL;
            cache->LRU[cachehit->listtype] = NULL;
        }
    }
    cachehit->listtype = LT_NONE;
    cachehit->prev = NULL;
    cachehit->next = NULL;
}

//Migrates entries from T lists to B lists
static ENTRY *replace(CACHE *cache)
{
    ENTRY *moveditem = NULL;

    if (cache->listSize[LT_T1] >= max(cache->target,1))
    {
        moveditem = PurgeLRU(LT_T1, cache);
        AddMRU(moveditem, LT_B1, cache);
        --cache->listSize[LT_T1];
        ++cache->listSize[LT_B1];
    }
    else
    {
        moveditem = PurgeLRU(LT_T2, cache);
        AddMRU(moveditem, LT_B2, cache);
        --cache->listSize[LT_T2];
        ++cache->listSize[LT_B2];
    }

    return moveditem;
}

static void destroyEntry(ENTRY *entry) {
	    free(entry->key);
}

//TODO Update config to store the different cache sizes differently
void 
initcache(void)
{
	unsigned i,j;
	Cache[0].size = Config_getCacheBadLimit ();
	Cache[1].size = Config_getCacheGoodLimit ();
	for (i = 2; i < ALL; i++)
		Cache[i].size = 256;
	for (i = 0; i < ALL; i++) {
        Cache[i].entries = 0;
        Cache[i].entrylist = malloc(Cache[i].size * 2 * sizeof(ENTRY));
        Cache[i].target = Cache[i].size;
		if ((Cache[i].cachemutex = Mutex_Create(MUTEX_MODE_NORMAL)) ==  NULL)
            return;

		for (j = 0; j < LT_NONE; j++) {
			Cache[i].listSize[j] = 0;
			Cache[i].LRU[j] = NULL;
			Cache[i].MRU[j] = NULL;
		}
    }
    rzb_log(LOG_DEBUG, "%s: Cache initialized", __func__);
}


//Unused and unneeded
/*
void finicache(void)
{
    unsigned i,j;

    Mutex_Lock(&cachemutex);

	for (j = 0; j < ALL; j++) {
        if (Cache[j].entrylist != NULL)
        {
            for (i = 0; i < Cache[j].entries; i++)
            {
                if (Cache[j].entrylist[i].key != NULL)
                    destroyEntry(&Cache[j].entrylist[i]);
            }

            free(Cache[i].entrylist);
	    }
	}

    Mutex_Unlock(&cachemutex);
}
*/
