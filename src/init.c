#include "common.h"
#include "task.h"
#include "print.h"
#include "timer.h"
#include "locking.h"
#include "sched.h"
#include "timer.h"

int v1(void *args);
int v2(void *args);

int K_INIT(void *args)
{
	// set system clock rate
	init_timer(CLOCK_INT_HZ);
	printk("Kernel clock is set to %d HZ\n", CLOCK_INT_HZ);

	create_thread(v1, NULL);
	create_thread(v2, NULL);

	while (1) {
		pause(1);
		printk("init done !\n");
	}

	return 0;
}

int v1(void *args)
{
	while (1) {
		pause(1);
		printk("v1 done !\n");
	}

	return 0;
}

int v2(void *args)
{
	while (1) {
		pause(1);
		printk("v2 done !\n");
	}

	return 0;
}
