#ifndef FS_H
#define FS_H

#include "common.h"
#include "device.h"
#include "file.h"
#include "keyval.h"
#include "list.h"

// replaced with types of specific fs
typedef void super_block_t;

// superblock
typedef struct {
	inode_t *(*alloc_inode) (super_block_t *);
	int (*free_inode) (inode_t *);
	int (*write_inode) (inode_t *);
	// originally get_inode and put_inode are used in pair, get_inode can
	// be regarded as read_inode but put_inode is not write but release
	inode_t *(*get_inode) (uint_t);
	int (*put_inode) (inode_t *);
	int (*sync_fs) (super_block_t * sb);
} super_ops_t;

typedef struct filesystem {
	const char *name;
	// mount operations on block device and return superblock structures
	super_block_t *(*mount) (blk_dev_t *, kv_option_t *);
	int (*umount) (super_block_t * sb, kv_option_t *);
	// format block device with specific filesystem
	int (*format) (blk_dev_t *, kv_option_t *);
	struct filesystem *next;
} filesystem_t;

typedef struct {
	filesystem_t *file_systems;
	mutex_t lock;
} fs_list_t;

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

extern int register_filesystem(filesystem_t * fs);
extern void init_root_fs();

// high-level filesystem interfaces
extern int fs_format(blk_dev_t * bdev, filesystem_t * fs, kv_option_t * opt);
extern int fs_mount(blk_dev_t * bdev, filesystem_t * fs, char *dir_name,
		    kv_option_t * opt);
extern int fs_umount(char *dir_name, kv_option_t * opt);

// mount utilities
extern super_block_t *mnt_lookup_super_by_bdev(blk_dev_t * bdev);

// default filesystems
#include "cfs.h"
// default filesystems-

#endif
