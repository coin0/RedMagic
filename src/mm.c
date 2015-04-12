#include "mm.h"
#include "print.h"
#include "debug.h"
#include "string.h"
#include "paging.h"
#include "boot.h"

void show_kernel_pos()
{
	printk("kernel start @ 0x%08X\n", k_start);
	printk("kernel end   @ 0x%08X\n", k_end);
	printk("memory used: %d KB\n\n", (k_end - k_start + 1023) / 1024);
}

void show_ARDS_from_multiboot(multiboot_t * mbp)
{
	_u32 mmap_addr = mbp->mmap_addr;
	_u32 mmap_length = mbp->mmap_length;

	_u32 mem_size = 0;
	_u32 mem_avail = 0;

	addr_range_t *mmap = (addr_range_t *) mmap_addr;

	printk("Memory map:\n");
	for (; (_u32) mmap < mmap_addr + mmap_length; mmap++) {
		printk("base_addr = 0x%X%08X, length = 0x%X%08X, type = 0x%X\n",
		       (_u32) mmap->base_addr_high, (_u32) mmap->base_addr_low,
		       (_u32) mmap->length_high, (_u32) mmap->length_low,
		       (_u32) mmap->type);

		if ((_u32) mmap->type == ARDS_TYPE_AVAIL)
			mem_avail += (_u32) mmap->length_low;
		mem_size += (_u32) mmap->length_low;
	}

	printk("\ntotal main memory size : %d KB\n", mem_size / 1024);
	printk("available memory size  : %d KB\n\n", mem_avail / 1024);
}

void get_mem_info_from_multiboot(multiboot_t * mbp, mem_info_t * minfo)
{
	_u32 mmap_addr = mbp->mmap_addr;
	_u32 mmap_length = mbp->mmap_length;

	_u32 mem_total;
	_u32 mem_avail;

	addr_range_t *mmap = (addr_range_t *) mmap_addr;
	addr_range_t *p = minfo->mmap_entries;

	// too many ARDSs? just increase MAX_ARDS_ENT in mm.h!
	ASSERT(MAX_ARDS_ENT < mmap_length);
	minfo->mmap_length = mmap_length;

	// calculate how many pages available
	for (; (_u32) mmap < mmap_addr + mmap_length; mmap++, p++) {
		// copy contents from multiboot
		p->size = (_u32) mmap->size;
		p->base_addr_high = (_u32) mmap->base_addr_high;
		p->base_addr_low = (_u32) mmap->base_addr_low;
		p->length_high = (_u32) mmap->length_high;
		p->length_low = (_u32) mmap->length_low;
		p->type = (_u32) mmap->type;

		mem_total += (_u32) mmap->length_low;
		if ((_u32) mmap->type == ARDS_TYPE_AVAIL)
			mem_avail += (_u32) mmap->length_low;
	}

	minfo->total_memory = mem_total;
	minfo->avail_memory = mem_avail;
}

page_directory_t k_pgdir;

void init_paging()
{
	_u32 *base, pages, length;
	mem_info_t minfo;
	addr_range_t *p;
	mmc_t kmm;

	// get total memory & available memory
	bzero((void *)&minfo, sizeof(mem_info_t));
	get_mem_info_from_multiboot(mbootp, &minfo);

	// how many pages actually needed to map whole memory
	pages = PAGE_CONTAIN(minfo.total_memory);

	//register_interrupt_handler(14, page_fault_handler);

	//switch_page_directory(&k_pgdir);
	while (1) ;

	// map unavailable memory to the end of virtual address space
	p = minfo.mmap_entries;

	for (; (_u32) p < (_u32) minfo.mmap_entries + minfo.mmap_length; p++) {
		if (p->type != ARDS_TYPE_AVAIL) {

		}
	}

//coin front

	//base = (void *)0x11000;
	base = (void *)0;
	length = PAGE_SIZE * 50;

	// initialize memory
	if (INIT_MM(&kmm, base, length))
		PANIC("Memory initialization failed");

	//void *a = alloc_frames(&mm, 5);
	//void *d = alloc_frames(&mm, 1);
	//void *b = alloc_frames(&mm, 1);
}

void page_fault_handler(registers_t regs)
{
	printk("\nIn specific handler !\n");
}

void switch_page_directory(page_directory_t * dir)
{
	asm volatile ("mov %0, %%cr3"::"r" (&dir->table_phy_addr));
	_u32 cr0;
	asm volatile ("mov %%cr0, %0":"=r" (cr0));
	cr0 |= 0x80000000;	// Enable paging!
	asm volatile ("mov %0, %%cr0"::"r" (cr0));
}

#if INIT_MM == MM_PAGE_TABLE

int MM_PAGE_TABLE(mmc_t * mmcp, void *base, _u32 length)
{
	_u32 *meta, *metap;	// fixed & floating ptr to meta data 
	void *basep, *endp;	// ptr for base and end(base+len) of free area
	_u32 mpg;		// number of pages to put meta
	_u32 npg, pages;
	_u32 meta_len;

	printk("page_size = %d bytes\n", PAGE_SIZE);
	printk("setup memory layout in type PAGE_TABLE\n");

	// do not use space less/more than a page
	bzero(base, length);
	basep = (void *)PAGE_ALIGN((_u32) base);
	endp = (void *)(((_u32) base + length) & PAGE_MASK);
	meta = basep;
	metap = meta;

	// calculate actual pages that will be available for
	// meta data and free frames, at least 2 pages are needed. 
	// ratio of pages needed between meta:frames=1:1024.
	pages = ((_u32) endp - (_u32) basep) / PAGE_SIZE;
	if (pages < 2)
		return 1;
	npg = pages * META_ENT_NUM_P_PG / (META_ENT_NUM_P_PG + 1);
	mpg = pages - npg;
	meta_len = npg * META_ENT_SIZE;
	ASSERT(mpg * META_ENT_NUM_P_PG >= npg);
	basep = (void *)((_u32) basep + mpg * PAGE_SIZE);

	// refresh mmc
	mmcp->base = base;
	mmcp->length = length;
	mmcp->meta_base = meta;
	mmcp->mframes = mpg;
	mmcp->meta_len = meta_len;
	mmcp->frame_base = basep;
	mmcp->nframes = npg;
	mmcp->nfree = npg;

	// initialize page table with PG_WHITE entry and address of
	// free frames accordingly 
	printk("table @ 0x%08X, free memory @ 0x%08X\n", (_u32) metap,
	       (_u32) basep);
	printk("%d frame(s), memory size %d KB\n", npg, PAGE_SIZE * npg / 1024);
	for (; (_u32) metap < (_u32) meta + meta_len; metap++) {
		*metap = (_u32) basep | PG_WHITE;
		basep = (_u32 *) ((_u8 *) basep + PAGE_SIZE);
	}
	print_memory((void *)meta, 5);

	return 0;
}

static _u32 *find_seq_pages(mmc_t * mmcp, _u32 npages)
{
	_u32 *cp = mmcp->meta_base;
	_u32 *pp = cp;
	_u32 seq_pages = 0;

	if (npages <= mmcp->nfree) {
		while ((_u32) cp < (_u32) (mmcp->meta_base) + mmcp->meta_len) {
			if (((*cp) & PGCOLOR_MASK) == PG_WHITE) {
				if (!seq_pages)
					pp = cp;
				seq_pages++;
				if (seq_pages == npages)
					return pp;
			} else {
				seq_pages = 0;
			}
			cp++;
		}
	}
	return (_u32 *) NULL;
}

static _u32 *find_page_index(mmc_t * mmcp, void *mp)
{
	_u32 page = PAGE_MASK & ((_u32) mp);
	_u32 *p = mmcp->meta_base;

	while ((_u32) p < (_u32) (mmcp->meta_base) + mmcp->meta_len) {
		if ((PAGE_MASK & (*p)) == page)
			return p;
		p++;
	}
	return (_u32 *) NULL;
}

void *alloc_frames(mmc_t * mmcp, _u32 npages)
{
	_u32 *tp = find_seq_pages(mmcp, npages);
	_u32 left, right, marker, n;

	if (!tp)
		PANIC("Out of memory");

	// just put Red 
	// |<R,R,R<|
	// do shifting in following cases
	// |<G,B,R<R,G,B,(W)
	// R,G,B,(W)<G,B,R<|
	// R,G,B<G,B,R<R,G,B,(W)
	// do XOR in following cases
	// R,G,B<B,R,G<G,B,R
	// R,G,B<G,B,R<B,R,G
	if (tp == mmcp->meta_base
	    && tp + npages == mmcp->meta_base + mmcp->nframes) {
		marker = PG_RED;
	} else if (tp == mmcp->meta_base) {
		marker = PGCOLOR_SHIFT(PGCOLOR(*(tp + npages)));
	} else if (tp + npages == mmcp->meta_base + mmcp->nframes) {
		marker = PGCOLOR_SHIFT(PGCOLOR(*(tp - 1)));
	} else {
		right = PGCOLOR(*(tp + npages));
		left = PGCOLOR(*(tp - 1));

		// left is PG_WHITE...not suppose to happen
		ASSERT(left != PG_WHITE);

		if (right == left || right == PG_WHITE)
			marker = PGCOLOR_SHIFT(left);
		else
			marker = PGCOLOR_XOR(left, right);
	}

	// now draw the color on the pages 
	for (n = 0; n < npages; n++) {
		ASSERT(PGCOLOR(*(tp + n)) == PG_WHITE);
		*(tp + n) |= marker;
	}

	mmcp->nfree -= npages;

	printk("alloc %d pg, free %d pg\n", npages, mmcp->nfree);
	print_memory((void *)mmcp->meta_base, 2);
	return (void *)(PAGE_MASK & (*tp));
}

int free_frames(mmc_t * mmcp, void *mp)
{
	_u32 *tp = find_page_index(mmcp, mp);
	_u32 *left, *right, *edge;
	_u32 marker = PGCOLOR(*tp);

	if (!tp)
		return 1;
	if (marker == PG_WHITE)
		return 1;

	edge = tp;
	for (left = tp - 1; left >= mmcp->meta_base; left--) {
		if (PGCOLOR(*left) != marker)
			break;
		edge = left;
	}
	left = edge;

	edge = tp;
	for (right = tp; right < mmcp->meta_base + mmcp->nframes; right++) {
		if (PGCOLOR(*right) != marker)
			break;
		edge = right;
	}
	right = edge;

	mmcp->nfree += right - left + 1;

	while (left <= right) {
		*left = PGCOLOR_RESET(*left);
		left++;
	}

	printk("free 0x%08X, free %d pg\n", mp, mmcp->nfree);
	print_memory((void *)mmcp->meta_base, 2);
	return 0;
}

#elif INIT_MM == MM_BUDDY

#endif
