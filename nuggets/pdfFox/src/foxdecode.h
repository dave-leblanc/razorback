#ifndef PDF_FOX_DECODE_H
#define PDF_FOX_DECODE_H

#include <zlib.h>
#include <string.h>
#include "foxtoken.h"
#include "foxparse.h"

#define ZLIB_CHUNK 16384

extern void streamDecode(PDFToken *token, PDFSyntaxNode *filter);
extern void testdec(PDFToken *token);

#endif
