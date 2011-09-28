# $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/Makefile $
# $Rev: 9231 $
# $Date$

# Makefile for the 16AI32SSC driver.

OBJ_FILES	=					\
			close.o				\
			device.o			\
			io.o				\
			ioctl.o				\
			irq.o				\
			open.o				\
			read.o				\
			reg.o				\
								\
			gsc_bar.o			\
			gsc_close.o			\
			gsc_dma.o			\
			gsc_init.o			\
			gsc_io.o			\
			gsc_ioctl.o			\
			gsc_irq.o			\
			gsc_kernel_2_2.o	\
			gsc_kernel_2_4.o	\
			gsc_kernel_2_6.o	\
			gsc_open.o			\
			gsc_proc.o			\
			gsc_read.o			\
			gsc_reg.o			\
			gsc_sem.o			\
			gsc_time.o			\
			gsc_wait.o			\
			gsc_write.o

GSC_SOURCES	=					\
			./gsc_makefile.inc	\
			./gsc_bar.c			\
			./gsc_close.c		\
			./gsc_dma.c			\
			./gsc_common.h		\
			./gsc_init.c		\
			./gsc_io.c			\
			./gsc_ioctl.c		\
			./gsc_irq.c			\
			./gsc_kernel_2_2.c	\
			./gsc_kernel_2_2.h	\
			./gsc_kernel_2_4.c	\
			./gsc_kernel_2_4.h	\
			./gsc_kernel_2_6.c	\
			./gsc_kernel_2_6.h	\
			./gsc_main.h		\
			./gsc_pci9056.h		\
			./gsc_pci9080.h		\
			./gsc_open.c		\
			./gsc_proc.c		\
			./gsc_read.c		\
			./gsc_reg.c			\
			./gsc_sem.c			\
			./gsc_time.c		\
			./gsc_wait.c		\
			./gsc_write.c



TARGET		:= 16ai32ssc

default: all

###############################################################################
# Everything here is needed so we can access gsc_makefile.inc.

.EXPORT_ALL_VARIABLES:

# Locate the source from the starting directory.
ifndef GSC_DEV_DIR
GSC_DEV_DIR	:= ${PWD}
endif

# Locate the GSC common sources.
ifndef GSC_DIR

GSC_COMMON	:= $(shell ls -d ../../../gsc_common/linux/driver 2>/dev/null | wc -l )

ifeq ("${GSC_COMMON}","1")

GSC_DIR		:= ${GSC_DEV_DIR}/../../../gsc_common/linux/driver

${GSC_DEV_DIR}/gsc_makefile.inc: ${GSC_DIR}/gsc_makefile.inc
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

else
GSC_DIR		:= ${GSC_DEV_DIR}
endif

endif

#include ${PWD}/gsc_makefile.inc
include ${GSC_DEV_DIR}/gsc_makefile.inc

