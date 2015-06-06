#include "debug.h"
#include "print.h"
#include "system.h"
#include "rtc.h"

void panic(const char *msg, const char *file, _u32 line)
{
	int wait_hlt = 2;

	// disable interrupt
	local_irq_disable();

	if (mpinfo.ismp) {
		// send SMP IPI stop command to other CPUs
		printk("\nwait %d seconds for other processors to stop\n",
		       wait_hlt);
		smp_halt_others();
		rtc_delay(wait_hlt);
	}
	// TODO panic handler, such as memory dump and panic callbacks

	printk("\n*** KERNEL PANIC on CPU #%u: %s, in %s line %d\n",
	       get_processor()->proc_id, msg, file, line);
	print_stack_trace();
	printk("***\n");

	while (1) ;
}
