#include "common.h"
#include "task.h"
#include "print.h"
#include "timer.h"
#include "locking.h"
#include "sched.h"

int v1(void *args);
int v2(void *args);

spinlock_t glock;
mutex_t mlock;

static unsigned int div = 1;
static unsigned int iv = 0;

int K_INIT(void *args)
{
	// set system clock rate
	init_timer(CLOCK_INT_HZ);

	//init_timer(CLOCK_INT_HZ);
	printk_color(rc_black, rc_blue, "\n Init Task is now running !\n");

	//spin_lock_init(&glock);
	mutex_init(&mlock);

	create_thread(v1, NULL);
	create_thread(v2, NULL);

	unsigned int i = 0;
	for (;;) {
		i++;
		if (i % div == 0 && iv != 20) {
			mutex_lock(&mlock);
			iv = 0;
			while (iv < 10) {
				printk_color(rc_black, rc_blue, "A");
				iv++;
			}
			iv = 21;
			mutex_unlock(&mlock);
		}
	}
	return 0;
}

int v1(void *args)
{
	unsigned int i = 0;

	for (;;) {
		i++;
		if (i % div == 0 && iv != 20) {
			mutex_lock(&mlock);
			iv = 0;
			while (iv < 10) {
				printk_color(rc_black, rc_red, "B");
				iv++;
			}
			iv = 22;
			mutex_unlock(&mlock);
		}
	}
	return 0;
}

int v2(void *args)
{
	unsigned int i = 0;

	for (;;) {
		i++;
		if (i % div == 0 && iv != 20) {
			mutex_lock(&mlock);
			iv = 0;
			while (iv < 10) {
				printk_color(rc_black, rc_white, "C");
				iv++;
			}
			iv = 23;
			mutex_unlock(&mlock);
		}
	}
	return 0;
}
