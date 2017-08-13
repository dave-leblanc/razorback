
#include <razorback/socket.h>
#include <razorback/thread.h>
#include <razorback/log.h>
#include <string.h>

#include "console.h"
#include "dispatcher.h"
#include "configuration.h"



void Console_Telnet_Server (struct Thread *p_pThread);
void Console_SSH_Server (struct Thread *p_pThread);

bool
Console_Initialize (struct RazorbackContext *p_pContext)
{
    struct Thread *thread;

    if (!Console_CLI_Initialize())
        return false;


    if (g_telnetConsole.enable == 1)
    {
        thread =
            Thread_Launch (Console_Telnet_Server, NULL,
                           (char *) "Console Telnet Server", p_pContext);
        if (thread == NULL)
        {
            rzb_log (LOG_ERR, "%s: Failed to launch telnet server thread", __func__);
        }
        Thread_Destroy(thread);
        thread = NULL;
    }
    if (g_sshConsole.enable == 1)
    {
        thread =
            Thread_Launch (Console_SSH_Server, NULL,
                           (char *) "Console SSH Server", p_pContext);
        if (thread == NULL)
        {
            rzb_log (LOG_ERR, "%s: Failed to launch ssh server thread", __func__);
        }
        Thread_Destroy(thread);
        thread = NULL;
    }
    return true;

}

void
Console_Telnet_Quit(struct ConsoleSession *session)
{
    free(session);
}

void
Console_Telnet_Server (struct Thread *p_pThread)
{
    struct Socket *l_pSocket;
    struct Socket *l_pAcceptSocket;
    struct ConsoleSession *session;
    struct Thread *thread;
    if ((l_pAcceptSocket =
         Socket_Listen ((const uint8_t *) g_telnetConsole.bindAddress,
                        g_telnetConsole.bindPort)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: Failed to start listening socket", __func__);
        return;
    }

    while (true)
    {

        if (Socket_Accept (&l_pSocket, l_pAcceptSocket) == 1)
        {
            if ((session = calloc(1, sizeof(struct ConsoleSession))) == NULL)
            {
                rzb_log(LOG_ERR, "%s: Failed to allocate console session", __func__);
                Socket_Close(l_pSocket);
                continue;
            }
            session->authenticated = false;
            session->socket = l_pSocket;
            session->quitCallback = Console_Telnet_Quit;
            rzb_log (LOG_DEBUG, "%s: Launching console thread", __func__);
            thread = Thread_Launch (Console_Thread, session,
                           (char *) "Console Session", p_pThread->pContext);
            if (thread == NULL)
                rzb_log(LOG_ERR, "%s: Failed to launch console thread", __func__);
            else
                Thread_Destroy(thread);
            thread = NULL;
        }

    }
}


