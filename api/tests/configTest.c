#include <stdio.h>
#include <razorback/uuids.h>
#include <razorback/config_file.h>
#include <string.h>

bool parseOption (const char *, conf_int_t *);
static RZBConfCallBack parseCallback = {
    &parseOption
};
char **strings;
conf_int_t stringsCount = 0;
conf_int_t *ints;
conf_int_t intsCount = 0;
conf_int_t *parsedStrings;
conf_int_t parsedStringsCount = 0;
uuid_t *uuids;
conf_int_t uuidsCount = 0;
bool *bools;
conf_int_t boolsCount = 0;

static struct ConfArray stringArray = {
    RZB_CONF_KEY_TYPE_STRING,
    (void **)&strings,
    &stringsCount,
    NULL
};

static struct ConfArray intArray = {
    RZB_CONF_KEY_TYPE_INT,
    (void **)&ints,
    &intsCount,
    NULL
};

static struct ConfArray parsedStringArray = {
    RZB_CONF_KEY_TYPE_PARSED_STRING,
    (void **)&parsedStrings,
    &parsedStringsCount,
    parseOption
};
static struct ConfArray uuidArray = {
    RZB_CONF_KEY_TYPE_UUID,
    (void **)&uuids,
    &uuidsCount,
    NULL
};
static struct ConfArray boolArray = {
    RZB_CONF_KEY_TYPE_BOOL,
    (void **)&bools,
    &boolsCount,
    NULL
};
struct item {
    conf_int_t i;
    bool b;
    char * string;
    conf_int_t parsed;
    uuid_t uuid;
}  __attribute__((__packed__));

struct item *items = NULL;
conf_int_t itemCount = 0;
static RZBConfKey_t listItems[] =
{
    {"Int", RZB_CONF_KEY_TYPE_INT, NULL, NULL},
    {"Bool", RZB_CONF_KEY_TYPE_BOOL, NULL, NULL},
    {"String", RZB_CONF_KEY_TYPE_STRING, NULL, NULL},
    {"ParsedString", RZB_CONF_KEY_TYPE_PARSED_STRING, NULL, &parseCallback},
    {"UUID", RZB_CONF_KEY_TYPE_UUID, NULL, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};
static struct ConfList list = {
    (void **)&items,
    &itemCount,
    listItems
};

bool
parseOption (const char *string, conf_int_t * val)
{
    if (!strncasecmp (string, "Option1", 7))
    {
        *val = 1;
        return true;
    }
    else if (!strncasecmp (string, "Option2", 7))
    {
        *val = 2;
        return true;
    }
    return false;

}

static conf_int_t testInt = 0;
static bool testBool = false;
static bool testBool2= false;
static char *testString = NULL;
static conf_int_t testParsedString1 = 0;
static conf_int_t testParsedString2 = 0;
static uuid_t testUuid;

static RZBConfKey_t global_config[] = {
    // Global Items
    {"Int", RZB_CONF_KEY_TYPE_INT, &testInt, NULL},
    {"Bool", RZB_CONF_KEY_TYPE_BOOL, &testBool, NULL},
    {"Bool2", RZB_CONF_KEY_TYPE_BOOL, &testBool2, NULL},
    {"String", RZB_CONF_KEY_TYPE_STRING, &testString, NULL},
    {"UUID", RZB_CONF_KEY_TYPE_UUID, &testUuid, NULL},
    {"ParsedString", RZB_CONF_KEY_TYPE_PARSED_STRING,
        &testParsedString1, &parseCallback},
    {"ParsedString2", RZB_CONF_KEY_TYPE_PARSED_STRING,
        &testParsedString2, &parseCallback},

    {"StringArray", RZB_CONF_KEY_TYPE_ARRAY, NULL, &stringArray},
    {"IntArray", RZB_CONF_KEY_TYPE_ARRAY, NULL, &intArray},
    {"ParsedArray", RZB_CONF_KEY_TYPE_ARRAY, NULL, &parsedStringArray},
    {"UuidArray", RZB_CONF_KEY_TYPE_ARRAY, NULL, &uuidArray},
    {"BoolArray", RZB_CONF_KEY_TYPE_ARRAY, NULL, &boolArray},
    {"List", RZB_CONF_KEY_TYPE_LIST, NULL, &list},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};


int main() {
    char uuid[UUID_STRING_LENGTH];
    readMyConfig ("./", "test.conf", global_config);
    printf("Int: %d\n", (int)testInt);
    printf("Bool: %d\n", (int)testBool);
    printf("Bool2: %d\n", (int)testBool2);
    printf("String: %s\n", testString);
    uuid_unparse(testUuid, uuid);
    printf("UUID: %s\n", uuid);
    printf("Parsed String1: %d\n", (int)testParsedString1);
    printf("Parsed String2: %d\n", (int)testParsedString2);
    for (conf_int_t i = 0; i < stringsCount; i++)
        printf("StringArray[%d]: %s\n", (int)i, strings[i]);
    for (conf_int_t i = 0; i < intsCount; i++)
        printf("IntArray[%d]: %d\n", (int)i, (int)ints[i]);
    for (conf_int_t i = 0; i < parsedStringsCount; i++)
        printf("ParsedArray[%d]: %d\n", (int)i, (int)parsedStrings[i]);
    for (conf_int_t i = 0; i < uuidsCount; i++)
    {
        uuid_unparse(uuids[i], uuid);
        printf("UuidArray[%d]: %s\n", (int)i, uuid);
    }
    for (conf_int_t i = 0; i < boolsCount; i++)
        printf("BoolArray[%d]: %d\n", (int)i, (int)bools[i]);

    for (conf_int_t i = 0; i < itemCount; i++)
    {
        uuid_unparse(items[i].uuid, uuid);
        printf("List[%d]: Int: %d\n", (int)i, (int)items[i].i);
        printf("List[%d]: Bool: %d\n", (int)i, (int)items[i].b);
        printf("List[%d]: String: %s\n", (int)i, items[i].string);
        printf("List[%d]: ParsedString: %d\n", (int)i, (int)items[i].parsed);
        printf("List[%d]: UUID: %s\n", (int)i, uuid);
    }

}


