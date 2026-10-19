// Engine object subclass with 12 embedded 0x38-byte slots (0x405160-0x4052c0).
// Base object: ctor 0x4137b0 / dtor 0x4137f0. Slot: ctor 0x428ab0 / dtor 0x428ae0.
#include "lane_engine.h"
struct Slot_00428ab0 { char raw[0x38]; Slot_00428ab0(); ~Slot_00428ab0(); };

class SlotBank_00405240 : public Node_00405160 {
public:
 Slot_00428ab0 slots[12];
 int selected, count, active;
 float scale;
 SlotBank_00405240(int ownerObject);
 virtual ~SlotBank_00405240();
#ifdef TITBIT_SOURCE_LINK
 virtual void Update(int);
#endif
};

Node_00405160::Node_00405160() {}
Node_00405160::~Node_00405160() {}
#ifdef TITBIT_SOURCE_LINK
// Original slot 6 is the shared one-byte RET at 0x4051b0.
void Node_00405160::Slot6() {}
#endif
SlotBank_00405240::~SlotBank_00405240() {}

// NONMATCHING 0x405240: store order of the zeroed counters vs owner differs
SlotBank_00405240::SlotBank_00405240(int ownerObject) {
 active = 0; count = 0; selected = 0;
 owner = ownerObject;
 scale = 1.0f;
}

