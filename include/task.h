#ifndef TASK_H
#define TASK_H

#include "common.h"
#include "mm.h"
#include "list.h"
#include "paging.h"

// stack size for each thread
#define T_STACK_SIZE 0x1000

// some typedef
typedef uint_t task_id_t;
typedef uint_t thread_id_t;
typedef uint_t tgroup_id_t;

typedef enum {
	T_INIT,
	T_READY,
	T_RUNNING,
	T_BLOCKED,
	T_COMPLETE,

	// insert above
	T_LAST_STATE
} thread_state_t, task_state_t;

#ifdef ARCH_X86_32

typedef struct {
	_u32 esp;
	_u32 ebp;
	_u32 ebx;
	_u32 esi;
	_u32 edi;
	_u32 eflags;
} __attribute__ ((packed)) thread_context_t;

#endif

typedef struct task {
	task_id_t task_id;
	task_state_t status;
	mmc_t *mm;
	page_directory_t *addr_space;
	struct task *parent;
	list_head_t thread_list;
	list_head_t task_list;
} __attribute__ ((packed)) task_t;

typedef struct thread {
	thread_id_t thread_id;
	thread_state_t status;
	thread_context_t context;
	addr_t ustack_base;
	size_t ustack_size;
	addr_t kstack_base;
	size_t kstack_size;
	task_t *task;
	list_head_t thread_list;
} __attribute__ ((packed)) thread_t;

typedef struct {
	tgroup_id_t group_id;
	list_head_t task_list;
} __attribute__ ((packed)) task_group_t;

// intitial task
#define K_INIT init
extern int K_INIT(void *args);
extern void setup_init_task();

extern task_id_t create_task(int (*fn) (void *), void *arg);
extern task_id_t create_kernel_task(int (*fn) (void *), void *arg);
extern thread_id_t create_thread(int (*fn) (void *), void *arg);

#endif
