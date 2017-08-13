#include "config.h"
#include <razorback/debug.h>
#include <razorback/socket.h>
#include <razorback/log.h>

#ifdef _MSC_VER
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <io.h>
#pragma comment(lib, "Ws2_32.lib")
#define OPT_CAST const char *
#else
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#define OPT_CAST void *
#define closesocket(x) close(x)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#include <openssl/ssl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


/* controls the amount sent per call to read or write */
#define	MAXRWSIZE	1024

static void
Socket_Destroy (struct Socket *sock)
{
    if (sock->pAddressInfo != NULL)
        freeaddrinfo (sock->pAddressInfo);
    free(sock);
}

static bool
Socket_CopyAddress (struct Socket *dest, const struct Socket *source)
{
    ASSERT (dest->pAddressInfo == NULL);
    if (dest->pAddressInfo != NULL)
    	return false;
    
    ASSERT (source->pAddressInfo != NULL);
    if (source->pAddressInfo == NULL)
    	return false;

    if ((dest->pAddressInfo = (struct addrinfo *)calloc(1,sizeof(struct addrinfo))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new address info", __func__);
        return false;
    }

    dest->pAddressInfo->ai_flags= source->pAddressInfo->ai_flags;
    dest->pAddressInfo->ai_family= source->pAddressInfo->ai_family;
    dest->pAddressInfo->ai_socktype= source->pAddressInfo->ai_socktype;
    dest->pAddressInfo->ai_protocol= source->pAddressInfo->ai_protocol;
    dest->pAddressInfo->ai_addrlen= source->pAddressInfo->ai_addrlen;
    
    dest->pAddressInfo->ai_next= NULL; // No next when we clone it.

    dest->pAddressInfo->ai_canonname = NULL;
    

    if ((dest->pAddressInfo->ai_addr = (struct sockaddr *)
         malloc (source->pAddressInfo->ai_addrlen)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new address", __func__);
        return false;
    }

    memcpy (dest->pAddressInfo->ai_addr, source->pAddressInfo->ai_addr,
            source->pAddressInfo->ai_addrlen);

    return true;
}

static bool
SocketAddress_Initialize (struct Socket *sock,
                          const uint8_t * address, uint16_t port)
{
	struct addrinfo aiHints;
    char portAsString[32];
	int ret;

    ASSERT (sock->pAddressInfo == NULL);
    if (sock->pAddressInfo != NULL)
    {
        rzb_log(LOG_ERR, "%s: Double address init", __func__);
        return false;
    }
    
    sprintf (portAsString, "%i", port);
    memset (&aiHints, 0, sizeof (struct addrinfo));
    
    aiHints.ai_family = AF_UNSPEC;
    aiHints.ai_socktype = SOCK_STREAM;
    aiHints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

	aiHints.ai_protocol = IPPROTO_TCP;
        
    if ((ret = getaddrinfo
        ((const char *) address, portAsString,
         &aiHints, &sock->pAddressInfo)) != 0)
    {

#ifdef _MSC_VER
		rzb_log(LOG_ERR, "Failed to get address info: %S, %d, %S", address, ret, gai_strerror(ret));
#else
        rzb_perror
            ("Failed to get address information in SocketAddress_Initialize: %s");
#endif
        sock->pAddressInfo = NULL;

        return false;
    }

    return true;
}

SO_PUBLIC struct Socket *
Socket_Listen (const unsigned char *sourceAddress, uint16_t port)
{
    struct Socket *sock;
    int on = 1;

	ASSERT (sourceAddress != NULL);
	if (sourceAddress == NULL)
		return NULL;

	ASSERT (port > 0);
	if (port <= 0)
		return  NULL;

    if ((sock = (struct Socket *)calloc (1, sizeof (struct Socket))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new socket", __func__);
        return NULL;
    }
   
    if (!SocketAddress_Initialize
        (sock, sourceAddress, port))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of SocketAddress_Initialize", __func__);
        Socket_Destroy(sock);
        return NULL;
    }

    if ((sock->iSocket =
         socket (sock->pAddressInfo->ai_family, sock->pAddressInfo->ai_socktype,
                 sock->pAddressInfo->ai_protocol)) == -1)
    {
        Socket_Destroy (sock);
        rzb_perror ("Socket_Listen failed due to failure of socket call: %s");
        return NULL;
    }

    if (setsockopt( sock->iSocket, SOL_SOCKET, SO_REUSEADDR, (OPT_CAST)&on, sizeof(on) ) == -1)
    {
        Socket_Destroy (sock);
        rzb_perror ("Socket_Listen failed due to failure of setsockopt: %s");
        return NULL;
    }
    if (bind
        (sock->iSocket,
         sock->pAddressInfo->ai_addr,
         sock->pAddressInfo->ai_addrlen) == -1)
    {
        Socket_Destroy (sock);
        rzb_perror ("Socket_Listen failed due to failure of bind call: %s");
        return NULL;
    }

    if (listen (sock->iSocket, SOMAXCONN) == -1)
    {
        Socket_Destroy (sock);
        rzb_perror ("Socket_Listen failed due to failure of listen call: %s");
        return NULL;
    }

    return sock;
}

SO_PUBLIC struct Socket *
Socket_Listen_Unix (const char *path)
{
#ifdef _MSC_VER
	ASSERT(false);
	return NULL;
#else
    struct Socket *sock;
    struct sockaddr_un *server;

    ASSERT (path != NULL);
    if (path == NULL)
    	return NULL;

    if((server = calloc(1, sizeof(struct sockaddr_un))) == NULL)
        return NULL;

    server->sun_family = AF_UNIX;
    strncpy (server->sun_path, path, sizeof (server->sun_path));
    server->sun_path[sizeof (server->sun_path) - 1] = '\0';

    if ((sock = (struct Socket *)calloc (1, sizeof (struct Socket))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new socket", __func__);
        return NULL;
    }
    if ((sock->pAddressInfo = (struct addrinfo *)calloc(1,sizeof(struct addrinfo))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new address info", __func__);
        return false;
    }

    sock->pAddressInfo->ai_family=AF_UNIX;
    sock->pAddressInfo->ai_next= NULL;
    sock->pAddressInfo->ai_canonname = NULL;
    sock->pAddressInfo->ai_addrlen= sizeof(struct sockaddr_un);
    sock->pAddressInfo->ai_addr = (struct sockaddr *)server;

    if ((sock->iSocket = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        Socket_Destroy (sock);
        rzb_perror ("Socket_Listen failed due to failure of socket call: %s");
        return NULL;
    }

    if (bind
        (sock->iSocket,
         sock->pAddressInfo->ai_addr,
         sock->pAddressInfo->ai_addrlen) == -1)
    {
        Socket_Destroy (sock);
        rzb_perror ("Socket_Listen_Unix failed due to failure of bind call: %s");
        return NULL;
    }

    if (listen (sock->iSocket, SOMAXCONN) == -1)
    {
        Socket_Destroy (sock);
        rzb_perror ("Socket_Listen failed due to failure of listen call: %s");
        return NULL;
    }

    return sock;
#endif
}

/* Returns: 0 = timeout, -1 = error, 1 = ok */
SO_PUBLIC int
Socket_Accept (struct Socket **retSock,
               const struct Socket *listeningSocket)
{
    struct Socket *sock = NULL;
    fd_set fdSet;
    struct timeval timeout;

    ASSERT (retSock != NULL);
    if (retSock == NULL)
    	return -1;

    ASSERT (listeningSocket != NULL);
    if (listeningSocket == NULL)
    	return -1;

    if ((sock = (struct Socket *)calloc(1, sizeof (struct Socket))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new socket", __func__);
        return -1;
    }

    Socket_CopyAddress (sock, listeningSocket);

    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    FD_ZERO (&fdSet);
    FD_SET (listeningSocket->iSocket, &fdSet);
    if (select (listeningSocket->iSocket +1, &fdSet, NULL, NULL, &timeout) < 0)
    {
        Socket_Destroy (sock);
        rzb_perror
            ("Socket_Accept failed due to failure of accept call: %s");
        return -1;
    }
    
    {
        // check for error
        if ((sock->iSocket =
             accept (listeningSocket->iSocket,
                     sock->pAddressInfo->ai_addr,
                     &sock->pAddressInfo->ai_addrlen)) == -1)
        {
            Socket_Destroy (sock);
            rzb_perror
                ("Socket_Accept failed due to failure of accept call: %s");
            return -1;
        }
        *retSock = sock;
        return 1;
    }
    return 0;
}

SO_PUBLIC struct Socket *
Socket_Connect (const unsigned char *destinationAddress, uint16_t port)
{
    struct addrinfo *cur = NULL;
    struct Socket *sock = NULL;

#ifdef _MSC_VER
	DWORD nTimeout = 5000;
#endif

	ASSERT (destinationAddress != NULL);

    if ((sock = (struct Socket *)calloc (1, sizeof (struct Socket))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new socket", __func__);
        return NULL;
    }
    sock->ssl =false;

    if (!SocketAddress_Initialize
        (sock, destinationAddress, port))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of SocketAddress_Initialize", __func__);
        return NULL;
    }
    
    cur = sock->pAddressInfo;
    while (cur != NULL)
    {
		if ((sock->iSocket = socket (cur->ai_family, cur->ai_socktype, cur->ai_protocol)) ==INVALID_SOCKET)
        {
            rzb_perror
                ("Socket_Connect failed due to failure of socket call: %s");
            cur = cur->ai_next;
            continue;
        }

#ifdef _MSC_VER
		setsockopt(sock->iSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeout, sizeof(int));
#endif

		if (connect(sock->iSocket, cur->ai_addr, cur->ai_addrlen) == SOCKET_ERROR)
        {
            rzb_perror
                ("Socket_Connect failed due to failure of connect call: %s");
            cur = cur->ai_next;
			closesocket(sock->iSocket);
            continue;
        }
        return sock;
    }

    rzb_log(LOG_ERR,"%s: All possible hosts exhausted", __func__);
    Socket_Close(sock);
    return NULL;
}

SO_PUBLIC struct Socket * 
SSL_Socket_Connect ( const uint8_t * destination, uint16_t port)
{
    struct Socket *sock;

    ASSERT (destination != NULL);
    if (destination == NULL)
    	return NULL;

    ASSERT(port > 0);
    if (port <= 0)
    	return NULL;

    if ((sock = Socket_Connect(destination, port)) == NULL)
    {
        return NULL;
    }

    sock->ssl =true;
    if ((sock->sslContext = SSL_CTX_new(SSLv23_client_method())) == NULL)
    {
        Socket_Close(sock);
        return NULL;
    }
    if ((sock->sslHandle = SSL_new (sock->sslContext)) == NULL)
    {
        Socket_Close(sock);
        return NULL;
    }
    
    if (!SSL_set_fd (sock->sslHandle, sock->iSocket))
    {
        Socket_Close(sock);
        return NULL;
    }

    // Initiate SSL handshake
    if (SSL_connect (sock->sslHandle) != 1)
    {
        Socket_Close(sock);
        return NULL;
    }
    return sock;
}

SO_PUBLIC void
Socket_Close (struct Socket *sock)
{
    ASSERT (sock != NULL);
    if (sock == NULL)
    	return;

    closesocket(sock->iSocket);

    if (sock->ssl)
    {
        if (sock->sslHandle)
        {
            SSL_shutdown (sock->sslHandle);
            SSL_free (sock->sslHandle);
        }
        if (sock->sslContext)
            SSL_CTX_free (sock->sslContext);
    }
    Socket_Destroy (sock);
}

SO_PUBLIC ssize_t
Socket_Tx (const struct Socket *sock, size_t size,
           const uint8_t * buffer)
{
    ssize_t amountSent = 0;
    ssize_t sendThisTime = 0;
    ssize_t sentThisTime = 0;

	ASSERT (sock != NULL);
	if (sock == NULL)
		return -1;

    ASSERT (buffer != NULL);
    if (buffer == NULL)
    	return -1;

    ASSERT (size > 0);
    if (size <= 0)
    	return -1;

    while ((size - amountSent )> 0)
    {
        sendThisTime = size - amountSent;
        if (sendThisTime > MAXRWSIZE)
        {
            sendThisTime = MAXRWSIZE;
        }
        if (sock->ssl)
        {
            sentThisTime =
                SSL_write (sock->sslHandle, buffer + amountSent,
                       sendThisTime);
        }
        else
        {
            sentThisTime =
                send (sock->iSocket, (const char *)buffer + amountSent,
                       sendThisTime, 0);
        }
        if (sentThisTime == SOCKET_ERROR)
        {
#ifdef _MSC_VER
			if (WSAGetLastError() == WSAETIMEDOUT)
				errno = EINTR;
#endif
            if (errno != EINTR && errno != EAGAIN) 
                rzb_perror ("Socket_Rx failed due to failure of read call: %s");

            return -1;
        }
        else if (sentThisTime == 0)
        	return amountSent;
        else
			amountSent += sentThisTime;
    }

    return amountSent;
}

SO_PUBLIC PRINTF_FUNC(2,3) bool
Socket_Printf (const struct Socket *sock, const char *fmt, ...)
{
    char *buffer = NULL;
    va_list argp;

    ASSERT(sock != NULL);
    if (sock == NULL)
    	return false;

    ASSERT(fmt != NULL);
    if (fmt == NULL)
    	return false;

    va_start(argp, fmt);

    if(vasprintf(&buffer, fmt, argp) == -1)
    {
        va_end(argp);
        return false;
    }

    va_end(argp);

    if (Socket_Tx(sock, strlen(buffer), (uint8_t *)buffer) != (ssize_t)strlen(buffer))
    {
        free(buffer);
        return true;
    }
    else
    {
        free(buffer);
        return false;
    }
}


SO_PUBLIC ssize_t
Socket_Rx (const struct Socket * sock, size_t len,
           uint8_t * buffer)
{
    ssize_t received = 0;
    ssize_t toReadThisTime = 0;
    ssize_t readThisTime = 0;

	ASSERT (sock != NULL);
	if (sock == NULL)
		return false;

    ASSERT (len > 0);
    if (len <= 0)
    	return false;

    ASSERT (buffer != NULL);
    if (buffer == NULL)
    	return false;

    while ((len - received) > 0)
    {
        toReadThisTime = len - received;
        if (toReadThisTime > MAXRWSIZE)
            toReadThisTime = MAXRWSIZE;
        if (sock->ssl)
        {
            readThisTime =
                SSL_read (sock->sslHandle, buffer + received,
                  toReadThisTime);
        }
        else
        {
            readThisTime =
                recv (sock->iSocket, (char *)buffer + received,
                      toReadThisTime, 0);
        }

        if (readThisTime == SOCKET_ERROR)
        {
#ifdef _MSC_VER
			if (WSAGetLastError() == WSAETIMEDOUT)
				errno = EINTR;
#endif
            if (errno != EINTR && errno != EAGAIN) 
                rzb_perror ("Socket_Rx failed due to failure of read call: %s");

            return -1;
        }
        else if (readThisTime == 0)
        	return received;
        else
			received += readThisTime;
    }

    return received;
}

SO_PUBLIC ssize_t
Socket_Rx_Until (const struct Socket * sock, uint8_t ** r_buffer,
                 uint8_t terminator)
{
    ssize_t now = 0;
    ssize_t total = 0;
    ssize_t bufSize = MAXRWSIZE;
    uint8_t *buffer = NULL;
    uint8_t *tmp = NULL;

	ASSERT (sock != NULL);
	if (sock == NULL)
		return -1;

	ASSERT (r_buffer != NULL);
	if (r_buffer == NULL)
		return -1;

	if ((buffer = calloc(MAXRWSIZE, sizeof (uint8_t))) == NULL)
		return -1;

    do
    {
        now = Socket_Rx (sock, 1, &buffer[total]);
        if (now == -1)
        {
        	free(buffer);
            if (errno != EINTR && errno != EAGAIN)
                rzb_log (LOG_ERR, "%s: failed due to failure of Socket_Rx", __func__);

            return -1;
        }
        else if (now == 0)
        {
        	free(buffer);
        	return 0;
        }
        total++;
        if (buffer[total-1] == terminator)
        {
        	*r_buffer = buffer;
            return total;
        }
        if (total == bufSize)
        {
        	tmp = buffer;
        	if ((buffer = realloc(buffer, bufSize + MAXRWSIZE)) == NULL)
        	{
        		free(tmp);
        		return -1;
        	}
        }

    } while (now > 0);
    // Unreachable
    return -1;
}
