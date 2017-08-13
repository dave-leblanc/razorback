#include <razorback/types.h>
#include <razorback/binary_buffer.h>
#include <razorback/queue.h>
#include <razorback/log.h>

#include <stdio.h>
#include <string.h>
int main()
{

    struct Queue *l_pTestQueue;

    // connect to the queue
    if ((l_pTestQueue = Queue_Create
         ((const uint8_t *) "/queue/Q.TEST", QUEUE_FLAG_RECV|QUEUE_FLAG_SEND)) == NULL)
    {
        rzb_log (LOG_ERR, "initialize fail");
        return -1;
    }

    // create a binary buffer
    struct BinaryBuffer *l_pBuffer;

    // write and read messages
    uint32_t l_iIndex;
    uint8_t l_sTestString[1024];

    for (l_iIndex = 0; l_iIndex < 1000; l_iIndex++)
    {

        sprintf ((char *) l_sTestString, "Hello World %i!", l_iIndex);
        if ((l_pBuffer = BinaryBuffer_Create ( strlen((char *)l_sTestString)+1)) == NULL )
        {
            rzb_log (LOG_ERR, "bb_create fail");
            return -1;
        }
        BinaryBuffer_Put_String (l_pBuffer,
                                 (const uint8_t *) l_sTestString);
        if (!Queue_Put
            (l_pTestQueue, l_pBuffer))
        {
            rzb_log (LOG_ERR, "put fail");
            return -1;
        }
        BinaryBuffer_Destroy(l_pBuffer);
        if ((l_pBuffer = Queue_Get
            (l_pTestQueue)) == NULL )
        {
            rzb_log (LOG_ERR, "get fail");
            return -1;
        }
        uint8_t *l_sMessage;
        l_sMessage = BinaryBuffer_Get_String (l_pBuffer);
        BinaryBuffer_Destroy (l_pBuffer);
        rzb_log (LOG_INFO, "Message Text: (%s)", (const char *) l_sMessage);
		free (l_sMessage);
    }

    // clean up
    Queue_Terminate (l_pTestQueue);
    return 0;
}

