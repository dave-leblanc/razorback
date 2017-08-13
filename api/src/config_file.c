#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include <WinSock2.h>
#include "safewindows.h"
#include "bobins.h"
#else //_MSC_VER
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif //_MSC_VER

#include <razorback/config_file.h>
#include <razorback/log.h>
#include "runtime_config.h"

#define API_CONFIG_FILE "rzb.conf"

#ifdef _MSC_VER
#define config_t void
#define config_setting_t void
#else //_MSC_VER
// Libconfig handle
static config_t config;
#endif //_MSC_VER

typedef enum
{
    UNINIT, READY, CONFIGURED, UNCONFIGURED
} CONFIGSTATE;

struct RZBConfigFile
{
    RZBConfKey_t *type;
#ifdef _MSC_VER
#else //_MSC_VER
    config_t config;
#endif //_MSC_VER
    struct RZBConfigFile *next;
};

struct RZBConfigFile;
typedef struct RZBConfigFile RZBConfigFile;

RZBConfigFile *configList = NULL;

CONFIGSTATE configState = UNCONFIGURED;

bool testFile (const char *);
#ifdef _MSC_VER
bool parseBlockWin32 (HKEY RegKey, RZBConfKey_t * block);
#else
bool parseBlock (config_t *, RZBConfKey_t *);
#endif
static char *getConfigFile (const char *, const char *);
bool parseRoutingType (const char *, conf_int_t *);

#ifndef _MSC_VER

static char *
getConfigFile (const char *configDir, const char *configFile)
{
	char *result;
    if (configDir == NULL)
    {
        configDir = ETC_DIR;
    }
    
    result =
        calloc (strlen (configDir) + strlen (configFile) + 2, sizeof (char));
    if (result == NULL)
        return NULL;

    strncpy (result, configDir, strlen (configDir) + 1);
    strncat (result, "/", 1);
    strncat (result, configFile, strlen (configFile));

    return result;
}

#endif


SO_PUBLIC bool
readMyConfig (const char *configDir, const char *configFile,
              RZBConfKey_t * config_fmt)
{
#ifdef _MSC_VER
	//Reading from the registry instead of a config file
	//Implements same API
	HKEY hkey = HKEY_LOCAL_MACHINE;
	HKEY razorback, rzbapikey, dirkey;
	LONG lRet;
	RZBConfigFile * file;
	
	dirkey = NULL;

	if (configFile == NULL) {
		rzb_log(LOG_ERR, "%s: configFile is NULL.", __func__);
		return false;
	}

	/*
	keyname = createConfKeyName(configDir, configFile);
	if (keyname == NULL) {
		rzb_log(LOG_ERR, "%s: Failed due to failure of createConfKeyName().", __func__);
		return false;
	}
	*/

	if (configState == UNINIT)
	{
		configState = READY;
	}

    if ((file = (RZBConfigFile *)calloc(1, sizeof (RZBConfigFile))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate config storage", __func__);
        return false;
    }

	file->type = config_fmt;
	
	if (configList == NULL)
    {
        configList = file;
    }
    else
    {
        file->next = configList;
        configList = file;
    }

	
	lRet = RegOpenKeyA(
		hkey,
		"SOFTWARE\\Razorback",
		&razorback);
	
	if(lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed because registry key does not exist. SOFTWARE", __func__);
		return false;
	}

	//Split configdir
	if (configDir == NULL) 
	{
		configDir = "default";
	}
	lRet = RegOpenKeyExA(
		razorback,
		configDir,
		0,
		KEY_READ,
		&dirkey);

	if(lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed because registry key does not exist: %s %d", __func__, configDir, lRet);
		return false;
	}
	
	//Split configfile

	lRet = RegOpenKeyExA(
		dirkey,
		configFile,
		0,
		KEY_READ,
		&rzbapikey);

	if(lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed because registry key does not exist: %S %d", __func__, configDir, lRet);
		return false;
	}
    

	if(!parseBlockWin32(rzbapikey, config_fmt)) {
		rzb_log(LOG_ERR, "%s: Failed due to failure of parseBlockWin32.", __func__);
		return false;
	}


	RegCloseKey(rzbapikey);
	RegCloseKey(dirkey);
	RegCloseKey(razorback);
	//RegCloseKey(software);
    //RegCloseKey(rzbapikey);

	return true;

#else
    if (configFile == NULL)
    {
        rzb_log (LOG_EMERG, "%s: configFile was null", __func__);
        return false;
    }
    if (configState == UNINIT)
    {
        memset (&config, 0, sizeof (config));
        config_init (&config);
        configState = READY;
    }

    char *configfile = getConfigFile (configDir, configFile);
    RZBConfigFile * file;

    if ((file =calloc(1, sizeof (RZBConfigFile))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate config storage", __func__);
        free(configfile);
        return false;
    }

    file->type = config_fmt;
    if (!testFile (configfile))
    {
        free (configfile);
        return false;
    }
    if (config_read_file (&file->config, configfile) != CONFIG_TRUE)
        rzb_log (LOG_ERR, "%s: failed to read file: %s", __func__, config_error_text (&config));

    if (configList == NULL)
    {
        configList = file;
    }
    else
    {
        file->next = configList;
        configList = file;
    }
    free (configfile);

    return parseBlock (&file->config, config_fmt);
	#endif
}

#ifdef _MSC_VER

static bool
parseArrayWin32 (HKEY key, RZBConfKey_t *block) 
{
	HKEY arraykey;
	DWORD type, numvalues, RegValue, keylen;
	DWORD RegValueLen = sizeof(RegValue);
	conf_int_t i, *ints;
	uuid_t *uuids;
	bool *bools;
	LONG lRet;
	char * stringvalue;//, wvaluename;
	char **strings, *valuename;
	
	struct ConfArray *arrayConf = (struct ConfArray *)block->callback;

	keylen = strlen(block->key) + 1;

	lRet = RegOpenKeyExA(
		key,
		block->key,
		0,
		KEY_READ,
		&arraykey);

	if (lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed to open subkey", __func__);
		return false;
	}


	lRet = RegQueryInfoKeyA(
		arraykey,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&numvalues,
		NULL,
		NULL,
		NULL,
		NULL);

	if (lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed to obtain number of values in subkey", __func__);
		return false;
	}

	if (numvalues <= 1) {
		*arrayConf->count = 0;
		return true;
	}

	numvalues = numvalues - 1;

	switch(arrayConf->type) {
		case RZB_CONF_KEY_TYPE_INT:
			if ((ints = (conf_int_t *)calloc(numvalues, sizeof(conf_int_t)))== NULL)
                return false;
            for (i = 0; i < numvalues; i++) {

				if (asprintf(&valuename, "%d", i+1) <= 0) {
					rzb_log(LOG_ERR, "%s: Failed due to failure of asprintf()", __func__);
					free(ints);
					return false;
				}

				keylen = strlen(valuename) + 1;


				lRet = RegQueryValueExA(
		            arraykey, 
		            valuename,
		            NULL, 
		            &type, 
		            (LPBYTE)&RegValue, 
		            &RegValueLen);

				if (lRet != ERROR_SUCCESS) {
					rzb_log(LOG_ERR, "%s: Failed to query registry value", __func__);
					return false;
				}

				if (type != REG_DWORD) {
					rzb_log(LOG_ERR, "%s: Failed because registry key is not the right type", __func__);
					return false;
				}

				ints[i] = RegValue;

				free(valuename);
			}

			*(arrayConf->count) = numvalues;
            *(arrayConf->data) = (void *)ints;
			break;

		case RZB_CONF_KEY_TYPE_STRING:
		case RZB_CONF_KEY_TYPE_PARSED_STRING:
		case RZB_CONF_KEY_TYPE_UUID:
		case RZB_CONF_KEY_TYPE_BOOL:
			if ((strings = (char **)calloc(numvalues, sizeof(char *))) == NULL)
				return false;
			for (i = 0; i < numvalues; i++) {
                
				if (asprintf(&valuename, "%d", i+1) <= 0) {
					rzb_log(LOG_ERR, "%s: Failed due to failure of asprintf()", __func__);
					return false;
				} 

				keylen = strlen(valuename) + 1;


				lRet = RegQueryValueExA(
		            arraykey, 
		            valuename,
		            NULL, 
		            &type, 
		            NULL, 
		            &RegValueLen);

				if (lRet != ERROR_SUCCESS) {
					rzb_log(LOG_ERR, "%s: Failed to query registry value length", __func__);
					return false;
			    }

				stringvalue = calloc(RegValueLen, sizeof(char));

				lRet = RegQueryValueExA(
		            arraykey, 
		            valuename,
		            NULL, 
		            &type, 
		            (LPBYTE)stringvalue, 
		            &RegValueLen);

				if (lRet != ERROR_SUCCESS) {
					rzb_log(LOG_ERR, "%s: Failed to query registry value", __func__);
					return false;
			    }

				if (type != REG_SZ) {
					rzb_log(LOG_ERR, "%s: Failed because registry key is not the right type", __func__);
					return false;
				}

				strings[i] = stringvalue;

				free(valuename);

			}

			if (arrayConf->type == RZB_CONF_KEY_TYPE_PARSED_STRING) {
				if ((ints = (conf_int_t *)calloc(numvalues, sizeof(conf_int_t)))== NULL)
                    return false;
				
				for (i = 0; i < numvalues; i++) {
					if (!(arrayConf->parseString (strings[i], ints++)))
						return false;
					free(strings[i]);
				}
			
				free(strings);

				*(arrayConf->count) = numvalues;
				*(arrayConf->data) = (void *)ints;
			}
			else if (arrayConf->type == RZB_CONF_KEY_TYPE_UUID) {
				if ((uuids = (uuid_t *)calloc(numvalues, sizeof(uuid_t)))== NULL)
					return false;

				for (i = 0; i < numvalues; i++) {
					if (uuid_parse (strings[i], (uuids[i])) == -1)
					{
						rzb_log (LOG_ERR, "%s: Failed to parse UUID: %s", __func__, strings[i]);
						return false;
					}
					free(strings[i]);
				}

				free(strings);

				*(arrayConf->count) = numvalues;
				*(arrayConf->data) = (void *)uuids;
			}
			else if (arrayConf->type == RZB_CONF_KEY_TYPE_BOOL) {
				if ((bools = (bool *)calloc(numvalues, sizeof(bool)))== NULL)
					return false;
				for (i = 0; i < numvalues; i++) {
			        if (strncasecmp(strings[i],"true",4) == 0)
                        bools[i] = true;
                    else if (strncasecmp(strings[i],"false",5) == 0)
                        bools[i] = false;
                    else
                    {
						rzb_log (LOG_ERR, "%s: Failed to parse bool: %s", __func__, strings[i]);
				        return false;
					}
					free(strings[i]);
				}

				free(strings);

				*(arrayConf->count) = numvalues;
				*(arrayConf->data) = (void *)bools;
			}
			else {	
				*(arrayConf->count) = numvalues;
				*(arrayConf->data) = (void *)strings;
			}
			break;

		default:
			break;

	}

	RegCloseKey(arraykey);

	return true;
}

#else

static bool 
parseArray(config_setting_t *config, RZBConfKey_t *block)
{
    int size;
    struct ConfArray *arrayConf = block->callback;
    void *data;

    conf_int_t *ints;
    char **strings;
    const char *tmp;
    uuid_t *uuids;
    bool *bools;

    size = config_setting_length(config);

    rzb_log(LOG_INFO, "%s: Array found: %d", block->key, size);
    if (size == 0)
    {
        *arrayConf->count = 0;
        return true;
    }

    if (arrayConf->type == RZB_CONF_KEY_TYPE_INT)
    {
        if ((data = calloc(size, sizeof(conf_int_t)))== NULL)
            return false;
        ints = data;
        for (conf_int_t i = 0; i < size; i++)
            ints[i] = config_setting_get_int_elem(config, i);

    }
    else if (arrayConf->type == RZB_CONF_KEY_TYPE_STRING)
    {
        if ((data = calloc(size, sizeof(char *)))== NULL)
            return false;
        strings = data;
        for (conf_int_t i = 0; i < size; i++)
            strings[i] = (char *)config_setting_get_string_elem(config, i);
    }
    else if (arrayConf->type == RZB_CONF_KEY_TYPE_PARSED_STRING)
    {
        if ((data = calloc(size, sizeof(conf_int_t)))== NULL)
            return false;
        ints = data;
        for (conf_int_t i = 0; i < size; i++)
        {
            tmp = config_setting_get_string_elem(config, i);
            if (!(arrayConf->parseString (tmp, ints++)))
                return false;
            
        }
    }
    else if (arrayConf->type == RZB_CONF_KEY_TYPE_UUID)
    {
        if ((data = calloc(size, sizeof(uuid_t)))== NULL)
            return false;
        uuids = data;
        for (conf_int_t i = 0; i < size; i++)
        {
            tmp = config_setting_get_string_elem(config, i);
            if (uuid_parse (tmp, (uuids[i])) == -1)
            {
                rzb_log (LOG_ERR, "%s: Failed to parse UUID: %s", __func__, tmp);
                return false;
            }
        }

    }
    else if (arrayConf->type == RZB_CONF_KEY_TYPE_BOOL)
    {
        if ((data = calloc(size, sizeof(bool)))== NULL)
            return false;
        bools = data;
        for (conf_int_t i = 0; i < size; i++)
        {
            tmp = config_setting_get_string_elem(config, i);
            if (strncasecmp(tmp,"true",4) == 0)
                bools[i] = true;
            else if (strncasecmp(tmp,"false",5) == 0)
                bools[i] = false;
            else
            {
                rzb_log (LOG_ERR, "%s: Failed to parse bool: %s", __func__, tmp);
                return false;
            }
        }

    }
    else
    {
        rzb_log (LOG_ERR, "%s: Unsupported array config attribute type.", __func__);
        return false;
    }

    *arrayConf->count = size;
    *arrayConf->data = data;
    return true;
}

#endif

#ifdef _MSC_VER
static bool
parseListWin32(HKEY key,  RZBConfKey_t *block) 
{
	HKEY listkey;
	RZBConfKey_t *cur;
	void *data;
	LONG lRet;
	char *itemData, *valuename;//, *intstr;
	DWORD i, RegValue, RegValueLen, type, itemSize, numvalues, keylen;
	char *stringvalue;
	struct ConfList *listConf;
	
	//block->callback is our list, keep a pointer to it
	listConf = (struct ConfList *)block->callback;

	keylen = strlen(block->key) + 1;


	//Open the registry subkey representing the name of list
	lRet = RegOpenKeyExA(
		key,
		block->key,
		0,
		KEY_READ,
		&listkey);

	//If the subkey doesn't open, it may not exist.
	if (lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed to open subkey", __func__);
		return false;
	}

	//Next, query the subkey to find out how many values 
	//are stored within. 
	lRet = RegQueryInfoKeyA(
		listkey,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&numvalues,
		NULL,
		NULL,
		NULL,
		NULL);

	//Again, this is a system call that shouldn't ever fail.
	if (lRet != ERROR_SUCCESS) {
		rzb_log(LOG_ERR, "%s: Failed to obtain number of values in subkey", __func__);
		return false;
	}

	//Registry subkeys always include a value called "default".
	//We are ignoring "default" here, so if numvalues == 1, 
	//then the subkey contains only "default" and none
	//of our named values.
	if (numvalues <= 1) {
		*listConf->count = 0;
		return true;
	}

	numvalues = numvalues - 1;

	//Set up while loop
	cur = listConf->items;

	//Propagate through all of the items expected in the
	//list. Add up the sizes of the type for each item
	//to get the amount of memory we'll need to allocate.
	while (cur->type != RZB_CONF_KEY_TYPE_END)
    {
        if (cur->type == RZB_CONF_KEY_TYPE_INT)
            itemSize += sizeof(conf_int_t);
        else if (cur->type == RZB_CONF_KEY_TYPE_STRING)
            itemSize += sizeof(char *);
        else if (cur->type == RZB_CONF_KEY_TYPE_PARSED_STRING)
            itemSize += sizeof(conf_int_t);
        else if (cur->type == RZB_CONF_KEY_TYPE_UUID)
            itemSize += sizeof(uuid_t);
        else if (cur->type == RZB_CONF_KEY_TYPE_BOOL)
            itemSize += sizeof(bool);
        else
        {
            rzb_log (LOG_ERR, "%s: Unsupported list member attribute type.", __func__);
            return false;
        }
    
        cur++;
    }

	//Allocate space for item data
	if ((data = calloc(numvalues, itemSize)) == NULL)
        return false;
	
	//data will be our pointer to the beginning of our block.
	//itemData will be our cursor. It points to where within
	//the block we are currently copying data.
	itemData = data;

	for (i = 0; i < numvalues; i++)
    {
        cur = listConf->items;
        while (cur->type != RZB_CONF_KEY_TYPE_END) {
			
			//Just like in the Array type, the names of our keys are the same as 
			//their index, starting at value "1".
			if (asprintf(&valuename, "%d", i+1) <= 0) {
				rzb_log(LOG_ERR, "%s: Failed due to failure of asprintf()", __func__);
			    return false;
			}
			
			switch (cur->type) {
			    case RZB_CONF_KEY_TYPE_INT:
						
					RegValueLen = sizeof(RegValue);
					
					//Expecting an int. Get Registry Value.
				    lRet = RegQueryValueExA(
			            listkey, 
			            valuename,
			            NULL, 
				        &type, 
					    (LPBYTE)&RegValue, 
						&RegValueLen);
	
					//Shouldn't fail unless value in registry is longer than 4 bytes.
					if (lRet != ERROR_SUCCESS) {
						rzb_log(LOG_ERR, "%s: Failed to query registry value", __func__);
						return false;
					}

					//Make sure the Registry value we queried is of the right type.
					if (type != REG_DWORD) {
						rzb_log(LOG_ERR, "%s: Failed because registry key is not the right type", __func__);
						return false;
					}

					//Store the result in our data block
					//and increment the cursor by the size of that result.
					 *(conf_int_t *)itemData = RegValue;
					itemData += sizeof(conf_int_t);

					break;

				case RZB_CONF_KEY_TYPE_STRING:
				case RZB_CONF_KEY_TYPE_PARSED_STRING:
				case RZB_CONF_KEY_TYPE_UUID:
				case RZB_CONF_KEY_TYPE_BOOL:
									
					//We're expecting a string but we have no 
					//idea how long it is. Querying with a NULL
					//storage buffer will return the number of bytes needed.
					lRet = RegQueryValueExA(
			            listkey, 
			            valuename,
			            NULL, 
				        &type, 
					    NULL, 
						&RegValueLen);

					//Shouldn't fail.
					if (lRet != ERROR_SUCCESS) {
						rzb_log(LOG_ERR, "%s: Failed to query registry value length", __func__);
						return false;
					}

					stringvalue = calloc(RegValueLen, sizeof(char));
					if (stringvalue == NULL) {
						rzb_log(LOG_ERR, "%s: Failed to allocate space for registry value.", __func__);
					}

					//Get the actual content of the string
					lRet = RegQueryValueExA(
			            listkey, 
			            valuename,
				        NULL, 
					    &type, 
						(LPBYTE)stringvalue, 
					    &RegValueLen);

					//Shouldn't fail.
					if (lRet != ERROR_SUCCESS) {
						rzb_log(LOG_ERR, "%s: Failed to query registry value", __func__);
						return false;
				    }

					//Make sure what we got is indeed a string
					if (type != REG_SZ) {
						rzb_log(LOG_ERR, "%s: Failed because registry key is not the right type", __func__);
						return false;
					}

					if (cur->type == RZB_CONF_KEY_TYPE_PARSED_STRING) {

						//For parsed strings, use the callback function
						if (!((RZBConfCallBack *)cur->callback)->parseString (stringvalue, (conf_int_t *)itemData))
							return false;

						free(stringvalue);
						itemData += sizeof(conf_int_t);
					}
					else if (cur->type == RZB_CONF_KEY_TYPE_UUID) {
						
						//For uuids, use uuid_parse()
						if (uuid_parse (stringvalue, *(uuid_t *)itemSize) == -1)
						{
							rzb_log (LOG_ERR, "%s: Failed to parse UUID: %s", __func__, stringvalue);
							return false;
						}

						free(stringvalue);
						itemData += sizeof(uuid_t);
					}
					else if (cur->type == RZB_CONF_KEY_TYPE_BOOL) {
						
						//For bools, use strncasecmp()
						if (strnicmp(stringvalue, "true", 4) == 0) {
							*(bool *)itemData = true;
						}
						else if (strnicmp(stringvalue, "false", 5) == 0) {
							*(bool *)itemData = false;
						}
						else {
							rzb_log (LOG_ERR, "%s: Failed to parse bool: %s", __func__, RegValue);
					     return false;
						}

						free(stringvalue);

						itemData += sizeof(bool);
					}
					else {
						//Otherwise, just use the value proper
						memcpy(itemData, &stringvalue, sizeof(void *));
						itemData += sizeof(char *);
					}

					break;
				
				default:
					rzb_log (LOG_ERR, "%s: Unsupported list member attribute type.", __func__);
					return false;
			}
			free(valuename);
		    cur++;
		}
	}

	*(listConf->data) = data;
    *(listConf->count) = numvalues;

	RegCloseKey(listkey);

	return true;
}

#else

static bool 
parseList(config_setting_t *config, RZBConfKey_t *block)
{
    int size =0, itemSize =0;
    struct ConfList *listConf = block->callback;
    RZBConfKey_t * cur;
    void *data;
    char **itemData;
    config_setting_t *item;
	conf_int_t *intItem;
    const char *tmp;
    bool *boolItem;
    uuid_t *uuidItem;
	conf_int_t i;

    size = config_setting_length(config);

    rzb_log(LOG_INFO, "%s: List found: %d", block->key, size);
    if (size == 0)
    {
        *listConf->count = 0;
        return true;
    }
    cur = listConf->items;
    while (cur->type != RZB_CONF_KEY_TYPE_END)
    {
        if (cur->type == RZB_CONF_KEY_TYPE_INT)
            itemSize += sizeof(conf_int_t);
        else if (cur->type == RZB_CONF_KEY_TYPE_STRING)
            itemSize += sizeof(char *);
        else if (cur->type == RZB_CONF_KEY_TYPE_PARSED_STRING)
            itemSize += sizeof(conf_int_t);
        else if (cur->type == RZB_CONF_KEY_TYPE_UUID)
            itemSize += sizeof(uuid_t);
        else if (cur->type == RZB_CONF_KEY_TYPE_BOOL)
            itemSize += sizeof(bool);
        else
        {
            rzb_log (LOG_ERR, "%s: Unsupported list member attribute type.", __func__);
            return false;
        }
    
        cur++;
    }
    if ((data = calloc(size, itemSize)) == NULL)
        return false;

    
    itemData = data;
    
    for (i = 0; i < size; i++)
    {
        cur = listConf->items;
        item = config_setting_get_elem(config, i);
        while (cur->type != RZB_CONF_KEY_TYPE_END)
        {
            if (cur->type == RZB_CONF_KEY_TYPE_INT)
            {
                intItem= (conf_int_t*)itemData;
                config_setting_lookup_int(item, cur->key, intItem);
                itemData += sizeof(conf_int_t);
            }
            else if (cur->type == RZB_CONF_KEY_TYPE_STRING)
            {
                config_setting_lookup_string(item, cur->key, &tmp);
                // Filthy
                memcpy(itemData, (char *)&tmp, sizeof(char *)); 
                itemData += sizeof(char *);
            }
            else if (cur->type == RZB_CONF_KEY_TYPE_PARSED_STRING)
            {
                config_setting_lookup_string(item, cur->key, &tmp);
                intItem= (conf_int_t*)itemData;
                if (!((RZBConfCallBack *)cur->callback)->parseString (tmp, intItem))
                    return false;
                itemData +=sizeof(conf_int_t);
            }
            else if (cur->type == RZB_CONF_KEY_TYPE_UUID)
            {
                config_setting_lookup_string(item, cur->key, &tmp);
                uuidItem = (uuid_t *)itemData;
                if (uuid_parse (tmp, *uuidItem) == -1)
                {
                    rzb_log (LOG_ERR, "%s: Failed to parse UUID: %s", __func__, tmp);
                    return false;
                }

                itemData += sizeof(uuid_t);
            }
            else if (cur->type == RZB_CONF_KEY_TYPE_BOOL)
            {
                config_setting_lookup_string(item, cur->key, &tmp);
                boolItem= (bool*)itemData;
                if (strncasecmp(tmp,"true",4) == 0)
                    *boolItem = true;
                else if (strncasecmp(tmp,"false",5) == 0)
                    *boolItem = false;
                else
                {
                    rzb_log (LOG_ERR, "%s: Failed to parse bool: %s", __func__, tmp);
                    return false;
                }
                itemData += sizeof(bool);
            }
            else
            {
                rzb_log (LOG_ERR, "%s: Unsupported list member attribute type.", __func__);
                return false;
            }
        
            cur++;
        }
    }
    *listConf->data = data;
    *listConf->count = size;

    return true;
}

#endif

#ifdef _MSC_VER

bool
parseBlockWin32 (HKEY RegKey, RZBConfKey_t * block) 
{
	LONG lRet = 0;
	DWORD type =0, expectedType=0, ParsedValue=0;
	DWORD RegValueLen=0, RegValue=0, keylen=0;
	char *stringvalue;

	while (block->type != RZB_CONF_KEY_TYPE_END) 
	{
		//Examine expected format
		//Make value that format and save inside variable
		
		keylen = strlen(block->key) + 1;

		switch (block->type) 
		{
			case RZB_CONF_KEY_TYPE_INT:
			
				RegValueLen = sizeof(LPDWORD);

				//Get value for key
	            lRet = RegQueryValueExA(
		            RegKey, 
					block->key,
		            NULL, 
		            &type, 
		            (LPBYTE)&RegValue, 
		            &RegValueLen);	

		        if (lRet != ERROR_SUCCESS) {
					rzb_log(LOG_ERR, "%s: Failed to retrieve registry value %s", __func__, block->key);
					break;
				}

		        if (type != REG_DWORD) {
            	    rzb_log(LOG_ERR, "%s: Failed because registry key is not the right type.", __func__);
		    	    return false;
		        }

				*(conf_int_t *)(block->dest) = RegValue;
				break;

			case RZB_CONF_KEY_TYPE_STRING:
			case RZB_CONF_KEY_TYPE_PARSED_STRING:
			case RZB_CONF_KEY_TYPE_UUID:
			case RZB_CONF_KEY_TYPE_BOOL:

				lRet = RegQueryValueExA(
		            RegKey, 
					block->key,
		            NULL, 
		            NULL, 
		            NULL, 
		            &RegValueLen);

				if (lRet != ERROR_SUCCESS) {
					rzb_log(LOG_ERR, "%s: Failed to retrieve size of registry value. %S", __func__, block->key);
					break;
				}

				stringvalue = calloc(RegValueLen, sizeof(char));
				if (stringvalue == NULL) {
					rzb_log(LOG_ERR, "%s: Failed to allocate memory.", __func__);
					return false;
				}

    			lRet = RegQueryValueExA(
		            RegKey, 
					block->key,
		            NULL, 
		            &type, 
		            (LPBYTE)stringvalue, 
		            &RegValueLen);

				if (lRet != ERROR_SUCCESS) {
					rzb_log(LOG_ERR, "%s: Failed to retireve registry value.", __func__);
					return false;
				}

		        if (type != REG_SZ) {
            	    rzb_log(LOG_ERR, "%s: Failed because registry value is not the right type.", __func__);
					return false;
		        }
				
				if (block->type == RZB_CONF_KEY_TYPE_PARSED_STRING) {

					

		        	if (!((RZBConfCallBack *)block->callback)->parseString (stringvalue, (conf_int_t *)&ParsedValue)) {
						rzb_log(LOG_ERR, "%s: Failed because callback function parseString() failed. Key: %s, Value: %s", __func__, block->key, stringvalue);
						return false;
			        }

					*(conf_int_t *)(block->dest) = ParsedValue;
					free(stringvalue);
		        }
				else if (block->type == RZB_CONF_KEY_TYPE_UUID) {
					if (uuid_parse (stringvalue, (unsigned char *)block->dest) == -1)
                    {
                        rzb_log (LOG_ERR, "%s: Failed to parse UUID: %s", __func__, RegValue);
                        return false;
                    }

					free(stringvalue);
				}
				else if (block->type == RZB_CONF_KEY_TYPE_BOOL) {
					if (_strnicmp(stringvalue, "true", 4) == 0) {
						*(bool *)block->dest = true;
					}
					else if (strnicmp(stringvalue, "false", 5) == 0) {
						*(bool *)block->dest = false;
					}
					else {
						rzb_log (LOG_ERR, "%s: Failed to parse bool: %s", __func__, RegValue);
                        return false;
					}
					free(stringvalue);
				}
		        else {
			        *(char **)(block->dest) = stringvalue;
		        }
				break;

			case RZB_CONF_KEY_TYPE_ARRAY:
				if (!parseArrayWin32(RegKey, block))
					return false;
				break;

			case RZB_CONF_KEY_TYPE_LIST:
				if (!parseListWin32(RegKey, block))
					return false;
				break;

			default:
				rzb_log (LOG_ERR, "%s: Unknown config attribute type.", __func__);
				return false;
				
		}

		block++;
	}
	return true;
}

#else

bool
parseBlock (config_t * config, RZBConfKey_t * block)
{
    int status = CONFIG_TRUE;
    conf_int_t t;
    config_setting_t *tt;
    const char *type;
    while (block->type != RZB_CONF_KEY_TYPE_END)
    {
        tt = config_lookup (config, block->key);
        if (tt == NULL)
        {
            rzb_log (LOG_WARNING, "%s: Cant find key: %s", __func__, block->key);
            block++;
            continue;
        }

        if (block->type == RZB_CONF_KEY_TYPE_INT)
        {
            status = config_lookup_int (config, block->key, &t);
            *(conf_int_t *) (block->dest) = t;
        }
        else if (block->type == RZB_CONF_KEY_TYPE_STRING)
        {
            status = config_lookup_string (config, block->key, block->dest);
        }
        else if (block->type == RZB_CONF_KEY_TYPE_PARSED_STRING)
        {
            status = config_lookup_string (config, block->key, &type);
            if (status != CONFIG_TRUE)
            {
                rzb_log (LOG_ERR, "%s: failed to lookup string: %s", __func__, config_error_text (config));
                return false;
            }
            if (!((RZBConfCallBack *)block->callback)->parseString (type, &t))
                return false;

            *(int *) (block->dest) = (int) t;
        }
        else if (block->type == RZB_CONF_KEY_TYPE_UUID)
        {
            status = config_lookup_string (config, block->key, &type);
            if (status != CONFIG_TRUE)
            {
                rzb_log (LOG_ERR, "%s: failed to lookup string", __func__, config_error_text (config));
                return false;
            }
            if (uuid_parse (type, block->dest) == -1)
            {
                rzb_log (LOG_ERR, "%s: Failed to parse UUID: %s", __func__, type);
                return false;
            }

        }
        else if (block->type == RZB_CONF_KEY_TYPE_BOOL)
        {
            status = config_lookup_string (config, block->key, &type);
            if (status != CONFIG_TRUE)
            {
                rzb_log (LOG_ERR, "%s: failed to lookup string", __func__, config_error_text (config));
                return false;
            }
            if (strncasecmp(type,"true",4) == 0)
                *(bool *)block->dest = true;
            else if (strncasecmp(type,"false",5) == 0)
                *(bool *)block->dest = false;
            else
            {
                rzb_log (LOG_ERR, "%s: Failed to parse bool: %s", __func__, type);
                return false;
            }

        }
        else if (block->type == RZB_CONF_KEY_TYPE_ARRAY)
        {
            if (config_setting_is_array(tt) == CONFIG_FALSE)
            {
                rzb_log (LOG_ERR, "%s: Failed to parse array: %s", __func__, block->key);
                return false;
            }
            if (!parseArray(tt, block))
                return false;
        }
        else if (block->type == RZB_CONF_KEY_TYPE_LIST)
        {
            if (config_setting_is_list(tt) == CONFIG_FALSE)
            {
                rzb_log (LOG_ERR, "%s: Failed to parse list: %s", __func__, block->key);
                return false;
            }
            if (!parseList(tt, block))
                return false;
        }
        else
        {
            rzb_log (LOG_ERR, "%s: Unknown config attribute type.", __func__);
            return false;
        }
        if (status != CONFIG_TRUE)
        {
            rzb_log (LOG_ERR, "%s: parsing failure: %s", __func__, config_error_text (config));
            return false;
        }
        block++;
    }
    return true;
}

#endif

bool
parseRoutingType (const char *string, conf_int_t * val)
{
    if (!strncasecmp (string, "opaque", 6))
    {
        *val = 0;
        return true;
    }
    else if (!strncasecmp (string, "transparent", 11))
    {
        *val = 1;
        return true;
    }
    return false;

}

#ifndef _MSC_VER
bool
testFile (const char *configfile)
{
    struct stat sb;
    int fd = open (configfile, O_RDONLY);

    if (fd == -1)
    {
        rzb_log (LOG_ERR, "%s: Failed to open (%s) in ", __func__, configfile);
        return false;
    }

    if (fstat (fd, &sb) == -1)
    {
        return false;
    }
    close (fd);
    return true;

}
#endif

SO_PUBLIC void
rzbConfCleanUp (void)
{
#ifndef _MSC_VER
    config_destroy (&config);
#endif
    while (configList != NULL)
    {
#ifndef _MSC_VER
        config_destroy (&configList->config);
#endif
        configList = configList->next;
    }
}
