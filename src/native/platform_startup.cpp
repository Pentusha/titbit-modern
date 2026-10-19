/* Authored engine startup reconstruction. Addresses/names are analysis labels,
 * not claims about the original source names. Win32 calls retain their ABI.
 * Error messages and the volume label are recovered text, not code payloads. */
typedef void *HANDLE;typedef HANDLE HWND;typedef HANDLE HDC;typedef HANDLE HINSTANCE;
typedef unsigned long DWORD;typedef int BOOL;
struct Rect {long left,top,right,bottom;};
typedef long (__stdcall *WindowProc)(HWND,unsigned,unsigned,long);
struct WindowClass {unsigned style;WindowProc proc;int classExtra,windowExtra;HINSTANCE instance;HANDLE icon,cursor,background;const char *menu,*name;};
struct PixelFormat {unsigned short size,version;DWORD flags;unsigned char pixelType,colorBits,redBits,redShift,greenBits,greenShift,blueBits,blueShift,alphaBits,alphaShift,accumBits,accumRed,accumGreen,accumBlue,accumAlpha,depthBits,stencilBits,auxBuffers,layerType,reserved;DWORD layerMask,visibleMask,damageMask;};
void __cdecl PickingTrace(const char*,...);
extern "C" {
__declspec(dllimport) BOOL __stdcall GetVolumeInformationA(const char*,char*,DWORD,DWORD*,DWORD*,DWORD*,char*,DWORD);
__declspec(dllimport) unsigned __stdcall GetDriveTypeA(const char*);
__declspec(dllimport) int __stdcall MessageBoxA(HWND,const char*,const char*,unsigned);
__declspec(dllimport) int __stdcall ReleaseDC(HWND,HDC);
__declspec(dllimport) BOOL __stdcall DestroyWindow(HWND);
__declspec(dllimport) BOOL __stdcall UnregisterClassA(const char*,HINSTANCE);
__declspec(dllimport) BOOL __stdcall wglMakeCurrent(HDC,HANDLE);
__declspec(dllimport) BOOL __stdcall wglDeleteContext(HANDLE);
__declspec(dllimport) HANDLE __stdcall wglCreateContext(HDC);
__declspec(dllimport) void __stdcall PostQuitMessage(int);
__declspec(dllimport) int __stdcall ChoosePixelFormat(HDC,const PixelFormat*);
__declspec(dllimport) BOOL __stdcall SetPixelFormat(HDC,int,const PixelFormat*);
__declspec(dllimport) HINSTANCE __stdcall GetModuleHandleA(const char*);
__declspec(dllimport) HANDLE __stdcall LoadIconA(HINSTANCE,const char*);
__declspec(dllimport) HANDLE __stdcall LoadCursorA(HINSTANCE,const char*);
__declspec(dllimport) unsigned short __stdcall RegisterClassA(const WindowClass*);
__declspec(dllimport) int __stdcall ShowCursor(BOOL);
__declspec(dllimport) HWND __stdcall CreateWindowExA(DWORD,const char*,const char*,DWORD,int,int,int,int,HWND,HANDLE,HINSTANCE,void*);
__declspec(dllimport) BOOL __stdcall SetRect(Rect*,int,int,int,int);
__declspec(dllimport) HDC __stdcall GetDC(HWND);
__declspec(dllimport) BOOL __stdcall ShowWindow(HWND,int);
__declspec(dllimport) BOOL __stdcall SetForegroundWindow(HWND);
__declspec(dllimport) HWND __stdcall SetFocus(HWND);
__declspec(dllimport) void __stdcall glViewport(int,int,int,int);
__declspec(dllimport) void __stdcall glDisable(unsigned);
__declspec(dllimport) void __stdcall glClear(unsigned);
__declspec(dllimport) void __stdcall glEnable(unsigned);
__declspec(dllimport) void __stdcall glClearDepth(double);
__declspec(dllimport) void __stdcall glDepthFunc(unsigned);
__declspec(dllimport) void __stdcall glShadeModel(unsigned);
__declspec(dllimport) void __stdcall glMatrixMode(unsigned);
/* Native calls the import-library thunk, rather than an inline IAT call. */
void __stdcall gluPerspective(double,double,double,double);
__declspec(dllimport) void __stdcall glPointSize(float);
__declspec(dllimport) void __stdcall glColor4f(float,float,float,float);
__declspec(dllimport) void __stdcall glLightModelfv(unsigned,const float*);
__declspec(dllimport) void __stdcall glMaterialfv(unsigned,unsigned,const float*);
__declspec(dllimport) void __stdcall glMaterialf(unsigned,unsigned,float);
__declspec(dllimport) void __stdcall glTexGeni(unsigned,unsigned,int);
__declspec(dllimport) void __stdcall glCullFace(unsigned);
__declspec(dllimport) const char *__stdcall glGetString(unsigned);
__declspec(dllimport) void __stdcall glGetIntegerv(unsigned,int*);
__declspec(dllimport) void *__stdcall wglGetProcAddress(const char*);
char *__cdecl _strdup(const char*);
unsigned __cdecl strlen(const char*);
long __stdcall GameWindowProc(HWND,unsigned,unsigned,long);
void __cdecl AttachGameWindow(HWND,bool);
void __cdecl ShutdownGameWindow();
int __cdecl memcmp(const void*,const void*,unsigned);
int __cdecl strcmp(const char*,const char*);
void *__cdecl memset(void*,int,unsigned);
}
#ifdef TITBIT_COMPAT
#include "../compat/compat.h"   /* CD check from the install folder; windowed/borderless game window (src/compat/README.md) */
#endif
#pragma intrinsic(memcmp)
#pragma intrinsic(strcmp)
#pragma intrinsic(memset)
#pragma intrinsic(strlen)
HDC gameDC=0;HWND gameWindow=0;HINSTANCE gameInstance=0;
class RenderContext {HANDLE context;public:void Release(HDC);bool Create(HDC,Rect*);void InitProjection(int,int);};extern RenderContext renderContext;
Rect gameRect={0,0,0,0};
#ifdef TITBIT_SOURCE_LINK
/* Native501020 is the rectangle inside pickingControl501018+8. The real
 * LINK build exposes that compiler-proved member, rather than a second Rect. */
extern Rect renderBounds;
#else
Rect renderBounds={0,0,0,0};
#endif
bool windowOption=false;
void *multiTex1=0,*multiTex2=0,*multiTex3=0,*multiTex4=0,*activeTexture=0,*clientActiveTexture=0;
bool multitextureAvailable=false;int maxTextureUnits=1;
extern "C" bool __cdecl HasGameExtension(char *list,char *wanted){
 int last=(int)strlen(wanted)-1;int length=(int)strlen(list);
 for(int i=0;i<length;++i){
  if(i==0||(i>1&&list[i-1]=='\n')){
   int matched=0;
   while(list[i]!='\n'){
    if(list[i]==wanted[matched])++matched;
    if(matched>last&&list[i+1]=='\n')return true;
    ++i;
   }
  }
 }
 return false;
}
extern "C" int __cdecl LoadGameExtensions(){
 char *list=_strdup(glGetString(0x1f03));int length=(int)strlen(list);
 for(int i=0;i<length;++i)if(list[i]==' ')list[i]='\n';
 if(HasGameExtension(list,"GL_ARB_multitexture")&&HasGameExtension(list,"GL_EXT_texture_env_combine")){
  glGetIntegerv(0x84e2,&maxTextureUnits);
  multiTex1=wglGetProcAddress("glMultiTexCoord1fARB");multiTex2=wglGetProcAddress("glMultiTexCoord2fARB");
  multiTex3=wglGetProcAddress("glMultiTexCoord3fARB");multiTex4=wglGetProcAddress("glMultiTexCoord4fARB");
  activeTexture=wglGetProcAddress("glActiveTextureARB");clientActiveTexture=wglGetProcAddress("glClientActiveTextureARB");
  multitextureAvailable=true;return 1;
 }
 multitextureAvailable=false;return 0;
}
void RenderContext::InitProjection(int width,int height){
 float material[]={1,1,1,1};
 glClearDepth(1);glDepthFunc(0x203);glShadeModel(0x1d01);glEnable(0xb71);glMatrixMode(0x1701);
 gluPerspective(1,(double)width/(double)height,1,2000);
 glMatrixMode(0x1700);glDepthFunc(0x203);glPointSize(4);glEnable(0xb44);glColor4f(1,1,1,1);
 glEnable(0xb57);glEnable(0xb50);glEnable(0xd80);
 float ambient[]={1,1,1,1};glLightModelfv(0xb53,ambient);glMaterialfv(0x404,0x1202,material);
 glMaterialf(0x404,0x1601,10);glTexGeni(0x2000,0x2500,0x2402);glTexGeni(0x2001,0x2500,0x2402);glCullFace(0x405);
 LoadGameExtensions();
}
extern "C" bool __stdcall ChooseGamePixelFormat(HDC dc){
 PixelFormat pf;memset(&pf,0,sizeof(pf));pf.size=sizeof(pf);pf.version=1;pf.flags=0x25;
 pf.layerType=0;pf.pixelType=0;pf.colorBits=32;pf.accumBits=0;pf.depthBits=32;pf.stencilBits=0;
 int format=ChoosePixelFormat(dc,&pf);
 if(!format){PickingTrace("ChoosePixelFormat failed");return false;}
 if(pf.flags&0x80){PickingTrace("Needs palette");return false;}
 if(!SetPixelFormat(dc,format,&pf)){PickingTrace("SetPixelFormat failed");return false;}
 return true;
}
bool RenderContext::Create(HDC dc,Rect *rect){
 if(!ChooseGamePixelFormat(dc))PostQuitMessage(0);
 context=wglCreateContext(dc);wglMakeCurrent(dc,context);
 InitProjection(rect->right,rect->bottom);return true;
}
void RenderContext::Release(HDC){
 wglMakeCurrent(0,0);
 if(context){wglDeleteContext(context);context=0;}
}

extern "C" int __cdecl CheckGameDisc(){
 char root[]="C:\\";DWORD volumeFlags;char label[2048];char drive;
 for(drive='C';drive<'[';++drive){
  root[0]=drive;
  if(GetVolumeInformationA(root,label,sizeof(label),0,&volumeFlags,&volumeFlags,0,0)
     &&!strcmp(label,"TITBIT")&&GetDriveTypeA(root)==5)break;
 }
 if(drive=='['){
  MessageBoxA(0,"\xcd\xe5 \xed\xe0\xe9\xe4\xe5\xed CD-ROM \xf1 \xe8\xe3\xf0\xee\xe9. \xd3\xe1\xe5\xe4\xe8\xf2\xe5\xf1\xfc \xf7\xf2\xee \xe4\xe8\xf1\xea \xf1 \xe8\xe3\xf0\xee\xe9 \xe2\xf1\xf2\xe0\xe2\xeb\xe5\xed \xe2 \xc2\xe0\xf8 CD-ROM \xe8 \xef\xee\xef\xf0\xee\xe1\xf3\xe9\xf2\xe5 \xf1\xed\xee\xe2\xe0","\xce\xf8\xe8\xe1\xea\xe0",16);
  return 1;
 }
 return 0;
}

extern "C" void __cdecl ShutdownGameWindow(){
 if(gameDC){
  renderContext.Release(gameDC);
  if(gameDC&&!ReleaseDC(gameWindow,gameDC)){
   MessageBoxA(0,"Release Device Context Failed.","SHUTDOWN ERROR",64);gameDC=0;
  }
 }
 if(gameWindow&&!DestroyWindow(gameWindow)){
  MessageBoxA(0,"Could Not Release hWnd.","SHUTDOWN ERROR",64);gameWindow=0;
 }
 if(!UnregisterClassA("OpenGL",gameInstance)){
  MessageBoxA(0,"Could Not Unregister Class.","SHUTDOWN ERROR",64);gameInstance=0;
 }
}

extern "C" int __cdecl CreateGameWindow(const char *title,int width,int height){
#ifdef TITBIT_COMPAT
 CompatGameCanvas(&width,&height);
#endif
 WindowClass wc;
 gameInstance=GetModuleHandleA(0);
 wc.style=0x23;wc.proc=GameWindowProc;wc.classExtra=0;wc.windowExtra=0;
 wc.instance=gameInstance;wc.icon=LoadIconA(gameInstance,(const char*)202);
 wc.cursor=LoadCursorA(0,(const char*)0x7f00);wc.background=0;wc.menu=0;wc.name="OpenGL";
 if(!RegisterClassA(&wc)){MessageBoxA(0,"Failed To Register The Window Class.","ERROR",48);return 0;}
 ShowCursor(0);
 gameWindow=CreateWindowExA(0x40008,"OpenGL",title,0x86000000,0,0,width,height,0,0,gameInstance,0);
 if(!gameWindow){ShutdownGameWindow();MessageBoxA(0,"Window Creation Error.","ERROR",48);return 0;}
 SetRect(&gameRect,0,0,width,height);renderBounds=gameRect;
 gameDC=GetDC(gameWindow);
 if(!gameDC){ShutdownGameWindow();MessageBoxA(0,"Can't Create A GL Device Context.","ERROR",48);return 0;}
 renderContext.Create(gameDC,&gameRect);
 ShowWindow(gameWindow,5);SetForegroundWindow(gameWindow);SetFocus(gameWindow);
 glViewport(gameRect.left,gameRect.top,gameRect.right-gameRect.left,gameRect.bottom-gameRect.top);
 glDisable(0xb71);glClear(0x4100);glEnable(0xb71);ShowCursor(0);
 AttachGameWindow(gameWindow,windowOption);return 1;
}
