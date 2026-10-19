// Authored native implementation for Clang-generated 32-bit Microsoft RTTI.
// Metadata layout evidence: clang/lib/CodeGen/MicrosoftCXXABI.cpp (LLVM).
// No original executable, Microsoft runtime source, or SDK objects are used.
#include <cstdint>
#include <cstdlib>
#include <cstring>

static_assert(sizeof(void*)==4,"Microsoft RTTI adapter requires ELF32");
extern "C" void *titbit_type_info_vtable[];
extern "C" [[noreturn]] void titbit_CxxThrowException(void*,const void*);

namespace {
struct Type {void *vtable,*cache;char name[1];};
struct PMD {int member,vbptr,vbslot;};
struct Hierarchy;
struct Base {
    Type *type;
    unsigned descendants;
    PMD offset;
    unsigned flags;
    const Hierarchy *hierarchy; // present when flags&64 (Clang/modern MSVC)
};
struct Hierarchy {unsigned signature,flags,count;const Base *const *bases;};
struct Locator {unsigned signature,offset,construction;Type *type;const Hierarchy *hierarchy;};
enum { PrivateOnPath=9, Ambiguous=2, PrivateEdge=4, HasHierarchy=64 };

bool same(const Type *a,const Type *b) {
    return a==b || (a && b && std::strcmp(a->name,b->name)==0);
}
bool valid(const Hierarchy *h) {
    return h && h->bases && h->count>0 && h->count<4096;
}
char *adjust(char *object,const PMD &p) {
    if(p.vbptr>=0) {
        const char *table=*reinterpret_cast<const char *const *>(object+p.vbptr);
        object+=p.vbptr+*reinterpret_cast<const int *>(table+p.vbslot);
    }
    return object+p.member;
}
const Locator *locator(void *object) {
    void *const *table=*static_cast<void *const *const *>(object);
    return reinterpret_cast<const Locator *const *>(table)[-1];
}
char *complete(void *object,const Locator *info) {
    char *result=static_cast<char *>(object)-info->offset;
    if(info->construction)
        result-=*reinterpret_cast<const int *>(static_cast<char *>(object)-info->construction);
    return result;
}
bool public_path(const Hierarchy *h,unsigned root,unsigned child) {
    // Preorder intervals encode the immediate parent without storing pointers.
    // The direct-edge flag remains meaningful even for old VC6 descriptors
    // that do not contain a per-base hierarchy pointer.
    unsigned current=child;
    while(current!=root) {
        if(h->bases[current]->flags&PrivateEdge)return false;
        bool found=false;
        for(unsigned p=current;p>root;) {
            --p;
            if(p+h->bases[p]->descendants>=current) {current=p;found=true;break;}
        }
        if(!found)return false;
    }
    return true;
}
bool contains_source(const Hierarchy *h,char *object,unsigned root,
                     const Type *source,char *source_object) {
    unsigned end=root+h->bases[root]->descendants+1;
    if(end>h->count)return false;
    for(unsigned i=root;i<end;++i) {
        const Base &b=*h->bases[i];
        if(same(b.type,source) && adjust(object,b.offset)==source_object && public_path(h,root,i))
            return true;
    }
    return false;
}
bool target_contains_source(const Hierarchy *whole,char *object,unsigned target,
                            const Type *source,char *source_object) {
    const Base &b=*whole->bases[target];
    if((b.flags&HasHierarchy) && valid(b.hierarchy))
        return contains_source(b.hierarchy,adjust(object,b.offset),0,source,source_object);
    return contains_source(whole,object,target,source,source_object);
}

// Reference-cast and null-typeid failures are thrown with Microsoft-layout
// metadata too, so the same translated typed-catch path handles them.
struct Catchable {int properties;Type *type;PMD offset;int size;void *copy;};
struct Catchables {int count;const Catchable *types[2];};
struct ThrowInfo {unsigned flags;void *destroy,*forward;const Catchables *types;};
struct ErrorObject {void *vtable;const char *message;int owns;};
struct ErrorType {void *vtable,*cache;char name[40];};
void __attribute__((thiscall)) error_destroy(void*) {}
const char *__attribute__((thiscall)) error_what(ErrorObject *p) {return p->message;}
void *__attribute__((thiscall)) error_delete(ErrorObject *p,unsigned flags) {
    if(flags&1)std::free(p);
    return p;
}
void *error_vtable[]={reinterpret_cast<void *>(&error_delete),reinterpret_cast<void *>(&error_what)};
ErrorType exception_type={titbit_type_info_vtable,nullptr,".?AVexception@@"};
ErrorType bad_cast_type={titbit_type_info_vtable,nullptr,".?AVbad_cast@@"};
ErrorType bad_typeid_type={titbit_type_info_vtable,nullptr,".?AVbad_typeid@@"};
Catchable exception_catch={0,reinterpret_cast<Type *>(&exception_type),{0,-1,0},sizeof(ErrorObject),nullptr};
Catchable bad_cast_catch={0,reinterpret_cast<Type *>(&bad_cast_type),{0,-1,0},sizeof(ErrorObject),nullptr};
Catchable bad_typeid_catch={0,reinterpret_cast<Type *>(&bad_typeid_type),{0,-1,0},sizeof(ErrorObject),nullptr};
Catchables bad_cast_catches={2,{&bad_cast_catch,&exception_catch}};
Catchables bad_typeid_catches={2,{&bad_typeid_catch,&exception_catch}};
ThrowInfo bad_cast_throw={0,reinterpret_cast<void *>(&error_destroy),nullptr,&bad_cast_catches};
ThrowInfo bad_typeid_throw={0,reinterpret_cast<void *>(&error_destroy),nullptr,&bad_typeid_catches};
[[noreturn]] void failed_cast() {
    ErrorObject error={error_vtable,"bad cast",0};
    titbit_CxxThrowException(&error,&bad_cast_throw);
}
[[noreturn]] void failed_typeid() {
    ErrorObject error={error_vtable,"bad typeid",0};
    titbit_CxxThrowException(&error,&bad_typeid_throw);
}
}

extern "C" void *__RTDynamicCast(void *input,int vf_delta,void *src,void *dst,int reference) {
    if(!input) {if(reference)failed_cast();return nullptr;}
    Type *source=static_cast<Type *>(src),*target=static_cast<Type *>(dst);
    char *source_object=static_cast<char *>(input)-vf_delta;
    if(same(source,target))return source_object;
    const Locator *info=locator(input);
    if(!info || !valid(info->hierarchy)) {if(reference)failed_cast();return nullptr;}
    char *object=complete(input,info);
    const Hierarchy *h=info->hierarchy;
    char *answer=nullptr;
    bool source_public=false;
    for(unsigned i=0;i<h->count;++i) {
        const Base &b=*h->bases[i];
        if(same(b.type,source) && adjust(object,b.offset)==source_object &&
           !(b.flags&(PrivateOnPath|PrivateEdge)))source_public=true;
        if(same(b.type,target) && target_contains_source(h,object,i,source,source_object)) {
            char *candidate=adjust(object,b.offset);
            if(answer && answer!=candidate) {if(reference)failed_cast();return nullptr;}
            answer=candidate;
        }
    }
    if(answer)return answer;
    if(source_public) {
        for(unsigned i=0;i<h->count;++i) {
            const Base &b=*h->bases[i];
            if(!same(b.type,target) || (b.flags&(PrivateOnPath|PrivateEdge)))continue;
            char *candidate=adjust(object,b.offset);
            if(answer && answer!=candidate) {if(reference)failed_cast();return nullptr;}
            answer=candidate;
        }
    }
    if(!answer && reference)failed_cast();
    return answer;
}
extern "C" void *__RTCastToVoid(void *input) {
    if(!input)return nullptr;
    const Locator *info=locator(input);
    if(!info)failed_typeid();
    return complete(input,info);
}
extern "C" void *__RTtypeid(void *input) {
    if(!input)failed_typeid();
    const Locator *info=locator(input);
    if(!info || !info->type)failed_typeid();
    return info->type;
}
extern "C" int __attribute__((thiscall)) titbit_type_info_equal(void *self,const void *rhs) {
    return same(static_cast<Type *>(self),static_cast<const Type *>(rhs));
}
extern "C" int __attribute__((thiscall)) titbit_type_info_not_equal(void *self,const void *rhs) {
    return !titbit_type_info_equal(self,rhs);
}
extern "C" int __attribute__((thiscall)) titbit_type_info_before(void *self,const void *rhs) {
    return std::strcmp(static_cast<Type *>(self)->name,static_cast<const Type *>(rhs)->name)<0;
}
extern "C" const char *__attribute__((thiscall)) titbit_type_info_raw_name(void *self) {
    return static_cast<Type *>(self)->name;
}
extern "C" const char *__attribute__((thiscall)) titbit_type_info_name(void *self) {
    Type *type=static_cast<Type *>(self);
    if(void *cached=__atomic_load_n(&type->cache,__ATOMIC_ACQUIRE))return static_cast<char *>(cached);
    const char *raw=type->name;
    const char *simple=nullptr;
    struct Fundamental {const char *code,*name;};
    static const Fundamental fundamentals[]={
        {".X","void"},{".D","char"},{".C","signed char"},{".E","unsigned char"},
        {".F","short"},{".G","unsigned short"},{".H","int"},{".I","unsigned int"},
        {".J","long"},{".K","unsigned long"},{".M","float"},{".N","double"},
        {"._J","__int64"},{"._K","unsigned __int64"},{"._N","bool"}};
    for(const Fundamental &f:fundamentals)if(std::strcmp(raw,f.code)==0) {simple=f.name;break;}
    char *decoded=nullptr;
    if(simple) {
        decoded=static_cast<char *>(std::malloc(std::strlen(simple)+1));
        if(decoded)std::strcpy(decoded,simple);
    } else if(std::strncmp(raw,".?AV",4)==0 || std::strncmp(raw,".?AU",4)==0) {
        // Ordinary class/struct components are emitted inner-name first. A
        // template's nested grammar is deliberately left as its stable raw name.
        const char *parts[64];size_t lengths[64];unsigned count=0;
        const char *p=raw+4;
        bool ordinary=true;
        while(*p && *p!='@' && count<64) {
            const char *end=std::strchr(p,'@');
            if(!end || std::memchr(p,'?',end-p)) {ordinary=false;break;}
            parts[count]=p;lengths[count]=end-p;++count;
            p=end+1;
        }
        if(ordinary && count && *p=='@') {
            const char *prefix=raw[3]=='V'?"class ":"struct ";
            size_t size=std::strlen(prefix)+1;
            for(unsigned i=0;i<count;++i)size+=lengths[i]+2;
            decoded=static_cast<char *>(std::malloc(size));
            if(decoded) {
                std::strcpy(decoded,prefix);char *out=decoded+std::strlen(prefix);
                for(unsigned i=count;i>0;--i) {
                    std::memcpy(out,parts[i-1],lengths[i-1]);out+=lengths[i-1];
                    if(i>1) {*out++=':';*out++=':';}
                }
                *out=0;
            }
        }
    }
    if(!decoded)return raw;
    void *expected=nullptr;
    if(!__atomic_compare_exchange_n(&type->cache,&expected,decoded,false,__ATOMIC_RELEASE,__ATOMIC_ACQUIRE)) {
        std::free(decoded);return static_cast<char *>(expected);
    }
    return decoded;
}
extern "C" void __attribute__((thiscall)) titbit_type_info_destruct(void *self) {
    Type *type=static_cast<Type *>(self);
    std::free(type->cache);type->cache=nullptr;
}
