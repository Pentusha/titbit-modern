/* The original system-speaker mixer becomes an application-output mixer.
 * Volume/mute affect actual PCM callbacks, without changing the Linux desktop
 * or another application's audio. Supported by MasterOutputVolume's API. */
#include "titbit_platform.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__i386__)
#define WAPI __attribute__((stdcall))
#else
#define WAPI
#endif
extern int32_t WAPI PostMessageA(void *,uint32_t,uint32_t,int32_t);
typedef struct {uint32_t magic;void *window;} Mixer;
typedef struct {uint32_t size,line,type,count,item_size;void *items;} LineControls;
typedef struct {uint32_t size,control,channels,multiple,item_size;void *items;} Details;
#define MIXER_MAGIC 0x54424d58u
static int valid(Mixer *m){return m&&m->magic==MIXER_MAGIC;}
unsigned WAPI mixerGetNumDevs(void){return tb_audio_devices()>0?1:0;}
uint32_t WAPI mixerGetID(void *wave,uint32_t *out,uint32_t flags){(void)flags;if(!wave||!out||!mixerGetNumDevs())return 5;*out=0;return 0;}
uint32_t WAPI mixerOpen(Mixer **out,unsigned id,uintptr_t callback,uintptr_t instance,uint32_t flags){(void)instance;if(!out||id||!mixerGetNumDevs()||(flags&~0x10000u))return 11;Mixer *m=calloc(1,sizeof(*m));if(!m)return 7;m->magic=MIXER_MAGIC;m->window=flags&0x10000u?(void*)callback:NULL;*out=m;return 0;}
uint32_t WAPI mixerClose(Mixer *m){if(!valid(m))return 5;m->magic=0;free(m);return 0;}
uint32_t WAPI mixerGetLineInfoA(Mixer *m,uint32_t *line,uint32_t flags){
 if(!valid(m)||!line||line[0]<168)return 11;
 if((flags&15)==3&&line[6]!=4)return 1024;
 if((flags&15)==2&&line[3]!=1)return 1024;
 uint32_t size=line[0];memset(line,0,168);line[0]=size;line[3]=1;line[4]=1;line[6]=4;line[7]=2;line[9]=2;
 memcpy((char*)line+40,"Titbit",7);memcpy((char*)line+56,"Titbit application output",26);return 0;
}
static void fill_control(uint32_t *control,uint32_t id){
 memset(control,0,148);control[0]=148;control[1]=id;control[2]=id==1?0x50030001u:0x20010002u;control[3]=1;
 memcpy((char*)control+20,id==1?"Volume":"Mute",id==1?7:5);memcpy((char*)control+36,id==1?"Application volume":"Application mute",id==1?19:17);
 control[25]=0;control[26]=id==1?65535:1;control[31]=id==1?65536:2;
}
uint32_t WAPI mixerGetLineControlsA(Mixer *m,LineControls *request,uint32_t flags){
 if(!valid(m)||!request||request->size<sizeof(*request)||request->line!=1||request->item_size<148||!request->items)return 11;
 unsigned kind=flags&15;if(kind==2){if(!request->count)return 11;if(request->type==0x50030001u)fill_control(request->items,1);else if(request->type==0x20010002u)fill_control(request->items,2);else return 1025;return 0;}
 if(kind==1){if(!request->count||(request->type!=1&&request->type!=2))return 1025;fill_control(request->items,request->type);return 0;}
 if(kind==0){if(request->count<2)return 11;fill_control(request->items,1);fill_control((uint32_t*)((char*)request->items+request->item_size),2);return 0;}return 11;
}
uint32_t WAPI mixerGetControlDetailsA(Mixer *m,Details *details,uint32_t flags){
 if(!valid(m)||!details||details->size<sizeof(*details)||!details->channels||details->channels>2||details->multiple||details->item_size!=4||!details->items||(flags&15))return 11;
 if(details->control!=1&&details->control!=2)return 1025;
 uint32_t value=details->control==1?tb_audio_master_get():(unsigned)tb_audio_mute_get();
 for(unsigned i=0;i<details->channels;i++)((uint32_t*)details->items)[i]=value;
 return 0;
}
uint32_t WAPI mixerSetControlDetails(Mixer *m,const Details *details,uint32_t flags){
 if(!valid(m)||!details||details->size<sizeof(*details)||!details->channels||details->channels>2||details->multiple||details->item_size!=4||!details->items||(flags&15))return 11;
 const uint32_t *values=details->items;for(unsigned i=1;i<details->channels;i++)if(values[i]!=values[0])return 8;
 if(details->control==1){if(!tb_audio_master_set(values[0]))return 11;}else if(details->control==2){if(values[0]>1)return 11;tb_audio_mute_set((int)values[0]);}else return 1025;
 if(m->window)PostMessageA(m->window,0x3d1,(uint32_t)(uintptr_t)m,(int32_t)details->control);
 return 0;
}
