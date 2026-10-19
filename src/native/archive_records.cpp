#include "archive_storage.h"
extern "C" void* __cdecl memset(void*,int,unsigned);
#pragma intrinsic(memset)
inline void* __cdecl operator new(unsigned,void* p){return p;}
inline void __cdecl operator delete(void*,void*){}
inline void __stdcall DestructArchiveRecords(ArchiveRecordProbe*elements,int count){
 if(elements&&count)for(;count--;elements++)elements->~ArchiveRecordProbe();
}
ArchiveRecordArrayProbe::~ArchiveRecordArrayProbe(){
 if(data){DestructArchiveRecords(data,size);::operator delete(data);}
}
void __stdcall ConstructArchiveRecords(ArchiveRecordProbe*elements,int count){
 if(elements&&count){
  memset(elements,0,count*sizeof(ArchiveRecordProbe));
  for(;count--;elements++)::new((void*)elements)ArchiveRecordProbe;
 }
}
void ArchiveRecordArrayProbe::SetSize(int newSize,int growBy){
 if(newSize<0)return;
 if(growBy!=-1)grow_by=growBy;
 if(newSize==0){
  if(data!=0){DestructArchiveRecords(data,size);::operator delete(data);data=0;}
  size=capacity=0;
 }else if(data==0){
  data=(ArchiveRecordProbe*)::operator new(newSize*sizeof(ArchiveRecordProbe));
  ConstructArchiveRecords(data,newSize);size=capacity=newSize;
 }else if(newSize<=capacity){
  if(newSize>size)ConstructArchiveRecords(&data[size],newSize-size);
  else if(size>newSize)DestructArchiveRecords(&data[newSize],size-newSize);
  size=newSize;
 }else{
  int grow=grow_by;
  if(grow==0){grow=size/8;grow=(grow<4)?4:((grow>1024)?1024:grow);}
  int newMax;
  if(newSize<capacity+grow)newMax=capacity+grow;else newMax=newSize;
  ArchiveRecordProbe*newData=(ArchiveRecordProbe*)::operator new(newMax*sizeof(ArchiveRecordProbe));
  memcpy(newData,data,size*sizeof(ArchiveRecordProbe));
  if(newSize>size)ConstructArchiveRecords(&newData[size],newSize-size);
  ::operator delete(data);data=newData;size=newSize;capacity=newMax;
 }
}
