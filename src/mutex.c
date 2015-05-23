#include "common.h"
#include "locking.h"
#include "task.h"
#include "sched.h"
#include "klog.h"

typedef struct {
	thread_t *threadp;
	list_head_t wq;
} mq_thread_t;

static inline void mutex_clear_owner(mutex_t * lock);
static inline void mutex_set_owner(mutex_t * lock);
static inline void __mutex_set_owner(mutex_t * lock, thread_t * to);
static inline void mutex_add_to_wq(mq_thread_t * p, mutex_t * lock);
static inline void mutex_remove_from_wq(mq_thread_t * p, mutex_t * lock);

void mutex_init(mutex_t * lock)
{
	init_rlock(&lock->mlock);
	spin_lock_init(&lock->wlock);
	INIT_LIST_HEAD(&lock->wq);
	mutex_clear_owner(lock);
}

void mutex_lock(mutex_t * lock)
{
	mq_thread_t wlist;

	spin_lock_irqsave(&lock->wlock);
	if (!acquire_rlock(&lock->mlock)) {
		// damn great, I got the lock!
		ASSERT(lock->owner == NULL);
		mutex_set_owner(lock);
	} else {
		// we failed, add to waiting list till the lock
		// keeper released the mutex and the first waiter
		// will be waked up
		for (;;) {
			wlist.threadp = get_curr_thread();
			mutex_add_to_wq(&wlist, lock);
			make_sleep();
			spin_unlock_irq(&lock->wlock);

			// pass CPU to any other threads
			schedule();

			spin_lock_irq(&lock->wlock);
			if (lock->owner == wlist.threadp) {
				mutex_remove_from_wq(&wlist, lock);
				break;
			}
		}
	}
	spin_unlock_irqrestore(&lock->wlock);
}

void mutex_unlock(mutex_t * lock)
{
	mq_thread_t *p;

	// if no one in the waiting list, just release the lock, otherwise
	// pass the key to the first guy standing in the front
	spin_lock_irqsave(&lock->wlock);
	if (list_empty(&lock->wq)) {
		mutex_clear_owner(lock);
		release_rlock(&lock->mlock);
	} else {
		p = list_first_entry(&lock->wq, mq_thread_t, wq);
		__mutex_set_owner(lock, p->threadp);
		wake_up(p->threadp);
	}
	spin_unlock_irqrestore(&lock->wlock);
}

static inline void mutex_clear_owner(mutex_t * lock)
{
	lock->owner = NULL;
}

static inline void mutex_set_owner(mutex_t * lock)
{
	__mutex_set_owner(lock, get_curr_thread());
}

static inline void __mutex_set_owner(mutex_t * lock, thread_t * to)
{
	lock->owner = to;
}

static inline void mutex_add_to_wq(mq_thread_t * p, mutex_t * lock)
{
	mq_thread_t *tmp;

	// return if already added
	list_for_each_entry(tmp, &lock->wq, wq) {
		if (tmp == p) {
			log_warn(LOG_MUTEX "thread 0x%08X already in mq\n",
				 p->threadp);
			return;
		}
	}

	list_add_tail(&p->wq, &lock->wq);
}

static inline void mutex_remove_from_wq(mq_thread_t * p, mutex_t * lock)
{
	mq_thread_t *tmp;

	list_for_each_entry(tmp, &lock->wq, wq) {
		if (tmp == p) {
			list_del(&p->wq);
			return;
		}
	}

	log_warn(LOG_MUTEX "no such entry 0x%08X in mq\n", p->threadp);
}
