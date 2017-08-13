#include "config.h"
#include <razorback/debug.h>
#include <razorback/queue.h>
#include <razorback/log.h>
#include <razorback/messages.h>
#include <razorback/list.h>
#include <stdio.h>
#include <string.h>
#ifdef _MSC_VER
#include "bobins.h"
#else //_MSC_VER
#include <strings.h>
#endif //_MSC_VER
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include "runtime_config.h"
#include "messages/core.h"

#define MBUF_SIZE 1024
struct StompMessage {
    char * sVerb;
    struct List *headers;
    uint8_t *pBody;
    size_t bodyLength;
};

static void 
Queue_Destroy_Stomp_Message (struct StompMessage *message) 
{
    if (message->sVerb != NULL)
        free(message->sVerb);
   
    if (message->headers != NULL)
        List_Destroy(message->headers);

    if (message->pBody != NULL)
        free(message->pBody);

    free(message);
}

static struct StompMessage *
Queue_Read_Message(struct Socket *socket) 
{
    struct StompMessage *message;
    char *buffer;
    ssize_t messageLen;
    char *headerItem;

	ASSERT (socket !=NULL);
	if (socket == NULL)
		return NULL;

    if ((message = (struct StompMessage *)calloc (1, sizeof (struct StompMessage))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate message struct", __func__);
        free(buffer);
        return NULL;
    }
    message->headers= Message_Header_List_Create();
   
readverb:
    // Read the VERB
    messageLen = Socket_Rx_Until (socket, (uint8_t**)&buffer, '\n');

    if (messageLen <= 0)
    {
        if (errno != EINTR)
            rzb_log (LOG_ERR, "%s: failed due to failure of Socket_Rx_Until", __func__);

        Queue_Destroy_Stomp_Message(message);
        return NULL;
    }
    if (messageLen == 1 && buffer[0] == '\n')
    {
    	free(buffer);
    	buffer = NULL;
        goto readverb;
    }
    if ((message->sVerb = (char *)calloc(messageLen, sizeof(char))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: failed due to failure of calloc", __func__);
        Queue_Destroy_Stomp_Message(message);
        free(buffer);
        return NULL;
    }
    buffer[messageLen -1] = '\0';
    // Copy the verb
    strncpy(message->sVerb, buffer, messageLen);
#ifdef STOMP_DEBUG
    rzb_log(LOG_DEBUG, "%s: Message Verb: %s - %u", __func__, message->sVerb, messageLen);
#endif
    free(buffer);
    buffer = NULL;
    if ((messageLen = Socket_Rx_Until (socket, (uint8_t**)&buffer, '\n')) <= 0)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Socket_Rx_Until", __func__);
        Queue_Destroy_Stomp_Message(message);
        return NULL;
    }
    

    while (messageLen != 1 && buffer[0] != '\n') // End of headers
    {
        buffer[messageLen -1] = '\0';
        headerItem = strchr(buffer, ':');
        if (headerItem == NULL) // No headers
            break;

        *headerItem = '\0';
        headerItem++;

        // Add the item to the list
        Message_Add_Header(message->headers, buffer, headerItem);

#ifdef STOMP_DEBUG
        rzb_log(LOG_DEBUG, "%s: Message Header: %s:%s", __func__, buffer, headerItem);
#endif

        if (strcasecmp(buffer, "content-length") == 0)
        {
            message->bodyLength =strtoul(headerItem, NULL, 10);
            if (message->bodyLength == 0)
            {
                rzb_log(LOG_ERR, "%s: Failed to parse message length: %s", __func__, headerItem);
                Queue_Destroy_Stomp_Message(message);
                free(buffer);
                return NULL;
            }
#ifdef STOMP_DEBUG
            rzb_log(LOG_DEBUG, "%s: Found content length: %d", __func__, message->bodyLength);
#endif
        }

        // Read the next line
        free(buffer);
        buffer = NULL;
        if ((messageLen = Socket_Rx_Until (socket, (uint8_t**)&buffer, '\n')) <= 0)
        {
            rzb_log (LOG_ERR,
                     "%s failed due to failure of Socket_Rx_Until", __func__);
            Queue_Destroy_Stomp_Message(message);
            free(buffer);
            return NULL;
        }
    }
    if (buffer != NULL)
    {
    	free(buffer);
    	buffer = NULL;
    }

    if (message->bodyLength != 0) // Read the message body
    {
		if ((message->pBody = (uint8_t *)malloc(message->bodyLength+1)) == NULL) 
        {
			//Need to store it
            rzb_log (LOG_ERR,
                     "%s: failed to allocate binary buffer", __func__);
            Queue_Destroy_Stomp_Message(message);
            return NULL;
        }
        if (Socket_Rx(socket, message->bodyLength, message->pBody) != (ssize_t)message->bodyLength)
        {
            rzb_log (LOG_ERR,
                     "%s: failed to read message body", __func__);
            Queue_Destroy_Stomp_Message(message);
            return NULL;
        }
        // read the final '\0'
        if (Socket_Rx (socket, 1, (uint8_t *)&message->pBody[message->bodyLength]) != 1)
        {
            rzb_log (LOG_ERR, "%s: failed due to Socket_Rx", __func__);
            Queue_Destroy_Stomp_Message(message);
            return NULL;
        }
    }
    else
    {
        // Read until we get a null
        messageLen = Socket_Rx_Until(socket, &message->pBody, '\0');
        if (messageLen <= 0)
        {
			rzb_log (LOG_ERR, "%s: failed due to Socket_Rx", __func__);
			Queue_Destroy_Stomp_Message(message);
			return NULL;
        }
		message->bodyLength = messageLen;
    }
    return message;
}

static int
Queue_Send_Header(void *vHeader, void *vSocket)
{
    struct MessageHeader *header = vHeader;
    struct Socket *socket = vSocket;
    char *line = NULL;
#ifdef STOMP_DEBUG
        rzb_log(LOG_DEBUG, "%s: Message Header: %s:%s", __func__, header->sName, header->sValue);
#endif
    if (asprintf(&line, "%s:%s\n", header->sName, header->sValue) == -1)
    {
        rzb_log(LOG_ERR, "%s: Failed to alloc header", __func__);
        return LIST_EACH_ERROR;
    }
    //printf("%s", line);
    if (Socket_Tx(socket, strlen(line), (uint8_t *)line) != (ssize_t)strlen(line))
    {
        rzb_log(LOG_ERR, "%s: Failed to send header", __func__);
        free(line);
        return LIST_EACH_ERROR;
    }
    free(line);

    return LIST_EACH_OK;
}

static bool 
Queue_Send_Message(struct Socket *socket, struct StompMessage *message)
{
    char *line;
#ifdef STOMP_DEBUG
    rzb_log(LOG_DEBUG, "%s: Message Verb: %s - %u", __func__, message->sVerb, message->bodyLength);
#endif

    if (asprintf(&line, "%s\n", message->sVerb) == -1)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate verb", __func__);
        return false;
    }

    if (Socket_Tx(socket, strlen(line), (uint8_t *)line) != (ssize_t)strlen(line)) {
        rzb_log(LOG_ERR, "%s: Failed to send verb", __func__);
        free(line);
        return false;
    }

    free(line);

    List_ForEach(message->headers, Queue_Send_Header, socket);

    if (Socket_Tx(socket, 1, (uint8_t *)"\n") != 1)
    {
        rzb_log(LOG_ERR, "%s: Failed to send end of header", __func__);
        return false;
    }
    if (message->pBody != NULL)
    {
        if (Socket_Tx(socket, message->bodyLength, message->pBody) != (ssize_t)message->bodyLength)
        {
            rzb_log(LOG_ERR, "%s: Failed to send message body", __func__);
            return false;
        }
    }
    if (Socket_Tx(socket, 1, (uint8_t *)"\0") != 1)
    {
        rzb_log(LOG_ERR, "%s: Failed to send end of message", __func__);
        return false;
    }
    return true;
}

static struct StompMessage *
Queue_Message_Create(const char * verb)
{
    struct StompMessage *message;
    if ((message = calloc(1, sizeof(struct StompMessage))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to alloc message", __func__);
        return NULL;
    }
    if ((message->sVerb = calloc(strlen(verb)+1, sizeof(char))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate verb", __func__);
        free(message);
        return NULL;
    }
    strcpy(message->sVerb, verb);
    message->headers = Message_Header_List_Create();
    return message;
}


static char *
Queue_Message_Get_Header(struct StompMessage *message, const char *name)
{
    struct MessageHeader *header = NULL;
    header = List_Find(message->headers, (void *)name);

    if (header != NULL)
        return header->sValue;
    else 
        return NULL;
}



static struct Socket *
Queue_Connect_Socket( const char * address,
                        int16_t port, const char * username,
                        const char * password, bool useSSL)
{
    struct Socket *socket;
    struct StompMessage *message;
	
	ASSERT (address != NULL);
    ASSERT (username != NULL);
    ASSERT (password != NULL);

    // open the socket
    if (useSSL)
        socket = SSL_Socket_Connect ((uint8_t *)address, port);
    else
        socket = Socket_Connect ((uint8_t *)address, port);

    if (socket == NULL )
    {
        if (useSSL)
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of SSL_Socket_Connect", __func__);
        else 
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of Socket_Connect", __func__);
        return NULL;
    }

    if ((message= Queue_Message_Create("CONNECT")) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: Failed to create connect message", __func__);
        Socket_Close(socket);
        return NULL;
    }

    // Put headers in backwards 
    if (!Message_Add_Header(message->headers, "passcode", password) || 
            !Message_Add_Header(message->headers, "login", username))
    {
        rzb_log(LOG_ERR, "%s: Failed to add auth headers", __func__);
        Queue_Destroy_Stomp_Message(message);
        Socket_Close(socket);
        return NULL;
    }

    // send the Connect message
    if (!Queue_Send_Message(socket, message))
    {
        rzb_log(LOG_ERR, "%s: Failed to send connect message", __func__);
        Socket_Close(socket);
        Queue_Destroy_Stomp_Message(message);
        return NULL;
    }
    Queue_Destroy_Stomp_Message(message); //< The Connect message

    if ((message = Queue_Read_Message(socket)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to read connection response", __func__);
        Socket_Close(socket);
        return false;
    }
    if (strcasecmp(message->sVerb, "CONNECTED") != 0)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of strncasecmp ( CONNECTED )", __func__);
        return NULL;
    }

    Queue_Destroy_Stomp_Message(message);

    // done
    return socket;
}

static bool
Queue_BeginReading (struct Queue *p_pQ)
{
    struct StompMessage *l_pMessage;

	ASSERT (p_pQ != NULL);

    // send the subscribe message

    if ((l_pMessage= Queue_Message_Create("SUBSCRIBE")) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: Failed to create subscribe message", __func__);
        return false;
    }

    if (!Message_Add_Header(l_pMessage->headers, "destination", p_pQ->sName) ||
            !Message_Add_Header(l_pMessage->headers, "ack", "client"))
    {
        rzb_log(LOG_ERR, "%s: Failed to add destination headers", __func__);
        Queue_Destroy_Stomp_Message(l_pMessage);
        return false;
    }

    // send the Connect message
    if (!Queue_Send_Message(p_pQ->pReadSocket, l_pMessage))
    {
        rzb_log(LOG_ERR, "%s: Failed to send subscribe message", __func__);
        Queue_Destroy_Stomp_Message(l_pMessage);
        return false;
    }

    Queue_Destroy_Stomp_Message(l_pMessage);

    return true;
}

static bool
Queue_EndReading (struct Queue *p_pQ)
{
    struct StompMessage *l_pMessage;

	ASSERT (p_pQ != NULL);

    // send the subscribe message

    if ((l_pMessage= Queue_Message_Create("UNSUBSCRIBE")) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: Failed to create unsubscribe message", __func__);
        return false;
    }

    if (!Message_Add_Header(l_pMessage->headers, "destination", p_pQ->sName))
    {
        rzb_log(LOG_ERR, "%s: Failed to add destination headers", __func__);
        Queue_Destroy_Stomp_Message(l_pMessage);
        return false;
    }

    // send the Connect message
    if (!Queue_Send_Message(p_pQ->pReadSocket, l_pMessage))
    {
        rzb_log(LOG_ERR, "%s: Failed to send unsubscribe message", __func__);
        Queue_Destroy_Stomp_Message(l_pMessage);
        return false;
    }
    Queue_Destroy_Stomp_Message(l_pMessage);

    return true;
}

static bool
Queue_Connect(struct Queue *queue)
{
    if ((queue->iFlags & QUEUE_FLAG_RECV) == QUEUE_FLAG_RECV)
    {
        if ((queue->pReadSocket = 
                    Queue_Connect_Socket(Config_getMqHost (), 
                        Config_getMqPort (), Config_getMqUser (), 
                        Config_getMqPassword (), Config_getMqSSL())) == NULL)
        {
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of Queue_Connect_Socket (Read)", __func__);
            return false;
        }
        if (!Queue_BeginReading (queue))
        {
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of Queue_BeginReading", __func__);
            return false;
        }
    }

    if ((queue->iFlags & QUEUE_FLAG_SEND) == QUEUE_FLAG_SEND)
    {
        if ((queue->pWriteSocket = 
                    Queue_Connect_Socket(Config_getMqHost (), 
                        Config_getMqPort (), Config_getMqUser (), 
                        Config_getMqPassword (), Config_getMqSSL())) == NULL)
        {
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of Queue_Connect_Socket (Write)", __func__);
            return false;
        }
    }
    return true;
}
static bool
Queue_Reconnect(struct Queue *queue)
{
    if ((queue->iFlags & QUEUE_FLAG_RECV) == QUEUE_FLAG_RECV &&
            queue->pReadSocket != NULL)
    {
        if (queue->pReadSocket != NULL)
        {
            Socket_Close (queue->pReadSocket);
            queue->pReadSocket = NULL;
        }
    }

    if ((queue->iFlags & QUEUE_FLAG_SEND) == QUEUE_FLAG_SEND &&
            queue->pWriteSocket != NULL)
    {
        if (queue->pWriteSocket != NULL)
        {
            Socket_Close (queue->pWriteSocket);
            queue->pWriteSocket = NULL;
        }
    }
        

    return Queue_Connect(queue);
}

SO_PUBLIC struct Queue *
Queue_Create (const char * p_sQueueName, int p_iFlags, int mode)
{
    struct Queue *l_pQueue;

	ASSERT (p_sQueueName != NULL);

    if ((l_pQueue = (struct Queue *)calloc (1, sizeof (struct Queue))) == NULL)
    {
        rzb_log (LOG_ERR, "%s: Failed to alloc new queue", __func__);
        return NULL;
    }

    if ((l_pQueue->sName = (char *)calloc(strlen((char *)p_sQueueName)+1, sizeof(char))) == NULL)
    {
        rzb_log (LOG_ERR, "%s: Failed to alloc new queue name", __func__);
        free(l_pQueue);
        return NULL;
    }
    strcpy(l_pQueue->sName, (char *)p_sQueueName);
    if (((l_pQueue->mReadMutex = Mutex_Create(MUTEX_MODE_NORMAL)) == NULL) ||
        ((l_pQueue->mWriteMutex = Mutex_Create(MUTEX_MODE_NORMAL)) == NULL))
    {
        free(l_pQueue);
        return NULL;
    }
    l_pQueue->iFlags = p_iFlags;
    if (!Queue_Connect(l_pQueue))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Queue_Connect", __func__);
        Queue_Terminate(l_pQueue);
        return NULL;
    }
    l_pQueue->mode = mode;
    return l_pQueue;
}

SO_PUBLIC void
Queue_Terminate (struct Queue *p_pQ)
{
    struct StompMessage *l_pMessage;

	ASSERT (p_pQ != NULL);

    Mutex_Lock (p_pQ->mReadMutex);
    Mutex_Lock (p_pQ->mWriteMutex);
    if ((l_pMessage= Queue_Message_Create("DISCONNECT")) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: Failed to create disconnect message", __func__);
    }
 
    if ((p_pQ->iFlags & QUEUE_FLAG_RECV) == QUEUE_FLAG_RECV &&
            p_pQ->pReadSocket != NULL)
    {
        Queue_EndReading (p_pQ);
        if (l_pMessage != NULL)
            Queue_Send_Message(p_pQ->pReadSocket, l_pMessage);
        Socket_Close (p_pQ->pReadSocket);
    }
    if ((p_pQ->iFlags & QUEUE_FLAG_SEND) == QUEUE_FLAG_SEND &&
            p_pQ->pWriteSocket != NULL)
    {
        if (l_pMessage != NULL)
            Queue_Send_Message(p_pQ->pWriteSocket, l_pMessage);
        Socket_Close (p_pQ->pWriteSocket);
    }

    if (l_pMessage != NULL)
        Queue_Destroy_Stomp_Message(l_pMessage);

    Mutex_Unlock (p_pQ->mReadMutex);
    Mutex_Unlock (p_pQ->mWriteMutex);
    Mutex_Destroy (p_pQ->mReadMutex);
    Mutex_Destroy (p_pQ->mWriteMutex);
    free(p_pQ->sName);
    free(p_pQ);
}

SO_PUBLIC struct Message *
Queue_Get (struct Queue *queue)
{
    struct Message *ret = NULL;
    struct StompMessage *message = NULL;
    struct StompMessage *ack = NULL;
    struct MessageHeader *header = NULL;
    char * messageId;

	ASSERT (queue);
    Mutex_Lock (queue->mReadMutex);

    if (( message = Queue_Read_Message (queue->pReadSocket)) == NULL)
    {
        if ( errno != EINTR )
        {
            rzb_perror ("failed due to failure of Queue_Read_Message: %s");
            while (!Queue_Reconnect(queue))
                rzb_log(LOG_ERR, "%s: Reconnecting", __func__);
        }
        Mutex_Unlock (queue->mReadMutex);
        return NULL;
    }
    if (strcasecmp(message->sVerb, "MESSAGE") == 0)
    {
        if ((messageId = Queue_Message_Get_Header(message, "message-id")) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to get message-id", __func__);
            Mutex_Unlock (queue->mReadMutex);
            Queue_Destroy_Stomp_Message(message);
            return NULL;
        }

        // Send Ack
        if ((ack = Queue_Message_Create("ACK")) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create ACK", __func__);
            Queue_Destroy_Stomp_Message(message);
            Mutex_Unlock (queue->mReadMutex);
            return NULL;
        }
        if (!Message_Add_Header(ack->headers, "message-id", messageId))
        {
            rzb_log(LOG_ERR, "%s: Failed to add ack message-id headers", __func__);
            Queue_Destroy_Stomp_Message(ack);
            Queue_Destroy_Stomp_Message(message);
            Mutex_Unlock (queue->mReadMutex);
            return NULL;
        }
        if (!Queue_Send_Message(queue->pReadSocket, ack))
        {
            rzb_log(LOG_ERR, "%s: Failed to send ack message", __func__);
            Queue_Destroy_Stomp_Message(ack);
            Queue_Destroy_Stomp_Message(message);
            Mutex_Unlock (queue->mReadMutex);
            return NULL;
        }
        Queue_Destroy_Stomp_Message(ack);

        Mutex_Unlock (queue->mReadMutex);
        if ((ret = (struct Message *)calloc(1,sizeof(struct Message))) == NULL)
        {
            Queue_Destroy_Stomp_Message(message);
			return NULL;
        }
        if ((header = (struct MessageHeader *)List_Find(message->headers, (void*)"rzb-msg-type")) == NULL)
        {
            free(ret);
            Queue_Destroy_Stomp_Message(message);
            return NULL;
        }
        ret->type =strtoul(header->sValue, NULL, 10);
        
        if ((header = (struct MessageHeader *)List_Find(message->headers, (void*)"rzb-msg-ver")) == NULL)
        {
            free(ret);
            Queue_Destroy_Stomp_Message(message);
            return NULL;
        }
        ret->version =strtoul(header->sValue, NULL, 10);
        ret->length = message->bodyLength;
        ret->headers = message->headers;
        ret->serialized = message->pBody;
        message->headers = NULL;
        message->pBody = NULL;
        Queue_Destroy_Stomp_Message(message);
        if (!Message_Setup(ret))
        {
            free(ret);
            return NULL;
        }
        ret->deserialize(ret, queue->mode);
        return ret;
    }

    errno = EAGAIN;
    Queue_Destroy_Stomp_Message(message);
    Mutex_Unlock (queue->mReadMutex);
    return NULL;
}
SO_PUBLIC bool
Queue_Put (struct Queue * queue,  struct Message * message)
{
    return Queue_Put_Dest(queue, message, queue->sName);
}

SO_PUBLIC bool
Queue_Put_Dest (struct Queue * queue,  struct Message * message, char *dest)
{
    struct StompMessage *stompMessage;
    char *messageId = NULL;
    char *messageLen = NULL;
    char *messageType = NULL;
    char *messageVer = NULL;
    char *l_pReceiptId = NULL;

    time_t curTime = time(NULL);

    ASSERT (queue != NULL);
    ASSERT (message != NULL);

    if (queue == NULL)
        return false;
    if (message == NULL)
        return false;
    
    Mutex_Lock (queue->mWriteMutex);

    // Dont serialize the message more than once.
    if (message->serialized == NULL)
    {
        if (!message->serialize(message, queue->mode))
        {
            rzb_log(LOG_ERR, "%s: Failed to serialize message", __func__);
            Mutex_Unlock (queue->mWriteMutex);
            return false;
        }
    }


    if ((stompMessage = Queue_Message_Create("SEND")) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to create SEND", __func__);
        Mutex_Unlock (queue->mWriteMutex);
        return false;
    }
    List_Destroy(stompMessage->headers);
    stompMessage->headers = message->headers;
    stompMessage->pBody = message->serialized;
    stompMessage->bodyLength = message->length;
#ifdef _MSC_VER
#define STOMP_LEN_FMT "%Iu"
#define STOMP_MID_FMT "message-%u"
#else
#define STOMP_LEN_FMT "%zu"
#define STOMP_MID_FMT "message-%ju"
#endif
    if (asprintf(&messageId, STOMP_MID_FMT, (uintmax_t)curTime) == -1)
    {
        stompMessage->pBody = NULL; // Wipe this so that the message code free's it.
        stompMessage->headers = NULL;
        Queue_Destroy_Stomp_Message(stompMessage);
        Mutex_Unlock (queue->mWriteMutex);
        return false;
    }

    if (asprintf(&messageLen, STOMP_LEN_FMT, stompMessage->bodyLength) == -1)
    {
        stompMessage->pBody = NULL; // Wipe this so that the message code free's it.
        stompMessage->headers = NULL;
        Queue_Destroy_Stomp_Message(stompMessage);
        Mutex_Unlock (queue->mWriteMutex);
        return false;
    }
    if (asprintf(&messageType, "%u", message->type) == -1)
    {
        stompMessage->pBody = NULL; // Wipe this so that the message code free's it.
        stompMessage->headers = NULL;
        Queue_Destroy_Stomp_Message(stompMessage);
        Mutex_Unlock (queue->mWriteMutex);
        return false;
    }
    if (asprintf(&messageVer, "%u", message->version) == -1)
    {
        stompMessage->pBody = NULL; // Wipe this so that the message code free's it.
        stompMessage->headers = NULL;
        Queue_Destroy_Stomp_Message(stompMessage);
        Mutex_Unlock (queue->mWriteMutex);
        return false;
    }

    if (!Message_Add_Header(stompMessage->headers, "receipt", messageId) ||
            !Message_Add_Header(stompMessage->headers, "destination", dest) ||
            !Message_Add_Header(stompMessage->headers, "amq-msg-type", "bytes") ||
            !Message_Add_Header(stompMessage->headers, "content-length", messageLen) ||
            !Message_Add_Header(stompMessage->headers, "rzb-msg-type", messageType) ||
            !Message_Add_Header(stompMessage->headers, "rzb-msg-ver", messageVer))
    {
        rzb_log(LOG_ERR, "%s: Failed to add ack message-id headers", __func__);
        stompMessage->pBody = NULL; // Wipe this so that the message code free's it.
        stompMessage->headers = NULL;
        Queue_Destroy_Stomp_Message(stompMessage);
        Mutex_Unlock (queue->mWriteMutex);
        return false;
    }
    free(messageLen);
    free(messageType);
    free(messageVer);

    while (!Queue_Send_Message(queue->pWriteSocket, stompMessage))
    {
		if (errno != EINTR)
        {
            while (!Queue_Reconnect(queue))
                rzb_log(LOG_ERR, "%s: Reconnecting", __func__);
            continue;
        }
        rzb_log(LOG_ERR, "%s: Failed to send message", __func__);
        stompMessage->pBody = NULL; // Wipe this so that the message code free's it.
        stompMessage->headers = NULL;
        Queue_Destroy_Stomp_Message(stompMessage);
        Mutex_Unlock (queue->mWriteMutex);
        return false;
    }

    stompMessage->headers = NULL;
    stompMessage->pBody = NULL; // Wipe this so that the message code free's it.
    Queue_Destroy_Stomp_Message(stompMessage);

    if (( stompMessage = Queue_Read_Message (queue->pWriteSocket)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Queue_Read_Message", __func__);
        Mutex_Unlock (queue->mWriteMutex);
        return false;
    }
    if (strcasecmp(stompMessage->sVerb, "RECEIPT") == 0)
    {
        if ((l_pReceiptId = Queue_Message_Get_Header(stompMessage, "receipt-id")) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to get receipt-id", __func__);
            Mutex_Unlock (queue->mWriteMutex);
            Queue_Destroy_Stomp_Message(stompMessage);
            return false;
        }
        if (strcmp(l_pReceiptId, messageId) != 0)
        {
            rzb_log(LOG_ERR, "%s: receipt-id did not match sent message: %s, %s", __func__, l_pReceiptId, messageId);
            Mutex_Unlock (queue->mWriteMutex);
            Queue_Destroy_Stomp_Message(stompMessage);
            return false;
        }
    }
    free(messageId);
    Queue_Destroy_Stomp_Message(stompMessage);
 
    Mutex_Unlock (queue->mWriteMutex);
    return true;
}

SO_PUBLIC void
Queue_GetQueueName (const char * p_sLeading, uuid_t p_pId,
                    char * p_sQueueName)
{
    char l_sUUID[UUID_STRING_LENGTH];

    uuid_unparse (p_pId, l_sUUID);
    sprintf (p_sQueueName, "%s.%s", p_sLeading, l_sUUID);
}
