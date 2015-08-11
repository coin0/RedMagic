#ifndef FILE_H
#define FILE_H

#ifndef FS_H
#error "include fs.h instead"
#endif

#include "common.h"

// inode types
typedef enum {
	FTYPE_NULL,
	FTYPE_REG,
	FTYPE_DIR,
	FTYPE_DEV,

	FTYPE_LAST
} ftype_t;

typedef ushort_t umode_t;
typedef uint_t loff_t;

// replaced with types of specific filesystem
typedef void inode_t;

typedef struct {
	inode_t *inode;
	uint_t reference;
} file_t;

typedef struct file_ops {
	int (*open) (inode_t *, file_t *);
	 loff_t(*seek) (inode_t *, loff_t, int);
	int (*release) (inode_t *, file_t *);
	 ssize_t(*read) (inode_t *, char *, size_t, loff_t *);
	 ssize_t(*write) (inode_t *, const char *, size_t, loff_t *);
	int (*ioctl) (inode_t *, file_t *, uint_t, uint_t);
} file_ops_t;

typedef struct {
	uint_t fd;
	file_t *file;
	struct file_ops *fop;
	list_head_t file_list;
} file_list_t;

#endif
