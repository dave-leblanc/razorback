##
## Copyright (C) 2008 Sourcefire Inc., All Rights Reserved.
##
## Please see the LICENSE file distributed with this source
## code archive for information covering the modification and
## redistribution of this file and binaries built from it.
##

AC_PREREQ([2.61])
m4_include([common_m4/versions.m4])
AC_INIT([clamavNugget], CLAMAV_NUGGET_VERSION_NUMBER, [https://sourceforge.net/apps/trac/razorbacktm/], [clamavNugget])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/ClamAVNugget.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign no-define tar-ustar])

# Pull in the checks for the build tools.
m4_include([common_m4/build_tools.m4])

# Pull in the platform specific build/configure environment options.
m4_include([common_m4/platform_flags.m4])

# Pull in the standard options for all components
m4_include([common_m4/nugget_options.m4])

# Pull in the library requirements for a simple .so loadable nugget.
m4_include([common_m4/nugget_simple_deps.m4])

#PKG_CHECK_MODULES([CLAMAV], [libclamav >= 7:1:1], [CLAM="YES"], [CLAM="NO"])
#if test "x$CLAM" = "xNO"; then
#    PKG_CHECK_MODULES([CLAMAV], [libclamav >= 0.97])
#fi
#
#CFLAGS="$CLAMAV_CFLAGS $CFLAGS"
#LIBS="$CLAMAV_LIBS $LIBS"
#
# Pull in the standard header and function checks.
m4_include([common_m4/standard_headers_functions.m4])

# Pull in the compiler customisation options for different compilers.
m4_include([common_m4/compiler_options.m4])

# Pull in the standard compiler and linker flags.
m4_include([common_m4/cflags.m4])

# Pulling configuration for standard nugget substituions.
m4_include([common_m4/nugget_subs.m4])


AC_CONFIG_FILES([ \
Makefile \
etc/Makefile \
m4/Makefile \
src/Makefile \
])
AC_OUTPUT
