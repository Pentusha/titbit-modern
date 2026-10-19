#ifndef SETUP_055_DIALOG_H
#define SETUP_055_DIALOG_H
// Exploratory actual ATL dialog ownership. Original game class names are unknown.
#include "app_module.h"
#define _ATL_NO_HOSTING
#define _Module gameModule
#include <atlwin.h>
#undef _Module
#include <stddef.h>

void *__cdecl operator new(unsigned);
void __cdecl operator delete(void *);

extern bool setupUseDS054; // Independently evidenced byte502478; no storage defined.
extern DWORD setupDisplayModeIndex054; // Independently evidenced DWORD502474.
class SetupDialogProbe;
// Authored ordinary source boundary around the unchanged SDK base construction.
// No fields or vtable slots are added by this class.
class ATL_NO_VTABLE SetupDialogBase053 : public ATL::CDialogImpl<SetupDialogProbe> {
public:
    SetupDialogBase053();
};

template <class T> struct SetupArray053 {
    // POD array operations reconstructed from native404b20; original template
    // name is unknown. No specialization, external storage, or byte payload.
    static void Construct(T *elements,int count) {
        if(elements && count) memset(elements,0,count*sizeof(T));
    }
    void SetSize(int newSize) {
        if(newSize<0) return;
        if(newSize==0) {
            if(data) {::operator delete(data);data=0;}
            size=capacity=0;
        } else if(data==0) {
            data=(T*)::operator new(newSize*sizeof(T));
            if(data) memset(data,0,newSize*sizeof(T));
            size=capacity=newSize;
        } else if(newSize<=capacity) {
            if(newSize>size) Construct(data+size,newSize-size);
            size=newSize;
        } else {
            int grow=grow_by;
            if(grow==0) {
                grow=size/8;
                if(grow<4) grow=4;
                else if(grow>1024) grow=1024;
            }
            int newCapacity;
            if(newSize<capacity+grow) newCapacity=capacity+grow;
            else newCapacity=newSize;
            T *newData=(T*)::operator new(newCapacity*sizeof(T));
            memcpy(newData,data,size*sizeof(T));
            if(newSize>size) Construct(newData+size,newSize-size);
            ::operator delete(data);
            data=newData;capacity=newCapacity;size=newSize;
        }
    }
    void SetAtGrow(int index,T value);
    int Add(T value) {int index=size;SetAtGrow(index,value);return index;}
    T& operator[](int index) {return data[index];}
    int GetSize()const {return size;}
    T *data;
    int size, capacity, grow_by;
    SetupArray053() { data=0; size=capacity=grow_by=0; }
    ~SetupArray053() { if(data) ::operator delete(data); }
};

class SetupDialogProbe : public SetupDialogBase053 {
public:
    enum {IDD=201};
    ATL::CWindow control_3ef,control_3f0,control_3f1,control_3f2;
    ATL::CWindow control_3f3,control_3f4,control_3ec,control_3ed;
    DEVMODEA display_mode;
    bool mode_available[6];
    int selected_resolution,selected_color_bits,selected_frequency;
    bool registry_option,option;
    SetupArray053<DEVMODEA> modes;
    SetupDialogProbe();
    ~SetupDialogProbe();

    LRESULT On1024(WORD,WORD,HWND,BOOL&);
    LRESULT On640(WORD,WORD,HWND,BOOL&);
    LRESULT On800(WORD,WORD,HWND,BOOL&);
    LRESULT OnRefresh(WORD,WORD,HWND,BOOL&);
    void UpdateModes(int resolution,int colorBits);
    LRESULT OnInit(UINT,WPARAM,LPARAM,BOOL&) {
        CenterWindow(GetParent());
        control_3ef=GetDlgItem(0x3ef); control_3f0=GetDlgItem(0x3f0);
        control_3f1=GetDlgItem(0x3f1); control_3f2=GetDlgItem(0x3f2);
        control_3f3=GetDlgItem(0x3f3); control_3f4=GetDlgItem(0x3f4);
        control_3ec=GetDlgItem(0x3ec); control_3ed=GetDlgItem(0x3ed);
        memset(&display_mode,0,sizeof(display_mode));
        memset(&display_mode,0,sizeof(display_mode));
        int modeIndex=0;
        display_mode.dmSize=sizeof(display_mode);
        display_mode.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL|DM_DISPLAYFREQUENCY;
        EnumDisplaySettingsA(0,ENUM_CURRENT_SETTINGS,&display_mode);
        mode_available[0]=false;mode_available[1]=false;mode_available[2]=false;
        mode_available[3]=false;mode_available[4]=false;mode_available[5]=false;
        while(EnumDisplaySettingsA(0,modeIndex,&display_mode)) {
            // Retain the original unusual OR condition. For unsigned bit depths
            // this allows every value at width1024; it is not corrected here.
            if(display_mode.dmPelsWidth==640 || display_mode.dmPelsWidth==800 ||
               display_mode.dmPelsWidth==1024 && (display_mode.dmBitsPerPel>=16 || display_mode.dmBitsPerPel<=32)) {
                display_mode.dmReserved1=modeIndex;
                modes.Add(display_mode);
                if(display_mode.dmPelsWidth==640 && display_mode.dmPelsHeight==480) mode_available[2]=true;
                if(display_mode.dmPelsWidth==800 && display_mode.dmPelsHeight==600) mode_available[3]=true;
                if(display_mode.dmPelsWidth==1024 && display_mode.dmPelsHeight==768) mode_available[4]=true;
            }
            ++modeIndex;
        }
        ::EnableWindow(GetDlgItem(0x3f0),mode_available[2]);
        ::EnableWindow(GetDlgItem(0x3f1),mode_available[3]);
        ::EnableWindow(GetDlgItem(0x3f2),mode_available[4]);
        selected_resolution=2;selected_color_bits=32;selected_frequency=0;
        HKEY key;DWORD type,size;
        RegCreateKeyA(HKEY_CURRENT_USER,"Software\\VZlab\\Titbit\\",&key);
        type=REG_DWORD;size=4;
        RegQueryValueExA(key,"resolution",0,&type,(BYTE*)&selected_resolution,&size);
        RegQueryValueExA(key,"colordepth",0,&type,(BYTE*)&selected_color_bits,&size);
        RegQueryValueExA(key,"refreshrate",0,&type,(BYTE*)&selected_frequency,&size);
        type=REG_BINARY;size=1;
        RegQueryValueExA(key,"suppress",0,&type,(BYTE*)&registry_option,&size);
        RegQueryValueExA(key,"useds",0,&type,(BYTE*)&setupUseDS054,&size);
        RegCloseKey(key);
        int width=0,height=0;
        switch(selected_resolution) {
        case 0:width=640;height=480;break;
        case 1:width=800;height=600;break;
        case 2:width=1024;height=768;break;
        }
        for(int i=0;i<modes.GetSize();i++) {
            if(modes[i].dmBitsPerPel==selected_color_bits && modes[i].dmPelsWidth==width &&
               modes[i].dmPelsHeight==height && modes[i].dmDisplayFrequency==selected_frequency) {
                setupDisplayModeIndex054=modes[i].dmReserved1;
                if(registry_option && !option) EndDialog(1);
            }
        }
        if(selected_resolution==0) ::SendMessageA(control_3f0,BM_SETCHECK,1,0);
        if(selected_resolution==1) ::SendMessageA(control_3f1,BM_SETCHECK,1,0);
        if(selected_resolution==2) ::SendMessageA(control_3f2,BM_SETCHECK,1,0);
        ::SendMessageA(control_3ec,BM_SETCHECK,registry_option,0);
        ::SendMessageA(control_3ed,BM_SETCHECK,setupUseDS054,0);
        UpdateModes(selected_resolution,selected_color_bits);
        return 1;
    }
    LRESULT OnOK(WORD,WORD,HWND,BOOL&) {
        HKEY key;
        RegCreateKeyA(HKEY_CURRENT_USER,"Software\\VZlab\\Titbit\\",&key);
        RegSetValueExA(key,"suppress",0,REG_BINARY,(BYTE*)&registry_option,1);
        RegSetValueExA(key,"useds",0,REG_BINARY,(BYTE*)&setupUseDS054,1);
        RegSetValueExA(key,"resolution",0,REG_DWORD,(BYTE*)&selected_resolution,4);
        RegSetValueExA(key,"colordepth",0,REG_DWORD,(BYTE*)&selected_color_bits,4);
        RegSetValueExA(key,"refreshrate",0,REG_DWORD,(BYTE*)&selected_frequency,4);
        RegCloseKey(key);
        EndDialog(1);return 0;
    }
    LRESULT OnCancel(WORD,WORD,HWND,BOOL&) {EndDialog(2);return 0;}
    LRESULT On16(WORD,WORD,HWND,BOOL&) {selected_color_bits=16;UpdateModes(selected_resolution,16);return 0;}
    LRESULT On32(WORD,WORD,HWND,BOOL&) {selected_color_bits=32;UpdateModes(selected_resolution,32);return 0;}
    LRESULT OnSuppress(WORD,WORD,HWND,BOOL&) {registry_option=!registry_option;return 0;}
    LRESULT OnUseDS(WORD,WORD,HWND,BOOL&) {setupUseDS054=!setupUseDS054;return 0;}
    BEGIN_MSG_MAP(SetupDialogProbe)
        MESSAGE_HANDLER(WM_INITDIALOG,OnInit)
        COMMAND_ID_HANDLER(IDOK,OnOK)
        COMMAND_ID_HANDLER(IDCANCEL,OnCancel)
        COMMAND_HANDLER(0x3f3,BN_CLICKED,On16)
        COMMAND_HANDLER(0x3f4,BN_CLICKED,On32)
        COMMAND_HANDLER(0x3f2,BN_CLICKED,On1024)
        COMMAND_HANDLER(0x3f0,BN_CLICKED,On640)
        COMMAND_HANDLER(0x3f1,BN_CLICKED,On800)
        COMMAND_HANDLER(0x3ef,CBN_SELCHANGE,OnRefresh)
        COMMAND_HANDLER(0x3ec,BN_CLICKED,OnSuppress)
        COMMAND_HANDLER(0x3ed,BN_CLICKED,OnUseDS)
    END_MSG_MAP()

};

// Metadata-only assertions; these declarations do not create setup storage.
typedef char BaseSize053[sizeof(SetupDialogBase053)==0x1c?1:-1];
typedef char LayoutEnd053[sizeof(SetupDialogProbe)==0xf8?1:-1];
typedef char ModeAt053[offsetof(SetupDialogProbe,display_mode)==0x3c?1:-1];
typedef char OptionAt053[offsetof(SetupDialogProbe,option)==0xe5?1:-1];
typedef char ArrayAt053[offsetof(SetupDialogProbe,modes)==0xe8?1:-1];


template<class T> void SetupArray053<T>::SetAtGrow(int index,T value) {
    if(index>=size) SetSize(index+1);
    data[index]=value;
}
#endif
