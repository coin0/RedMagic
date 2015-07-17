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

static fs_list_t fs_info = {
	.file_systems = NULL
};

static mnt_tab_t mnt_tab = {
	.mnt = NULL
};

static inline void lock_fs_list();
static inline void unlock_fs_list();
static filesystem_t *get_fs_list();

static filesystem_t *get_fs_list()
{
	return fs_info.file_systems;
}

int register_filesystem(filesystem_t * fs)
{
	filesystem_t *last;

	// add to tail of the list
	lock_fs_list();
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
	unlock_fs_list();

	return OK;
}

static inline void lock_fs_list()
{
	mutex_lock(&fs_info.lock);
}

static inline void unlock_fs_list()
{
	mutex_unlock(&fs_info.lock);
}

void init_root_fs()
{
	filesystem_t *rootfs;
	blk_dev_t *rootdev;
	int status;

	mutex_init(&fs_info.lock);

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
	status = do_format(rootdev, rootfs, NULL);
	if (status != OK)
		PANIC("root dev not foramtted");
	// TODO coin test
	while (1) ;
	// ....
	if ((status = do_mount(rootdev, rootfs, "/", 0, NULL)) != OK) {
		log_err(LOG_FILESYS "mount rootfs rv = %d\n", status);
		PANIC("root filesystem not mounted");
	}
}

int do_format(blk_dev_t * bdev, filesystem_t * fs, void *opts)
{
	return fs->format(bdev, opts);
}

int do_mount(blk_dev_t * bdev, filesystem_t * fs, const char *dir_name,
	     int flags, void *opts)
{
	return OK;
}
