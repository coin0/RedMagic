/*
 *  scheduler
 */

#include "common.h"
#include "task.h"
#include "sched.h"
#include "list.h"
#include "debug.h"
#include "heap.h"
#include "print.h"

static thread_t *pick_next_thread(cpu_state_t * cur);
static thread_t *__do_sched(cpu_state_t * cur);
static thread_t *__do_sched_rr(cpu_state_t * cur);
static thread_t *__do_sched_pior(cpu_state_t * cur);

void schedule()
{
	cpu_state_t *cpu;
	thread_t *rthread, *nextp;

	cpu = get_processor();
	nextp = pick_next_thread(cpu);
	rthread = cpu->rthread;
	cpu->rthread = nextp;
	switch_to(&rthread->context, &nextp->context);
}

static thread_t *pick_next_thread(cpu_state_t * cur)
{
	return __do_sched(cur);
}

static thread_t *__do_sched(cpu_state_t * cur)
{
	thread_t *nextp;

	switch (cur->scheduler) {
	case SCHED_RR:
		nextp = __do_sched_rr(cur);
		break;
	case SCHED_PRIOR:
		nextp = __do_sched_pior(cur);
		break;
	default:
		PANIC("#BUG");
		break;
	}

	return nextp;
}

static thread_t *__do_sched_rr(cpu_state_t * cur)
{
	rthread_list_t *tlist;
	unsigned int found = 0;

	if (list_empty(&cur->runq))
		PANIC("CPU is idle");

	list_for_each_entry(tlist, &cur->runq, runq) {
		if (tlist->threadp == cur->rthread) {
			found = 1;
			break;
		}
	}

	// what?! current running thread is not in rq?
	ASSERT(found);

	if (list_is_last(&tlist->runq, &cur->runq))
		tlist = list_first_entry(&cur->runq, rthread_list_t, runq);
	else
		tlist = list_next_entry(tlist, runq);

	return tlist->threadp;
}

static thread_t *__do_sched_pior(cpu_state_t * cur)
{
	return NULL;
}

void init_sched()
{
	cpu_state_t *cpu;
	rthread_list_t *init;

	cpu = get_processor();

	// before scheduling, the init task should be already added to rq
	ASSERT(!list_empty(&cpu->runq));

	init = list_first_entry(&cpu->runq, rthread_list_t, runq);
	cpu->rthread = init->threadp;
	cpu->scheduler = SCHED_DEFAULT;
	switch_to_init(&(cpu->rthread->context));
}

int add_task_to_rq(task_t * taskp)
{
	thread_t *threadp;

	ASSERT(taskp->status == T_INIT);
	threadp = list_entry(taskp->thread_list.next, thread_t, thread_list);
	if (!add_thread_to_rq(threadp)) {
		taskp->status = T_READY;
		return OK;
	}

	return 1;
}

int add_thread_to_rq(thread_t * threadp)
{
	cpu_state_t *cpu;
	rthread_list_t *ptr;

	ptr = (rthread_list_t *) kmalloc(sizeof(rthread_list_t));
	if (ptr == NULL)
		return 1;

	cpu = get_processor();
	ptr->threadp = threadp;
	list_add_tail(&ptr->runq, &cpu->runq);

	threadp->status = T_READY;

	return OK;
}

task_t *get_curr_task()
{
	cpu_state_t *cpu;

	cpu = get_processor();
	return cpu->rthread->task;
}

thread_t *get_curr_thread()
{
	cpu_state_t *cpu;

	cpu = get_processor();
	return cpu->rthread;
}
