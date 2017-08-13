AC_DEFINE([WITH_SERVER], 1, [Server Enabled])
AC_DEFINE([WITH_SSH1], 1, [SSH v1 Enabled])
AC_DEFINE([WITH_SFTP], 1, [SFTP Enabled])
AC_DEFINE([WITH_ZLIB], 1, [ZLIB Enabled])

AC_CHECK_HEADERS([argp.h])
AC_CHECK_HEADERS([pty.h])
AC_CHECK_HEADERS([termios.h])
AC_CHECK_HEADERS([openssl/aes.h])
AC_CHECK_HEADERS([openssl/blowfish.h])
AC_CHECK_HEADERS([openssl/des.h])
AC_CHECK_HEADERS([openssl/ec.h])
AC_CHECK_HEADERS([openssl/ecdsa.h])
AC_CHECK_HEADERS([openssl/ecdh.h])
AC_CHECK_HEADERS([wspiapi.h])
AC_CHECK_HEADERS([pthread.h])

AC_CHECK_FUNCS([snprintf])
AC_CHECK_FUNCS([_snprintf])
AC_CHECK_FUNCS([_snprintf_s])
AC_CHECK_FUNCS([vsnprintf])
AC_CHECK_FUNCS([_vsnprintf])
AC_CHECK_FUNCS([_vsnprintf_s])
AC_CHECK_FUNCS([strncpy])
AC_CHECK_FUNCS([cfmakeraw])
AC_CHECK_FUNCS([getaddrinfo])
AC_CHECK_FUNCS([poll])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([ntohll])

AC_CHECK_HEADERS([crypt.h])
AC_CHECK_LIB([crypto], [BN_new], [], [CRYPTO="no"])
AS_IF([test "x$CRYPTO" = "xno"],
             [AC_MSG_ERROR("crypto header file not found")]
     )
AC_DEFINE([HAVE_LIBCRYPTO], 1, [GCrypt Enabled])
 
##AC_CHECK_HEADERS([gcrypt.h])
##AC_CHECK_LIB([gcrypt], [gcry_strerror], [], [GCRYPT="no"])
##AS_IF([test "x$GCRYPT" = "xno"],
##             [AC_MSG_ERROR("gcrypt header file not found")]
##     )

AC_DEFINE([HAVE_PTHREAD], 1, [Force define as we check and define HAVE_LIBPTHREAD])
 

AC_CHECK_HEADERS([zlib.h])
AC_CHECK_LIB([z], [deflate], [], [ZLIB]="no")
AS_IF([test "x$ZLIB" = "xno"],
             [AC_MSG_ERROR("zlib header file not found")]
     )
AC_DEFINE([WITH_ZLIB], 1, [Zlib Enabled])
 
