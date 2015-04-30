// common.h -- Defines typedefs and some global functions.
// From JamesM's kernel development tutorials.

#ifndef COMMON_H
#define COMMON_H

#define ARCH_X86_32		// build-macro : platform

#include "type.h"

void outb(_u16 port, _u8 value);
_u8 inb(_u16 port);
_u16 inw(_u16 port);

#endif
