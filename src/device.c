#include "common.h"
#include "device.h"
#include "klog.h"

static int init_ramfs();
static int init_ide();

static dev_list_t devices[] = {
	{"ramfs", init_ramfs, 1, 1},
	{"ide", init_ide, 1, 1},
};

void init_dev()
{
	int status;
	uint_t i, dev_num;

	dev_num = sizeof(devices) / sizeof(dev_list_t);
	for (i = 0; i < dev_num; i++) {
		log_info(LOG_DEV "loading %s ...\n", devices[i].dev_name);
		if ((status = devices[i].init_func()) != OK) {
			log_err(LOG_DEV "%s was not loaded, status %d\n",
				devices[i].dev_name, status);
			continue;

		}
	}
}

static int init_ramfs()
{
	return OK;
}

static int init_ide()
{
	return OK;
}
