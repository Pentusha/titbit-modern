#ifndef TITBIT_POINTER_ARRAY_H
#define TITBIT_POINTER_ARRAY_H
void *__cdecl operator new(unsigned);void __cdecl operator delete(void *);
extern "C" void *__cdecl memcpy(void *,const void *,unsigned);
#pragma intrinsic(memcpy)
template<class T>struct ObservedPointerArray {
 T *data;int size,capacity,grow_by;
 ObservedPointerArray(){data=0;size=capacity=grow_by=0;}
 ~ObservedPointerArray(){if(data)::operator delete(data);}
 void SetSize(int,int=-1);
 void RemoveAll(){if(data){::operator delete(data);data=0;}size=capacity=0;}
};
#endif
