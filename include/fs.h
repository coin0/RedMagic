#ifndef FS_H
#define FS_H

#include "common.h"
#include "device.h"
#include "file.h"
#include "keyval.h"
#include "list.h"

// replaced with types of specific fs
typedef void super_block_t;

typedef struct filesystem {
	const char *name;

	// mount operations on block device and return superblock structures
	super_block_t *(*mount) (blk_dev_t *, kv_option_t *);
	int (*umount) (super_block_t * sb, kv_option_t *);

	// format block device with specific filesystem
	int (*format) (blk_dev_t *, kv_option_t *);
	int (*read_file) (char *path, uint_t offset, uint_t bytes,
			  char *buffer);
	int (*write_file) (char *path, uint_t offset, uint_t bytes,
			   char *buffer);
	int (*move_file) (char *src, char *dest);
	int (*copy_file) (char *src, char *dest);

	struct filesystem *next;
	// in-memory file buffer
	file_list_t *file_list;
} filesystem_t;

typedef struct {
	filesystem_t *file_systems;
	mutex_t lock;
} fs_list_t;

extern filesystem_t *fs_list();
extern int register_filesystem(filesystem_t * fs);
extern void init_root_fs();

// high-level filesystem interfaces
extern int fs_format(blk_dev_t * bdev, filesystem_t * fs, kv_option_t * opt);
extern int fs_mount(blk_dev_t * bdev, filesystem_t * fs, char *dir_name,
		    kv_option_t * opt);
extern int fs_umount(char *dir_name, kv_option_t * opt);

// default filesystems
#include "cfs.h"
// default filesystems-

#endif
