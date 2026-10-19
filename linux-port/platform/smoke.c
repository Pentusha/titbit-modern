#define _XOPEN_SOURCE 700
#include "titbit_platform.h"
#include <SDL.h>
#include <GL/gl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__i386__)
#define WAPI __attribute__((stdcall))
#else
#define WAPI
#endif
extern void *WAPI CreateFileA(const char *,uint32_t,uint32_t,void *,uint32_t,uint32_t,void *);
extern int32_t WAPI WriteFile(void *,const void *,uint32_t,uint32_t *,void *);
extern int32_t WAPI ReadFile(void *,void *,uint32_t,uint32_t *,void *);
extern uint32_t WAPI SetFilePointer(void *,int32_t,int32_t *,uint32_t);
extern int32_t WAPI SetEndOfFile(void *);
extern int32_t WAPI FlushFileBuffers(void *);
extern int32_t WAPI CloseHandle(void *);
extern void *WAPI CreateThread(void *,size_t,uint32_t (WAPI *)(void *),void *,uint32_t,uint32_t *);
extern uint32_t WAPI ResumeThread(void *);
extern void WAPI InitializeCriticalSection(void *);
extern void WAPI EnterCriticalSection(void *);
extern void WAPI LeaveCriticalSection(void *);
extern void WAPI DeleteCriticalSection(void *);
extern uint32_t WAPI TlsAlloc(void);
extern void *WAPI TlsGetValue(uint32_t);
extern int32_t WAPI TlsSetValue(uint32_t,void *);
extern int32_t WAPI TlsFree(uint32_t);
extern void *WAPI VirtualAlloc(void *,size_t,uint32_t,uint32_t);
extern int32_t WAPI VirtualFree(void *,size_t,uint32_t);
extern void *WAPI HeapCreate(uint32_t,size_t,size_t);
extern void *WAPI HeapAlloc(void *,uint32_t,size_t);
extern void *WAPI HeapReAlloc(void *,uint32_t,void *,size_t);
extern size_t WAPI HeapSize(void *,uint32_t,const void *);
extern int32_t WAPI HeapFree(void *,uint32_t,void *);
extern int32_t WAPI HeapDestroy(void *);
extern int32_t WAPI GetStringTypeExA(uint32_t,uint32_t,const char *,int,uint16_t *);
extern char *WAPI CharLowerA(char *);
extern unsigned WAPI mixerGetNumDevs(void);
extern uint32_t WAPI mixerOpen(void **,unsigned,uintptr_t,uintptr_t,uint32_t);
extern uint32_t WAPI mixerClose(void *);
extern uint32_t WAPI mixerGetLineInfoA(void *,uint32_t *,uint32_t);
extern uint32_t WAPI mixerGetLineControlsA(void *,void *,uint32_t);
extern uint32_t WAPI mixerGetControlDetailsA(void *,void *,uint32_t);
extern uint32_t WAPI mixerSetControlDetails(void *,const void *,uint32_t);
extern void *tb_gl_stdcall_proc(const char *);
static uint32_t test_tls;
static int thread_completed;
static uint32_t WAPI kernel_worker(void *argument) {
 assert(argument==(void*)(uintptr_t)123);assert(TlsGetValue(test_tls)==NULL);
 assert(TlsSetValue(test_tls,(void*)(uintptr_t)0x66));assert(TlsGetValue(test_tls)==(void*)(uintptr_t)0x66);
 __atomic_store_n(&thread_completed,42,__ATOMIC_RELEASE);return 42;
}
static void test_kernel(void) {
 unsigned char critical[24];InitializeCriticalSection(critical);EnterCriticalSection(critical);EnterCriticalSection(critical);LeaveCriticalSection(critical);LeaveCriticalSection(critical);DeleteCriticalSection(critical);
 test_tls=TlsAlloc();assert(test_tls!=0xffffffffu);assert(TlsSetValue(test_tls,(void*)(uintptr_t)0x55));
 void *thread=CreateThread(NULL,0,kernel_worker,(void*)(uintptr_t)123,4,NULL);assert(thread);
 tb_sleep(10);assert(__atomic_load_n(&thread_completed,__ATOMIC_ACQUIRE)==0);assert(ResumeThread(thread)==1);
 uint32_t start=tb_milliseconds();while(__atomic_load_n(&thread_completed,__ATOMIC_ACQUIRE)!=42&&tb_milliseconds()-start<2000)tb_sleep(2);
 assert(thread_completed==42);assert(TlsGetValue(test_tls)==(void*)(uintptr_t)0x55);assert(CloseHandle(thread));assert(TlsFree(test_tls));
 uint32_t *memory=VirtualAlloc(NULL,4096,0x2000,1);assert(memory);assert(VirtualAlloc(memory,4096,0x1000,4)==memory);
 memory[0]=0x12345678;assert(memory[0]==0x12345678);assert(VirtualFree(memory,4096,0x4000));assert(VirtualAlloc(memory,4096,0x1000,4)==memory);assert(memory[0]==0);assert(VirtualFree(memory,0,0x8000));
 void *heap=HeapCreate(0,0,8192);assert(heap);unsigned char *block=HeapAlloc(heap,8,64);assert(block&&HeapSize(heap,0,block)==64);
 for(unsigned n=0;n<64;n++)assert(block[n]==0);
 memcpy(block,"native heap",12);
 block=HeapReAlloc(heap,8,block,256);assert(block&&!memcmp(block,"native heap",12));for(unsigned n=64;n<256;n++)assert(block[n]==0);
 assert(HeapFree(heap,0,block));assert(HeapAlloc(heap,0,9000)==NULL);assert(HeapAlloc(heap,0,128));assert(HeapDestroy(heap));
 puts("PASS suspended native thread/resume, recursive critical section, TLS isolation, reserve/commit/decommit/release");
 puts("PASS tracked heap allocation/reallocation/zeroing/size limits/destruction");
}
static void test_locale_mixer(void) {
 const char text[]={'5','A',(char)0xc0,' '};uint16_t types[4];assert(GetStringTypeExA(0x419,1,text,4,types));
 assert((types[0]&4)&&(types[1]&257)==257&&(types[2]&257)==257&&(types[3]&72)==72);
 char lower[]={'T','E','s','T',(char)0xc0,0};CharLowerA(lower);assert(!memcmp(lower,"test",4)&&(unsigned char)lower[4]==0xe0);
 assert(mixerGetNumDevs()>0);void *mixer=NULL;assert(mixerOpen(&mixer,0,0,0,0)==0);
 uint32_t line[42]={0};line[0]=sizeof(line);line[6]=4;assert(mixerGetLineInfoA(mixer,line,3)==0&&line[3]==1&&line[7]==2&&line[9]==2);
 uint32_t control[37];struct {uint32_t size,line,type,count,item_size;void *items;} request={sizeof(request),1,0x50030001u,1,sizeof(control),control};
 assert(mixerGetLineControlsA(mixer,&request,2)==0&&control[1]==1&&control[26]==65535);
 uint32_t values[2]={32768,32768};struct {uint32_t size,control,channels,multiple,item_size;void *items;} details={sizeof(details),1,2,0,4,values};
 assert(mixerSetControlDetails(mixer,&details,0)==0&&tb_audio_master_get()==32768);values[0]=values[1]=0;assert(mixerGetControlDetailsA(mixer,&details,0)==0&&values[0]==32768&&values[1]==32768);
 details.control=2;values[0]=values[1]=1;assert(mixerSetControlDetails(mixer,&details,0)==0&&tb_audio_mute_get());
 values[0]=values[1]=0;assert(mixerSetControlDetails(mixer,&details,0)==0&&!tb_audio_mute_get());
 details.control=1;values[0]=values[1]=65535;assert(mixerSetControlDetails(mixer,&details,0)==0);assert(mixerClose(mixer)==0);
 puts("PASS CP1251 Unicode classification/case and application volume/mute mixer controls");
}

static void test_gl_stdcall(void) {
 typedef void (WAPI *Viewport)(GLint,GLint,GLsizei,GLsizei);
 typedef void (WAPI *Color)(GLfloat,GLfloat,GLfloat,GLfloat);
 typedef void (WAPI *Perspective)(GLdouble,GLdouble,GLdouble,GLdouble);
 typedef GLuint (WAPI *Lists)(GLsizei);
 typedef const GLubyte *(WAPI *String)(GLenum);
 typedef void (WAPI *Active)(GLenum);
 Viewport viewport=(Viewport)tb_gl_stdcall_proc("glViewport");
 Color color=(Color)tb_gl_stdcall_proc("glColor4f");
 Perspective perspective=(Perspective)tb_gl_stdcall_proc("gluPerspective");
 Lists lists=(Lists)tb_gl_stdcall_proc("glGenLists");
 String string=(String)tb_gl_stdcall_proc("glGetString");
 Active active=(Active)tb_gl_stdcall_proc("glActiveTextureARB");
 assert(viewport&&color&&perspective&&lists&&string&&active);
 volatile uint32_t guard=0xbadc0ffe;
#if defined(__i386__)
 uintptr_t before,after;
 __asm__ volatile("movl %%esp,%0":"=r"(before)::"memory");
#endif
 for(unsigned n=0;n<10000;n++) {
  viewport(0,0,320,240);color(.2f,.4f,.6f,1);perspective(45,4.0/3.0,1,1000);active(GL_TEXTURE0);
 }
 GLuint list=lists(1);assert(list);glDeleteLists(list,1);assert(string(GL_VERSION));
#if defined(__i386__)
 __asm__ volatile("movl %%esp,%0":"=r"(after)::"memory");
 assert(before==after);puts("PASS i386 stdcall stack balance: 40000 GL/GLU calls plus integer/pointer returns");
#else
 puts("PASS GL/GLU trampoline calls and integer/pointer returns (amd64; stdcall stack check requires i386)");
#endif
 assert(guard==0xbadc0ffe);assert(tb_gl_stdcall_proc("unsupported_unknown_signature")==NULL);
}

static void test_input(void) {
 SDL_Event e;TBEvent out;TBInput state;
 /* Drain window activation events before injecting the complete interaction. */
 while(tb_poll_event(&out)){}
 memset(&e,0,sizeof(e));e.type=SDL_KEYDOWN;e.key.keysym.scancode=SDL_SCANCODE_A;e.key.keysym.sym=SDLK_a;assert(SDL_PushEvent(&e)==1);
 memset(&e,0,sizeof(e));e.type=SDL_KEYDOWN;e.key.keysym.scancode=SDL_SCANCODE_AUDIOMUTE;e.key.keysym.sym=SDLK_AUDIOMUTE;assert(SDL_PushEvent(&e)==1);
 memset(&e,0,sizeof(e));e.type=SDL_MOUSEMOTION;e.motion.x=127;e.motion.y=73;e.motion.xrel=9;e.motion.yrel=-4;assert(SDL_PushEvent(&e)==1);
 memset(&e,0,sizeof(e));e.type=SDL_MOUSEBUTTONDOWN;e.button.button=SDL_BUTTON_LEFT;e.button.x=127;e.button.y=73;assert(SDL_PushEvent(&e)==1);
 unsigned key=0,motion=0,click=0;
 while(tb_poll_event(&out)){if(out.message==0x100){assert(out.wparam=='A');key++;}if(out.message==0x200){assert((out.lparam&65535)==127);motion++;}if(out.message==0x201)click++;}
 assert(key==1&&motion==1&&click==1);tb_input_read(&state,1);
 assert(state.keys[30]==128&&state.dx==9&&state.dy==-4&&state.buttons[0]==128);
 tb_input_read(&state,0);assert(state.dx==0&&state.dy==0&&state.buttons[0]==128);
 e.type=SDL_KEYUP;e.key.keysym.scancode=SDL_SCANCODE_A;e.key.keysym.sym=SDLK_a;assert(SDL_PushEvent(&e)==1);
 while(tb_poll_event(&out)){}tb_input_read(&state,0);assert(state.keys[30]==0);
 puts("PASS keyboard DIK state, relative mouse, click messages");
}
static void test_audio(void) {
 TBAudio *a=tb_audio_open(44100,2,16);assert(a);
 int16_t samples[4410*2];for(unsigned i=0;i<4410;i++)samples[2*i]=samples[2*i+1]=(i%100<50?6000:-6000);
 TBWaveHeader h;memset(&h,0,sizeof(h));h.data=(char*)samples;h.length=sizeof(samples);h.flags=TB_WAVE_PREPARED;
 assert(tb_audio_queue(a,&h));uint32_t start=tb_milliseconds();
 while(!(h.flags&TB_WAVE_DONE)&&tb_milliseconds()-start<2000)tb_sleep(5);
 assert((h.flags&TB_WAVE_DONE)&&!(h.flags&TB_WAVE_INQUEUE));
 assert(tb_audio_volume(a,64));h.flags=TB_WAVE_PREPARED;assert(tb_audio_queue(a,&h));tb_audio_reset(a);assert(h.flags&TB_WAVE_DONE);tb_audio_close(a);
 puts("PASS PCM callback playback, queue completion, reset and volume");
}
static void test_paths(void) {
 char directory[]="/tmp/titbit-platform-XXXXXX",name[512],resolved[512];assert(mkdtemp(directory));
 snprintf(name,sizeof(name),"%s/QUEST.ezd",directory);FILE *f=fopen(name,"wb");assert(f);fputs("fixture",f);fclose(f);
 char wanted[512];snprintf(wanted,sizeof(wanted),"%s\\quest.EZD",directory);
 assert(tb_resolve_path(wanted,resolved,sizeof(resolved),1));f=fopen(resolved,"rb");assert(f);assert(fgetc(f)=='f');fclose(f);
 assert(!tb_resolve_path("C:\\QUEST.ezd",resolved,sizeof(resolved),1));
 unlink(name);rmdir(directory);puts("PASS native case-insensitive resource paths");
}
static void test_file_api(void) {
 char directory[]="/tmp/titbit-file-api-XXXXXX",path[512];uint32_t done;char buffer[16]={0};
 assert(mkdtemp(directory));snprintf(path,sizeof(path),"%s/Save.Dat",directory);
 void *file=CreateFileA(path,0xc0000000u,0,NULL,1,0,NULL);assert(file!=(void*)(intptr_t)-1);
 assert(WriteFile(file,"save-fixture",12,&done,NULL)&&done==12);
 assert(SetFilePointer(file,4,NULL,0)==4);assert(SetEndOfFile(file));assert(FlushFileBuffers(file));assert(CloseHandle(file));
 snprintf(path,sizeof(path),"%s\\save.DAT",directory);
 file=CreateFileA(path,0x80000000u,0,NULL,3,0,NULL);assert(file!=(void*)(intptr_t)-1);
 assert(ReadFile(file,buffer,sizeof(buffer),&done,NULL)&&done==4&&!memcmp(buffer,"save",4));assert(CloseHandle(file));
 snprintf(path,sizeof(path),"%s/Save.Dat",directory);unlink(path);rmdir(directory);
 puts("PASS Win32-compatible create/write/seek/truncate/flush/read/close");
}
int main(void) {
 if(!tb_init()){fprintf(stderr,"SDL init: %s\n",tb_error());return 1;}
 TBWindow *w=tb_window_create("Titbit native platform validation",320,240,0);assert(w);
 void *context=tb_gl_context(w);if(!context){fprintf(stderr,"GL context: %s\n",tb_error());return 2;}
 assert(tb_gl_make_current(w,context));printf("OpenGL: %s / %s\n",glGetString(GL_VERSION),glGetString(GL_RENDERER));
 glViewport(0,0,320,240);glClearColor(.25f,.5f,.75f,1);glClear(GL_COLOR_BUFFER_BIT);glFinish();
 unsigned char pixel[4]={0};glReadPixels(160,120,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
 assert(pixel[0]>=62&&pixel[0]<=66&&pixel[1]>=126&&pixel[1]<=130&&pixel[2]>=189&&pixel[2]<=193);
 unsigned char *frame=malloc(320u*240u*4u);int capture_width,capture_height;
 assert(frame&&tb_read_frame_pixels(frame,320u*240u*4u,&capture_width,&capture_height));assert(capture_width==320&&capture_height==240&&!memcmp(frame,pixel,4));free(frame);
 tb_swap(w);puts("PASS native OpenGL compatibility context, framebuffer pixels and swap");
 uint64_t before=tb_counter();tb_sleep(20);assert(tb_counter()>before);assert(tb_frequency()>0);
 test_gl_stdcall();test_input();test_audio();test_paths();test_file_api();test_kernel();test_locale_mixer();tb_window_destroy(w);tb_shutdown();puts("ALL PLATFORM CHECKS PASSED");return 0;
}
