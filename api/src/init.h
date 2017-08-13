#ifndef RAZORBACK_INIT_H
#define RAZORBACK_INIT_H
#include <razorback/types.h>
#ifdef __cplusplus
extern "C" {
#endif
// log.c
extern bool configureLogging (void);

// local_cache.c
extern void initcache (void);

// uuids.c
extern void initUuids (void);

// runtime_config.c
void readApiConfig (void);

// api.c
void initApi (void);

bool Crypto_Initialize(void);

//magic.c
bool Magic_Init(void);

//messages/core.c
bool Message_Init(void);

//transfer/core.c
bool Transfer_Init(void);

#ifdef __cplusplus
}
#endif
#endif
