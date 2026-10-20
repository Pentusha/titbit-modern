/* Project-owned declarations for the 32-bit source ABI. No SDK implementation. */
#ifndef TITBIT_PORTABLE_CRT_H
#define TITBIT_PORTABLE_CRT_H
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif
#ifndef _PTRDIFF_T_DEFINED
#define _PTRDIFF_T_DEFINED
typedef int ptrdiff_t;
#endif
#ifndef __cplusplus
#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#endif
#endif
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif
#ifndef _CRTIMP
#define _CRTIMP
#endif
#ifndef _CRTIMP2
#define _CRTIMP2
#endif
#ifdef __cplusplus
#define TB_EXTERN_C_BEGIN extern "C" {
#define TB_EXTERN_C_END }
#else
#define TB_EXTERN_C_BEGIN
#define TB_EXTERN_C_END
#endif
#define _MAX_PATH 260
#define _MAX_DRIVE 3
#define _MAX_DIR 256
#define _MAX_FNAME 256
#define _MAX_EXT 256
#endif
