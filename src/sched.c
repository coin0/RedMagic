/*
 *  scheduler
 */

#include "common.h"
#include "task.h"
#include "sched.h"
#include "list.h"
#include "klog.h"
#include "heap.h"
#include "print.h"
#include "timer.h"

static cpu_state_t *pick_processor();
static cpu_state_t *__pick_processor_fsr();
static void rq_calc_len_and_ready(cpu_state_t * cpu, size_t * len,
				  size_t * ready);

static thread_t *pick_next_thread(cpu_state_t * cur);
static thread_t *__do_sched(cpu_state_t * cur);
static thread_t *__do_sched_rr(cpu_state_t * cur);
static thread_t *__do_sched_pior(cpu_state_t * cur);

static int add_task_to_rq(task_t * taskp);
static int add_thread_to_rq(thread_t * threadp);
static void *find_thread_in_rq(thread_t * threadp, list_head_t * q);

#define __set_thread_status(threadp, state) 		\
	do {  						\
		(threadp)->status = (state);		\
	}while(0);

#define __set_task_status(taskp, state)			\
	do {						\
		(taskp)->status = (state);		\
	}while(0);

static inline void set_thread_status(thread_state_t status);
static inline void set_task_status(task_state_t status);

/*
 *  core function to pick thread to run, it gets called 
 *  under following 3 cases
 *
 *  1. PIT interrupt
 *  2. explicitly call this function
 *  3. when a thread is going to sleep
 */
void schedule()
{
	cpu_state_t *cpu;
	thread_t *rthread, *nextp;

	preempt_disable();

	cpu = get_processor();
	rthread = cpu->rthread;
	nextp = pick_next_thread(cpu);
	cpu->rthread = nextp;

	// will re-enable preemption in switch_to(_init)
	if (rthread != NULL) {
		switch_to(&rthread->context, &nextp->context);
	} else
		switch_to_init(&nextp->context);
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

	if (cur->rthread != NULL) {
		tlist = find_thread_in_rq(cur->rthread, &cur->runq);
		if (tlist == NULL)
			PANIC("Running thread is not in RQ");
	}
	// if we do not have any thread in T_READY, let's fall into an
	// infinite loop till a thread is waked up, enable interrupt so
	// that we can receive INT events to wakeup threads, otherwise
	// probably a dead-lock
	local_irq_save();
	local_irq_enable();

	do {
		if (cur->rthread != NULL) {
			if (list_is_last(&tlist->runq, &cur->runq))
				tlist =
				    list_first_entry(&cur->runq, rthread_list_t,
						     runq);
			else
				tlist = list_next_entry(tlist, runq);
		} else {
			// probably this is the initial scheduling, fetch the
			// first entry of runq, if NULL, keep polling
			// otherwise set as current thread to start RR sched
			tlist =
			    list_first_entry_or_null(&cur->runq, rthread_list_t,
						     runq);
			if (tlist == NULL)
				continue;
			else
				cur->rthread = tlist->threadp;
		}
	} while (tlist == NULL || tlist->threadp->status != T_READY);

	local_irq_restore();

	return tlist->threadp;
}

static thread_t *__do_sched_pior(cpu_state_t * cur)
{
	return NULL;
}

static cpu_state_t *pick_processor()
{
	if (!mpinfo.ismp)
		return get_processor();

	return __pick_processor_fsr();
}

/* pick first cpu with shortest queue and ready */
static cpu_state_t *__pick_processor_fsr()
{
	uint_t i;
	size_t len, ready;
	size_t prev_len, prev_ready;
	cpu_state_t *cpu;

	ASSERT(mpinfo.ismp);

	for (i = 0, cpu = cpuset; i < mpinfo.ncpu; i++) {
		rq_calc_len_and_ready(&cpu[i], &len, &ready);
		if (i == 0 || len < prev_len
		    || (len == prev_len && ready < prev_ready)) {
			cpu = &cpuset[i];
			prev_len = len;
			prev_ready = ready;
		}
	}

	return cpu;
}

static void rq_calc_len_and_ready(cpu_state_t * cpu, size_t * len,
				  size_t * ready)
{
	rthread_list_t *tlist;

	*len = 0;
	*ready = 0;

	if (list_empty(&cpu->runq))
		return;

	tlist = list_first_entry(&cpu->runq, rthread_list_t, runq);
	for (;;) {
		(*len)++;
		if (tlist->threadp->status == T_READY)
			(*ready)++;
		if (list_is_last(&tlist->runq, &cpu->runq))
			break;
		tlist = list_next_entry(tlist, runq);
	}
}

void init_sched()
{
	cpu_state_t *cpu;

	cpu = get_processor();

	// before scheduling, for BSP the init task should be already added to rq
	ASSERT(!cpu->flag_bsp || (cpu->flag_bsp && !list_empty(&cpu->runq)));

	cpu->scheduler = SCHED_DEFAULT;
	schedule();
}

static inline void set_thread_status(thread_state_t status)
{
	__set_thread_status(get_curr_thread(), status);
}

static inline void set_task_status(task_state_t status)
{
	__set_task_status(get_curr_task(), status);
}

int init_task_sched(task_t * taskp)
{
	return add_task_to_rq(taskp);
}

int init_thread_sched(thread_t * threadp)
{
	return add_thread_to_rq(threadp);
}

/*
 *  following functions, add_* will handle current scheduling queue
 *  before using them, make sure you are holding cpu->sched_lock 
 */
static int add_task_to_rq(task_t * taskp)
{
	thread_t *threadp;

	// only support newly created task
	ASSERT(taskp->status == T_INIT);
	threadp = list_entry(taskp->thread_list.next, thread_t, thread_list);
	if (!add_thread_to_rq(threadp)) {
		__set_task_status(taskp, T_READY);
		return OK;
	}

	return 1;
}

static int add_thread_to_rq(thread_t * threadp)
{
	cpu_state_t *cpu;
	rthread_list_t *ptr;

	ptr = (rthread_list_t *) kmalloc(sizeof(rthread_list_t));
	if (ptr == NULL)
		return 1;

	cpu = pick_processor();
	ptr->threadp = threadp;
	spin_lock_irqsave(&cpu->rq_lock);
	list_add_tail(&ptr->runq, &cpu->runq);
	spin_unlock_irqrestore(&cpu->rq_lock);
	__set_thread_status(threadp, T_READY);

	return OK;
}

static void *find_thread_in_rq(thread_t * threadp, list_head_t * q)
{
	rthread_list_t *tmp = NULL;

	list_for_each_entry(tmp, q, runq) {
		if (tmp->threadp == threadp) {
			break;
		}
	}

	return tmp;
}

task_t *get_curr_task()
{
	return get_processor()->rthread->task;
}

thread_t *get_curr_thread()
{
	return get_processor()->rthread;
}

int make_sleep()
{
	set_thread_status(T_BLOCKED);

	return OK;
}

int make_sleep_resched()
{
	make_sleep();
	schedule();

	return OK;
}

int wake_up(thread_t * threadp)
{
	if (threadp->status == T_BLOCKED) {
		__set_thread_status(threadp, T_READY);
		return OK;
	} else {
		log_warn(LOG_SCHED "try to wake up non-sleeping thread[0x%08X]",
			 threadp);
		return 1;
	}
}

void pause(uint_t sec)
{
	thread_t *threadp = get_curr_thread();

	alarm_reset(&threadp->alarm);
	alarm_set(&threadp->alarm, TICK_SEC * sec);
	make_sleep_resched();
}

void check_thread_alarms()
{
	cpu_state_t *cpu;
	rthread_list_t *r;
	thread_t *thrp;

	cpu = get_processor();
	list_for_each_entry(r, &cpu->runq, runq) {
		// TODO for kernel object, need to add magic/chksum mechanism
		thrp = r->threadp;
		ASSERT(thrp != NULL);
		if (thrp->status == T_BLOCKED)
			if (alarm_check(&thrp->alarm)) {
				alarm_unset(&thrp->alarm);
				wake_up(thrp);
			}
	}
}
