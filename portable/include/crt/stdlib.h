#ifndef TITBIT_PORTABLE_STDLIB_H
#define TITBIT_PORTABLE_STDLIB_H
#include <titbit_crt.h>
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767
#define MB_CUR_MAX 1
typedef struct {int quot,rem;} div_t;
typedef struct {long quot,rem;} ldiv_t;
TB_EXTERN_C_BEGIN
void *malloc(size_t);void *calloc(size_t,size_t);void *realloc(void*,size_t);void free(void*);
void abort(void);void exit(int);void _exit(int);int atexit(void (__cdecl *)(void));
int atoi(const char*);long atol(const char*);double atof(const char*);
long strtol(const char*,char**,int);unsigned long strtoul(const char*,char**,int);double strtod(const char*,char**);
int rand(void);void srand(unsigned);char *getenv(const char*);int system(const char*);
int abs(int);long labs(long);div_t div(int,int);ldiv_t ldiv(long,long);
void qsort(void*,size_t,size_t,int (__cdecl *)(const void*,const void*));
void *bsearch(const void*,const void*,size_t,size_t,int (__cdecl *)(const void*,const void*));
char *_itoa(int,char*,int);char *_ltoa(long,char*,int);char *_ultoa(unsigned long,char*,int);
int _putenv(const char*);void _splitpath(const char*,char*,char*,char*,char*);void _makepath(char*,const char*,const char*,const char*,const char*);
TB_EXTERN_C_END
#define itoa _itoa
#endif
