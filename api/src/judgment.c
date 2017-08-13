#include "config.h"

#include <razorback/event.h>
#include <razorback/log.h>
#include <razorback/block.h>
#include <razorback/block_id.h>
#include <razorback/ntlv.h>
#include <razorback/thread.h>
#include <razorback/judgment.h>

#include <time.h>
#include <string.h>

#ifdef _MSC_VER
#include "bobins.h"
#endif


SO_PUBLIC struct Judgment *
Judgment_Create (struct EventId *eventId, struct BlockId *blockId)
{
    struct Judgment *judgment = NULL;
    struct timespec l_tsTime;
    struct RazorbackContext *l_pContext;

    l_pContext = Thread_GetCurrentContext ();

    memset(&l_tsTime, 0, sizeof(struct timespec));
    if (clock_gettime(CLOCK_REALTIME, &l_tsTime) == -1)
    {
        rzb_log(LOG_ERR, "%s: Failed to get time stamp", __func__);
        return NULL;
    }

    if ((judgment = calloc(1, sizeof(struct Judgment))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed allocate judgment", __func__);
        return NULL;
    }

    if ((judgment->pMetaDataList = NTLVList_Create()) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate judgment meta data", __func__);
        Judgment_Destroy(judgment);
        return NULL;
    }
    if (eventId != NULL)
    {
        if ((judgment->pEventId = EventId_Clone(eventId)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate judgment eventId", __func__);
            Judgment_Destroy(judgment);
            return NULL;
        }
    }
    if (blockId != NULL)
    {
        if ((judgment->pBlockId = BlockId_Clone(blockId)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate judgment blockId", __func__);
            Judgment_Destroy(judgment);
            return NULL;
        }
    }
    judgment->iSeconds = l_tsTime.tv_sec;
    judgment->iNanoSecs = l_tsTime.tv_nsec;
    uuid_copy(judgment->uuidNuggetId, l_pContext->uuidNuggetId);
    return judgment;
}

SO_PUBLIC void
Judgment_Destroy (struct Judgment *judgment)
{

    if (judgment->pEventId != NULL)
        EventId_Destroy(judgment->pEventId);

    if (judgment->pBlockId != NULL)
        BlockId_Destroy(judgment->pBlockId);

    if (judgment->pMetaDataList != NULL)
        List_Destroy(judgment->pMetaDataList);
    if (judgment->sMessage != NULL)
        free(judgment->sMessage);

    free(judgment);
}

SO_PUBLIC uint32_t 
Judgment_BinaryLength (struct Judgment *judgment)
{
    uint32_t size =  sizeof(judgment->uuidNuggetId)+
        (sizeof(uint64_t)*2) +
        sizeof (struct EventId) +
        sizeof (uint8_t) +
        NTLVList_Size(judgment->pMetaDataList) +
        (sizeof (uint32_t) * 6) +
        sizeof(uint8_t) +
        BlockId_BinaryLength(judgment->pBlockId);

    if (judgment->sMessage != NULL)
        size += strlen((char *)judgment->sMessage) +1;

    return size;
}

