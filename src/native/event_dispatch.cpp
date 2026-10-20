/* Authored event-to-SeeR bridge. Preserve native switch order and C++ EH. */
#include "event_dispatch.h"
extern "C" int __cdecl scCall_Instance(void *,int,...);
void __cdecl PickingTrace(const char *,...);
struct DispatchVector {
 float x,y,z;
#ifndef TITBIT_PORTABLE_HEADERS
 DispatchVector(float a,float b,float c):x(a),y(b),z(c){}
#endif
};
void ObservedEventScript::Dispatch(ObservedPickingMessage *value) {
 if (!instance || handlers[value->event]==-1) return;
 try {
  switch(value->event) {
  case 3:PickingTrace("EM_CREATE");scCall_Instance(instance,handlers[3]);break;
  case 4:PickingTrace("EM_DESTROY");scCall_Instance(instance,handlers[4]);break;
  case 5:scCall_Instance(instance,handlers[5],value->reserved28[1]);break;
  case 2:
#ifdef TITBIT_PORTABLE_HEADERS
   { DispatchVector position={value->floats[0],value->floats[1],value->floats[2]}; scCall_Instance(instance,handlers[2],position); }break;
#else
   scCall_Instance(instance,handlers[2],DispatchVector(value->floats[0],value->floats[1],value->floats[2]));break;
#endif
  case 8:PickingTrace("EM_LBUTTONDOWN");scCall_Instance(instance,handlers[8]);break;
  case 9:PickingTrace("EM_LBUTTONUP");scCall_Instance(instance,handlers[9]);break;
  case 10:PickingTrace("EM_SETFOCUS");scCall_Instance(instance,handlers[10]);break;
  case 11:PickingTrace("EM_KILLFOCUS");scCall_Instance(instance,handlers[11]);break;
  case 12:PickingTrace("EM_NOTIFY");scCall_Instance(instance,handlers[12],value->reserved28[0]);break;
  }
 }catch(...){PickingTrace("Exception in script");}
}
