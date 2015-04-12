// timer.c -- Initialises the PIT, and handles clock updates.
//            Written for JamesM's kernel development tutorials.

#include "common.h"
#include "timer.h"
#include "isr.h"
#include "print.h"

static _u32 tick = 0;

static void timer_callback(registers_t * regs)
{
	tick++;
	printk("Tick: %d\n", tick);
}

void init_timer(_u32 frequency)
{
	// Firstly, register our timer callback.
	register_interrupt_handler(IRQ0, &timer_callback);

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	_u32 divisor = 1193180 / frequency;

	// Send the command byte.
	outb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	_u8 l = (_u8) (divisor & 0xFF);
	_u8 h = (_u8) ((divisor >> 8) & 0xFF);

	// Send the frequency divisor.
	outb(0x40, l);
	outb(0x40, h);
}
