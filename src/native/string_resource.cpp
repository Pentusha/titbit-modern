/* Authored resource-string wrapper. Cache the instance before the Win32 call;
 * the native EAX result is consumed by the growing-buffer caller. */
extern "C" __declspec(dllimport) int __stdcall LoadStringA(void *,unsigned,char *,int);
#include "engine_module_resource.h"
int __stdcall ObservedLoadString(unsigned id,char *buffer,unsigned capacity) {
 void *instance=observed_resource_instance;
 int length=LoadStringA(instance,id,buffer,capacity);
 if (!length) *buffer=0;
 return length;
}
