extern "C" __declspec(dllimport) int __stdcall PostMessageA(void *,unsigned,unsigned,long);
extern void *gameWindow;
extern "C" void __cdecl CloseGameApp(){PostMessageA(gameWindow,16,0,0);}
