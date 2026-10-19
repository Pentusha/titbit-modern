// Core engine object classes (0x405e70-0x406a70, 0x40b090-0x40b130, 0x4137b0-0x4137f0).
#include "lane_engine.h"

EngineObject::EngineObject() { field8 = 0; owner = 0; child = 0; extra = 0; }
EngineObject::~EngineObject() {
 if (child) delete child;
 if (extra) extra->Release(1);
}

Frame_0040b090::Frame_0040b090() {}

Component_0040b0f0::Component_0040b0f0() { field8 = 0; owner = 0; }

// vtable 0x4cd454: ctor 0x405e90, dtor 0x405f10, deleting 0x405e70
class Model_00405e90 : public Node_00405160 {
public:
 int pad14;
 struct Part { virtual ~Part(); } *parts;     // 0x18, array-deleted
 void *buffer;                               // 0x1c
 int pad20, count;                           // 0x20, 0x24
 Frame_0040b090 frameA, frameB;              // 0x28, 0x64
 float scale;                                // 0xa0
 Model_00405e90(int ownerObject);
 virtual ~Model_00405e90();
#ifdef TITBIT_SOURCE_LINK
 virtual void Update(int);
 virtual bool RayTest(struct CVec3, struct CVec3, float *);
 virtual float Collide(struct CVec3, struct CVec3, float, float, struct CVec3 *, float *);
#endif
};
// NONMATCHING 0x405e90: original keeps EH states for the member frames; ours has none
Model_00405e90::Model_00405e90(int ownerObject) {
 owner = ownerObject; parts = 0; buffer = 0; count = 0; scale = 1.0f;
}
Model_00405e90::~Model_00405e90() {
 if (parts) delete[] parts;
 if (buffer) ::operator delete(buffer);
}

// vtable 0x4cd480: ctor 0x406a40, dtor 0x406a70, deleting 0x406a20
class Behaviour_00406a40 : public Component_0040b0f0 {
public:
 int fieldC, field10; float rate; int field18;
 Behaviour_00406a40(int ownerObject);
 virtual ~Behaviour_00406a40();
#ifdef TITBIT_SOURCE_LINK
 virtual ObservedString Export();
 virtual void Render(int);
#endif
};
// ctor 0x406a40 lives in behaviour_vc60.cpp: that unit was built with VC6.0 (byte-exact there)
Behaviour_00406a40::~Behaviour_00406a40() {}
