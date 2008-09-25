# Makefile
# Makefile for kmemory
# Copyright SLAC, 2008
# Author: Remi Machet

ifneq ($(KERNELRELEASE),)
	# We are inside the kernel
	obj-m := kmemory.o

else

	KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

all: modules app
	@true

%:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) $@

kmemory_test: kmemory_test.c kmemory.h
	$(CC) -o $@ $<

app: kmemory_test

.PHONY: app

app_clean:
	rm kmemory_test

.PHONY: app_clean

endif
