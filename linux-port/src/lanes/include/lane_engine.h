// Engine object model (claude lane). Class names are authored; the address in
// each name is the constructor (or first method) so tools/match.py binds it.
#ifndef LANE_ENGINE_H
#define LANE_ENGINE_H
void *__cdecl operator new(unsigned);
void __cdecl operator delete(void *);

class ObservedString;

struct Vec3Z { float x, y, z; Vec3Z() { x = y = z = 0; } };

// vtable 0x4cd5c0: ctor 0x4137b0, dtor 0x4137f0
class EngineObject { // original 0x4137b0
public:
 int owner, field8;
 EngineObject *child;        // deleted through slot 0
 struct Releasable { virtual void a(); virtual void b(); virtual void c(); virtual void Release(int); } *extra;
 EngineObject();
 virtual ~EngineObject();
#ifdef TITBIT_SOURCE_LINK
 virtual int GetType();
 virtual ObservedString Export();
 virtual void Update(int);
 virtual void Render(int);
#else
 virtual void Slot1();
 virtual void Slot2();
 virtual void Render();
#endif
};

#ifdef TITBIT_SOURCE_LINK
struct CVec3;
#endif
// vtable 0x4cd40c: ctor 0x405160, dtor 0x4051a0, deleting 0x405180
class Node_00405160 : public EngineObject {
public:
 Node_00405160();
 virtual ~Node_00405160();
#ifdef TITBIT_SOURCE_LINK
 virtual int GetType();
 virtual bool RayTest(struct CVec3, struct CVec3, float *);
 virtual void Slot6();
 virtual float Collide(struct CVec3, struct CVec3, float, float, struct CVec3 *, float *);
#endif
};

// vtable 0x4cd52c: ctor 0x40b090, dtor 0x40b0e0 (0x3c bytes)
class Frame_0040b090 {
public:
 float scale; Vec3Z a, b; float w; Vec3Z c; int tail[3];
 Frame_0040b090();
 virtual ~Frame_0040b090();
};

// vtable 0x4cd530: ctor 0x40b0f0, dtor 0x40b130
class Component_0040b0f0 {
public:
 int owner, field8;
 Component_0040b0f0();
#ifdef TITBIT_SOURCE_LINK
 virtual ObservedString Export();
 virtual void Update(int);
 virtual void Render(int);
#endif
 virtual ~Component_0040b0f0();
};
#endif
