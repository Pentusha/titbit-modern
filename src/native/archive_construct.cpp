#include "archive_startup.h"
extern "C" void* __cdecl memset(void*,int,unsigned);
#pragma intrinsic(memset)
inline void* __cdecl operator new(unsigned,void* p){return p;}
inline void __cdecl operator delete(void*,void*){}
void __stdcall ConstructArchiveEntries(ArchiveSearchEntryProbe*elements,int count){
 if(elements && count){
  memset((void*)elements,0,count*sizeof(ArchiveSearchEntryProbe));
  for(;count--;elements++)::new((void*)elements)ArchiveSearchEntryProbe;
 }
}
