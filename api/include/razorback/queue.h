/** @file queue.h
 * Queue structures and functions
 * IMPLEMENTS:
 * STOMP PROTOCOL
 * Creative Commons Attribution v2.5
 * http://stomp.codehaus.org/Protocol
 */

#ifndef	RAZORBACK_QUEUE_H
#define	RAZORBACK_QUEUE_H
#include <razorback/visibility.h>
#include <razorback/types.h>
#include <razorback/socket.h>
#include <razorback/lock.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COMMAND_QUEUE "/topic/COMMAND"
#define REQUEST_QUEUE "/queue/REQUEST"
#define LOG_QUEUE "/queue/LOG"
#define INPUT_QUEUE "/queue/INPUT"
#define JUDGMENT_QUEUE "/queue/JUDGMENT"

/** Queue
 * an attachment to a distributed message queue
 */
struct Queue
{
    struct Socket *pReadSocket;      ///< the socket for this queue
    struct Socket *pWriteSocket;      ///< the socket for this queue
    char *sName;                ///< The Queue name
    int iFlags;              ///< flags
    int mode;
    struct Mutex *mReadMutex; ///< The Read Lock
    struct Mutex *mWriteMutex; ///< The write lock
};

#define QUEUE_FLAG_SEND 0x01
#define QUEUE_FLAG_RECV 0x02

/** Initializes the queue
 * @param *p_sQueueName the name of the queue
 * @param *p_bSubscribe whether you need to read from the queue
 * @return a pointer to a new Queue Struct or NULL on error.
 */
SO_PUBLIC extern struct Queue *Queue_Create (const char * p_sQueueName,
                                       int p_iFlags, int mode);

/** Terminates the queue
 * @param p_pQ the queue to terminate
 */
SO_PUBLIC extern void Queue_Terminate (struct Queue *p_pQ);

/** Gets a binary buffer from the queue
 * @param p_pQ the queue
 * @param p_pBuffer the binary buffer
 * @return true if ok, false if error or timeout (errno==EAGAIN if timeout)
 */
SO_PUBLIC extern struct Message *Queue_Get (struct Queue *queue);

/** Puts a binary buffer into the queue
 * @param p_pQueue the queue
 * @param p_pBuffer the binary buffer
 * @return true if ok, false if error or timeout (errno==EAGAIN if timeout)
 */
SO_PUBLIC extern bool Queue_Put_Dest (struct Queue *queue, struct Message *message, char *dest);
SO_PUBLIC extern bool Queue_Put (struct Queue *queue, struct Message *message);

/** Gets a queue name from a uuid_t
 * @param p_sLeading the leading text
 * @param p_pId the uuid
 * @param p_sQueueName the destination text
 */
SO_PUBLIC extern void Queue_GetQueueName (const char * p_sLeading,
                                uuid_t p_pId, char * p_sQueueName);
#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_QUEUE_H
