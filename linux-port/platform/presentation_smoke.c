#define _POSIX_C_SOURCE 200809L
#include "titbit_platform.h"
#include "presentation.h"
#include <SDL.h>
#include <GL/gl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void geometry(void) {
 TBViewport vp=tb_fit_viewport(1920,1080,1024,768);int x,y;
 assert(vp.x==240&&vp.y==0&&vp.width==1440&&vp.height==1080);
 assert(tb_map_mouse(vp,960,540,1024,768,&x,&y)&&x==512&&y==384);
 assert(tb_map_mouse(vp,240,0,1024,768,&x,&y)&&x==0&&y==0);
 assert(!tb_map_mouse(vp,239,540,1024,768,&x,&y)&&x==0);
 assert(!tb_map_mouse(vp,1680,540,1024,768,&x,&y)&&x==1023);
 vp=tb_fit_viewport(1024,1024,1024,768);
 assert(vp.x==0&&vp.top==128&&vp.width==1024&&vp.height==768);
 assert(tb_map_mouse(vp,512,512,1024,768,&x,&y)&&x==512&&y==384);
 assert(!tb_map_mouse(vp,512,127,1024,768,&x,&y)&&y==0);
 vp=tb_fit_viewport(1920,1081,1024,768);
 assert(vp.top+vp.height+vp.y==1081);
 vp=tb_fit_viewport(3840,2160,1024,768);
 assert(vp.x==480&&vp.width==2880&&vp.height==2160);
 assert(tb_map_mouse(vp,1920,1080,1024,768,&x,&y)&&x==512&&y==384);
 vp=tb_fit_viewport(0,0,1024,768);assert(!tb_map_mouse(vp,0,0,1024,768,&x,&y));
 puts("PASS aspect-preserving viewport, high DPI and mouse/letterbox geometry");
}

static void drain(void) {TBEvent event;while(tb_poll_event(&event)){} }
static void display(void) {
 setenv("TITBIT_FULLSCREEN","0",1);
 assert(tb_init());
 TBWindow *w=tb_window_create("Titbit display validation",320,240,1);assert(w);
 void *context=tb_gl_context(w);assert(context&&tb_gl_make_current(w,context));
 tb_window_show(w,1);SDL_Window *native=SDL_GL_GetCurrentWindow();assert(native);
 SDL_SetWindowSize(native,1000,600);
 for(int n=0;n<20;n++){tb_sleep(10);drain();}
 int width,height;SDL_GetWindowSize(native,&width,&height);assert(width==1000&&height==600);
 assert(tb_window_move(w,0,0,1920,1080));
 tb_window_size(w,&width,&height);assert(width==320&&height==240);
 tb_gl_viewport(0,0,320,240);GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);
 assert(viewport[0]==100&&viewport[1]==0&&viewport[2]==800&&viewport[3]==600);
 glDisable(GL_DEPTH_TEST);glDisable(GL_SCISSOR_TEST);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 glColor3f(1,0,0);glBegin(GL_QUADS);glVertex2f(-1,-1);glVertex2f(1,-1);glVertex2f(1,1);glVertex2f(-1,1);glEnd();
 unsigned char pixel[4];glReadPixels(500,300,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==255&&pixel[1]==0);
 glReadPixels(10,300,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==0&&pixel[1]==0&&pixel[2]==0);
 puts("PASS resized drawable viewport and centered rendered pixels/black bars");
 SDL_Event event;memset(&event,0,sizeof(event));event.type=SDL_MOUSEMOTION;
 event.motion.windowID=SDL_GetWindowID(native);event.motion.x=500;event.motion.y=300;event.motion.xrel=5;event.motion.yrel=5;
 assert(SDL_PushEvent(&event)==1);TBEvent out;int motion=0;
 while(tb_poll_event(&out))if(out.message==0x200){assert(out.x==160&&out.y==120);motion++;}
 assert(motion==1);TBInput input;tb_input_read(&input,1);assert(input.dx==2&&input.dy==2);
 memset(&event,0,sizeof(event));event.type=SDL_MOUSEBUTTONDOWN;event.button.windowID=SDL_GetWindowID(native);
 event.button.button=SDL_BUTTON_LEFT;event.button.x=10;event.button.y=300;assert(SDL_PushEvent(&event)==1);
 while(tb_poll_event(&out))assert(out.message!=0x201);
 puts("PASS scaled absolute/relative mouse input and ignored letterbox clicks");
 memset(&event,0,sizeof(event));event.type=SDL_KEYDOWN;event.key.windowID=SDL_GetWindowID(native);event.key.keysym.sym=SDLK_F11;
 assert(SDL_PushEvent(&event)==1);drain();assert(SDL_GetWindowFlags(native)&SDL_WINDOW_FULLSCREEN);
 tb_sleep(50);drain();assert(SDL_PushEvent(&event)==1);drain();assert(!(SDL_GetWindowFlags(native)&SDL_WINDOW_FULLSCREEN));
 event.key.keysym.sym=SDLK_RETURN;event.key.keysym.mod=KMOD_ALT;
 assert(SDL_PushEvent(&event)==1);drain();assert(SDL_GetWindowFlags(native)&SDL_WINDOW_FULLSCREEN);
 puts("PASS F11 and Alt+Enter fullscreen toggles");
 tb_window_destroy(w);tb_shutdown();
}

int main(int argc,char **argv) {
 geometry();
 if(argc>1&&!strcmp(argv[1],"--display"))display();
 return 0;
}
