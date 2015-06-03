#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "interrupt.h"
#include "mm.h"

/*
P             V
  |ffffffff|  4G/32bit
  |        |
  |yyyyyyyy|  kheap end
  |        |
  |xxxxxxxx|  high memory start
D |        |
i |01000000|  K_SPACE_END, Physical mem map(mm_phys,kheap)
r |        |
e |00100000|  Kernel entry
c |        |
t |00001000|  Page Directory(mm_pgtbls)
m |        |
a |00000000|  K_SPACE_START
p |________|

*/

// assum page dirs start from the second page of physical memory
// the first page is reserved for the NULL pointer
#define PGD_BASE         0x00100000
#define PGD_MAX          5
#define PGD_IDX_KERNEL   0

// get specific entry by virtual address from PageDir and PageTable
#define PDE_INDEX(addr) (((_u32)(addr)>>22) & 0x3FF)
#define PTE_INDEX(addr) (((_u32)(addr)>>12) & 0x3FF)

// virtual address definition for kernel space
#define K_SPACE_START	0x0	// equal to PGD_BASE for now
#define K_SPACE_END	0x1000000

// high memory
// high memory start depends on actual physical memory length
#define K_HMEM_END      0xC0000000

// page entry masks
#define PAGE_PRESENT    0x1
#define PAGE_WRITE      0x2
#define PAGE_USER       0x4

/* define page entry as _u32
   detailed members intro of page entry
typedef struct page {
	_u32 present:1;		// Page present in memory
	_u32 rw:1;		// Read-only if clear, readwrite if set
	_u32 user:1;		// Supervisor level only if clear
	_u32 accessed:1;	// Has the page been accessed since last refresh?
	_u32 dirty:1;		// Has the page been written to since last refresh?
	_u32 unused:7;		// Amalgamation of unused and reserved bits
	_u32 frame:20;		// Frame address (shifted right 12 bits)
} page_t;
*/
typedef _u32 page_entry_t;

typedef struct {
	page_entry_t pages[1024];
} __attribute__ ((packed)) page_table_t;

typedef struct {
   /**
      Array of pointers to pagetables.
   **/
	page_table_t *tables[1024];
   /**
      Array of pointers to the pagetables above, but gives their *physical*
      location, for loading into the CR3 register.
   **/
	//_u32 table_phy_addr[1024];
   /**
      The physical address of tablesPhysical. This comes into play
      when we get our kernel heap allocated and the directory
      may be in a different location in virtual memory.
   **/
	//_u32 phy_addr;
} __attribute__ ((packed)) page_directory_t;

extern page_directory_t *k_pdir;
extern void init_paging();

extern void switch_page_directory(page_directory_t * dir);
extern void copy_page_directory_from(void *src, void *dst);

// map virtual address to physical address and unmap virtual address
extern int page_map(void *virt_addr, void *phys_addr, page_directory_t * pdir,
		    mmc_t * mp);
extern int page_unmap(void *virt_addr, page_directory_t * pdir);

#define __virt_to_phys(pdir, va) ({ 	\
	_u32 __addr = (_u32)(pdir)->tables[PDE_INDEX(va)] & PAGE_MASK;      \
	__addr = (_u32)((page_table_t *)__addr)->pages[PTE_INDEX(va)] & PAGE_MASK;\
	__addr | ((va) & (~PAGE_MASK)); })

// notice: phys_to_virt is not unique, for now, address (<high memory) is
//         direct mapping
#define __phys_to_virt_lm(pdir, pa)	(pa)

#endif
