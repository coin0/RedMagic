#ifndef LAPIC_H
#define LAPIC_H

#ifndef MP_H
#error "Include mp.h instead"
#endif

#include "common.h"

extern volatile uint_t *lapic_regp;
extern int lapic_get_id();
extern void init_local_apic();
extern void lapic_eoi();
extern void lapic_init_timer(_u32 frequency);
extern void lapic_startap(uchar_t apicid, addr_t addr);

#include "interrupt.h"

/* IPI commands sender */
extern void lapic_send_ipi_bcast(int vector);
extern void lapic_send_ipi_mcast(int vector);
extern void lapic_send_ipi_self(int vector);

#endif
