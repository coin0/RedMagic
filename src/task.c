#include "common.h"
#include "task.h"
#include "sched.h"
#include "debug.h"
#include "heap.h"
#include "list.h"
#include "klog.h"
#include "string.h"

// default task group including all user tasks
static task_group_t all_tasks;

// task group functions, make sure init the task group before using it
static void init_task_group(task_group_t * task_group);
static void add_to_task_group(task_group_t * task_group, task_t * task);

// internel functions for creating tasks and threads
static task_id_t __create_task(task_group_t * task_group,
			       task_id_t task_id, mmc_t * mm,
			       page_directory_t * addr_space, task_t * parent,
			       thread_id_t thread_id, int (*fn) (void *),
			       void *arg);
static thread_id_t __create_thread(task_t * task, int (*fn) (void *),
				   void *arg);

// thread exits here also checks if task should be cleaned up
static void __finish_thread();

// generate task id for newly created task
static task_id_t alloc_task_id();
static int verify_task_id(task_group_t * tgrp, task_id_t tid);

/*
 * functions
 */

void setup_init_task()
{
	task_id_t tid;

	init_task_group(&all_tasks);
	tid = create_kernel_task(K_INIT, NULL);
	if (!tid)
		PANIC("Kernel task setup failed");
}

static void init_task_group(task_group_t * task_group)
{
	INIT_LIST_HEAD(&task_group->task_list);
}

static void add_to_task_group(task_group_t * task_group, task_t * task)
{
	list_add_tail(&task->task_list, &task_group->task_list);
}

task_id_t create_task(int (*fn) (void *), void *arg)
{
	return __create_task(&all_tasks, 0, NULL, NULL, NULL, 0, fn, arg);
}

task_id_t create_kernel_task(int (*fn) (void *), void *arg)
{
	return __create_task(&all_tasks, 0, &mm_phys, NULL, NULL, 0, fn, arg);
}

static task_id_t __create_task(task_group_t * task_group,
			       task_id_t task_id, mmc_t * mm,
			       page_directory_t * addr_space, task_t * parent,
			       thread_id_t thread_id, int (*fn) (void *),
			       void *arg)
{
	task_t *taskp;
	mmc_t *mmcp;

	// + create task struct and set the task as INIT status
	taskp = (task_t *) kmalloc(sizeof(task_t));
	if (taskp == NULL) {
		log_err("could not allocate task struct\n");
		return 0;
	}
	bzero(taskp, sizeof(task_t));
	taskp->status = T_INIT;

	// + initialize task ID
	if (!task_id) {
		taskp->task_id = alloc_task_id(task_group);
		if (taskp->task_id == 0) {
			log_err("could not allocate new task ID\n");
			goto c_err;
		}
	} else {
		if (verify_task_id(task_group, task_id)) {
			log_err("fail to set specified task ID\n");
			goto c_err;
		}
		taskp->task_id = task_id;
	}

	// + initialize address space
	if (mm == NULL) {
		mmcp = (mmc_t *) kmalloc(sizeof(mmc_t));
		if (mmcp == NULL) {
			log_err("memory struct allocation error\n");
			goto c_err;
		}
		taskp->mm = mmcp;
	} else
		taskp->mm = mm;

	// + set address space (high memory area)
	if (addr_space == NULL) {
		taskp->addr_space = (page_directory_t *) get_zeroed_page();
		if (taskp->addr_space == NULL) {
			log_err("could not setup page directory\n");
			goto c_err;
		}
		copy_page_directory_from(k_pdir, taskp->addr_space);
	} else
		taskp->addr_space = addr_space;

	// + set parent
	if (parent == NULL) {
		// TODO set parent as current task
	} else
		taskp->parent = parent;

	// + initialize main thread
	INIT_LIST_HEAD(&taskp->thread_list);
	if (!__create_thread(taskp, fn, arg)) {
		log_err("could not create thread\n");
		return 0;
	}
	// + add to task group
	add_to_task_group(task_group, taskp);

	return taskp->task_id;

      c_err:

	// this 0 stands for failure
	return 0;
}

thread_id_t create_thread(int (*fn) (void *), void *arg)
{
	return 0;
}

static thread_id_t __create_thread(task_t * task, int (*fn) (void *), void *arg)
{
	thread_t *threadp;
	void *stackp;
	addr_t top;

	// alloc thread struct
	threadp = (thread_t *) kmalloc(sizeof(thread_t));
	if (threadp == NULL) {
		log_err("could not alloc thread struct\n");
		goto thr_err;
	}
	bzero(threadp, sizeof(thread_t));

	// set 'init' state so won't be scheduled before 'ready'
	threadp->status = T_INIT;

	// TODO generate thread id
	threadp->thread_id = 1;

	// initialize kernel stack, user stack should be initialized when the
	// first time the new thread is scheduled
	stackp = kmalloc(T_STACK_SIZE);
	if (stackp == NULL) {
		log_err("could not alloc stack space\n");
		goto thr_err;
	}
	threadp->kstack_base = (addr_t) stackp;
	threadp->kstack_size = T_STACK_SIZE;

	// reserve return address in the stack frame
	top = (addr_t) stackp + T_STACK_SIZE;

	top -= sizeof(addr_t *);
	*(addr_t *) top = (addr_t) __finish_thread;
	top -= sizeof(addr_t *);
	*(addr_t *) top = (addr_t) arg;
	top -= sizeof(addr_t *);
	*(addr_t *) top = (addr_t) fn;

#ifdef ARCH_X86_32
	// initialize context structure
	threadp->context.esp = top;
	// make sure interrupt is switched ON
	threadp->context.eflags = 0x200;
#endif

	// add this thread to task
	list_add_tail(&threadp->thread_list, &task->thread_list);

	return threadp->thread_id;

      thr_err:
	return 0;
}

static task_id_t alloc_task_id(task_group_t * tgrp)
{
	static task_id_t id = 0;

	id++;

	return id;
}

static int verify_task_id(task_group_t * tgrp, task_id_t tid)
{
	return 0;
}

static void __finish_thread()
{
	printk("Finished \n");
}
