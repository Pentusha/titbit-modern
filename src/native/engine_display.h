#ifndef ENGINE_DISPLAY_057_H
#define ENGINE_DISPLAY_057_H
#ifndef WINVER
#define WINVER 0x0400
#endif
#include <windows.h>
// Observed views only. No storage is defined for this148-byte mode snapshot
// or the independently bound mode selector; larger ownership is unproved.
extern DEVMODEA savedDisplayMode057;
extern DWORD setupDisplayModeIndex054;
extern int displayWidth,displayHeight;
extern "C" bool __cdecl InitGameDisplay();
#endif
