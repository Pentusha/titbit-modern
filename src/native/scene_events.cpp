/* Authored batch event dispatch. External script/loader operations are unbuilt. */
#include "picking_control_interfaces.h"
#include "engine_string.h"
#include "event_dispatch.h"
extern "C" void *__cdecl memmove(void *,const void *,unsigned);
template<class T> inline void DestructMessageRange(T *p,int count) {
 for (;count--;p++) p->~T();
}
void ObservedPickingScene::Drain() {
 int initial=message_size;
 for (int i=0;i<initial;++i) {
  ObservedPickingMessage value=message_data[i];
  if (value.selected) {
   if (value.event&0x800) {
    if (value.event==0x800) {
     reserved0=0;
     char *text=(char *)value.selected;
     Load(text);
     selected_object=0;
     ::operator delete(text);
    }
   } else {
    ((ObservedEventScript *)((char *)value.selected+0x44))->Dispatch(&value);
    if (value.event==4) Remove(value.selected);
   }
  }
 }
 DestructMessageRange(message_data,initial);
 ObservedPickingMessage *queued=message_data;
 int remaining=message_size-initial;
 if (remaining)
  memmove(queued,queued+initial,remaining*sizeof(*queued));
 message_size-=initial;
}
