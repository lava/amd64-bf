#include <sys/syscall.h>

.global _start

.section .text
        .type start, STT_FUNC

_start:
	// todo: maybe insert cfi here

	// Clear the frame pointer, to mark the outermost frame obviously.
	xorl %ebp, %ebp
	
	// Relocatable call to start
	mov %rsp, %rdi
	// This needs gcc or a fairly recent clang, since clang-3.6 will 
	// emit a R_X86_64_PC32 relocation here, which is just
	// wrong
	// gcc would actually recognize that start@GOTOFF is 64 bits wide
	// and upgrade a plain mov to movabs, but clang isnt smart
	// enough to do it (as of 3.8)
	movabs $start@GOTOFF, %rax
	leaq _GLOBAL_OFFSET_TABLE_(%rip), %rcx
	addq %rcx, %rax
	callq *%rax

	// exit
	mov %rax, %rdi
	mov $__NR_exit, %eax
	syscall
