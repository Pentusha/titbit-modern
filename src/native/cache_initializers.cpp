#include "scene_cleanup_resources.h"
inline void *__cdecl operator new(unsigned,void *p){return p;}
extern "C" int __cdecl atexit(void (__cdecl *)(void));
void __cdecl ConstructTextureCacheGlobal(){::new((void*)&observed_textures) ObservedTextureCache;}
void __cdecl DestroyTextureCacheGlobal(){observed_textures.~ObservedTextureCache();}
void __cdecl RegisterTextureCacheDestructor(){atexit(DestroyTextureCacheGlobal);}
void __cdecl InitializeTextureCache(){ConstructTextureCacheGlobal();RegisterTextureCacheDestructor();}
void __cdecl ConstructResourceCacheGlobal(){::new((void*)&observed_resources) ObservedResourceCache;}
void __cdecl DestroyResourceCacheGlobal(){observed_resources.~ObservedResourceCache();}
void __cdecl RegisterResourceCacheDestructor(){atexit(DestroyResourceCacheGlobal);}
void __cdecl InitializeResourceCache(){ConstructResourceCacheGlobal();RegisterResourceCacheDestructor();}
