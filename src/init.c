#include "common.h"
#include "task.h"
#include "print.h"
#include "timer.h"
#include "locking.h"
#include "sched.h"
#include "timer.h"
#include "rtc.h"

int v1(void *args);
int v2(void *args);
int v3(void *args);

static mutex_t mlock;
static semaphore_t ss;
static int mode = 1;
#include "system.h"

int K_INIT(void *args)
{
	// set system clock
	if (!mpinfo.ismp)
		init_pit_timer(CLOCK_INT_HZ);
	else
		init_apic_timer(CLOCK_INT_HZ);

	mutex_init(&mlock);
	sem_init(&ss, 2);

	create_thread(v1, NULL);
	create_thread(v2, NULL);
	create_thread(v3, NULL);

	return 0;
}

int v1(void *args)
{
	int i;
	while (1) {
		if (!mode)
			mutex_lock(&mlock);
		else
			sem_down(&ss);

		printk_color(rc_black, rc_red, "[B]");
		i = 10;
		for (; i > 0; i--) {
			printk_color(rc_black, rc_red, "B");
			pause(1);
		}

		if (!mode)
			mutex_unlock(&mlock);
		else
			sem_up(&ss);

		pause(1);
	}

	return 0;
}

int v2(void *args)
{
	int i;

	while (1) {
		if (!mode)
			mutex_lock(&mlock);
		else
			sem_down(&ss);

		printk_color(rc_black, rc_blue, "[C]");
		i = 10;
		for (; i > 0; i--) {
			printk_color(rc_black, rc_blue, "C");
			pause(1);
		}

		if (!mode)
			mutex_unlock(&mlock);
		else
			sem_up(&ss);

		pause(1);
	}

	return 0;
}

int v3(void *args)
{
	int i;

	while (1) {
		if (!mode)
			mutex_lock(&mlock);
		else
			sem_down(&ss);

		printk("[A]");
		i = 10;
		for (; i > 0; i--) {
			printk("A");
			pause(1);
		}

		if (!mode)
			mutex_unlock(&mlock);
		else
			sem_up(&ss);

		pause(1);
	}

	return 0;
}
