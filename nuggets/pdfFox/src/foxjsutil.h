#ifndef PDF_FOX_JSUTIL_H
#define PDF_FOX_JSUTIL_H

#include <stdint.h>
#define JSARG_VECTOR_INIT_SIZE 10

typedef struct _JSArgList {
    int count;
    char **vector;
} JSArgList;

extern JSArgList *JSFindFunction (char *content, uint32_t length, const char *function);
extern void destroyJSArgList (JSArgList *list);

#endif
