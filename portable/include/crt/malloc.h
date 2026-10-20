#include <stdlib.h>
#ifdef TITBIT_WINDOWS_UCRT
TB_EXTERN_C_BEGIN
void *_aligned_malloc(size_t,size_t);
void _aligned_free(void*);
TB_EXTERN_C_END
#define __mingw_aligned_malloc _aligned_malloc
#define __mingw_aligned_free _aligned_free
#endif
#define _alloca __builtin_alloca
#define alloca __builtin_alloca
