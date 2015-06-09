/*
 *   entry of C code for bootstrap processor
 */

#include "common.h"
#include "multiboot.h"
#include "system.h"
#include "print.h"
#include "timer.h"
#include "klog.h"
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
static int __main(cpu_state_t * cpu);
static void start_smp();

int main(multiboot_t * mbp)
{
	cpu_state_t *cpu;

	// first is to save critical info from bootloader
	// and get symbol tables so we can print back traces
	mbootp = mbp;
	init_debug(mbp);

	// reset text-mode
	c80_clear();
	printk_color(rc_black, rc_red, "\t\t\t\tRed Magic\n");

	// init processors
	init_bootstrap_processor();
	cpu = get_boot_processor();
	ASSERT(cpu != NULL);

	// initialize descriptors
	init_global_descriptor_table(cpu);
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

	// start system clock
	if (!mpinfo.ismp)
		init_pit_timer(CLOCK_INT_HZ);
	else
		init_apic_timer(CLOCK_INT_HZ);

	// initialize kernel task and scheduling
	setup_init_task();
	init_sched();

	// All our initialisation calls will go in here.
	return 0xDEADBABA;
}

// this is main() function for other processors
static int __main(cpu_state_t * cpu)
{
	// init AP
	init_application_processor();

	// init segmentation
	init_global_descriptor_table(cpu);
	init_interrupt_descriptor_table();

	// enable local interrupt
	local_irq_enable();

	// init paging
	switch_page_directory(k_pdir);

	// init sched
	init_sched();

	return 0xFADEFADE;
}

static void start_smp()
{
	int n;
	void *stack;

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

		// AFTER APs enter protected mode, 32bit code need jump back to
		// kernel text section, put this address before ADDR_AP_REAL so
		// AP boot section can simply get the address value
		*(void **)(ADDR_AP_REAL - 4) = (void *)&__main;

		// same as comments above, we allocate temporary stack space 
		// for other processors, remember to free it when init task is
		// setup to run
		stack = get_free_page();
		if (stack == NULL)
			PANIC("No memory for AP stack");
		*(void **)(ADDR_AP_REAL - 8) = (void *)stack;

		// each processor should maintain its own per-cpu struct
		*(void **)(ADDR_AP_REAL - 12) = (void *)&cpuset[n];

		// startup ! startup !! startup !!!
		lapic_startap(cpuset[n].proc_id, ADDR_AP_REAL);

		printk("CPU(AP) #%d is up ...\n", cpuset[n].proc_id);
	}
}
