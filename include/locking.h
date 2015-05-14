#ifndef LOCKING_H
#define LOCKING_H

#include "common.h"
#include "task.h"

typedef struct {
	uint_t rlock;
} rawlock_t;

#ifdef ARCH_X86_32

static inline uint_t xchg(volatile addr_t * addr, uint_t newval)
{
	uint_t result;

	// The + in "+m" denotes a read-modify-write operand.
	asm volatile ("lock; xchgl %0, %1":
		      "+m" (*addr), "=a"(result):"1"(newval):"cc");
	return result;
}

#endif

static inline void init_rlock(rawlock_t * lk)
{
	lk->rlock = 0;
}

static inline uint_t acquire_rlock(rawlock_t * lk)
{
	return xchg(&lk->rlock, 1);
}

static inline uint_t release_rlock(rawlock_t * lk)
{
	return xchg(&lk->rlock, 0);
}

/*
 *   Spinlock
 */
typedef struct {
	rawlock_t slock;
	void *owner_cpu;
} spinlock_t;

extern void spin_lock_init(spinlock_t * lock);

extern void spin_lock(spinlock_t * lock);
extern void spin_unlock(spinlock_t * lock);

extern void spin_lock_irq(spinlock_t * lock);
extern void spin_unlock_irq(spinlock_t * lock);

extern void spin_lock_irqsave(spinlock_t * lock);
extern void spin_unlock_irqrestore(spinlock_t * lock);

/*
 *   Mutexlock
 */

typedef struct {
	rawlock_t mlock;
	const char *name;
	thread_t *owner;
	list_head_t wq;
	spinlock_t wlock;
} mutex_t;

extern void mutex_init(mutex_t * lock);

extern void mutex_lock(mutex_t * lock);
extern void mutex_unlock(mutex_t * lock);

/*
 *   Semaphore
 */

typedef struct {
} semaphore_t;

#endif
