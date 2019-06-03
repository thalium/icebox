obj-m += module.o
KVER ?= $(shell uname -r)

all: elf clean

elf: module.ko
	cp module.ko elf

module.ko: module.c
	$(MAKE) -C /lib/modules/$(KVER)/build CONFIG_DEBUG_INFO=y M="$(PWD)" modules

clean:
	$(MAKE) -C /lib/modules/$(KVER)/build M="$(PWD)" clean

