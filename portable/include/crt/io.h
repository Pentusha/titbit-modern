#ifndef TITBIT_PORTABLE_IO_H
#define TITBIT_PORTABLE_IO_H
#include <titbit_crt.h>
TB_EXTERN_C_BEGIN
int _open(const char*,int,...);int _close(int);int _read(int,void*,unsigned);int _write(int,const void*,unsigned);
long _lseek(int,long,int);long _filelength(int);int _access(const char*,int);int _unlink(const char*);int _fileno(struct _iobuf*);
TB_EXTERN_C_END
#endif
