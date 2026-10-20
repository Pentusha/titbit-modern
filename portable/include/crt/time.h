#ifndef TITBIT_PORTABLE_TIME_H
#define TITBIT_PORTABLE_TIME_H
#include <titbit_crt.h>
#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
typedef long time_t;
#endif
typedef long clock_t;
#define CLOCKS_PER_SEC 1000
struct tm {int tm_sec,tm_min,tm_hour,tm_mday,tm_mon,tm_year,tm_wday,tm_yday,tm_isdst;};
TB_EXTERN_C_BEGIN
time_t time(time_t*);clock_t clock(void);struct tm *localtime(const time_t*);struct tm *gmtime(const time_t*);
time_t mktime(struct tm*);char *ctime(const time_t*);char *asctime(const struct tm*);double difftime(time_t,time_t);
size_t strftime(char*,size_t,const char*,const struct tm*);
TB_EXTERN_C_END
#endif
