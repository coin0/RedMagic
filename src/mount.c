/*
 *      Red Magic 1996 - 2015
 *
 *      mount.c - mount table, mountpoint, and mount utilities
 *
 *      2015 Lin Coin - initial version
 */

#include "common.h"
#include "mount.h"
#include "locking.h"
#include "string.h"

static mount_table_t mnt_tab;

void init_mtab()
{
	INIT_LIST_HEAD(&mnt_tab.mnt_next);
	mutex_init(&mnt_tab.mtab_lock);
}

void mtab_lock_read()
{
}

void mtab_lock_write()
{
}

void mtab_unlock_read()
{
}

void mtab_unlock_write()
{
}

void add_to_mtab(mount_point_t * mpp)
{
	list_add_tail(&mpp->mnt_next, &mnt_tab.mnt_next);
}

void del_from_mtab(mount_point_t * mpp)
{
	list_del(&mpp->mnt_next);
}

int check_dup_mountpoint(blk_dev_t * bdev, char *dir_name)
{
	mount_point_t *mpp;

	list_for_each_entry(mpp, &mnt_tab.mnt_next, mnt_next) {
		if (bdev == mpp->mnt_dev && strcmp(dir_name, mpp->mnt_dir) == 0)
			return -1;
	}

	return OK;
}

mount_point_t *lookup_mountpoint_by_dir(char *dir_name)
{
	mount_point_t *mpp;

	list_for_each_entry_reverse(mpp, &mnt_tab.mnt_next, mnt_next) {
		if (strcmp(dir_name, mpp->mnt_dir) == 0) {
			return mpp;
		}
	}

	return NULL;
}

// caller need to hold mnt_tab.mtab_lock
super_block_t *lookup_super_by_bdev(blk_dev_t * bdev)
{
	mount_point_t *mpp;

	list_for_each_entry_reverse(mpp, &mnt_tab.mnt_next, mnt_next)
	    if (mpp->mnt_dev == bdev)
		return mpp->mnt_sb;

	return NULL;
}
