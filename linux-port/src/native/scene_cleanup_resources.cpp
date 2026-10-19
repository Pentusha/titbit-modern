#include "scene_cleanup_resources.h"
void ObservedTextureRecord::Reset(){width=0;format=0;memset(state.slots,0,sizeof(state.slots));texture=0;height=0;state.Reset();}
void ObservedTextureRecord::Clear(){if(texture)glDeleteTextures(1,&texture);Reset();}
bool ObservedTextureCache::Reset(int size){
 if(data){while(count>0){--count;data[size].Clear();}::operator delete(data);data=0;}
 if(size)data=(ObservedTextureRecord*)::operator new(size*sizeof(ObservedTextureRecord));
 if(!data)return false;
 count=size;while(size-- >0)data[size].Reset();return true;
}
bool ObservedTextureCache::Release(int index){if(index>=count)return false;if(data[index].state.b)--data[index].state.b;return true;}
void ObservedTextureCache::Clear(){Reset(0);}
void ObservedResourceRecord::Reset(){data20=0;memset(slots,0,sizeof(slots));textures[0]=0;textures[1]=0;textures[2]=0;data70=0;}
void ObservedResourceRecord::Clear(){if(textures[2])observed_textures.Release(textures[2]);if(textures[1])observed_textures.Release(textures[1]);if(textures[0])observed_textures.Release(textures[0]);Reset();}
bool ObservedResourceCache::Reset(int size){
 if(data){for(int i=0;i<count;++i)data[i].Clear();::operator delete(data);data=0;}
 if(size){data=(ObservedResourceRecord*)::operator new(size*sizeof(ObservedResourceRecord));memset(data,0,size*sizeof(ObservedResourceRecord));}
 count=size;used=0;return true;
}
bool ObservedResourceCache::Release(int index){if(index>=count)return false;if(!data[index].refs)return false;--data[index].refs;if(!data[index].refs)data[index].Clear();return true;}
void ObservedResourceCache::Clear(){Reset(0);}
ObservedAnimationRecord::~ObservedAnimationRecord(){Clear();}
void ObservedAnimationRecord::Clear(){
 if(buffer38){::operator delete(buffer38);buffer38=0;}
 if(buffer50){::operator delete(buffer50);buffer50=0;}
 if(buffer40){::operator delete(buffer40);buffer40=0;}
 if(buffer54){::operator delete(buffer54);buffer54=0;}
 if(buffer44){::operator delete(buffer44);buffer44=0;}
 if(buffer4c){::operator delete(buffer4c);buffer4c=0;}
 if(buffer30){::operator delete(buffer30);buffer30=0;}
 if(buffer6c){::operator delete(buffer6c);buffer6c=0;}
 if(buffer68){::operator delete(buffer68);buffer68=0;}
 if(resources){for(int i=0;i<resource_count;++i)observed_resources.Release(resources[i]);::operator delete(resources);resources=0;}
 state48=0;state3c=0;state2c=0;
}
