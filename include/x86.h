#ifndef X86_H
#define X86_H

#include "type.h"

// write sequential memory bytes to the specified port
static inline void outsl(int port, const void *addr, int cnt)
{
	asm volatile ("cld; rep outsl":
		      "=S" (addr), "=c"(cnt):
		      "d"(port), "0"(addr), "1"(cnt):"cc");
}

static inline void insl(int port, void *addr, int cnt)
{
	asm volatile ("cld; rep insl":
		      "=D" (addr), "=c"(cnt):
		      "d"(port), "0"(addr), "1"(cnt):"memory", "cc");
}

// Write a byte out to the specified port.
static inline void outb(_u16 port, _u8 value)
{
	asm volatile ("outb %1, %0"::"dN" (port), "a"(value));
}

static inline _u8 inb(_u16 port)
{
	_u8 ret;
	asm volatile ("inb %1, %0":"=a" (ret):"dN"(port));
	return ret;
}

static inline _u16 inw(_u16 port)
{
	_u16 ret;
	asm volatile ("inw %1, %0":"=a" (ret):"dN"(port));
	return ret;
}

#endif
