#ifndef CFS_H
#define CFS_H

#include "fs.h"
#include "device.h"

// inode types
#define CFS_NULL  0x00
#define CFS_REG   0x01
#define CFS_DIR   0x02
#define CFS_DEV   0x08

//
// On-Disk struct
//

#define CFS_INDEX_LEN 8
#define CFS_INDEX_D   0		// fixed 0
#define CFS_INDEX_L2  5		// 1-7
#define CFS_INDEX_L3  7		// 2-7

#define CFS_FNAME_LEN  256
#define CFS_BLOCK_SIZE 1024	// default block size

typedef struct cfs_dinode {
	_u8 filename[CFS_FNAME_LEN];
	_u8 type;
	_u32 size;

	// data fields - blkno of next blk
	_u32 data[CFS_INDEX_LEN];
} __attribute__ ((packed)) cfs_dinode_t;

typedef struct cfs_dsuper {
	_u32 block_size;	// filesystem block size
	_u32 nblocks;		// number of blocks available
	_u32 ninodes;		// number of inodes available
	_u32 rooti;		// root inode
	_u32 bitmap;		// block number of bitmap 
} __attribute__ ((packed)) cfs_dsuper_t;

//
// In-Memory struct
//

typedef struct cfs_inode {
	blk_dev_t *bdev;
	_u32 ino;
	_u32 reference;
	_u8 flags;
	struct cfs_inode *iptr[CFS_INDEX_LEN];
	cfs_dinode_t dinode;
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
	_u8 flag_ro;
	_u8 flag_dirty;
	cfs_inode_t *rptr;
	super_ops_t *s_op;
	cfs_dsuper_t dsuper;
} cfs_super_t;

extern int initfs_cfs();

#endif
