/* spin lock */

#include "common.h"
#include "locking.h"
#include "cpu.h"
#include "debug.h"
#include "interrupt.h"

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

void spin_unlock_irqsave(spinlock_t * lock)
{
	spin_release(lock);
	preempt_enable();
	local_irq_enable();
}

static inline void spin_acquire(spinlock_t * lock)
{
	while (acquire_rlock(&lock->slock)) ;
	ASSERT(lock->owner_cpu == NULL);
	lock->owner_cpu = get_processor();

}

static inline void spin_release(spinlock_t * lock)
{
	release_rlock(&lock->slock);
	lock->owner_cpu = NULL;
}
