#ifndef MOUNT_H
#define MOUNT_H

#include "common.h"
#include "fs.h"

// mountpoint flags
#define MNT_READONLY 0x01

typedef struct mount_point {
	blk_dev_t *mnt_dev;
	char *mnt_dir;
	filesystem_t *fs_type;
	super_block_t *mnt_sb;
	list_head_t mnt_next;
	uint_t mnt_flags;
} mount_point_t;

typedef struct {
	list_head_t mnt_next;
	mutex_t mtab_lock;
} mount_table_t;

// mount utilities
extern void init_mtab();
extern void mtab_lock_read();
extern void mtab_lock_write();
extern void mtab_unlock_read();
extern void mtab_unlock_write();
extern void add_to_mtab(mount_point_t * mpp);
extern void del_from_mtab(mount_point_t * mpp);
extern int check_dup_mountpoint(blk_dev_t * bdev, char *dir_name);
extern mount_point_t *lookup_mountpoint_by_dir(char *dir_name);
extern super_block_t *lookup_super_by_bdev(blk_dev_t * bdev);

#endif
