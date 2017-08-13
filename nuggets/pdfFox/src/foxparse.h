#ifndef PDF_FOX_PARSE_H
#define PDF_FOX_PARSE_H

#include "foxdebug.h"

/**Definition of nodetype for tree searches...
 * Not currently used.
 */
typedef enum {BRANCH, TERMINAL} NodeType;

/**Main defintion of a syntax tree node.
 * Contains pointers to optional child nodes (node of higher grammatical precedence)
 * as well as one to a sibling node (node of equal or lower precedence).
 * Also contains pointer to an associated token.
 */
typedef struct _PDFSyntaxNode {
    struct _PDFSyntaxNode *sibling;
    struct _PDFSyntaxNode *child[3];
    NodeType type;
    PDFToken *token;
} PDFSyntaxNode;

/**Parser utility functions.
 */
extern PDFSyntaxNode *getNewNode();
extern bool createPDFTree(FILE *);

#endif
