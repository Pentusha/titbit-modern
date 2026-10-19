#include <stdio.h>
extern int probe_rtti(void);
int main(void) {
    int result=probe_rtti();
    printf("native MS RTTI: %s (%d)\n",result?"FAIL":"PASS",result);
    return result;
}
