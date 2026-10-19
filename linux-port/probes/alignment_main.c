#include <stdio.h>
int alignment_results[8];
extern int probe_alignment(int);
extern int probe_alignment_misaligned(void);
int main(void) {
    int result=probe_alignment(7);
    printf("native call alignment:");
    for(int i=0;i<8;i++)printf(" %d",alignment_results[i]);
    printf(" (failures=%d)\n",result);
    int other=probe_alignment_misaligned();
    printf("after deliberately misaligned callback entry:");
    for(int i=0;i<8;i++)printf(" %d",alignment_results[i]);
    printf(" (failures=%d)\n",other);
    result+=other;
    return result;
}
