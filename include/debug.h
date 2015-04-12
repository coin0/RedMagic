#ifndef DEBUG_H
#define DEBUG_H

#include "common.h"
#include "elf.h"

// Debug switch (debug-on by default)
#define MODE_DBG		// build-macro: debug mode
#define MODE_TRC		// build-macro: trace mode

extern void init_debug(multiboot_t * mboot_ptr);
extern void print_stack_trace();
extern void print_cur_status();
extern void print_memory(void *bp, _u32 lines);

void panic(const char *msg, const char *file, _u32 line);

#define PANIC(msg) panic(msg, __FILE__, __LINE__);

#define ASSERT(x)                                 \
           	if (!(x)) {                                     \
                        PANIC("ASSERTION-FAILED: "#x);           \
                }

#endif
