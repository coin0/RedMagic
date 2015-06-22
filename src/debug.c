/*
 *      Red Magic 1996 - 2015
 *
 *      debug.c - debug functions to help print useful information at runtime
 *
 *      2015 Lin Coin - initial version
 */

#include "debug.h"
#include "print.h"
#include "elf.h"
#include "multiboot.h"

static elf_t kernel_elf;

void init_debug(multiboot_t * mboot_ptr)
{
	kernel_elf = elf_from_multiboot(mboot_ptr);
}

void print_cur_status()
{
	static int round = 0;
	_u16 cs, ds, es, ss;

	asm volatile ("mov %%cs, %0;"
		      "mov %%ds, %1;"
		      "mov %%es, %2;"
		      "mov %%ss, %3;":"=m" (cs), "=m"(ds), "=m"(es), "=m"(ss));

	printk("print_cur_status %d\n", round);
	printk("  @ring %d\n", cs & 0x3);
	printk("   cs = 0x%016x\n", cs);
	printk("   ds = 0x%016x\n", ds);
	printk("   es = 0x%016x\n", es);
	printk("   ss = 0x%016x\n", ss);
	++round;
}

void print_stack_trace()
{
	_u32 *ebp, *eip;

	asm volatile ("mov %%ebp, %0":"=r" (ebp));
	while (ebp) {
		// eip will be the last to be pushed before function call
		// function will push ebp and then assign esp upon it
		// get eip(func entry) and the stack-top(ebp) of the last function call 
		eip = ebp + 1;
		printk("   [0x%x] %s\n", *eip,
		       elf_lookup_symbol(*eip, &kernel_elf));
		ebp = (_u32 *) * ebp;
	}
}

void print_memory(void *bp, _u32 lines)
{
	_u32 *p = (_u32 *) bp;
	for (; lines > 0; lines--) {
		printk("[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X\n",
		       (_u32) p, *p, *(p + 1), *(p + 2), *(p + 3));
		p += 4;
	}
}
