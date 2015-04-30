#include "common.h"
#include "task.h"
#include "print.h"

int K_INIT(void *args)
{
	printk_color(rc_black, rc_blue, "\n Init Task is now running !\n");
	return 0;
}
