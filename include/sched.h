#ifndef SCHED_H
#define SCHED_H

#include "common.h"
#include "task.h"
#include "list.h"

// frequency of scheduling interrupts
#define SCHED_HZ 100

typedef enum {
	SCHED_RR,
	SCHED_PRIOR,

	// No Move
	SCHED_LAST
} scheduler_t;

#define SCHED_DEFAULT SCHED_RR

#include "cpu.h"

typedef struct {
	uint_t slice;
} sched_rr_t;

typedef struct {
	uint_t slice;
	uint_t prior;
} sched_prior_t;

typedef struct {
	union {
		sched_rr_t rr;
		sched_prior_t pp;
	};
	thread_t *threadp;
	list_head_t runq;	// to cpu_state_t.runq
} rthread_list_t;

// add newly created task and thread to cpu run queue
int add_task_to_rq(task_t * tskp);
int add_thread_to_rq(thread_t * thrp);

// get current running task
extern task_t *get_curr_task();

// OS starts scheduling
extern void init_sched();

extern void switch_to(thread_context_t * prev, thread_context_t * next);
extern void switch_to_init(thread_context_t * next);
extern void schedule();

#endif
