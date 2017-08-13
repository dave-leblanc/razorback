#ifndef RAZORBACK_SAFEWINDOWS_H
#define RAZORBACK_SAFEWINDOWS_H

#if !defined __CYGWIN__ && !defined _MSC_VER
# error Including win32/safewindows.h, but neither __CYGWIN__ nor _MSC_VER defined!
#endif

// Prevent windows.h from defining min and max macros.
#ifndef NOMINMAX
# define NOMINMAX
#endif

// Prevent windows.h from including lots of obscure win32 api headers
// which we don't care about and will just slow down compilation and
// increase the risk of symbol collisions.
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <Windows.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#endif // RAZORBACK_SAFEWINDOWS_H