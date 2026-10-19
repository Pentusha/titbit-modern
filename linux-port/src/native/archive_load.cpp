#include "archive_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <stdarg.h>
extern const char archiveReadMode[],archiveStringFormat[],archiveSignature[];
extern const char archiveCorruptMessage[],archiveWrongSizeMessage[];
const char*__cdecl FindArchiveSubstring(const char*,const char*);
void __cdecl PickingTrace(const char*,...);
void __cdecl FormatArchiveString(ObservedString*string,const char*format,...){
 va_list args;va_start(args,format);string->FormatV(format,args);va_end(args);
}
int ArchiveStorageProbe::Load(ObservedString filename,bool option){
 archiveFilename=filename;
 FILE*file=fopen(filename,archiveReadMode);
 if(file==0){fclose(file);return 0;}
 long actualSize=_filelength(file->_file);
 fread(&signature,4,1,file);
 ObservedString signatureText;
 FormatArchiveString(&signatureText,archiveStringFormat,this);
 const char*found=FindArchiveSubstring(signatureText,archiveSignature);
 if(found==0||found-(const char*)signatureText!=0){
  fclose(file);PickingTrace(archiveCorruptMessage);return 0;
 }
 fread(&recordCount,4,1,file);fread(&directoryBytes,4,1,file);fread(&totalBytes,4,1,file);
 if(actualSize!=totalBytes){fclose(file);PickingTrace(archiveWrongSizeMessage);return 0;}
 char*directory=(char*)malloc(directoryBytes-12);
 fread(directory,directoryBytes-12,1,file);
 char*cursor=directory;
 ArchiveRecordProbe record;
 int count=recordCount;
 if(count>0)do{
  FormatArchiveString(&record.name,archiveStringFormat,cursor);
  cursor+=record.name.GetLength()+1;record.name.MakeLower();
  FormatArchiveString(&record.secondaryName,archiveStringFormat,cursor);
  cursor+=record.secondaryName.GetLength()+1;record.secondaryName.MakeLower();
  record.compression=*(unsigned*)cursor;cursor+=4;
  record.offset=*(unsigned*)cursor;cursor+=4;
  record.decodedSize=*(unsigned*)cursor;cursor+=4;
  record.packedSize=*(unsigned*)cursor;cursor+=4;
  records.Add(record);
 }while(--count);
 fclose(file);free(directory);return 1;
}
