#ifndef TITBIT_PORTABLE_WCHAR_H
#define TITBIT_PORTABLE_WCHAR_H
#include <titbit_crt.h>
TB_EXTERN_C_BEGIN
size_t wcslen(const wchar_t*);wchar_t *wcscpy(wchar_t*,const wchar_t*);wchar_t *wcsncpy(wchar_t*,const wchar_t*,size_t);
int wcscmp(const wchar_t*,const wchar_t*);int wcsncmp(const wchar_t*,const wchar_t*,size_t);wchar_t *wcschr(const wchar_t*,wchar_t);wchar_t *wcsrchr(const wchar_t*,wchar_t);
TB_EXTERN_C_END
#endif
