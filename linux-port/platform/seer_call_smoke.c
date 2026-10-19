#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
extern int callfunc(void *,void *,int);
extern int callmember(void *,void *,int);
extern double callfunc_d(void *,void *,int);
extern double callmember_d(void *,void *,int);
extern uint32_t _msvc_regEDX;
__attribute__((noinline)) static uint64_t wide_result(uint32_t a,uint32_t b){return ((uint64_t)b<<32)|a;}
__attribute__((noinline)) static int sum(int a,int b,int c){return a+2*b+3*c;}
__attribute__((noinline)) static int zero(void){return 91;}
__attribute__((noinline)) static double add_double(double a,double b){return a+b;}
typedef struct {int value;} Object;
__attribute__((thiscall,noinline)) static int member(Object *object,int a,int b){return object->value+a*2+b*3;}
__attribute__((thiscall,noinline)) static double member_double(Object *object,double a,double b){return object->value+a+b;}
int main(void){
 int args[]={3,5,7};Object object={11};uintptr_t members[]={(uintptr_t)&object,5,7};
 assert(callfunc(zero,NULL,0)==91);assert(callfunc(sum,args,sizeof(args))==34);
 uint32_t words[]={0x89abcdefu,0x12345678u};assert((uint32_t)callfunc(wide_result,words,sizeof(words))==words[0]);assert(_msvc_regEDX==words[1]);
 assert(callmember(member,members,sizeof(members))==42);
 double values[]={1.25,3.5};assert(fabs(callfunc_d(add_double,values,sizeof(values))-4.75)<1e-12);
 unsigned char dmember[4+sizeof(values)];memcpy(dmember,&members[0],4);memcpy(dmember+4,values,sizeof(values));
 assert(fabs(callmember_d(member_double,dmember,sizeof(dmember))-15.75)<1e-12);
 volatile uint32_t guard=0xdeadbeef;uintptr_t before,after;
 __asm__ volatile("movl %%esp,%0":"=r"(before)::"memory");
 for(unsigned n=0;n<100000;n++){assert(callfunc(sum,args,sizeof(args))==34);assert(callmember(member,members,sizeof(members))==42);}
 __asm__ volatile("movl %%esp,%0":"=r"(after)::"memory");
 assert(before==after&&guard==0xdeadbeef);puts("PASS i386 SeeR cdecl/thiscall, EAX:EDX, x87 double, 200000 calls with balanced ESP");return 0;
}
