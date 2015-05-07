#ifndef IDE_H
#define IDE_H

#include "common.h"

struct buf {
	int flags;
	uint_t dev;
	uint_t sector;
	struct buf *prev;	// LRU cache list
	struct buf *next;
	struct buf *qnext;	// disk queue
	char data[512];
};

#define B_BUSY  0x1		// buffer is locked by some process
#define B_VALID 0x2		// buffer has been read from disk
#define B_DIRTY 0x4		// buffer needs to be written to disk

extern void ide_rw(struct buf *b);

#endif
