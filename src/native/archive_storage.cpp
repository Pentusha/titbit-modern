#include "archive_storage.h"
ArchiveStorageProbe::ArchiveStorageProbe(ObservedString filename,bool option)
 :archiveFilename((const char*)0),secondaryFilename((const char*)0){
 Load(filename,option);
}
