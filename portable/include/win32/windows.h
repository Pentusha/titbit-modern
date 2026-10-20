#ifndef TITBIT_PORTABLE_WINDOWS_H
#define TITBIT_PORTABLE_WINDOWS_H
#include_next <windows.h>
/* The recovered string views declare the Win32 functions with nonvolatile
 * LONG pointers. Keep those external stdcall imports instead of MinGW's
 * macros that rename them to Clang's differently typed compiler intrinsics. */
#undef InterlockedIncrement
#undef InterlockedDecrement
#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllimport) long __stdcall InterlockedIncrement(long *);
__declspec(dllimport) long __stdcall InterlockedDecrement(long *);
#ifdef __cplusplus
}
#endif
#endif
