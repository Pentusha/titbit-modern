#include <stdio.h>
extern int probe_run(void);
int main(void) {
    int result=probe_run();
    printf("native ELF32 MS-layout probe: %s (%d)\n",result?"FAIL":"PASS",result);
    return result;
}
