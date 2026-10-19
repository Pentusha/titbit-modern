// Object component base (vtable 0x4cd530: Export, Update, Render, dtor):
// the empty XML export and LINK.Delete for components.
#include "../engine41/e41_engine.h"

class Component_0040b0f0 {
public:
 EngineObject *owner;        // +0x04
 Component_0040b0f0 *next;            // +0x08
 Component_0040b0f0();
 virtual ObservedString Export();
 virtual void Update(int time);
 virtual void Render(int pass);
 virtual ~Component_0040b0f0();
 void Unlink_0040b2a0();
 void Delete_0040b2d0();
};

ObservedString Component_0040b0f0::Export() {
 ObservedString xml;
 return xml;
}

void Component_0040b0f0::Delete_0040b2d0() {
 Unlink_0040b2a0();
 if (this) delete this;
}

// Native 0x40b150, walk the component chain using Update slot +4.
void Component_0040b0f0::Update(int time){if(next)next->Update(time);}

// Native 0x40b140, Render slot +8.
void Component_0040b0f0::Render(int pass){if(next)next->Render(pass);}
