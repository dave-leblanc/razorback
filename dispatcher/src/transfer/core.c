#include "config.h"
#include <razorback/debug.h>
#include <razorback/types.h>
#include <razorback/log.h>
#include <razorback/hash.h>

#include "transfer/core.h"
#include "configuration.h"
static struct List *sg_serverList = NULL;
static int TransferServer_Cmp(void *a, void *b);
static int TransferServer_KeyCmp(void *a, void *key);

bool
Transfer_Server_Init(void)
{
    sg_serverList = List_Create(LIST_MODE_GENERIC,
            TransferServer_Cmp,
            TransferServer_KeyCmp,
            NULL,
            NULL, // Clone
            NULL, // Lock
            NULL); // Unlock
    if (sg_serverList == NULL)
        return false;
    // XXX: Replaced with module loading
    Transfer_Server_SSH_Init();
    return true;
}


bool 
TransferServer_Register(struct TransferServerDescriptor *desc)
{
    return List_Push(sg_serverList, desc);
}

bool 
Transport_IsSupported(uint8_t protocol)
{
    struct TransferServerDescriptor *trans= NULL;
    trans = List_Find(sg_serverList, &protocol);
    return trans == NULL;
}

bool 
Transfer_Server_Start(void)
{
    struct TransferServerDescriptor *trans= NULL;
    trans = List_Find(sg_serverList, &transferProtocol);
    if (trans == NULL)
        return false;
    return trans->start();
}


static int 
TransferServer_Cmp(void *a, void *b)
{
    struct TransferServerDescriptor *dA = a;
    struct TransferServerDescriptor *dB = b;
    if (dA == dB)
        return 0;
    return (dA->id - dB->id);
}

static int 
TransferServer_KeyCmp(void *a, void *key)
{
    struct TransferServerDescriptor *dA = a;
    uint8_t *id = key;
    return (dA->id - *id);
}

