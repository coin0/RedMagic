#include "common.h"
#include "mp.h"
#include "klog.h"
#include "interrupt.h"
#include "timer.h"
#include "rtc.h"
#include "isr.h"

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
#define INIT       0x00000500	// INIT/RESET
#define STARTUP    0x00000600	// Startup IPI
#define DELIVS     0x00001000	// Delivery status
#define ASSERT_    0x00004000	// Assert interrupt (vs deassert)
#define DEASSERT   0x00000000
#define LEVEL      0x00008000	// Level triggered
#define BCAST      0x00080000	// Send to all APICs, including self.
#define BUSY       0x00001000
#define FIXED      0x00000000
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

static void lapic_write(int index, int value)
{
	lapic_regp[index] = value;

	// wait for write to finish, by reading
	lapic_regp[ID];
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
static uint_t intval = 0;

static void isr_apic_tm_test(registers_t * regs);
static uint_t test_intrs_per_second();
inline static void intr_reset();
inline static void intr_inc();
inline static void intr_get(uint_t * i);

inline static void intr_reset()
{
	intval = 0;
}

inline static void intr_inc()
{
	intval++;
}

inline static void intr_get(uint_t * i)
{
	*i = intval;
}

static void isr_apic_tm_test(registers_t * regs)
{
	intr_inc();
}

#define R1 80
#define D1 100000
#define R2 40
#define D2 50000
#define R3 20
#define D3 10000
#define R4 10
#define D4 3000
#define R5 2
#define D5 1000

void lapic_init_timer(_u32 frequency)
{
	uint_t intrs, icr, freq;

	icr = TIMER_APIC_DEFAULT_ICR;
	lapic_set_timer(icr);

	for (freq = frequency;;) {
		// test how many ticks in one second, so we can get 1sec and 1msec
		intrs = test_intrs_per_second();
		log_dbg(LOG_CPU "apic timer: [[ %d ]]\n", intrs);

		// a simple way to adjust interrupt interval
		if (intrs <= freq - R1)
			icr -= D1;
		else if (intrs >= freq + R1)
			icr += D1;
		else {
			if (intrs <= freq - R2)
				icr -= D2;
			else if (intrs >= freq + R2)
				icr += D2;
			else {
				if (intrs <= freq - R3)
					icr -= D3;
				else if (intrs >= freq + R3)
					icr += D3;
				else {
					if (intrs <= freq - R4)
						icr -= D4;
					else if (intrs >= freq + R4)
						icr += D4;
					else {
						if (intrs <= freq - R5)
							icr -= D5;
						else if (intrs >= freq + R5)
							icr += D5;
						else
							break;
					}
				}
			}
		}
		lapic_set_timer(icr);
		log_dbg(LOG_CPU "ICR: %d\n", icr);
	}
}

static uint_t test_intrs_per_second()
{
	uint_t intrs;
	rtcdate_t rtc1, rtc2;
	isr_t isr;

	rtc_time(&rtc1);
	isr = register_interrupt_handler(IRQ_TIMER, &isr_apic_tm_test);
	do {
		rtc_time(&rtc2);
		intr_reset();
	} while (rtc1.second == rtc2.second);
	do {
		rtc_time(&rtc1);
	} while (rtc1.second == rtc2.second);
	register_interrupt_handler(IRQ_TIMER, isr);
	intr_get(&intrs);

	return intrs;
}

///// end of APIC timer ///////
