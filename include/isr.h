//
// isr.h -- Interface and structures for high level interrupt service routines.
// Part of this code is modified from Bran's kernel development tutorials.
// Rewritten for JamesM's kernel development tutorials.
//

#ifndef ISR_H
#define ISR_H

#ifndef INTERRUPT_H
#error "Include interrupt.h instead"
#endif

#include "common.h"

typedef struct registers {
	_u32 ds;		// Data segment selector
	_u32 edi;
	_u32 esi;
	_u32 ebp;
	_u32 esp;
	_u32 ebx;
	_u32 edx;
	_u32 ecx;
	_u32 eax;		// Pushed by pusha.
	_u32 int_no;
	_u32 err_code;		// Interrupt number and error code (if applicable)
	_u32 eip;
	_u32 cs;
	_u32 eflags;
	_u32 useresp;
	_u32 ss;		// Pushed by the processor automatically.
} registers_t;

// A few defines to make life a little easier
#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

// easy to understand and include some special IRQ verctors
#define IRQ_TIMER       IRQ0
#define IRQ_KBD         IRQ1
#define IRQ_COM1        IRQ4
#define IRQ_IDE         IRQ14
#define IRQ_ERROR       (IRQ0 + 19)
#define IRQ_SPURIOUS    (IRQ0 + 31)

// Enables registration of callbacks for interrupts or IRQs.
// For IRQs, to ease confusion, use the #defines above as the
// first parameter.
typedef void (*isr_t) (registers_t *);
isr_t register_interrupt_handler(_u8 n, isr_t handler);

#endif
