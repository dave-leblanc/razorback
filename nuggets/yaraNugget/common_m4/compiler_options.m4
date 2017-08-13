ICC=no
if eval "echo $CC|grep icc >/dev/null" ; then
    if eval "$CC -help |grep libcxa >/dev/null" ; then
        CFLAGS="$CFLAGS -ip -static-libcxa"
        XCCFLAGS="-XCClinker -static-libcxa"
    else
        CFLAGS="$CFLAGS -ip -static-intel"
        XCCFLAGS="-XCClinker -static-intel"
    fi
    if test "$COPT" = "-O2" ; then
        COPT="-O3"
    fi
    CWARNINGS="-wd869"
    ICC=yes
    GCC=no
fi

if test "$GCC" = yes ; then
    if test "x$enable_debug" = "xyes"; then
        CFLAGS="-ggdb $CFLAGS"
    else
        CFLAGS="-O2 $CFLAGS"
    fi

    if test "x$enable_profile" = "xyes"; then
        LDFLAGS="$LDFLAGS -XCClinker -pg"
    fi

    CFLAGS="$CFLAGS -std=c99 -fno-strict-aliasing"
    CPPFLAGS="$CPPFLAGS -fno-strict-aliasing"
    CWARNINGS="$CWARNINGS -Wall -Werror -Wwrite-strings -Wformat -fdiagnostics-show-option -Wextra -Wformat-security -Wsign-compare -Wcast-align -Wno-unused-parameter"
    if test "x$NOT_PEDANTIC" = "x"; then
        CWARNINGS="$CWARNINGS -pedantic"
    fi
    echo $CFLAGS
fi

