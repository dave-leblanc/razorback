#ifndef RAZORBACK_CONSOLE_H
#define RAZORBACK_CONSOLE_H
#include <razorback/socket.h>

struct ConsoleSession
{
    bool authenticated;
    char *username;
    struct Socket *socket;
    void (*quitCallback)(struct ConsoleSession *);
    void *userdata;
};

// Console.c
bool Console_Initialize(struct RazorbackContext *p_pContext);

// Console_cli.c
bool Console_CLI_Initialize();
void Console_Thread (struct Thread *p_pThread);

// Console_ssh.c
void Console_SSH_Server (struct Thread *p_pThread);
 
#endif
