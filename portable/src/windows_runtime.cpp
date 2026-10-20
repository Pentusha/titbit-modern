/* Source-owned support for the recovered C++ object ABI on modern Windows.
 * Allocation, exception propagation and C runtime come from the current UCRT
 * and VCRuntime; no VC6 runtime libraries or implementation headers are used. */
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <new>
#include <windows.h>
#include <objbase.h>

static char *copy_message(const char *message) {
    if (!message) return 0;
    size_t length=strlen(message)+1;
    char *copy=static_cast<char*>(malloc(length));
    if (copy) memcpy(copy,message,length);
    return copy;
}
exception::exception():_m_what(0),_m_doFree(0) { }
exception::exception(const __exString& message):_m_what(copy_message(message)),_m_doFree(_m_what!=0) { }
exception::exception(const exception& other):_m_what(copy_message(other._m_what)),_m_doFree(_m_what!=0) { }
exception& exception::operator=(const exception& other) {
    if(this!=&other) {
        char *replacement=copy_message(other._m_what);
        if(_m_doFree) free(const_cast<char*>(_m_what));
        _m_what=replacement;_m_doFree=replacement!=0;
    }
    return *this;
}
exception::~exception() {if(_m_doFree)free(const_cast<char*>(_m_what));}
__exString exception::what() const {return _m_what?_m_what:"Unknown exception";}
extern "C" bool __cdecl __uncaught_exception();
namespace std {
bool __cdecl uncaught_exception() {return ::__uncaught_exception();}
void __cdecl _Xlen() {throw length_error("string too long");}
void __cdecl _Xran() {throw out_of_range("invalid string position");}
void __cdecl terminate() {::abort();}
}
void *operator new(size_t size) {void *p=malloc(size?size:1);if(!p)throw std::bad_alloc();return p;}
void *operator new[](size_t size) {return ::operator new(size);}
void operator delete(void *p) throw() {free(p);}
void operator delete[](void *p) throw() {free(p);}

/* Modern Windows SDKs omit the obsolete dinput.lib, although Windows still
 * provides DirectInput5 in dinput.dll. Resolve the documented entry point at
 * runtime and retain the exact import-pointer ABI consumed by the game. */
typedef HRESULT (WINAPI *InputCreateFunction)(HINSTANCE,DWORD,void**,IUnknown*);
static HRESULT WINAPI create_direct_input(HINSTANCE instance,DWORD version,void **result,IUnknown *outer) {
    if(!result) return E_POINTER;
    *result=0;
    HMODULE library=GetModuleHandleA("dinput.dll");
    if(!library) library=LoadLibraryA("dinput.dll");
    if(!library) return HRESULT_FROM_WIN32(GetLastError());
    InputCreateFunction create=reinterpret_cast<InputCreateFunction>(GetProcAddress(library,"DirectInputCreateA"));
    if(!create) return HRESULT_FROM_WIN32(GetLastError());
    return create(instance,version,result,outer);
}
extern "C" {
extern const char titbit_empty_text[1]={0};
HRESULT WINAPI DirectInputCreateA(HINSTANCE instance,DWORD version,void **result,IUnknown *outer) {
    return create_direct_input(instance,version,result,outer);
}
InputCreateFunction titbit_direct_input_import __asm__("__imp__DirectInputCreateA@16")=&create_direct_input;
}
