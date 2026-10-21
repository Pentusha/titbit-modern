#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include "titbit_platform.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <dlfcn.h>
#if defined(__i386__)
#define WAPI __attribute__((stdcall))
#else
#define WAPI
#endif
typedef void *HANDLE;
typedef int32_t BOOL;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef int32_t (WAPI *WNDPROC)(HANDLE,uint32_t,uint32_t,int32_t);
typedef struct { int32_t left,top,right,bottom; } RECT;
typedef struct { HANDLE window;uint32_t message,wparam;int32_t lparam;uint32_t time;int32_t x,y; } MSG;
typedef struct { uint32_t style;WNDPROC proc;int32_t cls_extra,win_extra;HANDLE instance,icon,cursor,background;const char *menu,*name; } WNDCLASS;
typedef struct { uint16_t size,version;uint32_t flags;uint8_t type,color,red,red_shift,green,green_shift,blue,blue_shift,alpha,alpha_shift,accum,accum_red,accum_green,accum_blue,accum_alpha,depth,stencil,aux,layer,reserved;uint32_t layer_mask,visible_mask,damage_mask; } PixelFormat;
static WNDPROC window_proc;
static TBWindow *main_window;
typedef struct {char *name;WNDPROC proc;void *icon;} WindowClassRecord;
static WindowClassRecord registered_classes[32];
typedef struct WindowRecord {TBWindow *window;WNDPROC proc;struct WindowRecord *next;} WindowRecord;
static WindowRecord *window_records;
static MSG messages[128];static unsigned qfirst,qlast;
static _Thread_local DWORD last_error;
void WAPI SetLastError(DWORD e){last_error=e;}
DWORD WAPI GetLastError(void){return last_error;}
static void err(void){last_error=errno;}
uint16_t WAPI RegisterClassA(const WNDCLASS *wc){if(!wc||!wc->proc||!wc->name)return 0;for(unsigned i=0;i<32;i++){if(registered_classes[i].name&&!strcmp(registered_classes[i].name,wc->name))return i+1;if(!registered_classes[i].name){registered_classes[i]=(WindowClassRecord){strdup(wc->name),wc->proc,wc->icon};window_proc=wc->proc;return i+1;}}last_error=8;return 0;}
uint16_t WAPI RegisterClassExA(const void *wc){if(!wc||*(const uint32_t*)wc<sizeof(WNDCLASS)+4)return 0;return RegisterClassA((const WNDCLASS*)((const char*)wc+4));}
BOOL WAPI UnregisterClassA(const char *name,HANDLE instance){(void)instance;if(!name)return 0;for(unsigned i=0;i<32;i++)if(registered_classes[i].name&&!strcmp(registered_classes[i].name,name)){free(registered_classes[i].name);registered_classes[i]=(WindowClassRecord){0};return 1;}last_error=1411;return 0;}
HANDLE WAPI CreateWindowExA(DWORD ex,const char *cls,const char *title,DWORD style,int x,int y,int width,int height,HANDLE parent,HANDLE menu,HANDLE instance,void *param){
 (void)ex;(void)style;(void)x;(void)y;(void)parent;(void)menu;(void)instance;(void)param;
 WNDPROC proc=NULL;void *icon=NULL;for(unsigned i=0;i<32;i++)if(registered_classes[i].name&&cls&&!strcmp(registered_classes[i].name,cls)){proc=registered_classes[i].proc;icon=registered_classes[i].icon;break;}
 if(!proc){last_error=1407;return NULL;}
 int game_canvas=!parent&&cls&&!strcmp(cls,"OpenGL");
 TBWindow *window=tb_window_create(title,width>0?width:1,height>0?height:1,game_canvas);if(!window)return NULL;
 WindowRecord *record=malloc(sizeof(*record));if(!record){tb_window_destroy(window);last_error=8;return NULL;}*record=(WindowRecord){window,proc,window_records};window_records=record;
 tb_window_icon(window,icon);
 if(width>0&&height>0)main_window=window;
 return window;
}
TBWindow *tb_win32_main_window(void){return main_window;}
BOOL WAPI DestroyWindow(HANDLE w){WindowRecord **cursor=&window_records;while(*cursor&&(*cursor)->window!=w)cursor=&(*cursor)->next;if(!*cursor){last_error=1400;return 0;}WindowRecord *record=*cursor;*cursor=record->next;free(record);tb_window_destroy(w);if(w==main_window)main_window=NULL;return 1;}
HANDLE WAPI GetDC(HANDLE w){return w;}
int WAPI ReleaseDC(HANDLE w,HANDLE dc){return w==dc;}
int WAPI ChoosePixelFormat(HANDLE dc,const PixelFormat *format){
 /* SDL's preselected format is double-buffered RGBA with a 24-bit depth
  * buffer. Like Win32 ChoosePixelFormat, choose the closest supported format
  * rather than requiring the requested 32-bit depth to be exact. */
 if(!tb_window_opengl(dc)||!format||format->size<sizeof(*format)||format->version!=1||format->type||format->layer)return 0;
 return 1;
}
BOOL WAPI SetPixelFormat(HANDLE dc,int format,const PixelFormat *request){return format==1&&ChoosePixelFormat(dc,request)==1;}
HANDLE WAPI wglCreateContext(HANDLE dc){return tb_gl_context(dc);}
BOOL WAPI wglMakeCurrent(HANDLE dc,HANDLE context){return tb_gl_make_current(dc,context);}
BOOL WAPI wglDeleteContext(HANDLE context){tb_gl_delete_context(context);return 1;}
extern void *tb_gl_stdcall_proc(const char *name);
void *WAPI wglGetProcAddress(const char *name){return tb_gl_stdcall_proc(name);}
BOOL WAPI SwapBuffers(HANDLE dc){tb_swap(dc);return dc!=NULL;}
BOOL WAPI ShowWindow(HANDLE w,int mode){return tb_window_show(w,mode!=0);}
BOOL WAPI MoveWindow(HANDLE w,int x,int y,int width,int height,BOOL repaint){(void)repaint;return tb_window_move(w,x,y,width,height);}
BOOL WAPI SetForegroundWindow(HANDLE w){return tb_window_focus(w);}
HANDLE WAPI SetFocus(HANDLE w){HANDLE previous=main_window;return tb_window_focus(w)?previous:NULL;}
HANDLE WAPI GetActiveWindow(void){return main_window;}
int32_t WAPI SendMessageA(HANDLE w,uint32_t message,uint32_t wp,int32_t lp){for(WindowRecord *record=window_records;record;record=record->next)if(record->window==w)return record->proc(w,message,wp,lp);return window_proc?window_proc(w,message,wp,lp):0;}
int WAPI ShowCursor(BOOL show){return tb_cursor(show);}
BOOL WAPI SetRect(RECT *r,int l,int t,int right,int b){if(!r)return 0;r->left=l;r->top=t;r->right=right;r->bottom=b;return 1;}
BOOL WAPI GetClientRect(HANDLE w,RECT *r){int width=0,height=0;if(!w||!r)return 0;tb_window_size(w,&width,&height);return SetRect(r,0,0,width,height);}
BOOL WAPI PostMessageA(HANDLE w,uint32_t message,uint32_t wp,int32_t lp){unsigned next=(qlast+1)%128;if(next==qfirst)return 0;messages[qlast]=(MSG){w,message,wp,lp,tb_milliseconds(),0,0};qlast=next;return 1;}
void WAPI PostQuitMessage(int code){PostMessageA(main_window,0x12,(uint32_t)code,0);}
BOOL WAPI PeekMessageA(MSG *out,HANDLE w,uint32_t min,uint32_t max,uint32_t remove){
 TBEvent e;(void)w;(void)min;(void)max;
 if(qfirst==qlast&&tb_poll_event(&e))PostMessageA(main_window,e.message,(uint32_t)e.wparam,(int32_t)e.lparam);
 if(qfirst==qlast)return 0;
 *out=messages[qfirst];if(remove&1)qfirst=(qfirst+1)%128;return 1;
}
int32_t WAPI DispatchMessageA(const MSG *m){return SendMessageA(m->window,m->message,m->wparam,m->lparam);}
BOOL WAPI TranslateMessage(const MSG *m){(void)m;return 0;/* Engine consumes keydown directly, not WM_CHAR. */}
int32_t WAPI DefWindowProcA(HANDLE w,uint32_t message,uint32_t wp,int32_t lp){(void)w;(void)wp;(void)lp;if(message==0x10)PostQuitMessage(0);return 0;}
BOOL WAPI QueryPerformanceCounter(int64_t *out){if(!out)return 0;*out=(int64_t)tb_counter();return 1;}
BOOL WAPI QueryPerformanceFrequency(int64_t *out){if(!out)return 0;*out=(int64_t)tb_frequency();return 1;}
DWORD WAPI timeGetTime(void){return tb_milliseconds();}
void WAPI Sleep(DWORD ms){tb_sleep(ms);}
LONG WAPI InterlockedExchange(volatile LONG *p,LONG v){return __atomic_exchange_n(p,v,__ATOMIC_SEQ_CST);}
LONG WAPI InterlockedIncrement(volatile LONG *p){return __atomic_add_fetch(p,1,__ATOMIC_SEQ_CST);}
LONG WAPI InterlockedDecrement(volatile LONG *p){return __atomic_sub_fetch(p,1,__ATOMIC_SEQ_CST);}
void WAPI OutputDebugStringA(const char *s){if(s)fputs(s,stderr);}
DWORD WAPI GetCurrentProcessId(void){return (DWORD)getpid();}
DWORD WAPI GetCurrentThreadId(void){return (DWORD)(uintptr_t)pthread_self();}
DWORD WAPI GetCurrentDirectoryA(DWORD size,char *out){char *cwd=getcwd(NULL,0);size_t length;if(!cwd){err();return 0;}length=strlen(cwd);if(size>length&&out)memcpy(out,cwd,length+1);free(cwd);return (DWORD)(size>length?length:length+1);}
BOOL WAPI SetCurrentDirectoryA(const char *name){char path[4096];if(!tb_resolve_path(name,path,sizeof(path),1))return 0;if(chdir(path)){err();return 0;}return 1;}
DWORD WAPI GetModuleFileNameA(HANDLE module,char *out,DWORD size){ssize_t n;(void)module;if(!out||!size)return 0;n=readlink("/proc/self/exe",out,size-1);if(n<0){err();return 0;}out[n]=0;return (DWORD)n;}
DWORD WAPI GetEnvironmentVariableA(const char *name,char *out,DWORD size){const char *s=getenv(name);size_t n;if(!s){last_error=203;return 0;}n=strlen(s);if(size>n&&out)memcpy(out,s,n+1);return (DWORD)(size>n?n:n+1);}
BOOL WAPI SetEnvironmentVariableA(const char *name,const char *value){int r=value?setenv(name,value,1):unsetenv(name);if(r)err();return r==0;}

/* Handles own descriptors. The magic distinguishes these from SDL HWNDs. */
typedef struct { uint32_t magic;int fd,owned; } FileHandle;
#define FILE_MAGIC 0x54424648u
static FileHandle *file(HANDLE h){if(!h||h==(HANDLE)(intptr_t)-1)return NULL;FileHandle *f=h;return f->magic==FILE_MAGIC?f:NULL;}
HANDLE WAPI CreateFileA(const char *name,DWORD access,DWORD share,void *security,DWORD creation,DWORD attributes,HANDLE model){
 char path[4096];FileHandle *f;int flags;(void)share;(void)security;(void)attributes;(void)model;
 flags=(access&0x40000000u)?((access&0x80000000u)?O_RDWR:O_WRONLY):O_RDONLY;
 switch(creation){case 1:flags|=O_CREAT|O_EXCL;break;case 2:flags|=O_CREAT|O_TRUNC;break;case 3:break;case 4:flags|=O_CREAT;break;case 5:flags|=O_TRUNC;break;default:last_error=87;return (HANDLE)(intptr_t)-1;}
 if(!tb_resolve_path(name,path,sizeof(path),creation==3||creation==5)){last_error=2;return (HANDLE)(intptr_t)-1;}
 f=calloc(1,sizeof(*f));if(!f){last_error=8;return (HANDLE)(intptr_t)-1;}f->fd=open(path,flags,0666);
 if(f->fd<0){err();free(f);return (HANDLE)(intptr_t)-1;}f->magic=FILE_MAGIC;f->owned=1;return f;
}
BOOL WAPI ReadFile(HANDLE h,void *buffer,DWORD count,DWORD *done,void *overlapped){FileHandle *f=file(h);ssize_t n;if(done)*done=0;if(!f||overlapped){last_error=87;return 0;}do n=read(f->fd,buffer,count);while(n<0&&errno==EINTR);if(n<0){err();return 0;}if(done)*done=(DWORD)n;return 1;}
BOOL WAPI WriteFile(HANDLE h,const void *buffer,DWORD count,DWORD *done,void *overlapped){FileHandle *f=file(h);ssize_t n;if(done)*done=0;if(!f||overlapped){last_error=87;return 0;}do n=write(f->fd,buffer,count);while(n<0&&errno==EINTR);if(n<0){err();return 0;}if(done)*done=(DWORD)n;return 1;}
DWORD WAPI SetFilePointer(HANDLE h,LONG low,LONG *high,DWORD origin){FileHandle *f=file(h);off_t result,offset=high?((int64_t)*high<<32)|(uint32_t)low:low;if(!f||origin>2){last_error=87;return 0xffffffffu;}result=lseek(f->fd,offset,(int)origin);if(result<0){err();return 0xffffffffu;}if(high)*high=(LONG)((uint64_t)result>>32);last_error=0;return (DWORD)result;}
BOOL WAPI SetEndOfFile(HANDLE h){FileHandle *f=file(h);off_t pos;if(!f)return 0;pos=lseek(f->fd,0,SEEK_CUR);if(pos<0||ftruncate(f->fd,pos)){err();return 0;}return 1;}
BOOL WAPI FlushFileBuffers(HANDLE h){FileHandle *f=file(h);if(!f)return 0;if(fsync(f->fd)){err();return 0;}return 1;}
extern int tb_kernel_close_handle(void *);
BOOL WAPI CloseHandle(HANDLE h){FileHandle *f=file(h);if(!f){if(tb_kernel_close_handle(h))return 1;last_error=6;return 0;}int result=close(f->fd);f->magic=0;if(f->owned)free(f);if(result)err();return result==0;}
DWORD WAPI GetFileType(HANDLE h){return file(h)?1:0;}
static FileHandle std_handles[3]={{FILE_MAGIC,0,0},{FILE_MAGIC,1,0},{FILE_MAGIC,2,0}};
HANDLE WAPI GetStdHandle(DWORD which){if(which==0xfffffff6u)return &std_handles[0];if(which==0xfffffff5u)return &std_handles[1];if(which==0xfffffff4u)return &std_handles[2];last_error=87;return (HANDLE)(intptr_t)-1;}

typedef struct __attribute__((packed)) { uint16_t format,channels;uint32_t rate,bytes;uint16_t align,bits,extra; } WAVEFORMAT;
DWORD WAPI waveOutOpen(HANDLE *out,unsigned device,const WAVEFORMAT *format,uintptr_t callback,uintptr_t instance,DWORD flags){(void)device;(void)instance;if(!out||!format||format->format!=1||callback||flags)return 32;*out=tb_audio_open(format->rate,format->channels,format->bits);return *out?0:4;}
DWORD WAPI waveOutPrepareHeader(HANDLE device,TBWaveHeader *h,unsigned size){if(!device||!h||size<sizeof(*h))return 11;h->flags|=TB_WAVE_PREPARED;return 0;}
DWORD WAPI waveOutUnprepareHeader(HANDLE device,TBWaveHeader *h,unsigned size){if(!device||!h||size<sizeof(*h))return 11;if(h->flags&TB_WAVE_INQUEUE)return 33;h->flags&=~TB_WAVE_PREPARED;return 0;}
DWORD WAPI waveOutWrite(HANDLE device,TBWaveHeader *h,unsigned size){if(!device||!h||size<sizeof(*h)||!(h->flags&TB_WAVE_PREPARED))return 34;return tb_audio_queue(device,h)?0:11;}
DWORD WAPI waveOutReset(HANDLE device){if(!device)return 5;tb_audio_reset(device);return 0;}
DWORD WAPI waveOutClose(HANDLE device){if(!device)return 5;tb_audio_close(device);return 0;}
