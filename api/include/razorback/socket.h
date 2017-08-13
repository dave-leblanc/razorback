/** @file socket.h
 * Socket API.
 */

#ifndef	RAZORBACK_SOCKET_H
#define RAZORBACK_SOCKET_H

#include <razorback/visibility.h>
#include <razorback/types.h>
#include <sys/types.h>
#ifdef _MSC_VER
#include <WinSock2.h>
#define ssize_t SSIZE_T
#else //_MSC_VER
#include <sys/socket.h>
#endif //_MSC_VER
#include <openssl/ssl.h>


#ifdef __cplusplus
extern "C" {
#endif

/** Socket Structure
 */
struct Socket
{
#ifdef _MSC_VER
	SOCKET iSocket;
#else
    int iSocket;           ///< The Socket FD
#endif
    struct addrinfo *pAddressInfo;
    bool ssl;
    SSL *sslHandle;
    SSL_CTX *sslContext;
};

/** Starts a socket listening
 * @param *sourceAddress The address
 * @param port The port
 * @return A new socket or NULL on error.
 */
SO_PUBLIC extern struct Socket * Socket_Listen (const uint8_t * sourceAddress,
                           uint16_t port);

/** Start a UNIX socket listening 
 * @param path Path to the socket file
 * @return A new socket or NULL on error.
 */
SO_PUBLIC extern struct Socket * Socket_Listen_Unix (const char * path);

/** Starts a socket listening
 * @param *destinationAddress The address
 * @param port The port
 * @return A new socket or NULL on error.
 */
SO_PUBLIC extern struct Socket * SSL_Socket_Connect ( const uint8_t * destinationAddress,
                            uint16_t port);

/** Starts a socket from a listening socket
 * @param *socket the socket
 * @param *listeningSocket the listening socket
 * @return 0 = timeout, -1 = error, 1 = ok
 */
SO_PUBLIC extern int Socket_Accept (struct Socket **socket,
                          const struct Socket *listeningSocket);

/** Starts a connecting socket
 * @param *destinationAddress the address
 * @param port the port
 * @return a new socket on success null on failure.
 */
SO_PUBLIC extern struct Socket * Socket_Connect ( const uint8_t * destinationAddress,
                            uint16_t port);

/** Close a socket
 * @param *socket the socket
 */
SO_PUBLIC extern void Socket_Close (struct Socket *socket);

/** transmits on a socket
 * @param *socket the socket
 * @param len the size of data
 * @param *buffer the data
 * @return The number of bytes sent on success, -1 on error, 0 on socket closed
 */
SO_PUBLIC extern ssize_t Socket_Tx (const struct Socket *socket, size_t len,
                       const uint8_t * buffer);

/** Formats a string and transmits it on a socket
 * @param socket the socket to use.
 * @param fmt The printf format string
 * @param ... the format arguments.
 * @return true on success false on error.
 */
SO_PUBLIC PRINTF_FUNC(2,3) extern bool
Socket_Printf (const struct Socket *socket, const char *fmt, ...);

/** receives on a socket
 * @param *socket the socket
 * @param len the size to read
 * @param *buffer the data
 * @return The number of bytes read on success, -1 on error, 0 on socket closed
 */
SO_PUBLIC extern ssize_t Socket_Rx (const struct Socket *socket, size_t len,
                       uint8_t * buffer);

/** receives on a socket until the terminator is reached
 * @param socket the socket
 * @param r_buffer Buffer with the read data
 * @param terminator the terminating character
 * @return -1 on error, 0 on EOF, len+1 if the terminator was not found, or the amount of data read >0 && <= len if the terminator was found.
 */
SO_PUBLIC extern ssize_t Socket_Rx_Until (const struct Socket *socket, uint8_t ** r_buffer,
                             uint8_t terminator);

#if 0
/** determines whether there is any data available
 * @param socket the socket
 * @return true if data available, false otherwise
 */
SO_PUBLIC extern bool Socket_ReadyForRead (const struct Socket *socket);
#endif

#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_SOCKET_H
