#ifndef FILE_H
#define FILE_H

#ifndef FS_H
#error "include fs.h instead"
#endif

#include "common.h"

typedef ushort_t umode_t;
typedef uint_t loff_t;

// replaced with types of specific filesystem
typedef void inode_t;
typedef void file_t;

typedef struct {
} inode_ops_t;

typedef struct {
	int (*open) (inode_t *, file_t *);
	 loff_t(*seek) (file_t *, loff_t, int);
	int (*release) (inode_t *, file_t *);
	 ssize_t(*read) (file_t *, char *, size_t, loff_t *);
	 ssize_t(*write) (file_t *, const char *, size_t, loff_t *);
	int (*ioctl) (inode_t *, file_t *, uint_t, uint_t);
} file_ops_t;

#endif
