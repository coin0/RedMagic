/*
 *      Red Magic 1996 - 2015
 *
 *      cfs.c - a common unix-like virtual filesystem layer
 *
 *      2015 Lin Coin - initial version
 */

#include "common.h"
#include "cfs.h"
#include "string.h"
#include "heap.h"
#include "klog.h"

#define CFS_BOOT_BLOCK_I   0
#define CFS_SUPER_BLOCK_I  1
#define CFS_BITMAP_BLOCK_I 2

ssize_t cfs_format(const char *name, blk_dev_t * bdev, void *opt);
super_block_t *cfs_mount(const char *name, int flags, blk_dev_t * bdev,
			 void *opt);

static filesystem_t cfs = {
	.name = "cfs_v1",
	.mount = cfs_mount,
	.format = cfs_format
};

//
// filesystem basic
//

int initfs_cfs()
{
	return register_filesystem(&cfs);
}

ssize_t cfs_format(const char *name, blk_dev_t * bdev, void *opt)
{
	uint_t fs_bsize = CFS_BLOCK_SIZE;
	char *buf;
	size_t n;
	int status;
	ssize_t fmt_blks;

	// TODO opt - specify block_size

	// now start from first block
	fmt_blks = 0;

	// cleanup first block (boot block)
	buf = (char *)kmalloc(fs_bsize);
	bzero(buf, fs_bsize);
	n = bdev_calc_nblks(bdev, fs_bsize);
	status = bdev_write_seq(bdev, CFS_BOOT_BLOCK_I, n, buf);
	if (status != OK) {
		log_err(LOG_CFS "format: boot_block on bdev 0x%08X", bdev);
		goto out;
	}
	fmt_blks++;

	// now super-block

	// now bitmaps

	// now free i-node blocks

	//bdev_read_seq(bdev, 0, bdev_calc_nblks(bdev, fs_bsize), buf);

	// coin front 2

      out:
	kfree(buf);

	return 0;
}

super_block_t *cfs_mount(const char *name, int flags, blk_dev_t * bdev,
			 void *opt)
{
	return NULL;
}

// superblock operations

// inode operations

// bitmap functions

// TODO - log area functions

//
// file operations
//

int file_open(inode_t * inode, file_t * filp)
{
	return 0;
}

loff_t file_seek(file_t * file, loff_t offset, int origin)
{
	return 0;
}

int file_release(inode_t * inode, file_t * filp)
{
	return 0;
}

ssize_t file_read(file_t * filp, char *buf, size_t len, loff_t * ppos)
{
	return 0;
}

ssize_t file_write(file_t * filp, const char *buf, size_t len, loff_t * ppos)
{
	return 0;
}

// TODO ioctl
