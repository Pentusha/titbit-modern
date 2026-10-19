.text
.globl _start
_start:
    andl $-16, %esp
    call probe_run
    movl %eax, %ebx
    testl %eax, %eax
    jnz 1f
    movl $4, %eax
    movl $1, %ebx
    movl $message, %ecx
    movl $message_end-message, %edx
    int $0x80
    xorl %ebx, %ebx
1:
    movl $1, %eax
    int $0x80
.section .rodata
message:
    .ascii "native ELF32 MS-layout/vtable/thiscall/stdcall probe: PASS\n"
message_end:
.section .note.GNU-stack,"",@progbits
