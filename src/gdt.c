//
// descriptor_tables.c - Initialises the GDT and IDT, and defines the 
// default ISR and IRQ handler.
// Based on code from Bran's kernel development tutorials.
// Rewritten for JamesM's kernel development tutorials.
//

#include "common.h"
#include "gdt.h"

// Internal function prototypes.
static void init_gdt();
static void gdt_set_gate(_s32, _u32, _u32, _u8, _u8);

gdt_entry_t gdt_entries[5];
gdt_ptr_t gdt_ptr;

// Initialisation routine - zeroes all the interrupt service routines,
// initialises the GDT and IDT.
void init_global_descriptor_table()
{
	// Initialise the global descriptor table.
	init_gdt();
}

static void init_gdt()
{
	gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
	gdt_ptr.base = (_u32) & gdt_entries;

	gdt_set_gate(0, 0, 0, 0, 0);	// Null segment
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);	// Code segment
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);	// Data segment
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);	// User mode code segment
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);	// User mode data segment

	gdt_flush((_u32) & gdt_ptr);
}

// Set the value of one GDT entry.
static void gdt_set_gate(_s32 num, _u32 base, _u32 limit, _u8 access, _u8 gran)
{
	gdt_entries[num].base_low = (base & 0xFFFF);
	gdt_entries[num].base_middle = (base >> 16) & 0xFF;
	gdt_entries[num].base_high = (base >> 24) & 0xFF;

	gdt_entries[num].limit_low = (limit & 0xFFFF);
	gdt_entries[num].granularity = (limit >> 16) & 0x0F;

	gdt_entries[num].granularity |= gran & 0xF0;
	gdt_entries[num].access = access;
}
