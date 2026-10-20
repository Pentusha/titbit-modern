#include <stdio.h>
extern int titbit_vc6_cpp_smoke(void);
#ifdef TITBIT_PORTABLE_SMOKE
extern int titbit_portable_container_smoke(void);
#endif
int main(void) {
 int result = titbit_vc6_cpp_smoke();
 printf("Source ABI / native runtime smoke: %d\n", result);
#ifdef TITBIT_PORTABLE_SMOKE
 if (!result) result = titbit_portable_container_smoke();
 printf("Portable container ownership/layout smoke: %d\n", result);
#endif
 return result;
}
