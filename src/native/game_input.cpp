#define DIRECTINPUT_VERSION 0x0500
#include <windows.h>
#include <dinput.h>
LPDIRECTINPUTA gameInput=0;LPDIRECTINPUTDEVICEA gameMouse=0,gameKeyboard=0;bool inputReady=false;
unsigned char rawKeys[256]={0};long mouseDeltaY=0,mouseDeltaX=0;bool mouseLeft=false,mouseRight=false;
extern "C" int __cdecl InitGameInput(HINSTANCE instance,HWND window){
 if(DirectInputCreateA(instance,0x500,&gameInput,0)<0)return 0;
 gameInput->CreateDevice(GUID_SysMouse,&gameMouse,0);gameInput->CreateDevice(GUID_SysKeyboard,&gameKeyboard,0);
 gameMouse->SetCooperativeLevel(window,6);gameKeyboard->SetCooperativeLevel(window,6);
 gameMouse->SetDataFormat(&c_dfDIMouse);gameKeyboard->SetDataFormat(&c_dfDIKeyboard);
 if(gameMouse->Acquire()<0)return 0;if(gameKeyboard->Acquire()<0)return 0;inputReady=true;return 1;
}
extern "C" void __cdecl ReleaseGameInput(){inputReady=false;if(gameInput){if(gameMouse){gameMouse->Release();gameMouse=0;}if(gameKeyboard){gameKeyboard->Release();gameKeyboard=0;}gameInput->Release();gameInput=0;}}
extern "C" void __cdecl AcquireGameInput(){if(inputReady){gameMouse->Acquire();gameKeyboard->Acquire();}}
extern "C" void __cdecl PollGameInput(){
 DIMOUSESTATE state;
 if(inputReady){
  if(gameKeyboard->GetDeviceState(256,rawKeys)==DIERR_INPUTLOST)AcquireGameInput();
  if(gameMouse->GetDeviceState(sizeof(state),&state)==DIERR_INPUTLOST)AcquireGameInput();
  mouseDeltaX=state.lX;mouseDeltaY=state.lY;mouseRight=state.rgbButtons[1]!=0;mouseLeft=state.rgbButtons[0]!=0;
 }
}
