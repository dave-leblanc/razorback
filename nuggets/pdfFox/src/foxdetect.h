#ifndef PDF_FOX_DETECT_H
#define PDF_FOX_DETECT_H

typedef enum { UNKNOWN, JAVASCRIPT, U3D, TIFF, URI, DECODEPARAMS, SUBTYPE, TRUEOPENTYPE, JBIG2, JPX} PDFStreamType;
typedef struct _DecodeParams {
	uint32_t predictor;
	uint32_t colors;
	uint32_t bitspercomp;
	uint32_t columns;
	uint32_t earlychange;
} DecodeParams;

void Dig (uint8_t *content, uint32_t length, PDFStreamType type);

#endif
