.text
.globl align_zero,align_one,align_three,align_six,align_double
.type align_zero,@function
align_zero:
align_one:
align_three:
align_six:
align_double:
    leal 4(%esp),%eax
    andl $15,%eax
    ret
.globl native_align_stdcall
native_align_stdcall:
    leal 4(%esp),%eax
    andl $15,%eax
    ret $12
.globl native_align_thiscall
native_align_thiscall:
    leal 4(%esp),%eax
    andl $15,%eax
    ret $8
.globl probe_alignment_misaligned
probe_alignment_misaligned:
    subl $4,%esp
    pushl $7
    call probe_alignment
    addl $8,%esp
    ret
.section .note.GNU-stack,"",@progbits
