#include <stdio.h>
extern int probe_nested(int);
extern int probe_propagate(void);
extern int probe_by_value(void);
extern int probe_throw_lvalue(void);
extern int trace_value,error_copies,error_destroyed;
int main(void) {
    setbuf(stdout,0);
    int failed=0;
    const int values[]={1,70,170};
    const int traces[]={321,3421,34251};
    for(int i=0;i<3;i++) {
        int result=probe_nested(i);
        printf("nested EH[%d]: return=%d, RAII=%d, exception-copies=%d, destroyed=%d\n",
               i,result,trace_value,error_copies,error_destroyed);
        if(result!=values[i] || trace_value!=traces[i]) failed=1;
        if(i && (error_copies!=0 || error_destroyed!=1)) failed=1;
    }
    int propagated=probe_propagate();
    printf("unmatched handler propagates: return=%d, RAII=%d\n",propagated,trace_value);
    if(propagated!=180 || trace_value!=6)failed=1;
    int value=probe_by_value();
    printf("catch by value: return=%d copies=%d destroyed=%d\n",value,error_copies,error_destroyed);
    if(value!=90 || error_copies!=1 || error_destroyed!=2)failed=1;
    value=probe_throw_lvalue();
    printf("throw lvalue: return=%d copies=%d destroyed=%d\n",value,error_copies,error_destroyed);
    if(value!=91 || error_copies!=1 || error_destroyed!=2)failed=1;
    return failed;
}
