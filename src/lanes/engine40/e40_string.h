// Copy of lanes/engine41/e41_string.h for the 0x40xxxx units, which saw
// operator delete declared WITHOUT throw() (EH state stores around inline
// string destructors depend on it; e.g. 0x40e3e0 matches only this way).
// WTL CString (atlmisc.h) as the engine compiled it, under the name the
// published units bind (ObservedString). Every member is inline as in WTL;
// VC6's inline budget decides per call site whether the body is expanded or the
// out-of-line COMDAT copy is called, and those copies are bound in
// lanes/symbols.json / analysis bindings:
//   copy ctor 0x402040, ctor(const char*) 0x409400, dtor 0x402300,
//   AllocBuffer 0x402120, Release 0x402180, static Release 0x406700,
//   AllocBeforeWrite 0x402260, AssignCopy 0x402330, operator=(const char*) 0x403d90,
//   ConcatCopy 0x40e8f0, ConcatInPlace 0x4075a0, += text 0x409960,
//   += string 0x40e1d0, operator+(text, string) 0x40e070, Format 0x402630.
#ifndef E40_STRING_H
#define E40_STRING_H
extern "C" {
 __declspec(dllimport) long __stdcall InterlockedDecrement(long *);
 __declspec(dllimport) long __stdcall InterlockedIncrement(long *);
 __declspec(dllimport) int __stdcall lstrlenA(const char *);
 __declspec(dllimport) char *__stdcall CharLowerA(char *);
 __declspec(dllimport) char *__stdcall CharNextA(const char *);
 __declspec(dllimport) int __stdcall LoadStringA(void *, unsigned, char *, int);
 void *__cdecl memcpy(void *, const void *, unsigned);
 void *__cdecl memmove(void *, const void *, unsigned);
}
#pragma intrinsic(memcpy)
void *__cdecl operator new(unsigned);
void __cdecl operator delete(void *);
struct ObservedStringHeader { long refs; int length, capacity; };
extern char *observed_empty_string;                 // 0x4dd300
extern ObservedStringHeader *observed_empty_header; // 0x4dd304
namespace WTL { class CAppModule; }
extern WTL::CAppModule gameModule;
#define observed_resource_instance (*(void *const *)((const char *)&gameModule + 8))
int __stdcall ObservedLoadString(unsigned, char *, unsigned);

class ObservedString {
public:
 char *data;
 ObservedString() { Init(); }
#ifdef TITBIT_SOURCE_LINK
 ObservedString(const ObservedString &other);
#else
#ifdef E40_COPY_CALL
 ObservedString(const ObservedString &other);                       // 0x402040, out of line in this unit
#else
 ObservedString(const ObservedString &other) {
  if (other.Header()->refs >= 0) { data = other.data; InterlockedIncrement(&Header()->refs); }
  else { Init(); Assign(other.data); }
 }
#endif
#endif
 ObservedString(const char *text) {
  Init();
  if (text && (unsigned short)((unsigned long)text >> 16) == 0) {
#ifdef E40_LOADSTRING_CALL
   /* units where VC6 kept WTL CString::LoadString out of line (0x406520) */
   LoadString((unsigned)text & 0xffff);
  } else {
#else
   unsigned id = (unsigned)text & 0xffff;
   char temporary[256];
   int length = LoadStringA(observed_resource_instance, id, temporary, 256);
   if (!length) temporary[0] = 0;
   if (256 - length > 2) Assign(temporary);
   else {
    int capacity = 256;
    do { capacity += 256; length = ObservedLoadString(id, GetBuffer(capacity - 1), capacity); }
    while (capacity - length <= 2);
    ReleaseBuffer(-1);
   }
  } else {
#endif
   int length = SafeStrlen(text);
   if (length && Allocate(length)) memcpy(data, text, length);
  }
 }
 ~ObservedString() {
  if (Header() != observed_empty_header && InterlockedDecrement(&Header()->refs) <= 0)
   delete (unsigned char *)Header();
 }
 int GetLength() const { return Header()->length; }
 int LoadString(unsigned id);                                       // 0x406520
 operator const char *() const { return data; }
 ObservedString &operator=(const ObservedString &other) {
  if (data != other.data) {
   if ((Header()->refs < 0 && Header() != observed_empty_header) || other.Header()->refs < 0)
    AssignCopy(other.Header()->length, other.data);
   else { Release(); data = other.data; InterlockedIncrement(&Header()->refs); }
  }
  return *this;
 }
 ObservedString &operator=(const char *text) { return Assign(text); }
 ObservedString &Assign(const char *text) { AssignCopy(SafeStrlen(text), text); return *this; }
 ObservedString &operator+=(const char *text) { ConcatInPlace(SafeStrlen(text), text); return *this; }
 ObservedString &operator+=(const ObservedString &other) { ConcatInPlace(other.Header()->length, other.data); return *this; }
 friend ObservedString __stdcall operator+(const char *text, const ObservedString &other);
 int __cdecl Format(const char *format, ...);
 char *GetBuffer(int);
 void ReleaseBuffer(int);
 void CopyBeforeWrite() {
  if (Header()->refs > 1) {
   ObservedStringHeader *old = Header();
   Release();
   if (Allocate(old->length)) memcpy(data, (char *)(old + 1), old->length + 1);
  }
 }
 void MakeLower() { CopyBeforeWrite(); CharLowerA(data); }
 static const char *__cdecl _cstrrchr(const char *text, char ch);   // 0x407b50
 int ReverseFind(char ch) const { const char *p = _cstrrchr(data, ch); return p == 0 ? -1 : (int)(p - data); }
 ObservedString Mid(int first) const;                               // 0x407b80
 ObservedString Mid(int first, int count) const;                    // 0x407bb0
 ObservedString Left(int count) const;                              // 0x4076a0
 friend ObservedString __stdcall operator+(const ObservedString &a, const ObservedString &b);   // 0x40c3e0
 static char *__cdecl _cstrstr(const char *text, const char *find);  // 0x401fb0
 int Replace(const char *old_text, const char *new_text) {
  int source_length = SafeStrlen(old_text);
  if (source_length == 0) return 0;
  int replacement_length = SafeStrlen(new_text);
  int count = 0;
  char *start = data, *end = data + Header()->length, *target;
  while (start < end) {
   while ((target = _cstrstr(start, old_text)) != 0) { count++; start = target + source_length; }
   start += lstrlenA(start) + 1;
  }
  if (count > 0) {
   CopyBeforeWrite();
   int old_length = Header()->length;
   int new_length = old_length + (replacement_length - source_length) * count;
   if (Header()->capacity < new_length || Header()->refs > 1) {
    ObservedStringHeader *old = Header();
    char *text = data;
    if (!Allocate(new_length)) return -1;
    memcpy(data, text, old->length);
    Release(old);
   }
   start = data; end = data + Header()->length;
   while (start < end) {
    while ((target = _cstrstr(start, old_text)) != 0) {
     int balance = old_length - (target - data + source_length);
     memmove(target + replacement_length, target + source_length, balance);
     memcpy(target, new_text, replacement_length);
     start = target + replacement_length;
     start[balance] = 0;
     old_length += replacement_length - source_length;
    }
    start += lstrlenA(start) + 1;
   }
   Header()->length = new_length;
  }
  return count;
 }
 int Remove(char ch) {
  CopyBeforeWrite();
  char *source = data, *dest = data, *end = data + Header()->length;
  while (source < end) {
   if (*source != ch) { *dest = *source; dest = CharNextA(dest); }
   source = CharNextA(source);
  }
  *dest = 0;
  int count = source - dest;
  Header()->length -= count;
  return count;
 }
 ObservedStringHeader *Header() const { return (ObservedStringHeader *)data - 1; }
private:
 void Init() { data = observed_empty_string; }
 static int __stdcall SafeStrlen(const char *text) { return text == 0 ? 0 : lstrlenA(text); }
 int Allocate(int length) {
  if (length == 0) { Init(); return 1; }
  ObservedStringHeader *p = (ObservedStringHeader *)::operator new(length + 13);
  if (!p) return 0;
  p->refs = 1; ((char *)(p + 1))[length] = 0; p->length = length; p->capacity = length;
  data = (char *)(p + 1);
  return 1;
 }
 void Release() {
  if (Header() != observed_empty_header) {
   if (InterlockedDecrement(&Header()->refs) <= 0) ::operator delete(Header());
   Init();
  }
 }
 static void __stdcall Release(ObservedStringHeader *header) {
  if (header != observed_empty_header && InterlockedDecrement(&header->refs) <= 0)
   ::operator delete(header);
 }
 int AllocBeforeWrite(int length) {
  int ok = 1;
  if (Header()->refs > 1 || length > Header()->capacity) { Release(); ok = Allocate(length); }
  return ok;
 }
 void AssignCopy(int length, const char *text) {
  if (AllocBeforeWrite(length)) { memcpy(data, text, length); Header()->length = length; data[length] = 0; }
 }
 int ConcatCopy(int length1, const char *text1, int length2, const char *text2) {
  int ok = 1;
  int length = length1 + length2;
  if (length != 0) {
   ok = Allocate(length);
   if (ok) { memcpy(data, text1, length1); memcpy(data + length1, text2, length2); }
  }
  return ok;
 }
 void ConcatInPlace(int length, const char *text) {
  if (length == 0) return;
  if (Header()->refs > 1 || Header()->length + length > Header()->capacity) {
   ObservedStringHeader *old = Header();
   if (ConcatCopy(Header()->length, data, length, text)) Release(old);
  } else {
   memcpy(data + Header()->length, text, length);
   Header()->length += length;
   data[Header()->length] = 0;
  }
 }
};
#endif
