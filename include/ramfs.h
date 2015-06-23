#ifndef RAMFS_H
#define RAMFS_H

#ifndef DEVICE_H
#error "include device.h instead"
#endif

#include "common.h"
#include "locking.h"
#include "list.h"

typedef struct {
	list_head_t rw;
	char *ramhead;
	mutex_t ramlock;
} dev_ramfs_t;

extern int init_ramfs(dev_t * dev);

#endif
