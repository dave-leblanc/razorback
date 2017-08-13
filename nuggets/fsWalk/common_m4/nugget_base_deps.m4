m4_include([common_m4/api_deps.m4])

NUGGET_VERSION=NUGGET_VERSION_NUMBER

# Support searching the for the API in $prefix and in the argument to --with-api
export PKG_CONFIG_PATH=$prefix/lib/pkgconfig:$PKG_CONFIG_PATH
if test "x$with_api" != "x"; then
    export PKG_CONFIG_PATH=$with_api:$PKG_CONFIG_PATH
fi

PKG_CHECK_MODULES([RAZORBACK], [razorback >= 0.1.3])

# Munge the compiler flags if we specify where the API is located.
if test "x$with_api" != "x"; then
    RAZORBACK_CFLAGS="-I$with_api/include -I$with_api/libssh/include"
    RAZORBACK_LIBS="-L$with_api/src -lrazorback_api"
else
    RAZORBACK_CFLAGS="-I${includedir} -I${includedir}/razorback"
fi
