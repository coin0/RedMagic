#ifndef ELF_H
#define ELF_H

#include "type.h"
#include "multiboot.h"

#define ELF32_ST_TYPE(i) ((i)&0xf)

// ELF section header
typedef struct {
	_u32 name;
	_u32 type;
	_u32 flags;
	_u32 addr;
	_u32 offset;
	_u32 size;
	_u32 link;
	_u32 info;
	_u32 addralign;
	_u32 entsize;
} __attribute__ ((packed)) elf_section_header_t;

// ELF symbol
typedef struct {
	_u32 name;
	_u32 value;
	_u32 size;
	_u8 info;
	_u8 other;
	_u16 shndx;
} __attribute__ ((packed)) elf_symbol_t;

// ELF
typedef struct {
	elf_symbol_t *symtab;
	_u32 symtabsz;
	const char *strtab;
	_u32 strtabsz;
} elf_t;

// get elf info from multiboot struct 
elf_t elf_from_multiboot(multiboot_t * mb);

// get elf symbol 
const char *elf_lookup_symbol(_u32 addr, elf_t * elf);

#endif
