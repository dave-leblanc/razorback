#include "config.h"
#include <razorback/debug.h>
#include <razorback/metadata.h>
#include <razorback/ntlv.h>
#include <razorback/uuids.h>
#include <razorback/log.h>

#include <string.h>
#ifdef _MSC_VER
#include "bobins.h"
#endif


SO_PUBLIC bool 
Metadata_Add_String (struct List *list, uuid_t name, const char *string)
{
    uuid_t uuidType;
    if (!UUID_Get_UUID(NTLV_TYPE_STRING, UUID_TYPE_NTLV_TYPE, uuidType))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup type uuid", __func__);
        return false;
    }
    return Metadata_Add(list, name, uuidType, strlen(string)+1, (uint8_t *)string);
}

SO_PUBLIC bool 
Metadata_Get_String (struct List *list, uuid_t name, uint32_t *len, const char **string)
{
    uuid_t uuidType;
    if (!UUID_Get_UUID(NTLV_TYPE_STRING, UUID_TYPE_NTLV_TYPE, uuidType))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup type uuid", __func__);
        return false;
    }
    return Metadata_Get(list, name, uuidType, len, (const uint8_t **)string);
}

SO_PUBLIC bool 
Metadata_Add_IPv4 (struct List *list, uuid_t name, const uint8_t *addr)
{
    uuid_t uuidType;
    if (!UUID_Get_UUID(NTLV_TYPE_IPv4_ADDR, UUID_TYPE_NTLV_TYPE, uuidType))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup type uuid", __func__);
        return false;
    }
    return Metadata_Add(list, name, uuidType, 4, addr);
}

SO_PUBLIC bool 
Metadata_Get_IPv4 (struct List *list, uuid_t name, const uint8_t **addr)
{
    uuid_t uuidType;
    uint32_t size;
    if (!UUID_Get_UUID(NTLV_TYPE_IPv4_ADDR, UUID_TYPE_NTLV_TYPE, uuidType))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup type uuid", __func__);
        return false;
    }
    return Metadata_Get(list, name, uuidType, &size, addr);
}

SO_PUBLIC bool 
Metadata_Add_IPv6 (struct List *list, uuid_t name, const uint8_t *addr)
{
    uuid_t uuidType;
    if (!UUID_Get_UUID(NTLV_TYPE_IPv6_ADDR, UUID_TYPE_NTLV_TYPE, uuidType))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup type uuid", __func__);
        return false;
    }
    return Metadata_Add(list, name, uuidType, (128/8), addr);
}

SO_PUBLIC bool 
Metadata_Get_IPv6 (struct List *list, uuid_t name, const uint8_t **addr)
{
    uuid_t uuidType;
    uint32_t size;
    if (!UUID_Get_UUID(NTLV_TYPE_IPv6_ADDR, UUID_TYPE_NTLV_TYPE, uuidType))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup type uuid", __func__);
        return false;
    }
    return Metadata_Get(list, name, uuidType, &size, addr);
}

SO_PUBLIC bool 
Metadata_Add_Port (struct List *list, uuid_t name, const uint16_t port)
{
    uuid_t uuidType;
    if (!UUID_Get_UUID(NTLV_TYPE_PORT, UUID_TYPE_NTLV_TYPE, uuidType))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup type uuid", __func__);
        return false;
    }
    return Metadata_Add(list, name, uuidType, 2, (uint8_t*)&port);
}

SO_PUBLIC bool 
Metadata_Get_Port (struct List *list, uuid_t name, uint16_t *port)
{
    uuid_t uuidType;
    uint32_t size;
    uint16_t *lPort;
    if (!UUID_Get_UUID(NTLV_TYPE_PORT, UUID_TYPE_NTLV_TYPE, uuidType))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup type uuid", __func__);
        return false;
    }
    if (!Metadata_Get(list, name, uuidType, &size, (const uint8_t**)&lPort))
        return false;

    *port = *lPort;
    free(lPort);
    return true;
}

SO_PUBLIC bool 
Metadata_Add_Filename (struct List *list, const char *name)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_FILENAME, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_String(list, uuidName, name);
}

SO_PUBLIC bool 
Metadata_Add_Hostname (struct List *list, const char *name)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_HOSTNAME, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_String(list, uuidName, name);
}

SO_PUBLIC bool 
Metadata_Add_URI (struct List *list, const char *name)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_URI, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_String(list, uuidName, name);
}

SO_PUBLIC bool 
Metadata_Add_HttpRequest (struct List *list, const char *name)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_HTTP_REQUEST, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_String(list, uuidName, name);
}

SO_PUBLIC bool 
Metadata_Add_HttpResponse (struct List *list, const char *name)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_HTTP_RESPONSE, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_String(list, uuidName, name);
}

SO_PUBLIC bool 
Metadata_Add_MalwareName (struct List *list, const char *vendor, const char *name)
{
    uuid_t uuidName;
    char *tmp;
    bool ret = true;

    if (!UUID_Get_UUID(NTLV_NAME_MALWARENAME, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    if (asprintf(&tmp, "%s:%s", vendor, name) == -1)
    {
        return false;
    }
    ret= Metadata_Add_String(list, uuidName,tmp);
    free(tmp);
    return ret;
}

SO_PUBLIC bool 
Metadata_Add_Report (struct List *list, const char *text)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_REPORT, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_String(list, uuidName, text);
}

SO_PUBLIC bool 
Metadata_Add_CVE (struct List *list, const char *text)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_CVE, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_String(list, uuidName, text);
}

SO_PUBLIC bool 
Metadata_Add_BID (struct List *list, const char *text)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_BID, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_String(list, uuidName, text);
}
SO_PUBLIC bool 
Metadata_Add_OSVDB (struct List *list, const char *text)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_OSVDB, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_String(list, uuidName, text);
}

SO_PUBLIC bool 
Metadata_Add_IPv4_Source (struct List *list, const uint8_t *addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_SOURCE, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_IPv4(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Get_IPv4_Source (struct List *list, const uint8_t **addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_SOURCE, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Get_IPv4(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Add_IPv4_Destination (struct List *list, const uint8_t *addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_DEST, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_IPv4(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Get_IPv4_Destination (struct List *list, const uint8_t **addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_DEST, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Get_IPv4(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Add_IPv6_Source (struct List *list, const uint8_t *addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_SOURCE, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_IPv6(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Get_IPv6_Source (struct List *list, const uint8_t **addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_SOURCE, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Get_IPv6(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Add_IPv6_Destination (struct List *list, const uint8_t *addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_DEST, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_IPv6(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Get_IPv6_Destination (struct List *list, const uint8_t **addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_DEST, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Get_IPv6(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Add_Port_Source (struct List *list, const uint16_t addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_SOURCE, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_Port(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Get_Port_Source (struct List *list, uint16_t *addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_SOURCE, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Get_Port(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Add_Port_Destination (struct List *list, const uint16_t addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_DEST, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Add_Port(list, uuidName, addr);
}

SO_PUBLIC bool 
Metadata_Get_Port_Destination (struct List *list, uint16_t *addr)
{
    uuid_t uuidName;
    if (!UUID_Get_UUID(NTLV_NAME_DEST, UUID_TYPE_NTLV_NAME, uuidName))
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
        return false;
    }
    return Metadata_Get_Port(list, uuidName, addr);
}

