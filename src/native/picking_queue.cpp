/* Authored Scene queue/lookup reconstruction. The array growth algorithm is
 * corroborated by the installed historical MFC AFXTEMPL.H SetSize source;
 * negative-size, null/count guards and message construction are target-specific.
 * No vendor binary objects/libraries or original bytes are build inputs. */
#include "picking_control_interfaces.h"
extern "C" void *__cdecl memcpy(void *, const void *, unsigned);
extern "C" int __cdecl strcmp(const char *, const char *);
#pragma intrinsic(memcpy, strcmp)
void *__cdecl operator new(unsigned);
void __cdecl operator delete(void *);
inline void *__cdecl operator new(unsigned, void *p) {return p;}

struct ConstructedPickingMessage : ObservedPickingMessage {
 ConstructedPickingMessage() {memset(this, 0, sizeof(*this));}
};
void __stdcall ConstructPickingMessages(ObservedPickingMessage *p, int count) {
 if (p && count) {
  memset(p, 0, count * sizeof(*p));
  for (; count--; ++p)
   ::new((void *)p) ConstructedPickingMessage;
 }
}

bool ObservedPickingScene::Send(ObservedPickingMessage *input) {
 if (!input->selected) {
  input->selected = Resolve(input);
  if (!input->selected) return false;
 }
 ObservedPickingMessage value = *input;
 int index = message_size;
 int new_size = index + 1;
 if (new_size >= 0) {
  if (new_size == 0) {
   if (message_data) {
    ::operator delete(message_data);
    message_data = 0;
   }
   message_size = message_capacity = 0;
  } else if (!message_data) {
   message_data = (ObservedPickingMessage *)::operator new(new_size * sizeof(*message_data));
   ConstructPickingMessages(message_data, new_size);
   message_size = message_capacity = new_size;
  } else if (new_size <= message_capacity) {
   if (new_size > message_size)
    ConstructPickingMessages(&message_data[message_size], new_size - message_size);
   message_size = new_size;
  } else {
   int grow = message_grow_by;
   if (!grow) {
    grow = message_size / 8;
    grow = grow < 4 ? 4 : (grow > 1024 ? 1024 : grow);
   }
   int capacity;
   if (new_size < message_capacity + grow)
    capacity = message_capacity + grow;
   else capacity = new_size;
   ObservedPickingMessage *data = (ObservedPickingMessage *)::operator new(capacity * sizeof(*data));
   memcpy(data, message_data, message_size * sizeof(*data));
   if (new_size > message_size)
    ConstructPickingMessages(&data[message_size], new_size - message_size);
   ::operator delete(message_data);
   message_data = data;
   message_size = new_size;
   message_capacity = capacity;
  }
 }
 message_data[index] = value;
 return true;
}

ObservedPickingNode *ObservedPickingScene::Resolve(ObservedPickingMessage *message) {
 for (int i = 0; i < lists[0].size; ++i) {
  ObservedPickingNode *p = Find(lists[0].data[i], (const char *)message->reserved0);
  if (p) return p;
 }
 for (i = 0; i < lists[1].size; ++i) {
  ObservedPickingNode *p = Find(lists[1].data[i], (const char *)message->reserved0);
  if (p) return p;
 }
 for (i = 0; i < lists[2].size; ++i) {
  ObservedPickingNode *p = Find(lists[2].data[i], (const char *)message->reserved0);
  if (p) return p;
 }
 return 0;
}

ObservedPickingNode *ObservedPickingScene::Find(ObservedPickingNode *p, const char *name) {
 while (p) {
  if (p->Type() == 0 && !strcmp(p->name, name)) return p;
  p = p->next;
 }
 return 0;
}
