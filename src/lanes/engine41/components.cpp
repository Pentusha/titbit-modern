// Object components (attachments, vtable layout as Component 0x4cd530:
// Export, Update, Render, dtor) used by the 0x41xxxx units.
#include "e41_engine.h"

class Component_0040b0f0 {
public:
 EngineObject *owner;        // +0x04
 Component_0040b0f0 *next;            // +0x08
 Component_0040b0f0();
 virtual ObservedString Export();
 virtual void Update(int time);
 virtual void Render(int pass);
 virtual ~Component_0040b0f0();
};

// vtable 0x4cd5ec: dtor 0x415e30, deleting 0x415e10, Render 0x415e60
class Behaviour : public Component_0040b0f0 { // original 0x415e40
public:
 int fieldC; // native Render clears +0x0c; allocation extent is 0x10
 Behaviour(EngineObject *owner);
 virtual void Render(int pass);
 virtual ~Behaviour();
};

Behaviour::Behaviour(EngineObject *owner_) { owner = owner_; }
Behaviour::~Behaviour() {}

#include "behaviour_motion.h"
