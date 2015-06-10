#ifndef DEVICE_H
#define DEVICE_H

#include "common.h"

typedef struct {
	char *dev_name;
	int (*init_func) (void);
	int onboot;
	int enabled;
} dev_list_t;

extern void init_dev();

#endif
