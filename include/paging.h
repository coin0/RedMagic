#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "isr.h"

#define PDE_PHY_ADDR 0x00000000
#define PDE_MAX_NUM  5

typedef struct page {
	_u32 present:1;		// Page present in memory
	_u32 rw:1;		// Read-only if clear, readwrite if set
	_u32 user:1;		// Supervisor level only if clear
	_u32 accessed:1;	// Has the page been accessed since last refresh?
	_u32 dirty:1;		// Has the page been written to since last refresh?
	_u32 unused:7;		// Amalgamation of unused and reserved bits
	_u32 frame:20;		// Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table {
	page_t pages[1024];
} page_table_t;

typedef struct page_directory {
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
} page_directory_t;

/**
  Handler for page faults.
**/
extern void page_fault_handler(registers_t regs);
extern void switch_page_directory(page_directory_t * dir);

#endif
