#include "picking_control_interfaces.h"

template<class T>inline void ConstructPointerElements(T *p,int count){
 if(p && count){memset(p,0,count*sizeof(T));}
}
template<class T>void ObservedPointerArray<T>::SetSize(int new_size,int grow){
 if(new_size<0)return;
 if(grow!=-1)grow_by=grow;
 if(new_size==0){if(data){::operator delete(data);data=0;}size=capacity=0;}
 else if(data==0){data=(T*)::operator new(new_size*sizeof(T));ConstructPointerElements(data,new_size);size=capacity=new_size;}
 else if(new_size<=capacity){if(new_size>size)ConstructPointerElements(data+size,new_size-size);size=new_size;}
 else{
  int growth=grow_by;
  if(!growth){growth=size/8;growth=growth<4?4:(growth>1024?1024:growth);}
  int new_capacity=(new_size<capacity+growth)?capacity+growth:new_size;
  T *next=(T*)::operator new(new_capacity*sizeof(T));
  memcpy(next,data,size*sizeof(T));
  if(new_size>size)ConstructPointerElements(next+size,new_size-size);
  ::operator delete(data);data=next;size=new_size;capacity=new_capacity;
 }
}
template struct ObservedPointerArray<ObservedPickingNode *>;
template struct ObservedPointerArray<ObservedSpatialObject *>;
