#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "list.h"
#include "task.h"
#include "sched.h"

typedef struct {
	char *proc_id;
	list_head_t runq;
	thread_t *rthread;
	scheduler_t scheduler;
} cpu_state_t;

extern void init_processor();
extern cpu_state_t *get_processor();

#define CPU_CUR &cpu_1

#endif
