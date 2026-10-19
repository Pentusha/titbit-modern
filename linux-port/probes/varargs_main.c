#include <stdio.h>
#include <stdarg.h>
extern int probe_varargs(void);
int native_vformat(char *out,unsigned size,const char *format,char *arguments) {
    return vsnprintf(out,size,format,arguments);
}
int main(void) {
    int result=probe_varargs();
    printf("native SDK variadic ABI: %s (%d)\n",result?"FAIL":"PASS",result);
    return result;
}
