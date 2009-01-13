# Makefile
# Makefile for kmemory
# Copyright SLAC, 2008
# Author: Remi Machet

ifneq ($(KERNELRELEASE),)
	# We are inside the kernel
	obj-m := dma_splice.o

else

	KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

all: modules app
	@true

%:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) $@

app: 

.PHONY: app

app_clean:

.PHONY: app_clean

endif
