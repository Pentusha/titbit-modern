// Archive record copy, lookup by name and record payload loading (0x4015e0-0x401b80).
#define OBSERVED_STRING_INLINE_COPY
#define OBSERVED_STRING_RELEASE_EXTERNAL
#include "../../native/archive_storage.h"
#include <stdio.h>
#include <stdlib.h>
extern "C" int __stdcall uncompress(unsigned char *, unsigned long *, const unsigned char *, unsigned long);
extern "C" __declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);
void __cdecl PickingTrace(const char *, ...);

// NONMATCHING 0x4015e0: inline string copy register allocation differs
ArchiveRecordProbe::ArchiveRecordProbe(const ArchiveRecordProbe &other)
 : name(other.name), secondaryName(other.secondaryName) {
 compression = other.compression; offset = other.offset; decodedSize = other.decodedSize;
 packedSize = other.packedSize; cached = other.cached;
}

class ArchiveStorageView {
public:
 unsigned signature; int recordCount; unsigned directoryBytes, totalBytes;
 ObservedString archiveFilename, secondaryFilename;
 ArchiveRecordArrayProbe records;
};

static inline int IsEmptyName(const ObservedString &s) { return s.GetLength() == 0; }

char *ArchiveStorageProbe::LoadRecord(int index) {
 ArchiveStorageView *self = (ArchiveStorageView *)this;
 if (self->records.size < index) { PickingTrace("number too big !!!"); return 0; }
 if (IsEmptyName(self->archiveFilename) < 0) { PickingTrace("no name"); return 0; }
 unsigned offset = self->records.data[index].offset;
 FILE *file = fopen(self->archiveFilename, "rb");
 if (file == 0) { fclose(file); return 0; }
 fseek(file, offset, SEEK_SET);
 char *out = (char *)malloc(self->records.data[index].decodedSize);
 unsigned long outLength = self->records.data[index].decodedSize;
 if (self->records.data[index].compression != 0) {
  char *packed = (char *)malloc(self->records.data[index].packedSize);
  unsigned packedLength = self->records.data[index].packedSize;
  fread(packed, packedLength, 1, file);
  fclose(file);
  int result = uncompress((unsigned char *)out, &outLength, (unsigned char *)packed, packedLength);
  if (result == -4) { PickingTrace("not enough memory"); return 0; }
  if (result == -5) { PickingTrace("enough room in the output buffer"); return 0; }
  if (result == -3) { PickingTrace("input data was corrupted"); return 0; }
  free(packed);
  self->records.data[index].cached = out;
  return out;
 }
 fread(out, outLength, 1, file);
 fclose(file);
 self->records.data[index].cached = out;
 return out;
}

// NONMATCHING 0x401a50: loop element address computed in a different register
int ArchiveStorageProbe::Find(ObservedString name) {
 ArchiveStorageView *self = (ArchiveStorageView *)this;
 name.MakeLower();
 for (int i = self->records.size; i > 0;) {
  --i;
  if (lstrcmpA(self->records.data[i].name, name) == 0) return i;
 }
 return -1;
}
