#ifndef DEVICE_H
#define DEVICE_H

#include "common.h"
#include "list.h"

#define MAX_DEVICES 32

typedef enum {
	DEV_BLOCK,

	// insert above
	DEV_LAST
} dev_type_t;

typedef struct {
	char *name;
	dev_type_t type;
	int (*init_func) (void);
	void *ptr;
	int onboot;
	int enabled;
} dev_t;

extern void init_dev();
extern dev_t *get_dev_by_name(const char *name);
/* 
 *   block device 
 */

#define BLOCK_SIZE 512

typedef struct buf_cache {
	dev_t dev;
	uint_t blkno;
	uint_t flag;
	list_head_t list;
	list_head_t io;
	uchar_t blk[BLOCK_SIZE];
} buf_cache_t;

typedef struct {
	int (*read_block) (buf_cache_t *, uint_t);
	int (*write_block) (buf_cache_t *);
} blk_dev_ops_t;

typedef struct {
	uint_t cache_num;
	size_t block_size;
	list_head_t list;
	list_head_t io;
	blk_dev_ops_t ops;
} blk_dev_t;

#define B_BUSY  0x1		// buffer is locked by some process
#define B_VALID 0x2		// buffer has been read from disk
#define B_DIRTY 0x4		// buffer needs to be written to disk

#include "ramfs.h"

#endif
