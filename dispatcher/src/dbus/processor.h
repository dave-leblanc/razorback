#ifndef DBUS_PROCESSOR_H
#define DBUS_PROCESSOR_H

#include <razorback/types.h>

void DBus_Init(void);
bool DBus_Push_Update(void);
bool DBus_File_Delete(struct BlockId *bid, uint8_t locality);

#endif
