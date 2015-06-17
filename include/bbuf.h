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

typedef struct buf_cache {
	struct blk_dev *bdev;
	uint_t blkno;
	uint_t flags;
	list_head_t list;
	list_head_t io;
	list_head_t wq;
	uchar_t data[BLOCK_SIZE];
} buf_cache_t;

#endif
