#include "common.h"
#include "cpu.h"

static cpu_state_t cpu_1;

static void reset_cpu_state(cpu_state_t * cpu);

void init_processor()
{
	reset_cpu_state(&cpu_1);
}

cpu_state_t *get_processor()
{
	// TODO unique processor for now
	return CPU_CUR;
}

static void reset_cpu_state(cpu_state_t * cpu)
{
	INIT_LIST_HEAD(&cpu->runq);
}
