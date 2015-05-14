#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "list.h"
#include "task.h"
#include "sched.h"
#include "locking.h"

typedef struct {
	char *proc_id;
	uint_t preempt_on;
	uint_t saved_flags;
	list_head_t runq;
	thread_t *rthread;
	spinlock_t rq_lock;
	scheduler_t scheduler;
	list_head_t cpu_list;
} cpu_state_t;

typedef struct {
	uint_t cpu_num;
	list_head_t cpu_list;
} cpu_set_t;

extern void init_processor();
extern cpu_state_t *get_processor();
extern uint_t get_cpu_num();

extern void preempt_enable();
extern void preempt_disable();

#define CPU_CUR &cpu_1

#endif
