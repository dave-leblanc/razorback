#ifndef PDF_FOX_SEMANTICS_H
#define PDF_FOX_SEMANTICS_H

//State functions
extern void setStreamLength(uint32_t length);
extern uint32_t getStreamLength();
extern void setUnresolved(uint32_t value);
extern uint32_t getUnresolved();

//Semantic functions
extern bool object_Sem(PDFSyntaxNode *node, uint32_t length, long int offset);
extern uint32_t resolveIndirect(FILE *file, PDFSyntaxNode *node);
extern DecodeParams *prepDecodeParams(PDFSyntaxNode *node);

#endif
