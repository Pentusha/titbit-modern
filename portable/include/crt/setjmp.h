#ifndef TITBIT_PORTABLE_SETJMP_H
#define TITBIT_PORTABLE_SETJMP_H
typedef int jmp_buf[16];
#include <titbit_crt.h>
TB_EXTERN_C_BEGIN
int _setjmp3(jmp_buf,int,...);void longjmp(jmp_buf,int);
TB_EXTERN_C_END
#define setjmp(b) _setjmp3(b,0)
#endif
