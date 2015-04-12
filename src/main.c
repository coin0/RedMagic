// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
// Made for JamesM's tutorials 

#include "common.h"
#include "multiboot.h"
#include "system.h"
#include "print.h"
#include "timer.h"
#include "debug.h"
#include "mm.h"

multiboot_t *mbootp;

int main(multiboot_t * mbp)
{
	// first is to save critical info from bootloader
	mbootp = mbp;

#ifdef MODE_DBG
	init_debug(mbp);
#endif

	// reset text-mode
	c80_clear();
	printk_color(rc_black, rc_red, "\t\t\t\tRed Magic\n");

	// initialize descriptors
	init_global_descriptor_table();
	init_interrupt_descriptor_table();

	// interrupt on
	asm volatile ("sti");

	// show memory map ready for paging
	show_kernel_pos();
	show_ARDS_from_multiboot(mbp);
	init_paging();

	//print_cur_status();
	//print_stack_trace();  
	//init_timer(10000000);
	//asm volatile ("sti");
	//asm volatile ("int $0xe");

	// All our initialisation calls will go in here.
	return 0xDEADBABA;
}
