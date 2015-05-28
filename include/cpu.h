#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "list.h"
#include "task.h"
#include "sched.h"
#include "locking.h"

typedef struct {
	uint_t proc_id;
	uint_t flag_bsp;
	uint_t preempt_on;
	uint_t saved_flags;
	list_head_t runq;
	thread_t *rthread;
	spinlock_t rq_lock;
	scheduler_t scheduler;
	list_head_t cpu_list;
} cpu_state_t;

#define MAX_CPUS 32
extern cpu_state_t cpuset[];

typedef struct {
	uint_t cpu_num;
	list_head_t cpu_list;
} cpu_set_t;

// global processor initialization
extern void init_processor();

// call mp.c
extern int init_mp();

// functions
extern cpu_state_t *get_processor();
extern size_t get_cpu_count();
extern void inc_cpu_count();
extern void cpu_reset_state(cpu_state_t * cpu);

#define cpu_set_val(cpu, member, val) ((cpu)->member = (val))

extern void preempt_enable();
extern void preempt_disable();

#endif
