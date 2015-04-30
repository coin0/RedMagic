#ifndef HEAP_H
#define HEAP_H

#include "common.h"
#include "mm.h"
#include "paging.h"
#include "list.h"

// define kheap size
#define K_HEAP_SIZE   0x100000

// heap algorithms
#define HEAP_FB   fb_heap_init
#define HEAP_FF   ff_heap_init
#define HEAP_SLAB slab_heap_init
#define INIT_HEAP HEAP_FB	// build-macro

// features
#if INIT_HEAP == HEAP_FB || INIT_HEAP == HEAP_FF

#define HBLK_NUM_P_PG (PAGE_SIZE/sizeof(heap_block_t))

typedef struct {
	addr_t base;
	size_t size;
	list_head_t lh;
} __attribute__ ((packed)) heap_block_t;

#elif INIT_HEAP == HEAP_SLAB

#endif // end of features

typedef struct {
	const char *heap_name;
	mmc_t *mmcp;
	addr_t start;
	addr_t end;

#if INIT_HEAP == HEAP_FB || INIT_HEAP == HEAP_FF
	heap_block_t *free;
	heap_block_t *res;
	heap_block_t *avail;
#endif

} __attribute__ ((packed)) heap_desc_t;

// init heap
int init_heap(heap_desc_t * heap, mmc_t * mp, const char *name, size_t size);

// init kernel heap
void init_kheap();
void *kmalloc(size_t size);
int kfree(void *p);
//--
#endif
