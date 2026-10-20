#ifndef TITBIT_PORTABLE_ERRNO_H
#define TITBIT_PORTABLE_ERRNO_H
#include <titbit_crt.h>
TB_EXTERN_C_BEGIN
int *_errno(void);
TB_EXTERN_C_END
#define errno (*_errno())
#define EDOM 33
#define ERANGE 34
#define ENOMEM 12
#define EINVAL 22
#define ENOENT 2
#endif
