#ifndef PRINT_H
#define PRINT_H

#include "C80.h"
#include "vargs.h"

extern int sprintf(char *buff, const char *format, ...);
extern void printk(const char *format, ...);
extern void printk_color(real_color_t back, real_color_t fore,
			 const char *format, ...);

#endif // PRINT_H
