#ifndef IOAPIC_H
#define IOAPIC_H

#ifndef MP_H
#error "Include mp.h instead"
#endif

extern uchar_t ioapic_id;
extern void init_io_apic();
extern void ioapic_enable(int irq, int cpunum);

#endif
