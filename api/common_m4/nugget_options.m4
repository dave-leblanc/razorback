m4_include([common_m4/standard_options.m4])

AC_ARG_WITH(with_api, [AS_HELP_STRING([--with-api],[Location of the API])], [with_api=$withval], [])

AC_ARG_ENABLE(master-nugget-check, [AS_HELP_STRING([--disable-master-nugget-check], [Do not check for the presence of master_nugget even if its required.])], [enable_master_nugget_check=$enableval], [enable_master_nugget_check="yes"])

