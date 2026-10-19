#include <cassert>
#include <cstdio>
extern "C" int callfunc(void *,void *,int);
extern "C" int callmember(void *,void *,int);
static int fail(){throw 42;}
struct Object {int value;};
__attribute__((thiscall)) static int member(Object *value){throw value->value;}
int main(){
 bool caught=false;try{callfunc((void*)fail,nullptr,0);}catch(int value){assert(value==42);caught=true;}assert(caught);
 Object object{87};void *arguments[]={&object};caught=false;
 try{callmember((void*)member,arguments,sizeof(arguments));}catch(int value){assert(value==87);caught=true;}assert(caught);
 std::puts("PASS native C++ exception unwinding through SeeR cdecl/thiscall assembly frames");
}
