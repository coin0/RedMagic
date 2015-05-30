#include "common.h"
#include "mp.h"
#include "klog.h"
#include "interrupt.h"

#define IOAPIC  0xFEC00000	// Default physical address of IO APIC

#define REG_ID     0x00		// Register index: ID
#define REG_VER    0x01		// Register index: version
#define REG_TABLE  0x10		// Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000	// Interrupt disabled
#define INT_LEVEL      0x00008000	// Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000	// Active low (vs high)
#define INT_LOGICAL    0x00000800	// Destination is CPU id (vs APIC ID)

// IO APIC MMIO structure: write reg, then read or write data.
typedef struct {
	uint_t reg;
	uint_t pad[3];
	uint_t data;
} ioapic_t;

static volatile ioapic_t *ioapic;
uchar_t ioapic_id;

static uint_t ioapic_read(uint_t reg);
static void ioapic_write(uint_t reg, uint_t data);

static uint_t ioapic_read(uint_t reg)
{
	ioapic->reg = reg;
	return ioapic->data;
}

static void ioapic_write(uint_t reg, uint_t data)
{
	ioapic->reg = reg;
	ioapic->data = data;
}

void init_io_apic()
{
	int i, id, maxintr;

	if (!mpinfo.ismp)
		return;

	ioapic = (ioapic_t *) IOAPIC;
	maxintr = (ioapic_read(REG_VER) >> 16) & 0xFF;
	id = ioapic_read(REG_ID) >> 24;
	if (id != ioapic_id)
		PANIC("ioapicinit: id not equal to IO-APIC ID");

	// Mark all interrupts edge-triggered, active high, disabled,
	// and not routed to any CPUs.
	for (i = 0; i <= maxintr; i++) {
		ioapic_write(REG_TABLE + 2 * i, INT_DISABLED | (IRQ0 + i));
		ioapic_write(REG_TABLE + 2 * i + 1, 0);
	}
}

void ioapic_enable(int irq, int cpuid)
{

	if (!mpinfo.ismp) {
		printk("ioapic_enable: not mp system\n");
		return;
	}
	// Mark interrupt edge-triggered, active high,
	// enabled, and routed to the given cpu_id,
	// which happens to be that cpu's APIC ID.
	ioapic_write(REG_TABLE + 2 * irq - IRQ0, irq);
	ioapic_write(REG_TABLE + 2 * irq - IRQ0 + 1, cpuid << 24);
}
