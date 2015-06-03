#ifndef MM_H
#define MM_H

#include "common.h"
#include "multiboot.h"

// page calculation -
// PAGE_CONTAIN calculates how many pages needed
#define PAGE_SIZE 0x1000
#define PAGE_MASK (~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr) (((addr)+PAGE_SIZE - 1) & PAGE_MASK)
#define PAGE_CONTAIN(mem) ((mem)/PAGE_SIZE+((mem)%PAGE_SIZE?1:0))

// mm type
#define MM_PAGE_TABLE minit_pgtbl
#define MM_BUDDY      minit_buddy
#define INIT_MM       MM_PAGE_TABLE	// build-macro: select mm type

typedef struct {
	void *base;
	_u32 length;

#if INIT_MM == MM_PAGE_TABLE
	// PAGE_TABLE will adjust base and length as follows
	_u32 *meta_base;	// base addr of page table
	_u32 mframes;
	_u32 meta_len;		// length of meta data
	void *frame_base;	// real base addr of free frames
#endif

	_u32 nframes;		// frames managed
	_u32 nfree;		// available free frames
} __attribute__ ((packed)) mmc_t;

extern int INIT_MM(mmc_t * mmcp, void *base, _u32 length);
extern void copy_mm_from(mmc_t * mmcp, void *src, void *dst);
void *alloc_frames(mmc_t * mmcp, _u32 npages);
void *alloc_frame(mmc_t * mmcp);
int free_frames(mmc_t * mmcp, void *mp);

#define ARDS_TYPE_AVAIL 1
#define ARDS_TYPE_RESV  2

// address range descriptor structure
// we can get the struct via BIOS INT 15
// called by chk_mem_addr_range in memory.s.
typedef struct {
	_u32 size;		//not exist in standard ARDS
	_u32 base_addr_low;
	_u32 base_addr_high;
	_u32 length_low;
	_u32 length_high;
	_u32 type;
} __attribute__ ((packed)) addr_range_t;

// this struct is used to save multiboot information before paging mechanism 
// is started, so we no longer need bootloader
#define MAX_ARDS_ENT 10
typedef struct {
	_u32 total_memory;
	_u32 avail_memory;
	_u32 mmap_length;
	addr_range_t mmap_entries[MAX_ARDS_ENT];
} __attribute__ ((packed)) mem_info_t;

// kenel position defined in link conf
extern _u8 k_start[];

extern void show_kernel_pos();
extern void show_ARDS_from_multiboot(multiboot_t * mbp);

// operations on mm_phys
extern mmc_t mm_phys;
extern void *get_free_pages(size_t npg);
extern void *get_free_page();
extern void *get_zeroed_page();
extern int free_pages(void *page);
extern int free_page(void *page);

// operations on high memory
extern void *get_free_pages_high(size_t npg);
extern void *get_free_page_high();
extern void *get_zeroed_page_high();
extern int free_pages_high(void *page);
extern int free_page_high(void *page);
extern addr_t get_high_mem_start();
extern inline size_t get_high_mem_len();

extern void get_mem_info_from_multiboot(multiboot_t * mbp, mem_info_t * minfo);

// PAGE_TABLE
#if INIT_MM == MM_PAGE_TABLE

#define META_ENT_SIZE     (sizeof(_u32))
#define META_ENT_NUM_P_PG (PAGE_SIZE/META_ENT_SIZE)

#define PGCOLOR_MASK 0xFF
#define PGCOLOR(x) ((x)&(PGCOLOR_MASK))
#define PGCOLOR_XOR(a, b) ((a)^(b))
#define PGCOLOR_SHIFT(a)  ((a)%3+1)
#define PGCOLOR_RESET(a)  (PAGE_MASK&(a))
#define PG_WHITE 0x00
#define PG_RED   0x01
#define PG_GREEN 0x10
#define PG_BLUE  0x11

#endif
// --

#endif
