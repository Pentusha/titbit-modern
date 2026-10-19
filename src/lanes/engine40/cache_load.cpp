// Load-by-name for the walk-area cache (0x407880) and the skin cache (0x40a300):
// the file name without directory and extension is the key; a record already
// loaded gets its reference count raised.
#include "e40_string.h"

struct WalkArea_00407ff0 {
 char name[0x20];
 int used, refs, age;                 // +0x20
 char body[0x138b8 - 0x2c];
 bool Load_00408440(ObservedString file);
};

class WalkAreaCache_00502480 {
public:
 int step, count;
 WalkArea_00407ff0 *data;
 int Find_00407d10(ObservedString name);
 int FindFree_00407e50();
 bool Reserve_00407eb0(int size);
 int Load_00407880(ObservedString path, int age);
};

int WalkAreaCache_00502480::Load_00407880(ObservedString path, int age) {
 path.MakeLower();
 ObservedString name;
 name = path.Mid(path.ReverseFind('\\') + 1);
 name = name.Mid(0, name.ReverseFind('.'));
 int index = Find_00407d10(name);
 if (index == -1) {
  index = FindFree_00407e50();
  if (index == -1) {
   index = count;
   Reserve_00407eb0(step + index);
  }
  data[index].used = index;
  if (!data[index].Load_00408440(path)) return -1;
  data[index].age = age;
  data[index].refs = 1;
  return index;
 }
 data[index].refs++;
 return index;
}

struct SoundHolder_0040e380 {
 void *vtable;
 char name[0x20];
 int used, refs, age;                 // +0x24
 char body[0x434 - 0x30];
 bool Load_0040e3e0(ObservedString file);
};

class SkinCache_00502490 {
public:
 int step, count;
 SoundHolder_0040e380 *data;
 int Find_0040a5d0(ObservedString name);
 int FindFree_0040a720();
 bool Reserve_0040a790(int size);
 int Load_0040a300(ObservedString path, int age);
};

int SkinCache_00502490::Load_0040a300(ObservedString path, int age) {
 path.MakeLower();
 ObservedString name;
 name = path.Mid(path.ReverseFind('\\') + 1);
 name = name.Mid(0, name.ReverseFind('.'));
 int index = Find_0040a5d0(name);
 if (index == -1) {
  index = FindFree_0040a720();
  if (index == -1) {
   index = count;
   Reserve_0040a790(step + index);
  }
  data[index].used = index;
  if (!data[index].Load_0040e3e0(path)) return -1;
  data[index].age = age;
  data[index].refs = 1;
  return index;
 }
 data[index].refs++;
 return index;
}
