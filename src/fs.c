/*
 *      Red Magic 1996 - 2015
 *
 *      fs.c - filesystem management
 *
 *      2015 Lin Coin - new file to manage filesystems outside cfs
 */

#include "fs.h"

filesystem_t *file_systems = NULL;

int register_filesystem(filesystem_t * fs)
{
	// coin front

	return OK;
}

int init_root_fs()
{
	initfs_cfs();

	return OK;
}
