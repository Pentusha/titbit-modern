/* Compile-only guards for the SDK-facing records embedded in game objects.
 * Build with the same i386 Microsoft target and includes as the game. */
#include <windows.h>
#include <mmsystem.h>
#define DIRECTINPUT_VERSION 0x0500
#include <dinput.h>
#include <atlbase.h>
#include <stddef.h>
extern ATL::CComModule portable_win32_module;
#define _Module portable_win32_module
#include <atlwin.h>
#undef _Module
#define CHECK_ABI(name,condition) typedef char name[(condition)?1:-1]
CHECK_ABI(pointer_width,sizeof(void*)==4);
CHECK_ABI(guid_record,sizeof(GUID)==16);
CHECK_ABI(display_mode_record,sizeof(DEVMODEA)==148);
CHECK_ABI(display_width_offset,offsetof(DEVMODEA,dmPelsWidth)==108);
CHECK_ABI(display_frequency_offset,offsetof(DEVMODEA,dmDisplayFrequency)==120);
CHECK_ABI(wave_format_record,sizeof(WAVEFORMATEX)==18);
CHECK_ABI(wave_header_record,sizeof(WAVEHDR)==32);
CHECK_ABI(mixer_line_record,sizeof(MIXERLINEA)==168);
CHECK_ABI(mixer_control_record,sizeof(MIXERCONTROLA)==148);
CHECK_ABI(mixer_details_record,sizeof(MIXERCONTROLDETAILS)==24);
CHECK_ABI(mouse_state_record,sizeof(DIMOUSESTATE)==16);
CHECK_ABI(input_object_record,sizeof(DIOBJECTDATAFORMAT)==16);
CHECK_ABI(input_format_record,sizeof(DIDATAFORMAT)==24);
CHECK_ABI(module_record,sizeof(ATL::CComModule)==132);
CHECK_ABI(module_resource_offset,offsetof(ATL::_ATL_MODULE,m_hInstResource)==8);
CHECK_ABI(simple_array_record,sizeof(ATL::CSimpleArray<HWND>)==12);
typedef ATL::CSimpleMap<DWORD,void*> MessageMap;
CHECK_ABI(simple_map_record,sizeof(MessageMap)==12);
CHECK_ABI(dialog_record,sizeof(ATL::CDialogImplBaseT<ATL::CWindow>)==28);
