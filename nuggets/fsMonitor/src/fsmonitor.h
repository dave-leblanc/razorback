#include <signal.h>
#include <razorback/types.h>
#include <razorback/log.h>

#define MAX_FILE_NAME 1024
static volatile sig_atomic_t halt_processing = 0;

bool fsMonitor_init();
bool fsMonitor_add_path(char *path);
bool fsMonitor_main();
bool fsMonitor_handle_file(char *filename);
bool fsMonitor_cleanup();

