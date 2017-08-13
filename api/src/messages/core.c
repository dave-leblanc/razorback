#include "config.h"
#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/list.h>

#include "messages/core.h"
#include "messages/cnc/core.h"
#include "init.h"

#include <string.h>
static struct List * handlerList = NULL;
static bool Message_Add_Directed_Headers(struct Message *message, const uuid_t source, const uuid_t dest);


static int MessageHeader_KeyCmp(void *a, void *id);
static int MessageHeader_Cmp(void *a, void *b);
static void MessageHeader_Delete(void *a);

static int MessageHandler_KeyCmp(void *a, void *id);
static int MessageHandler_Cmp(void *a, void *b);
static void MessageHandler_Delete(void *a);

struct Message *
Message_Create(uint32_t type, uint32_t version, size_t msgSize)
{
    struct Message *message;
    if ((message = calloc(1,sizeof(struct Message))) == NULL)
        return NULL;
    message->type = type;
    message->version = version;
    if (msgSize > 0)
    {
        if ((message->message = calloc(1,msgSize)) == NULL)
            return NULL;
    }
    message->headers = Message_Header_List_Create();
    return message;
}

SO_PUBLIC void
Message_Destroy(struct Message *message)
{
    ASSERT(message != NULL);
    if (message == NULL)
        return;

    if (message->message != NULL)
        free(message->message);

    if (message->serialized != NULL)
        free(message->serialized);

    List_Destroy(message->headers);
    free(message);
}

SO_PUBLIC bool
Message_Add_Header(struct List *headers, const char *p_sName, const char *p_sValue)
{
    struct MessageHeader *l_pHeader;
    if ((l_pHeader = calloc(1, sizeof (struct MessageHeader))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to alloc new header", __func__);
        return false;
    }
    if ((l_pHeader->sName = calloc(1, strlen(p_sName)+1)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to alloc new header name", __func__);
        free(l_pHeader);
        return false;
    }
    if ((l_pHeader->sValue = calloc(1, strlen(p_sValue)+1)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to alloc new header value", __func__);
        free(l_pHeader->sName);
        free(l_pHeader);
        return false;
    }
    strcpy(l_pHeader->sName, p_sName);
    strcpy(l_pHeader->sValue, p_sValue);
    List_Push(headers, l_pHeader);
    return true;
}


SO_PUBLIC struct List *
Message_Header_List_Create(void)
{
    return List_Create(LIST_MODE_GENERIC, 
            MessageHeader_Cmp,
            MessageHeader_KeyCmp,
            MessageHeader_Delete,
            NULL, NULL, NULL);
}

static int 
MessageHeader_KeyCmp(void *a, void *id)
{
    struct MessageHeader *item = (struct MessageHeader *)a;
    char *key = id;
    return strcmp(item->sName, key);
}

static int
MessageHeader_Cmp(void *a, void *b)
{
    struct MessageHeader *iA = (struct MessageHeader *)a;
    struct MessageHeader *iB = (struct MessageHeader *)b;
    if (a == b)
        return 0;
    if ( (strcmp(iA->sName, iA->sName) == 0) ||
            ( strcmp(iA->sValue, iB->sValue) == 0 ))
    {
        return 0;
    }
    return -1;

}

static void 
MessageHeader_Delete(void *a)
{
    struct MessageHeader *header = a;
    free(header->sName);
    free(header->sValue);
    free(header);
}


bool
Message_Init()
{
    handlerList = List_Create(LIST_MODE_GENERIC, 
            MessageHandler_Cmp,
            MessageHandler_KeyCmp,
            MessageHandler_Delete,
            NULL, NULL, NULL);

    if (handlerList == NULL)
        return false;

    MessageBlockSubmission_Init();
    MessageCacheReq_Init();
    MessageCacheResp_Init();
    MessageInspectionSubmission_Init();
    MessageJudgmentSubmission_Init();
    MessageLogSubmission_Init();
    MessageLogSubmission_Init();
    MessageAlertPrimary_Init();
    MessageAlertChild_Init();
    MessageOutputLog_Init();
    MessageOutputEvent_Init();

    Message_CnC_Bye_Init();
    Message_CnC_CacheClear_Init();
    Message_CnC_ConfigAck_Init();
    Message_CnC_ConfigUpdate_Init();
    Message_CnC_Error_Init();
    Message_CnC_Go_Init();
    Message_CnC_Hello_Init();
    Message_CnC_Pause_Init();
    Message_CnC_Paused_Init();
    Message_CnC_RegReq_Init();
    Message_CnC_RegResp_Init();
    Message_CnC_Running_Init();
    Message_CnC_Term_Init();
    Message_CnC_ReReg_Init();

    return true;
}

bool
Message_Setup(struct Message *message)
{
    struct MessageHandler *handler = NULL;
	
	handler = List_Find(handlerList, &message->type);
    if (handler == NULL) 
        return false;
	
    message->serialize = handler->serialize;
    message->deserialize = handler->deserialize;
    message->destroy = handler->destroy;

    return true;
}

bool Message_Register_Handler(struct MessageHandler *handler)
{
    return List_Push(handlerList, handler);
}



static int 
MessageHandler_KeyCmp(void *a, void *id)
{
    struct MessageHandler *item = (struct MessageHandler *)a;
    uint32_t *key = id;
    return (item->type - *key);
}

static int
MessageHandler_Cmp(void *a, void *b)
{
    struct MessageHandler *iA = (struct MessageHandler *)a;
    struct MessageHandler *iB = (struct MessageHandler *)b;
    if (a == b)
        return 0;
    return (iA->type - iB->type);
}

static void 
MessageHandler_Delete(void *a)
{
}

static bool 
Message_Add_Directed_Headers(struct Message *message, const uuid_t source, const uuid_t dest)
{
    char uuid[UUID_STRING_LENGTH];
    uuid_unparse(source, uuid);
    Message_Add_Header(message->headers, MSG_CNC_HEADER_SOURCE, uuid);
    uuid_unparse(dest, uuid);
    Message_Add_Header(message->headers, MSG_CNC_HEADER_DEST, uuid);

    return true;
}

SO_PUBLIC struct Message * 
Message_Create_Directed(uint32_t type, uint32_t version, 
        size_t msgSize, const uuid_t source, const uuid_t dest)
{
    struct Message *msg;
    if ((msg = Message_Create(type, version, msgSize)) == NULL)
        return NULL;

    Message_Add_Directed_Headers(msg, source, dest);
    return msg;
}

SO_PUBLIC struct Message * 
Message_Create_Broadcast(uint32_t type, uint32_t version, 
        size_t msgSize, const uuid_t source)
{
    struct Message *msg;
    uuid_t dest;
    uuid_clear(dest);

    if ((msg = Message_Create(type, version, msgSize)) == NULL)
        return NULL;

    Message_Add_Directed_Headers(msg, source, dest);
    return msg;
}

SO_PUBLIC bool
Message_Serialize_Empty(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;


    switch (mode)
    {
    case MESSAGE_MODE_BIN:
    case MESSAGE_MODE_JSON:
        break;
    default:
        rzb_log(LOG_ERR, "%s: Invalid serialization mode", __func__);
        return false;
    }
    if ((message->serialized = calloc(2, sizeof(char))) == NULL)
        return false;

    message->serialized[0]=' ';
    message->serialized[1]='\0';
    message->length=1;

    return true;
}

SO_PUBLIC bool
Message_Deserialize_Empty(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;


    switch (mode)
    {
    case MESSAGE_MODE_BIN:
    case MESSAGE_MODE_JSON:
        return true;
    default:
        rzb_log(LOG_ERR, "%s: Invalid serialization mode", __func__);
        return false;
    }
    return false;
}

SO_PUBLIC bool 
Message_Get_Nuggets(struct Message *message, uuid_t source, uuid_t dest)
{
    struct MessageHeader *header;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((header = List_Find(message->headers, (void *)MSG_CNC_HEADER_DEST)) == NULL)
        return false;

    if (uuid_parse(header->sValue, dest) != 0)
        return false;

    if ((header = List_Find(message->headers, (void *)MSG_CNC_HEADER_SOURCE)) == NULL)
        return false;

    if (uuid_parse(header->sValue, source) != 0)
        return false;

    return true;
}

