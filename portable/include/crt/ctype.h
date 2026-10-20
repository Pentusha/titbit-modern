#ifndef TITBIT_PORTABLE_CTYPE_H
#define TITBIT_PORTABLE_CTYPE_H
#include <titbit_crt.h>
TB_EXTERN_C_BEGIN
extern unsigned short *_pctype;
int isalnum(int);int isalpha(int);int iscntrl(int);int isdigit(int);int isgraph(int);int islower(int);int isprint(int);int ispunct(int);int isspace(int);int isupper(int);int isxdigit(int);
int tolower(int);int toupper(int);int _isctype(int,int);
TB_EXTERN_C_END
#define _UPPER 1
#define _LOWER 2
#define _DIGIT 4
#define _SPACE 8
#define _PUNCT 16
#define _CONTROL 32
#define _BLANK 64
#define _HEX 128
#define _ALPHA 0x103
#endif
