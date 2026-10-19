#ifndef ARCHIVE_054_STORAGE_H
#define ARCHIVE_054_STORAGE_H
#include "engine_string.h"
// Authored field labels. Size40 is independently given by Mount's new(0x28).
// The four on-disk header fields are left uninitialized until Load reads them.
struct ArchiveRecordProbe {
 ObservedString name,secondaryName;
 unsigned compression,offset,decodedSize,packedSize;
 void *cached;
 ArchiveRecordProbe(){}
 ArchiveRecordProbe(const ArchiveRecordProbe&);
 ArchiveRecordProbe&operator=(const ArchiveRecordProbe&);
 ~ArchiveRecordProbe(){}
};
class ArchiveRecordArrayProbe {
public:
 ArchiveRecordProbe *data;
 int size,capacity,grow_by;
 ArchiveRecordArrayProbe(){data=0;size=capacity=grow_by=0;}
 ~ArchiveRecordArrayProbe();
 void SetSize(int,int=-1);
 void SetAtGrow(int index,ArchiveRecordProbe entry){if(index>=size)SetSize(index+1,-1);data[index]=entry;}
 int Add(ArchiveRecordProbe entry){int index=size;SetAtGrow(index,entry);return index;}
};
class ArchiveStorageProbe {
 unsigned signature;
 int recordCount;
 unsigned directoryBytes,totalBytes;
 ObservedString archiveFilename,secondaryFilename;
 ArchiveRecordArrayProbe records;
public:
 ArchiveStorageProbe(ObservedString,bool);
 int Load(ObservedString,bool);
 char *LoadRecord(int);
 int Find(ObservedString);
};
#endif
