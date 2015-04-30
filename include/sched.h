#ifndef SCHED_H
#define SCHED_H

#include "common.h"
#include "task.h"
#include "list.h"

typedef struct {
	thread_t *thread;
	list_head_t list;
} thread_list_t;

typedef struct {
	list_head_t ready;
	list_head_t blocked;
	list_head_t complete;
	thread_t *rthread;
} sched_list_t;

typedef struct {
	sched_list_t taskq;
} cpu_state_t;

extern void pick_next_thread(cpu_state_t * cur);

extern void init_sched();

extern void set_thread(thread_state_t state);
extern void switch_to(thread_context_t * prev, thread_context_t * next);
extern void schedule();

extern task_t *get_curr_task();
#endif
