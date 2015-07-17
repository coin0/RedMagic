// common.h -- Defines typedefs and some global functions.
// From JamesM's kernel development tutorials.

#ifndef COMMON_H
#define COMMON_H

#define ARCH_X86_32		// build-macro : platform

#include "type.h"
#include "math.h"

#ifdef ARCH_X86_32
#include "x86.h"
#endif

#endif
