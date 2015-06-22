/*
 *      Red Magic 1996 - 2015
 *
 *      init.c - initial entry of multi-tasking
 *
 *      2015 Lin Coin - initial version
 */

#include "common.h"
#include "task.h"
#include "print.h"
#include "timer.h"
#include "locking.h"
#include "sched.h"
#include "timer.h"
#include "rtc.h"
#include "device.h"

int v1(void *args);
int v2(void *args);
int v3(void *args);
int v(void *args);
int u(void *args);

static mutex_t mlock;
static semaphore_t ss;
static int mode = 0;
#include "system.h"

int K_INIT(void *args)
{
	dev_t *dev;
	blk_dev_t *bdev;
	uchar_t a[BLOCK_SIZE];
	uint_t i = 0;

	create_thread(u, NULL);

	dev = get_dev_by_name("ramfs");
	printk("%s\n", dev->name);
	bdev = (blk_dev_t *) (dev->ptr);
	while (1) {
		bdev_sync_buffer(bdev);
		bdev_read_buffer(bdev, i++ % 100, a);
	}
	//create_thread(v, NULL);

	return 0;
}

int u(void *args)
{
	dev_t *dev;
	blk_dev_t *bdev;
	uchar_t a[BLOCK_SIZE];

	dev = get_dev_by_name("ramfs");
	bdev = (blk_dev_t *) (dev->ptr);
	while (1) {
		bdev_write_buffer(bdev, 1, a);
	}
	return 0;
}

int v(void *args)
{
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
		i = 5;
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
		i = 5;
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

		printk_color(rc_black, rc_green, "[A]");
		i = 5;
		for (; i > 0; i--) {
			printk_color(rc_black, rc_green, "A");
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
