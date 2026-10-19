#ifndef TITBIT_PICKING_CONTROL_INTERFACES_H
#define TITBIT_PICKING_CONTROL_INTERFACES_H
/* Observed ABI/layout views, not recovered complete game class identities.
 * Native globals remain external; fixture storage is not reconstruction. */
extern "C" void *__cdecl memset(void *, int, unsigned);
#pragma intrinsic(memset)
struct ObservedPickingMessage {
 unsigned reserved0[8];
 void *selected;
 unsigned event;
 union {unsigned reserved28[8];float floats[8];};
};
class ObservedPickingNode {
public:
 virtual void Slot0();
 virtual int Type();
 unsigned reserved4, reserved8;
 ObservedPickingNode *next;
 unsigned reserved10;
 char name[1];
};
struct ObservedPickingList {
 ObservedPickingNode **data;
 int size, capacity, grow_by;
};
class ObservedString;
#include "scene_owner_view.h"
#endif
