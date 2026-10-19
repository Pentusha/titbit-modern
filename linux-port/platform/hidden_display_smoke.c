/* Check the real desktop GL transport without ever mapping a visible window.
 * This is a test executable, not part of the engine or a replacement scene. */
#include "titbit_platform.h"
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
    if(argc!=2)return 2;
    if(!tb_init()){fprintf(stderr,"SDL: %s\n",tb_error());return 1;}
    TBWindow *window=tb_window_create("Titbit hidden GL diagnostic",64,64,0);
    if(!window||!tb_gl_make_current(window,tb_gl_context(window))){fprintf(stderr,"GL: %s\n",tb_error());return 1;}
    glClearColor(.25f,.5f,.75f,1);glClear(GL_COLOR_BUFFER_BIT);glFinish();
    TBFrameStats frame;
    int ok=tb_capture_frame(argv[1],&frame);
    printf("vendor=%s\nrenderer=%s\nversion=%s\n",glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
    printf("pixels=%u, nonblack=%u\n",(unsigned)frame.pixels,(unsigned)frame.nonblack);
    tb_window_destroy(window);tb_shutdown();
    return ok&&frame.nonblack==4096?0:1;
}
