/*
 *      Red Magic 1996 - 2015
 *
 *      ramfs.c - block devices based on RAM
 *
 *      2015 Lin Coin - initial version
 */

#include "common.h"
#include "device.h"
#include "klog.h"
#include "mm.h"
#include "heap.h"

#define RAMFS_PAGES 10
#define RAMFS_BUF_BLKS 100

// declarations
static blk_dev_ops_t ramfs_dev_ops;
static int ramfs_write_block(buf_cache_t * buf);
static int ramfs_read_block(buf_cache_t * buf);

static blk_dev_ops_t ramfs_dev_ops = {
	.read_block = ramfs_read_block,
	.write_block = ramfs_write_block
};

static int ramfs_write_block(buf_cache_t * buf)
{
	printk("<W %d>\n", buf->blkno);
	return OK;
}

static int ramfs_read_block(buf_cache_t * buf)
{
	printk("[R %d]\n", buf->blkno);
	return OK;
}

int init_ramfs(dev_t * dev)
{
	uchar_t *fs_head = NULL;
	blk_dev_t *blk_dev;
	dev_ramfs_t *ramfs;
	int err;

	blk_dev = dev->ptr;
	ASSERT(blk_dev != NULL);
	blk_dev->ops = &ramfs_dev_ops;

	// allocate RAM blocks
	fs_head = get_free_pages(RAMFS_PAGES);
	if (fs_head == NULL)
		return -2;
	blk_dev->total_blks = RAMFS_PAGES / BLOCK_SIZE;

	// init buffer cache
	err = bdev_init_buffer_cache(blk_dev, RAMFS_BUF_BLKS);
	if (err)
		return err;

	// init device specific metadata
	ramfs = kmalloc(sizeof(dev_ramfs_t));
	if (ramfs == NULL)
		return -3;
	ramfs->ramhead = fs_head;
	mutex_init(&ramfs->ramlock);
	blk_dev->meta = ramfs;

	return OK;
}
