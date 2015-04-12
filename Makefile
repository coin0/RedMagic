
SRC=src
INC=include
SCRIPTS=scripts

CH := $(shell find . -name "*.c" -o -name "*.h")

all: kernel

run:
	./run.sh

build:clean kernel

kernel:
	cd $(SRC) && make

gdb:
	gdb $(SRC)/kernel -x $(SCRIPTS)/gdbinit
kill:
	-pkill qemu

clean: kill
	cd $(SRC) && make clean
	cd $(INC) && make clean
indent:
	for f in $(CH);  \
		do \
		indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 $$f; \
	done

help:
	@echo "make [run|build|gdb|kill|indent]"
