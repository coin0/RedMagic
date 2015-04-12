#!/bin/bash

losetup /dev/loop0 floppy.img
mount /dev/loop0 /mnt

if [[ -e 'src/kernel' ]]; then
	cp src/kernel /mnt/kernel
else
	echo 'kernel image not found.'
fi
umount /dev/loop0
losetup -d /dev/loop0

