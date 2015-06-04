/*
 *   entry of C code for bootstrap processor
 */

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
#include "string.h"

multiboot_t *mbootp;

// apinit_start and apinit_end are defined in linker script
extern uchar_t apinit_start[];
extern uchar_t apinit_end[];

extern void ap_start32();
static void start_smp();

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

	// start all APs
	if (mpinfo.ismp)
		start_smp();

	// initialize kernel task and scheduling
	setup_init_task();
	init_sched();

	// All our initialisation calls will go in here.
	return 0xDEADBABA;
}

static void start_smp()
{
	int n;

	for (n = 0; n < mpinfo.ncpu; n++) {
		if (cpuset[n].flag_bsp) {
			printk("CPU(BSP) #%d is up ...\n", cpuset[n].proc_id);
			continue;
		}
		// because each AP will initially begin with 16-bits real mode
		// we need to move these code under 1M address, but kernel
		// is usually loaded above 1M with this 16-bits code embedded
		// use 'apinit_start' and 'apinit_end' to locate 16-bits in
		// linker script and move them to target address (<1M)
		memmove((void *)ADDR_AP_REAL, apinit_start,
			(addr_t) apinit_end - (addr_t) apinit_start);
		lapic_startap(cpuset[n].proc_id, ADDR_AP_REAL);
		printk("CPU(AP) #%d is up ...\n", cpuset[n].proc_id);
	}
}
