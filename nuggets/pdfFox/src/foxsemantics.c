#include "foxobjtab.h"
#include "foxparse.h"
#include "foxreport.h"
#include "foxdetect.h"
#include <stdint.h>
#include <stdbool.h>

static uint32_t currentStreamLength = 0;
static uint32_t containsUnresolved = 0; 
static PDFStreamType streamState = UNKNOWN;

void setStreamLength(uint32_t length) {
	currentStreamLength = length;
}

uint32_t getStreamLength() {
	return currentStreamLength;
}

void setUnresolved(uint32_t value) {
	containsUnresolved = value;
}

uint32_t getUnresolved() {
	return containsUnresolved;
}

void setStreamState(PDFStreamType state) {
	streamState = state;
}

PDFStreamType getStreamState() {
	return streamState;
}

/**This function attempts to resolve all hitherto unresolved indirect objects
 * once parsing has reached the end of the file.
 *
 * If all indirect references have a corresponding object, this phase
 * should take care of them.
 */
uint32_t resolveIndirect(FILE *file, PDFSyntaxNode *node) {
    ObjTableEntry *entry;
    uint32_t ref = (uint32_t)strtoul((char *)node->child[0]->token->content, NULL, 10);
    uint32_t rev = (uint32_t)strtoul((char *)node->child[1]->token->content, NULL, 10);

    entry = getPDFObjEntry(ref, rev);
    if (entry == NULL) {
        foxLog(NONFATAL, "%s: Could not retrieve object from reference.\n", __func__);
        containsUnresolved = 1;
        return 2;
    }

    if (fseek(file, entry->offset, SEEK_SET) != 0) {
        foxLog(FATAL, "%s: Can't figure out starting pos in file.\n", __func__);
		return 0;
    }

    return 1;
}


bool object_Sem(PDFSyntaxNode *node, uint32_t length, long int offset) {
    ObjTableEntry *entry = getNewObjTabEntry();
    if (entry == NULL) {
		return false;
	}

	//XXX: Works, but ugly
    entry->refnum = (uint32_t)strtoul((char *)node->child[0]->token->content, NULL, 10);
    entry->revnum = (uint32_t)strtoul((char *)node->child[1]->token->content, NULL, 10);
    entry->length = length;
    entry->offset = offset;
    if (containsUnresolved == 1) {
        entry->fullyResolved = 0;
        containsUnresolved = 0;
    }
    else
        entry->fullyResolved = 1;

    if (addPDFObjEntry (entry) == 0) {
		free(entry);
	}

	return true;
}

//XXX TOTAL PoS
//    Redo this immediately
DecodeParams *prepDecodeParams(PDFSyntaxNode *node) {
    
	PDFSyntaxNode *temp;

    DecodeParams *DParams = (DecodeParams *)calloc(1, sizeof(DecodeParams));
    if (DParams == NULL)
		return NULL;

    if (node->child[0]) {
        temp = node->child[0];
        //Entry
        while (temp) {
            //Should have a name
            if (!temp->child[0])
                continue;

            if (!temp->child[0]->token)
                continue;

            switch (temp->child[0]->token->type) {
                case NAME_DCPRMS_COLUMNS:
                    if (!temp->child[1])
                        continue;

                    if (!temp->child[1]->token)
                        continue;

                    if (temp->child[1]->token->type != INTEGER)
                        continue;

                    DParams->columns = (uint32_t)strtoul((char *)temp->child[1]->token->content, NULL, 10);

                    break;

                case NAME_DCPRMS_BPC:
                    if (!temp->child[1])
                        continue;

                    if (!temp->child[1]->token)
                        continue;

                    if (temp->child[1]->token->type != INTEGER)
                        continue;

                    DParams->bitspercomp = (uint32_t)strtoul((char *)temp->child[1]->token->content, NULL, 10);

                    break;

                case NAME_DCPRMS_COLORS:
                    if (!temp->child[1])
                        continue;

                    if (!temp->child[1]->token)
                        continue;

                    if (temp->child[1]->token->type != INTEGER)
                        continue;

                    DParams->colors = (uint32_t)strtoul((char *)temp->child[1]->token->content, NULL, 10);
                    break;

                case NAME_DCPRMS_PREDICTOR:
                    if (!temp->child[1])
                        continue;

                    if (!temp->child[1]->token)
                        continue;

                    if (temp->child[1]->token->type != INTEGER)
                        continue;

                    DParams->predictor = (uint32_t)strtoul((char *)temp->child[1]->token->content, NULL, 10);

                    break;

                default:
                    if (!temp->child[1])
                        continue;

                    if (!temp->child[1]->token)
                        continue;

                    if (temp->child[1]->token->type != INTEGER)
                        continue;

                    DParams->earlychange = (uint32_t)strtoul((char *)temp->child[1]->token->content, NULL, 10);

                    break;
            }

            temp = temp->sibling;
        }
    }

    return DParams;
}

