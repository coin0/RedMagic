#ifndef GDT_H
#define GDT_H

#define SEG_NULL  0
#define SEG_KCODE 1
#define SEG_KDATA 2
#define SEG_UCODE 3
#define SEG_UDATA 4

// This structure contains the value of one GDT entry.
// We use the attribute 'packed' to tell GCC not to change
// any of the alignment in the structure.
typedef struct {
	_u16 limit_low;		// The lower 16 bits of the limit.
	_u16 base_low;		// The lower 16 bits of the base.
	_u8 base_middle;	// The next 8 bits of the base.
	_u8 access;		// Access flags, determine what ring this segment can be used in.
	_u8 granularity;
	_u8 base_high;		// The last 8 bits of the base.
} __attribute__ ((packed)) gdt_entry_t;

typedef struct {
	_u16 limit;		// The upper 16 bits of all selector limits.
	_u32 base;		// The address of the first gdt_entry_t struct.
} __attribute__ ((packed)) gdt_ptr_t;

#include "cpu.h"

// Initialisation function is publicly accessible.
extern void init_global_descriptor_table(struct cpu_state *cpu);

// Lets us access our ASM functions from our C code.
extern void gdt_flush(_u32);

#endif
