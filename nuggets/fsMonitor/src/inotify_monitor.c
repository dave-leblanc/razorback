#include <inotifytools/inotifytools.h>
#include <inotifytools/inotify.h>

#include "fsmonitor.h"

HRESULT fsMonitor_init() {
   // Make sure inotify tools initializes properly
   if(!inotifytools_initialize()) {
      fprintf(stderr, "%s\n", strerror(inotifytools_error()));
      return R_FAIL;
   }
   return R_SUCCESS;
}

HRESULT fsMonitor_add_path(char *path) {
    if(!inotifytools_watch_recursively(path, IN_CLOSE_WRITE)) {
        fprintf(stderr, "%s\n", strerror(inotifytools_error()));
        return R_FAIL;
    }
    return R_SUCCESS;
}

HRESULT fsMonitor_cleanup() {
    inotifytools_cleanup();
    return R_SUCCESS;
}

HRESULT fsMonitor_main() {
    struct inotify_event *event;
    char filename[MAX_FILE_NAME];
    // Read all of the events and send any files being written to
    while(!halt_processing && (event = inotifytools_next_event(-1))) {
        // Make sure we can get the filename
        if(inotifytools_snprintf(filename, sizeof(filename) - 1, event, (char *)"%w%f") <= 0) {
            fprintf(stderr, "Unable to get event filename\n");
            continue;
        }
        
        fsMonitor_handle_file(filename);
    }
    return R_SUCCESS;
}
