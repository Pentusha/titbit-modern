// Logical SDK GUID initializers; no vendor GUID .obj/.lib payloads.
// Independent sections preserve the original independently observed placement.
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <initguid.h>
#pragma const_seg(".rdata$DS0")
DEFINE_GUID(CLSID_DirectSound, 0x47d4d946, 0x62e8, 0x11cf, 0x93, 0xbc, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
#pragma const_seg(".rdata$DS1")
DEFINE_GUID(IID_IDirectSound, 0x279AFA83, 0x4981, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);
#pragma const_seg()
