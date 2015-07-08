#ifndef FS_H
#define FS_H

#include "common.h"
#include "device.h"
#include "file.h"

// replaced with types of specific fs
typedef void super_block_t;

// superblock
typedef struct {
} super_ops_t;

typedef struct filesystem {
	const char *name;
	// mount operations on block device and return superblock structures
	super_block_t *(*mount) (const char *, int, blk_dev_t *, void *);
	// format block device with specific filesystem
	 ssize_t(*format) (const char *, blk_dev_t *, void *);
	struct filesystem *next;
} filesystem_t;

extern int register_filesystem(filesystem_t * fs);
extern int init_root_fs();

// default filesystems
#include "cfs.h"

#endif
