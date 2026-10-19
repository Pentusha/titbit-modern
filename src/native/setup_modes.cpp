#include "setup_dialog.h"
#define LoadStringA SetupUnusedLoadStringA055
#include "engine_string.h"
#undef LoadStringA
// The actual native cdecl variadic provider402630. No implementation modeled.
void __cdecl FormatArchiveString(ObservedString*,const char*,...);
void SetupDialogProbe::UpdateModes(int resolution,int colorBits) {
    bool found=false;
    WPARAM selected=(WPARAM)-1;
    DWORD bestFrequency=0;
    ::SendMessageA(control_3ef,CB_RESETCONTENT,0,0);
    mode_available[0]=false;mode_available[1]=false;
    DWORD width,height;
    switch(resolution) {
    case 0:width=640;height=480;break;
    case 1:width=800;height=600;break;
    case 2:width=1024;height=768;break;
    }
    for(int i=0;i<modes.GetSize();i++) {
        const DEVMODEA *current=&modes[i];
        if(current->dmPelsWidth==width && current->dmPelsHeight==height) {
            if(current->dmBitsPerPel==16) mode_available[0]=true;
            if(current->dmBitsPerPel==32) mode_available[1]=true;
            if(current->dmBitsPerPel==colorBits) {
                found=true;
                ObservedString text((const char*)0);
                FormatArchiveString(&text,"%d",current->dmDisplayFrequency);
                WPARAM entry=::SendMessageA(control_3ef,CB_ADDSTRING,0,(LPARAM)(const char*)text);
                ::SendMessageA(control_3ef,CB_SETITEMDATA,entry,modes[i].dmReserved1);
                if(bestFrequency<=modes[i].dmDisplayFrequency) {
                    bestFrequency=modes[i].dmDisplayFrequency;
                    setupDisplayModeIndex054=modes[i].dmReserved1;
                    selected=entry;
                    selected_frequency=bestFrequency;
                }
            }
        }
    }
    if(found) {
        ::EnableWindow(GetDlgItem(0x3f3),mode_available[0]);
        ::EnableWindow(GetDlgItem(0x3f4),mode_available[1]);
        if(colorBits==32) {
            ::SendMessageA(control_3f4,BM_SETCHECK,1,0);
            ::SendMessageA(control_3f3,BM_SETCHECK,0,0);
        } else if(colorBits==16) {
            ::SendMessageA(control_3f4,BM_SETCHECK,0,0);
            ::SendMessageA(control_3f3,BM_SETCHECK,1,0);
        }
        if(bestFrequency==0) {
            mode_available[5]=false;
            ::EnableWindow(GetDlgItem(0x3fa),mode_available[5]);
            ::EnableWindow(GetDlgItem(0x3f8),mode_available[5]);
            ::EnableWindow(GetDlgItem(0x3f9),mode_available[5]);
            ::EnableWindow(GetDlgItem(0x3f9),mode_available[5]);
            ::EnableWindow(GetDlgItem(0x3ef),mode_available[5]);
        } else {
            ::SendMessageA(control_3ef,CB_SETCURSEL,selected,0);
        }
    } else {
        if(colorBits==32) UpdateModes(resolution,16);
        else if(colorBits==16) UpdateModes(resolution,32);
    }
}
