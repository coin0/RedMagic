// monitor.h -- Defines the interface for monitor.h
// From JamesM's kernel development tutorials.

#ifndef C80_H
#define C80_H

#include "common.h"

typedef enum {
	rc_black = 0,
	rc_blue = 1,
	rc_green = 2,
	rc_cyan = 3,
	rc_red = 4,
	rc_magenta = 5,
	rc_brown = 6,
	rc_light_grey = 7,
	rc_dark_grey = 8,
	rc_light_blue = 9,
	rc_light_green = 10,
	rc_light_cyan = 11,
	rc_light_red = 12,
	rc_light_magenta = 13,
	rc_light_brown = 14,	// yellow
	rc_white = 15
} real_color_t;

// Write a single character out to the screen.
extern void c80_put(char c);

// Clear the screen to all black.
extern void c80_clear();

// Output a null-terminated ASCII string to the monitor.
extern void c80_write(char *c);

// Output characters in textmode with color
extern void c80_write_color(char *cstr, real_color_t back, real_color_t fore);
extern void c80_putc_color(char c, real_color_t back, real_color_t fore);

#endif
