extern "C" __declspec(dllimport) long __stdcall ChangeDisplaySettingsA(void *,unsigned);
#ifdef TITBIT_COMPAT
#include "../compat/compat.h"   /* no forced display mode switch (src/compat/README.md) */
#endif
extern "C" void __cdecl RestoreGameDisplay(){ChangeDisplaySettingsA(0,0);}
