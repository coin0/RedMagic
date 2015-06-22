/*
 *      Red Magic 1996 - 2015
 *
 *      lapic.c - local APIC code for intel chipset
 *
 *      2015 Lin Coin - initial version based on xv6
 */

#include "common.h"
#include "mp.h"
#include "klog.h"
#include "interrupt.h"
#include "timer.h"
#include "rtc.h"
#include "paging.h"

// m from xv6
// Local APIC registers, divided by 4 for use as uint[] indices.
#define ID      (0x0020/4)	// ID
#define VER     (0x0030/4)	// Version
#define TPR     (0x0080/4)	// Task Priority
#define EOI     (0x00B0/4)	// EOI
#define SVR     (0x00F0/4)	// Spurious Interrupt Vector
#define ENABLE     0x00000100	// Unit Enable
#define ESR     (0x0280/4)	// Error Status
#define ICRLO   (0x0300/4)	// Interrupt Command
#define FIXED      0x00000000
#define LOWEST     0x00000100
#define SMI        0x00000200
#define REMRD      0x00000300
#define NMI        0x00000400
#define INIT       0x00000500	// INIT/RESET
#define STARTUP    0x00000600	// Startup IPI
#define DELIVS     0x00001000	// Delivery status
#define _ASSERT    0x00004000	// Assert interrupt (vs deassert)
#define DEASSERT   0x00000000
#define LEVEL      0x00008000	// Level triggered
#define SELF       0x00040000	// Send to self.
#define BCAST      0x00080000	// Send to all APICs, including self.
#define MCAST      0x000C0000	// Send to all APICs execept self.
#define BUSY       0x00001000
#define ICRHI   (0x0310/4)	// Interrupt Command [63:32]
#define TIMER   (0x0320/4)	// Local Vector Table 0 (TIMER)
#define X1         0x0000000B	// divide counts by 1
#define PERIODIC   0x00020000	// Periodic
#define PCINT   (0x0340/4)	// Performance Counter LVT
#define LINT0   (0x0350/4)	// Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)	// Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)	// Local Vector Table 3 (ERROR)
#define MASKED     0x00010000	// Interrupt masked
#define TICR    (0x0380/4)	// Timer Initial Count
#define TCCR    (0x0390/4)	// Timer Current Count
#define TDCR    (0x03E0/4)	// Timer Divide Configuration

// global lapic register pointer
// each processor has its own local apic, and in mproc.c, init_mp will find
// mp configurations for lapic address, which has been mapped @ the same
// address in main memory, so we just need one following pointer for all.
volatile uint_t *lapic_regp = NULL;

static void lapic_write(int index, int value);
static void lapic_set_timer(uint_t icr);
static void lapic_irq_stop_cpu_handler(registers_t * regs);

static void lapic_write(int index, int value)
{
	lapic_regp[index] = value;

	// wait for write to finish, by reading
	lapic_regp[ID];
}

int lapic_get_id()
{
	if (lapic_regp != NULL)
		return lapic_regp[ID] >> 24;

	return -1;
}

void init_local_apic()
{
	ASSERT(lapic_regp != NULL);

	// Enable local APIC; set spurious interrupt vector.
	lapic_write(SVR, ENABLE | IRQ_SPURIOUS);

	// Disable logical interrupt lines.
	lapic_write(LINT0, MASKED);
	lapic_write(LINT1, MASKED);

	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if (((lapic_regp[VER] >> 16) & 0xFF) >= 4)
		lapic_write(PCINT, MASKED);

	// Map error interrupt to IRQ_ERROR.
	lapic_write(ERROR, IRQ_ERROR);

	// Clear error status register (requires back-to-back writes).
	lapic_write(ESR, 0);
	lapic_write(ESR, 0);

	// Ack any outstanding interrupts.
	lapic_write(EOI, 0);

	// Send an Init Level De-Assert to synchronise arbitration ID's.
	lapic_write(ICRHI, 0);
	lapic_write(ICRLO, BCAST | INIT | LEVEL);
	while (lapic_regp[ICRLO] & DELIVS) ;

	// Enable interrupts on the APIC (but not on the processor).
	lapic_write(TPR, 0);

	// register IPI command handlers
	register_interrupt_handler(IRQ_STOP_CPU, &lapic_irq_stop_cpu_handler);
}

// Acknowledge interrupt.
void lapic_eoi(void)
{
	if (!mpinfo.ismp)
		return;

	ASSERT(lapic_regp != NULL);
	lapic_write(EOI, 0);
}

static void lapic_set_timer(uint_t icr)
{
	// The timer repeatedly counts down at bus frequency
	// from lapic[TICR] and then issues an interrupt.
	// If xv6 cared more about precise timekeeping,
	// TICR would be calibrated using an external time source.
	lapic_write(TDCR, X1);
	lapic_write(TIMER, PERIODIC | IRQ_TIMER);
	lapic_write(TICR, icr);
}

///////////////
//  APIC timer
///////////////
void lapic_init_timer(_u32 frequency)
{
	rtcdate_t rtc1, rtc2;
	uint_t icr = ~0;
	uint_t ccr;

	rtc_time(&rtc1);
	do {
		rtc_time(&rtc2);
	} while (rtc1.second == rtc2.second);

	lapic_set_timer(icr);
	do {
		rtc_time(&rtc1);
	} while (rtc1.second == rtc2.second);
	ccr = lapic_regp[TCCR];

	lapic_set_timer((icr - ccr) / frequency);
	log_dbg(LOG_CPU "ICR delta in 1 sec: %u\n", icr - ccr);
}

///// end of APIC timer ///////

// Start additional processor running entry code at addr.
// See Appendix B of MultiProcessor Specification.
void lapic_startap(uchar_t apicid, addr_t addr)
{
	int i;
	ushort_t *wrv;

	// "The BSP must initialize CMOS shutdown code to 0AH
	// and the warm reset vector (DWORD based at 40:67) to point at
	// the AP startup code prior to the [universal startup algorithm]."
	outb(CMOS_PORT, 0xF);	// offset 0xF is shutdown code
	outb(CMOS_PORT + 1, 0x0A);
	wrv = (ushort_t *) __phys_to_virt_lm(NULL, (0x40 << 4 | 0x67));	// Warm reset vector
	wrv[0] = 0;
	wrv[1] = addr >> 4;

	// "Universal startup algorithm."
	// Send INIT (level-triggered) interrupt to reset other CPU.
	lapic_write(ICRHI, apicid << 24);
	lapic_write(ICRLO, INIT | LEVEL | _ASSERT);
	rtc_test_delay(100);
	lapic_write(ICRLO, INIT | LEVEL);
	rtc_test_delay(200);	// should be 10ms, but too slow in Bochs!

	// Send startup IPI (twice!) to enter code.
	// Regular hardware is supposed to only accept a STARTUP
	// when it is in the halted state due to an INIT.  So the second
	// should be ignored, but it is part of the official Intel algorithm.
	// Bochs complains about the second one.  Too bad for Bochs.
	for (i = 0; i < 2; i++) {
		lapic_write(ICRHI, apicid << 24);
		lapic_write(ICRLO, STARTUP | (addr >> 12));
		rtc_test_delay(200);
	}
}

void lapic_send_ipi_bcast(int vector)
{
	uint_t cfg = 0;

	cfg |= FIXED | BCAST | vector;
	lapic_write(ICRLO, cfg);
}

void lapic_send_ipi_mcast(int vector)
{
	uint_t cfg = 0;

	cfg |= FIXED | MCAST | vector;
	lapic_write(ICRLO, cfg);
}

void lapic_send_ipi_self(int vector)
{
	uint_t cfg = 0;

	cfg |= FIXED | SELF | vector;
	lapic_write(ICRLO, cfg);
}

static void lapic_irq_stop_cpu_handler(registers_t * regs)
{
	printk("CPU #%u received IRQ_STOP_CPU, stopped.\n", lapic_get_id());
	for (;;) ;
}
