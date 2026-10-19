/* Non-game Win32/OpenGL integration test. Run on HiddenDesktop only. */
#define WINVER 0x0500
#define TITBIT_COMPAT_IMPLEMENTATION
#include <windows.h>
#include <GL/gl.h>
#include "../compat.h"
#include "../display_geometry.h"
#pragma intrinsic(memset)
static int failures, mouse_x, mouse_y, clicks;
extern "C" int _fltused = 0; // MSVC floating-point marker for this no-CRT test.
#define CHECK(x) do { if (!(x)) { char s[96]; DWORD n; wsprintfA(s,"FAILED line %d\r\n",__LINE__); WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),s,lstrlenA(s),&n,0); ++failures; } } while(0)
static LRESULT CALLBACK Proc(HWND w, UINT m, WPARAM a, LPARAM b) {
 if (m == WM_MOUSEMOVE) { mouse_x = (short)LOWORD(b); mouse_y = (short)HIWORD(b); }
 if (m == WM_LBUTTONDOWN) ++clicks;
 return DefWindowProcA(w,m,a,b);
}
extern "C" void __cdecl WinMainCRTStartup(void) {
 CompatPlatformStartup();
 CompatViewport vp = CompatFitViewport(1920,1080,1024,768,false);
 CHECK(vp.x==240 && vp.width==1440 && vp.height==1080);
 vp=CompatFitViewport(3840,2160,1024,768,false);
 CHECK(vp.x==480 && vp.width==2880 && vp.height==2160);
 vp=CompatFitViewport(1280,1024,1024,768,false);
 CHECK(vp.y==32 && vp.top==32 && vp.height==960);
 vp=CompatFitViewport(1024,769,1024,768,false);
 CHECK(vp.y==0 && vp.top==1);
 int x,y; CHECK(!CompatMapMouse(vp,10,0,1024,768,&x,&y));
 CHECK(CompatMapMouse(vp,0,1,1024,768,&x,&y) && x==0 && y==0);
 vp=CompatFitViewport(1920,1080,1024,768,true);
 CHECK(vp.width==1920 && vp.height==1080);
 WNDCLASSA wc; ZeroMemory(&wc,sizeof(wc));wc.style=CS_OWNDC;wc.lpfnWndProc=Proc;
 wc.hInstance=GetModuleHandleA(0);wc.lpszClassName="OpenGL";RegisterClassA(&wc);
 DEVMODEA before,after;ZeroMemory(&before,sizeof(before));before.dmSize=sizeof(before);
 EnumDisplaySettingsA(0,ENUM_CURRENT_SETTINGS,&before);
 HWND w=(HWND)CompatCreateWindowExA(WS_EX_TOPMOST,"OpenGL","Modern display unit",WS_POPUP,
                                  0,0,1024,768,0,0,wc.hInstance,0);
 CHECK(w!=0);CHECK(!(GetWindowLongA(w,GWL_EXSTYLE)&WS_EX_TOPMOST));
 RECT r;GetClientRect(w,&r);CHECK(r.right==1600 && r.bottom==900);
 HDC dc=GetDC(w);PIXELFORMATDESCRIPTOR pf;ZeroMemory(&pf,sizeof(pf));
 pf.nSize=sizeof(pf);pf.nVersion=1;pf.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
 pf.iPixelType=PFD_TYPE_RGBA;pf.cColorBits=32;pf.cDepthBits=24;
 int format=ChoosePixelFormat(dc,&pf);CHECK(format!=0);CHECK(SetPixelFormat(dc,format,&pf));
 HGLRC gl=wglCreateContext(dc);CHECK(gl!=0);CHECK(wglMakeCurrent(dc,gl));
 glClearColor(1,0,0,1);CompatBeginFrame();glClear(GL_COLOR_BUFFER_BIT);glFinish();
 int view[4];glGetIntegerv(GL_VIEWPORT,view);
 CHECK(view[0]==200 && view[1]==0 && view[2]==1200 && view[3]==900);
 unsigned char pixel[4];glReadPixels(0,450,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
 CHECK(pixel[0]==0 && pixel[1]==0 && pixel[2]==0);
 glReadPixels(800,450,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);CHECK(pixel[0]==255);
 SendMessageA(w,WM_MOUSEMOVE,0,MAKELPARAM(800,450));CHECK(mouse_x==512 && mouse_y==384);
 SendMessageA(w,WM_LBUTTONDOWN,0,MAKELPARAM(0,450));CHECK(clicks==0);
 SendMessageA(w,WM_LBUTTONDOWN,0,MAKELPARAM(800,450));CHECK(clicks==1);
 SendMessageA(w,WM_SYSKEYDOWN,VK_RETURN,0x20000000);
 MONITORINFO mi;mi.cbSize=sizeof(mi);GetMonitorInfoA(MonitorFromWindow(w,MONITOR_DEFAULTTONEAREST),&mi);
 GetWindowRect(w,&r);CHECK(EqualRect(&r,&mi.rcMonitor));
 CHECK(!(GetWindowLongA(w,GWL_STYLE)&WS_CAPTION));CHECK(!(GetWindowLongA(w,GWL_EXSTYLE)&WS_EX_TOPMOST));
 CompatBeginFrame();CHECK(glGetError()==GL_NO_ERROR);
 SendMessageA(w,WM_SYSKEYDOWN,VK_RETURN,0x20000000);
 GetClientRect(w,&r);CHECK(r.right==1600 && r.bottom==900);
 SendMessageA(w,WM_SIZE,0,0);CompatBeginFrame();
 ZeroMemory(&after,sizeof(after));after.dmSize=sizeof(after);EnumDisplaySettingsA(0,ENUM_CURRENT_SETTINGS,&after);
 CHECK(before.dmPelsWidth==after.dmPelsWidth && before.dmPelsHeight==after.dmPelsHeight &&
       before.dmDisplayFrequency==after.dmDisplayFrequency);
 wglMakeCurrent(0,0);wglDeleteContext(gl);ReleaseDC(w,dc);DestroyWindow(w);
 char result[80];DWORD written;wsprintfA(result,"Modern display failures: %d\r\n",failures);
 WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),result,lstrlenA(result),&written,0);
 ExitProcess(failures);
}
