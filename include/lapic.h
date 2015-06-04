#ifndef LAPIC_H
#define LAPIC_H

extern volatile uint_t *lapic_regp;
extern void init_local_apic();
extern void lapic_eoi();
extern void lapic_init_timer(_u32 frequency);
extern void lapic_startap(uchar_t apicid, addr_t addr);

#endif
