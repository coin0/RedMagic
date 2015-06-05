#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "list.h"
#include "task.h"
#include "sched.h"
#include "locking.h"
#include "mp.h"

// forward declaration for gdt.h
struct cpu_state;
#include "gdt.h"

#define MAX_GDT_ENT_PCPU 10

typedef struct cpu_state {
	uint_t proc_id;
	uint_t flag_bsp;
	uint_t preempt_on;
	uint_t saved_flags;
	list_head_t runq;
	thread_t *rthread;
	spinlock_t rq_lock;
	scheduler_t scheduler;

#ifdef ARCH_X86_32
	gdt_entry_t gdt_entries[MAX_GDT_ENT_PCPU];
	gdt_ptr_t gdt_ptr;
#endif

	list_head_t cpu_list;
} cpu_state_t;

#define MAX_CPUS 32
extern cpu_state_t cpuset[];

typedef struct {
	uint_t cpu_num;
	list_head_t cpu_list;
} cpu_set_t;

// global processor initialization
extern void init_bootstrap_processor();
extern void init_application_processor();

// functions
extern cpu_state_t *get_processor();
extern cpu_state_t *get_boot_processor();
extern size_t get_cpu_count();
extern void cpu_reset_state(cpu_state_t * cpu);

#define cpu_set_val(cpu, member, val) ((cpu)->member = (val))

extern void preempt_enable();
extern void preempt_disable();

#endif
