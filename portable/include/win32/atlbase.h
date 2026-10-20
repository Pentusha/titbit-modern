#ifndef TITBIT_PORTABLE_ATLBASE_H
#define TITBIT_PORTABLE_ATLBASE_H
/* Project-owned compatibility surface for the small ATL subset used by the
 * reconstructed application. This is not an ATL SDK implementation. Windows
 * declarations come from the public mingw-w64 headers. Object sizes are part
 * of the recovered 32-bit game ABI and are checked below. */
#include <windows.h>
#include <objbase.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#define ATL_NO_VTABLE __declspec(novtable)
#define ATLASSERT(condition) ((void)0)
#define ATLTRY(statement) try { statement; } catch (...) { }
namespace ATL {
struct _ATL_OBJMAP_ENTRY {
    const CLSID *pclsid;
    void *update_registry, *get_class_object, *create_instance, *factory;
    DWORD registration;
    void *description, *category_map;
    void (__stdcall *object_main)(bool);
};
struct _ATL_TERM_ENTRY {
    void (__stdcall *callback)(DWORD);
    DWORD argument;
    _ATL_TERM_ENTRY *next;
};
struct _ATL_MODULE {
    DWORD cbSize;
    HINSTANCE m_hInst, m_hInstResource, m_hInstTypeLib;
    _ATL_OBJMAP_ENTRY *m_pObjMap;
    LONG m_nLockCnt;
    HANDLE m_hHeap;
    CRITICAL_SECTION m_csTypeInfoHolder, m_csWindowCreate, m_csObjMap;
    DWORD m_dwAtlBuildVer;
    void *m_pCreateWndList;
    bool m_bDestroyHeap;
    const GUID *m_pguidVer;
    DWORD m_dwHeaps;
    HANDLE *m_phHeaps;
    DWORD m_dwThreadID;
    _ATL_TERM_ENTRY *m_pTermFuncs;
};
typedef char module_size_is_132[(sizeof(_ATL_MODULE)==132)?1:-1];

inline HRESULT __stdcall AtlModuleTerm(_ATL_MODULE *module) {
    if (!module) return E_INVALIDARG;
    while (module->m_pTermFuncs) {
        _ATL_TERM_ENTRY *entry=module->m_pTermFuncs;
        module->m_pTermFuncs=entry->next;
        entry->callback(entry->argument);
        delete entry;
    }
    if (module->m_pObjMap) {
        for (_ATL_OBJMAP_ENTRY *entry=module->m_pObjMap; entry->pclsid; ++entry)
            if (entry->object_main) entry->object_main(false);
    }
    if (module->m_phHeaps) {
        for (DWORD i=0;i<=module->m_dwHeaps;++i)
            if (module->m_phHeaps[i]) HeapDestroy(module->m_phHeaps[i]);
        HeapFree(module->m_hHeap,0,module->m_phHeaps);
        module->m_phHeaps=0;
    }
    DeleteCriticalSection(&module->m_csTypeInfoHolder);
    DeleteCriticalSection(&module->m_csWindowCreate);
    DeleteCriticalSection(&module->m_csObjMap);
    if (module->m_bDestroyHeap && module->m_hHeap) HeapDestroy(module->m_hHeap);
    module->m_hHeap=0;
    return S_OK;
}
class CComModule : public _ATL_MODULE {
public:
    HRESULT Init(_ATL_OBJMAP_ENTRY *map,HINSTANCE instance,const GUID *libid=0) {
        (void)libid;
        memset(static_cast<_ATL_MODULE*>(this),0,sizeof(_ATL_MODULE));
        cbSize=sizeof(_ATL_MODULE);
        m_hInst=m_hInstResource=m_hInstTypeLib=instance;
        m_pObjMap=map;
        m_dwAtlBuildVer=0x300;
        m_bDestroyHeap=true;
        InitializeCriticalSection(&m_csTypeInfoHolder);
        InitializeCriticalSection(&m_csWindowCreate);
        InitializeCriticalSection(&m_csObjMap);
        m_hHeap=HeapCreate(0,0,0);
        if (!m_hHeap) {
            DeleteCriticalSection(&m_csTypeInfoHolder);
            DeleteCriticalSection(&m_csWindowCreate);
            DeleteCriticalSection(&m_csObjMap);
            return E_OUTOFMEMORY;
        }
        if (map) for (_ATL_OBJMAP_ENTRY *entry=map; entry->pclsid; ++entry)
            if (entry->object_main) entry->object_main(true);
        return S_OK;
    }
    void Term() { AtlModuleTerm(this); }
    HINSTANCE GetModuleInstance() const { return m_hInst; }
    HINSTANCE GetResourceInstance() const { return m_hInstResource; }
    HINSTANCE SetResourceInstance(HINSTANCE value) { HINSTANCE old=m_hInstResource;m_hInstResource=value;return old; }
};

template<class T> class CSimpleArray {
public:
    T *m_aT;
    int m_nSize,m_nAllocSize;
    CSimpleArray():m_aT(0),m_nSize(0),m_nAllocSize(0) { }
    ~CSimpleArray() { RemoveAll(); }
    int GetSize() const { return m_nSize; }
    T& operator[](int index) { return m_aT[index]; }
    const T& operator[](int index) const { return m_aT[index]; }
    bool Add(const T& value) {
        if (m_nSize==m_nAllocSize) {
            int capacity=m_nAllocSize?m_nAllocSize*2:4;
            T *data=static_cast<T*>(::operator new(capacity*sizeof(T)));
            int i=0;
            try { for (;i<m_nSize;++i) new(data+i) T(m_aT[i]); }
            catch (...) { while(i) data[--i].~T();::operator delete(data);throw; }
            for(i=0;i<m_nSize;++i)m_aT[i].~T();
            ::operator delete(m_aT);m_aT=data;m_nAllocSize=capacity;
        }
        new(m_aT+m_nSize) T(value);++m_nSize;return true;
    }
    void RemoveAll() {
        for(int i=0;i<m_nSize;++i)m_aT[i].~T();
        ::operator delete(m_aT);m_aT=0;m_nSize=m_nAllocSize=0;
    }
private:
    CSimpleArray(const CSimpleArray&);
    CSimpleArray& operator=(const CSimpleArray&);
};
template<class K,class V> class CSimpleMap {
public:
    K *m_aKey;
    V *m_aVal;
    int m_nSize;
    CSimpleMap():m_aKey(0),m_aVal(0),m_nSize(0) { }
    ~CSimpleMap() { RemoveAll(); }
    int GetSize() const { return m_nSize; }
    int FindKey(const K& key) const { for(int i=0;i<m_nSize;++i)if(m_aKey[i]==key)return i;return -1; }
    V Lookup(const K& key) const { int i=FindKey(key);return i<0?V():m_aVal[i]; }
    bool Add(const K& key,const V& value) {
        K *keys=new K[m_nSize+1];V *values=0;
        try { values=new V[m_nSize+1];for(int i=0;i<m_nSize;++i){keys[i]=m_aKey[i];values[i]=m_aVal[i];}keys[m_nSize]=key;values[m_nSize]=value; }
        catch (...) { delete[]keys;delete[]values;throw; }
        delete[]m_aKey;delete[]m_aVal;m_aKey=keys;m_aVal=values;++m_nSize;return true;
    }
    bool Remove(const K& key) { int i=FindKey(key);if(i<0)return false;for(;i<m_nSize-1;++i){m_aKey[i]=m_aKey[i+1];m_aVal[i]=m_aVal[i+1];}--m_nSize;return true; }
    void RemoveAll() { delete[]m_aKey;delete[]m_aVal;m_aKey=0;m_aVal=0;m_nSize=0; }
private:
    CSimpleMap(const CSimpleMap&);
    CSimpleMap& operator=(const CSimpleMap&);
};
}
#endif
