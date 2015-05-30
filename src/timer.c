// timer.c -- Initialises the PIT, and handles clock updates.
//            Written for JamesM's kernel development tutorials.

#include "common.h"
#include "timer.h"
#include "interrupt.h"
#include "print.h"
#include "sched.h"
#include "klog.h"
#include "cpu.h"
#include "locking.h"

static _u32 ticks = 0;

#define _HZ_DIV(hz) (((hz) >= CLOCK_INT_HZ) ? 1 : CLOCK_INT_HZ / (hz))
#define IF_HZ_EQ(hz) if (!(ticks % _HZ_DIV(hz)))

static void timer_callback(registers_t * regs)
{
	cpu_state_t *cpu;

	ticks++;

	cpu = get_processor();

	IF_HZ_EQ(CHK_ALM_HZ) {
		// check alarms inside blocked threads
		check_thread_alarms();
	}

	IF_HZ_EQ(SCHED_HZ) {
		if (cpu->preempt_on)
			schedule();
	}
}

void init_timer(_u32 frequency)
{
	// Firstly, register our timer callback.
	init_timer_cb();

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	_u32 divisor = CLOCK_TICK_RATE / frequency;

	// Send the command byte.
	outb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	_u8 l = (_u8) (divisor & 0xFF);
	_u8 h = (_u8) ((divisor >> 8) & 0xFF);

	// Send the frequency divisor.
	outb(0x40, l);
	outb(0x40, h);
}

void init_timer_cb()
{
	register_interrupt_handler(IRQ0, &timer_callback);
}

/**********
   alarm
 **********/

void alarm_reset(alarm_t * alarm)
{
	alarm->enabled = 0;
	alarm->up = 0;
	alarm->start = 0;
	alarm->duration = 0;
}

void alarm_set(alarm_t * alarm, uint_t duration)
{
	alarm->enabled = 1;
	alarm->up = 0;
	alarm->start = ticks;
	alarm->duration = duration;
}

void alarm_unset(alarm_t * alarm)
{
	alarm->enabled = 0;
}

void alarm_restart(alarm_t * alarm)
{
	alarm_set(alarm, alarm->duration);
}

uint_t alarm_isset(alarm_t * alarm)
{
	return alarm->enabled;
}

uint_t alarm_check(alarm_t * alarm)
{
	uint_t now = ticks;

	if (alarm_isset(alarm))
		return time_after(now, alarm->start + alarm->duration);
	else
		return 0;
}
