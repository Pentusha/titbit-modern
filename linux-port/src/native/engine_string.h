#ifndef TITBIT_ENGINE_STRING_H
#define TITBIT_ENGINE_STRING_H
/* Observed four-byte string handle / 12-byte shared header. These authored
 * views do not infer complete original class names or global initialization. */
extern "C" {
 __declspec(dllimport) long __stdcall InterlockedDecrement(long *);
 __declspec(dllimport) long __stdcall InterlockedIncrement(long *);
 __declspec(dllimport) char *__stdcall CharLowerA(char *);
 __declspec(dllimport) int __stdcall lstrlenA(const char *);
 __declspec(dllimport) int __stdcall LoadStringA(void *, unsigned, char *, int);
 void *__cdecl memcpy(void *, const void *, unsigned);
}
#pragma intrinsic(memcpy)
void *__cdecl operator new(unsigned);
void __cdecl operator delete(void *);
struct ObservedStringHeader {long refs;int length,capacity;};
extern char *observed_empty_string;
extern ObservedStringHeader *observed_empty_header;
#include "engine_module_resource.h"
int __stdcall ObservedLoadString(unsigned,char *,unsigned);
class ObservedString {
 char *data;
 ObservedStringHeader *Header() const {return (ObservedStringHeader *)data-1;}
 void Init() {data=observed_empty_string;}
 void Release();
 int Allocate(int length) {
  if (!length) {data=observed_empty_string;return true;}
  ObservedStringHeader *p=(ObservedStringHeader *)::operator new(length+13);
  if (!p) return false;
  p->refs=1;
  ((char *)(p+1))[length]=0;
  p->length=length;p->capacity=length;data=(char *)(p+1);return true;
 }
 /* Keep the copy and allocation layers separate: their inline expansion
  * reproduces the historical assignment's effective-address scheduling. */
 inline int BeforeWrite(int length) {
  if (Header()->refs>1 || length>Header()->capacity) {
   Release();return Allocate(length);
  }
  return true;
 }
 inline void AssignCopy(int length,const char *text) {
  if (BeforeWrite(length)) {
   memcpy(data,text,length);Header()->length=length;data[length]=0;
  }
 }
public:
 /* Historical BOOL/EAX allocation gates and archive callsite inline forms.
  * Class and method identifiers are authored views of the observed ABI. */
 static int __stdcall SafeStrlen(const char *text){return text==0?0:lstrlenA(text);}
 ObservedString &operator=(const ObservedString &other){
  if(data!=other.data){
   if((Header()->refs<0 && Header()!=observed_empty_header)||other.Header()->refs<0)
    AssignCopy(other.Header()->length,other.data);
   else{Release();data=other.data;InterlockedIncrement(&Header()->refs);}
  }
  return *this;
 }
 ObservedString &operator=(const char *text){AssignCopy(SafeStrlen(text),text);return *this;}
 void CopyBeforeWrite(){
  if(Header()->refs>1){
   ObservedStringHeader *old=Header();
   Release();
   if(Allocate(old->length))memcpy(data,(char*)(old+1),old->length+1);
  }
 }
 void MakeLower(){CopyBeforeWrite();CharLowerA(data);}
 int GetLength()const{return Header()->length;}
 void FormatV(const char*,char*);
 ObservedString(){Init();}
 #if defined(OBSERVED_STRING_INLINE_COPY) && !defined(TITBIT_SOURCE_LINK)
 ObservedString(const ObservedString&other){
  if(other.Header()->refs<0){Init();*this=(const char*)other.data;}
  else{data=other.data;InterlockedIncrement(&Header()->refs);}
 }
 #else
 ObservedString(const ObservedString&);
 #endif
 #ifdef OBSERVED_STRING_WTL_LOADSTRING
 // WTL layout: the constructor calls the LoadString member (0x406520). Opt-in per
 // unit; define OBSERVED_STRING_LOADSTRING_INLINE as well where the original unit
 // expanded LoadString inline. Units without the macro keep the old expansion.
 int LoadString(unsigned id);
 ObservedString(const char *text) {
  data=observed_empty_string;
  if (text) {
   if ((unsigned short)((unsigned long)text>>16)==0) LoadString((unsigned)text&0xffff);
   else {
    int length=lstrlenA(text);
    if (length && Allocate(length)) memcpy(data,text,length);
   }
  }
 }
 #else
 ObservedString(const char *text) {
  data=observed_empty_string;
  if (text) {
   if ((unsigned short)((unsigned long)text>>16)==0) {
    unsigned id=(unsigned)text&0xffff;
    char temporary[256];
    int length=LoadStringA(observed_resource_instance,id,temporary,256);
    if (!length) temporary[0]=0;
    if (256-length>2) Assign(temporary);
    else {
     int capacity=256;
     do {capacity+=256;length=ObservedLoadString(id,GetBuffer(capacity-1),capacity);}
     while (capacity-length<=2);
     ReleaseBuffer(-1);
    }
   } else {
    int length=lstrlenA(text);
    if (length && Allocate(length)) memcpy(data,text,length);
   }
  }
 }
 #endif
 __forceinline ~ObservedString() {
  if (Header()!=observed_empty_header && InterlockedDecrement(&Header()->refs)<=0)
   delete (unsigned char*)Header();
 }
 operator const char *()const{return data;}
 ObservedString &Assign(const char *);
 #if defined(OBSERVED_STRING_WTL_BUFFER) && !defined(TITBIT_SOURCE_LINK)
 // WTL CString::GetBuffer / ReleaseBuffer expanded inline (AllocBuffer 0x402120,
 // static Release(CStringData*) 0x406700, out-of-line CopyBeforeWrite 0x4021c0).
 static void __stdcall ReleaseHeader(ObservedStringHeader *header);
 void CopyBeforeWriteOutOfLine();
 char *GetBuffer(int min_length) {
  if (Header()->refs > 1 || min_length > Header()->capacity) {
   ObservedStringHeader *old = Header();
   int old_length = old->length;
   if (min_length < old_length) min_length = old_length;
   if (!Allocate(min_length)) return data;
   memcpy(data, (char *)(old + 1), old_length + 1);
   Header()->length = old_length;
   ReleaseHeader(old);
  }
  return data;
 }
 void ReleaseBuffer(int new_length = -1) {
  CopyBeforeWriteOutOfLine();
  if (new_length == -1) new_length = lstrlenA(data);
  Header()->length = new_length;
  data[new_length] = 0;
 }
 #else
 char *GetBuffer(int);
 void ReleaseBuffer(int);
 #endif
};
#if defined(OBSERVED_STRING_WTL_LOADSTRING) && defined(OBSERVED_STRING_LOADSTRING_INLINE)
inline int ObservedString::LoadString(unsigned id) {
  char temporary[256];
  int length=LoadStringA(observed_resource_instance,id,temporary,256);
  if (!length) temporary[0]=0;
  if (256-length>2) { Assign(temporary); return length>0; }
  int capacity=256;
  do {capacity+=256;length=ObservedLoadString(id,GetBuffer(capacity-1),capacity);}
  while (capacity-length<=2);
  ReleaseBuffer(-1);
  return length>0;
 }
#endif
struct ObservedStringPair {
 ObservedString first,second;
 ~ObservedStringPair();
};
#ifndef OBSERVED_STRING_RELEASE_EXTERNAL
#include "engine_string_release.inl"
#endif
#endif
