/*
 *      Red Magic 1996 - 2015
 *
 *      fs.c - filesystem management
 *
 *      2015 Lin Coin - new file to manage filesystems outside cfs
 */

#include "fs.h"
#include "klog.h"
#include "string.h"
#include "heap.h"
#include "list.h"
#include "locking.h"

static fs_list_t fs_info = {
	.file_systems = NULL
};

static mount_table_t mnt_tab;

static filesystem_t *get_fs_list()
{
	return fs_info.file_systems;
}

int register_filesystem(filesystem_t * fs)
{
	filesystem_t *last;

	// add to tail of the list
	mutex_lock(&fs_info.lock);
	last = get_fs_list();
	if (last != NULL) {
		while (last->next) {
			if (!strcmp(last->name, fs->name))
				break;
			last = last->next;
		}
		last->next = fs;
	} else
		fs_info.file_systems = fs;
	fs->next = NULL;
	mutex_unlock(&fs_info.lock);

	return OK;
}

void init_root_fs()
{
	filesystem_t *rootfs;
	blk_dev_t *rootdev;
	int status;

	// initialize kernel fs_type table and mount table
	mutex_init(&fs_info.lock);
	INIT_LIST_HEAD(&mnt_tab.mnt_next);
	mutex_init(&mnt_tab.mtab_lock);

	// the first registered filesystem will be initial rootfs
	// root device is based on the ramfs
	ASSERT(get_fs_list() == NULL);
	// register the first filesystem as rootfs type and use RAM as rootdev
	initfs_cfs();
	if ((rootfs = get_fs_list()) == NULL)
		PANIC("root filesystem not registered");
	rootdev = get_bdev_by_name("ramfs");
	if (rootdev == NULL)
		PANIC("root dev not available");

	// format ramfs area and mount 
	log_info(LOG_FILESYS "formatting ramfs area ...\n");
	status = fs_format(rootdev, rootfs, NULL);
	if (status != OK)
		PANIC("root dev not foramtted");
	if ((status = fs_mount(rootdev, rootfs, "/", NULL)) != OK) {
		log_err(LOG_FILESYS "mount rootfs rv = %d\n", status);
		PANIC("root filesystem not mounted");
	}
}

int fs_format(blk_dev_t * bdev, filesystem_t * fs, kv_option_t * opt)
{
	return fs->format(bdev, opt);
}

int fs_mount(blk_dev_t * bdev, filesystem_t * fs, char *dir_name,
	     kv_option_t * opt)
{
	mount_point_t *mpp;
	int status = OK;

	mutex_lock(&mnt_tab.mtab_lock);

	// check mount table if mountpoints already exits
	// and move to the tail
	list_for_each_entry(mpp, &mnt_tab.mnt_next, mnt_next) {
		if (bdev == mpp->mnt_dev) {
			if (strcmp(dir_name, mpp->mnt_dir) == 0) {
				log_info(LOG_FILESYS
					 "dev 0x%08X is already mounted on %s",
					 bdev, dir_name);
				status = -1;
				goto out;
			}
		}
	}

	// create a new mountpoint
	mpp = (mount_point_t *) kmalloc(sizeof(mount_point_t));
	if (mpp == NULL) {
		status = -2;
		goto out;
	}
	// mount specific filesystem
	mpp->mnt_dev = bdev;
	mpp->mnt_dir = dir_name;
	mpp->fs_type = fs;
	mpp->mnt_flags = 0;
	mpp->mnt_sb = fs->mount(bdev, opt);
	if (mpp->mnt_sb == NULL) {
		kfree(mpp);
		status = -3;
		goto out;
	}
	// parse options to set mount flags
	if (opt != NULL)
		if (kv_option_get(opt, "ro") != NULL)
			mpp->mnt_flags |= MNT_READONLY;

	// add to tail of mount table
	list_add_tail(&mpp->mnt_next, &mnt_tab.mnt_next);

      out:
	mutex_unlock(&mnt_tab.mtab_lock);

	return status;
}

int fs_umount(char *dir_name, kv_option_t * opt)
{
	mount_point_t *mpp;
	int found = 0, status = OK;

	mutex_lock(&mnt_tab.mtab_lock);

	// search mountpoint name "dir_name" in reverse to get the latest 
	// filesystem mounted
	list_for_each_entry_reverse(mpp, &mnt_tab.mnt_next, mnt_next) {
		if (strcmp(dir_name, mpp->mnt_dir) == 0) {
			found = 1;
			break;
		}
	}

	if (!found) {
		log_err(LOG_FILESYS "mountpoint %s not found\n", dir_name);
		status = -1;
		goto out;
	}
	// umount the filesystem and release possible resources
	status = mpp->fs_type->umount(mpp->mnt_sb, opt);
	if (status != OK) {
		status = -2;
		goto out;
	}

	list_del(&mnt_tab.mnt_next);
	kfree(mpp);

      out:
	mutex_unlock(&mnt_tab.mtab_lock);

	return status;
}

// caller need to hold mnt_tab.mtab_lock
super_block_t *mnt_lookup_super_by_bdev(blk_dev_t * bdev)
{
	mount_point_t *mpp;

	list_for_each_entry_reverse(mpp, &mnt_tab.mnt_next, mnt_next)
	    if (mpp->mnt_dev == bdev)
		return mpp->mnt_sb;

	return NULL;
}
