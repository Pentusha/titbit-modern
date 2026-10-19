#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Virtual file over the archive search paths (0x4203d0-0x420ab0).
// Search entry kinds: 1 = folder (name substring), 2 = mounted archive, 4 = root directory.
#include "../../native/archive_startup.h"
#include <stdio.h>
extern "C" {
 __declspec(dllimport) char *__stdcall CharNextA(const char *);
 int __cdecl memcmp(const void *, const void *, unsigned);
 long __cdecl _filelength(int);
}
#pragma intrinsic(memcmp)
void __cdecl FormatArchiveString(ObservedString *, const char *, ...);
const char *__cdecl FindChar_0040c3a0(const char *text, char ch);

class ArchiveView : public ArchiveStorageProbe {
public:
 ObservedString GetName_00401950(int index);
 unsigned GetSize_00401b80(int index);
};

enum { SOURCE_FOLDER = 1, SOURCE_ARCHIVE = 2, SOURCE_ROOT = 4 };

class VfsFile {  // 0x4203d0 (was VfsFile_004203d0)
public:
 ObservedString name;
 unsigned position;
 void *handle;          // FILE* (folder/root) or record bytes (archive)
 unsigned size;
 int source;            // index into archiveSearchPaths
 int kind;
 VfsFile();
 ~VfsFile();
 int Locate_00420420(const char *path);
 void Open_004206b0(const char *path);
 unsigned Read_004209b0(char *buffer, unsigned count);
 void Close_00420a60();
};

VfsFile::VfsFile() { handle = 0; size = 0; source = -1; kind = 0; }
VfsFile::~VfsFile() {}

// NONMATCHING: faithful translation
int VfsFile::Locate_00420420(const char *path) {
 int count = archiveSearchPaths.size;
 if (count == 0) return 0;
 for (int i = 0; i < count; i++) {
  ArchiveSearchEntryProbe *entry = &archiveSearchPaths[i];
  switch (entry->kind) {
  case SOURCE_FOLDER: {
   const char *folder = entry->path;
   const char *found = folder;                       // empty path: WTL Find returns index 0
   int length = lstrlenA(path);
   if (length != 0) {
    found = 0;
    const char *p = folder;
    while (FindChar_0040c3a0(p, *path)) {
     if (memcmp(p, path, length) == 0) { found = p; break; }
     p = CharNextA(p);
    }
   }
   if (found && found - (const char *)entry->path != -1) { source = i; return 1; }
  } break;
  case SOURCE_ARCHIVE:
   if (entry->archive->Find(ObservedString(path)) != -1) { source = i; return 1; }
   break;
  case SOURCE_ROOT: {
   ObservedString full;
   FormatArchiveString(&full, "%s%s", (const char *)entry->path, path);
   FILE *file = fopen(full, "rb");
   if (file) { fclose(file); source = i; return 1; }
  } break;
  }
 }
 return 0;
}

// NONMATCHING: faithful translation
void VfsFile::Open_004206b0(const char *path) {
 if (!Locate_00420420(path)) return;
 if (source == -1) return;
 ArchiveSearchEntryProbe *entry = &archiveSearchPaths[source];
 switch (entry->kind) {
 case SOURCE_FOLDER: {
  FILE *file = fopen(path, "rb");
  if (!file) return;
  handle = file;
  size = _filelength(file->_file);
  position = 0;
  kind = SOURCE_FOLDER;
  name = path;
 } break;
 case SOURCE_ARCHIVE: {
  ArchiveView *archive = (ArchiveView *)entry->archive;
  int index = archive->Find(ObservedString(path));
  if (index == -1) return;
  handle = archive->LoadRecord(index);
  size = archive->GetSize_00401b80(index);
  name = archive->GetName_00401950(index);
  position = 0;
  kind = SOURCE_ARCHIVE;
 } break;
 case SOURCE_ROOT: {
  ObservedString full;
  FormatArchiveString(&full, "%s%s", (const char *)entry->path, path);
  FILE *file = fopen(full, "rb");
  if (!file) return;
  setvbuf(file, 0, _IOFBF, 0x10000);
  handle = file;
  size = _filelength(file->_file);
  position = 0;
  kind = SOURCE_ROOT;
  name = path;
 } break;
 }
}

// NONMATCHING: faithful translation
unsigned VfsFile::Read_004209b0(char *buffer, unsigned count) {
 if (size == 0) return 0;
 if (handle == 0) return 0;
 switch (kind) {
 case SOURCE_FOLDER: return fread(buffer, 1, count, (FILE *)handle);
 case SOURCE_ARCHIVE:
  if (position > size) return 0;
  if (position + count > size) count = size - position;
  memcpy(buffer, (char *)handle + position, count);
  position += count;
  return count;
 case SOURCE_ROOT: return fread(buffer, 1, count, (FILE *)handle);
 }
 return 0;
}

// NONMATCHING: faithful translation
void VfsFile::Close_00420a60() {
 if (size == 0 || handle == 0) return;
 switch (kind) {
 case SOURCE_ARCHIVE:
  ::operator delete(handle);
  size = 0; position = 0; handle = 0;
  return;
 case SOURCE_FOLDER:
 case SOURCE_ROOT:
  fclose((FILE *)handle);
  size = 0; position = 0; handle = 0;
  return;
 }
}

// Adds a folder search entry (kind 1); SetArchiveRoot (kind 4) is 0x420e60.
int AddArchiveFolder_00420ab0(const char *name) {
 ArchiveSearchEntryProbe entry;
 entry.path = name;
 entry.path.MakeLower();
 entry.kind = SOURCE_FOLDER;
 return archiveSearchPaths.Add(entry);
}
