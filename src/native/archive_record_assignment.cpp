#include "archive_storage.h"
ArchiveRecordProbe&ArchiveRecordProbe::operator=(const ArchiveRecordProbe&other){
 name=other.name;secondaryName=other.secondaryName;
 compression=other.compression;offset=other.offset;decodedSize=other.decodedSize;
 packedSize=other.packedSize;cached=other.cached;return*this;
}
