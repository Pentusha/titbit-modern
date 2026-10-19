// Source restoration from native behavior. Initialize and SetEnabled remain codegen candidates.
#include "winmm_volume.h"


MasterOutputVolume* current_volume_probe;
LRESULT CALLBACK VolumeProbeWindow(HWND hwnd, UINT message, WPARAM w, LPARAM l);

MasterOutputVolume::MasterOutputVolume()
    : initialized(false), line_ready(false), available(false), mixer_id(0),
      mixer(0), window(0), minimum(-1), maximum(-1), callback(0), user(0)
{
    initialized = Initialize();
    if (initialized) {
        current_volume_probe = this;
        if (!Query()) {
            Close();
            current_volume_probe = 0;
        }
    }
}

bool MasterOutputVolume::Initialize()
{
    if (!mixerGetNumDevs()) return false;
    WAVEFORMATEX format; memset(&format, 0, sizeof(PCMWAVEFORMAT));
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nBlockAlign = 2;
    format.cbSize = 0;
    format.nSamplesPerSec = 22050;
    format.wBitsPerSample = 16;
    format.nAvgBytesPerSec = 44100;
    HWAVEOUT wave;
    if (waveOutOpen(&wave, WAVE_MAPPER, &format, 0, 0, 0) != MMSYSERR_NOERROR) return false;
    MMRESULT result = mixerGetID((HMIXEROBJ)wave, &mixer_id, MIXER_OBJECTF_HWAVEOUT);
    waveOutClose(wave);
    if (result != MMSYSERR_NOERROR) return false;
    WNDCLASSEX wc; memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = "Master Output Volume Msg Wnd Class";
    wc.lpfnWndProc = VolumeProbeWindow;
    RegisterClassEx(&wc);
    window = CreateWindowEx(0, "Master Output Volume Msg Wnd Class", "Master Output Volume Msg Wnd",
                            WS_POPUP | WS_DISABLED, 0, 0, 0, 0, 0, 0, 0, 0);
    if (!window) return false;
    ShowWindow(window, SW_HIDE);
    if (mixerOpen(&mixer, mixer_id, (DWORD)window, 0, CALLBACK_WINDOW) != MMSYSERR_NOERROR) {
        DestroyWindow(window);
        return false;
    }
    return true;
}

void MasterOutputVolume::Close()
{
    mixerClose(mixer);
    DestroyWindow(window);
    line_ready = false;
    initialized = false;
}

void MasterOutputVolume::ControlChanged(DWORD id)
{
    if (control_id == id) {
        DWORD value = GetVolume();
        if (value != (DWORD)-1 && callback) callback(value, user);
    }
}

bool MasterOutputVolume::Query()
{
    if (!initialized) return false;
    MIXERLINE line; memset(&line, 0, sizeof(line));
    line.cbStruct = sizeof(line);
    line.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
    if (mixerGetLineInfo((HMIXEROBJ)mixer, &line, MIXER_GETLINEINFOF_COMPONENTTYPE)) return false;
    MIXERCONTROL control; memset(&control, 0, sizeof(control));
    control.cbStruct = sizeof(control);
    MIXERLINECONTROLS controls; memset(&controls, 0, sizeof(controls));
    controls.cbStruct = sizeof(controls);
    controls.dwLineID = line.dwLineID;
    controls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    controls.cControls = 1;
    controls.cbmxctrl = sizeof(control);
    controls.pamxctrl = &control;
    if (!mixerGetLineControls((HMIXEROBJ)mixer, &controls, MIXER_GETLINECONTROLSF_ONEBYTYPE)
        && !(control.fdwControl & MIXERCONTROL_CONTROLF_DISABLED)) available = true;
    channels = line.cChannels;
    line_id = controls.dwLineID;
    control_id = control.dwControlID;
    minimum = control.Bounds.dwMinimum;
    maximum = control.Bounds.dwMaximum;
    steps = control.Metrics.cSteps;
    line_ready = true;
    return true;
}

bool MasterOutputVolume::IsAvailable() { return available; }
void MasterOutputVolume::Enable() { SetEnabled(true); }
void MasterOutputVolume::Disable() { SetEnabled(false); }
DWORD MasterOutputVolume::GetSteps() { if (!available) return (DWORD)-1; return steps; }
DWORD MasterOutputVolume::GetMinimum() { if (!available) return (DWORD)-1; return minimum; }
DWORD MasterOutputVolume::GetMaximum() { if (!available) return (DWORD)-1; return maximum; }

DWORD MasterOutputVolume::GetVolume()
{
    if (!available) return (DWORD)-1;
    DWORD* values = (DWORD*)malloc(channels * sizeof(DWORD));
    if (!values) return (DWORD)-1;
    MIXERCONTROLDETAILS details; memset(&details, 0, sizeof(details));
    details.dwControlID = control_id;
    details.cbStruct = sizeof(details);
    details.cChannels = channels;
    details.cMultipleItems = 0;
    details.cbDetails = sizeof(DWORD);
    details.paDetails = values;
    MMRESULT result = mixerGetControlDetails((HMIXEROBJ)mixer, &details, MIXER_GETCONTROLDETAILSF_VALUE);
    DWORD value = values[0];
    free(values);
    if (result != MMSYSERR_NOERROR) return (DWORD)-1;
    return value;
}

void MasterOutputVolume::SetVolume(DWORD value)
{
    if (!available || value < minimum || value > maximum) return;
    DWORD* values = (DWORD*)malloc(channels * sizeof(DWORD));
    if (!values) return;
    for (int i = 0; i < (int)channels; ++i) values[i] = value;
    MIXERCONTROLDETAILS details; memset(&details, 0, sizeof(details));
    details.dwControlID = control_id;
    details.cbStruct = sizeof(details);
    details.cChannels = channels;
    details.cMultipleItems = 0;
    details.cbDetails = sizeof(DWORD);
    details.paDetails = values;
    mixerSetControlDetails((HMIXEROBJ)mixer, &details, MIXER_SETCONTROLDETAILSF_VALUE);
    free(values);
}

void MasterOutputVolume::SetCallback(void (__stdcall *fn)(DWORD, DWORD), DWORD argument)
{
    callback = fn;
    user = argument;
}

LRESULT CALLBACK VolumeProbeWindow(HWND hwnd, UINT message, WPARAM w, LPARAM l)
{
    if (message == MM_MIXM_CONTROL_CHANGE && current_volume_probe)
        current_volume_probe->ControlChanged((DWORD)l);
    return DefWindowProc(hwnd, message, w, l);
}

void MasterOutputVolume::SetEnabled(bool enabled)
{
    if (!line_ready) return;
    MIXERLINE line; memset(&line, 0, sizeof(line));
    line.cbStruct=sizeof(line); line.dwLineID=line_id;
    if (mixerGetLineInfo((HMIXEROBJ)mixer,&line,MIXER_GETLINEINFOF_LINEID)) return;
    int count = line.cControls;
    int channel_count = line.cChannels;
    MIXERLINECONTROLS controls; memset(&controls,0,sizeof(controls));
    MIXERCONTROL* items=(MIXERCONTROL*)malloc(count*sizeof(MIXERCONTROL));
    if (!items) return;
    memset(items,0,sizeof(items));
    for (int i=0; i<count; ++i) items[i].cbStruct=sizeof(MIXERCONTROL);
    controls.cbStruct=sizeof(controls); controls.dwLineID=line.dwLineID;
    controls.cControls=count; controls.cbmxctrl=sizeof(MIXERCONTROL); controls.pamxctrl=items;
    if (!mixerGetLineControls((HMIXEROBJ)mixer,&controls,MIXER_GETLINECONTROLSF_ALL)) {
      for (int i=0; i<count; ++i) {
        BOOL value;
        switch (items[i].dwControlType) {
        case MIXERCONTROL_CONTROLTYPE_MUTE: value=!enabled; break;
        case 0x70010000: value=enabled; break;
        case 0x70010001: value=enabled; break;
        case 0x71010000: value=enabled; break;
        case 0x71010001: value=enabled; break;
        default: continue;
        }
        MIXERCONTROLDETAILS details; memset(&details,0,sizeof(details));
        details.cbStruct=sizeof(details); details.dwControlID=items[i].dwControlID;
        int n=channel_count;
        if(items[i].fdwControl&MIXERCONTROL_CONTROLF_UNIFORM) n=1;
        DWORD* values; DWORD multiple;
        if(items[i].fdwControl&MIXERCONTROL_CONTROLF_MULTIPLE) {
          multiple=items[i].cMultipleItems;
          values=(DWORD*)malloc(multiple*n*sizeof(DWORD));
          if(!values) continue;
          for(int k=0; k<(int)multiple; ++k) {
            for(int j=0; j<n; ++j) values[k+j]=value;
          }
        } else {
          multiple=0;
          values=(DWORD*)malloc(n*sizeof(DWORD));
          if(!values) continue;
          for(int j=0; j<n; ++j) values[j]=value;
        }
        details.cMultipleItems=multiple; details.cChannels=n;
        details.cbDetails=sizeof(DWORD); details.paDetails=values;
        mixerSetControlDetails((HMIXEROBJ)mixer,&details,MIXER_SETCONTROLDETAILSF_VALUE);
        free(values);
      }
    }
    free(items);
}
