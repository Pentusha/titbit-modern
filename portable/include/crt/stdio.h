#ifndef TITBIT_PORTABLE_STDIO_H
#define TITBIT_PORTABLE_STDIO_H
#include <titbit_crt.h>
#include <stdarg.h>
typedef struct _iobuf {char *_ptr;int _cnt;char *_base;int _flag,_file,_charbuf,_bufsiz;char *_tmpfname;} FILE;
#define _FILE_DEFINED
typedef __int64 fpos_t;
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define _IOFBF 0
#define _IOLBF 0x40
#define _IONBF 4
#define BUFSIZ 512
#define FILENAME_MAX 260
#define L_tmpnam 14
TB_EXTERN_C_BEGIN
extern FILE _iob[];
#ifdef TITBIT_WINDOWS_UCRT
FILE *__cdecl __acrt_iob_func(unsigned);
#define stdin (__acrt_iob_func(0))
#define stdout (__acrt_iob_func(1))
#define stderr (__acrt_iob_func(2))
#else
#define stdin (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])
#endif
FILE *fopen(const char*,const char*); FILE *freopen(const char*,const char*,FILE*); FILE *tmpfile(void);
int fclose(FILE*); int fflush(FILE*); size_t fread(void*,size_t,size_t,FILE*); size_t fwrite(const void*,size_t,size_t,FILE*);
int fseek(FILE*,long,int); long ftell(FILE*); int fgetpos(FILE*,fpos_t*); int fsetpos(FILE*,const fpos_t*);
void rewind(FILE*); int feof(FILE*);int ferror(FILE*); void clearerr(FILE*);
int fgetc(FILE*);int fputc(int,FILE*);int ungetc(int,FILE*);char *fgets(char*,int,FILE*);int fputs(const char*,FILE*);
int setvbuf(FILE*,char*,int,size_t);void setbuf(FILE*,char*);
int printf(const char*,...);int fprintf(FILE*,const char*,...);int sprintf(char*,const char*,...);
int vprintf(const char*,va_list);int vfprintf(FILE*,const char*,va_list);int vsprintf(char*,const char*,va_list);
int _snprintf(char*,size_t,const char*,...);int _vsnprintf(char*,size_t,const char*,va_list);
int snprintf(char*,size_t,const char*,...);int vsnprintf(char*,size_t,const char*,va_list);
int scanf(const char*,...);int fscanf(FILE*,const char*,...);int sscanf(const char*,const char*,...);
int puts(const char*);int putchar(int);int getchar(void);int remove(const char*);int rename(const char*,const char*);
void perror(const char*);int _fileno(FILE*);int _filbuf(FILE*);int _flsbuf(int,FILE*);
TB_EXTERN_C_END
#define getc(f) fgetc(f)
#define putc(c,f) fputc(c,f)
#define fileno _fileno
#endif
