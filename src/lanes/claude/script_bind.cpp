// Compile a game object's script and bind its `self` variable (0x40e720).
#include "lane_string.h"
extern "C" int scGet_Symbol(void *instance, char *name);   // SeeR, 0x476401
void __cdecl PickingTrace(const char *, ...);               // engine log, 0x403650

struct ScriptString : LaneString {
 int AllocBeforeWrite_00402260(int length);
 ScriptString(const ScriptString &other) {
  if (other.Header()->refs < 0) {
   Init();
   int length = other.data ? lstrlenA(other.data) : 0;
   if (AllocBeforeWrite_00402260(length)) {
    memcpy(data, other.data, length); Header()->length = length; data[length] = 0;
   }
  } else { data = other.data; InterlockedIncrement(&Header()->refs); }
 }
 __forceinline ~ScriptString() {
  if (Header() != observed_empty_header && InterlockedDecrement(&Header()->refs) <= 0)
   delete (unsigned char *)Header();
 }
 int ConcatCopy_0040e8f0(int len1, const char *s1, int len2, const char *s2);
 void operator+=(const char *text) {
  int length = lstrlenA(text);
  if (length == 0) return;
  if (Header()->refs < 2 && Header()->length + length <= Header()->capacity) {
   memcpy(data + Header()->length, text, length);
   Header()->length += length;
   data[Header()->length] = 0;
  } else {
   LaneStringHeader *old = Header();
   if (ConcatCopy_0040e8f0(old->length, data, length, text)) ReleaseHeader_00406700(old);
  }
 }
 static void __stdcall ReleaseHeader_00406700(LaneStringHeader *header);
};

struct ScriptInstanceView { int pad; char *data; };
struct ObjectScript { bool Compile_0041a7f0(ScriptString source); };

class ScriptedObject_0040e720 {
public:
 char head[0x44];
 ObjectScript script;                 // 0x44
 int pad48;
 ScriptInstanceView *instance;        // 0x4c
 bool BindScript_0040e720(ScriptString source);
};

bool ScriptedObject_0040e720::BindScript_0040e720(ScriptString source) {
 if (!script.Compile_0041a7f0(source)) {
  PickingTrace("failed to compile script");
  return false;
 }
 int self = scGet_Symbol(instance, "self");
 *(ScriptedObject_0040e720 **)(instance->data + self + 0x400) = this;
 source += " compiled";
 PickingTrace(source.data);
 return true;
}
