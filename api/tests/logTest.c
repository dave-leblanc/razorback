#include <stdio.h>
#include <razorback/config_file.h>
#include <razorback/log.h>


int main() {
    rzb_log(LOG_EMERG, "Emergency test");
    rzb_log(LOG_ALERT, "Alert test");
    rzb_log(LOG_CRIT, "Critical test");
    rzb_log(LOG_ERR, "Error test");
    rzb_log(LOG_WARNING, "Warning test");
    rzb_log(LOG_NOTICE, "Notice test");
    rzb_log(LOG_INFO, "Info test");
    rzb_log(LOG_DEBUG, "Debug test");

}


