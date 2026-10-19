/* Entry for the real source-only LINK build. Replay hooks use Win32 only and
 * must be installed before the actual CRT/application initializer graph runs.
 * Ordinary compatibility execution leaves replay disabled.
 */
#if !defined(TITBIT_SOURCE_LINK) || !defined(TITBIT_COMPAT)
#error This entry belongs only to the source-linked compatibility build.
#endif
#define TITBIT_COMPAT_IMPLEMENTATION
#include "compat.h"

extern "C" void __cdecl WinMainCRTStartup(void);
extern "C" void __cdecl SourceCompatStartup(void)
{
    CompatPlatformStartup();
    CompatStartup();
    WinMainCRTStartup();
}
