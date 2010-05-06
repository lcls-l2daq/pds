# Set UNAME on the command line to change which kernel we're
# installing into or building against.
UNAME := $(shell uname -r)
KERNEL26 := 2.6
KERNELVERSION := $(findstring $(KERNEL26),$(UNAME))
MODULES_DIR ?= /lib/modules/$(UNAME)
INSTALL_DIR := $(MODULES_DIR)/kernel/drivers/pci3e

ifeq ($(KERNELVERSION),2.6)

obj-m	:= pci3e.o

INCLUDE	:= -I/usr/include/asm/mach-default/
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD		:= $(shell pwd)

pci3e::
	$(MAKE) -C $(KDIR) $(INCLUDE) SUBDIRS=$(PWD) modules

else

TARGET	:= pci3e
INCLUDE	:= -I/lib/modules/`uname -r`/build/include -I/usr/include/asm/mach-default/
CFLAGS	:= -O2 -Wall -DMODULE -D__KERNEL__ -DLINUX
CC	:= gcc

pci3e:: ${TARGET}.o

${TARGET}.o: ${TARGET}.c
	$(CC) $(CFLAGS) ${INCLUDE} -c ${TARGET}.c

endif

demo: pci3edemo.c pci3eHelper.c pci3eHelper.h pci3e.h
	gcc -o demo pci3edemo.c pci3eHelper.c -pthread

clean:
	rm *.o demo

install:
	[ -f pci3e.ko ] || { echo Error: Kernel module not present.; exit 1; }
	[ -d $(INSTALL_DIR) ] || sudo mkdir $(INSTALL_DIR)
	sudo cp pci3e.ko $(INSTALL_DIR)
	sudo depmod -a $(UNAME)
