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
#include "locking.h"
#include "mount.h"

#define CFS_ROOT_INODE    0

#define CFS_BOOT_BLOCK    0
#define CFS_SUPER_BLOCK   1
#define CFS_INODE_BLOCK   2

static int cfs_format(blk_dev_t * bdev, kv_option_t * opt);
static super_block_t *cfs_mount(blk_dev_t * bdev, kv_option_t * opt);
static int cfs_umount(super_block_t * sb, kv_option_t * opt);

static filesystem_t cfs = {
	.name = CFS_FS_NAME,
	.mount = cfs_mount,
	.umount = cfs_umount,
	.format = cfs_format
};

static inode_t *cfs_alloc_inode(super_block_t * sb);
static int cfs_free_inode(inode_t * inode);
static int cfs_sync_fs(super_block_t * sb);
static inode_t *cfs_get_inode(uint_t ino);
static int cfs_put_inode(inode_t * inode);

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
// since read_super need to scan device block to find superblock, need limit
// the blocks to scan (do not stupidly scan the whole device)
#define CFS_READ_SUPER_MAX_BSCAN 8
static int cfs_read_super(blk_dev_t * bdev, cfs_dsuper_t * dsb);

// formatting
static int cfs_fmt_bootblock(blk_dev_t * bdev, const cfs_dsuper_t * dsb);
static int cfs_setup_superblock(blk_dev_t * bdev, kv_option_t * opt,
				cfs_dsuper_t * dsb);
static int cfs_fmt_inode_area(blk_dev_t * bdev, const cfs_dsuper_t * dsb);
static int cfs_fmt_inode_bitmap(blk_dev_t * bdev, const cfs_dsuper_t * dsb);
static int cfs_fmt_data_bitmap(blk_dev_t * bdev, const cfs_dsuper_t * dsb);
static int cfs_setup_root_inode(blk_dev_t * bdev, cfs_dsuper_t * dsb);

// superblock
static int cfs_destroy_inode_cache(cfs_super_t * sb);
static int cfs_test_umount(cfs_super_t * sb);

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

static int cfs_format(blk_dev_t * bdev, kv_option_t * opt)
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

static int cfs_setup_superblock(blk_dev_t * bdev, kv_option_t * opt,
				cfs_dsuper_t * dsb)
{
	_u32 fs_bsize, max_blocks;
	char *buf, *opt_val;
	int status = OK;

	// give default values then parse options
	fs_bsize = CFS_BLOCK_SIZE;
	max_blocks = (bdev->total_blks * bdev->block_size) / fs_bsize;

	// parse options
	if (opt != NULL) {
		opt_val = kv_option_get(opt, "bsize");
		if (opt_val != NULL)
			fs_bsize = atoi(opt_val);
		opt_val = kv_option_get(opt, "max_blk");
		if (opt_val != NULL)
			max_blocks = atoi(opt_val);
	}
	// ** main **
	dsb->magic = CFS_MAGIC;
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

static super_block_t *cfs_mount(blk_dev_t * bdev, kv_option_t * opt)
{
	cfs_super_t *sb;
	int status;

	// if already mounted, just increase reference
	sb = (cfs_super_t *) lookup_super_by_bdev(bdev);
	if (sb != NULL) {
		spin_lock(&sb->s_lock);
		sb->reference++;
		spin_unlock(&sb->s_lock);
		return (super_block_t *) sb;
	}
	sb = (cfs_super_t *) kmalloc(sizeof(cfs_super_t));
	if (sb == NULL) {
		log_err(LOG_CFS "cfs_mount: mem alloc failed\n");
		return NULL;
	}
	// read from device, get the on-disk super and copy it into in-memory super
	status = cfs_read_super(bdev, &sb->dsuper);
	if (status != OK) {
		log_err(LOG_CFS
			"cfs_mount: (%d) failed to read super\n", status);
		goto err;
	}
	// initialize in-memory struct
	spin_lock_init(&sb->s_lock);
	sb->bdev = bdev;
	sb->flag_dirty = 0;
	sb->reference = 0;

	return (super_block_t *) sb;

      err:
	kfree(sb);
	return NULL;
}

static int cfs_umount(super_block_t * sb, kv_option_t * opt)
{
	cfs_super_t *cfs_sb = (cfs_super_t *) sb;
	int status;

	if (cfs_test_umount(cfs_sb) != OK)
		return -1;

	// only if reference is 0 should we start to umount
	spin_lock(&cfs_sb->s_lock);
	cfs_sb->reference--;
	spin_unlock(&cfs_sb->s_lock);
	if (cfs_sb->reference > 0)
		return OK;

	// sync disks and check references of inodes, if everything is ok
	// release superblock, root inode
	cfs_sync_fs(sb);
	status = cfs_destroy_inode_cache(cfs_sb);
	if (status != OK)
		return -2;
	kfree(sb);

	return OK;
}

static int cfs_read_super(blk_dev_t * bdev, cfs_dsuper_t * dsb)
{
	int status = OK;
	char *buf;
	const uint_t bdev_bs = bdev->block_size;
	uint_t n;
	cfs_dsuper_t tmp_dsb;

	// we don't know block size when read_super is called, the first thing
	// to do is to search the magic field so that we can get block size and
	// the location of the super block. let's begin with the first block of
	// current block device
	ASSERT(bdev->block_size % BLOCK_SIZE == 0);
	buf = kmalloc(bdev_bs);
	if (buf == NULL)
		return -1;
	for (n = 0; n < CFS_READ_SUPER_MAX_BSCAN; n++) {
		bdev_read_block(bdev, n, buf);
		memcpy((char *)&tmp_dsb, buf, sizeof(cfs_dsuper_t));
		if (tmp_dsb.magic == CFS_MAGIC)
			break;
	}
	if (n == CFS_READ_SUPER_MAX_BSCAN) {
		status = -2;
		goto out;
	}
	status =
	    cfs_read_block(bdev, (char *)dsb, CFS_SUPER_BLOCK, n * bdev_bs);
	if (status != OK) {
		status = -3;
		goto out;
	}

      out:
	kfree(buf);

	return status;
}

static int cfs_destroy_inode_cache(cfs_super_t * sb)
{
	return OK;
}

static int cfs_test_umount(cfs_super_t * sb)
{
	return OK;
}

//
// superblock operations
//

static inode_t *cfs_alloc_inode(super_block_t * sb)
{
	return NULL;
}

static int cfs_free_inode(inode_t * inode)
{
	return OK;
}

static int cfs_sync_fs(super_block_t * sb)
{
	cfs_super_t *cfs_sb = (cfs_super_t *) sb;

	// TODO notice following comments
	// we don't care about anything, just flush the block buffer, if one
	// day we implement inode cache, bitmap cache whatever cache, we should
	// flush these (filesystem level) first then block buffers (block device
	// level)
	bdev_sync_buffer(cfs_sb->bdev);

	return OK;
}

static inode_t *cfs_get_inode(uint_t ino)
{
	return NULL;
}

static int cfs_put_inode(inode_t * inode)
{
	return OK;
}

// inode operations

// bitmap functions

// TODO - log area functions

// namex functions

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
