#include "debug.h"
#include "print.h"
#include "system.h"

void panic(const char *msg, const char *file, _u32 line)
{
	// disable interrupt
	local_irq_disable();

	// TODO panic handler

	printk("\n*** KERNEL PANIC: %s, in %s line %d\n", msg, file, line);

#ifdef MODE_DBG
	print_stack_trace();
#else
	printk("Stack trace not available, use debug-on kernel.\n");
#endif
	printk("***\n");

	// TODO handler done

	while (1) ;
}
