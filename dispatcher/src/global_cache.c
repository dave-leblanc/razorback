#include <libmemcached/memcached.h>
#include <razorback/config_file.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include "global_cache.h"
#include "config.h"
#include "configuration.h"

memcached_st *memc = NULL;
memcached_server_st *servers = NULL;
const char *cachetypestring[] = {"GOODHASH", "BADHASH", "URL"};
uint8_t cachesequence[ALL];
static pthread_mutex_t sg_mutex;

static void incrementSequence();

void 
GlobalCache_Lock (void)
{
    pthread_mutex_lock(&sg_mutex);
}

void 
GlobalCache_Unlock (void)
{
    pthread_mutex_unlock(&sg_mutex);
}

Lookup_Result checkGlobalEntry(uint8_t *key, uint32_t size, uint32_t *sfflags, uint32_t *entflags, CacheType type) {
    memcached_return rc;
    char *lookup, *value;
    size_t value_size;
    uint32_t flags;
    pthread_mutex_lock(&sg_mutex);
	size_t stringsize = strlen(cachetypestring[type]);

    if (key == NULL || sfflags == NULL || entflags == NULL) {
		rzb_log(LOG_ERR, "%s: NULL pointer argument to function", __func__);
        pthread_mutex_unlock(&sg_mutex);
		return R_ERROR;
	}
    
    if ((lookup = (char *)malloc(size + stringsize + 1)) == NULL) {
		rzb_log(LOG_ERR, "%s: Malloctile dysfunction", __func__);
        pthread_mutex_unlock(&sg_mutex);
		return R_ERROR;
	}
	
    memcpy(lookup, key, size);
	memcpy(lookup+size, cachetypestring[type], stringsize);
	lookup[size + stringsize] = cachesequence[type];

    //value = memcached_get (memc, (char *)key, size, &value_size, &flags, &rc);
    value = memcached_get(memc, lookup, size+stringsize+1, &value_size, &flags, &rc);
	if (rc == MEMCACHED_NOTFOUND) {
		free(lookup);
        pthread_mutex_unlock(&sg_mutex);
		return R_NOT_FOUND;
	}


	if (rc != MEMCACHED_SUCCESS) {
		rzb_log(LOG_ERR, "%s: %s", __func__, memcached_strerror(memc, rc));
		free(lookup);
        pthread_mutex_unlock(&sg_mutex);
		return R_ERROR;
    }

	//TODO: Change this to expected size
/*	if (value_size < 8) {
		printf("WTF\n");
		free(lookup);
		return R_ERROR;
	}*/

	memcpy(sfflags, value, 4);
	memcpy(entflags, value+4, 4);

	free(lookup);
    free(value);
    pthread_mutex_unlock(&sg_mutex);

	return R_FOUND;
}

Lookup_Result addGlobalEntry(uint8_t *key, uint32_t size, uint32_t sfflags, uint32_t entflags, CacheType type) {
	memcached_return rc;
	size_t stringsize = strlen(cachetypestring[type]);
	char *lookup;

    uint8_t flagsstore[8];
    pthread_mutex_lock(&sg_mutex);

	memcpy(flagsstore, &sfflags, 4);
	memcpy(flagsstore+4, &entflags, 4);

    if (key == NULL) {
		rzb_log(LOG_ERR, "%s: NULL pointer argument to function", __func__);
        pthread_mutex_unlock(&sg_mutex);
		return R_ERROR;
	}

    if ((lookup = (char *)malloc(size + stringsize + 1)) == NULL) {
        rzb_log(LOG_ERR, "%s: Malloctile dysfunction", __func__);
        pthread_mutex_unlock(&sg_mutex);
        return R_ERROR;
    }
   
    memcpy(lookup, key, size);
    memcpy(lookup+size, cachetypestring[type], stringsize);
	lookup[size + stringsize] = cachesequence[type];

    rc = memcached_set(memc, lookup, size+stringsize+1, (char *)flagsstore, 8, (time_t)0, (uint32_t)0);
	if (rc != MEMCACHED_SUCCESS) {
		rzb_log(LOG_ERR, "%s: %s", __func__, memcached_strerror(memc, rc));
		free(lookup);
        pthread_mutex_unlock(&sg_mutex);
		return R_ERROR;
	}

    free(lookup);
    pthread_mutex_unlock(&sg_mutex);

	return R_SUCCESS;
}

Lookup_Result updateGlobalEntry(uint8_t *key, uint32_t size, uint32_t sfflags, uint32_t entflags, CacheType type) {
    memcached_return rc;
    size_t stringsize = strlen(cachetypestring[type]);
    char *lookup;

    uint8_t flagsstore[8];

    pthread_mutex_lock(&sg_mutex);

    memcpy(flagsstore, &sfflags, 4);
    memcpy(flagsstore+4, &entflags, 4);

    if (key == NULL) {
		rzb_log(LOG_ERR, "%s: NULL pointer argument to function", __func__);
        pthread_mutex_unlock(&sg_mutex);
        return R_ERROR;
    }

    if ((lookup = (char *)malloc(size + stringsize + 1)) == NULL) {
        rzb_log(LOG_ERR, "%s: Malloctile dysfunction", __func__);
        pthread_mutex_unlock(&sg_mutex);
        return R_ERROR;
    }

    memcpy(lookup, key, size);
    memcpy(lookup+size, cachetypestring[type], stringsize);
    lookup[size + stringsize] = cachesequence[type];

    rc = memcached_replace(memc, lookup, size+stringsize+1, (char *)flagsstore, 8, (time_t)0, (uint32_t)0);
    if (rc == MEMCACHED_NOTSTORED) {
		free(lookup);
        pthread_mutex_unlock(&sg_mutex);
        return R_NOT_FOUND;
	}

    if (rc != MEMCACHED_SUCCESS) {
        rzb_log(LOG_ERR, "%s: %s", __func__, memcached_strerror(memc, rc));
		free(lookup);
        pthread_mutex_unlock(&sg_mutex);
        return R_ERROR;
    }

    free(lookup);
    pthread_mutex_unlock(&sg_mutex);

    return R_SUCCESS;	
}

Lookup_Result removeGlobalEntry(uint8_t *key, uint32_t size, CacheType type) {
	memcached_return rc;
    size_t stringsize = strlen(cachetypestring[type]);
    char *lookup;
    
    pthread_mutex_lock(&sg_mutex);

    if (key == NULL) {
		rzb_log(LOG_ERR, "%s: NULL pointer argument to function", __func__);
        pthread_mutex_unlock(&sg_mutex);
        return R_ERROR;
    }

    if ((lookup = (char *)malloc(size + stringsize + 1)) == NULL) {
        rzb_log(LOG_ERR, "%s: Malloctile dysfunction", __func__);
        pthread_mutex_unlock(&sg_mutex);
        return R_ERROR;
    }

    memcpy(lookup, key, size);
    memcpy(lookup+size, cachetypestring[type], stringsize);
    lookup[size + stringsize] = cachesequence[type]; 

    rc = memcached_delete(memc, lookup, size+stringsize+1, (time_t)0);
	if (rc == MEMCACHED_NOTFOUND) {
		free(lookup);
        pthread_mutex_unlock(&sg_mutex);
		return R_NOT_FOUND;
	}

    if (rc != MEMCACHED_SUCCESS) {
        rzb_log(LOG_ERR, "%s: %s", __func__,memcached_strerror(memc, rc));
		free(lookup);
        pthread_mutex_unlock(&sg_mutex);
        return R_ERROR;
    }

    free(lookup);
    pthread_mutex_unlock(&sg_mutex);

    return R_SUCCESS; 
}

Lookup_Result clearGlobalEntry(CacheType type, ClearMethod method) {
    pthread_mutex_lock(&sg_mutex);
	switch (method) {
		case FULL:
			incrementSequence(type);
			break;

		default:
			rzb_log(LOG_ERR, "%s: Invalid clear method", __func__);
            pthread_mutex_unlock(&sg_mutex);
			return R_ERROR;
			break;
	}
    pthread_mutex_unlock(&sg_mutex);

	return R_SUCCESS;
}

static void incrementSequence(CacheType type) {

    int i;

	if (type != ALL) {
	    if (cachesequence[type] == 0x39)
		    cachesequence[type] = 0x30;
	    else 
		    cachesequence[type]++;
	}
	else {
        for (i = 0; i < ALL; i ++) {
            if (cachesequence[i] == 0x39)
                cachesequence[i] = 0x30;
            else
                cachesequence[i]++;
		}
	}
}

uint32_t Global_Cache_Initialize () {
    pthread_mutexattr_t mutexAttr;
    unsigned i;
    memcached_return rc;
    struct CacheHost *temp = NULL;

    memset(&mutexAttr, 0, sizeof(pthread_mutexattr_t));
    memset(&sg_mutex, 0, sizeof(pthread_mutex_t));
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&sg_mutex, &mutexAttr);

    temp = g_cacheHosts;
    for (conf_int_t i = 0; i < g_cacheHostCount; i++)
    {
        servers = memcached_server_list_append(servers, temp->hostname, temp->port, &rc);
		if (rc != MEMCACHED_SUCCESS) {
		     rzb_log(LOG_ERR, "%s: %s", __func__, memcached_strerror(memc, rc));
			 return 0;
		}
		temp++;
	}

    memc = memcached_create(NULL);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
    rc = memcached_server_push(memc, servers);
    if (rc != MEMCACHED_SUCCESS) {
         rzb_log(LOG_ERR, "%s: %s", __func__, memcached_strerror(memc, rc));
         return 0;
    }

    //memcached_pool_st *pool = memcached_pool(optionstring, strlen(optionstring));

	for (i = 0; i < ALL; i++)
		cachesequence[i] = 0x30;

	return 1;
}


void finiMemcached (void) {
	memcached_server_free(servers);
	memcached_free(memc);
}

