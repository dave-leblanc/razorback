#include <razorback/visibility.h>
#ifndef ENABLE_ASSERT
#define ENABLE_ASSERT
#endif
#ifndef ENABLE_UNIMPLEMENTED_ASSERT
#define ENABLE_UNIMPLEMENTED_ASSERT
#endif


#define __func__ __FUNCTION__

// String handling functions
#ifndef snprintf
#define snprintf _snprintf
#endif

#define strcasecmp(X,Y)  _stricmp(X,Y)
#define strncasecmp(X,Y,Z)  _strnicmp(X,Y,Z)

// File handling funcitons
#define access _access
#define R_OK 04
#define W_OK 02
#define F_OK 00
#define write _write
#define open _open
#define close _close
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#define S_IRGRP 0
#define S_IROTH 0

#define be64toh(a) _byteswap_uint64(a)
#define be32toh(a) _byteswap_ulong(a)
#define be16toh(a) _byteswap_ushort(a)

#define htobe64(a) _byteswap_uint64(a)
#define htobe32(a) _byteswap_ulong(a)
#define htobe16(a) _byteswap_ushort(a)

//#define inet_ntop InetNtop
//#define inet_pton InetPton
//#define ctime_r ctime

#define HAVE__SNPRINTF_S
#define HAVE__VSNPRINTF_S
#define HAVE_LIBCRYPTO
#define HAVE_GETADDRINFO

#define HAVE_OPENSSL_AES_H
#define HAVE_OPENSSL_BLOWFISH_H
#define HAVE_OPENSSL_DES_H
#define HAVE_OPENSSL_ECDH_H
#define LIBSSH_EXPORTS
#ifndef WITH_SERVER
#define WITH_SERVER
#endif
#define WITH_SFTP
#define ETC_DIR "C:\\Razorback\\"
//#define CNC_DEBUG
//#define STOMP_DEBUG