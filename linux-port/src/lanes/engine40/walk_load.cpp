// Walk-area file loader (0x408440): "<tag><version 1><vertex count><segment
// count><floor[]><ceiling[]><start segment>" followed by the segments.
#include "e40_string_buf.h"
extern "C" __declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);

class VfsFile {  // 0x4203d0 (was VfsFile_004203d0)
public:
 ObservedString name;
 unsigned position;
 void *handle;
 unsigned size;
 int source, kind;
 VfsFile();
 ~VfsFile();
 int Locate_00420420(const char *path);
 void Open_004206b0(const char *path);
 unsigned Read_004209b0(void *buffer, unsigned count);
 void Close_00420a60();
};

struct AreaVec { float x, y, z; };
struct AreaEdge_00406770 {           // 0x38 bytes
 int index;
 AreaVec *floor0, *floor1, *ceil0, *ceil1;
 AreaVec control0, control1;
 int flags;
 AreaEdge_00406770 *next, *prev;
};

struct WalkArea_00407ff0 {
 char name[0x2c];
 AreaVec floor[1000];                // 0x2c
 AreaVec ceiling[1000];              // 0x2f0c
 int vertexCount;                    // 0x5dec
 AreaEdge_00406770 edges[1000];      // 0x5df0
 int segmentCount;                   // 0x138b0
 AreaEdge_00406770 *cursor;          // 0x138b4
 bool Load_00408440(ObservedString file);
};

bool WalkArea_00407ff0::Load_00408440(ObservedString file) {
 ObservedString text, tag, unused1, unused2;
 try {
  VfsFile in;
  if (!in.Locate_00420420(file)) return false;
  in.Open_004206b0(file);
  int version = 1;
  in.Read_004209b0(tag.GetBuffer(4), 4);
  tag.ReleaseBuffer();
  lstrcmpA(tag, "SPLP");                 // result ignored
  in.Read_004209b0(&version, 4);
  if (version != 1) return false;
  in.Read_004209b0(&vertexCount, 4);
  in.Read_004209b0(&segmentCount, 4);
  in.Read_004209b0(floor, vertexCount * sizeof(AreaVec));
  in.Read_004209b0(ceiling, vertexCount * sizeof(AreaVec));
  int index;
  in.Read_004209b0(&index, 4);
  cursor = &edges[index];
  for (int i = 0; i < segmentCount; i++) {
   in.Read_004209b0(&edges[i].index, 4);
   in.Read_004209b0(&edges[i].control0, sizeof(AreaVec));
   in.Read_004209b0(&edges[i].control1, sizeof(AreaVec));
   in.Read_004209b0(&edges[i].flags, 4);
   in.Read_004209b0(&index, 4);
   edges[i].floor0 = &floor[index];
   in.Read_004209b0(&index, 4);
   edges[i].floor1 = &floor[index];
   in.Read_004209b0(&index, 4);
   edges[i].next = &edges[index];
   in.Read_004209b0(&index, 4);
   edges[i].prev = &edges[index];
   in.Read_004209b0(&index, 4);
   edges[i].ceil0 = &ceiling[index];
   in.Read_004209b0(&index, 4);
   edges[i].ceil1 = &ceiling[index];
  }
  in.Close_00420a60();
  return true;
 } catch (...) {
 }
 return false;
}
