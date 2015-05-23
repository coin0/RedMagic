/* spin lock */

#include "common.h"
#include "locking.h"
#include "cpu.h"
#include "debug.h"
#include "interrupt.h"
#include "klog.h"

static inline void spin_acquire(spinlock_t * lock);
static inline void spin_release(spinlock_t * lock);

void spin_lock_init(spinlock_t * lock)
{
	init_rlock(&lock->slock);
	lock->owner_cpu = NULL;
}

void spin_lock(spinlock_t * lock)
{
	preempt_disable();
	spin_acquire(lock);
}

uint_t spin_trylock(spinlock_t * lock)
{
	preempt_disable();
	if (acquire_rlock(&lock->slock)) {
		// shit , failed
		preempt_enable();
		return 0;
	}
	// I got it !!!
	return 1;
}

void spin_unlock(spinlock_t * lock)
{
	spin_release(lock);
	preempt_enable();
}

void spin_lock_irq(spinlock_t * lock)
{
	local_irq_disable();
	preempt_disable();
	spin_acquire(lock);
}

void spin_unlock_irq(spinlock_t * lock)
{
	spin_release(lock);
	preempt_enable();
	local_irq_enable();
}

void spin_lock_irqsave(spinlock_t * lock)
{
	local_irq_save();
	preempt_disable();
	spin_acquire(lock);
}

void spin_unlock_irqrestore(spinlock_t * lock)
{
	spin_release(lock);
	local_irq_restore();
	preempt_enable();
}

static inline void spin_acquire(spinlock_t * lock)
{
	while (acquire_rlock(&lock->slock)) ;
	ASSERT(lock->owner_cpu == NULL);
	lock->owner_cpu = get_processor();
}

static inline void spin_release(spinlock_t * lock)
{
	lock->owner_cpu = NULL;
	release_rlock(&lock->slock);
}
