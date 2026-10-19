#include "engine_display.h"
#include <string.h>
#pragma intrinsic(memset)
#ifdef TITBIT_COMPAT
#include "../compat/compat.h"   /* no forced display mode switch (src/compat/README.md) */
#endif
extern "C" bool __cdecl InitGameDisplay(){
 DEVMODEA mode;memset(&mode,0,sizeof(mode));mode.dmSize=sizeof(mode);
 EnumDisplaySettingsA(0,ENUM_CURRENT_SETTINGS,&mode);
 savedDisplayMode057=mode;
 memset(&mode,0,sizeof(mode));mode.dmSize=sizeof(mode);
 mode.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT|DM_DISPLAYFREQUENCY;
 EnumDisplaySettingsA(0,setupDisplayModeIndex054,&mode);
 displayHeight=mode.dmPelsHeight;displayWidth=mode.dmPelsWidth;
 DWORD bits=mode.dmBitsPerPel,frequency=mode.dmDisplayFrequency;
 memset(&mode,0,sizeof(mode));mode.dmSize=sizeof(mode);
 mode.dmBitsPerPel=bits;mode.dmPelsWidth=displayWidth;mode.dmPelsHeight=displayHeight;
 mode.dmDisplayFrequency=frequency;
 mode.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
 if(frequency)mode.dmFields|=DM_DISPLAYFREQUENCY;
 ChangeDisplaySettingsA(&mode,0);
 return true;
}
