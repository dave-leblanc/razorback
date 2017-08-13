#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
m4_include([common_m4/versions.m4])
AC_INIT([api], API_VERSION_NUMBER, [https://sourceforge.net/apps/trac/razorbacktm/])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/thread.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign no-define tar-ustar subdir-objects])

APIVERSION=API_LIB_VERSION_NUMBER

# Pull in the checks for the build tools.
m4_include([common_m4/build_tools.m4])

# Pull in the platform specific build/configure environment options.
m4_include([common_m4/platform_flags.m4])

# Pull in the dependancies on on the API (use the nugget way).
m4_include([common_m4/api_deps.m4])

# Pull in the standard header and function checks.
m4_include([common_m4/standard_headers_functions.m4])

# Pull in the standard options for all components
m4_include([common_m4/standard_options.m4])

AC_ARG_ENABLE([cnc_debug], [AS_HELP_STRING([--enable-cnc-debug],[Enable command and control debug])], [enable_cnc_debug=$enableval], [enable_cnc_debug="no"])
if test "x$enable_cnc_debug" = "xyes"; then
    AC_DEFINE([CNC_DEBUG], 1, [Command and Control Debug Mode])
fi

AC_ARG_ENABLE([stomp_debug], [AS_HELP_STRING([--enable-stomp-debug],[Enable stomp debug])], [enable_stomp_debug=$enableval], [enable_stomp_debug="no"])
if test "x$enable_stomp_debug" = "xyes"; then
    AC_DEFINE([STOMP_DEBUG], 1, [Stomp Debug Mode])
fi

AC_ARG_ENABLE([magic_debug], [AS_HELP_STRING([--enable-magic-debug],[Enable magic debug])], [enable_magic_debug=$enableval], [enable_magic_debug="no"])
if test "x$enable_magic_debug" = "xyes"; then
    AC_DEFINE([MIME_MAGIC_DEBUG], 1, [Magic Debug Mode])
fi
m4_include([libssh/configure.m4])

AC_CHECK_HEADERS([sys/endian.h], [], [SYS_ENDIAN_HEADER="no"]) 
# Pull in the compiler customisation options for different compilers.
m4_include([common_m4/compiler_options.m4])

# Pull in the standard compiler and linker flags.
m4_include([common_m4/cflags.m4])

AC_SUBST(XCCFLAGS)
AC_SUBST(APIVERSION)
AC_SUBST(LIBS)
AC_SUBST(extra_incl)

AC_CONFIG_FILES([ \
razorback.pc \
Makefile \
m4/Makefile \
etc/Makefile \
include/Makefile \
src/Makefile \
libssh/Makefile \
libssh/src/Makefile \
libssh/src/threads/Makefile \
libssh/include/Makefile \
tests/Makefile \
])
AC_OUTPUT
