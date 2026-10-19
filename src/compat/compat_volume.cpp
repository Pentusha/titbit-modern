/* Modern-Windows volume bridge, confined to TITBIT_COMPAT.
 * The original script wrapper divides legacy mixer bounds; on systems without
 * that mixer both bounds are -1 and ON_CREATE aborts before creating the knob.
 * waveOut/DirectSound use the process-specific GUID_NULL session. Control that
 * session through Core Audio, leaving the endpoint/system master untouched.
 * SDK interface prefixes are declared here so the real source builds with VC6.
 * https://learn.microsoft.com/windows/win32/coreaudio/audio-sessions
 */
#define TITBIT_COMPAT_IMPLEMENTATION
#include <windows.h>
#include <objbase.h>
#include "compat.h"
#include "compat_internal.h"

struct CompatSimpleVolume : IUnknown {
 virtual HRESULT __stdcall SetMasterVolume(float level, const GUID *context) = 0;
 virtual HRESULT __stdcall GetMasterVolume(float *level) = 0;
 virtual HRESULT __stdcall SetMute(BOOL mute, const GUID *context) = 0;
 virtual HRESULT __stdcall GetMute(BOOL *mute) = 0;
};
struct CompatSessionManager : IUnknown {
 virtual HRESULT __stdcall GetAudioSessionControl(const GUID *, DWORD, IUnknown **) = 0;
 virtual HRESULT __stdcall GetSimpleAudioVolume(const GUID *, DWORD, CompatSimpleVolume **) = 0;
};
struct CompatAudioDevice : IUnknown {
 virtual HRESULT __stdcall Activate(REFIID iid, DWORD context, void *params, void **out) = 0;
};
struct CompatDeviceEnumerator : IUnknown {
 virtual HRESULT __stdcall EnumAudioEndpoints(int flow, DWORD state, IUnknown **out) = 0;
 virtual HRESULT __stdcall GetDefaultAudioEndpoint(int flow, int role, CompatAudioDevice **out) = 0;
};
static const GUID CLSID_CompatDevices = {0xbcde0395,0xe52f,0x467c,{0x8e,0x3d,0xc4,0x57,0x92,0x91,0x69,0x2e}};
static const GUID IID_CompatDevices = {0xa95664d2,0x9614,0x4f35,{0xa7,0x46,0xde,0x8d,0xb6,0x36,0x17,0xe6}};
static const GUID IID_CompatSessions = {0xbfa971f1,0x4d5e,0x40bb,{0x93,0x5e,0x96,0x70,0x39,0xbf,0xbe,0xe4}};
static int last_volume = 100;

/* Acquire/release on the calling thread, respecting a pre-existing COM
 * apartment. No cached COM pointer can outlive its apartment or endpoint. */
static HRESULT OpenVolume(CompatSimpleVolume **out) {
 CompatDeviceEnumerator *devices = 0;
 CompatAudioDevice *device = 0;
 CompatSessionManager *sessions = 0;
 *out = 0;
 HRESULT hr = CoCreateInstance(CLSID_CompatDevices, 0, CLSCTX_INPROC_SERVER,
                               IID_CompatDevices, (void **)&devices);
 if (SUCCEEDED(hr)) hr = devices->GetDefaultAudioEndpoint(0, 0, &device); // render, console
 if (SUCCEEDED(hr)) hr = device->Activate(IID_CompatSessions, CLSCTX_INPROC_SERVER, 0, (void **)&sessions);
 if (SUCCEEDED(hr)) hr = sessions->GetSimpleAudioVolume(0, 0, out); // default, this process only
 if (sessions) sessions->Release();
 if (device) device->Release();
 if (devices) devices->Release();
 return hr;
}

extern "C" int __cdecl CompatGetMasterVolume(void) {
 HRESULT init = CoInitialize(0);
 if (FAILED(init) && init != RPC_E_CHANGED_MODE) {
  CompatLog("volume", "COM initialization failed", init);
  return last_volume;
 }
 CompatSimpleVolume *volume = 0;
 HRESULT hr = OpenVolume(&volume);
 float level = 1.0f;
 if (SUCCEEDED(hr)) hr = volume->GetMasterVolume(&level);
 if (volume) volume->Release();
 if (SUCCEEDED(init)) CoUninitialize();
 if (SUCCEEDED(hr)) {
  int percent = (int)(level * 100.0f + 0.5f);
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  last_volume = percent;
 } else CompatLog("volume", "session read failed", hr);
 return last_volume; // keep settings usable even with no output device
}

extern "C" int __cdecl CompatSetMasterVolume(int percent) {
 if (percent < 0) percent = 0;
 if (percent > 100) percent = 100;
 last_volume = percent;
 HRESULT init = CoInitialize(0);
 if (FAILED(init) && init != RPC_E_CHANGED_MODE) {
  CompatLog("volume", "COM initialization failed", init);
  return 0;
 }
 CompatSimpleVolume *volume = 0;
 HRESULT hr = OpenVolume(&volume);
 if (SUCCEEDED(hr)) hr = volume->SetMasterVolume(percent * 0.01f, 0);
 if (volume) volume->Release();
 if (SUCCEEDED(init)) CoUninitialize();
 CompatLog("volume", SUCCEEDED(hr) ? "session percent" : "session write failed",
           SUCCEEDED(hr) ? percent : hr);
 return SUCCEEDED(hr);
}
