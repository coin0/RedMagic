// main.c -- Defines the C-code kernel entry point, calls initialisation routines.
// Made for JamesM's tutorials 

#include "common.h"
#include "multiboot.h"
#include "system.h"
#include "print.h"
#include "timer.h"
#include "debug.h"
#include "heap.h"
#include "paging.h"
#include "sched.h"
#include "cpu.h"

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

	// init processors
	init_bootstrap_processor();

	// initialize descriptors
	init_global_descriptor_table();
	init_interrupt_descriptor_table();
	init_io_apic();

	// interrupt on
	local_irq_enable();

	// memory management
	show_kernel_pos();
	show_ARDS_from_multiboot(mbp);
	init_paging();
	init_kheap();

	// set system clock rate
	if (!mpinfo.ismp) {
		init_timer(CLOCK_INT_HZ);
		printk("Kernel clock is set to %d HZ\n", CLOCK_INT_HZ);
	} else {
		// APIC timer has been already set in init_lapic
		init_timer_cb();
	}

	// initialize kernel task and scheduling
	setup_init_task();
	init_sched();

	// All our initialisation calls will go in here.
	return 0xDEADBABA;
}
