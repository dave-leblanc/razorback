#ifndef PDF_FOX_REPORT_H
#define PDF_FOX_REPORT_H

#include <stdint.h>
#include "config.h"

#ifdef RZB_PDF_FOX_NUGGET
#include <razorback/types.h>
#endif

#define PDF_FOX_SHOW_FATAL
//#define PDF_FOX_SHOW_NONFATAL
//#define PDF_FOX_SHOW_DEBUG

typedef enum {PRINT, FATAL, NONFATAL, PDF_DEBUG} REPORTMODE;

extern void registerWithRZB ();
extern void foxLog (REPORTMODE type, const char *fmt, ...);
extern void foxReport(const char *msg, const char *cve, uint32_t sid, uint32_t sfflags, uint32_t entflags, uint32_t priority);
extern void initPDFFoxJudgment (struct Judgment *incoming);
#endif
