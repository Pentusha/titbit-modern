/* SeeR's C recovery boundary uses the installed VC6 jmp_buf field contract.
 * This preserves the caller context directly; no glibc jmp_buf is overlaid. */
.text
.globl titbit_setjmp3
.type titbit_setjmp3,@function
titbit_setjmp3:
 movl 4(%esp),%edx
 movl %ebp,0(%edx)
 movl %ebx,4(%edx)
 movl %edi,8(%edx)
 movl %esi,12(%edx)
 leal 4(%esp),%eax
 movl %eax,16(%edx)
 movl 0(%esp),%eax
 movl %eax,20(%edx)
 xorl %eax,%eax
 ret
.size titbit_setjmp3,.-titbit_setjmp3
.globl titbit_longjmp
.type titbit_longjmp,@function
titbit_longjmp:
 movl 4(%esp),%edx
 movl 8(%esp),%eax
 testl %eax,%eax
 jnz 1f
 incl %eax
1:
 movl 0(%edx),%ebp
 movl 4(%edx),%ebx
 movl 8(%edx),%edi
 movl 12(%edx),%esi
 movl 16(%edx),%esp
 jmp *20(%edx)
.size titbit_longjmp,.-titbit_longjmp
.globl _chkstk
.type _chkstk,@function
_chkstk:
 pushl %ecx
 leal 8(%esp),%ecx
 cmpl $4096,%eax
 jb 3f
2:
 subl $4096,%ecx
 testb $0,(%ecx)
 subl $4096,%eax
 cmpl $4096,%eax
 jae 2b
3:
 subl %eax,%ecx
 testb $0,(%ecx)
 movl %esp,%eax
 movl %ecx,%esp
 movl (%eax),%ecx
 movl 4(%eax),%eax
 pushl %eax
 ret
.size _chkstk,.-_chkstk
.section .note.GNU-stack,"",@progbits
