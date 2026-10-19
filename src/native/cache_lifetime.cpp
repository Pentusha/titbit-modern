#include "scene_cleanup_resources.h"
ObservedTextureCache::ObservedTextureCache(){}
ObservedTextureCache::~ObservedTextureCache(){Reset(0);}
ObservedResourceCache::ObservedResourceCache(){data=0;count=0;allocation_step=10;}
ObservedResourceCache::~ObservedResourceCache(){Reset(0);}
