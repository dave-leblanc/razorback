#ifndef DISP_TRANSFER_CORE_H
#define DISP_TRANSFER_CORE_H
#include <razorback/types.h>
#include <razorback/connected_entity.h>

struct TransferServerDescriptor {
    uint8_t id;
    const char *name;
    const char *description;
    bool (*start)(void);
};

bool Transfer_Server_Init(void);
bool TransferServer_Register(struct TransferServerDescriptor *desc);

bool Transfer_Server_Start(void);

// Init functions
bool Transfer_Server_SSH_Init(void);

#endif
