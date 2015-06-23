#ifndef BBUF_H
#define BBUF_H

#ifndef DEVICE_H
#error "include device.h instead"
#endif

#include "common.h"

/*
 *   buffer IO
 */

#define BLOCK_SIZE 512

#define B_BUSY   0x1		// buffer is locked by some process
#define B_VALID  0x2		// buffer has been read from disk
#define B_DIRTY  0x4		// buffer needs to be written to disk
#define B_UNUSED 0x8		// newly initialized buffer

typedef struct buf_cache {
	struct blk_dev *bdev;
	uint_t blkno;
	uint_t flags;
	list_head_t list;
	list_head_t io;
	list_head_t wq;
	char data[BLOCK_SIZE];
} buf_cache_t;

#endif
