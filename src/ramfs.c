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
#include "string.h"

#define RAMFS_PAGES 100
#define RAMFS_BUF_BLKS 100

// declarations
static blk_dev_ops_t ramfs_dev_ops;
static int ramfs_write_block(buf_cache_t * buf);
static int ramfs_read_block(buf_cache_t * buf);
static int ramfs_readwrite(int rw, buf_cache_t * buf);
static int ramfs_validate_rw(buf_cache_t * buf);

static blk_dev_ops_t ramfs_dev_ops = {
	.read_block = ramfs_read_block,
	.write_block = ramfs_write_block
};

static int ramfs_write_block(buf_cache_t * buf)
{
	ASSERT(buf->flags & B_DIRTY);
	return ramfs_readwrite(0, buf);
}

static int ramfs_read_block(buf_cache_t * buf)
{
	ASSERT(!(buf->flags & B_DIRTY));
	return ramfs_readwrite(1, buf);
}

static int ramfs_readwrite(int rw, buf_cache_t * buf)
{
	char *seek;
	blk_dev_t *bdev = buf->bdev;
	dev_ramfs_t *ramfs;

	// validation for devices and IOs
	ASSERT(bdev != NULL);
	if (ramfs_validate_rw(buf) != OK)
		return -1;

	ramfs = bdev->meta;
	seek = ramfs->ramhead + bdev->block_size * buf->blkno;

	if (rw) {
		memcpy(buf->data, seek, bdev->block_size);
	} else {
		memcpy(seek, buf->data, bdev->block_size);
	}

	return OK;
}

int init_ramfs(dev_t * dev)
{
	char *fs_head = NULL;
	blk_dev_t *blk_dev;
	dev_ramfs_t *ramfs;
	int err;

	blk_dev = dev->ptr;
	ASSERT(blk_dev != NULL);
	blk_dev->ops = &ramfs_dev_ops;

	// allocate RAM blocks
	fs_head = get_free_pages(RAMFS_PAGES);
	if (fs_head == NULL) {
		err = -2;
		goto no_page;
	}
	blk_dev->total_blks = RAMFS_PAGES * PAGE_SIZE / blk_dev->block_size;

	// init buffer cache
	err = bdev_init_buffer_cache(blk_dev, RAMFS_BUF_BLKS);
	if (err)
		goto bbuf_failed;

	// init device specific metadata
	ramfs = kmalloc(sizeof(dev_ramfs_t));
	if (ramfs == NULL) {
		err = -3;
		goto heap_failed;
	}
	ramfs->ramhead = fs_head;
	mutex_init(&ramfs->ramlock);
	blk_dev->meta = ramfs;

	log_info(LOG_RAMFS "fs_head:0x%08X, npgs:%u\n", fs_head, RAMFS_PAGES);

	return OK;

      heap_failed:
	if (ramfs != NULL)
		kfree(ramfs);
      bbuf_failed:
	if (fs_head != NULL)
		free_pages(fs_head);
      no_page:
	return err;
}

static int ramfs_validate_rw(buf_cache_t * buf)
{
	blk_dev_t *bdev = buf->bdev;

	if (bdev == NULL) {
		log_err("null bdev buffer 0x%08X\n");
		return -1;
	}
	if (buf->blkno >= bdev->total_blks) {
		log_err("invalid blkno %d of bdev 0x%08X\n", buf->blkno, bdev);
		return -1;
	}

	return OK;
}
