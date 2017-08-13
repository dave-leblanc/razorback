#ifndef DBUS_MESSAGES_H
#define DBUS_MESSAGES_H
#include <razorback/types.h>
#include <razorback/list.h>

#define MESSAGE_GROUP_DBUS    0x01000000  ///< Submission
#define MESSAGE_TYPE_RT_ANNOUNCE    ( MESSAGE_GROUP_DBUS | 1 )
#define MESSAGE_TYPE_RT_UPDATE      ( MESSAGE_GROUP_DBUS | 2 )
#define MESSAGE_TYPE_RT_REQUEST     ( MESSAGE_GROUP_DBUS | 3 )
#define MESSAGE_TYPE_FILE_DELETE    ( MESSAGE_GROUP_DBUS | 4 )

#define RT_UPDATE_REASON_CHANGE 1
#define RT_UPDATE_REASON_REQUEST 2

struct MessageRT_Announce
{
    uint64_t serial;
};

struct MessageRT_Update
{
    uint64_t serial;
    uint8_t reason;
    struct List *list;
};

struct MessageFile_Delete
{
	uint8_t localityId;
	struct BlockId *bid;
};

void DBus_RT_Announce_Init(void);
struct Message * RT_Announce_Initialize (void);

void DBus_RT_Update_Init(void);
struct Message * RT_Update_Initialize (uint8_t reason);
struct Message * RT_Update_Initialize_To (uint8_t reason, uuid_t dest);

void DBus_RT_Request_Init(void);
struct Message * RT_Request_Initialize (uuid_t dest);

void DBus_File_Delete_Init(void);
struct Message * File_Delete_Initialize (struct BlockId *bid, uint8_t locality);

#endif
