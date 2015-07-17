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

#define CFS_BOOT_BLOCK    0
#define CFS_SUPER_BLOCK   1
#define CFS_INODE_BLOCK   2

static ssize_t cfs_format(blk_dev_t * bdev, void *opt);
static super_block_t *cfs_mount(blk_dev_t * bdev, int flags, void *opt);

static filesystem_t cfs = {
	.name = CFS_FS_NAME,
	.mount = cfs_mount,
	.format = cfs_format
};

//
// static private functions
//

// IO
static int cfs_zero_block(blk_dev_t * bdev, uint_t index, size_t fs_bsize);
static int cfs_write_block(blk_dev_t * bdev, char *buf, uint_t index,
			   size_t fs_bsize);
static int cfs_write_seq(blk_dev_t * bdev, char *buf, uint_t index,
			 size_t fs_bsize, size_t fs_nblks);
static int cfs_read_block(blk_dev_t * bdev, char *buf, uint_t index,
			  size_t fs_bsize);

// formatting
static int cfs_fmt_bootblock(blk_dev_t * bdev, const cfs_dsuper_t * dsb);
static int cfs_setup_superblock(blk_dev_t * bdev, void *opt,
				cfs_dsuper_t * dsb);
static int cfs_fmt_inode_area(blk_dev_t * bdev, const cfs_dsuper_t * dsb);
static int cfs_fmt_inode_bitmap(blk_dev_t * bdev, const cfs_dsuper_t * dsb);
static int cfs_fmt_data_bitmap(blk_dev_t * bdev, const cfs_dsuper_t * dsb);
static int cfs_setup_root_inode(blk_dev_t * bdev, cfs_dsuper_t * dsb);

//
// filesystem basic
//

int initfs_cfs()
{
	return register_filesystem(&cfs);
}

static int cfs_read_block(blk_dev_t * bdev, char *buf, uint_t index,
			  size_t fs_bsize)
{
	int status;
	size_t nbblks;

	nbblks = bdev_calc_nblks(bdev, fs_bsize);
	status = bdev_read_seq(bdev, buf, index * nbblks, nbblks);

	return status;
}

static int cfs_zero_block(blk_dev_t * bdev, uint_t index, size_t fs_bsize)
{
	char *buf;
	int status = OK;

	buf = (char *)kmalloc(fs_bsize);
	if (buf == NULL)
		return -1;
	bzero(buf, fs_bsize);
	status = cfs_write_block(bdev, buf, index, fs_bsize);
	if (status != OK) {
		status = -2;
		goto write_error;
	}

      write_error:
	kfree(buf);

	return status;
}

static int cfs_write_block(blk_dev_t * bdev, char *buf, uint_t index,
			   size_t fs_bsize)
{
	return cfs_write_seq(bdev, buf, index, fs_bsize, 1);
}

static int cfs_write_seq(blk_dev_t * bdev, char *buf, uint_t index,
			 size_t fs_bsize, size_t fs_nblks)
{
	uint_t bdev_nblks;
	int status;

	bdev_nblks = bdev_calc_nblks(bdev, fs_bsize);
	status =
	    bdev_write_seq(bdev, buf, bdev_nblks * index,
			   fs_nblks * bdev_nblks);

	return status;
}

static ssize_t cfs_format(blk_dev_t * bdev, void *opt)
{
	cfs_dsuper_t dsb;

	if (cfs_setup_superblock(bdev, opt, &dsb) != OK) {
		log_err(LOG_CFS "format: superblock on bdev 0x%08X\n", bdev);
		return -1;
	}
	if (cfs_fmt_bootblock(bdev, &dsb) != OK) {
		log_err(LOG_CFS "format: boot_block on bdev 0x%08X\n", bdev);
		return -2;
	}
	if (cfs_fmt_inode_area(bdev, &dsb) != OK) {
		log_err(LOG_CFS "format: inode block on bdev 0x%08X\n", bdev);
		return -3;
	}
	if (cfs_fmt_inode_bitmap(bdev, &dsb) != OK) {
		log_err(LOG_CFS "format: inode bitmap on bdev 0x%08X\n", bdev);
		return -4;
	}
	if (cfs_fmt_data_bitmap(bdev, &dsb) != OK) {
		log_err(LOG_CFS "format: data bitmap on bdev 0x%08X\n", bdev);
		return -5;
	}
	if (cfs_setup_root_inode(bdev, &dsb) != OK) {
		log_err(LOG_CFS "format: setup root on bdev 0x%08X\n", bdev);
		return -6;
	}
	if (bdev_sync_buffer(bdev) != OK) {
		log_err(LOG_CFS "format: sync failed on bdev 0x%08X\n", bdev);
		return -7;
	}

	return OK;
}

static int cfs_fmt_bootblock(blk_dev_t * bdev, const cfs_dsuper_t * dsb)
{
	return cfs_zero_block(bdev, CFS_BOOT_BLOCK, dsb->block_size);
}

static int cfs_setup_superblock(blk_dev_t * bdev, void *opt, cfs_dsuper_t * dsb)
{
	_u32 fs_bsize, max_blocks;
	char *buf;
	int status = OK;

	// give default values then parse options
	fs_bsize = CFS_BLOCK_SIZE;
	max_blocks = (bdev->total_blks * bdev->block_size) / fs_bsize;
	if (opt != NULL) {
		if (((cfs_fmt_opt_t *) opt)->block_size != 0)
			fs_bsize = ((cfs_fmt_opt_t *) opt)->block_size;
		if (((cfs_fmt_opt_t *) opt)->max_blocks != 0)
			max_blocks = ((cfs_fmt_opt_t *) opt)->max_blocks;
	}
	// ** main **
	dsb->block_size = fs_bsize;
	dsb->rooti = 0;

	// ** super **
	dsb->sb_start = CFS_SUPER_BLOCK;

	// ** inode **
	dsb->inode_start = CFS_INODE_BLOCK;
	dsb->inode_nblks =
	    MATH_DIV_CEIL(CFS_MAX_INODES, fs_bsize / sizeof(cfs_dinode_t));
	dsb->ninodes = CFS_MAX_INODES;

	// ** bitmaps **
	dsb->ibmap = dsb->inode_start + dsb->inode_nblks;
	dsb->ibmap_nblks = MATH_DIV_CEIL(CFS_MAX_INODES, (fs_bsize * 8));
	dsb->bbmap = dsb->ibmap + dsb->ibmap_nblks;
	// calculate number of bbmap needed
	// keypoint (bbmap_nblks : data_nblks = 1 : fs_bsize * 8(bits))
	dsb->bbmap_nblks =
	    MATH_DIV_CEIL(max_blocks - dsb->bbmap - 1, 1 + fs_bsize * 8);

	// ** data **
	dsb->data_start = dsb->bbmap + dsb->bbmap_nblks;
	ASSERT(dsb->data_start < max_blocks);
	// rest of blocks are data
	dsb->nblocks = max_blocks - dsb->data_start;

	// ** sync **
	// write superblock to disk
	buf = (char *)kmalloc(sizeof(fs_bsize));
	if (buf == NULL)
		return -1;
	bzero(buf, fs_bsize);
	memcpy(buf, (char *)&dsb, sizeof(dsb));
	status = cfs_write_block(bdev, buf, CFS_SUPER_BLOCK, fs_bsize);
	if (status != OK)
		status = -2;
	kfree(buf);

	return status;
}

static int cfs_fmt_inode_area(blk_dev_t * bdev, const cfs_dsuper_t * dsb)
{
	char *buf;
	cfs_dinode_t dino;
	int n;
	const size_t isize = sizeof(cfs_dinode_t);
	const size_t bsize = dsb->block_size;
	int status = OK;

	buf = (char *)kmalloc(bsize);
	if (buf == NULL)
		return -1;
	bzero(buf, bsize);
	bzero(&dino, isize);
	dino.type = CFS_INO_NULL;

	// fill the block with empty inodes
	for (n = 0; n < bsize / isize; n++)
		memcpy(buf + isize * n, (char *)&dino, isize);
	for (n = 0; n < dsb->inode_nblks; n++) {
		status =
		    cfs_write_block(bdev, buf, dsb->inode_start + n, bsize);
		if (status != OK) {
			status = -2;
			goto out;
		}
	}

      out:
	kfree(buf);

	return status;
}

static int cfs_fmt_inode_bitmap(blk_dev_t * bdev, const cfs_dsuper_t * dsb)
{
	uint_t n;
	int status = OK;

	for (n = 0; n < dsb->ibmap_nblks; n++) {
		status = cfs_zero_block(bdev, dsb->ibmap + n, dsb->block_size);
		if (status != OK)
			return -1;
	}

	return status;
}

static int cfs_fmt_data_bitmap(blk_dev_t * bdev, const cfs_dsuper_t * dsb)
{
	uint_t n;
	int status = OK;

	for (n = 0; n < dsb->bbmap_nblks; n++) {
		status = cfs_zero_block(bdev, dsb->bbmap + n, dsb->block_size);
		if (status != OK)
			return -1;
	}

	return status;
}

static int cfs_setup_root_inode(blk_dev_t * bdev, cfs_dsuper_t * dsb)
{
	cfs_dinode_t dino;
	int status = OK;
	char *buf;

	// create an inode (dir type) and flush to storage
	bzero((char *)&dino, sizeof(dino));
	strcpy((char *)dino.filename, "__CFS_ROOT__");
	dino.type = CFS_INO_DIR;
	dino.size = 1;
	dino.parent = 0;	// self
	bzero(dino.data, sizeof(dino.data));

	buf = (char *)kmalloc(dsb->block_size);
	if (buf == NULL)
		return -1;
	status = cfs_read_block(bdev, buf, CFS_INODE_BLOCK, dsb->block_size);
	if (status != OK) {
		status = -2;
		goto out;
	}
	memcpy(buf, (char *)&dino, sizeof(cfs_dinode_t));
	status = cfs_write_block(bdev, buf, CFS_INODE_BLOCK, dsb->block_size);
	if (status != OK) {
		status = -3;
		goto out;
	}
	// we added root inode, update both inode bmap and block bmap(size=1 block)
	bzero(buf, dsb->block_size);
	buf[0] = 1;		// bitmap is in little-endian
	status = cfs_write_block(bdev, buf, dsb->ibmap, dsb->block_size);
	if (status != OK) {
		status = -4;
		goto out;
	}
	status = cfs_write_block(bdev, buf, dsb->bbmap, dsb->block_size);
	if (status != OK) {
		status = -5;
		goto out;
	}
	// finally update the meta data in superblock
	dsb->ninodes--;
	dsb->nblocks--;
	bzero(buf, dsb->block_size);
	memcpy(buf, (char *)dsb, sizeof(cfs_dsuper_t));
	status = cfs_write_block(bdev, buf, CFS_SUPER_BLOCK, dsb->block_size);
	if (status != OK) {
		status = -6;
		goto out;
	}

      out:
	kfree(buf);

	return status;
}

static super_block_t *cfs_mount(blk_dev_t * bdev, int flags, void *opt)
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
