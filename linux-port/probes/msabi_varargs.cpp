#include <stdarg.h>
extern "C" int native_vformat(char *,unsigned,const char *,char *);
extern "C" int strcmp(const char *,const char *);
static int format(char *result,const char *fmt,...) {
    va_list args;va_start(args,fmt);
    int count=native_vformat(result,256,fmt,args);
    va_end(args);
    return count;
}
static int scan(const char *tag,...) {
    va_list args;va_start(args,tag);
    int first=va_arg(args,int);
    double second=va_arg(args,double);
    const char *third=va_arg(args,const char *);
    __int64 fourth=va_arg(args,__int64);
    // SeeR relies on va_arg remaining an lvalue to pass packed arguments on.
    int *fifth=&va_arg(args,int);
    va_end(args);
    return first==17 && second==42.5 && strcmp(third,"archive")==0 &&
        fourth==0x1234567812345678LL && *fifth==29;
}
extern "C" int probe_varargs() {
    char out[256];
    format(out,"%s:%d:%.1f","QUEST.EZD",17,42.5);
    if(strcmp(out,"QUEST.EZD:17:42.5"))return 1;
    return !scan("test",17,42.5,"archive",0x1234567812345678LL,29);
}
