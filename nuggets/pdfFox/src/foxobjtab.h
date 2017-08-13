#ifndef PDF_FOX_OBJTAB_H
#define PDF_FOX_OBJTAB_H

#include <stdint.h>

typedef struct _ObjTableEntry {
	struct _ObjTableEntry *next;
    uint32_t refnum;
    uint32_t revnum;
    uint32_t length;
    long int offset;
    int fullyResolved;
} ObjTableEntry;

ObjTableEntry *getNewObjTabEntry();
ObjTableEntry *getNextUnresolved();
ObjTableEntry *getPDFObjEntry (uint32_t refnum, uint32_t revnum);
int addPDFObjEntry (ObjTableEntry *);

#endif
