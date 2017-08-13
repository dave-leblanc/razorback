#ifndef RAZORBACK_REQUEUE_H
#define RAZORBACK_REQUEUE_H

#include <razorback/types.h>

#define REQUEUE_TYPE_BLOCK 1
#define REQUEUE_TYPE_ERROR 2
#define REQUEUE_TYPE_PENDING 3

struct RequeueRequest
{
    int type;
    struct BlockId *blockId;
    uuid_t appType;
};


extern void Requeue_Thread (struct Thread *p_pThread);
extern bool Requeue_Request(struct RequeueRequest *req);



#endif
