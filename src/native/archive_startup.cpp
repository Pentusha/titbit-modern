#include "archive_startup.h"
int SetArchiveRoot(const char*name){
 ArchiveSearchEntryProbe entry;
 entry.path=name;
 entry.path.MakeLower();
 entry.kind=4;
 return archiveSearchPaths.Add(entry);
}
int MountArchive(const char*name){
 ArchiveSearchEntryProbe entry;
 entry.path=name;
 entry.path.MakeLower();
 entry.kind=2;
 entry.archive=0;
 int index=archiveSearchPaths.Add(entry);
 archiveSearchPaths[index].archive=new ArchiveStorageProbe(ObservedString(name),false);
 return index;
}

ArchiveSearchEntryProbe::ArchiveSearchEntryProbe():path((const char*)0){archive=0;kind=0;}
ArchiveSearchEntryProbe::~ArchiveSearchEntryProbe(){
 switch(kind){case 2: if(archive)::operator delete(archive);break;}
}

// Observed copy order: empty string member first, archive pointer, string assignment,
// then kind. The field order and native 12-byte array stride are independent evidence.
ArchiveSearchEntryProbe::ArchiveSearchEntryProbe(const ArchiveSearchEntryProbe&other):path((const char*)0){
 archive=other.archive;path=other.path;kind=other.kind;
}
ArchiveSearchEntryProbe&ArchiveSearchEntryProbe::operator=(const ArchiveSearchEntryProbe&other){
 archive=other.archive;path=other.path;kind=other.kind;return *this;
}

inline void __stdcall DestructArchiveEntries(ArchiveSearchEntryProbe*elements,int count){
 if(!elements||!count)return;
 for(;count--;elements++)elements->~ArchiveSearchEntryProbe();
}
void ArchiveSearchArrayProbe::SetSize(int newSize,int growBy){
 if(newSize<0)return;
 if(growBy!=-1)grow_by=growBy;
 if(newSize==0){
  if(data!=0){DestructArchiveEntries(data,size);::operator delete(data);data=0;}
  size=capacity=0;
 }else if(data==0){
  data=(ArchiveSearchEntryProbe*)::operator new(newSize*sizeof(ArchiveSearchEntryProbe));
  ConstructArchiveEntries(data,newSize);size=capacity=newSize;
 }else if(newSize<=capacity){
  if(newSize>size)ConstructArchiveEntries(&data[size],newSize-size);
  else if(size>newSize)DestructArchiveEntries(&data[newSize],size-newSize);
  size=newSize;
 }else{
  int grow=grow_by;
  if(grow==0){grow=size/8;grow=(grow<4)?4:((grow>1024)?1024:grow);}
  int newMax;
  if(newSize<capacity+grow)newMax=capacity+grow;else newMax=newSize;
  ArchiveSearchEntryProbe*newData=(ArchiveSearchEntryProbe*)::operator new(newMax*sizeof(ArchiveSearchEntryProbe));
  memcpy(newData,data,size*sizeof(ArchiveSearchEntryProbe));
  if(newSize>size)ConstructArchiveEntries(&newData[size],newSize-size);
  ::operator delete(data);data=newData;size=newSize;capacity=newMax;
 }
}
