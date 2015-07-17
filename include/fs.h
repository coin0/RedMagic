#ifndef FS_H
#define FS_H

#include "common.h"
#include "device.h"
#include "file.h"

// replaced with types of specific fs
typedef void super_block_t;

// superblock
typedef struct {
	inode_t *(*alloc_inode) (super_block_t *);
	int (*free_inode) (inode_t *);
	int (*sync_fs) (super_block_t * sb);
} super_ops_t;

typedef struct filesystem {
	const char *name;
	// mount operations on block device and return superblock structures
	super_block_t *(*mount) (blk_dev_t *, int, void *);
	// format block device with specific filesystem
	 ssize_t(*format) (blk_dev_t *, void *);
	struct filesystem *next;
} filesystem_t;

typedef struct {
	filesystem_t *file_systems;
	mutex_t lock;
} fs_list_t;

typedef struct mnt_list {
	blk_dev_t *mnt_dev;
	char *mnt_dir;
	super_block_t *mnt_sb;
	struct mnt_list *mnt_next;
} mnt_list_t;

typedef struct {
	mnt_list_t *mnt;
} mnt_tab_t;

extern int register_filesystem(filesystem_t * fs);
extern void init_root_fs();

// high-level filesystem interfaces
extern int do_format(blk_dev_t * bdev, filesystem_t * fs, void *pts);
extern int do_mount(blk_dev_t * bdev, filesystem_t * fs, const char *dir_name,
		    int flags, void *opts);

// default filesystems
#include "cfs.h"
// default filesystems-

#endif
