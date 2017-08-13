AC_CHECK_LIB([curl], [curl_easy_perform], [
    CURL_CFLAGS=`curl-config --cflags`
    CURL_LIBS=`curl-config --libs`
    CFLAGS="$CFLAGS $CURL_CFLAGS"
    LIBS="$CURL_LIBS $LIBS"
], [CURL="no"])
AS_IF([test "x$CURL" = "xno"],
          [AC_MSG_ERROR("curl library not found")]
     )

AC_CHECK_HEADERS([curl/curl.h], [], [CURL_HEADER="no"])
AS_IF([test "x$CURL_HEADER" = "xno"],
          [AC_MSG_ERROR("curl header file not found")]
     )


