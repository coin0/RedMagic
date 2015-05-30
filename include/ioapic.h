#ifndef IOAPIC_H
#define IOAPIC_H

extern uchar_t ioapic_id;
extern void init_io_apic();
extern void ioapic_enable(int irq, int cpunum);

#endif
