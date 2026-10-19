// Archive directory: name of a record by index (0x401950, used by the VFS).
// View of ArchiveStorageProbe (src/native/archive_storage.h).
#include "e40_string.h"

struct ArchiveRecord { ObservedString name, secondaryName; unsigned compression, offset, decodedSize, packedSize; void *cached; };
class ArchiveView {
public:
 unsigned signature;
 int recordCount;
 unsigned directoryBytes, totalBytes;
 ObservedString archiveFilename, secondaryFilename;
 ArchiveRecord *records;              // +0x18
 int size, capacity, grow_by;
 ObservedString Name_00401950(int index);
};

ObservedString ArchiveView::Name_00401950(int index) {
 return records[index].name;
}
