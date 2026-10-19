#include "setup_dialog.h"
LRESULT SetupDialogProbe::On1024(WORD,WORD,HWND,BOOL&) {selected_resolution=2;UpdateModes(2,selected_color_bits);return 0;}
LRESULT SetupDialogProbe::On640(WORD,WORD,HWND,BOOL&) {selected_resolution=0;UpdateModes(0,selected_color_bits);return 0;}
LRESULT SetupDialogProbe::On800(WORD,WORD,HWND,BOOL&) {selected_resolution=1;UpdateModes(1,selected_color_bits);return 0;}
LRESULT SetupDialogProbe::OnRefresh(WORD,WORD,HWND,BOOL&) {WPARAM selected=control_3ef.SendMessage(CB_GETCURSEL,0,0);setupDisplayModeIndex054=control_3ef.SendMessage(CB_GETITEMDATA,selected,0);return 0;}
