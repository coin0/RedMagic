# Makefile for JamesM's kernel tutorials.
# The C and C++ rules are already setup by default.
# The only one that needs changing is the assembler 
# rule, as we use nasm instead of GNU as.


C_OBJ=$(patsubst %.c,%.o,$(wildcard *.c))
S_OBJ=$(patsubst %.s,%.o,$(wildcard *.s))
SOURCES=$(S_OBJ) $(C_OBJ)
BIN=kernel

CFLAGS=-I../include/ -Wall -Wno-main -m32 -g -nostdlib -nostdinc -fno-builtin -fno-stack-protector
LDFLAGS=-Tlink.ld -melf_i386
ASFLAGS=-I../include/ -g -felf

all: $(SOURCES) link

clean:
	-rm *.o $(BIN) *~

link:
	@echo "LD " $(BIN) 
	@ld $(LDFLAGS) -o $(BIN) $(SOURCES)

.c.o:
	@echo "CC " $<
	@gcc $(CFLAGS) -c $<
.s.o:
	@echo "ASM" $<
	@nasm $(ASFLAGS) $<

indent:
kill:
	-pkill qemu
