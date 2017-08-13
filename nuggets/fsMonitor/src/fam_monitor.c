#include <fam.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "fsmonitor.h"
FAMConnection fc;
fd_set readfds;
int fam_fd;

// The monitored path store
//
struct pathNode {
    char *path;
    FAMRequest *fr;
    struct pathNode *next;
};
struct pathNode;
typedef struct pathNode pathNode;

struct changeEvent {
    char * path;
    timer_t timerid;
    struct changeEvent *next;
    struct changeEvent *prev;
};
struct changeEvent;
typedef struct changeEvent changeEvent;

pthread_mutex_t eventListMutex = PTHREAD_MUTEX_INITIALIZER;

pathNode *pathList = NULL;

changeEvent *eventList = NULL;


const char *
event_name(FAMEvent event)
{
    static const char *famevent[] = {
        "",
        "FAMChanged",
        "FAMDeleted",
        "FAMStartExecuting",
        "FAMStopExecuting",
        "FAMCreated",
        "FAMMoved",
        "FAMAcknowledge",
        "FAMExists",
        "FAMEndExist"
    };
    static char unknown_event[10];
 
    if (event.code < FAMChanged || event.code > FAMEndExist)
    {
        sprintf(unknown_event, "unknown (%d)", event.code);
        return unknown_event;
    }
    return famevent[event.code];
}

void 
timer_handler(union sigval val) 
{
    changeEvent *cEvent;
    cEvent = (changeEvent *)val.sival_ptr;
    pthread_mutex_lock(&eventListMutex);
    timer_delete(cEvent->timerid);

    if (cEvent == eventList) 
    {
        eventList = cEvent->next;
        if (eventList != NULL )
            eventList->prev = NULL;
    } 
    else 
    {
        if (cEvent->next != NULL)
            cEvent->next->prev = cEvent->prev;
        
        cEvent->prev->next = cEvent->next;
    }
    pthread_mutex_unlock(&eventListMutex);
    fsMonitor_handle_file(cEvent->path); 
    free(cEvent->path);
    free(cEvent);
}

bool
handle_event(FAMEvent event) 
{
    struct stat status;
    char *realPath;
    int monitoredPath = 0;
    changeEvent *cEvent = NULL;
    struct sigevent *tProps;
    struct itimerspec its;

    its.it_value.tv_sec = 10;
    its.it_value.tv_nsec = 1;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    switch (event.code) 
    {
    case FAMEndExist:
    case FAMAcknowledge:
        return true;
    default:
        break;
    }

    if (strcmp((char *)event.userdata, event.filename) == 0 )  
    {
        monitoredPath = 1;
        if (asprintf(&realPath, "%s", event.filename) <= 0) 
        {
            rzb_log(LOG_ERR, "handle_event: Failed to alloc realPath");
            return false;
        }
    }
    else 
    {
        if (asprintf(&realPath, "%s/%s", (char *)event.userdata, event.filename) <= 0) 
        {
            rzb_log(LOG_ERR, "handle_event: Failed to alloc realPath");
            return false;
        }
    }

    if (event.code == FAMDeleted) 
    {
        pathNode *prev = pathList;
        pathNode *n = pathList;
        while (n != NULL) 
        {
            if (strcmp(n->path, realPath) == 0) 
            {
                rzb_log(LOG_DEBUG, "Removing watch on: %s", realPath);
                FAMCancelMonitor(&fc, n->fr);
                if (pathList == n)
                {
                    pathList = n->next;
                } 
                else 
                {
                    prev->next = n->next;
                }
                break;
            }
            n = n->next;
        }
        free(n);
        free(realPath);
        return true;

    }
    if (stat(realPath, &status) < 0) 
    {
        rzb_log(LOG_ERR, "handle_event: Failed to stat: %s", event.filename);
        free(realPath);
        return false;
    }
    switch (event.code) 
    {
        case FAMChanged:
        case FAMCreated:
        case FAMExists:
            if (( monitoredPath ==0)  && (status.st_mode & S_IFMT) == S_IFDIR) 
            {
                rzb_log(LOG_DEBUG, "Adding sub watch for: %s", realPath);
                return fsMonitor_add_path(realPath);
            }
            if ((status.st_mode & S_IFMT) == S_IFDIR) 
                break;
            if (event.code == FAMExists)
                break;

            pthread_mutex_lock(&eventListMutex);
            // Walk the pending list for pending changes:
            cEvent = eventList;
            while (cEvent != NULL) 
            {
                if (strcmp(cEvent->path, realPath) == 0) 
                {
                    if (timer_settime(cEvent->timerid, 0, &its, NULL) == -1) 
                    {
                        rzb_perror("handle_event: Error setting time: %s");
                    }
                    pthread_mutex_unlock(&eventListMutex);
                    free(realPath);
                    return true;
                }
                cEvent = cEvent->next;
            }
            cEvent = NULL;
            // No event create a new one.
            cEvent = malloc(sizeof(changeEvent));
            if (cEvent == NULL) 
            {
                rzb_log(LOG_ERR, "handle_event: Failed to malloc changeEvent");
                pthread_mutex_unlock(&eventListMutex);
                free(realPath);
                return false;
            }
            tProps = calloc(1, sizeof(struct sigevent));
            if (tProps == NULL) 
            {
                rzb_log(LOG_ERR, "handle_event: Failed to malloc tProps");
                free(cEvent);
                free(realPath);
                pthread_mutex_unlock(&eventListMutex);
                return false;
            }

            if (asprintf(&cEvent->path, "%s", realPath) == -1)
            {
                rzb_log(LOG_ERR, "handle_event: Failed to copy path to event");
            }

            // setup the timer
            tProps->sigev_notify=SIGEV_THREAD;
            tProps->sigev_value.sival_ptr = cEvent;
            tProps->sigev_notify_function = &timer_handler;
            if (timer_create(CLOCK_REALTIME, tProps, &cEvent->timerid) == -1 ) 
            {
                rzb_perror("handle_event: Error creating timer: %s");
            }
            cEvent->prev = NULL;
            if (eventList == NULL) 
            {
                cEvent->next = NULL;
            } 
            else 
            {
                cEvent->next = eventList;
                eventList->prev = cEvent;
            }
            eventList = cEvent;
            if (timer_settime(cEvent->timerid, 0, &its, NULL) == -1)
            {
                rzb_perror("handle_event: Error setting time: %s");
            }
            pthread_mutex_unlock(&eventListMutex);
            break;
        case FAMDeleted:
            break;
        default:
            break;
    }
    free(realPath);
    return true;
}

bool
fsMonitor_init() 
{
    memset(&fc, 0, sizeof(FAMConnection));
    if ((FAMOpen(&fc)) < 0)
    {
        rzb_log(LOG_ERR, "Failed to connect to fam daemon");
        return false;
    }
    fam_fd = FAMCONNECTION_GETFD(&fc);
    FD_ZERO(&readfds);
    FD_SET(fam_fd, &readfds);
 
    return true;
}

bool
fsMonitor_add_path(char *path) 
{
    FAMRequest *frp;
    int rc;
    struct stat status;
    pathNode *node;

    frp = malloc(sizeof(FAMRequest));
    if (frp == NULL) 
    {
        rzb_perror("fsMonitor_add_path: malloc request: %s");
        return false;
    }
    node = calloc(1, sizeof(pathNode));
    if (node == NULL) 
    {
        rzb_perror("fsMonitor_add_path: malloc pathNode: %s");
        return false;
    }
   
    if (stat(path, &status) < 0) 
    {
        rzb_log(LOG_ERR, "Failed to stat: %s", path);
        free(node);
        free(frp);
        return false;
    }

    if ((status.st_mode & S_IFMT) == S_IFDIR)
        rc = FAMMonitorDirectory(&fc, path, frp, path);
    else 
    {
        rzb_log(LOG_ERR, "Can't monitor something thats not a directory: %s", path);
        free(node);
        free(frp);
        return false;
    }

    if (rc < 0) 
    {
        rzb_log(LOG_ERR, "FAM Error");
        free(frp);
        free(node);
        return false;
    }

    if (asprintf(&node->path,"%s", path) == -1 ) 
    {
        rzb_log(LOG_ERR, "Failed to copy path into event node");
    }
    node->fr = frp;
    if (pathList == NULL) 
    {
        node->next = NULL;
        pathList = node;
    } 
    else 
    {
        node->next = pathList;
        pathList = node;
    }

    return true;
}

bool
fsMonitor_cleanup() 
{
    return true;
}

bool
fsMonitor_main() {
    FAMEvent fe;
    while(!halt_processing) 
    {
        if (select(fam_fd + 1, &readfds, NULL, NULL, NULL) < 0) 
        {
             rzb_perror("fsMonitor_main: select failed: %s");
             return false;
        }
        if (FD_ISSET(fam_fd, &readfds))
        {
            if (FAMNextEvent(&fc, &fe) < 0)
            {
                rzb_perror("fsMonitor_main: FAMNextEvent: %s");
                return false;
            }
            handle_event(fe);

        }
    }
 
    return true;
}
