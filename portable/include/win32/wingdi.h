#ifndef TITBIT_PORTABLE_WINGDI_H
#define TITBIT_PORTABLE_WINGDI_H
/* mingw-w64 exposes the newest DEVMODEA (156 bytes). The recovered executable
 * owns the older documented 148-byte record, ending at dmReserved2. Keep that
 * data ABI while obtaining all other GDI declarations from the open SDK.
 * Win32 display functions accept either record through its dmSize field. */
#define _devicemodeA _titbit_system_devicemodeA
#define DEVMODEA TITBIT_SYSTEM_DEVMODEA
#define PDEVMODEA TITBIT_SYSTEM_PDEVMODEA
#define NPDEVMODEA TITBIT_SYSTEM_NPDEVMODEA
#define LPDEVMODEA TITBIT_SYSTEM_LPDEVMODEA
#define DEVMODEW TITBIT_SYSTEM_DEVMODEW
#define PDEVMODEW TITBIT_SYSTEM_PDEVMODEW
#define NPDEVMODEW TITBIT_SYSTEM_NPDEVMODEW
#define LPDEVMODEW TITBIT_SYSTEM_LPDEVMODEW
#define DEVMODE TITBIT_SYSTEM_DEVMODE
#define PDEVMODE TITBIT_SYSTEM_PDEVMODE
#define NPDEVMODE TITBIT_SYSTEM_NPDEVMODE
#define LPDEVMODE TITBIT_SYSTEM_LPDEVMODE
#include_next <wingdi.h>
#undef _devicemodeA
#undef DEVMODEA
#undef PDEVMODEA
#undef NPDEVMODEA
#undef LPDEVMODEA
#undef DEVMODEW
#undef PDEVMODEW
#undef NPDEVMODEW
#undef LPDEVMODEW
#undef DEVMODE
#undef PDEVMODE
#undef NPDEVMODE
#undef LPDEVMODE
typedef TITBIT_SYSTEM_DEVMODEW DEVMODEW;
typedef TITBIT_SYSTEM_PDEVMODEW PDEVMODEW;
typedef TITBIT_SYSTEM_NPDEVMODEW NPDEVMODEW;
typedef TITBIT_SYSTEM_LPDEVMODEW LPDEVMODEW;
typedef struct _devicemodeA {
    BYTE dmDeviceName[32];
    WORD dmSpecVersion,dmDriverVersion,dmSize,dmDriverExtra;
    DWORD dmFields;
    union {
        struct { short dmOrientation,dmPaperSize,dmPaperLength,dmPaperWidth;
                 short dmScale,dmCopies,dmDefaultSource,dmPrintQuality; };
        struct { POINTL dmPosition;DWORD dmDisplayOrientation,dmDisplayFixedOutput; };
    };
    short dmColor,dmDuplex,dmYResolution,dmTTOption,dmCollate;
    BYTE dmFormName[32];
    WORD dmLogPixels;
    DWORD dmBitsPerPel,dmPelsWidth,dmPelsHeight;
    union { DWORD dmDisplayFlags,dmNup; };
    DWORD dmDisplayFrequency,dmICMMethod,dmICMIntent,dmMediaType,dmDitherType;
    DWORD dmReserved1,dmReserved2;
} DEVMODEA,*PDEVMODEA,*NPDEVMODEA,*LPDEVMODEA;
#ifndef UNICODE
typedef DEVMODEA DEVMODE;
typedef PDEVMODEA PDEVMODE;
typedef NPDEVMODEA NPDEVMODE;
typedef LPDEVMODEA LPDEVMODE;
#else
typedef DEVMODEW DEVMODE;
typedef PDEVMODEW PDEVMODE;
typedef NPDEVMODEW NPDEVMODE;
typedef LPDEVMODEW LPDEVMODE;
#endif
typedef char devmode_size_is_148[(sizeof(DEVMODEA)==148)?1:-1];
#endif
