/*
 *  heaps
 */

#include "common.h"
#include "mm.h"
#include "paging.h"
#include "print.h"
#include "heap.h"
#include "list.h"
#include "debug.h"
#include "string.h"
#include "klog.h"

static void *__kmalloc(size_t size, heap_desc_t * heap);
static int __kfree(void *p, heap_desc_t * heap);
static int INIT_HEAP(heap_desc_t * heap);
static void show_heap(heap_desc_t * heap);

// global kernel heap descriptor
static heap_desc_t kheap;

int init_heap(heap_desc_t * heap, const char *name, size_t size)
{
	size_t pages = PAGE_CONTAIN(size);
	void *start = get_free_pages_high(pages);

	if (start == NULL)
		return 1;

	heap->heap_name = name;
	heap->start = (addr_t) start;
	heap->end = heap->start + size;

	return INIT_HEAP(heap);
}

void init_kheap()
{
	if (init_heap(&kheap, "kernel_heap", K_HEAP_SIZE))
		PANIC("Kernel heap not set up");
}

void *kmalloc(size_t size)
{
	// make sure kernel heap has been initialized
	ASSERT(kheap.start && kheap.end);
	return __kmalloc(size, &kheap);
}

int kfree(void *p)
{
	ASSERT(kheap.start && kheap.end);
	return __kfree(p, &kheap);
}

#if INIT_HEAP == HEAP_FB

// first-best allocation
static int HEAP_FB(heap_desc_t * heap)
{
	heap_block_t *p, *metap;

	// get a free page and initialize free list and reserved list
	metap = (heap_block_t *) get_zeroed_page_high();

	if (!metap)
		return 1;

	p = metap;
	heap->free = p++;
	heap->res = p++;
	heap->avail = p++;

	INIT_LIST_HEAD(&(heap->free->lh));
	INIT_LIST_HEAD(&(heap->res->lh));
	INIT_LIST_HEAD(&(heap->avail->lh));

	log_info("init heap: %s ...\n start = 0x%08X, end = 0x%08X\n",
		 heap->heap_name, heap->start, heap->end);
	log_info(" head = 0x%08X\n", (addr_t) metap);

	p->base = heap->start;
	p->size = heap->end - heap->start;
	list_add(&p->lh, &(heap->free->lh));
	p++;

	// initialize 'avail' list
	for (; (addr_t) p < (addr_t) metap + PAGE_SIZE; p++)
		list_add_tail(&p->lh, &(heap->avail->lh));

	return 0;
}

static void *__kmalloc(size_t size, heap_desc_t * heap)
{
	heap_block_t *hbp, *found = NULL;
	heap_block_t *metap, *p;

	heap_block_t *hd_free = heap->free;
	heap_block_t *hd_res = heap->res;
	heap_block_t *hd_avail = heap->avail;

	size_t delta = ~0;

	list_for_each_entry(hbp, &hd_free->lh, lh) {
		// look for the best fit by delta
		if (hbp->size > size) {
			if (hbp->size - size < delta) {
				delta = hbp->size - size;
				found = hbp;
			}
		} else if (hbp->size == size) {
			// gotcha
			list_move(&hbp->lh, &hd_res->lh);
			return (void *)(hbp->base);
		}
	}

	if (!found) {
		log_err("no more blocks of size %d bytes for heap %s\n", size,
			heap->heap_name);
		return NULL;
	}
	// add new entry for 'res' list
	hbp = list_first_entry_or_null(&hd_avail->lh, heap_block_t, lh);
	if (hbp == NULL) {
		// well, we do not have space for new entries, allocate
		// another page.
		metap = (heap_block_t *) get_zeroed_page_high();
		if (!metap)
			return NULL;

		// initialize 'avail' list
		for (p = metap; (addr_t) p < (addr_t) metap + PAGE_SIZE; p++)
			list_add_tail(&p->lh, &hd_avail->lh);

		// retry now
		hbp = list_first_entry_or_null(&hd_avail->lh, heap_block_t, lh);
		if (hbp == NULL)
			return NULL;
	}
	// init new entry and add this entry to 'res' list
	hbp->size = size;
	hbp->base = found->base;
	found->size -= size;
	found->base += size;
	list_move(&hbp->lh, &hd_res->lh);

	printk("\n<__kmalloc: 0x%08X> \n", size);
	show_heap(heap);

	return (void *)(hbp->base);
}

static int __kfree(void *p, heap_desc_t * heap)
{
	heap_block_t *hbp, *found = NULL;
	heap_block_t *lp = NULL, *rp = NULL;

	heap_block_t *hd_free = heap->free;
	heap_block_t *hd_res = heap->res;
	heap_block_t *hd_avail = heap->avail;

	list_for_each_entry(hbp, &hd_res->lh, lh) {
		if (hbp->base == (addr_t) p) {
			found = hbp;
			list_move(&found->lh, &hd_free->lh);
			break;
		}
	}

	if (!found)
		return 1;

	// merge released block into free list
	// handle special cases by IF statements
	list_for_each_entry(hbp, &hd_free->lh, lh) {
		if (hbp->base + hbp->size == found->base) {
			// [free][found] -> [free+found]
			found->base = hbp->base;
			found->size += hbp->size;
			lp = hbp;
		} else if (hbp->base == found->base + found->size) {
			// [found][free] -> [found+free]
			found->size += hbp->size;
			rp = hbp;
		}
		if (lp && rp)
			break;
	}

	if (lp)
		list_move(&lp->lh, &hd_avail->lh);
	if (rp)
		list_move(&rp->lh, &hd_avail->lh);

	// TODO check if page-release is needed

	printk("\n<__kfree: 0x%08X> \n", (addr_t) p);
	show_heap(heap);

	return 0;
}

static void show_heap(heap_desc_t * heap)
{
	heap_block_t *hbp;

	printk("\nfree_list: ");
	list_for_each_entry(hbp, &(heap->free->lh), lh) {
		printk("[0x%08X]0x%08X(0x%08X) ->", hbp, hbp->base, hbp->size);
	}
	printk("|\nres_list: ");
	list_for_each_entry(hbp, &(heap->res->lh), lh) {
		printk("[0x%08X]0x%08X(0x%08X) ->", hbp, hbp->base, hbp->size);
	}
	printk("|\n");
}

#elif INIT_HEAP == HEAP_FF

// first-fit allocation
static int HEAP_FF(heap_desc_t * heap)
{

}

#endif
