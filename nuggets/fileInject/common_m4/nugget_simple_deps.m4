# This filw provides the basic autoconf macros for creating a simple .so loadable nugget for use behind master_nugget.

m4_include([common_m4/nugget_base_deps.m4])

if test "x$enable_master_nugget_check" = "xyes"; then
    export PATH=$prefix/bin:$PATH
    AC_CHECK_PROG([MASTER_NUGGET], [masterNugget],[masterNugget],["no"],,)
    AS_IF([test "x$MASTER_NUGGET" = "xno"],
              [AC_MSG_ERROR("master_nugget not found")]
         )
fi
m4_include([common_m4/visibility.m4])
