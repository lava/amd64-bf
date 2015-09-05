#include <sys/syscall.h>

.global _start

_start:
	// todo: maybe insert cfi here

	// Clear the frame pointer, to mark the outermost frame obviously.  */
	xorl %ebp, %ebp
	
	// Relocatable call to start
	// int status = start(rsp);
	mov %rsp, %rdi
	mov $start@GOTOFF, %rax
	leaq _GLOBAL_OFFSET_TABLE_(%rip), %rcx
	addq %rcx, %rax
	callq *%rax

	// exit(status);
	mov %rax, %rdi
	mov $__NR_exit, %eax
	syscall
