/** @file metadata.h
 * Metadata wrapper functions.
 */
#ifndef RAZORBACK_METADATA_H
#define RAZORBACK_METADATA_H

#include <razorback/visibility.h>
#include <razorback/types.h>
#include <razorback/ntlv.h>

#ifdef __cplusplus
extern "C" {
#endif

#define Metadata_Add NTLVList_Add	///< Add metadata to a metadata list.
#define Metadata_Get NTLVList_Get   ///< Get metadata from a metadata list.

/** Add a string to a metadata list.
 * @param list The metadata list.
 * @param name The NTLV Name of the metadata object.
 * @param string The string to add to the list.
 * @return True on success, false on error.
 */

SO_PUBLIC extern bool Metadata_Add_String (struct List *list, uuid_t name, const char *string);

/** Get a string from a metadata list.
 * @note This will return the first item in the list with that name.  If there is more than
 * with the same name type tuple then the first one will be returned.
 * @param list The metadata list.
 * @param name The NTLV Name of the metadata object.
 * @param len The length of the returned strng
 * @param string The a pointer to a char* to store the string in
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_String (struct List *list, uuid_t name, uint32_t *len, const char **string);

/** Add an IPv4 address to a metadata list.
 * @param list The metadata list.
 * @param name The NTLV Name of the metadata object.
 * @param addr The IP address.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_IPv4 (struct List *list, uuid_t name, const uint8_t *addr);

/** Get an IPv4 address from the metadata list.
 * @param list The metadata list.
 * @param name The NTLV Name of the metadata object.
 * @param addr A pointer to a uint8_t* to store the address in.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_IPv4 (struct List *list, uuid_t name, const uint8_t **addr);

/** Add an IPv6 address to a metadata list
 * @param list The metadata list.
 * @param name The NTLV Name of the metadata object.
 * @param addr The address to add to the list,
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_IPv6 (struct List *list, uuid_t name, const uint8_t *addr);

/** Get an IPv6 address from metadata list.
 * @param list The metadata list.
 * @param name The NTLV Name of the metadata object.
 * @param addr A pointer to a uint8_* to store the address in.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_IPv6 (struct List *list, uuid_t name, const uint8_t **addr);

/** Add a port to a metadata list.
 * @param list The metadata list.
 * @param name The NTLV Name of the metadata object.
 * @param port The port to add to the list.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_Port (struct List *list, uuid_t name, const uint16_t port);

/** Get a port from a metadata list.
 * @param list The metadata list.
 * @param name The NTLV Name of the metadata object.
 * @param port A pointer to the uint16_t to store the port in.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_Port (struct List *list, uuid_t name, uint16_t *port);

/** Add a filename to a metadata list.
 * @param list The metadata list.
 * @param name The file name.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_Filename (struct List *list, const char *name);

/** Add a hostname to a metadata list.
 * @param list The metadata list.
 * @param name The hostname.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_Hostname (struct List *list, const char *name);

/** Add a URI to a metadata list.
 * @param list The metadata list.
 * @param uri The URI to add.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_URI (struct List *list, const char *uri);

/** Add a path to a metadata list.
 * @param list The metadata list.
 * @param path The path to add.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_Path (struct List *list, const char *path);

/** Add a malware name to a metadata list.
 * @param list The metadata list.
 * @param vendor The detection vendor name.
 * @param name The vendors name for the malware.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_MalwareName (struct List *list, const char *vendor, const char *name);

/** Add a report to a metadata list.
 * @param list The metadata list.
 * @param text The report text.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_Report (struct List *list, const char *text);

/** Add a CVE to a metadata list.
 * @param list The metadata list.
 * @param text The CVE number.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_CVE (struct List *list, const char *text);

/** Add a BugTraq ID to a metadata list.
 * @param list The metadata list.
 * @param text The BugTraq ID.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_BID (struct List *list, const char *text);

/** Add a OSVDB id to a metadata list.
 * @param list The metadata list.
 * @param text The OSVDB Id.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_OSVDB (struct List *list, const char *text);

/** Add a HTTP request to a metadata list.
 * @param list The metadata list.
 * @param name The HTTP Request
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_HttpRequest (struct List *list, const char *name);

/** Add a HTTP Response to a metadata list.
 * @param list The metadata list.
 * @param name The HTTP Response
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_HttpResponse (struct List *list, const char *name);

/** Add an IPv4 source address to a metadata list.
 * @param list The metadata list.
 * @param addr The address.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_IPv4_Source (struct List *list, const uint8_t *addr);

/** Get an IPv4 source address from a metadata list.
 * @param list The metadata list.
 * @param addr A pointer to a uint8_t* to store the address in.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_IPv4_Source (struct List *list, const uint8_t **addr);

/** Add an IPv4 destination address to a metadata list.
 * @param list The metadata list.
 * @param addr The address.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_IPv4_Destination (struct List *list, const uint8_t *addr);

/** Get an IPv4 destination address from a metadata list.
 * @param list The metadata list.
 * @param addr A pointer to a uint8_t* to store the address in.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_IPv4_Destination (struct List *list, const uint8_t **addr);

/** Add an IPv6 source address to a metadata list.
 * @param list The metadata list.
 * @param addr The address.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_IPv6_Source (struct List *list, const uint8_t *addr);

/** Get an IPv6 source address from a metadata list.
 * @param list The metadata list.
 * @param addr A pointer to a uint8_t* to store the address in.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_IPv6_Source (struct List *list, const uint8_t **addr);

/** Add an IPv6 destination address to a metadata list.
 * @param list The metadata list.
 * @param addr The address.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_IPv6_Destination (struct List *list, const uint8_t *addr);

/** Get an IPv6 destination address from a metadata list.
 * @param list The metadata list.
 * @param addr A pointer to a uint8_t* to store the address in.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_IPv6_Destination (struct List *list, const uint8_t **addr);

/** Add a source port to a metadata list.
 * @param list The metadata list.
 * @param port The port.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_Port_Source (struct List *list, const uint16_t port);

/** Get a source port from a metadata list.
 * @param list The metadata list.
 * @param port A pointer to a uint16_t to store the port in.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_Port_Source (struct List *list, uint16_t *port);

/** Add a destination port to a metadata list.
 * @param list The metadata list.
 * @param port The port.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Add_Port_Destination (struct List *list, const uint16_t port);

/** Get a destination port from a metadata list.
 * @param list The metadata list.
 * @param port A pointer to a uint16_t to store the port in.
 * @return True on success, false on error.
 */
SO_PUBLIC extern bool Metadata_Get_Port_Destination (struct List *list, uint16_t *port);
#ifdef __cplusplus
}
#endif
#endif
