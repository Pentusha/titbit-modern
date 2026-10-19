#ifndef TITBIT_SCENE_CLEANUP_RESOURCES_H
#define TITBIT_SCENE_CLEANUP_RESOURCES_H
#include "game_audio.h"
void *__cdecl operator new(unsigned);void __cdecl operator delete(void *);
extern "C" void *__cdecl memset(void *,int,unsigned);
extern "C" __declspec(dllimport) void __stdcall glDeleteTextures(int,const unsigned *);
#pragma intrinsic(memset)
/* Authored record views follow native array strides; global cache owners have
 * only observed prefixes and are never allocated/defined by this header. */
struct ObservedTextureRecord {
 ObservedSoundState state;unsigned unknown2c,width,height,format,texture;
 void Reset();void Clear();
};
struct ObservedTextureCache {
 ObservedTextureCache();virtual ~ObservedTextureCache();int count;ObservedTextureRecord *data;
 bool Reset(int);bool Release(int);void Clear();
};
extern ObservedTextureCache observed_textures;
struct ObservedResourceRecord {
 unsigned slots[8];unsigned data20;int refs;unsigned unknown28;
 unsigned char reserved2c[0x60-0x2c];int textures[3];unsigned unknown6c,data70;
 void Reset();void Clear();
};
struct ObservedResourceCache {
 ObservedResourceCache();virtual ~ObservedResourceCache();ObservedResourceRecord *data;int count,used,allocation_step;
 bool Reset(int);bool Release(int);void Clear();
};
extern ObservedResourceCache observed_resources;
struct ObservedAnimationRecord {
 unsigned char reserved0[0x2c];unsigned state2c;void *buffer30;unsigned unknown34;
 void *buffer38;unsigned state3c;void *buffer40,*buffer44;unsigned state48;
 void *buffer4c,*buffer50,*buffer54;unsigned char reserved58[16];void *buffer68,*buffer6c;
 int *resources;int resource_count;
 ~ObservedAnimationRecord();void Clear();
};
extern ObservedAnimationRecord *observed_animation_data;
extern int observed_animation_count;
typedef char TextureStride64[(sizeof(ObservedTextureRecord)==64)?1:-1];
typedef char ResourceStride116[(sizeof(ObservedResourceRecord)==116)?1:-1];
typedef char AnimationStride120[(sizeof(ObservedAnimationRecord)==120)?1:-1];
#endif
