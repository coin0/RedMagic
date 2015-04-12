#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "common.h"

// cs -> 0x00000000 code
// ds,ss,es,fs,gs -> 0x00000000 data
// A20 on, paging off, int off, eax = 0x00000000
// boot info is now saved in ebx
typedef struct {
	// multiboot version
	_u32 flags;

	_u32 mem_lower;
	_u32 mem_upper;

	_u32 boot_device;
	_u32 cmdline;
	_u32 mods_count;
	_u32 mods_addr;

	// ELF kernel info
	_u32 num;
	_u32 size;
	_u32 addr;
	_u32 shndx;

	_u32 mmap_length;
	_u32 mmap_addr;

	_u32 drives_length;
	_u32 drives_addr;
	_u32 config_table;
	_u32 boot_loader_name;
	_u32 apm_table;
	_u32 vbe_control_info;
	_u32 vbe_mode_info;
	_u32 vbe_mode;
	_u32 vbe_interface_seg;
	_u32 vbe_interface_off;
	_u32 vbe_interface_len;
} __attribute__ ((packed)) multiboot_t;

typedef struct {
	_u32 size;
	_u32 base_addr_low;
	_u32 base_addr_high;
	_u32 length_low;
	_u32 length_high;
	_u32 type;
} __attribute__ ((packed)) mmap_entry_t;

#endif
