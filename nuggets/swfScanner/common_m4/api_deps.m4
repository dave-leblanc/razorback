# Setup the sysconfdir for all consumers.
m4_include(common_m4/as_ac_expand.m4)
m4_include(common_m4/set_dirs.m4)

AC_CHECK_LIB([rt], [timer_create])
AC_CHECK_LIB([m], [floor])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([crypto], [EVP_DigestInit])
AC_CHECK_LIB([ssl], [SSL_write])
AC_CHECK_LIB([magic], [magic_open])
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([pcre], [pcre_compile])

AC_CHECK_LIB([uuid], [uuid_copy], [], [UUID="no"])
AS_IF([test "x$UUID" = "xno"],
          [AC_MSG_ERROR("uuid library not found")]
     )

#AC_CHECK_LIB([curl], [curl_easy_perform], [
#    CURL_CFLAGS=`curl-config --cflags`
#    CURL_LIBS=`curl-config --libs`
#    CFLAGS="$CFLAGS $CURL_CFLAGS"
#    LIBS="$CURL_LIBS $LIBS"
#], [CURL="no"])
#AS_IF([test "x$CURL" = "xno"],
#          [AC_MSG_ERROR("curl library not found")]
#     )

AC_CHECK_HEADERS([uuid/uuid.h], [], [UUID_HEADER="no"])
AS_IF([test "x$UUID_HEADER" = "xno"],
          [AC_MSG_ERROR("uuid header file not found")]
     )

AC_CHECK_HEADERS([openssl/evp.h], [], [OPENSSL_HEADER="no"])
AS_IF([test "x$OPENSSL_HEADER" = "xno"],
          [AC_MSG_ERROR("openssl header file not found")]
     )

#AC_CHECK_HEADERS([magic.h], [], [MAGIC_HEADER="no"])
#AS_IF([test "x$MAGIC_HEADER" = "xno"],
#          [AC_MSG_ERROR("magic header file not found")]
#     )

#AC_CHECK_HEADERS([pcre.h], [], [PCRE_HEADER="no"])
#AS_IF([test "x$PCRE_HEADER" = "xno"],
#          [AC_MSG_ERROR("pcre header file not found")]
#     )

PKG_CHECK_MODULES([LIBCONFIG], [libconfig >= 1.3.2])
CFLAGS="$LIBCONFIG_CFLAGS $CFLAGS"
LIBS="$LIBCONFIG_LIBS $LIBS"

#AC_CHECK_HEADERS([curl/curl.h], [], [CURL_HEADER="no"])
#AS_IF([test "x$CURL_HEADER" = "xno"],
#          [AC_MSG_ERROR("curl header file not found")]
#     )

PKG_CHECK_MODULES([JSON], [json >= 0.9])

CFLAGS="$JSON_CFLAGS $CFLAGS"
LIBS="$JSON_LIBS $LIBS"

