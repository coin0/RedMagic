#ifndef RAMFS_H
#define RAMFS_H

#ifndef DEVICE_H
#error "include device.h instead"
#endif

extern int init_ramfs(dev_t * dev);

#endif
