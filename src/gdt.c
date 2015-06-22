/*
 *      Red Magic 1996 - 2015
 *
 *      gdt.c - setup per-cpu GDT entries for the X86 architecture
 *
 *      2015 Lin Coin - initial version
 */

#include "common.h"
#include "gdt.h"
#include "cpu.h"

// Internal function prototypes.
static void init_gdt(cpu_state_t * cpu);
static void gdt_set_gate(cpu_state_t * cpu, _s32 num, _u32 base, _u32 limit,
			 _u8 access, _u8 gran);

void init_global_descriptor_table(cpu_state_t * cpu)
{
	// Initialise the global descriptor table.
	init_gdt(cpu);
}

static void init_gdt(cpu_state_t * cpu)
{
	cpu->gdt_ptr.limit = (sizeof(gdt_entry_t) * MAX_GDT_ENT_PCPU) - 1;
	cpu->gdt_ptr.base = (_u32) & cpu->gdt_entries;

	gdt_set_gate(cpu, SEG_NULL, 0, 0, 0, 0);	// Null segment
	gdt_set_gate(cpu, SEG_KCODE, 0, 0xFFFFFFFF, 0x9A, 0xCF);	// Code segment
	gdt_set_gate(cpu, SEG_KDATA, 0, 0xFFFFFFFF, 0x92, 0xCF);	// Data segment
	gdt_set_gate(cpu, SEG_UCODE, 0, 0xFFFFFFFF, 0xFA, 0xCF);	// User mode code segment
	gdt_set_gate(cpu, SEG_UDATA, 0, 0xFFFFFFFF, 0xF2, 0xCF);	// User mode data segment
	gdt_flush((_u32) (&cpu->gdt_ptr));
}

// Set the value of one GDT entry.
static void gdt_set_gate(cpu_state_t * cpu, _s32 num, _u32 base, _u32 limit,
			 _u8 access, _u8 gran)
{
	cpu->gdt_entries[num].base_low = (base & 0xFFFF);
	cpu->gdt_entries[num].base_middle = (base >> 16) & 0xFF;
	cpu->gdt_entries[num].base_high = (base >> 24) & 0xFF;

	cpu->gdt_entries[num].limit_low = (limit & 0xFFFF);
	cpu->gdt_entries[num].granularity = (limit >> 16) & 0x0F;

	cpu->gdt_entries[num].granularity |= gran & 0xF0;
	cpu->gdt_entries[num].access = access;
}
