#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__i386__)
#define WAPI __attribute__((stdcall))
#else
#define WAPI
#endif
typedef struct { uint32_t size;int32_t width,height;uint16_t planes,bits;uint32_t compression,image;int32_t xppm,yppm;uint32_t used,important; } DIB;
extern int32_t WAPI AVIStreamOpenFromFileA(void **,const char *,uint32_t,int32_t,unsigned,void *);
extern int32_t WAPI AVIStreamLength(void *);
extern int32_t WAPI AVIStreamSampleToTime(void *,int32_t);
extern void *WAPI AVIStreamGetFrameOpen(void *,const DIB *);
extern DIB *WAPI AVIStreamGetFrame(void *,int32_t);
extern int32_t WAPI AVIStreamGetFrameClose(void *);
extern uint32_t WAPI AVIStreamRelease(void *);
extern void *WAPI CreateCompatibleDC(void *);
extern int32_t WAPI DeleteDC(void *);
extern void *WAPI CreateDIBSection(void *,const DIB *,unsigned,void **,void *,uint32_t);
extern void *WAPI SelectObject(void *,void *);
extern int32_t WAPI DeleteObject(void *);
extern void *WAPI DrawDibOpen(void);
extern int32_t WAPI DrawDibClose(void *);
extern int32_t WAPI DrawDibDraw(void *,void *,int,int,int,int,const DIB *,const void *,int,int,int,int,unsigned);
static uint64_t hash(const void *data,size_t size) {uint64_t h=14695981039346656037ull;const unsigned char *p=data;while(size--){h^=*p++;h*=1099511628211ull;}return h;}
int main(int argc,char **argv) {
 setvbuf(stdout,NULL,_IOLBF,0);
 assert(sizeof(DIB)==40);
 for(int f=1;f<argc;f++) {
  void *stream=NULL;assert(AVIStreamOpenFromFileA(&stream,argv[f],0x73646976u,0,0,NULL)==0);
  int32_t count=AVIStreamLength(stream);assert(count>0);assert(AVIStreamSampleToTime(stream,count)>0);
  void *frames=AVIStreamGetFrameOpen(stream,NULL);assert(frames);int positions[]={0,count/2,count-1,0};
  uint64_t first=0;
  for(unsigned i=0;i<4;i++) {
   DIB *frame=AVIStreamGetFrame(frames,positions[i]);assert(frame&&frame->size==40&&frame->bits==32&&frame->width>0&&frame->height>0);
   uint64_t digest=hash(frame+1,frame->image);if(i==0)first=digest;if(i==3)assert(digest==first);
   const char *dump=getenv("TITBIT_AVI_TEST_DUMP");
   if(i==0&&dump){const char *base=strrchr(argv[f],'/');char path[4096];snprintf(path,sizeof(path),"%s/%s.bgra",dump,base?base+1:argv[f]);FILE *out=fopen(path,"wb");assert(out);assert(fwrite(frame+1,1,frame->image,out)==frame->image);assert(fclose(out)==0);}
   void *dc=CreateCompatibleDC(NULL),*bits=NULL;assert(dc);
   void *bitmap=CreateDIBSection(dc,frame,0,&bits,NULL,0);assert(bitmap&&bits);SelectObject(dc,bitmap);
   void *draw=DrawDibOpen();assert(draw);
   assert(DrawDibDraw(draw,dc,0,0,frame->width,frame->height,frame,frame+1,0,0,frame->width,frame->height,0));
   assert(!memcmp(bits,frame+1,frame->image));
   printf("FRAME %s %d %dx%d %016llx\n",argv[f],positions[i],frame->width,frame->height,(unsigned long long)digest);
   DrawDibClose(draw);DeleteObject(bitmap);DeleteDC(dc);
  }
  AVIStreamGetFrameClose(frames);AVIStreamRelease(stream);
 }
 puts("ALL AVI/DIB CHECKS PASSED");return 0;
}
