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

// spaces for page tables are allocated when needed, good news is each page
// table is equivalant to a page in size, use mmc to manage spaces consumption
static mmc_t mm_pgtbls;
static mem_info_t minfo;
static void page_fault_handler(registers_t * regs);

void init_paging()
{
	_u32 i, pgtbls_end = 0;
	page_directory_t *pdp;
	page_table_t *ptp;
	addr_range_t *mp;
	void *p;

	// scan initial memory address space from 0x00
	bzero((void *)&minfo, sizeof(mem_info_t));
	get_mem_info_from_multiboot(mbootp, &minfo);
	mp = minfo.mmap_entries;
	for (; (_u32) mp < (_u32) minfo.mmap_entries + minfo.mmap_length; mp++) {
		if (mp->base_addr_low == 0x0 && mp->base_addr_high == 0x0) {
			if (mp->type != ARDS_TYPE_AVAIL || mp->length_low == 0)
				PANIC("Low address(0x0) not available.");
			pgtbls_end = mp->length_low;
			break;
		}
	}

	// stop! where is the ARDS entry starting from 0x0?
	ASSERT(pgtbls_end > 0);

	// stop! space not enough, we need PGD_MAX page_directory_t entries 
	// and at least 5 free frames)
	ASSERT(PGD_BASE + sizeof(page_directory_t) * PGD_MAX + PAGE_SIZE * 5 <=
	       0x0 + pgtbls_end);

	// initialize page directory tables
	pdp = (page_directory_t *) PGD_BASE;
	for (i = 0; i < PGD_MAX; i++) {
		bzero((void *)pdp, sizeof(page_directory_t));
	}

	// initialize page tables areas
	if (INIT_MM(&mm_pgtbls, (void *)&pdp[i], pgtbls_end - 1))
		PANIC("MM_INIT failed");

	// begin to initialize paging for kernel space
	// we keep identical map (phys addr = virt addr) from
	// K_SPACE_START -> K_SPACE_END
	p = (void *)(K_SPACE_START & PAGE_MASK);
	while ((_u32) p < PAGE_ALIGN(K_SPACE_END)) {
		ptp = (page_table_t *) alloc_frames(&mm_pgtbls, 1);
		if (ptp == NULL)
			PANIC("No free frames");

		// init PDE
		pdp[PGD_IDX_KERNEL].tables[PDE_INDEX(p)] =
		    (page_table_t *) ((_u32) ptp | PAGE_PRESENT | PAGE_WRITE |
				      PAGE_USER);

		// init PTE : 1024 pages per PT
		bzero((void *)ptp, sizeof(page_table_t));
		for (i = ((_u32) p >> 12) % 1024; i < 1024; i++) {
			ptp->pages[PTE_INDEX(p)] =
			    (page_entry_t) ((_u32) p | PAGE_PRESENT |
					    PAGE_WRITE | PAGE_USER);

			// next page entry
			p = (void *)((_u32) p + PAGE_SIZE);
		}
	}

	// register page fault handler and enable paging
	register_interrupt_handler(14, &page_fault_handler);
	switch_page_directory(&pdp[PGD_IDX_KERNEL]);
}

static void page_fault_handler(registers_t * regs)
{
	_u32 cr2;
	char msg[64];

	asm volatile ("mov %%cr2, %0":"=r" (cr2));
	sprintf(msg, "*** PAGE FAULT @0x%08X, e=0x%08X", cr2, regs->err_code);
	printk("\n%s\n", msg);

	// stop here
	while (1) ;
}

void switch_page_directory(page_directory_t * dir)
{
	_u32 cr0;

	asm volatile ("mov %0, %%cr3"::"r" (&dir->tables));

	// enable paging
	asm volatile ("mov %%cr0, %0":"=r" (cr0));
	cr0 |= 0x80000000;
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
		return NULL;

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
