#include "common.h"
#include "cpu.h"
#include "list.h"
#include "string.h"
#include "klog.h"

// defined in mproc.c
extern int init_mp();

cpu_state_t cpuset[MAX_CPUS];

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

void init_application_processor()
{
	init_local_apic();
}

cpu_state_t *get_boot_processor()
{
	int n;

	for (n = 0; n < mpinfo.ncpu; n++)
		if (cpuset[n].flag_bsp)
			return &cpuset[n];

	return NULL;
}

cpu_state_t *get_processor()
{
	int cpu_id, n;

	cpu_id = lapic_get_id();
	for (n = 0; n < mpinfo.ncpu; n++)
		if (cpuset[n].proc_id == cpu_id)
			return &cpuset[n];

	return NULL;
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
