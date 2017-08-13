AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug],[Enable debug build options])], [enable_debug=$enableval], [enable_debug="no"])
if test "x$enable_debug" = "xyes"; then
    AC_DEFINE([DEBUG], 1, [Debug Mode])
fi
AC_ARG_ENABLE([assert], [AS_HELP_STRING([--enable-assert],[Enable assert build options])], [enable_assert=$enableval], [enable_assert="no"])
if test "x$enable_assert" = "xyes"; then
    AC_DEFINE([ENABLE_ASSERT], 1, [Asserts Enabled])
fi

AC_ARG_ENABLE([unimplemented], [AS_HELP_STRING([--enable-unimplemented],[Enable unimplemented asserts options])], [enable_unimplemented=$enableval], [enable_unimplemented="no"])
if test "x$enable_unimplemented" = "xyes"; then
    AC_DEFINE([ENABLE_UNIMPLEMENTED_ASSERT], 1, [Asserts Enabled])
fi


INLINE=''
AC_ARG_ENABLE([inline], [AS_HELP_STRING([--disable-inline], [Do not use inline functions])], [enable_inline=$enableval], [enable_inline="yes"])
if test "x$enable_inline" = "xyes"; then
    INLINE="inline"
fi
AC_DEFINE_UNQUOTED(INLINE, $INLINE, [Inlining status])

AC_ARG_ENABLE([perf], [AS_HELP_STRING([--disable-perf], [Disable performance statistics])], [enable_perf=$enableval], [enable_perf="yes"])
if test "x$enable_perf" = "xyes"; then
    AC_DEFINE([RZB_PERF], 1, [Performance Stats])
fi

AC_ARG_ENABLE([profile], [AS_HELP_STRING([--enable-profile], [Enable profile build options])], [enable_profile=$enableval], [enable_profile="no"])
if test "x$enable_profile" = "xyes"; then
    AC_DEFINE([PROFILE], 1, [Profiling])
    PROFILE_LDFLAGS="-XCClinker -pg"
fi

