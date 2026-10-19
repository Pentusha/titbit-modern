#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Scene string table (key/value pairs at scene+0xfe8): add, translate, and the
// UTF-8 to CP1251 unescape of XML text (0x42a530-0x42abe0).
#include "../../native/scene_pair_array.h"
extern "C" __declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);

// ScenePairElement destructor emitted out of line (0x424530).
void __fastcall DestroyPair_00424530(ScenePairElement *pair);

struct PairStorage42 {
 ScenePairElement *data;
 int size, capacity, grow_by;
 void SetSize_0042aa60(int newSize, int growBy);
};

class TextTable42 {
public:
 void *vtable;
 PairStorage42 storage;
 void Add_0042a530(const ScenePairElement &pair);
 ObservedString Translate_0042a6e0(ObservedString key);
};

static void __stdcall ConstructPairs_0042abe0(ScenePairElement *elements, int count);

// NONMATCHING: faithful translation
void TextTable42::Add_0042a530(const ScenePairElement &pair) {
 ScenePairElement entry(pair);                        // copy construction (WTL copy ctor)
 int index = storage.size;
 ScenePairElement value(entry);
 if (index >= storage.size) storage.SetSize_0042aa60(index + 1, -1);
 storage.data[index].first = value.first;
 storage.data[index].second = value.second;
}

// Returns the value stored for `key`, or the key itself.
// NONMATCHING: faithful translation
ObservedString TextTable42::Translate_0042a6e0(ObservedString key) {
 for (int i = 0; i < storage.size; i++)
  if (lstrcmpA(storage.data[i].first, key) == 0) return storage.data[i].second;
 return key;
}

struct Utf8String42 {
 char *data;
 ObservedStringHeader *Header() const { return (ObservedStringHeader *)data - 1; }
 void Init() { data = observed_empty_string; }
 ~Utf8String42() { ReleaseInline(); }
 void ReleaseInline() {
  if (Header() != observed_empty_header && InterlockedDecrement(&Header()->refs) <= 0)
   delete (unsigned char *)Header();
 }
 int ConcatCopy_0040e8f0(int len1, const char *s1, int len2, const char *s2);
 static void __stdcall ReleaseHeader_00406700(ObservedStringHeader *header);
 void Append(char c) {
  ObservedStringHeader *h = Header();
  if (h->refs > 1 || h->length + 1 > h->capacity) {
   if (ConcatCopy_0040e8f0(h->length, data, 1, &c)) ReleaseHeader_00406700(h);
  } else {
   data[h->length] = c;
   h->length++;
   data[h->length] = 0;
  }
 }
};

// Two-byte UTF-8 Cyrillic (D0 xx / D1 xx) to single-byte CP1251.
// NONMATCHING: faithful translation
ObservedString __cdecl Unescape_0042a8b0(ObservedString text) {
 Utf8String42 out;
 out.Init();
 const char *in = text;
 int length = text.GetLength();
 for (int i = 0; i < length; i++) {
  char c = in[i];
  if (c == (char)0xd0) c = in[++i] + 0x30;
  else if (c == (char)0xd1) c = in[++i] + 0x70;
  out.Append(c);
 }
 return *(ObservedString *)&out;                      // copy (shares the buffer), then ~out
}

// NONMATCHING: faithful translation
void PairStorage42::SetSize_0042aa60(int newSize, int growBy) {
 if (newSize < 0) return;
 if (growBy != -1) grow_by = growBy;
 if (newSize == 0) {
  if (data) {
   ScenePairElement *p = data;
   for (int n = size; n; n--) { DestroyPair_00424530(p); p++; }
   ::operator delete(data);
   data = 0;
  }
  capacity = size = 0;
  return;
 }
 if (data == 0) {
  data = (ScenePairElement *)::operator new(newSize * sizeof(ScenePairElement));
  ConstructPairs_0042abe0(data, newSize);
  capacity = newSize;
  size = newSize;
  return;
 }
 if (newSize > capacity) {
  int grow = grow_by;
  if (grow == 0) {
   grow = size / 8;
   grow = (grow < 4) ? 4 : ((grow > 1024) ? 1024 : grow);
  }
  int newMax = capacity + grow;
  if (newMax <= newSize) newMax = newSize;
  ScenePairElement *newData = (ScenePairElement *)::operator new(newMax * sizeof(ScenePairElement));
  memcpy(newData, data, size * sizeof(ScenePairElement));
  if (size < newSize) ConstructPairs_0042abe0(&newData[size], newSize - size);
  ::operator delete(data);
  data = newData;
  capacity = newMax;
 } else if (size < newSize) {
  ConstructPairs_0042abe0(&data[size], newSize - size);
 } else if (size > newSize) {
  ScenePairElement *p = &data[newSize];
  for (int n = size - newSize; n; n--) { DestroyPair_00424530(p); p++; }
 }
 size = newSize;
}

extern "C" void *__cdecl memset(void *, int, unsigned);
#pragma intrinsic(memset)
inline void *__cdecl operator new(unsigned, void *p) { return p; }
inline void __cdecl operator delete(void *, void *) {}

static void __stdcall ConstructPairs_0042abe0(ScenePairElement *elements, int count) {
 if (elements && count) {
  memset(elements, 0, count * sizeof(ScenePairElement));
  for (; count--; elements++) ::new((void *)elements) ScenePairElement;
 }
}
