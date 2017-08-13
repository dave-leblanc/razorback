##
## Copyright (C) 2008 Sourcefire Inc., All Rights Reserved.
##
## Please see the LICENSE file distributed with this source
## code archive for information covering the modification and
## redistribution of this file and binaries built from it.
##

AC_PREREQ([2.61])
m4_include([common_m4/versions.m4])
AC_INIT([dispatcher], DISPATCHER_VERSION_NUMBER, [https://sourceforge.net/apps/trac/razorbacktm/])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/dispatcher.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign no-define tar-ustar subdir-objects])

# Pull in the checks for the build tools.
m4_include([common_m4/build_tools.m4])

# Pull in the platform specific build/configure environment options.
m4_include([common_m4/platform_flags.m4])

# Pull in the standard options for all components
m4_include([common_m4/nugget_options.m4])

# Pull in the dependancies on on the API (use the nugget way).
m4_include([common_m4/nugget_base_deps.m4])

MYSQL=
MYSQL_PATH=`mysql_config --libs_r`
AC_CHECK_LIB([mysqlclient_r], [mysql_init], [], [MYSQL="no"], [${MYSQL_PATH}])

AS_IF([test "x$MYSQL" = "xno"],
          [AC_MSG_ERROR(["mysqlclient_r library not found, go get it from http://www.mysql.com"])]
     )
LIBS="$LIBS $MYSQL_PATH"
MYSQL_CFLAGS=`mysql_config --cflags`

PKG_CHECK_MODULES([LIBMEMCACHED], [libmemcached >= 0.31])

###AC_CHECK_LIB([memcachedutil], [memcached_pool_create], [], [L="no"], [${MYSQL_PATH}])

## Hack to put the MySQL CFLAGS into the CFLAGS var via the RAZORBACK_CFLAGS var.
RAZORBACK_CFLAGS="$RAZORBACK_CFLAGS $MYSQL_CFLAGS $LIBMEMCACHED_CFLAGS"

AC_CHECK_LIB([cli], [cli_init], [], [CLI="no"])
AS_IF([test "x$CLI" = "xno"],
          [AC_MSG_ERROR("cli library not found")]
     )

AC_CHECK_HEADERS([libcli.h], [], [CLI_HEADER="no"])
AS_IF([test "x$CLI_HEADER" = "xno"],
          [AC_MSG_ERROR("cli header file not found")]
     )

AC_CHECK_FUNCS([strtok strtok_r])

SNMP_CFLAGS=''
SNMP_LDFLAGS=''
SNMP_LIBS=''
AC_ARG_ENABLE([snmp], [AS_HELP_STRING([--enable-snmp],[Enable snmp build options])], [enable_snmp=$enableval], [enable_snmp="no"])
if test "x$enable_snmp" = "xyes"; then
    SNMP_LIBS=`net-snmp-config --netsnmp-agent-libs`
    SNMP_CFLAGS=`net-snmp-config --cflags`
    SNMP_LDFLAGS=`net-snmp-config --ldflags`
    AC_CHECK_HEADERS([net-snmp/net-snmp-config.h], [], [SNMP_HEADER="no"])
    AS_IF([test "x$SNMP_HEADER" = "xno"],
              [AC_MSG_ERROR("snmp header file not found")]
         )
    AC_DEFINE([WITH_SNMP], 1, [SNMP Enabled])
fi
AC_SUBST(SNMP_CFLAGS)
AC_SUBST(SNMP_LDFLAGS)
AC_SUBST(SNMP_LIBS)
AM_CONDITIONAL([WITH_SNMP], [test "x$enable_snmp" = "xyes"])

# Pull in the standard header and function checks.
m4_include([common_m4/standard_headers_functions.m4])

# Pull in the compiler customisation options for different compilers.
m4_include([common_m4/compiler_options.m4])

# Pull in the standard compiler and linker flags.
m4_include([common_m4/cflags.m4])

AC_ARG_ENABLE([sftp-debug], [AS_HELP_STRING([--enable-sftp-debug],[Enable sftp debug build options])], [enable_sftp_debug=$enableval], [enable_sftp_debug="no"])
if test "x$enable_sftp_debug" = "xyes"; then
    AC_DEFINE([SFTP_DEBUG], 1, [SFTP Debug Mode])
fi


DISPATCHERINCLUDES='-I$(top_srcdir)/src'
DISPATCHERLIBRARIES='$(RAZORBACK_LIBS) $(LIBMEMCACHED_LIBS)'

AC_SUBST(DISPATCHERINCLUDES)
AC_SUBST(DISPATCHERLIBRARIES)
AC_SUBST(extra_incl)
AC_SUBST(XCCFLAGS)
AS_AC_EXPAND(PKGLIBDIR, "${libdir}/${PACKAGE}")

AC_CONFIG_FILES([ \
Makefile \
doc/Makefile \
etc/Makefile \
etc/init.d/dispatcher \
m4/Makefile \
src/Makefile \
share/Makefile \
tools/Makefile
])
AC_OUTPUT
