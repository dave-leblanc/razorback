#include <rzb_conf.h>

const char *my_string;

static RZBConfKey_t my_config[] = {
    { "MY_STRING", RZB_CONF_KEY_TYPE_STRING, &my_string },
    { NULL, RZB_CONF_KEY_TYPE_END, NULL }
};

if (readApiConfig(NULL) != R_SUCCESS)
    printf("Failed to load api config file\n");

if (readMyConfig(NULL, "rzb_custom.conf", my_config) != R_SUCCESS)
    printf("Failed to load custom config file\n");
else
    printf("MY_STRING: %s\n", my_string);

