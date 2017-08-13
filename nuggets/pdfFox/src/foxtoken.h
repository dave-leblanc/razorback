#ifndef PDF_FOX_TOKEN_H
#define PDF_FOX_TOKEN_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define BUF_IDENT_SIZE 100


/**Defines each token's type.
 * Each token has its own integer value
 */
typedef enum {TOK_ERROR, ARRAY, BINHEAD, COMMENT, CLOSEANGLE, CLOSEARRAY, CLOSEDICT, CLOSEPAREN, DECIMAL, DICTIONARY, END, ENDOBJ, ENDSTREAM, IDENT, MINUS, NAME, NAME_STRMLEN, FILTER, FILTER_HEXDECODE, FILTER_85DECODE, FILTER_LZWDECODE, FILTER_FLATEDECODE, FILTER_RLEDECODE, FILTER_CCITTDECODE, FILTER_JBIG2DECODE, FILTER_DCTDECODE, FILTER_JPXDECODE, FILTER_CRYPTDECODE, NEWLINE, HEXSTRING, INTEGER, LITSTRING, REAL, NULLOBJ, OBJ, OPENANGLE, OPENARRAY, OPENDICT, OPENPAREN, PDFVERS, PLUS, REF, STARTXREF, STREAM, STREAMCONTENT, TRAILER, XREF, TOK_TRUE, TOK_FALSE, ENDOFFILE, NAME_SUBTYPE, NAME_S, NAME_JAVASCRIPT, NAME_JS, NAME_TRUETYPE, NAME_OPENTYPE, NAME_DECODEPARAMS, NAME_URI, NAME_DCPRMS_COLUMNS, NAME_DCPRMS_BPC, NAME_DCPRMS_COLORS, NAME_DCPRMS_PREDICTOR } PDFTokenType;

/**Used by the lexical analyzer to track state.
 * States are generally active when tokenizing multi-character strings.
 */
typedef enum {START, INCOMMENT, INANGLE, INCLOSEANGLE, INIDENT, INNAME, INNUMBER, INREAL} PDFTokenizeState;

/**The main definition of the PDFToken type.
 * Contains the type of token as well as optional fields
 * for the particular set of characters attributed to that token
 * (for token types that are resolved from a pattern rather than 
 * a specific sequence of characters).
 */
typedef struct _PDFToken {
    PDFTokenType type;
    uint32_t length;
    uint8_t *content;
} PDFToken;

/**Utility functions for tokenizing.
 * Used by the Parser.
 */
extern PDFToken *getNextToken(FILE *file);
extern PDFToken *newPDFToken();
extern void checkNameKeyword(PDFToken *token);
extern void destroyPDFToken(PDFToken *token);
extern PDFToken *tokenizeStream(FILE *file, uint32_t length);
extern PDFToken *tokenizeHexString(FILE *file);
extern PDFToken *tokenizeLitString(FILE *file);
extern bool tokenizeRef(FILE *file);
extern bool isWhitespace(char c);

#endif
