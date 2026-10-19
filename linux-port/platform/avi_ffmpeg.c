/* Source-build implementation of the VfW/GDI subset used by AviPlane.
 * FFmpeg decodes the original stream; libswscale produces bottom-up BGRA DIBs.
 * No Windows codec installation or subprocess is used. AVI audio is not
 * exposed by AviPlane's VIDEO-only API; game audio remains in Audiere. */
#include "titbit_platform.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#if defined(__i386__)
#define WAPI __attribute__((stdcall))
#else
#define WAPI
#endif
typedef struct { uint32_t size;int32_t width,height;uint16_t planes,bits;uint32_t compression,image;int32_t xppm,yppm;uint32_t used,important; } DIB;
typedef struct { uint32_t type,handler,flags,caps;uint16_t priority,language;uint32_t scale,rate,start,length,initial,suggested,quality,sample_size;int32_t left,top,right,bottom;uint32_t edits,format_changes;char name[64]; } StreamInfo;
typedef struct {
 AVFormatContext *format;AVCodecContext *codec;AVFrame *frame;AVPacket *packet;struct SwsContext *sws;
 int stream,refs,pending,valid,eof_sent;int64_t last,frames;AVRational rate;DIB *dib;
} Video;
typedef struct { uint32_t magic;DIB info;unsigned char *pixels; } Bitmap;
typedef struct { uint32_t magic;Bitmap *selected; } MemoryDC;
#define BITMAP_MAGIC 0x5442424du
#define DC_MAGIC 0x54424443u

static void video_free(Video *v) {
 if(!v)return;
 sws_freeContext(v->sws);av_packet_free(&v->packet);av_frame_free(&v->frame);
 avcodec_free_context(&v->codec);avformat_close_input(&v->format);free(v->dib);free(v);
}
void WAPI AVIFileInit(void) { /* Current libavformat requires no global initialization. */ }
void WAPI AVIFileExit(void) { /* Individual streams own and release decoder state. */ }
int32_t WAPI AVIStreamOpenFromFileA(void **out,const char *file,uint32_t type,int32_t index,unsigned mode,void *clsid) {
 char path[4096];Video *v;const AVCodec *codec;(void)mode;(void)clsid;
 if(!out||!file||type!=0x73646976u||index!=0)return (int32_t)0x80044002u;
 *out=NULL;if(!tb_resolve_path(file,path,sizeof(path),1))return (int32_t)0x80044001u;
 v=calloc(1,sizeof(*v));if(!v)return (int32_t)0x8007000eu;
 if(avformat_open_input(&v->format,path,NULL,NULL)<0||avformat_find_stream_info(v->format,NULL)<0)goto fail;
 v->stream=av_find_best_stream(v->format,AVMEDIA_TYPE_VIDEO,-1,-1,&codec,0);if(v->stream<0)goto fail;
 v->codec=avcodec_alloc_context3(codec);if(!v->codec)goto fail;
 if(avcodec_parameters_to_context(v->codec,v->format->streams[v->stream]->codecpar)<0||avcodec_open2(v->codec,codec,NULL)<0)goto fail;
 if(v->codec->width<=0||v->codec->height<=0||v->codec->width>16384||v->codec->height>16384)goto fail;
 v->rate=av_guess_frame_rate(v->format,v->format->streams[v->stream],NULL);if(!v->rate.num||!v->rate.den)goto fail;
 v->frames=v->format->streams[v->stream]->nb_frames;
 if(v->frames<=0&&v->format->streams[v->stream]->duration>0)v->frames=av_rescale_q(v->format->streams[v->stream]->duration,v->format->streams[v->stream]->time_base,av_inv_q(v->rate));
 if(v->frames<=0||v->frames>INT32_MAX)goto fail;
 v->frame=av_frame_alloc();v->packet=av_packet_alloc();
 size_t bytes=(size_t)v->codec->width*v->codec->height*4;if(bytes>512u*1024u*1024u)goto fail;
 v->dib=calloc(1,sizeof(DIB)+bytes);if(!v->frame||!v->packet||!v->dib)goto fail;
 v->dib->size=40;v->dib->width=v->codec->width;v->dib->height=v->codec->height;v->dib->planes=1;v->dib->bits=32;v->dib->image=(uint32_t)bytes;
 v->last=-1;v->refs=1;*out=v;return 0;
fail:video_free(v);return (int32_t)0x80044008u;
}
int32_t WAPI AVIStreamInfoA(Video *v,StreamInfo *out,int32_t size) {
 if(!v||!out||size<(int32_t)sizeof(*out))return (int32_t)0x80070057u;
 memset(out,0,sizeof(*out));out->type=0x73646976u;out->handler=v->format->streams[v->stream]->codecpar->codec_tag;
 out->scale=(uint32_t)v->rate.den;out->rate=(uint32_t)v->rate.num;out->length=(uint32_t)v->frames;
 out->right=v->codec->width;out->bottom=v->codec->height;out->suggested=v->dib->image;out->quality=0xffffffffu;return 0;
}
int32_t WAPI AVIStreamLength(Video *v) { return v?(int32_t)v->frames:-1; }
int32_t WAPI AVIStreamSampleToTime(Video *v,int32_t sample) { return v?(int32_t)av_rescale_q(sample,av_inv_q(v->rate),(AVRational){1,1000}):-1; }
void *WAPI AVIStreamGetFrameOpen(Video *v,const DIB *wanted) { if(!v|| (wanted&&wanted->bits!=32))return NULL;v->refs++;return v; }
uint32_t WAPI AVIStreamRelease(Video *v) { unsigned refs;if(!v)return 0;refs=--v->refs;if(!refs)video_free(v);return refs; }
int32_t WAPI AVIStreamGetFrameClose(Video *v) { AVIStreamRelease(v);return 0; }
void *WAPI AVIStreamGetFrame(Video *v,int32_t position) {
 int result;if(!v||position<0||position>=v->frames)return NULL;if(position==v->last)return v->dib;
 AVStream *stream=v->format->streams[v->stream];int64_t target=av_rescale_q(position,av_inv_q(v->rate),stream->time_base);
 if(stream->start_time!=AV_NOPTS_VALUE)target+=stream->start_time;
 if(position!=v->last+1) {
  if(av_seek_frame(v->format,v->stream,target,AVSEEK_FLAG_BACKWARD)<0)return NULL;
  avcodec_flush_buffers(v->codec);v->pending=v->valid=v->eof_sent=0;
 }
 for(;;) {
  result=v->pending?0:avcodec_receive_frame(v->codec,v->frame);
  if(result==0) {
   int64_t pts=v->frame->best_effort_timestamp;
   /* Empty AVI samples mean "hold previous frame". Several shipped videos
    * end with such samples, so stream length legitimately exceeds decoded
    * frames. Retain a lookahead frame instead of displaying it too early. */
   if(pts!=AV_NOPTS_VALUE&&pts>target&&v->valid){v->pending=1;v->last=position;return v->dib;}
   v->pending=0;
   v->sws=sws_getCachedContext(v->sws,v->frame->width,v->frame->height,v->frame->format,v->dib->width,v->dib->height,AV_PIX_FMT_BGRA,SWS_BILINEAR,NULL,NULL,NULL);
   if(!v->sws)return NULL;
   uint8_t *dest[4]={(uint8_t*)(v->dib+1)+(size_t)(v->dib->height-1)*v->dib->width*4,NULL,NULL,NULL};
   int stride[4]={-v->dib->width*4,0,0,0};
   if(sws_scale(v->sws,(const uint8_t *const*)v->frame->data,v->frame->linesize,0,v->frame->height,dest,stride)!=v->dib->height)return NULL;
   v->valid=1;
   if(pts==AV_NOPTS_VALUE||pts>=target){v->last=position;return v->dib;}
   continue;
  }
  if(result==AVERROR_EOF){if(v->valid){v->last=position;return v->dib;}return NULL;}
  if(result!=AVERROR(EAGAIN))return NULL;
  if(v->eof_sent){if(v->valid){v->last=position;return v->dib;}return NULL;}
  for(;;) {result=av_read_frame(v->format,v->packet);if(result<0||v->packet->stream_index==v->stream)break;av_packet_unref(v->packet);}
  if(result<0){v->eof_sent=1;if(avcodec_send_packet(v->codec,NULL)<0)return NULL;}
  else {result=avcodec_send_packet(v->codec,v->packet);av_packet_unref(v->packet);if(result<0&&result!=AVERROR(EAGAIN))return NULL;}
 }
}
void *WAPI CreateCompatibleDC(void *original) { (void)original;MemoryDC *dc=calloc(1,sizeof(*dc));if(dc)dc->magic=DC_MAGIC;return dc; }
int32_t WAPI DeleteDC(MemoryDC *dc) { if(!dc||dc->magic!=DC_MAGIC)return 0;dc->magic=0;free(dc);return 1; }
void *WAPI CreateDIBSection(MemoryDC *dc,const DIB *info,unsigned usage,void **bits,void *section,uint32_t offset) {
 (void)dc;(void)usage;if(!bits||!info||info->size<40||info->planes!=1||info->bits!=32||info->compression||info->width<=0||!info->height||info->height==INT32_MIN||section||offset)return NULL;
 unsigned height=(unsigned)abs(info->height);size_t bytes=(size_t)info->width*height*4;if(bytes>512u*1024u*1024u)return NULL;
 Bitmap *b=calloc(1,sizeof(*b));if(!b)return NULL;b->pixels=calloc(1,bytes);if(!b->pixels){free(b);return NULL;}
 b->magic=BITMAP_MAGIC;b->info=*info;b->info.image=(uint32_t)bytes;*bits=b->pixels;return b;
}
void *WAPI SelectObject(MemoryDC *dc,Bitmap *b) { if(!dc||dc->magic!=DC_MAGIC||!b||b->magic!=BITMAP_MAGIC)return NULL;Bitmap *old=dc->selected;dc->selected=b;return old; }
int32_t WAPI DeleteObject(Bitmap *b) { if(!b||b->magic!=BITMAP_MAGIC)return 0;b->magic=0;free(b->pixels);free(b);return 1; }
void *WAPI DrawDibOpen(void) { uint32_t *handle=malloc(sizeof(*handle));if(handle)*handle=0x54424444u;return handle; }
int32_t WAPI DrawDibClose(uint32_t *handle) { if(!handle||*handle!=0x54424444u)return 0;*handle=0;free(handle);return 1; }
int32_t WAPI DrawDibDraw(uint32_t *handle,MemoryDC *dc,int x,int y,int width,int height,const DIB *info,const void *pixels,int sx,int sy,int sw,int sh,unsigned flags) {
 (void)flags;if(!handle||*handle!=0x54424444u||!dc||dc->magic!=DC_MAGIC||!dc->selected||!info||!pixels||info->bits!=32||info->compression)return 0;
 Bitmap *b=dc->selected;int source_height=abs(info->height),dest_height=abs(b->info.height);
 if(x<0||y<0||width<=0||height<=0||sx<0||sy<0||sw<=0||sh<=0||x+width>b->info.width||y+height>dest_height||sx+sw>info->width||sy+sh>source_height)return 0;
 for(int row=0;row<height;row++) {
  int sr=sy+(int)((int64_t)row*sh/height),dr=y+row;
  if(info->height>0)sr=source_height-1-sr;
  if(b->info.height>0)dr=dest_height-1-dr;
  const unsigned char *src=(const unsigned char*)pixels+(size_t)sr*info->width*4;
  unsigned char *dst=b->pixels+((size_t)dr*b->info.width+x)*4;
  for(int col=0;col<width;col++)memcpy(dst+col*4,src+(sx+(int)((int64_t)col*sw/width))*4,4);
 }return 1;
}
