#pragma inline_depth(3)


// MFC-style pointer array (data, size, capacity, grow_by) and the depth-sorted
// insert used by the render lists (0x411d70).
extern "C" {
void *__cdecl memset(void *, int, unsigned);
void *__cdecl memcpy(void *, const void *, unsigned);
void *__cdecl memmove(void *, const void *, unsigned);
}
#pragma intrinsic(memset, memcpy)
void *__cdecl operator new(unsigned);
void __cdecl operator delete(void *);

#ifdef TITBIT_PORTABLE_HEADERS
#include "../include/ptr_array_construct.h"
#else
template<class T> inline void __stdcall ConstructElements_004120d0(T *p, int n) {
 if (p && n) memset(p, 0, n * sizeof(T));
}
#endif
template<class T> inline void __stdcall DestructElements_004120c0(T *, int) {}

template<class T> class PtrArray {
public:
 T *data;
 int size, capacity, grow_by;
 inline void SetSize(int new_size, int grow = -1);
 void SetAtGrow(int index, T value) { if (index >= size) SetSize(index + 1, -1); data[index] = value; }
 int Add(T value) { int index = size; SetAtGrow(index, value); return index; }
 void InsertAt(int index, T value, int count = 1) {
  if (index >= size) SetSize(index + count, -1);
  else {
   int old_size = size;
   SetSize(size + count, -1);
   DestructElements_004120c0(&data[old_size], count);
   memmove(&data[index + count], &data[index], (old_size - index) * sizeof(T));
   ConstructElements_004120d0(&data[index], count);
  }
  while (count--) data[index++] = value;
 }
};

template<class T> inline void PtrArray<T>::SetSize(int new_size, int grow) {
 if (new_size < 0) return;
 if (grow != -1) grow_by = grow;
 if (new_size == 0) {
  if (data) { operator delete(data); data = 0; }   // game: no DestructElements here
  size = capacity = 0;
 } else if (data == 0) {
  data = (T *)operator new(new_size * sizeof(T));
  ConstructElements_004120d0(data, new_size);
  size = capacity = new_size;
 } else if (new_size <= capacity) {
  if (new_size > size) ConstructElements_004120d0(&data[size], new_size - size);
  else if (size > new_size) DestructElements_004120c0(&data[new_size], size - new_size);
  size = new_size;
 } else {
  int growth = grow_by;
  if (growth == 0) {
   growth = size / 8;
   growth = (growth < 4) ? 4 : ((growth > 1024) ? 1024 : growth);
  }
  int new_capacity;
  if (new_size < capacity + growth) new_capacity = capacity + growth;
  else new_capacity = new_size;
  T *next = (T *)operator new(new_capacity * sizeof(T));
  memcpy(next, data, size * sizeof(T));
  if (new_size > size) ConstructElements_004120d0(&next[size], new_size - size);
  operator delete(data);
  data = next;
  size = new_size;
  capacity = new_capacity;
 }
}

struct DepthFrame { float x, y, unused, depth; };
struct DepthItem { char head[0x38]; DepthFrame *frame; };

struct DepthList : PtrArray<DepthItem *> { // original 0x411d70
 int Insert_00411d70(DepthItem *item);
};

int DepthList::Insert_00411d70(DepthItem *item) {
 for (int i = 0; i < size; i++)
  if (data[i]->frame->depth > item->frame->depth) { InsertAt(i, item); return i; }
 return Add(item);
}
