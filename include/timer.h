// timer.h -- Defines the interface for all PIT-related functions.
//            Written for JamesM's kernel development tutorials.

#ifndef TIMER_H
#define TIMER_H

#include "common.h"

#define CLOCK_TICK_RATE 1193180
#define CLOCK_INT_HZ    1000

// define some useful time slices
#define TICK_SEC CLOCK_INT_HZ
#define TICK_MSEC (CLOCK_INT_HZ / 1000 == 0 ? 1 : CLOCK_INT_HZ / 1000);

extern void init_timer(_u32 frequency);

typedef struct {
	uint_t enabled;
	uint_t up;
	uint_t start;
	uint_t duration;
} alarm_t;

#define time_after(a, b) ((long)(b) - (long)(a) < 0)
#define time_before(a, b) time_after(b, a)

extern void alarm_reset(alarm_t * alarm);
extern void alarm_set(alarm_t * alarm, uint_t duration);
extern void alarm_unset(alarm_t * alarm);
extern uint_t alarm_isset(alarm_t * alarm);
extern void alarm_restart(alarm_t * alarm);
extern uint_t alarm_check(alarm_t * alarm);

#endif
