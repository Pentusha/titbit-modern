#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Script timer component (vtable 0x4cd6f0): posts message 5 to its owner every
// `interval` ms, `repeats` times (-1 = forever) (0x42ca40-0x42cab0).
#include "../../native/picking_control_interfaces.h"

struct ScenePost42 { bool Send_00424780(ObservedPickingMessage *message); };
extern ScenePost42 g_005011c0;

struct ComponentE42 {
 virtual ObservedString Slot0();
 virtual void Slot1(int);
 virtual void Update(int time);                      // +0x08
 virtual ~ComponentE42();                            // +0x0c
 void *owner;
 ComponentE42 *next;
 ComponentE42();                                     // 0x40b0f0
 void Unlink_0040b2a0();
};

class Timer : public ComponentE42 {  // 0x42ca70 (was Timer_0042ca70)
public:
 int nextTime;                     // 0x0c
 int interval;                     // 0x10
 int repeats;                      // 0x14
 int id;                           // 0x18
 Timer(int anId, int now, int anInterval, int count);
 virtual ~Timer();
 virtual void Update(int time);
};

Timer::Timer(int anId, int now, int anInterval, int count) {
 id = anId;
 interval = anInterval;
 nextTime = now + anInterval;
 repeats = count;
}

Timer::~Timer() {}

// NONMATCHING: faithful translation
void Timer::Update(int time) {
 if (time >= nextTime) {
  if (repeats == 0) {
   if (next) next->Update(time);
   Unlink_0040b2a0();
   delete this;
   return;
  }
  if (repeats > 0) repeats--;
  nextTime = interval + time;
  ObservedPickingMessage message;
  memset(&message, 0, sizeof(message));
  message.selected = owner;
  message.event = 5;
  ((int *)message.reserved28)[1] = id;                  // +0x2c (original)
  g_005011c0.Send_00424780(&message);
 }
 if (next) next->Update(time);
}
