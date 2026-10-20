#ifndef TITBIT_PORTABLE_STRING_H
#define TITBIT_PORTABLE_STRING_H
#include <titbit_crt.h>
#include <wchar.h>
TB_EXTERN_C_BEGIN
void *memcpy(void*,const void*,size_t);void *memmove(void*,const void*,size_t);void *memset(void*,int,size_t);
int memcmp(const void*,const void*,size_t);void *memchr(const void*,int,size_t);
size_t strlen(const char*);char *strcpy(char*,const char*);char *strncpy(char*,const char*,size_t);
char *strcat(char*,const char*);char *strncat(char*,const char*,size_t);
int strcmp(const char*,const char*);int strncmp(const char*,const char*,size_t);
char *strchr(const char*,int);char *strrchr(const char*,int);char *strstr(const char*,const char*);
size_t strspn(const char*,const char*);size_t strcspn(const char*,const char*);char *strpbrk(const char*,const char*);
char *strtok(char*,const char*);char *strerror(int);char *_strdup(const char*);
int _stricmp(const char*,const char*);int _strnicmp(const char*,const char*,size_t);int _memicmp(const void*,const void*,size_t);
char *_strlwr(char*);char *_strupr(char*);char *_strrev(char*);
TB_EXTERN_C_END
#define strdup _strdup
#define stricmp _stricmp
#define strnicmp _strnicmp
#define strlwr _strlwr
#define strupr _strupr
#endif
