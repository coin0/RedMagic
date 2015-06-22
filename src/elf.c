/*
 *      Red Magic 1996 - 2015
 *
 *      elf.c - GNU/Linux compatible ELF library
 *
 *      2015 Lin Coin - initial version, some of code is from hurlex's tutorial
 */

#include "common.h"
#include "string.h"
#include "elf.h"

// return elf info struct
elf_t elf_from_multiboot(multiboot_t * mb)
{
	int i;
	elf_t elf;
	elf_section_header_t *sh = (elf_section_header_t *) mb->addr;

	_u32 shstrtab = sh[mb->shndx].addr;
	for (i = 0; i < mb->num; i++) {
		const char *name = (const char *)(shstrtab + sh[i].name);

		// look for string table
		if (strcmp(name, ".strtab") == 0) {
			elf.strtab = (const char *)sh[i].addr;
			elf.strtabsz = sh[i].size;
		}
		// look for symbol table
		if (strcmp(name, ".symtab") == 0) {
			elf.symtab = (elf_symbol_t *) sh[i].addr;
			elf.symtabsz = sh[i].size;
		}
	}

	return elf;
}

const char *elf_lookup_symbol(_u32 addr, elf_t * elf)
{
	int i;

	for (i = 0; i < (elf->symtabsz / sizeof(elf_symbol_t)); i++) {
		if (ELF32_ST_TYPE(elf->symtab[i].info) != 0x2) {
			continue;
		}
		if ((addr >= elf->symtab[i].value)
		    && (addr < (elf->symtab[i].value + elf->symtab[i].size))) {
			return (const char *)((_u32) elf->strtab +
					      elf->symtab[i].name);
		}
	}

	return NULL;
}
