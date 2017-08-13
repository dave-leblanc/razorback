/** @file messages.h
 * Razorback API messages.
 */
#ifndef RAZORBACK_MESSAGES_H
#define RAZORBACK_MESSAGES_H

#include <razorback/visibility.h>
#include <razorback/message_formats.h>
#include <razorback/types.h>
#include <razorback/api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MESSAGE_MODE_BIN 1
#define MESSAGE_MODE_JSON 2

/** Message Groups
 * @{
 */
#define MESSAGE_GROUP_C_AND_C   0x10000000  ///< Command And Control
#define MESSAGE_GROUP_CACHE     0x20000000  ///< Global Cache
#define MESSAGE_GROUP_SUBMIT    0x40000000  ///< Submission
#define MESSAGE_GROUP_OUTPUT    0x80000000  ///< Submission
/// @}
/** Command And Control Messages
 * @{
 */
#define MESSAGE_TYPE_HELLO          ( MESSAGE_GROUP_C_AND_C | 1 )   ///< Hello Message
#define MESSAGE_TYPE_REG_REQ        ( MESSAGE_GROUP_C_AND_C | 2 )   ///< Registration Request
#define MESSAGE_TYPE_REG_RESP       ( MESSAGE_GROUP_C_AND_C | 3 )   ///< Registration Response
#define MESSAGE_TYPE_REG_ERR        ( MESSAGE_GROUP_C_AND_C | 4 )   ///< Registration Error
#define MESSAGE_TYPE_CONFIG_UPDATE  ( MESSAGE_GROUP_C_AND_C | 5 )   ///< Configuration Update Notice
#define MESSAGE_TYPE_CONFIG_ACK     ( MESSAGE_GROUP_C_AND_C | 6 )   ///< Configuration Updated OK
#define MESSAGE_TYPE_CONFIG_ERR     ( MESSAGE_GROUP_C_AND_C | 7 )   ///< Configuration Update Failed
#if 0
#define MESSAGE_TYPE_STATS_REQ      ( MESSAGE_GROUP_C_AND_C | 8 )   ///< Statistics Request
#define MESSAGE_TYPE_STATS_RESP     ( MESSAGE_GROUP_C_AND_C | 9 )   ///< Statistics Response (With Data)
#define MESSAGE_TYPE_STATS_ERR      ( MESSAGE_GROUP_C_AND_C | 10 )  ///< Statistics Error
#endif
#define MESSAGE_TYPE_PAUSE          ( MESSAGE_GROUP_C_AND_C | 11 )  ///< Pause Processing
#define MESSAGE_TYPE_PAUSED         ( MESSAGE_GROUP_C_AND_C | 12 )  ///< Pause Confirmation
#define MESSAGE_TYPE_GO             ( MESSAGE_GROUP_C_AND_C | 13 )  ///< Unpause Processing
#define MESSAGE_TYPE_RUNNING        ( MESSAGE_GROUP_C_AND_C | 14 )  ///< Unpuase Confirmation
#define MESSAGE_TYPE_TERM           ( MESSAGE_GROUP_C_AND_C | 15 )  ///< Terminate Processing
#define MESSAGE_TYPE_BYE            ( MESSAGE_GROUP_C_AND_C | 16 )  ///< Terminating Processing
#define MESSAGE_TYPE_CLEAR          ( MESSAGE_GROUP_C_AND_C | 17 )  ///< Clear Local Cache
#define MESSAGE_TYPE_REREG          ( MESSAGE_GROUP_C_AND_C | 18 )  ///< Reregister
/// @}

/** Global Cache Messages
 * @{
 */
#define MESSAGE_TYPE_REQ            ( MESSAGE_GROUP_CACHE | 1 ) ///< Global Cache Request
#define MESSAGE_TYPE_RESP           ( MESSAGE_GROUP_CACHE | 2 ) ///< Global Cache Response
/// @}

/** Submission Messages
 * @{
 */
#define MESSAGE_TYPE_BLOCK          ( MESSAGE_GROUP_SUBMIT | 1 )    ///< Data Block Submission
#define MESSAGE_TYPE_JUDGMENT       ( MESSAGE_GROUP_SUBMIT | 2 )    ///< Judgment Submission
#define MESSAGE_TYPE_INSPECTION     ( MESSAGE_GROUP_SUBMIT | 3 )    ///< Inspection Submission
#define MESSAGE_TYPE_LOG            ( MESSAGE_GROUP_SUBMIT | 4 )    ///< Log Message
#define MESSAGE_TYPE_ALERT          ( MESSAGE_GROUP_SUBMIT | 5 )    ///< Alert Message
/// @}

/** Output Messages
 * @{
 */
#define MESSAGE_TYPE_ALERT_PRIMARY      ( MESSAGE_GROUP_OUTPUT | 1 )    ///< Block turns to bad
#define MESSAGE_TYPE_ALERT_CHILD        ( MESSAGE_GROUP_OUTPUT | 2 )    ///< Block turns to bad due to child
#define MESSAGE_TYPE_OUTPUT_EVENT       ( MESSAGE_GROUP_OUTPUT | 3 )    ///< Event Record
#define MESSAGE_TYPE_OUTPUT_LOG         ( MESSAGE_GROUP_OUTPUT | 4 )    ///< Log Record
#define MESSAGE_TYPE_OUTPUT_INSPECTION  ( MESSAGE_GROUP_OUTPUT | 5 )    ///< Inspection status
/// @}


/** Message Versions
 * @{
 */
#define MESSAGE_VERSION_1 1
/// @}

#define MSG_CNC_HEADER_SOURCE   "Source_Nugget"
#define MSG_CNC_HEADER_DEST     "Dest_Nugget"

#define DISP_HELLO_FLAG_LM (1 << 0)
#define DISP_HELLO_FLAG_LS (1 << 1)
#define DISP_HELLO_FLAG_DD (1 << 2)
#define DISP_HELLO_FLAG_LF (1 << 3)

/** Message handler structure
 */
struct MessageHandler
{
    uint32_t type;											///< Message type code
    bool (*serialize)(struct Message *msg, int mode);		///< Serialization function pointer
    bool (*deserialize)(struct Message *msg, int mode);		///< Deserialization function pointer
    void (*destroy)(struct Message *msg);					///< Destructor function pointer
};

/** Create a list for storing message headers
 * @return A new list on success, NULL on failure.
 */
SO_PUBLIC extern struct List * Message_Header_List_Create(void);

/** Add a header to a header list structure.
 * @param headers The header list.
 * @param name The header name.
 * @param value The header value.
 * @return true on success false on failure
 */
SO_PUBLIC extern bool Message_Add_Header(struct List *headers, const char *name, const char *value);

/** Register a message handler
 * @param handler The handler structure.
 * @return true on success, false on error.
 */
SO_PUBLIC extern bool Message_Register_Handler(struct MessageHandler *handler);

/** Get nugget ID headers from a message
 * @param message The message
 * @param source The UUID to store the source nugget id in.
 * @param dest The UUID to store the destination nugget in.
 * @return true on success, false on error.
 */
SO_PUBLIC extern bool Message_Get_Nuggets(struct Message *message, uuid_t source, uuid_t dest);

/** Deserialize an empty message
 * @param message The message
 * @param mode Messaging mode.
 * @return true on success, false on failure.
 */
SO_PUBLIC extern bool Message_Deserialize_Empty(struct Message *message, int mode);

/** Deserialize an empty message
 * @param message The message
 * @param mode Messaging mode.
 * @return true on success, false on failure.
 */
SO_PUBLIC extern bool Message_Serialize_Empty(struct Message *message, int mode);

/** Create a message with directed message headers on a topic.
 * @param type Message type.
 * @param version Message version
 * @param msgSize Message size
 * @param source Source nugget UUID
 * @param dest Destination nugget UUID
 * @return A new message on success, NULL on failure.
 */
SO_PUBLIC extern struct Message * Message_Create_Directed(uint32_t type, uint32_t version, size_t msgSize, const uuid_t source, const uuid_t dest);

/** Create a message with broadcast message headers on a topic.
 * @param type Message type.
 * @param version Message version
 * @param msgSize Message size
 * @param source Source nugget UUID
 * @return A new message on success, NULL on failure.
 */
SO_PUBLIC extern struct Message * Message_Create_Broadcast(uint32_t type, uint32_t version, size_t msgSize, const uuid_t source);

/** Destroy a message.
 * @param message The message to destroy.
 */
SO_PUBLIC extern void Message_Destroy(struct Message *message);


/** Initializes message cache request
 * @param requestorUUID the requester
 * @param blockId the block id
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message*  MessageCacheReq_Initialize (
                                        const uuid_t requestorUUID,
                                        const struct BlockId *blockId);


/** Initializes message cache response
 * @param iblockId the block id
 * @param sfFlags  the code
 * @param entFlags the code
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message* MessageCacheResp_Initialize (
                                         const struct BlockId *blockId,
                                         uint32_t sfFlags, uint32_t entFlags);


/** Initializes message block submission
 * @param event The event
 * @param reason The submission reason
 * @param locality The locality the block was stored in.
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message*  MessageBlockSubmission_Initialize (
                                               struct Event *event,
                                               uint32_t reason, uint8_t locality);


/** Initializes message block submission
 * @param reason The judgment type.
 * @param judgment The judgment data.
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message* MessageJudgmentSubmission_Initialize (
                                                  uint8_t reason,
                                                  struct Judgment* judgment);

/** Initializes message inspection submission
 * @param event The event
 * @param reason the reason
 * @param localityCount The number of localities the block is stored in
 * @param locality Array of localities the block is stored in
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message* MessageInspectionSubmission_Initialize (
                                                    const struct Event *event,
                                                    uint32_t reason,
                                                    uint32_t localityCount,
                                                    uint8_t *localities
                                                    );

/** Initializes a hello message
 * @param context The context of this message.
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message* MessageHello_Initialize (struct RazorbackContext *context);


/** Initializes a registration request message
 * @param disptacherId Dispatcher to send the request to.
 * @param sourceNugget the source
 * @param nuggetTye the nugget type
 * @param applicationType the application type
 * @param dataTypeCount the number of data types
 * @param dataTypeList the data types
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message* MessageRegistrationRequest_Initialize (
                                                   const uuid_t dispatcherId, 
                                                   const uuid_t sourceNugget,
                                                   const uuid_t nuggetType,
                                                   const uuid_t applicationType,
                                                   uint32_t dataTypeCount,
                                                   uuid_t * dataTypeList);


/** Initializes a registration response message
 * @param sourceNugget the source
 * @param destNugget the destination
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message* MessageRegistrationResponse_Initialize (
                                                    const uuid_t sourceNugget,
                                                    const uuid_t destNugget);

/** Initializes a configuration update message
 * @param sourceNugget the source
 * @param destNugget the destination
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message * MessageConfigurationUpdate_Initialize (
                                                   const uuid_t sourceNugget,
                                                   const uuid_t destNugget);


/** Initializes a configuration acknowledgment message
 * @param sourceNugget the source
 * @param destNugget the destination
 * @param nuggetType the type of nugget
 * @param applicationType the application type
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message* MessageConfigurationAck_Initialize (
                                                const uuid_t sourceNugget,
                                                const uuid_t sestNugget,
                                                const uuid_t nuggetType,
                                                const uuid_t applicationType);


/** Initializes a pause message
 * @param sourceNugget the source
 * @param destNugget the destination
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message * MessagePause_Initialize (
                                     const uuid_t sourceNugget,
                                     const uuid_t destNugget);


/** Initializes a paused message
 * @param sourceNugget the source
 * @param destNugget the destination
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message* MessagePaused_Initialize (
                                      const uuid_t sourceNugget,
                                      const uuid_t destNugget);


/** Initializes a go message
 * @param sourceNugget the source
 * @param destNugget the destination
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message * MessageGo_Initialize (
                                  const uuid_t sourceNugget,
                                  const uuid_t destNugget);



/** Initializes a running message
 * @param sourceNugget the source
 * @param destNugget the destination
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message * MessageRunning_Initialize (
                                       const uuid_t sourceNugget,
                                       const uuid_t destNugget);


/** Initializes a terminate message
 * @param sourceNugget the source
 * @param destNugget the destination
 * @param reason the reason
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message * MessageTerminate_Initialize (
                                         const uuid_t sourceNugget,
                                         const uuid_t destNugget,
                                         const uint8_t * reason);


/** Initializes a bye message
 * @param sourceNugget the source
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message * MessageBye_Initialize (
                                   const uuid_t sourceNugget);


/** Initialize a cache clear message
 * @param sourceNugget the source
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message * MessageCacheClear_Initialize (
                                   const uuid_t sourceNugget);

/** Initialize a re-registration message.
 * @param sourceNugget The source
 * @param destNugget The destination
 * @return A new message on success, NULL On failure.
 */
SO_PUBLIC struct Message *
MessageReReg_Initialize (
                         const uuid_t sourceNugget,
                         const uuid_t destNugget);

/** Initialize an error message
 * @param errorCode The error code
 * @param message The message text
 * @param sourceNugget The source
 * @param destNugget The destination
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message * MessageError_Initialize (
                                    uint32_t errorCode,
                                    const char *message,
                                    const uuid_t sourceNugget,
                                    const uuid_t destNugget);

/** Initialize a log message
 * @param nuggetId The source nugget.
 * @param priority The message level
 * @param message The message text.
 * @param eventId The optional event id, use NULL if no event is linked to this log message.
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message *
MessageLog_Initialize (
                         const uuid_t nuggetId,
                         uint8_t priority,
                         char *message,
                         struct EventId *eventId);

/** Initialize a primary alert message
 * @param event The event that triggered inspection
 * @param block The block that triggered the alert.
 * @param metadata The alert metadata.
 * @param nugget The inspector that triggered the alert.
 * @param judgment The judgment data from the inspector.
 * @param new_SF_Flags Post alert block SF Flags
 * @param new Ent_Flags Post alert block Enterprise flags.
 * @param old_SF_Flags Pre alert block SF Flags
 * @param old_Ent_Flags Pre alert block Enterprise flags.
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message *
MessageAlertPrimary_Initialize (
                                   struct Event *event,
                                   struct Block *block,
                                   struct List *metadata,
                                   struct Nugget *nugget,
                                   struct Judgment *judgment,
                                   uint32_t new_SF_Flags, uint32_t new_Ent_Flags,
                                   uint32_t old_SF_Flags, uint32_t old_Ent_Flags);

/** Initialize a child alert message
 * @param block The block that has changed state.
 * @param child The child block that triggered the state change.
 * @param nugget The nugget that sent the alert.
 * @param parentCount The number of blocks that have this block as a parent.
 * @param eventCount The number of events associated with this block.
 * @param new_SF_Flags Post alert block SF Flags
 * @param new Ent_Flags Post alert block Enterprise flags.
 * @param old_SF_Flags Pre alert block SF Flags
 * @param old_Ent_Flags Pre alert block Enterprise flags.
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message *
MessageAlertChild_Initialize (
                                   struct Block *block,
                                   struct Block *child,
                                   struct Nugget *nugget,
                                   uint64_t parentCount, uint64_t eventCount,
                                   uint32_t new_SF_Flags, uint32_t new_Ent_Flags,
                                   uint32_t old_SF_Flags, uint32_t old_Ent_Flags);

/** Initialize an event output message
 * @param event The event that has occurred.
 * @param nugget The nugget that created the event.
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message *
MessageOutputEvent_Initialize (
                                   struct Event *event,
                                   struct Nugget *nugget);

/** Initialize a log output message
 * @param log The log message received to be output
 * @param nugget The nugget that sent the log message.
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC extern struct Message *
MessageOutputLog_Initialize (
                                   struct MessageLogSubmission *log,
                                   struct Nugget *nugget);

/** Initialize an inspection status output message.
 * @param nugget The nugget that is inspecting the block
 * @param blockId The block ID of the block being inspected.
 * @param reason The reason for this output.
 * @param final Is this the last inspection to complete.
 * @return A new message on success,  NULL On failure.
 */
SO_PUBLIC struct Message *
MessageOutputInspection_Initialize (
                                   struct Nugget *nugget,
                                   struct BlockId *blockId,
                                   uint8_t reason,
                                   bool final);

#ifdef __cplusplus
}
#endif
#endif //RAZORBACK_MESSAGES_H
