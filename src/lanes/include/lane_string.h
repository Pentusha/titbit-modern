// Engine refcounted string (WTL/ATL CString layout) as a lane-local view.
// Methods carry their original address so tools/match.py binds them.
#ifndef LANE_STRING_H
#define LANE_STRING_H
extern "C" {
 __declspec(dllimport) long __stdcall InterlockedDecrement(long *);
 __declspec(dllimport) long __stdcall InterlockedIncrement(long *);
 __declspec(dllimport) int __stdcall lstrlenA(const char *);
 void *__cdecl memcpy(void *, const void *, unsigned);
}
#pragma intrinsic(memcpy)
void *__cdecl operator new(unsigned);
void __cdecl operator delete(void *);
struct LaneStringHeader { long refs; int length, capacity; };
extern char *observed_empty_string;                 // 0x4dd300
extern LaneStringHeader *observed_empty_header;     // 0x4dd304 (as the published TUs name it)
class LaneString {
public:
 char *data;
 LaneStringHeader *Header() const { return (LaneStringHeader *)data - 1; }
 void Init() { data = observed_empty_string; }
 void ReleaseInline() {
  if (Header() != observed_empty_header) {
   if (InterlockedDecrement(&Header()->refs) <= 0) delete (unsigned char *)Header();
   Init();
  }
 }
 int AllocInline(int length) {
  if (length == 0) { Init(); return 1; }
  LaneStringHeader *p = (LaneStringHeader *)::operator new(length + 13);
  if (!p) return 0;
  p->refs = 1; ((char *)(p + 1))[length] = 0; p->length = length; p->capacity = length;
  data = (char *)(p + 1); return 1;
 }
 int Allocate_00402120(int length);   // out-of-line Allocate
 void Release_00402180();             // out-of-line Release
};
#endif
