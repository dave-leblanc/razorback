#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef RZB_PDF_FOX_NUGGET
#include <razorback/log.h>
#include <razorback/types.h>
#include <razorback/metadata.h>
#include <razorback/api.h>
#endif

#include "foxreport.h"

const char *reg = "<?xml version=\"1.0\"?>"
                  "<razorback>\n<registration>"
                  "<nugget_id>1ceb82dd-543b-4117-a9d8-45768e38f310</nugget_id>"
                  "<application_type>d721e5f0-a5b7-4cea-9eae-4111117d72c4</application_type>"
                  "<data_types>"
                  "<data_type>PDF_FILE</data_type>"
                  "</data_types>"
                  "</registration>\n</razorback>\n";


const char *RZBAlert = "<?xml version=\"1.0\"?>"
             "<razorback>"
             "<response>"
             "<verdict priority=\"2\" gid=\"6969\" sid=\"1\">"
             "<flags>"
             "<sourcefire>"
             "<set>128</set>"
             "<unset>0</unset>"
             "</sourcefire>"
             "<enterprise>"
             "<set>0</set>"
             "<unset>0</unset>"
             "</enterprise>"
             "</flags>"
             "<message>%s</message>"
             "<metadata>"
             "<entry>"
             "<type>REPORT</type>"
             "<data>PDF Fox: Message contains more information on the error.</data>"
             "</entry>"
             "</metadata>"
             "</verdict>"
             "</response>"
             "</razorback>\n";


void registerWithRZB () {
	printf("%s", reg);
	exit(1);
}
			
//#define PDF_FOX_COMMAND_LINE

/*
 *
 * Reports Errors
 *
 */
void foxLog (REPORTMODE type, const char *fmt, ...)
{
    char *msg = NULL;
    
    va_list argp;
    va_start (argp, fmt);

    if (vasprintf (&msg, fmt, argp) == -1)
        return;

    switch(type) {
        case PRINT:
#ifdef PDF_FOX_COMMAND_LINE
            printf("%s", msg);
#endif
#ifdef RZB_PDF_FOX_NUGGET
#endif
            break;
        case FATAL:
#ifdef PDF_FOX_COMMAND_LINE
            printf("[FATAL] %s", msg);
            exit(-1);
#endif
#ifdef RZB_PDF_FOX_NUGGET
			rzb_log(LOG_ERR, "Shutting Down Context. %s", msg);
#endif
            break;

        case NONFATAL:
#ifdef PDF_FOX_SHOW_NONFATAL
#ifdef PDF_FOX_COMMAND_LINE
            printf("[NONFATAL] %s", msg);
#endif
#ifdef RZB_PDF_FOX_NUGGET
			rzb_log(LOG_ERR, "%s", msg);
#endif
#endif
            break;
       
		case PDF_DEBUG:
#ifdef PDF_FOX_SHOW_DEBUG
#ifdef PDF_FOX_COMMAND_LINE
            printf("[PDF_DEBUG] %s", msg);
#endif
#ifdef RZB_PDF_FOX_NUGGET
			rzb_log(LOG_DEBUG, "%s", msg);
#endif
#endif
            break;
    }
 
    va_end (argp);
    if (msg != NULL)
        free (msg);
}

#ifdef PDF_FOX_COMMAND_LINE
void foxReport(const char *msg, const char *cve, uint32_t sid, uint32_t sfflags, uint32_t entflags, uint32_t priority) {
	printf("ALERT: Malicious PDF found.\nMessage: %s %s\nsid: %d\n", cve, msg, sid);
}
#endif
#ifdef RZB_PDF_FOX_NUGGET
#include <razorback/judgment.h>
#include <razorback/types.h>

static struct Judgment *judgment;

void initPDFFoxJudgment (struct Judgment *incoming){

	judgment = incoming;
	judgment->iGID = 9;

}

/**For razorback reporting,
 * We'll need several pieces of data...
 *
 * Message, sid, flags, priority...
 *
 * All of this depends on the vuln in question...
 */

void foxReport(const char *msg, const char *cve, uint32_t sid, uint32_t sfflags, uint32_t entflags, uint32_t priority) {
    judgment->sMessage = (uint8_t *)msg;
	judgment->iSID = sid;
    judgment->Set_SfFlags = sfflags;
	judgment->Set_EntFlags = entflags;
	judgment->iPriority = priority;
    Metadata_Add_CVE(judgment->pMetaDataList, cve);
	Razorback_Render_Verdict(judgment);
}
#endif
