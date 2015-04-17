#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "isr.h"
#include "mm.h"

// assum page dirs start from the second page of physical memory
// the first page is reserved for the NULL pointer
#define PGD_BASE         0x00001000
#define PGD_MAX          5
#define PGD_IDX_KERNEL   0

// get specific entry by virtual address from PageDir and PageTable
#define PDE_INDEX(addr) (((_u32)(addr)>>22) & 0x3FF)
#define PTE_INDEX(addr) (((_u32)(addr)>>12) & 0x3FF)

// virtual address definition for kernel space
#define K_SPACE_START	0x0	// equal to PGD_BASE for now
#define K_SPACE_END	0x1000000

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
	_u32 table_phy_addr[1024];
   /**
      The physical address of tablesPhysical. This comes into play
      when we get our kernel heap allocated and the directory
      may be in a different location in virtual memory.
   **/
	_u32 phy_addr;
} __attribute__ ((packed)) page_directory_t;

/**
  Handler for page faults.
**/
extern void switch_page_directory(page_directory_t * dir);

// map virtual address to physical address and unmap virtual address
extern int page_map(void *virt_addr, void *phys_addr, page_directory_t * pdir,
		    mmc_t * mp);
extern int page_unmap(void *virt_addr, page_directory_t * pdir);

#endif
