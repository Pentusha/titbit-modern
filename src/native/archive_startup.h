#ifndef ARCHIVE_053_BUNDLE_H
#define ARCHIVE_053_BUNDLE_H
// Exploratory archive search-path frontends. Labels are authored.
#include "engine_string.h"
#include "archive_storage.h"
struct ArchiveSearchEntryProbe {
 int kind;
 ObservedString path;
 ArchiveStorageProbe* archive;
 ArchiveSearchEntryProbe();
 ArchiveSearchEntryProbe(const ArchiveSearchEntryProbe&);
 ~ArchiveSearchEntryProbe();
 ArchiveSearchEntryProbe &operator=(const ArchiveSearchEntryProbe&);
};
// Observed four-field array prefix; matching MFC Add/SetAtGrow algorithm is
// corroboration, not proof of original class names or inheritance.
// Explicit bit guard restores this observed singleton lifetime only.
// Original class identity and hypothetical other array instances are unknown.
extern unsigned char archiveSearchDestroyed;
class ArchiveSearchArrayProbe {
public:
 ArchiveSearchEntryProbe* data;int size,capacity,grow_by;
 ArchiveSearchArrayProbe(){data=0;size=capacity=grow_by=0;}
 ~ArchiveSearchArrayProbe(){if(!(archiveSearchDestroyed&1)){archiveSearchDestroyed|=1;if(data){ArchiveSearchEntryProbe* p=data;int n=size;while(n--){p->~ArchiveSearchEntryProbe();++p;}::operator delete(data);}}}
 void SetSize(int,int=-1);
 void SetAtGrow(int index,ArchiveSearchEntryProbe entry){
  if(index>=size)SetSize(index+1,-1);
  data[index]=entry;
 }
 int Add(ArchiveSearchEntryProbe entry){int index=size;SetAtGrow(index,entry);return index;}
 ArchiveSearchEntryProbe &operator[](int index){return data[index];}
};
extern ArchiveSearchArrayProbe archiveSearchPaths;

void __stdcall ConstructArchiveEntries(ArchiveSearchEntryProbe*,int);

#endif
