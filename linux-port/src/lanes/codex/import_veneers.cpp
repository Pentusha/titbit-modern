/* Authored x86 import veneers. Import identities/slots come from the original PE import directory; no vendor object bytes. */
// gluBuild2DMipmaps
extern "C" void *g_004cd074;
extern "C" __declspec(naked) void FUN_004a7dd6() { __asm jmp dword ptr [g_004cd074] }
// AVIStreamGetFrameOpen
extern "C" void *g_004cd024;
extern "C" __declspec(naked) void FUN_004a7ddc() { __asm jmp dword ptr [g_004cd024] }
// AVIStreamSampleToTime
extern "C" void *g_004cd01c;
extern "C" __declspec(naked) void FUN_004a7de2() { __asm jmp dword ptr [g_004cd01c] }
// AVIStreamLength
extern "C" void *g_004cd020;
extern "C" __declspec(naked) void FUN_004a7de8() { __asm jmp dword ptr [g_004cd020] }
// AVIStreamInfoA
extern "C" void *g_004cd018;
extern "C" __declspec(naked) void FUN_004a7dee() { __asm jmp dword ptr [g_004cd018] }
// AVIStreamOpenFromFileA
extern "C" void *g_004cd014;
extern "C" __declspec(naked) void FUN_004a7df4() { __asm jmp dword ptr [g_004cd014] }
// AVIFileInit
extern "C" void *g_004cd034;
extern "C" __declspec(naked) void FUN_004a7dfa() { __asm jmp dword ptr [g_004cd034] }
// DrawDibDraw
extern "C" void *g_004cd210;
extern "C" __declspec(naked) void FUN_004a7e00() { __asm jmp dword ptr [g_004cd210] }
// AVIStreamGetFrame
extern "C" void *g_004cd030;
extern "C" __declspec(naked) void FUN_004a7e06() { __asm jmp dword ptr [g_004cd030] }
// AVIFileExit
extern "C" void *g_004cd038;
extern "C" __declspec(naked) void FUN_004a7e0c() { __asm jmp dword ptr [g_004cd038] }
// AVIStreamRelease
extern "C" void *g_004cd028;
extern "C" __declspec(naked) void FUN_004a7e12() { __asm jmp dword ptr [g_004cd028] }
// AVIStreamGetFrameClose
extern "C" void *g_004cd02c;
extern "C" __declspec(naked) void FUN_004a7e18() { __asm jmp dword ptr [g_004cd02c] }
// DrawDibClose
extern "C" void *g_004cd20c;
extern "C" __declspec(naked) void FUN_004a7e1e() { __asm jmp dword ptr [g_004cd20c] }
// DrawDibOpen
extern "C" void *g_004cd208;
extern "C" __declspec(naked) void FUN_004a7e24() { __asm jmp dword ptr [g_004cd208] }
// RtlUnwind: PE import slot 0x4cd1c8; forward its four stdcall arguments.
extern "C" void *g_004cd1c8;
extern "C" __declspec(naked) void __stdcall FUN_004c637c(void *, void *, void *, void *)
{ __asm jmp dword ptr [g_004cd1c8] }
