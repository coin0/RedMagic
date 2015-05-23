/*
 *	Red Magic 1996 - 2015
 *
 *	semaphore.c - provide semaphore syncronization for kernel
 *
 *	2015 Lin Coin - initial version
 */

#include "common.h"
#include "locking.h"
#include "klog.h"
#include "sched.h"

typedef struct {
	thread_t *threadp;
	list_head_t wq;
} semq_thread_t;

static void sem_add_to_wq(semq_thread_t * p, semaphore_t * sem);
static void sem_remove_from_wq(semq_thread_t * p, semaphore_t * sem);

static int __sem_down(semaphore_t * sem);
static int __sem_up(semaphore_t * sem);

void sem_init(semaphore_t * sem, uint_t count)
{
	spin_lock_init(&sem->semlock);
	INIT_LIST_HEAD(&sem->wq);
	sem->count = count;
}

void sem_down(semaphore_t * sem)
{
	int rv;

	spin_lock_irqsave(&sem->semlock);
	for (;;) {
		if (sem->count > 0) {
			sem->count--;
			break;
		} else {
			rv = __sem_down(sem);
			if (!rv)
				break;
		}
	}
	spin_unlock_irqrestore(&sem->semlock);
}

void sem_up(semaphore_t * sem)
{
	spin_lock_irqsave(&sem->semlock);
	if (list_empty(&sem->wq))
		sem->count++;
	else
		__sem_up(sem);
	spin_unlock_irqrestore(&sem->semlock);
}

static int __sem_down(semaphore_t * sem)
{
	semq_thread_t wlist;

	wlist.threadp = get_curr_thread();
	sem_add_to_wq(&wlist, sem);
	make_sleep();
	spin_unlock_irq(&sem->semlock);

	schedule();

	// waked up, return from scheduler
	spin_lock_irq(&sem->semlock);

	return OK;
}

static int __sem_up(semaphore_t * sem)
{
	semq_thread_t *p;

	// must remove the first waiter from queue here, because semaphore
	// allows multiple threads entering critical section and when they
	// leave, all of them will call sem_up(), if we do not remove waiter
	// in __sem_up(), the first waiter can only be removed from queue
	// when scheduling INT comes, so semaphore count won't get increased
	p = list_first_entry(&sem->wq, semq_thread_t, wq);
	wake_up(p->threadp);
	sem_remove_from_wq(p, sem);

	return OK;
}

static void sem_add_to_wq(semq_thread_t * p, semaphore_t * sem)
{
	semq_thread_t *tmp;

	// if already exists in wq, log warning
	list_for_each_entry(tmp, &sem->wq, wq) {
		if (tmp == p) {
			log_warn(LOG_SEM "thread 0x%08X already in semq",
				 p->threadp);
			return;
		}
	}

	list_add_tail(&p->wq, &sem->wq);
}

static void sem_remove_from_wq(semq_thread_t * p, semaphore_t * sem)
{
	semq_thread_t *tmp;

	list_for_each_entry(tmp, &sem->wq, wq) {
		if (tmp == p) {
			list_del(&p->wq);
			return;
		}
	}

	log_warn(LOG_SEM "no such entry 0x%08X in semq", p->threadp);
}
