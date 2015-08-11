#ifndef CFS_H
#define CFS_H

#include "common.h"
#include "fs.h"
#include "device.h"
#include "locking.h"

#define CFS_FS_NAME "cfs_v1"
#define CFS_MAGIC   0xEF0002FE

// inode types
#define CFS_INO_NULL  0x00
#define CFS_INO_REG   0x01
#define CFS_INO_DIR   0x02
#define CFS_INO_DEV   0x08

//
// On-Disk struct
//

/* CFS on-disk layout
|0 boot block|1 super block|2 inodes|3 inode-bitmap|4 block-bitmap|5 blocks|
*/

#define CFS_INDEX_LEN 8
#define CFS_INDEX_D   0		// fixed 0
#define CFS_INDEX_L2  5		// 1-7
#define CFS_INDEX_L3  7		// 2-7

#define CFS_FNAME_LEN  256
#define CFS_BLOCK_SIZE 1024	// default block size

#define CFS_MAX_INODES  500

typedef struct cfs_dinode {
	_u8 filename[CFS_FNAME_LEN];
	_u8 type;
	_u32 size;
	_u32 parent;

	// data fields - blkno of next blk
	_u32 data[CFS_INDEX_LEN];
} __attribute__ ((packed)) cfs_dinode_t;

typedef struct cfs_dsuper {
	_u32 magic;		// cfs magic
	_u32 block_size;	// filesystem block size
	_u32 rooti;		// root inode

	// super
	_u32 sb_start;		// block index of superblock

	// bitmap
	_u32 bbmap;		// index of block bitmap block
	_u32 bbmap_nblks;	// number of block bitmap
	_u32 ibmap;		// index of inode bitmap block
	_u32 ibmap_nblks;	// number of inode bitmap

	// inode area
	_u32 inode_start;	// index of start of inode blocks
	_u32 inode_nblks;	// number of blocks to store inode info
	_u32 ninodes;		// number of inodes available

	// data area
	_u32 data_start;	// index of start of datablocks
	_u32 nblocks;		// number of blocks available
} __attribute__ ((packed)) cfs_dsuper_t;

//
// In-Memory struct
//

typedef struct cfs_inode {
	blk_dev_t *bdev;
	_u32 ino;
	_u32 reference;
	_u8 flags;
	cfs_dinode_t dinode;
	spinlock_t ilock;
} cfs_inode_t;

typedef struct cfs_file {
	_u8 type;
	_u8 access;
	_u32 reference;
	cfs_inode_t *iptr;
} cfs_file_t;

typedef struct cfs_dentry {
} cfs_dentry_t;

typedef struct cfs_super {
	blk_dev_t *bdev;
	_u8 flag_dirty;
	_u32 reference;
	cfs_dsuper_t dsuper;
	spinlock_t s_lock;
} cfs_super_t;

extern int initfs_cfs();

#endif
