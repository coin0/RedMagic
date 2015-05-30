#ifndef LAPIC_H
#define LAPIC_H

extern volatile uint_t *lapic_regp;
extern void init_local_apic();
extern void lapic_eoi();

#endif
