// timer.h -- Defines the interface for all PIT-related functions.
//            Written for JamesM's kernel development tutorials.

#ifndef TIMER_H
#define TIMER_H

#include "common.h"

#define CLOCK_TICK_RATE 1193180
#define CLOCK_INT_HZ    1000

extern void init_timer(_u32 frequency);

#endif
