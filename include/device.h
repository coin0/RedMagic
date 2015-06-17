#ifndef DEVICE_H
#define DEVICE_H

#include "common.h"
#include "list.h"
#include "locking.h"

#define MAX_DEVICES 32

typedef enum {
	DEV_BLOCK,

	// insert above
	DEV_LAST
} dev_type_t;

typedef struct dev {
	char *name;
	dev_type_t type;
	int (*init_func) (struct dev *);
	void *ptr;
	int onboot;
	int enabled;
} dev_t;

extern void init_dev();
extern dev_t *get_dev_by_name(const char *name);

/* 
 *   block device 
 */

#include "bbuf.h"

typedef struct {
	int (*read_block) (buf_cache_t *, uint_t);
	int (*write_block) (buf_cache_t *);
} blk_dev_ops_t;

typedef struct blk_dev {
	uint_t buf_num;
	size_t block_size;
	uint_t total_blks;
	list_head_t list;
	list_head_t io;
	blk_dev_ops_t *ops;
	mutex_t lock;
} blk_dev_t;

// functions
extern int bdev_init_buffer_cache(blk_dev_t * bdev, size_t blks);
extern size_t bdev_read_buffer(blk_dev_t * bdev, uint_t blkno, uchar_t * data);
extern size_t bdev_write_buffer(blk_dev_t * bdev, uint_t blkno, uchar_t * data);

// default devices
#include "ramfs.h"
#include "ide.h"

#endif
