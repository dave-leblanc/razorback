/** @file visibility.h
 * Library symbol visibility macro.
 */
#ifndef _RAZORBACK_VISIBILTY_H
#define _RAZORBACK_VISIBILTY_H

#if defined _WIN32 || defined __CYGWIN__
#  ifdef BUILDING_SO
#    ifdef __GNUC__
#      define SO_PUBLIC __attribute__((dllexport))
#    else
#      define SO_PUBLIC __declspec(dllexport)   // Note: actually gcc seems to also supports this syntax.
#    endif
#  else
#    ifdef __GNUC__
#      define SO_PUBLIC __attribute__((dllimport))
#    else
#      define SO_PUBLIC __declspec(dllimport)   // Note: actually gcc seems to also supports this syntax.
#    endif
#  endif
#  define DLL_LOCAL
#  define PRINTF_FUNC(x,y)
#else
#  if __GNUC__ >= 4
#    define SO_PUBLIC __attribute__ ((visibility("default")))
#    define SO_LOCAL  __attribute__ ((visibility("hidden")))
#  else
#    define SO_PUBLIC
#    define SO_LOCAL
#  endif
#  define PRINTF_FUNC(x,y) __attribute__((format (printf, x, y)))
#endif

#endif /* _RAZORBACK_VISIBILTY_H */
