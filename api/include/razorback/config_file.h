/** @file config_file.h
 * Razorback Configuration API
 */
#ifndef RAZORBACK_CONFIG_FILE_H
#define RAZORBACK_CONFIG_FILE_H

#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
typedef DWORD conf_int_t;

#else //_MSC_VER
#include <libconfig.h>




#if (((LIBCONFIG_VER_MAJOR == 1) && (LIBCONFIG_VER_MINOR >= 4)) \
       || (LIBCONFIG_VER_MAJOR > 1))
    /* use features present in libconfig 1.4 and later */
typedef int conf_int_t;
#else
typedef long conf_int_t;
#endif

#endif //_MSC_VER

/** Avaliable types to read from the config file.
 */
typedef enum
{
    RZB_CONF_KEY_TYPE_STRING = 5,   ///< A String.
    RZB_CONF_KEY_TYPE_INT,      ///< A signed int.
    RZB_CONF_KEY_TYPE_PARSED_STRING,    ///< A string with a callback to turn it into an int.
    RZB_CONF_KEY_TYPE_UUID,     ///< A UUID in string format.
    RZB_CONF_KEY_TYPE_BOOL,     ///< A Bool in string format.
    RZB_CONF_KEY_TYPE_ARRAY,    ///< An array of simple items
    RZB_CONF_KEY_TYPE_LIST,     ///< An list of complex items
    RZB_CONF_KEY_TYPE_END       ///< End of block marker.
} RZB_CONF_KEY_TYPE_t;

/**
 * Configuration callbacks
 */
typedef struct
{
    bool (*parseString) (const char *, conf_int_t *);   ///< Call back to convert the passed string into an int.
} RZBConfCallBack;

struct ConfArray 
{
    RZB_CONF_KEY_TYPE_t type;
    void **data;
    conf_int_t *count;
    bool (*parseString) (const char *, conf_int_t *);   ///< Call back to convert the passed string into an int.
};
/**
 * Configuration file entry definition.
 */
typedef struct
{
    const char *key;            ///< The path to the entry in the config file.
    RZB_CONF_KEY_TYPE_t type;   ///< The ::RZB_CONF_KEY_TYPE_t value for the type of value to read for the entry.
    void *dest;                 ///< A pointer to the pointer to the memory to be used to store the value.
    void *callback;  ///< Callback structure.
} RZBConfKey_t;

struct ConfList
{
    void **data;
    conf_int_t *count;
    RZBConfKey_t *items;
};

/** Read a component configuration file.
 * @param *configDir the dir to look in
 * @param *configFile the file read
 * @param *config the structure of the file.
 * @return true on success false on error
 */
SO_PUBLIC extern bool readMyConfig (const char *configDir, const char *configFile,
                          RZBConfKey_t * config);

/** Clean the memory allocated by ::readApiConfig and ::readMyConfig
 *
 */
SO_PUBLIC extern void rzbConfCleanUp (void);

#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_CONFIG_FILE_H

 /** @example read_config.c
  * The following example shows how to use ::readMyConfig
  */
