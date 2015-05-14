#include "common.h"
#include "cpu.h"
#include "list.h"
#include "string.h"

static cpu_set_t all_cpus;
static cpu_state_t cpu_1;

static void init_cpu_set(cpu_set_t * set);
static void add_processor(cpu_state_t * cpu, cpu_set_t * set);
static void reset_cpu_state(cpu_state_t * cpu);
static uint_t __get_cpu_num(cpu_set_t * set);

void init_processor()
{
	init_cpu_set(&all_cpus);
	add_processor(&cpu_1, &all_cpus);
}

cpu_state_t *get_processor()
{
	// TODO UP for now
	return CPU_CUR;
}

static uint_t __get_cpu_num(cpu_set_t * set)
{
	return set->cpu_num;
}

uint_t get_cpu_num()
{
	return __get_cpu_num(&all_cpus);
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

static void init_cpu_set(cpu_set_t * set)
{
	bzero(set, sizeof(cpu_set_t));
	set->cpu_num = 0;
	INIT_LIST_HEAD(&set->cpu_list);
}

static void add_processor(cpu_state_t * cpu, cpu_set_t * set)
{
	list_add(&cpu->cpu_list, &set->cpu_list);
	set->cpu_num++;

	reset_cpu_state(cpu);
}

static void reset_cpu_state(cpu_state_t * cpu)
{
	INIT_LIST_HEAD(&cpu->runq);
	spin_lock_init(&cpu->rq_lock);
	preempt_enable();
}
