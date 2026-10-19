#include <stdio.h>
extern int titbit_vc6_cpp_smoke(void);
int main(void) {
 int result = titbit_vc6_cpp_smoke();
 printf("VC6 header ABI / native runtime smoke: %d\n", result);
 return result;
}
