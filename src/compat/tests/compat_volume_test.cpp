/* Silent integration test: production VC6 bridge versus current SDK interfaces
 * and the real waveOut process session. Never changes endpoint master volume. */
#define TITBIT_COMPAT_IMPLEMENTATION
#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <stdio.h>
#include "../compat.h"

int main() {
 HRESULT hr = CoInitialize(0);
 if (FAILED(hr)) return 1;
 IMMDeviceEnumerator *devices = 0;
 IMMDevice *device = 0;
 IAudioSessionManager2 *sessions = 0;
 ISimpleAudioVolume *volume = 0;
 IAudioEndpointVolume *endpoint = 0;
 HWAVEOUT wave = 0;
 WAVEFORMATEX format = {WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0};
 if (waveOutOpen(&wave, WAVE_MAPPER, &format, 0, 0, 0)) return 2;
 if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_INPROC_SERVER,
                            __uuidof(IMMDeviceEnumerator), (void **)&devices))) return 3;
 if (FAILED(devices->GetDefaultAudioEndpoint(eRender, eConsole, &device))) return 4;
 if (FAILED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, 0, (void **)&sessions))) return 5;
 if (FAILED(sessions->GetSimpleAudioVolume(0, 0, &volume))) return 6;
 if (FAILED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, 0, (void **)&endpoint))) return 7;
 float initial = 1, endpoint_before = 1, endpoint_after = 1;
 if (FAILED(volume->GetMasterVolume(&initial)) || FAILED(endpoint->GetMasterVolumeLevelScalar(&endpoint_before))) return 8;
 int failures = 0;
 int input[] = {18, 82, -5, 150};
 int wanted[] = {18, 82, 0, 100};
 for (int i = 0; i < 4; ++i) {
  float actual = -1;
  int ok = CompatSetMasterVolume(input[i]);
  hr = volume->GetMasterVolume(&actual);
  int percent = (int)(actual * 100 + .5f);
  int readback = CompatGetMasterVolume();
  printf("set %d -> SDK session %.3f, bridge %d, status %d\n", input[i], actual, readback, ok);
  failures += !ok || FAILED(hr) || percent != wanted[i] || readback != wanted[i];
 }
 // Independently enumerate the waveOut-created session and verify its owner
 // and volume. This catches a bridge controlling an empty/unrelated session.
 IAudioSessionEnumerator *list = 0;
 int count = 0, found = 0;
 if (SUCCEEDED(sessions->GetSessionEnumerator(&list))) {
  list->GetCount(&count);
  for (int j = 0; j < count; ++j) {
   IAudioSessionControl *control = 0;
   IAudioSessionControl2 *identity = 0;
   if (SUCCEEDED(list->GetSession(j, &control))) {
    if (SUCCEEDED(control->QueryInterface(__uuidof(IAudioSessionControl2), (void **)&identity))) {
     DWORD pid = 0;
     if (SUCCEEDED(identity->GetProcessId(&pid)) && pid == GetCurrentProcessId()) {
      ISimpleAudioVolume *actual_volume = 0;
      if (SUCCEEDED(control->QueryInterface(__uuidof(ISimpleAudioVolume), (void **)&actual_volume))) {
       float level = -1;
       hr = actual_volume->GetMasterVolume(&level);
       printf("waveOut session pid %lu, volume %.3f\n", pid, level);
       found += SUCCEEDED(hr) && level > .999f;
       actual_volume->Release();
      }
     }
     identity->Release();
    }
    control->Release();
   }
  }
  list->Release();
 }
 failures += found != 1;
 endpoint->GetMasterVolumeLevelScalar(&endpoint_after);
 failures += endpoint_before != endpoint_after;
 printf("endpoint master unchanged: %.6f -> %.6f\n", endpoint_before, endpoint_after);
 volume->SetMasterVolume(initial, 0);
 endpoint->Release(); volume->Release(); sessions->Release(); device->Release(); devices->Release();
 waveOutClose(wave);
 CoUninitialize();
 printf("failures: %d\n", failures);
 return failures ? 9 : 0;
}
