#ifndef RTC_H
#define RTC_H

#include "common.h"

typedef struct {
	uint_t second;
	uint_t minute;
	uint_t hour;
	uint_t day;
	uint_t month;
	uint_t year;
} rtcdate_t;

inline static void rtc_test_delay(uint_t factor)
{
	uint_t i, j;

	if (factor == ~0)
		factor--;
	for (i = 0; i < factor; i++)
		for (j = 0; j < factor; j++) ;
}

extern void rtc_time(rtcdate_t * rtc);

#endif
