/*
 *      Red Magic 1996 - 2015
 *
 *      idt.c - setup IDT entries for the platform of X86
 *
 *      2015 Lin Coin - initial version
 */

#include "common.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "cpu.h"

static void init_idt();
static void init_cpu_isr();
static void idt_set_gate(_u8, _u32, _u16, _u8);

idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr;

isr_t interrupt_handlers[256];

void init_interrupt_descriptor_table()
{
	init_idt();
}

static void init_cpu_isr()
{
	idt_set_gate(0, (_u32) isr0, 0x08, 0x8E);
	idt_set_gate(1, (_u32) isr1, 0x08, 0x8E);
	idt_set_gate(2, (_u32) isr2, 0x08, 0x8E);
	idt_set_gate(3, (_u32) isr3, 0x08, 0x8E);
	idt_set_gate(4, (_u32) isr4, 0x08, 0x8E);
	idt_set_gate(5, (_u32) isr5, 0x08, 0x8E);
	idt_set_gate(6, (_u32) isr6, 0x08, 0x8E);
	idt_set_gate(7, (_u32) isr7, 0x08, 0x8E);
	idt_set_gate(8, (_u32) isr8, 0x08, 0x8E);
	idt_set_gate(9, (_u32) isr9, 0x08, 0x8E);
	idt_set_gate(10, (_u32) isr10, 0x08, 0x8E);
	idt_set_gate(11, (_u32) isr11, 0x08, 0x8E);
	idt_set_gate(12, (_u32) isr12, 0x08, 0x8E);
	idt_set_gate(13, (_u32) isr13, 0x08, 0x8E);
	idt_set_gate(14, (_u32) isr14, 0x08, 0x8E);
	idt_set_gate(15, (_u32) isr15, 0x08, 0x8E);
	idt_set_gate(16, (_u32) isr16, 0x08, 0x8E);
	idt_set_gate(17, (_u32) isr17, 0x08, 0x8E);
	idt_set_gate(18, (_u32) isr18, 0x08, 0x8E);
	idt_set_gate(19, (_u32) isr19, 0x08, 0x8E);
	idt_set_gate(20, (_u32) isr20, 0x08, 0x8E);
	idt_set_gate(21, (_u32) isr21, 0x08, 0x8E);
	idt_set_gate(22, (_u32) isr22, 0x08, 0x8E);
	idt_set_gate(23, (_u32) isr23, 0x08, 0x8E);
	idt_set_gate(24, (_u32) isr24, 0x08, 0x8E);
	idt_set_gate(25, (_u32) isr25, 0x08, 0x8E);
	idt_set_gate(26, (_u32) isr26, 0x08, 0x8E);
	idt_set_gate(27, (_u32) isr27, 0x08, 0x8E);
	idt_set_gate(28, (_u32) isr28, 0x08, 0x8E);
	idt_set_gate(29, (_u32) isr29, 0x08, 0x8E);
	idt_set_gate(30, (_u32) isr30, 0x08, 0x8E);
	idt_set_gate(31, (_u32) isr31, 0x08, 0x8E);
}

static void init_8259A()
{
	// Remap the irq table.
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);

	idt_set_gate(32, (_u32) irq0, 0x08, 0x8E);
	idt_set_gate(33, (_u32) irq1, 0x08, 0x8E);
	idt_set_gate(34, (_u32) irq2, 0x08, 0x8E);
	idt_set_gate(35, (_u32) irq3, 0x08, 0x8E);
	idt_set_gate(36, (_u32) irq4, 0x08, 0x8E);
	idt_set_gate(37, (_u32) irq5, 0x08, 0x8E);
	idt_set_gate(38, (_u32) irq6, 0x08, 0x8E);
	idt_set_gate(39, (_u32) irq7, 0x08, 0x8E);
	idt_set_gate(40, (_u32) irq8, 0x08, 0x8E);
	idt_set_gate(41, (_u32) irq9, 0x08, 0x8E);
	idt_set_gate(42, (_u32) irq10, 0x08, 0x8E);
	idt_set_gate(43, (_u32) irq11, 0x08, 0x8E);
	idt_set_gate(44, (_u32) irq12, 0x08, 0x8E);
	idt_set_gate(45, (_u32) irq13, 0x08, 0x8E);
	idt_set_gate(46, (_u32) irq14, 0x08, 0x8E);
	idt_set_gate(47, (_u32) irq15, 0x08, 0x8E);

	// TODO disable Master/Slave int with masks except Slave IRQ
}

static void init_smp_irq()
{
	if (!mpinfo.ismp)
		return;

	idt_set_gate(IRQ_INVAL_TLB, (_u32) smp_irq_inval_tlb, 0x08, 0x8E);
	idt_set_gate(IRQ_STOP_CPU, (_u32) smp_irq_stop_cpu, 0x08, 0x8E);
	idt_set_gate(IRQ_LOCAL_TIMER, (_u32) smp_irq_local_timer, 0x08, 0x8E);
}

static void init_idt()
{
	idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
	idt_ptr.base = (_u32) & idt_entries;

	memset((_u8 *) & idt_entries, 0, sizeof(idt_entry_t) * 256);

	init_cpu_isr();
	init_8259A();
	init_smp_irq();

	idt_flush((_u32) & idt_ptr);
}

static void idt_set_gate(_u8 num, _u32 base, _u16 sel, _u8 flags)
{
	idt_entries[num].base_lo = base & 0xFFFF;
	idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

	idt_entries[num].sel = sel;
	idt_entries[num].always0 = 0;
	// We must uncomment the OR below when we get to using user-mode.
	// It sets the interrupt gate's privilege level to 3.
	idt_entries[num].flags = flags /* | 0x60 */ ;
}

// This gets called from our ASM interrupt handler stub.
// For cpu internel exceptions.
void isr_handler(registers_t * regs)
{
	if (interrupt_handlers[regs->int_no]) {
		interrupt_handlers[regs->int_no] (regs);
	} else {
		printk("CPU #%u received interrupt #%d\n",
		       get_processor()->proc_id, regs->int_no);
	}
}

// This gets called from our ASM interrupt handler stub.
// for interrupt requests from 8259A
void irq_handler(registers_t * regs)
{
	// TODO if not pic should we skip 8259?
	if (regs->int_no >= IRQ0 && regs->int_no <= IRQ15) {
		// Send an EOI (end of interrupt) signal to the PICs.
		// If this interrupt involved the slave.
		if (regs->int_no >= 40) {
			// Send reset signal to slave.
			outb(0xA0, 0x20);
		}
		// Send reset signal to master. (As well as slave, if necessary).
		outb(0x20, 0x20);
	}
	// send an EOI to local APIC
	lapic_eoi();

	if (interrupt_handlers[regs->int_no] != 0) {
		isr_t handler = interrupt_handlers[regs->int_no];
		handler(regs);
	}
}

isr_t register_interrupt_handler(_u8 n, isr_t handler)
{
	isr_t prev_isr;

	prev_isr = interrupt_handlers[n];
	interrupt_handlers[n] = handler;

	return prev_isr;
}

void local_irq_disable()
{
	asm volatile ("cli");
}

void local_irq_enable()
{
	asm volatile ("sti");
}

void local_irq_save()
{
	cpu_state_t *cpu;
	uint_t flags;

	cpu = get_processor();
	flags = local_get_flags();
	local_irq_disable();
	cpu->saved_flags = flags;
}

void local_irq_restore()
{
	cpu_state_t *cpu;

	cpu = get_processor();
	local_set_flags(cpu->saved_flags);
}
