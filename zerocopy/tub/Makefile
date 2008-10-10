# Makefile
# Makefile for kmemory
# Copyright SLAC, 2008
# Author: Remi Machet

ifneq ($(KERNELRELEASE),)
	# We are inside the kernel
	obj-m := tub.o

else

	KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

all: modules app
	@true

%:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) $@

tub_test: tub_test.c tub.h
	$(CC) -o $@ $<

app: tub_test

.PHONY: app

app_clean:
	rm tub_test

.PHONY: app_clean

endif
