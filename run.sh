#!/bin/bash

qemu-system-i386 $@ -fda floppy.img -gdb tcp::1234 -smp 2 --curses
