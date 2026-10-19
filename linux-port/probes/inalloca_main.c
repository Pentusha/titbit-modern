#include <stdio.h>
extern int probe_inalloca(void);
__attribute__((naked)) int native_stack_align(void) {
    __asm__("leal 4(%esp),%eax\n\tandl $15,%eax\n\tret");
}
int main(void) {
    int result=probe_inalloca();
    printf("native MS nontrivial by-value / sret ABI: %s (%d)\n",result?"FAIL":"PASS",result);
    return result;
}
