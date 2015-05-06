#include "common.h"
#include "task.h"
#include "print.h"
#include "timer.h"

int v1(void *args);
int v2(void *args);

int K_INIT(void *args)
{
	// set system clock rate
	init_timer(CLOCK_INT_HZ);

	//init_timer(CLOCK_INT_HZ);
	printk_color(rc_black, rc_blue, "\n Init Task is now running !\n");

	create_thread(v1, NULL);
	create_thread(v2, NULL);

	unsigned int i = 0;
	for (;;) {
		i++;
		if (i % 10000000) {
			printk_color(rc_black, rc_blue, "A");
		}
	}
	return 0;
}

int v1(void *args)
{
	unsigned int i = 0;
	for (;;) {
		i++;
		if (i % 10000000) {
			printk_color(rc_black, rc_red, "B");
		}
	}
	return 0;
}

int v2(void *args)
{
	unsigned int i = 0;
	for (;;) {
		i++;
		if (i % 10000000) {
			printk_color(rc_black, rc_white, "C");
		}
	}
	return 0;
}
