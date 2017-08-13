#include "foxtoken.h"
#include "foxobjtab.h"
#include "foxreport.h"

#define OBJ_TABLE_NUM_ENTRIES 4111 //Prime number

static ObjTableEntry *table[OBJ_TABLE_NUM_ENTRIES];

/*
 * Table Definition and Declaration
 */
typedef struct _PDFObjectTable {
	uint32_t numEntries;
	uint32_t tableSize;
	ObjTableEntry **table;
} PDFObjectTable;

static PDFObjectTable ObjTable;

/*
 * Constructor
 *
 * Initialized Object Table
 */
__attribute__ ((constructor)) void initObjTable () {
    int i;
	for (i = 0; i < OBJ_TABLE_NUM_ENTRIES; i++) {
		table[i] = NULL;
	}
	
	ObjTable.table = table;
	ObjTable.numEntries = 0;
	ObjTable.tableSize = OBJ_TABLE_NUM_ENTRIES;
}

/*
 * Find slot for object
 */
static uint32_t hashObj (ObjTableEntry *entry) {
	return (entry->refnum % OBJ_TABLE_NUM_ENTRIES);
}

/*
 *
 * Initializes new object table entry
 *
 */
ObjTableEntry *getNewObjTabEntry() {
	ObjTableEntry *entry;

	if ((entry = calloc(1, sizeof(ObjTableEntry))) == NULL) {
		foxLog(FATAL, "%s: Could not allocate memory.\n", __func__);
		return NULL;
	}

	return entry;
}

/*
 *
 * Adds an entry to the table by ref, then rev
 *
 */
int addPDFObjEntry (ObjTableEntry *entry) {

    uint32_t index;
	ObjTableEntry *temp;

	index = hashObj(entry);

	if (table[index] == NULL) {
		table[index] = entry;
	}
	else {
        temp = table[index];
	
	    if (temp->refnum == entry->refnum && temp->revnum == entry->revnum) {
		    foxLog(NONFATAL, "%s: Entry already exists with that ref/rev.\n", __func__);
		    return 0;
	    }

		while (temp->next != NULL) {
            temp = temp->next;

            if (temp->refnum == entry->refnum && temp->revnum == entry->revnum) {
                foxLog(NONFATAL, "%s: Entry already exists with that ref/rev.\n", __func__);
                return 0;
            }   
		}
 
        temp->next = entry;
	}

    return 1;

}

/*
 * Retrieve entry by ref and rev
 *
 */
ObjTableEntry *getPDFObjEntry (uint32_t refnum, uint32_t revnum) {
    ObjTableEntry *entry;

	entry = table[refnum];

	while (entry != NULL) {
		if (entry->refnum == refnum && entry->revnum == revnum)
			return entry;

		entry = entry->next;
	}

	return NULL;
}

/*
 * Cycle through table and pass by the next
 * unresolved object on the table.
 *
 */
ObjTableEntry *getNextUnresolved() {
	static unsigned int index = 0, ready = 1;
	static ObjTableEntry *temp, *entry;

    for (;index < ObjTable.tableSize; index++) {
		if (ready)
		    temp = ObjTable.table[index];
		while (temp != NULL) {
			if (temp->fullyResolved == 0) {
				ready = 0;
				entry = temp;
				temp = temp->next;
				return entry;
			}
			temp = temp->next;
		}
		index++;
		ready = 1;
	}

	return NULL;
}

/*
 * This is for DEBUG purposes
 *  
 * Do one final check to see if we missed any indirect
 * references.
 *
 */
__attribute__ ((destructor)) void countUnresolved() {
	
	ObjTableEntry *temp, *entry;
#ifdef PDF_FOX_SHOW_DEBUG
	unsigned int index;
	uint32_t count = 0;

	for (index = 0; index < ObjTable.tableSize; index++) {
		temp = ObjTable.table[index];
		while (temp != NULL) {
			if (temp->fullyResolved == 0) {
			    count++;
			}
			temp = temp->next;
		}
		index++;
	}

	foxLog(PDF_DEBUG, "Remaining unresolved references = %d\n.", count);
#endif
    uint32_t i;

	for (i = 0; i < ObjTable.tableSize; i++) {
		temp = ObjTable.table[i];
		while (temp != NULL) {
			entry = temp->next;
			free(temp);
			temp = entry;
		}
		i++;
	}

}
