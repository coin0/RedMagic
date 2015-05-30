#include "common.h"
#include "cpu.h"
#include "list.h"
#include "string.h"
#include "klog.h"
#include "mp.h"

// defined in mproc.c
extern int init_mp();

cpu_state_t cpuset[MAX_CPUS];
static size_t cpunum;

void init_bootstrap_processor()
{
	int ap;

	printk("intialize processors ...\n");
	bzero(cpuset, sizeof(cpuset));
	ap = init_mp();
	if (ap < 0) {
		PANIC(LOG_CPU "init_mp: error");
	} else if (ap > 0) {
		init_local_apic();
	}
}

cpu_state_t *get_processor()
{
	return &cpuset[0];
}

size_t get_cpu_count()
{
	return cpunum;
}

void preempt_enable()
{
	cpu_state_t *cpu;
	cpu = get_processor();
	cpu->preempt_on = 1;
}

void preempt_disable()
{
	cpu_state_t *cpu;
	cpu = get_processor();
	cpu->preempt_on = 0;
}

void cpu_reset_state(cpu_state_t * cpu)
{
	INIT_LIST_HEAD(&cpu->runq);
	spin_lock_init(&cpu->rq_lock);
	cpu->flag_bsp = 0;
	cpu->preempt_on = 1;
}
