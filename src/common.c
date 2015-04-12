// common.c -- Defines some global functions.
// From JamesM's kernel development tutorials.

#include "common.h"

// Write a byte out to the specified port.
void outb(_u16 port, _u8 value)
{
	asm volatile ("outb %1, %0"::"dN" (port), "a"(value));
}

_u8 inb(_u16 port)
{
	_u8 ret;
	asm volatile ("inb %1, %0":"=a" (ret):"dN"(port));
	return ret;
}

_u16 inw(_u16 port)
{
	_u16 ret;
	asm volatile ("inw %1, %0":"=a" (ret):"dN"(port));
	return ret;
}
