// Force callable providers for the existing inline ObservedString bodies.
// Explicit template instantiation can name a private member; the exported
// member pointers make the compiler emit its real source definition.
#include "../native/engine_string.h"
#include "../lanes/include/ptr_array_construct.h"

struct AllocateMember {
 typedef int (ObservedString::*type)(int);
 friend type member(AllocateMember);
};
struct ReleaseMember {
 typedef void (ObservedString::*type)();
 friend type member(ReleaseMember);
};
template<class Tag,typename Tag::type Function> struct EmitMember {
 friend typename Tag::type member(Tag) {return Function;}
};
template struct EmitMember<AllocateMember,&ObservedString::Allocate>;
template struct EmitMember<ReleaseMember,&ObservedString::Release>;
extern "C" {
 __attribute__((used)) AllocateMember::type titbit_keep_string_allocate=member(AllocateMember());
 __attribute__((used)) ReleaseMember::type titbit_keep_string_release=member(ReleaseMember());
}

class SceneObject;
void __stdcall ConstructPointers_004120d0(SceneObject **p,int n) {
 ConstructElements_004120d0(p,n);
}
