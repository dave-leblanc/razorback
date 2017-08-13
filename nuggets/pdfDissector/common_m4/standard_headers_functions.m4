# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h unistd.h stdarg.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_TYPE_UINTPTR_T
AC_TYPE_INTPTR_T
AC_TYPE_INTMAX_T
AC_TYPE_UINTMAX_T
AC_CHECK_TYPES([size_t])
AC_CHECK_TYPES([ssize_t])
AC_CHECK_TYPES([uint16_t])
AC_CHECK_TYPES([uint32_t])
AC_CHECK_TYPES([uint8_t])
AC_CHECK_TYPES([uintptr_t])
AC_CHECK_TYPES([intptr_t])
AC_CHECK_TYPES([intmax_t])
AC_CHECK_TYPES([uintmax_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([inet_ntoa memset munmap socket strdup strerror strncasecmp strstr strtoul])

