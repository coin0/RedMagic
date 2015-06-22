/*
 *      Red Magic 1996 - 2015
 *
 *      device.c - hardware irrelevant device frameworks
 *
 *      2015 Lin Coin - initial version
 */

#include "common.h"
#include "device.h"
#include "klog.h"
#include "heap.h"
#include "string.h"
#include "locking.h"

/* common block devices */
static blk_dev_ops_t com_blk_dev_ops;
static void __init_dev_common(dev_t * dev);
static void __init_blk_dev_common(dev_t * dev);
static int bdev_common_write_block(buf_cache_t * buf);
static int bdev_common_read_block(buf_cache_t * buf);

/* device list */
static dev_t devices[] = {
	{"ramfs", DEV_BLOCK, init_ramfs, NULL, 1, 1},
	{"hda", DEV_BLOCK, init_ide_master, NULL, 1, 1},
};

void init_dev()
{
	int status;
	uint_t i, dev_num;

	dev_num = sizeof(devices) / sizeof(dev_t);
	for (i = 0; i < dev_num; i++) {
		if (!devices[i].onboot)
			continue;
		log_info(LOG_DEV "loading %s ...\n", devices[i].name);
		__init_dev_common(&devices[i]);
		if ((status = devices[i].init_func(&devices[i])) != OK) {
			log_err(LOG_DEV "%s was not loaded, status %d\n",
				devices[i].name, status);
			continue;
		}
	}
}

dev_t *get_dev_by_name(const char *name)
{
	uint_t i, dev_num;

	dev_num = sizeof(devices) / sizeof(dev_t);
	for (i = 0; i < dev_num; i++)
		if (strcmp(devices[i].name, name) == 0)
			return &devices[i];
	return NULL;
}

static void __init_dev_common(dev_t * dev)
{
	switch (dev->type) {
	case DEV_BLOCK:
		__init_blk_dev_common(dev);
		break;
	default:
		PANIC("#BUG");
		break;
	}
}

static void __init_blk_dev_common(dev_t * dev)
{
	blk_dev_t *blk_dev;

	// TODO destroy device 'kfree'
	dev->ptr = (blk_dev_t *) kmalloc(sizeof(blk_dev_t));
	if (dev->ptr == NULL)
		PANIC("common blk_dev_init failed");
	blk_dev = dev->ptr;

	blk_dev->buf_num = 0;
	blk_dev->block_size = BLOCK_SIZE;
	blk_dev->total_blks = 0;
	INIT_LIST_HEAD(&blk_dev->list);
	INIT_LIST_HEAD(&blk_dev->io);
	blk_dev->ops = &com_blk_dev_ops;
	mutex_init(&blk_dev->lock);
}

static blk_dev_ops_t com_blk_dev_ops = {
	.read_block = bdev_common_read_block,
	.write_block = bdev_common_write_block
};

static int bdev_common_write_block(buf_cache_t * buf)
{
	return OK;
}

static int bdev_common_read_block(buf_cache_t * buf)
{
	return OK;
}
