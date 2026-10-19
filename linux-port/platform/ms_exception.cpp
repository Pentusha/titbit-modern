// Bridge for source-built Microsoft-layout C++ objects and Linux DWARF EH.
// Compile this translation unit with the native GNU C++ ABI, always as ELF32.
#include <cstdlib>
#include <cstring>
#include <new>
#include <exception>
#include <stdint.h>

namespace {
struct TypeDescriptor { void *vtable, *spare; char name[1]; };
struct CatchableType {
    int properties;
    TypeDescriptor *type;
    int member_displacement, vbtable_displacement, vbase_displacement;
    int size;
    void *copy;
};
struct CatchableTypes { int count; CatchableType *types[1]; };
struct ThrowInfo { unsigned attributes; void *destroy, *forward; CatchableTypes *types; };
typedef void (__attribute__((thiscall)) *Destructor)(void *);
typedef void (__attribute__((thiscall)) *Copy)(void *, const void *);
typedef void (__attribute__((thiscall)) *VirtualCopy)(void *, const void *, int);
struct Storage { unsigned references; void *object; const ThrowInfo *info; };
const unsigned marker = 0x54424548;

void *adjust(void *object, const CatchableType &type) {
    char *result = static_cast<char *>(object);
    if (type.vbtable_displacement >= 0) {
        const char *table = *reinterpret_cast<const char **>(result + type.vbtable_displacement);
        result += type.vbtable_displacement +
            *reinterpret_cast<const int *>(table + type.vbase_displacement);
    }
    return result + type.member_displacement;
}
void copy_object(void *destination, const void *source, const CatchableType &type) {
    if (type.copy) {
        if (type.properties & 4)
            reinterpret_cast<VirtualCopy>(type.copy)(destination, source, 1);
        else
            reinterpret_cast<Copy>(type.copy)(destination, source);
    } else {
        std::memcpy(destination, source, static_cast<size_t>(type.size));
    }
}
struct MsException {
    unsigned tag;
    Storage *storage;
    explicit MsException(Storage *value) noexcept : tag(marker), storage(value) {}
    MsException(const MsException &other) noexcept : tag(marker), storage(other.storage) {
        ++storage->references;
    }
    ~MsException() noexcept {
        if (--storage->references == 0) {
            if (storage->info->destroy)
                reinterpret_cast<Destructor>(storage->info->destroy)(storage->object);
            std::free(storage->object);
            std::free(storage);
        }
    }
};
}

extern "C" __attribute__((noreturn)) void titbit_CxxThrowException(void *object, const void *metadata) {
    if (!object && !metadata) throw;
    const ThrowInfo *info = static_cast<const ThrowInfo *>(metadata);
    if (!object || !info || !info->types || info->types->count <= 0) std::terminate();
    const CatchableType &type = *info->types->types[0];
    if (type.size <= 0) std::terminate();
    Storage *storage = static_cast<Storage *>(std::malloc(sizeof(Storage)));
    if (!storage) throw std::bad_alloc();
    storage->references = 1;
    storage->info = info;
    storage->object = std::malloc(static_cast<size_t>(type.size));
    if (!storage->object) { std::free(storage); throw std::bad_alloc(); }
    // MSVC constructs the exception temporary before calling this function.
    // Its ownership transfers to the EH runtime; a second copy would leave the
    // compiler's stack temporary (and its refcounted members) undisposed.
    std::memcpy(storage->object, object, static_cast<size_t>(type.size));
    throw MsException(storage);
}

extern "C" bool titbit_eh_match(void *caught, const void *descriptor, int flags, void *destination) {
    if (!descriptor) return true; // catch (...), including native runtime exceptions
    MsException *exception = static_cast<MsException *>(caught);
    if (!exception || exception->tag != marker) return false;
    const TypeDescriptor *wanted = static_cast<const TypeDescriptor *>(descriptor);
    const CatchableTypes &types = *exception->storage->info->types;
    for (int i = 0; i < types.count; ++i) {
        const CatchableType &type = *types.types[i];
        if (std::strcmp(type.type->name, wanted->name)) continue;
        if ((type.properties & 2) && !(flags & 8)) continue;
        void *source = adjust(exception->storage->object, type);
        if (destination) {
            if (flags & 8) *static_cast<void **>(destination) = source;
            else if (type.properties & 1) {
                std::memcpy(destination, source, static_cast<size_t>(type.size));
                if (type.size == sizeof(void *) && *static_cast<void **>(destination))
                    *static_cast<void **>(destination) = adjust(*static_cast<void **>(destination), type);
            } else copy_object(destination, source, type);
        }
        return true;
    }
    return false;
}

extern "C" __attribute__((noreturn)) void titbit_eh_rethrow(void *) { throw; }

// MS TypeDescriptor's vftable is stored but never called by this bridge.
// A genuine deleting destructor is supplied for any explicit type_info cleanup.
extern "C" void *__attribute__((thiscall)) titbit_type_info_delete(void *p, unsigned flags) {
    if (flags & 1) std::free(p);
    return p;
}
extern "C" {
void *titbit_type_info_vtable[] = { reinterpret_cast<void *>(&titbit_type_info_delete) };
}
