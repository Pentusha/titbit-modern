// Authored source names. Native allocation proves sizeof52; virtual slots and fields are evidenced.
#ifndef TITBIT_WINMM_VOLUME_H
#define TITBIT_WINMM_VOLUME_H
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
class MasterOutputVolume {
public:
    bool initialized, line_ready, available;
    UINT mixer_id;
    HMIXER mixer;
    DWORD line_id, control_id, channels;
    HWND window;
    DWORD minimum, maximum, steps;
    void (__stdcall *callback)(DWORD, DWORD);
    DWORD user;
    MasterOutputVolume();
    bool Initialize();
    bool Query();
    void Close();
    void ControlChanged(DWORD id);
    void SetEnabled(bool enabled);
    virtual bool IsAvailable();
    virtual void Enable();
    virtual void Disable();
    virtual DWORD GetSteps();
    virtual DWORD GetMinimum();
    virtual DWORD GetMaximum();
    virtual DWORD GetVolume();
    virtual void SetVolume(DWORD value);
    virtual void SetCallback(void (__stdcall *fn)(DWORD, DWORD), DWORD argument);
};
#endif
