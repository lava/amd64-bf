#include <elf.h>
#include <sys/syscall.h>

#include "elf-bf.h"

static unsigned char* ip;
static uint64_t* sp;

static void cmd_write()
{
	uint64_t sys_write = __NR_write;
	uint64_t fd = 1; // stdout
	uint64_t count = 1;
	asm volatile ("mov %[write], %%rax\n"
		"mov %[fd], %%rdi\n"
		"mov %[addr], %%rsi\n"
		"mov %[count], %%rdx\n"
		"syscall\n"
		: 
		: [write]"r"(sys_write), [addr]"r"(sp), [fd]"r"(fd), [count]"r"(count)
		: "rsi", "rdi", "rax", "rdx", "rcx", "r11");

	++ip;
}

static void cmd_read()
{
	uint64_t sys_read = __NR_read;
	uint64_t fd = 0; // stdin
	uint64_t count = 1;
	char buf;
	asm volatile ("mov %[read], %%rax\n"
		"mov %[fd], %%rdi\n"
		"mov %[addr], %%rsi\n"
		"mov %[count], %%rdx\n"
		"syscall\n"
		: 
		: [read]"r"(sys_read), [addr]"r"(&buf), [fd]"r"(fd), [count]"r"(count)
		: "rsi", "rdi", "rax", "rdx", "rcx", "r11");

	*sp = buf; // can't let the syscall write directly to sp because we want to reset the whole cell
	++ip;
}

static void cmd_syscall()
{
	uint64_t sys_nr=sp[0], rdi=sp[1], rsi=sp[2], rdx=sp[3], r10=sp[4], r8=sp[5], r9=sp[6];
	uint64_t result;
	asm("mov %[sys_nr], %%rax\n"
		"mov %[rdi], %%rdi\n"
		"mov %[rsi], %%rsi\n"
		"mov %[rdx], %%rdx\n"
		"mov %[r10], %%r10\n"
		"mov %[r8], %%r8\n"
		"mov %[r9], %%r9\n"
		"syscall\n"
		"mov %%rax, %[result]"
		: [result]"=r"(result)
		: [sys_nr]"rm"(sys_nr), [rdi]"rm"(rdi), [rsi]"rm"(rsi), [rdx]"rm"(rdx), [r10]"rm"(r10), [r8]"rm"(r8), [r9]"rm"(r9)
		: "rax", "rdi", "rsi", "rdx", "r10", "r9", "r8", "rcx", "r11" // ABI: "The kernel destroys registers %rcx and %r11."
		);

	*sp = result;
	++ip;
}

static void cmd_copy()
{
	int count=1;
	while(*++ip == '=') ++count;

	sp[count] = sp[0];
}

static void cmd_load()
{
	uint64_t* ptr = (uint64_t*)*sp;
	*sp = *ptr;
	++ip;
}

static void cmd_plus()
{
	++*sp;
	++ip;
}

static void cmd_minus()
{
	--*sp;
	++ip;
}

static void cmd_right()
{
	++sp;
	++ip;
}

static void cmd_left()
{
	--sp;
	++ip;
}

static void cmd_lbrak()
{
	if(*sp == 0) {
		int stack = 1;
		while(stack) {
			++ip;
			if(*ip == '[')
				++stack;
			else if(*ip == ']')
				--stack;
		}
	}

	++ip;
}

static void cmd_rbrak()
{
	int stack = 1;
	uint64_t hops = 0;

	while(stack) {
		++hops;
		--ip;
		if(*ip == '[')
			--stack;
		else if(*ip == ']')
			++stack;
	}
}



typedef void (*command)();

static command commands[] = { 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, cmd_syscall, 0, 0, 0, 0, 0, 0, 0, 0, cmd_load, cmd_plus, cmd_read, cmd_minus, cmd_write, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cmd_left, cmd_copy, cmd_right, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cmd_lbrak, 0, cmd_rbrak, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 };

long start(void** stack) {
	*(int*)stack = 0; //argc

	uint64_t** argv = (uint64_t**)(stack+1);

	while(*argv) *argv++ = 0;

	uint64_t** envp = argv+1;

	while(*envp) *envp++ = 0;

	Elf64_auxv_t* auxv = (Elf64_auxv_t*) (envp+1);

	Elf64_Phdr* phdr = 0;
	long phnum = 0;

	void* base = 0;
	void* entry = 0;

	int finished = 0;
	while (!finished) {
		switch (auxv->a_type) {
		case AT_NULL:
			finished = 1;
			break;
		case AT_PHDR:
			phdr = (Elf64_Phdr*)auxv->a_un.a_val;
			break;
		case AT_PHNUM:
			phnum = auxv->a_un.a_val;
			break;
		case AT_BASE:
			base = (void*)auxv->a_un.a_val;
			break;

		// Collecting legal values in case they are needed later:
		case AT_EXECFD:
		case AT_ENTRY:
		case AT_IGNORE:
		case AT_PHENT:
		case AT_PAGESZ:
		case AT_FLAGS:
		case AT_NOTELF:
		case AT_UID:
		case AT_EUID:
		case AT_GID:
		case AT_EGID:
		case AT_CLKTCK:
		case AT_SYSINFO:
		case AT_SYSINFO_EHDR:
			break;
		}

		auxv->a_type = 0;
		auxv->a_un.a_val = 0;
		++auxv;
	}

	// todo: read program from 


	unsigned char* tape_begin = 0;
	unsigned char* tape_end = 0;


	// scan segment header for size of data segment
	// (assuming text goes all the way to the end)
	for (long i=0; i<phnum; ++i) {
		if (phdr[i].p_type == PT_BF) {
			tape_begin = (unsigned char*) phdr[i].p_vaddr;
			tape_end   = tape_begin + phdr[i].p_memsz;
		}
	}

	// prepare tape (cells were already zeroed out above)
	sp = (uint64_t*)(stack+1);
	sp[-1] = (uint64_t) stack;

	ip = tape_begin;
	while (ip < tape_end) {
		// The compiler will enter absolute addresses in commands, but since
		// we are shared they will effectively turn into offsets after
		// relocation
		long offset = (long)commands[*ip];
		command cmd = base + offset;
		if (offset != 0) 
			cmd();
		else
			++ip;
	}

	return 0;
}
