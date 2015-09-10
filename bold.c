#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <elf.h>

#include "elf-bf.h"

/* File layout: (PT_LOAD segment spans everything)
 *   Elf Header
 *   Segment Header Table
 *   Interp string
 *   Code
 */

#ifndef LDPATH
#define LDPATH /lib64/ld-bf.so
#endif

#define MACRO_STR_IMPL(x) #x
#define MACRO_STR(x) MACRO_STR_IMPL(x)

const char* interp_string = MACRO_STR(LDPATH);

int main(int argc, char* argv[]) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <file.bf> <out.elf>\n", argv[0]);
		return 1;
	}

	int phnum = 5;

	char* sourcefile = argv[1];
	struct stat source_stat;
	stat(sourcefile, &source_stat);
	int source = open(sourcefile, O_RDONLY);
	void* source_map = mmap(NULL, source_stat.st_size, PROT_READ, MAP_PRIVATE, source, 0);
	if (source_map == MAP_FAILED) {
		fprintf(stderr, "Couldn't read input: %s\n", strerror(errno));
		return 1;
	}

	char* elffile = argv[2];
	int elf = open(elffile, O_RDWR | O_CREAT, 0775);
	long long maplen = sizeof(Elf64_Ehdr) + phnum*sizeof(Elf64_Phdr) + strlen(interp_string) + source_stat.st_size;

	int error = ftruncate(elf, maplen);
	if (error) {
		fprintf(stderr, "Couldn't resize file: %s\n", strerror(errno));
		return 1;
	}

	void* map = mmap(NULL, maplen, PROT_READ | PROT_WRITE, MAP_SHARED, elf, 0);
	if (map == MAP_FAILED) {
		fprintf(stderr, "Map failed: %s\n", strerror(errno));
		return 1;
	}

	unsigned long base_addr = 0x400000; // not sure why, but this seems to be the standard

	// fill header
	Elf64_Ehdr* header = (Elf64_Ehdr*)map;
	header->e_ident[EI_MAG0] = ELFMAG0;
	header->e_ident[EI_MAG1] = ELFMAG1;
	header->e_ident[EI_MAG2] = ELFMAG2;
	header->e_ident[EI_MAG3] = ELFMAG3;
	header->e_ident[EI_CLASS] = ELFCLASS64;
	header->e_ident[EI_DATA]  = ELFDATA2LSB;
	header->e_ident[EI_VERSION] = EV_CURRENT;
	header->e_ident[EI_OSABI] = ELFOSABI_SYSV;
	header->e_ident[EI_ABIVERSION] = 0;
	header->e_type = ET_EXEC;
	header->e_ehsize = sizeof(Elf64_Ehdr);
	header->e_machine = EM_X86_64;
	header->e_shoff = 0;
	header->e_shentsize = sizeof(Elf64_Shdr);
	header->e_shnum = 0;
	header->e_phoff = sizeof(Elf64_Ehdr);
	header->e_phentsize = sizeof(Elf64_Phdr);
	header->e_phnum = phnum;


	Elf64_Phdr* phdr   = (Elf64_Phdr*)(map + header->e_phoff + 0*header->e_phentsize);
	phdr->p_type = PT_PHDR;
	phdr->p_flags = PF_R;
	phdr->p_offset = sizeof(Elf64_Ehdr);
	phdr->p_filesz = phnum*sizeof(Elf64_Phdr);
	phdr->p_vaddr = base_addr + phdr->p_offset;
	phdr->p_memsz  = phdr->p_filesz;

	Elf64_Phdr* interp = (Elf64_Phdr*)(map + header->e_phoff + 1*header->e_phentsize);
	interp->p_type = PT_INTERP; // INTERP must precede any loadable segment header
	interp->p_flags = PF_R;
	interp->p_offset = phdr->p_offset + phdr->p_filesz;
	interp->p_filesz = strlen(interp_string) + 1;
	interp->p_memsz = interp->p_filesz;
	interp->p_vaddr = base_addr + interp->p_offset;

	Elf64_Phdr* bf = (Elf64_Phdr*)(map + header->e_phoff + 2*header->e_phentsize);
	bf->p_type = PT_BF;
	bf->p_flags = PF_R; // don't need PF_X since the code is interpreted
	bf->p_offset = interp->p_offset + interp->p_filesz;
	bf->p_filesz = source_stat.st_size;
	bf->p_memsz = bf->p_filesz;
	bf->p_vaddr = base_addr + bf->p_offset;

	Elf64_Phdr* data = (Elf64_Phdr*)(map + header->e_phoff + 3*header->e_phentsize);
	data->p_type = PT_LOAD;
	data->p_flags = PF_R;
	data->p_offset = 0; // overlaps with all other segments (otherwise they wouldn't get loaded)
	data->p_filesz = maplen;
	data->p_memsz  = data->p_filesz;
	data->p_vaddr = base_addr;

	Elf64_Phdr* stack  = (Elf64_Phdr*)(map + header->e_phoff + 4*header->e_phentsize);
	stack->p_type = PT_GNU_STACK;
	stack->p_flags = PF_R | PF_W;
	stack->p_filesz = 0; // the kernel knows PT_GNU_STACK and will map an appropriate region
	stack->p_offset = 0;
	stack->p_vaddr = 0; 
	stack->p_memsz = 0;


	strncpy(map + interp->p_offset, interp_string, interp->p_filesz);
	memcpy (    map + bf->p_offset,    source_map,     bf->p_filesz);
	header->e_entry = bf->p_vaddr;

	return 0;
}
