/* SeeR x86 argument-vector calls. Equivalent to callasm.c's MSVC branch:
 * callfunc uses cdecl, callmember takes this from params[0] into ECX and the
 * callee pops the remaining arguments. EAX/EDX and x87 ST0 results survive.
 * We additionally align native call sites to Linux i386's 16-byte boundary.
 */
#if !defined(__i386__)
#error SeeR's recovered argument vectors require a 32-bit target
#endif
.text
.macro PROLOGUE member=0
 .cfi_startproc
 pushl %ebp
 .cfi_def_cfa_offset 8
 .cfi_offset %ebp,-8
 movl %esp,%ebp
 .cfi_def_cfa_register %ebp
 pushl %ebx
 .cfi_offset %ebx,-12
 pushl %esi
 .cfi_offset %esi,-16
 pushl %edi
 .cfi_offset %edi,-20
 movl 8(%ebp),%ebx
 movl 12(%ebp),%esi
 movl 16(%ebp),%ecx
.if \member
 testl %ecx,%ecx
 jz 9f
 addl $4,%esi
 subl $4,%ecx
.endif
 subl %ecx,%esp
 andl $-16,%esp
 movl %esp,%edi
 cld
 rep movsb
.if \member
 movl 12(%ebp),%edx
 movl (%edx),%ecx
9:
.endif
 call *%ebx
.endm
.macro SAVE_EDX
 call 1f
1:popl %ecx
 addl $_GLOBAL_OFFSET_TABLE_+(.-1b),%ecx
 movl _msvc_regEDX@GOT(%ecx),%ecx
 movl %edx,(%ecx)
.endm
.macro EPILOGUE
 leal -12(%ebp),%esp
 popl %edi
 .cfi_restore %edi
 popl %esi
 .cfi_restore %esi
 popl %ebx
 .cfi_restore %ebx
 leave
 .cfi_def_cfa %esp,4
 .cfi_restore %ebp
 ret
 .cfi_endproc
.endm
.globl callfunc
.type callfunc,@function
callfunc:
 PROLOGUE 0
 cmpl $0,16(%ebp)
 je 2f
 SAVE_EDX
2:
 EPILOGUE
.size callfunc,.-callfunc
.globl callmember
.type callmember,@function
callmember:
 PROLOGUE 1
 cmpl $0,16(%ebp)
 je 2f
 SAVE_EDX
2:
 EPILOGUE
.size callmember,.-callmember
.globl callfunc_d
.type callfunc_d,@function
callfunc_d:
 PROLOGUE 0
 EPILOGUE
.size callfunc_d,.-callfunc_d
.globl callmember_d
.type callmember_d,@function
callmember_d:
 PROLOGUE 1
 EPILOGUE
.size callmember_d,.-callmember_d
.bss
.align 4
.globl _msvc_regEDX
.type _msvc_regEDX,@object
.size _msvc_regEDX,4
_msvc_regEDX:.zero 4
.section .note.GNU-stack,"",@progbits
