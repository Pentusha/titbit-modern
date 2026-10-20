#include "titbit_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct { void *window; unsigned message,wparam; int lparam; unsigned time; int x,y; } GameMessage;
#if defined(__i386__)
#define WAPI __attribute__((stdcall))
#else
#define WAPI
#endif
extern int titbit_linux_run(void);
extern void titbit_linux_initialize_pod_globals(void);
extern int WAPI PeekMessageA(GameMessage *,void *,unsigned,unsigned,unsigned);
extern int WAPI TranslateMessage(const GameMessage *);
extern int WAPI DispatchMessageA(const GameMessage *);
int tb_linux_poll(void) {
    GameMessage message;
    while (PeekMessageA(&message,0,0,0,1)) {
        if (message.message==0x12) return 0;
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return 1;
}
unsigned tb_linux_frame_limit(void) {
    const char *value=getenv("TITBIT_TEST_FRAMES");
    return value ? (unsigned)strtoul(value,0,10) : 0;
}
int tb_linux_has_assets(void) {
    char path[4096];
    if (tb_resolve_path("QUEST.EZD",path,sizeof(path),1)) return 1;
    fprintf(stderr,"QUEST.EZD is missing. Import your game files with tools/import_assets.sh.\n");
    return 0;
}
void tb_linux_report(unsigned frames) {
    fprintf(stderr,"Titbit Linux: completed %u game frames\n",frames);
}
int main(int argc,char **argv) {
    for(int i=1;i<argc;++i) {
        if(!strcmp(argv[i],"--data") && i+1<argc) {
            if(chdir(argv[++i])) {perror("--data");return 2;}
        } else if(!strcmp(argv[i],"--windowed")) {
            setenv("TITBIT_FULLSCREEN","0",1);
        } else if(!strcmp(argv[i],"--fullscreen")) {
            setenv("TITBIT_FULLSCREEN","1",1);
        } else if(!strcmp(argv[i],"--help")) {
            puts("titbit [--data GAME_DIRECTORY] [--windowed | --fullscreen]\nF11 / Alt+Enter: toggle fullscreen");return 0;
        } else {fprintf(stderr,"Unknown argument: %s\n",argv[i]);return 2;}
    }
    if (!tb_init()) {fprintf(stderr,"SDL: %s\n",tb_error());return 1;}
    titbit_linux_initialize_pod_globals();
    int result=titbit_linux_run();
    tb_shutdown();
    return result;
}
