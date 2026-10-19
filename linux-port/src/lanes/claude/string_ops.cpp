// Out-of-line string members and small helpers (0x4021c0-0x4048d0).
#include "lane_string.h"
extern "C" __declspec(dllimport) int __stdcall ShellExecuteExA(void *);

class LaneStringOps : public LaneString {
public:
 void CopyBeforeWrite_004021c0();
 int AllocBeforeWrite_00402260(int length);
 LaneStringOps &Assign_004023e0(const LaneStringOps &other);
};

void LaneStringOps::CopyBeforeWrite_004021c0() {
 if (Header()->refs > 1) {
  LaneStringHeader *old = Header();
  ReleaseInline();
  if (AllocInline(old->length)) memcpy(data, old + 1, old->length + 1);
 }
}

// NONMATCHING 0x402260: constant 1 is not hoisted into eax as in the original
int LaneStringOps::AllocBeforeWrite_00402260(int length) {
 if (Header()->refs > 1 || length > Header()->capacity) {
  ReleaseInline();
  return AllocInline(length);
 }
 return 1;
}

// NONMATCHING 0x4023e0: branch layout differs (2 bytes longer)
LaneStringOps &LaneStringOps::Assign_004023e0(const LaneStringOps &other) {
 if (data != other.data) {
  if ((Header()->refs < 0 && Header() != observed_empty_header) || other.Header()->refs < 0) {
   int length = other.Header()->length;
   if (Header()->refs > 1 || length > Header()->capacity) {
    ReleaseInline();
    if (!Allocate_00402120(length)) return *this;
   }
   memcpy(data, other.data, length);
   Header()->length = length; data[length] = 0;
  } else {
   ReleaseInline();
   data = other.data;
   InterlockedIncrement(&Header()->refs);
  }
 }
 return *this;
}

struct ShellExecuteInfo {
 unsigned long cbSize, fMask; void *hwnd; const char *lpVerb, *lpFile, *lpParameters, *lpDirectory;
 int nShow; void *hInstApp, *lpIDList; const char *lpClass; void *hkeyClass; unsigned long dwHotKey;
 void *hIcon, *hProcess;
};

struct LocalString : LaneString {
 LocalString() { Init(); }
 __forceinline ~LocalString() {
  if (Header() != observed_empty_header && InterlockedDecrement(&Header()->refs) <= 0)
   delete (unsigned char *)Header();
 }
};

// NONMATCHING 0x403690: tail after ShellExecuteEx (string dtor placement) differs
void __cdecl OpenDocument_00403690(const char *path) {
 try {
  LocalString file;
  int length = path ? lstrlenA(path) : 0;
  if (file.Header()->refs > 1 || length > file.Header()->capacity) {
   file.Release_00402180();
   if (!file.Allocate_00402120(length)) goto run;
  }
  memcpy(file.data, path, length);
  file.Header()->length = length; file.data[length] = 0;
 run:
  ShellExecuteInfo info;
  info.lpFile = file.data;
  info.cbSize = sizeof(info);
  info.fMask = 0x440;
  info.lpVerb = "open";
  info.lpParameters = 0;
  info.lpDirectory = 0;
  info.nShow = 3;
  ShellExecuteExA(&info);
 } catch (...) {
 }
}

struct OwnerWithBuffer_004048d0 { char pad[0xe8]; void *buffer; void Free_004048d0(); };
void OwnerWithBuffer_004048d0::Free_004048d0() { if (buffer) ::operator delete(buffer); }

// Static release of a string header (0x406700).
void __stdcall ReleaseHeader_00406700(LaneStringHeader *header) {
 if (header != observed_empty_header && InterlockedDecrement(&header->refs) <= 0)
  delete (unsigned char *)header;
}

// Last occurrence of a character, MBCS aware (0x407b50).
extern "C" __declspec(dllimport) char *__stdcall CharNextA(const char *);
const char *__cdecl ReverseFind_00407b50(const char *text, char ch) {
 const char *last = 0;
 while (*text) {
  if (*text == ch) last = text;
  text = CharNextA(text);
 }
 return last;
}

class LaneStringEx : public LaneString {
public:
 void Append_004075a0(int length, const char *text);
 LaneStringEx Mid_00407bb0(int first, int count) const;
 LaneStringEx Mid_00407b80(int first) const;
 LaneStringEx Left_004076a0(int count) const;
 LaneStringEx() { Init(); }
 LaneStringEx(const LaneStringEx &other) {
  if (other.Header()->refs < 0) {
   Init();
   int length = lstrlenA(other.data);
   if (Header()->refs > 1 || length > Header()->capacity) {
    Release_00402180();
    if (!Allocate_00402120(length)) return;
   }
   memcpy(data, other.data, length); Header()->length = length; data[length] = 0;
  } else { data = other.data; InterlockedIncrement(&Header()->refs); }
 }
 __forceinline ~LaneStringEx() {
  if (Header() != observed_empty_header && InterlockedDecrement(&Header()->refs) <= 0)
   delete (unsigned char *)Header();
 }
};

void LaneStringEx::Append_004075a0(int length, const char *text) {
 if (length == 0) return;
 LaneStringHeader *old = Header();
 if (old->refs < 2 && old->length + length <= old->capacity) {
  memcpy(data + old->length, text, length);
  Header()->length += length;
  data[Header()->length] = 0;
  return;
 }
 int oldLength = old->length, total = oldLength + length;
 if (total != 0) {
  LaneStringHeader *p = (LaneStringHeader *)::operator new(total + 13);
  if (!p) return;
  p->refs = 1; ((char *)(p + 1))[total] = 0; p->length = total; p->capacity = total;
  data = (char *)(p + 1);
  memcpy(data, old + 1, oldLength);
  memcpy(data + oldLength, text, length);
 }
 if (old != observed_empty_header && InterlockedDecrement(&old->refs) <= 0) delete (unsigned char *)old;
}

LaneStringEx LaneStringEx::Mid_00407bb0(int first, int count) const {
 if (first < 0) first = 0;
 if (count < 0) count = 0;
 int length = Header()->length;
 if (first + count > length) count = length - first;
 if (first > length) count = 0;
 LaneStringEx result;
 if (count != 0) {
  LaneStringHeader *p = (LaneStringHeader *)::operator new(count + 13);
  if (p) {
   p->refs = 1; ((char *)(p + 1))[count] = 0; p->length = count; p->capacity = count;
   result.data = (char *)(p + 1);
   memcpy(result.data, data + first, count);
  }
 }
 return result;
}

LaneStringEx LaneStringEx::Mid_00407b80(int first) const { return Mid_00407bb0(first, Header()->length - first); }

LaneStringEx LaneStringEx::Left_004076a0(int count) const {
 if (count < 0) count = 0;
 else if (count > Header()->length) count = Header()->length;
 LaneStringEx result;
 if (count != 0) {
  LaneStringHeader *p = (LaneStringHeader *)::operator new(count + 13);
  if (p) {
   p->refs = 1; ((char *)(p + 1))[count] = 0; p->length = count; p->capacity = count;
   result.data = (char *)(p + 1);
   memcpy(result.data, data, count);
  }
 }
 return result;
}

// Indexed access into an array of 0x138b8-byte records (0x407e30) and lookup
// by the record's name, which starts each record (0x407d10).
extern "C" __declspec(dllimport) char *__stdcall CharLowerA(char *);
extern "C" __declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);
struct BigRecordArray_00407e30 {
 int pad; int count; char *items;
 char *At_00407e30(int index);
 int Find_00407d10(LaneStringEx name);
};
int BigRecordArray_00407e30::Find_00407d10(LaneStringEx name) {
 if (name.Header()->refs > 1) {
  LaneStringHeader *old = name.Header();
  name.ReleaseInline();
  if (name.Allocate_00402120(old->length)) memcpy(name.data, old + 1, old->length + 1);
 }
 CharLowerA(name.data);
 for (int i = 0; i < count; i++)
  if (lstrcmpA(name.data, items + i * 0x138b8) == 0) return i;
 return -1;
}
char *BigRecordArray_00407e30::At_00407e30(int index) {
 if (index >= 0 && index < count) return items + index * 0x138b8;
 return 0;
}

// operator+ (string, string) 0x40c3e0 and (string, const char *) 0x40c540.
static inline void ConcatInto(LaneStringEx &s, int len1, const char *s1, int len2, const char *s2) {
 int total = len1 + len2;
 if (total != 0) {
  LaneStringHeader *p = (LaneStringHeader *)::operator new(total + 13);
  if (p) {
   p->refs = 1; ((char *)(p + 1))[total] = 0; p->length = total; p->capacity = total;
   s.data = (char *)(p + 1);
   memcpy(s.data, s1, len1);
   memcpy(s.data + len1, s2, len2);
  }
 }
}
LaneStringEx __stdcall Concat_0040c3e0(const LaneStringEx &a, const LaneStringEx &b) {
 LaneStringEx s;
 ConcatInto(s, a.Header()->length, a.data, b.Header()->length, b.data);
 return s;
}
LaneStringEx __stdcall Concat_0040c540(const LaneStringEx &a, const char *b) {
 LaneStringEx s;
 int length = b ? lstrlenA(b) : 0;
 ConcatInto(s, a.Header()->length, a.data, length, b);
 return s;
}

// operator+ (const char *, string) 0x40e070 and operator+= (string) 0x40e1d0.
LaneStringEx __stdcall Concat_0040e070(const char *a, const LaneStringEx &b) {
 LaneStringEx s;
 int length = a ? lstrlenA(a) : 0;
 ConcatInto(s, length, a, b.Header()->length, b.data);
 return s;
}
struct LaneStringCat : LaneStringEx { LaneStringCat &AppendString_0040e1d0(const LaneStringEx &other); };
LaneStringCat &LaneStringCat::AppendString_0040e1d0(const LaneStringEx &other) {
 int length = other.Header()->length;
 if (length == 0) return *this;
 LaneStringHeader *old = Header();
 if (old->refs < 2 && old->length + length <= old->capacity) {
  memcpy(data + old->length, other.data, length);
  Header()->length += length;
  data[Header()->length] = 0;
  return *this;
 }
 int oldLength = old->length;
 if (oldLength + length != 0) {
  if (!Allocate_00402120(oldLength + length)) return *this;
  memcpy(data, old + 1, oldLength);
  memcpy(data + oldLength, other.data, length);
 }
 if (old != observed_empty_header && InterlockedDecrement(&old->refs) <= 0) delete (unsigned char *)old;
 return *this;
}

// Replace every occurrence of `from` with `to`; returns the count (0x40eff0).
extern "C" void *__cdecl memmove(void *, const void *, unsigned);
extern "C" int __cdecl memcmp(const void *, const void *, unsigned);
#pragma intrinsic(memcmp)
static inline const char *MbFindChar(const char *p, char c) {
 while (*p && *p != c) p = CharNextA(p);
 return *p == c ? p : 0;
}
static inline const char *MbFind(const char *text, const char *pattern) {
 int length = lstrlenA(pattern);
 if (length == 0) return 0;
 const char *p = text;
 while ((p = MbFindChar(p, *pattern)) != 0) {
  if (memcmp(p, pattern, length) == 0) return p;
  p = CharNextA(p);
 }
 return 0;
}
struct LaneStringReplace : LaneString { int Replace_0040eff0(const char *from, const char *to); };
int LaneStringReplace::Replace_0040eff0(const char *from, const char *to) {
 int fromLength;
 if (from == 0 || (fromLength = lstrlenA(from)) == 0) return 0;
 int toLength = to ? lstrlenA(to) : 0;
 int count = 0;
 char *start = data, *end = data + Header()->length, *target;
 while (start < end) {
  while ((target = (char *)MbFind(start, from)) != 0) { count++; start = target + fromLength; }
  start += lstrlenA(start) + 1;
 }
 if (count > 0) {
  if (Header()->refs > 1) {
   LaneStringHeader *old = Header();
   Release_00402180();
   if (Allocate_00402120(old->length)) memcpy(data, old + 1, old->length + 1);
  }
  int oldLength = Header()->length;
  int newLength = oldLength + (toLength - fromLength) * count;
  if (Header()->capacity < newLength || Header()->refs > 1) {
   LaneStringHeader *old = Header();
   if (newLength == 0) Init();
   else {
    LaneStringHeader *p = (LaneStringHeader *)::operator new(newLength + 13);
    if (!p) return -1;
    p->refs = 1; ((char *)(p + 1))[newLength] = 0; p->length = newLength; p->capacity = newLength;
    data = (char *)(p + 1);
   }
   memcpy(data, old + 1, old->length);
   if (old != observed_empty_header && InterlockedDecrement(&old->refs) <= 0) delete (unsigned char *)old;
  }
  start = data; end = data + Header()->length;
  while (start < end) {
   while ((target = (char *)MbFind(start, from)) != 0) {
    int balance = oldLength - (target - data + fromLength);
    memmove(target + toLength, target + fromLength, balance);
    memcpy(target, to, toLength);
    start = target + toLength;
    start[balance] = 0;
    oldLength += toLength - fromLength;
   }
   start += lstrlenA(start) + 1;
  }
  Header()->length = newLength;
 }
 return count;
}
