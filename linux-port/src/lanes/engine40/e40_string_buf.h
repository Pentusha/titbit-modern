// Minimal view of the engine string (WTL CString, same layout and class name as
// lanes/engine41/e41_string.h) for units in which GetBuffer/ReleaseBuffer are
// expanded inline. Do not include together with e41_string.h.
#ifndef E40_STRING_BUF_H
#define E40_STRING_BUF_H
extern "C" {
 __declspec(dllimport) long __stdcall InterlockedDecrement(long *);
 __declspec(dllimport) int __stdcall lstrlenA(const char *);
 __declspec(dllimport) int __stdcall LoadStringA(void *, unsigned, char *, int);
 void *__cdecl memcpy(void *, const void *, unsigned);
}
#pragma intrinsic(memcpy)
void *__cdecl operator new(unsigned);
void __cdecl operator delete(void *);
struct ObservedStringHeader { long refs; int length, capacity; };
extern char *observed_empty_string;                 // 0x4dd300
extern ObservedStringHeader *observed_empty_header; // 0x4dd304
#ifdef TITBIT_SOURCE_LINK
#include "../../native/engine_module_resource.h"
#define g_00502328 observed_resource_instance
#else
extern "C" void *g_00502328; // gameModule resource instance at +8
#endif

class ObservedString {
public:
 char *data;
 ObservedString() { Init(); }
 ObservedString(const ObservedString &other);       // 0x402040
 ~ObservedString() {
  if (Header() != observed_empty_header && InterlockedDecrement(&Header()->refs) <= 0)
   delete (unsigned char *)Header();
 }
 operator const char *() const { return data; }
 ObservedStringHeader *Header() const { return (ObservedStringHeader *)data - 1; }
 int LoadString(unsigned id);
private:
 void Init() { data = observed_empty_string; }
 int Allocate(int length);                          // 0x402120
 void Release();                                    // 0x402180
 static void __stdcall Release(ObservedStringHeader *header);   // 0x406700
public:
 __forceinline int AllocateInline(int length) {
  if (length == 0) { Init(); return 1; }
  ObservedStringHeader *p = (ObservedStringHeader *)::operator new(length + 13);
  if (!p) return 0;
  p->refs = 1; ((char *)(p + 1))[length] = 0; p->length = length; p->capacity = length;
  data = (char *)(p + 1);
  return 1;
 }
 char *GetBuffer(int min_length) {
  if (Header()->refs > 1 || min_length > Header()->capacity) {
   ObservedStringHeader *old = Header();
   int old_length = old->length;
   if (min_length < old_length) min_length = old_length;
   if (!Allocate(min_length)) return data;
   memcpy(data, (char *)(old + 1), old_length + 1);
   Header()->length = old_length;
   Release(old);
  }
  return data;
 }
 __forceinline char *GetBufferExpanded(int min_length) {
  if (Header()->refs > 1 || min_length > Header()->capacity) {
   ObservedStringHeader *old = Header();
   int old_length = old->length;
   if (min_length < old_length) min_length = old_length;
   if (!AllocateInline(min_length)) return data;
   memcpy(data, (char *)(old + 1), old_length + 1);
   Header()->length = old_length;
   Release(old);
  }
  return data;
 }
 void CopyBeforeWrite();                           // 0x4021c0
 __forceinline void CopyBeforeWriteExpanded() {
  if (Header()->refs > 1) {
   ObservedStringHeader *old = Header();
   Release();
   if (Allocate(old->length)) memcpy(data, (char *)(old + 1), old->length + 1);
  }
 }
 __forceinline void ReleaseBufferExpanded(int new_length = -1) {
  CopyBeforeWriteExpanded();
  if (new_length == -1) new_length = lstrlenA(data);
  Header()->length = new_length;
  data[new_length] = 0;
 }
 void ReleaseBuffer(int new_length = -1) {
  CopyBeforeWrite();                                // out of line: 0x4021c0
  if (new_length == -1) new_length = lstrlenA(data);
  Header()->length = new_length;
  data[new_length] = 0;
 }
 private:
 static int __stdcall LoadText(unsigned id, char *buffer, int size) {
  int length = LoadStringA(g_00502328, id, buffer, size);
  if (length == 0) buffer[0] = 0;
  return length;
 }
 void AssignCopy(int length, const char *text) {
  if (Header()->refs > 1 || length > Header()->capacity) {
   Release();
   if (!Allocate(length)) return;
  }
  memcpy(data, text, length);
  Header()->length = length;
  data[length] = 0;
 }
};

#endif
