#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
#define OBSERVED_STRING_LOADSTRING_INLINE  // this unit expands LoadString inline (original)
// Sound effect bank and script sound objects (0x4277f0-0x428280).
#include "../../native/engine_string.h"
extern "C" __declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);
const char *__cdecl ReverseFind_00407b50(const char *text, char ch);

struct StringOps42 : ObservedString {
 ObservedString Mid_00407b80(int first) const;
 void MakeLowerCase() { MakeLower(); }
};

struct SoundState42 {
 char name[0x20];
 int index, refs, param;
 SoundState42();                                     // 0x402ce0
};

// One bank entry (0x34 bytes, vtable 0x4cd698).
class SoundSlot {  // 0x4277f0 (was SoundSlot_004277f0)
public:
 SoundState42 state;
 int reserved30;
 SoundSlot();
 virtual ~SoundSlot();                       // 0x427830
 bool Load_00427840(ObservedString path);
};

struct SoundSlotArray {
 int growBy;
 int count;
 SoundSlot *slots;
 SoundSlotArray() { slots = 0; growBy = 5; }
 bool Grow_00427c50(int newCount);
};
class SoundBank : public SoundSlotArray {  // 0x427870 (was SoundBank_00427870)
public:
 SoundBank();
 virtual ~SoundBank();
 int Register_00427900(ObservedString path, int param);
};
extern SoundBank g_005ee704;

// Script sound object (0x20 bytes, vtable 0x4cd6a0).
struct RefCounted42 { virtual void __stdcall Ref(); virtual void __stdcall Release(); };
extern RefCounted42 *g_005ee6fc;
class Sound {  // 0x427e10 (was Sound_00427e10)
public:
 int id;                         // 0x04
 bool playing;                   // 0x08
 int channel;                    // 0x0c
 int loop;                       // 0x10
 int volume;                     // 0x14
 bool unloaded;                  // 0x18
 int reserved1c;                 // 0x1c
 Sound();
 virtual ~Sound();
 void Load_00427f90(ObservedString name);
 void Release_00428020();
};

SoundSlot::SoundSlot() {
 reserved30 = 0;
}

SoundSlot::~SoundSlot() {}

// Loading of sound effect files is not implemented: every registration fails.
bool SoundSlot::Load_00427840(ObservedString path) {
 return false;
}

SoundBank::SoundBank() {}

// NONMATCHING: faithful translation
SoundBank::~SoundBank() {
 SoundSlot *p = slots;
 if (p) {
  for (int n = count; n; n--) { p->~SoundSlot(); p++; }
  ::operator delete(slots);
  slots = 0;
 }
 count = 0;
}

static void __stdcall ConstructSlots_00427ce0(SoundSlot *slots, int count);

// Returns the slot for a sound file, sharing an already registered name.
// NONMATCHING: faithful translation
int SoundBank::Register_00427900(ObservedString path, int param) {
 path.MakeLower();
 ObservedString file;
 const char *slash = ReverseFind_00407b50(path, '\\');
 file = ((StringOps42 &)path).Mid_00407b80((slash ? slash - (const char *)path : -1) + 1);
 {
  ObservedString key(file);
  key.MakeLower();
  for (int i = 0; i < count; i++) {
   if (lstrcmpA(key, slots[i].state.name) == 0) {
    slots[i].state.refs++;
    return i;
   }
  }
 }
 int slot = -1;
 int last = count - 1;
 if (last >= 2) {
  for (int k = 1; k < last; k++) {
   if (slots[k].state.index == 0) { slot = k; continue; }   // empty: remember, keep looking
   if (slots[k].state.param < 1000 && slots[k].state.refs == 0) { slot = k; break; }
  }
  if (slot != -1) { SoundSlot *reuse = &slots[slot]; reuse->~SoundSlot(); }   // virtual call
 }
 if (slot == -1) {
  slot = count;
  Grow_00427c50(growBy + count);
 }
 slots[slot].state.index = slot;
 if (!slots[slot].Load_00427840(path)) return -1;
 slots[slot].state.param = param;
 slots[slot].state.refs = 1;
 return slot;
}

// NONMATCHING: faithful translation
bool SoundSlotArray::Grow_00427c50(int newCount) {
 if (count >= newCount) return false;
 SoundSlot *data = (SoundSlot *)::operator new(newCount * sizeof(SoundSlot));
 memcpy(data, slots, count * sizeof(SoundSlot));
 if (slots) ::operator delete(slots);
 slots = data;
 ConstructSlots_00427ce0(&data[count], newCount - count);
 count = newCount;
 return true;
}

extern "C" void *__cdecl memset(void *, int, unsigned);
#pragma intrinsic(memset)
inline void *__cdecl operator new(unsigned, void *p) { return p; }
inline void __cdecl operator delete(void *, void *) {}

// NONMATCHING: faithful translation
static void __stdcall ConstructSlots_00427ce0(SoundSlot *slots, int count) {
 if (slots && count) {
  memset(slots, 0, count * sizeof(SoundSlot));
  for (; count--; slots++) ::new((void *)slots) SoundSlot;
 }
}

// NONMATCHING: faithful translation
Sound::Sound() {
 unloaded = true;
 playing = false;
 channel = -1;
 volume = -101;
 loop = -1;
 reserved1c = 0;
 if (g_005ee6fc) {
  g_005ee6fc->Release();
  g_005ee6fc = 0;
 }
}

Sound::~Sound() {
 Release_00428020();
}

void Sound::Load_00427f90(ObservedString name) {
 id = g_005ee704.Register_00427900(name, 1);
 if (id != -1) unloaded = false;
}

void Sound::Release_00428020() {
 if (!unloaded && id >= 0 && id < g_005ee704.count) {
  if (g_005ee704.slots[id].state.refs) g_005ee704.slots[id].state.refs--;
 }
}

// Script CreateSound(name)
// NONMATCHING: faithful translation
Sound *__cdecl CreateSound_004280a0(const char *name) {
 Sound *sound = new Sound;
 sound->Load_00427f90(ObservedString(name));
 return sound;
}

// Script DeleteSound(sound)
void __cdecl DeleteSound_00428220(Sound *sound) {
 if (sound) delete sound;
}

class MasterOutputVolume42 {
public:
 virtual bool IsAvailable();
 virtual void Enable();
 virtual void Disable();
 virtual unsigned long GetSteps();
 virtual unsigned long GetMinimum();
 virtual unsigned long GetMaximum();
 virtual unsigned long GetVolume();
 virtual void SetVolume(unsigned long value);
};
extern MasterOutputVolume42 *g_005ee718;

// Script SetMasterVolume(percent)
// NONMATCHING: faithful translation
#ifdef TITBIT_COMPAT
#include "../../compat/compat.h"
#endif
void __cdecl SetMasterVolume_00428230(int percent) {
#ifdef TITBIT_COMPAT
 CompatSetMasterVolume(percent);
#else
 unsigned long maximum = g_005ee718->GetMaximum();
 unsigned long minimum = g_005ee718->GetMinimum();
 g_005ee718->SetVolume((int)((maximum - minimum) * percent) / 100);
#endif
}

// Script GetMasterVolume()
// NONMATCHING: faithful translation
int __cdecl GetMasterVolume_00428280() {
#ifdef TITBIT_COMPAT
 return CompatGetMasterVolume();
#else
 unsigned range = g_005ee718->GetMaximum() - g_005ee718->GetMinimum();
 return (unsigned)(g_005ee718->GetVolume() * 100 - g_005ee718->GetMinimum() * 100) / range;   // unsigned div (original)
#endif
}
