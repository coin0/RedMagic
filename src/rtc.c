#include "common.h"
#include "rtc.h"
#include "string.h"
#include "timer.h"

#define SECS    0x00
#define MINS    0x02
#define HOURS   0x04
#define DAY     0x07
#define MONTH   0x08
#define YEAR    0x09

#define DELAY_CMOS_READ 5

static uint_t cmos_read(uint_t reg)
{
	outb(CMOS_PORT, reg);
	rtc_test_delay(DELAY_CMOS_READ);

	return inb(CMOS_RETURN);
}

static void fill_rtcdate(rtcdate_t * rtc)
{
	rtc->second = cmos_read(SECS);
	rtc->minute = cmos_read(MINS);
	rtc->hour = cmos_read(HOURS);
	rtc->day = cmos_read(DAY);
	rtc->month = cmos_read(MONTH);
	rtc->year = cmos_read(YEAR);
}

// qemu seems to use 24-hour GWT and the values are BCD encoded
void rtc_time(rtcdate_t * rtc)
{
	rtcdate_t t1, t2;
	int sb, bcd;

	sb = cmos_read(CMOS_STATB);

	bcd = (sb & (1 << 2)) == 0;

	// make sure CMOS doesn't modify time while we read it
	for (;;) {
		fill_rtcdate(&t1);
		if (cmos_read(CMOS_STATA) & CMOS_UIP)
			continue;
		fill_rtcdate(&t2);
		if (memcmp(&t1, &t2, sizeof(t1)) == 0)
			break;
	}

	// convert
	if (bcd) {
#define    CONV(x)     (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
		CONV(second);
		CONV(minute);
		CONV(hour);
		CONV(day);
		CONV(month);
		CONV(year);
#undef     CONV
	}

	*rtc = t1;
	rtc->year += 2000;
}

void rtc_delay(uint_t seconds)
{
	rtcdate_t rtc1, rtc2;
	uint_t repeat;

	for (repeat = 0; repeat < seconds; repeat++) {
		rtc_time(&rtc2);
		do {
			rtc_time(&rtc1);
		} while (rtc1.second == rtc2.second);
	}
}
