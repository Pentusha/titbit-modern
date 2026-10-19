// Engine scene-object base as the 0x41xxxx units see it (5 virtual slots).
// Same class name as lanes/include/lane_engine.h so ctor 0x4137b0 / dtor
// 0x4137f0 bind; do not include both headers in one unit.
#ifndef E41_ENGINE_H
#define E41_ENGINE_H
#include "e41_string.h"

struct Vec3Z { float x, y, z; Vec3Z() { x = y = z = 0; } };

struct ObjectExtra {                   // +0x10 attachment (Component)
 virtual ObservedString Export();
 virtual void Update(int);            // +0x04
 virtual void Render(int);            // +0x08
 virtual void Release(int);           // +0x0c
};

// vtable 0x4cd5c0: dtor 0x4137d0, GetType 0x413a10, Export 0x413900, Update 0x4138d0, Render 0x4138a0
class EngineObject { // original 0x4137b0
public:
 EngineObject *owner;        // +0x04 (parent)
 int field8;                          // +0x08
 EngineObject *next;         // +0x0c (sibling chain, owned)
 ObjectExtra *extra;                  // +0x10
 EngineObject();
 virtual ~EngineObject();
 virtual int GetType();
 virtual ObservedString Export();
 virtual void Update(int time);
 virtual void Render(int pass);
 void AppendNext_00413820(EngineObject *object);
 void Detach_00413850();
 void Delete_00413f50();
 void AttachExtra_0041e530(ObjectExtra *item);
};
#endif
