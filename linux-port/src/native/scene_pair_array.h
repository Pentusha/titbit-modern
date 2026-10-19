#ifndef TITBIT_SCENE_PAIR_ARRAY_H
#define TITBIT_SCENE_PAIR_ARRAY_H
#include "engine_string.h"
/* Authored two-field element view. This does not redefine the separately
 * source-built ObservedStringPair or its external destructor contract. */
struct ScenePairElement {ObservedString first,second;};
struct ScenePairStorage {
 ScenePairElement* data;int size,capacity,grow;
 ScenePairStorage(){data=0;size=capacity=grow=0;}
 ~ScenePairStorage(){
  if(data){ScenePairElement* p=data;int n=size;
   while(n--){p->~ScenePairElement();++p;}
   ::operator delete(data);
  }
 }
};
/* Twenty-byte source view, not proof of the original complete class extent.
 * No native Scene/global storage is allocated by this declaration. */
class ScenePairArray {
 ScenePairStorage storage;
public:
 ScenePairArray();virtual ~ScenePairArray();
};
#endif
