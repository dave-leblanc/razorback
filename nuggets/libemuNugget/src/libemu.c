#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>
#include <time.h>
#include <arpa/inet.h>
#include <emu/emu.h>
#include <emu/emu_cpu.h>
#include <emu/emu_cpu_data.h>
#include <emu/emu_shellcode.h>
#include <emu/emu_memory.h>
#include <emu/emu_cpu_instruction.h>
#include <emu/emu_instruction.h>


#include <razorback/api.h>
#include <razorback/config_file.h>
#include <razorback/ntlv.h>
#include <razorback/uuids.h>
#include <razorback/log.h>
#include <razorback/visibility.h>
#include <razorback/judgment.h>
#include <razorback/metadata.h>

#define LIBEMU_NUGGET_CONFIG "libemu.conf"
UUID_DEFINE(LIBEMU, 0x40, 0x56, 0x60, 0x82, 0xaf, 0xaf, 0x11, 0xdf, 0x9e, 0xd5, 0xe3, 0xab, 0x45, 0xe0, 0x14, 0xa4);


bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(shellcode_handler);

static uuid_t sg_uuidNuggetId;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;

static struct RazorbackContext *sg_pContext;

static struct RazorbackInspectionHooks sg_InspectionHooks = {
    shellcode_handler,
    processDeferredList,
    NULL,
    NULL
};


RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"Threads.Initial", RZB_CONF_KEY_TYPE_INT, &sg_initThreads, NULL },
    {"Threads.Max", RZB_CONF_KEY_TYPE_INT, &sg_maxThreads, NULL },
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

DECL_INSPECTION_FUNC(shellcode_handler)
{
    struct emu *emu = emu_new();
    //struct emu_cpu *cpu = NULL;
    //char instruction[MAX_INSTRUCTION_SIZE];
    int shellcode_start = 0;
    struct Judgment *judgment = NULL;

    // Run the simple shellcode check
    if((shellcode_start = emu_shellcode_test(emu, block->data.pointer, block->pId->iLength)) != -1) {
        printf("Shellcode detected: %u\n", shellcode_start);
        if ((judgment = Judgment_Create(eventId, block->pId)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate judgment", __func__);
            return JUDGMENT_REASON_ERROR;
        }
        // Set the message
        if (asprintf((char **)&judgment->sMessage, "Shellcode detected starting at offset 0x%X", shellcode_start) == -1)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate judgment message", __func__);
            Judgment_Destroy(judgment);
            return JUDGMENT_REASON_ERROR;
        }

        judgment->iPriority = 1;
#if 0
      // Loop through each instruction adding it to long data
      do {
         cpu = emu_cpu_get(emu);

         if(emu_cpu_parse(cpu) == 0) {
            snprintf(instruction, MAX_INSTRUCTION_SIZE, "%s\n", cpu->instr_string);

            if((strlen(instruction) + alert.ld_size) < MAX_LONG_SIZE) {
               strncat(long_data, instruction, (MAX_LONG_SIZE - alert.ld_size));
               alert.ld_size += strlen(instruction);
            }
         }
      }
      while(!emu_cpu_step(emu_cpu_get(emu)));
#endif

      // Finally, send the alert
      Razorback_Render_Verdict(judgment);
   }
   emu_free(emu);
   return JUDGMENT_REASON_DONE;
}

SO_PUBLIC DECL_NUGGET_INIT
{
    uuid_t l_pEmuUuid;
    uuid_t l_pList[1];
    UUID_Get_UUID(DATA_TYPE_SHELL_CODE, UUID_TYPE_DATA_TYPE, l_pList[0]);
    uuid_copy(l_pEmuUuid, LIBEMU);

    readMyConfig(NULL, LIBEMU_NUGGET_CONFIG, sg_Config);
    sg_pContext = Razorback_Init_Inspection_Context (
            sg_uuidNuggetId, l_pEmuUuid, 1, l_pList,
            &sg_InspectionHooks, sg_initThreads, sg_maxThreads);

    return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    Razorback_Shutdown_Context(sg_pContext);
}
bool processDeferredList (struct DeferredList *p_pDefferedList)
{
    return true;
}

