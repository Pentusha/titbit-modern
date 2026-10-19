/* Replacement for native/game_input.cpp in the Linux build. Exports retain
 * engine globals and entry point names; SDL supplies the real device state. */
#include "titbit_platform.h"
void *gameInput=0,*gameMouse=0,*gameKeyboard=0;
bool inputReady=false;
unsigned char rawKeys[256]={0};
long mouseDeltaY=0,mouseDeltaX=0;
bool mouseLeft=false,mouseRight=false;
extern "C" int InitGameInput(void *,void *) { inputReady=tb_init()!=0;return inputReady; }
extern "C" void ReleaseGameInput() { inputReady=false; }
extern "C" void AcquireGameInput() { inputReady=true; }
extern "C" void PollGameInput() {
 if(!inputReady)return;TBInput state;tb_input_read(&state,1);
 for(unsigned n=0;n<256;n++)rawKeys[n]=state.keys[n];
 mouseDeltaX=state.dx;mouseDeltaY=state.dy;mouseLeft=state.buttons[0]!=0;mouseRight=state.buttons[1]!=0;
}
