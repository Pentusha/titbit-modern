#include <stdio.h>
extern int probe_eh(int);
int main(void) {
    int a=probe_eh(0), b=probe_eh(77);
    printf("native MS-layout/DWARF EH: no-throw=%d, caught-int=%d\n",a,b);
    return a!=0 || b!=87;
}
