#include "common.h"
#include "device.h"
#include "klog.h"
#include "mm.h"
#include "heap.h"

#define RAMFS_PAGES 10
#define RAMFS_BUF_BLKS 100

static uchar_t *ramfs = NULL;

// declarations
static blk_dev_ops_t ramfs_dev_ops;
static int ramfs_write_block(buf_cache_t * buf);
static int ramfs_read_block(buf_cache_t * buf, uint_t blkno);

static blk_dev_ops_t ramfs_dev_ops = {
	.read_block = ramfs_read_block,
	.write_block = ramfs_write_block
};

static int ramfs_write_block(buf_cache_t * buf)
{
	return OK;
}

#include "cpu.h"
static int ramfs_read_block(buf_cache_t * buf, uint_t blkno)
{
	cpu_state_t *cpu = get_processor();
	printk("proc:%u, blkno: %u\n", cpu->proc_id, blkno);

	return OK;
}

int init_ramfs(dev_t * dev)
{
	blk_dev_t *blk_dev;
	int err;

	blk_dev = dev->ptr;
	ASSERT(blk_dev != NULL);
	blk_dev->ops = &ramfs_dev_ops;

	// allocate RAM blocks
	ramfs = get_free_pages(RAMFS_PAGES);
	if (ramfs == NULL)
		return -1;
	blk_dev->total_blks = RAMFS_PAGES / BLOCK_SIZE;

	// init buffer cache
	err = bdev_init_buffer_cache(blk_dev, RAMFS_BUF_BLKS);
	if (err)
		return err;

	return OK;
}
