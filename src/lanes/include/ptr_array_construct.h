#ifndef TITBIT_PTR_ARRAY_CONSTRUCT_H
#define TITBIT_PTR_ARRAY_CONSTRUCT_H
extern "C" void *__cdecl memset(void *, int, unsigned);
// Shared existing source body, original 0x4120d0.
template<class T> inline void __stdcall ConstructElements_004120d0(T *p, int n) {
 if (p && n) memset(p, 0, n * sizeof(T));
}
#endif
